/*
 * Copyright 2018-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <string.h>
#include <time.h>

/* Board includes */
#include "board.h"

#include "ais_alerts.h"
#include "ais_streamer.h"
#include "aisv2_app.h"
#include "amazon_wake_word.h"
#include "network_connection.h"
#include "sln_flash.h"
#include "sln_flash_mgmt.h"
#include "sln_cfg_file.h"
#include "streamer_pcm.h"
#include "wifi_credentials.h"

#include "sln_reset.h"
#include "sln_convert.h"

#ifdef FFS_ENABLED
#include "factory_reset_api.h"
#endif

/*! @brief Values for flow control buffer states. */
const char *ais_buffer_state_str[] = {
    "AIS_BUFFER_STATE_GOOD,",     "AIS_BUFFER_STATE_OVERRUN,",          "AIS_BUFFER_STATE_OVERRUN_WARNING,",
    "AIS_BUFFER_STATE_UNDERRUN,", "AIS_BUFFER_STATE_UNDERRUN_WARNING,", "AIS_BUFFER_STATE_INVALID"};

const char *ais_state_str[] = {"AIS_STATE_IDLE", "AIS_STATE_THINKING", "AIS_STATE_SPEAKING", "AIS_STATE_ALERTING",
                               "AIS_STATE_INVALID"};

#define TICK_THRESHOLD (portTICK_PERIOD_MS * 50)

#include "audio_processing_task.h"
#include "limits.h"
#include "reconnection_task.h"
#include "task.h"
#include "ux_attention_system.h"
#include "sln_RT10xx_RGB_LED_driver.h"
#include "app_events.h"

static QueueHandle_t s_appEventQueue;
extern TaskHandle_t xUXAttentionTaskHandle;

static uint8_t *pu8PreambleData;
static uint32_t u32PreambleIdx  = 0;
static uint32_t u32PreambleSize = 0;

static TaskHandle_t s_alertsTask;
extern QueueHandle_t g_alertQueue;

TimerHandle_t s_thinkStateTimer = NULL;

__attribute__((section(".ocram_non_cacheable_bss"))) ais_app_data_t appData;

static void vThinkStateTimerCallback(TimerHandle_t xTimer);

void AIS_AppInit(void)
{
    appData.state                    = AIS_STATE_IDLE;
    appData.prevState                = AIS_STATE_IDLE;
    appData.currTime                 = 0;
    appData.prevTime                 = 0;
    appData.lastRefresh              = 0;
    appData.speakerOffsetStart       = 0;
    appData.speakerOffsetEnd         = 0;
    appData.speakerOffsetWritten     = 0;
    appData.speakerOpusFramesFlushed = 0;
    appData.overrunSequence          = 0;
    appData.bargein                  = false;
    appData.speakerOpen              = false;
    appData.speakerOpenSent          = false;
    appData.speakerBufferState       = AIS_BUFFER_STATE_UNDERRUN_WARNING;
    appData.underrunWarningTime      = 0;
    appData.overrunWarningTime       = 0;
    memset(&(appData.speakerMarker), 0, AIS_APP_MAX_SPEAKER_MARKERS * sizeof(echoMarker_t));

    s_alertsTask = AIS_Alerts_GetThisHandle();

    if (!s_thinkStateTimer)
    {
        s_thinkStateTimer = xTimerCreate("ThinkStateTimer",                               /* name */
                                         pdMS_TO_TICKS(AIS_APP_THINK_STATE_TIMEOUT_MSEC), /* period */
                                         pdFALSE,                                         /* self reload */
                                         (void *)0,                                       /* id */
                                         vThinkStateTimerCallback /* function to call when expiring */
        );

        if (!s_thinkStateTimer)
        {
            configPRINTF(("xTimerCreate failed\r\n"));
        }
    }
}

void AIS_APP_set_queue_handle(QueueHandle_t *handle)
{
    if ((NULL != handle) && (NULL != *handle))
    {
        s_appEventQueue = *handle;
    }
}

uint32_t AIS_AppCallback_Microphone(uint8_t **data, uint32_t *size)
{
    uint32_t ret = kStatus_NoTransferInProgress;

    // Get diff between current tick and previous tick; treated as signed to account for overflow
    int32_t diff = (int32_t)xTaskGetTickCount() - (int32_t)appData.previousTickTime;

    if (TICK_THRESHOLD <= diff)
    {
        if (kMicCloudWakeVerifier == audio_processing_get_state())
        {
            ret = audio_processing_get_continuous_utterance(&pu8PreambleData, &u32PreambleSize, &u32PreambleIdx, size,
                                                            data);
        }
        else if (kMicRecording == audio_processing_get_state())
        {
            /* Needs to be an if else or the last of the wake word will be overwritten */
            ret = audio_processing_get_output_buffer(data, size);
        }

        // Store this publish time
        appData.previousTickTime = xTaskGetTickCount();
    }

    return ret;
}

static void _SpeakerEchoCheck(ais_handle_t *handle, echoMarker_t *marker)
{
    if ((NULL != handle) && (NULL != marker))
    {
        if (marker->echoRequest && STREAMER_IsPlaying((streamer_handle_t *)handle->audioPlayer))
        {
            streamer_handle_t *streamer = (streamer_handle_t *)handle->audioPlayer;
            uint32_t buffered           = STREAMER_GetQueuedRaw(streamer);
            uint64_t offsetReached      = appData.speakerOffsetWritten - buffered;

            /* Send event when we have played back this data. */
            if (offsetReached >= marker->offset)
            {
                AIS_EventSpeakerMarkerEncountered(handle, marker->marker);
                marker->echoRequest = false;
            }
        }
    }
}

static void _SpeakerEchoMarkers(ais_handle_t *handle)
{
    if (NULL != handle)
    {
        uint8_t marker_it = 0;
        for (marker_it = 0; marker_it < AIS_APP_MAX_SPEAKER_MARKERS; marker_it++)
        {
            _SpeakerEchoCheck(handle, &(appData.speakerMarker[marker_it]));
        }
    }
}

/*!
 * @brief Handle the transition to a new speaker buffer state. Optionally send a BufferStateChanged
 *        event to the service, otherwise just print some info regarding the transition.
 *
 * @param *handle    Reference to current ais_handle_t in use
 * @param newState   The new state into which the transition is made
 * @param sendEvent  When true, a BufferStateChanged event is sent, when false a log is printed
 * @param buffered   The number of bytes stored in the speaker ring buffer
 *
 * @return void
 */
static void _SpeakerBufferStateTransition(ais_handle_t *handle,
                                          ais_buffer_state_t newState,
                                          bool sendEvent,
                                          uint32_t buffered)
{
    uint32_t lastSeq = handle->topicSequence[AIS_TOPIC_SPEAKER] - 1;

    appData.prevSpeakerBufferState = appData.speakerBufferState;
    appData.speakerBufferState     = newState;

    if (sendEvent)
    {
        AIS_EventBufferStateChanged(handle, appData.prevSpeakerBufferState, appData.speakerBufferState, lastSeq);
    }
    else
    {
        configPRINTF(("BufferStateChanged, old: %s new: %s buff: %d\r\n",
                      AIS_MapBufferState(appData.prevSpeakerBufferState),
                      AIS_MapBufferState(appData.speakerBufferState), buffered));
    }
}

/*!
 * @brief Check if a speaker buffer state transition into state OVERRUN_WARNING is needed.
 *        If the transition is made from an inferior state (UNDERRUN, UNDERRUN_WARNING, GOOD),
 *        a BufferStateChanged event is sent.
 *
 * @param *handle    Reference to current ais_handle_t in use
 * @param buffered   The number of bytes stored in the speaker ring buffer
 *
 * @return bool      True if buffered met the threshold conditions, false otherwise. When threshold
 *                   conditions are met, there is no need to verify other potential state transitions.
 */
static bool _OverrunWarningThresholdCheck(ais_handle_t *handle, uint32_t buffered)
{
    bool ret = false;

    if ((buffered >= AWS_AUDIO_BUFFER_OVERRUN_THRESHOLD) && (buffered < AWS_AUDIO_BUFFER_SIZE))
    {
        /* Transition to OVERRUN_WARNING from an inferior state */
        if ((appData.speakerBufferState == AIS_BUFFER_STATE_UNDERRUN) ||
            (appData.speakerBufferState == AIS_BUFFER_STATE_UNDERRUN_WARNING) ||
            (appData.speakerBufferState == AIS_BUFFER_STATE_GOOD))
        {
            /* Get diff between current tick and previous tick; treated as signed to account for overflow */
            int32_t diff = (int32_t)xTaskGetTickCount() - (int32_t)appData.overrunWarningTime;

            if ((diff * portTICK_PERIOD_MS) > AIS_APP_WARNINGS_THRESHOLD_MSEC)
            {
                /* BufferStateChanged event is sent */
                _SpeakerBufferStateTransition(handle, AIS_BUFFER_STATE_OVERRUN_WARNING, true, buffered);

                /* Update time stamp */
                appData.overrunWarningTime = xTaskGetTickCount();
            }
        }
        else
            /* Transition to OVERRUN_WARNING from a superior state */
            if (appData.speakerBufferState == AIS_BUFFER_STATE_OVERRUN)
        {
            /* BufferStateChanged event is sent to cloud only when the
             * transition to OVERRUN_WARNING happens from an inferior state */
            _SpeakerBufferStateTransition(handle, AIS_BUFFER_STATE_OVERRUN_WARNING, false, buffered);
        }

        /*  Return true when threshold conditions are met */
        ret = true;
    }

    return ret;
}

/*!
 * @brief Check if a speaker buffer state transition into state UNDERRUN_WARNING is needed.
 *        If the transition is made from a superior state (OVERRUN, OVERRUN_WARNING, GOOD),
 *        a BufferStateChanged event is sent.
 *
 * @param *handle    Reference to current ais_handle_t in use
 * @param buffered   The number of bytes stored in the speaker ring buffer
 *
 * @return bool      True if buffered met the threshold conditions, false otherwise. When threshold
 *                   conditions are met, there is no need to verify other potential state transitions.
 */
static bool _UnderrunWarningThresholdCheck(ais_handle_t *handle, uint32_t buffered)
{
    bool ret = false;

    if ((buffered < AWS_AUDIO_BUFFER_UNDERRUN_THRESHOLD) && (buffered > 0))
    {
        /* Transition to UNDERRUN_WARNING from a superior state */
        if ((appData.speakerBufferState == AIS_BUFFER_STATE_OVERRUN) ||
            (appData.speakerBufferState == AIS_BUFFER_STATE_OVERRUN_WARNING) ||
            (appData.speakerBufferState == AIS_BUFFER_STATE_GOOD))
        {
            /* Get diff between current tick and previous tick; treated as signed to account for overflow */
            int32_t diff = (int32_t)xTaskGetTickCount() - (int32_t)appData.underrunWarningTime;

            if ((diff * portTICK_PERIOD_MS) > AIS_APP_WARNINGS_THRESHOLD_MSEC)
            {
                /* BufferStateChanged event is sent */
                _SpeakerBufferStateTransition(handle, AIS_BUFFER_STATE_UNDERRUN_WARNING, true, buffered);

                /* Update time stamp */
                appData.underrunWarningTime = xTaskGetTickCount();
            }
        }
        else
            /* Transition to UNDERRUN_WARNING from an inferior state */
            if (appData.speakerBufferState == AIS_BUFFER_STATE_UNDERRUN)
        {
            /* BufferStateChanged event is sent to cloud only when the
             * transition to UNDERRUN_WARNING happens from a superior state */
            _SpeakerBufferStateTransition(handle, AIS_BUFFER_STATE_UNDERRUN_WARNING, false, buffered);
        }

        /*  Return true when threshold conditions are met */
        ret = true;
    }

    return ret;
}

/*!
 * @brief Check if a speaker buffer state transition into state GOOD is needed.
 *        A BufferStateChanged event is NOT sent, no matter what the previous state was.
 *
 * @param *handle    Reference to current ais_handle_t in use
 * @param buffered   The number of bytes stored in the speaker ring buffer
 *
 * @return bool      True if buffered met the threshold conditions, false otherwise. When threshold
 *                   conditions are met, there is no need to verify other potential state transitions.
 */
static bool _GoodBufferStateCheck(ais_handle_t *handle, uint32_t buffered)
{
    bool ret = false;

    if ((buffered >= AWS_AUDIO_BUFFER_UNDERRUN_THRESHOLD) && (buffered < AWS_AUDIO_BUFFER_OVERRUN_THRESHOLD))
    {
        if (appData.speakerBufferState != AIS_BUFFER_STATE_GOOD)
        {
            /* BufferStateChanged event is NEVER sent when transitioning to GOOD state */
            _SpeakerBufferStateTransition(handle, AIS_BUFFER_STATE_GOOD, false, buffered);
        }

        /*  Return true when threshold conditions are met */
        ret = true;
    }

    return ret;
}

/*!
 * @brief Check if a speaker buffer state transition into state UNDERRUN is needed.
 *        A BufferStateChanged event is always sent, unless the previous state was UNDERRUN.
 *
 * @param *handle    Reference to current ais_handle_t in use
 * @param buffered   The number of bytes stored in the speaker ring buffer
 *
 * @return bool      True if buffered met the threshold conditions, false otherwise. When threshold
 *                   conditions are met, there is no need to verify other potential state transitions.
 */
static bool _UnderrunBufferStateCheck(ais_handle_t *handle, uint32_t buffered)
{
    bool ret = false;

    if (buffered == 0)
    {
        /* Transition to UNDERRUN, always from a superior state */
        if (appData.speakerBufferState != AIS_BUFFER_STATE_UNDERRUN)
        {
            /* BufferStateChanged event is sent */
            _SpeakerBufferStateTransition(handle, AIS_BUFFER_STATE_UNDERRUN, true, buffered);
        }

        /*  Return true when threshold conditions are met */
        ret = true;
    }

    return ret;
}

static void _SpeakerFlowControl(ais_handle_t *handle)
{
    streamer_handle_t *streamer = (streamer_handle_t *)handle->audioPlayer;
    uint32_t buffered           = STREAMER_GetQueued(streamer);
    bool threshold_check_ret    = false;

    if (STREAMER_IsPlaying(streamer) && appData.speakerOpenSent)
    {
        /* If we received all data for the current playback, don't check
         * for speaker buffer state transitions. The service already finished sending
         * all the data, there is no reason to regulate the flow anymore */
        bool dataComplete = false;
        if ((appData.speakerOffsetEnd > 0) && (appData.speakerOffsetWritten >= appData.speakerOffsetEnd))
        {
            dataComplete = true;
        }

        if (false == dataComplete)
        {
            /* Based on the value of buffered, we can only be in one of the cases
             * below. Using threshold_check_ret to detect when one of the scenarios was met,
             * in order to skip verifying the other scenarios afterwards */

            /* NOTE: transition into OVERRUN is handled by AIS_AppCallback_SpeakerOverflow! */

            /* CASE 1: nothing is buffered, check if UNDERRUN transition needed */
            threshold_check_ret = _UnderrunBufferStateCheck(handle, buffered);

            /* CASE 2: below underrun threshold, check if UNDERRUN_WARNING transition needed */
            if (false == threshold_check_ret)
            {
                threshold_check_ret = _UnderrunWarningThresholdCheck(handle, buffered);
            }

            /* CASE 3: above underrun, below overrun thresholds, check if GOOD state transition needed */
            if (false == threshold_check_ret)
            {
                threshold_check_ret = _GoodBufferStateCheck(handle, buffered);
            }

            /* CASE 4: above overrun threshold, check if OVERRUN_WARNING transition needed */
            if (false == threshold_check_ret)
            {
                threshold_check_ret = _OverrunWarningThresholdCheck(handle, buffered);
            }
        }
    }
    else /* streamer not playing */
    {
        if (buffered >= AWS_AUDIO_BUFFER_OVERRUN_THRESHOLD)
        {
            /* If we are in barge-in state, pull the number of bytes and throw half of them away if overflow,
             * no matter what the speaker buffer state is */
            if ((audio_processing_get_state() == kMicRecording))
            {
                uint32_t queued = STREAMER_GetQueued(streamer);

                uint32_t numberOfFrames = (queued / STREAMER_PCM_OPUS_FRAME_SIZE);
                uint32_t bytesToDrop    = ((numberOfFrames / 2) * STREAMER_PCM_OPUS_FRAME_SIZE);

                STREAMER_Read(NULL, bytesToDrop);
            }
        }
        else
            /* Diligently update the speaker buffer state below, without
             * sending a BufferStateChanged event */
            if (buffered < AWS_AUDIO_BUFFER_UNDERRUN_THRESHOLD)
        {
            if (appData.speakerBufferState != AIS_BUFFER_STATE_UNDERRUN_WARNING)
            {
                _SpeakerBufferStateTransition(handle, AIS_BUFFER_STATE_UNDERRUN_WARNING, false, buffered);
            }
        }
        else
        {
            if (appData.speakerBufferState != AIS_BUFFER_STATE_GOOD)
            {
                _SpeakerBufferStateTransition(handle, AIS_BUFFER_STATE_GOOD, false, buffered);
            }
        }
    }
}

static void _SpeakerFlushedKeepTrack(uint32_t dataFlushed)
{
    configPRINTF(("Flushed %lu bytes from the streamer!\r\n", dataFlushed));

    /* Remember what we flushed to use info at next interaction in OpenSpeaker */
    appData.speakerOpusFramesFlushed = dataFlushed / STREAMER_PCM_OPUS_FRAME_SIZE;

    /* Subtract the offset by the remaining data flushed from the streamer *
     * This is to ensure the offsets are in sync as the data we just flushed
     * from the streamer hasn't been played */
    appData.speakerOffsetWritten -= (appData.speakerOpusFramesFlushed * STREAMER_PCM_OPUS_DATA_SIZE);
}

static void _SpeakerStop(ais_handle_t *handle)
{
    streamer_handle_t *streamer = (streamer_handle_t *)handle->audioPlayer;

    /* Check for EOS from the streamer. */
    if (streamer->eos)
    {
        streamer->eos = false;
        /* Stop the streamer */
        configPRINTF(("[AIS App] Stopping streamer playback\r\n"));
        STREAMER_Stop(streamer);

        /* Send SpeakerClosed event response if we have safely finished */
        if ((appData.speakerOffsetEnd > 0) && (appData.speakerOffsetWritten >= appData.speakerOffsetEnd))
        {
            appData.speakerOpen     = false;
            appData.speakerOpenSent = false;
            AIS_EventSpeakerClosed(handle, appData.speakerOffsetWritten);

            configPRINTF(("[AIS App] Exiting Speaker State\r\n"));
            AIS_ClearState(handle, AIS_TASK_STATE_SPEAKER);

            /* Reset streamer playback state. */
            appData.speakerBufferState = AIS_BUFFER_STATE_UNDERRUN_WARNING;

            /* Return to default CPU speed when Idle */
            BOARD_RevertClock();
        }
    }
    else if (appData.bargein && appData.speakerOpen)
    {
        if (STREAMER_IsPlaying(streamer))
        {
            /* Stop the streamer */
            configPRINTF(("[AIS App] Stopping streamer playback.\r\n"));

            uint32_t flushedSize = STREAMER_Stop(streamer);
            if (flushedSize > 0)
            {
                /* Save offsets to use on next interaction */
                _SpeakerFlushedKeepTrack(flushedSize);
            }
        }

        /* Update the speaker flag and offsets */
        appData.bargein         = false;
        appData.speakerOpen     = false;
        appData.speakerOpenSent = false;

        if (appData.speakerOffsetWritten == appData.speakerOffsetStart)
        {
            /* TODO remove - add one for the good faith */
            appData.speakerOffsetWritten++;
        }
        appData.speakerOffsetEnd = appData.speakerOffsetWritten;

        /* Send SpeakerClosed event immediately */
        AIS_EventSpeakerClosed(handle, appData.speakerOffsetWritten);
    }
}

static void _SpeakerStart(ais_handle_t *handle)
{
    streamer_handle_t *streamer = (streamer_handle_t *)handle->audioPlayer;
    uint32_t totalExpected      = 0;

    if (appData.speakerOpen)
    {
        if (appData.speakerOffsetEnd > 0)
        {
            /* Calculate total amount of bytes received in this audio stream. */
            totalExpected = appData.speakerOffsetEnd - appData.speakerOffsetStart;
        }

        /* Check for conditions to begin streamer playback. */
        if (!STREAMER_IsPlaying(streamer) && (audio_processing_get_state() == kIdle))
        {
            uint32_t buffered = STREAMER_GetQueued(streamer);

            bool startThresholdMet = buffered > AWS_AUDIO_START_THRESHOLD;
            bool allDataCollected  = totalExpected && (buffered >= totalExpected);
            bool endOffsetMet =
                (appData.speakerOffsetEnd > 0) && (appData.speakerOffsetWritten >= appData.speakerOffsetEnd);

            if (startThresholdMet || /* Network buffer threshold has been exceeded */
                allDataCollected ||  /* All data collected already in the buffer */
                endOffsetMet)        /* All data received, based on offsets */
            {
                configPRINTF(("[AIS App] Starting streamer playback.\r\n"));
                STREAMER_Start(streamer);
                STREAMER_SetVolume(appData.volume);

                /* Avoid sending multiple SpeakerOpened messages during a playback */
                if (!appData.speakerOpenSent)
                {
                    /* Send speaker opened message with the start offset of playback */
                    AIS_EventSpeakerOpened(handle, appData.speakerOffsetStart);
                    appData.speakerOpenSent = true;
                }
            }
        }
    }
}

static void _SpeakerOpenTimeout(ais_handle_t *handle)
{
    ais_app_data_t *appData = AIS_APP_GetAppData();
    TickType_t *timer       = &handle->speakerOpenTimer;

    /* Check to see if we received SpeakerOpen, but no speaker messages and the timeout for this expired */
    if ((appData->speakerOpen == true) && (*timer != 0))
    {
        if (AIS_TimeoutCheck(*timer, AIS_MSG_OPEN_SPEAKER_TIMEOUT_MSEC))
        {
            /* Timeout: disconnect from service */
            if (reconnection_task_set_event(kReconnectAISDisconnect))
            {
                configPRINTF(("OpenSpeaker timeout, did not receive a speaker msg for %d sec\r\n",
                              AIS_MSG_OPEN_SPEAKER_TIMEOUT_MSEC / 1000));
                *timer = 0;
            }
        }
    }
}

void AIS_AppCallback_SpeakerState(ais_handle_t *handle)
{
    AIS_State_Lock(handle);

    /* Check marker for sending SpeakerMarkerEncountered. */
    _SpeakerEchoMarkers(handle);

    /* Check for audio buffer over/underrun issues. */
    _SpeakerFlowControl(handle);

    /* Check if playback is stopped or needs to - due to EOS or barge-in */
    _SpeakerStop(handle);

    /* Check if playback needs to start */
    _SpeakerStart(handle);

    /* Check if SpeakerOpen timeout reached */
    _SpeakerOpenTimeout(handle);

    AIS_State_Unlock(handle);
}

/* Return available audio buffer space. */
uint32_t AIS_AppCallback_SpeakerAvailableBuffer(ais_handle_t *handle)
{
    streamer_handle_t *streamer = (streamer_handle_t *)handle->audioPlayer;
    uint32_t buffered;

    buffered = STREAMER_GetQueued(streamer);

    return (AWS_AUDIO_BUFFER_SIZE - buffered);
}

void AIS_AppCallback_SpeakerOverflow(ais_handle_t *handle, uint32_t sequence)
{
    streamer_handle_t *streamer = (streamer_handle_t *)handle->audioPlayer;

    AIS_State_Lock(handle);

    configPRINTF(("[AIS App] Speaker overflow detected, sequence: %d\r\n", sequence));

    /* Do not send an OVERRUN event if we're still receiving speaker messages after the speaker was closed */
    if (appData.speakerOpen == true)
    {
        /* Need to send Overflow regardless of the previous state
         * Buffered parameter does not matter for this call */
        _SpeakerBufferStateTransition(handle, AIS_BUFFER_STATE_OVERRUN, true, 0);

        appData.overrunSequence = sequence;
    }
    else
    {
        handle->topicSequence[AIS_TOPIC_SPEAKER]++;
    }

    if (!STREAMER_IsPlaying(streamer))
    {
        uint32_t queued = STREAMER_GetQueued(streamer);

        uint32_t numberOfFrames = (queued / STREAMER_PCM_OPUS_FRAME_SIZE);
        uint32_t bytesToDrop    = ((numberOfFrames / 2) * STREAMER_PCM_OPUS_FRAME_SIZE);

        STREAMER_Read(NULL, bytesToDrop);
    }

    AIS_State_Unlock(handle);
}

/* When this function is executed, there is enough space to buffer the data. */
void AIS_AppCallback_Speaker(ais_handle_t *handle, uint8_t *data, uint32_t size, uint64_t offset, uint8_t count)
{
    /* TODO: don't write data to the streamer if the offset is before the
     * OpenSpeaker value.  Need to buffer 3-5 message sequences for reorder and
     * throwaway detection. */

    uint32_t currentOffset = 0;
    uint32_t frameSize     = 0;
    uint32_t currentCount  = 0;

    AIS_State_Lock(handle);

    /* While there are still OPUS frames to process, keep pushing into the streamer with size */
    while (count >= currentCount)
    {
        /* Shift the offset to start processing the next frame */
        currentOffset += frameSize;
        frameSize = (size - sizeof(offset)) / (count + 1);

        if (handle->config->speakerDecoder == AIS_SPEAKER_DECODER_OPUS)
        {
            /* Write the size of the packet to the streamer. */
            STREAMER_Write((uint8_t *)&frameSize, sizeof(uint32_t));
        }

        /* Write the actual data shifted with the next offset
         * The frame size is normally 160 bytes but the count divisor should figure this out */
        STREAMER_Write(data + currentOffset, frameSize);

        /* Shift the new offset with the size we just wrote */
        offset += frameSize;

        /* Increment the count to indicate we processed this frame */
        currentCount++;
    }

    appData.speakerOffsetWritten = offset;

    AIS_State_Unlock(handle);
}

void AIS_AppCallback_SpeakerMarker(ais_handle_t *handle, uint32_t marker)
{
    configPRINTF(("[AIS App] Speaker Marker Received: %d\r\n", marker));

    AIS_State_Lock(handle);

    uint8_t marker_it            = 0;
    echoMarker_t *current_marker = NULL;
    echoMarker_t swapping_value;
    int i, j;

    for (marker_it = 0; marker_it < AIS_APP_MAX_SPEAKER_MARKERS; marker_it++)
    {
        current_marker = &(appData.speakerMarker[marker_it]);

        if (current_marker->echoRequest == false)
        {
            break;
        }
    }

    /* If no free marker found, should not happen,(echoRequest != false), overwrite the last entry in the list */
    if (AIS_APP_MAX_SPEAKER_MARKERS == marker_it)
    {
        marker_it = AIS_APP_MAX_SPEAKER_MARKERS - 1;
    }

    current_marker = &(appData.speakerMarker[marker_it]);

    /* Insertion of new received marker */
    current_marker->marker      = marker;
    current_marker->offset      = appData.speakerOffsetWritten;
    current_marker->echoRequest = true;

    /* Keep the list of marker sorted in increasing order,
     *  implementation of standard bubble sort */

    for (i = 0; i < AIS_APP_MAX_SPEAKER_MARKERS - 1; i++)
    {
        for (j = 0; j < AIS_APP_MAX_SPEAKER_MARKERS - i - 1; j++)
        {
            if (appData.speakerMarker[j].marker > appData.speakerMarker[j + 1].marker)
            {
                swapping_value               = appData.speakerMarker[j];
                appData.speakerMarker[j]     = appData.speakerMarker[j + 1];
                appData.speakerMarker[j + 1] = swapping_value;
            }
        }
    }

    AIS_State_Unlock(handle);
}

void AIS_AppCallback_OpenSpeaker(ais_handle_t *handle, uint64_t offset)
{
    char long_buf[LONG_STR_BUFSIZE]   = {0};
    int64_t bytesToDrop               = 0;
    uint64_t u64SpeakerStreamerOffset = ((offset / STREAMER_PCM_OPUS_DATA_SIZE) * STREAMER_PCM_OPUS_FRAME_SIZE);
    uint64_t u64SpeakerStreamerEnd =
        ((appData.speakerOffsetEnd / STREAMER_PCM_OPUS_DATA_SIZE) * STREAMER_PCM_OPUS_FRAME_SIZE);
    uint32_t u32SpeakerFlushedSize = appData.speakerOpusFramesFlushed * STREAMER_PCM_OPUS_FRAME_SIZE;

    /* Discard any data prior to 'offset'. Also consider what we have flushed at barge-in */
    bytesToDrop = u64SpeakerStreamerOffset - u64SpeakerStreamerEnd - u32SpeakerFlushedSize;

    /* Need to dump all the speaker data from the previous request for barge-in use case
     * If u64SpeakerStreamerEnd is 0 then the last request reached the end so we are not in the barge-in use case*/
    if (bytesToDrop > 0 && u64SpeakerStreamerEnd > 0)
    {
        sln_long_to_str((uint64_t)bytesToDrop, long_buf);
        configPRINTF(("[AIS App] Dropping extra packets %s\r\n", long_buf));
        STREAMER_Read(NULL, bytesToDrop);
    }

    sln_long_to_str(offset, long_buf);
    configPRINTF(("[AIS App] OpenSpeaker offset: %s\r\n", long_buf));

    appData.speakerOpen = true;
    /* This offset value is high, it doesn't know when you interrupted
     * the last message so it contains data that has been dropped. But we
     * need it's value so we can determine when the interrupted message has ended */
    appData.speakerOffsetStart       = offset;
    appData.speakerOffsetEnd         = 0;
    appData.speakerOpusFramesFlushed = 0;

    /* Make sure CPU boost remains active */
    BOARD_BoostClock();

    /* Only go to idle if we aren't in a wakeup or mic sending state. This is a guard against
     * Messages that have been sent from the client and server side and they are in flight (timing)
     */
    if (audio_processing_get_state() == kMicStopRecording || audio_processing_get_state() == kMicStop)
    {
        audio_processing_set_state(kIdle);
    }
}

void AIS_AppCallback_CloseSpeaker(ais_handle_t *handle, uint64_t offset, bool immediate)
{
    char long_buf[LONG_STR_BUFSIZE] = {0};

    if (!immediate)
    {
        sln_long_to_str(offset, long_buf);
        configPRINTF(("[AIS App] CloseSpeaker offset: %s\r\n", long_buf));
    }
    else
    {
        configPRINTF(("[AIS App] CloseSpeaker (immediately)\r\n"));
    }

    AIS_State_Lock(handle);
    /* Protect streamer updates */
    if (appData.speakerOpen)
    {
        if (!immediate)
        {
            appData.speakerOffsetEnd = offset;
        }
        else
        {
            /* Signal a barge-in situation */
            appData.bargein = true;
        }
    }
    /* Finished streamer updates */
    AIS_State_Unlock(handle);
}

void AIS_AppCallback_TerminateSpeaker(ais_handle_t *handle)
{
    streamer_handle_t *streamer = (streamer_handle_t *)handle->audioPlayer;

    AIS_State_Lock(handle);

    /* Stop the streamer and flush/ignore all data in flight  */
    configPRINTF(("Resetting streamer playback!\r\n"));
    STREAMER_Stop(streamer);

    appData.speakerOpen = false;
    AIS_ClearState(handle, AIS_TASK_STATE_SPEAKER);

    AIS_State_Unlock(handle);
}

/*!
 * @brief Set UX and Audioprocessing "idle" state (used for IDLE, DO_NOT_DISTURB, Speaking End, and
 * NOTIFICATION_AVAILABLE)
 *
 * @param uxState Target LED UX state to set for "idle" state
 * @param micState Target state of audio processing task
 */
static void set_device_idle_state(ux_attention_states_t uxState, app_events_category_audio_t micState)
{
    ux_attention_set_state(uxState);
    audio_processing_set_state(micState);
}

void AIS_AppCallback_SetLocale(ais_handle_t *handle, char *locale)
{
    app_events_t event;
    char *data = NULL;

    if (locale != NULL)
    {
        data = pvPortMalloc(sizeof(handle->config->locale));
        if (data != NULL)
        {
            memcpy(data, locale, sizeof(handle->config->locale));

            event.category = kAvsEvents;
            event.event    = kSetLocale;
            event.data     = data;

            // Add event onto the queue
            while (errQUEUE_FULL == xQueueSend(s_appEventQueue, &event, (TickType_t)0))
            {
                vTaskDelay(portTICK_PERIOD_MS * 50);
                configPRINTF(("Warning: Fail to send SetLocale event, queue is full, trying again\r\n"));
            }
        }
    }
}

uint32_t idleCount = 0U, thinkCount = 0U, speakCount = 0U;
void AIS_AppCallback_SetAttentionState(ais_state_t state, uint64_t offset, bool immediate)
{
    /* App TODO: handle service state as part of Alexa UX attention system. */
    char long_buf[LONG_STR_BUFSIZE] = {0};
    ux_attention_states_t currUxState;

    if (!immediate)
    {
        sln_long_to_str(offset, long_buf);
        configPRINTF(("[AIS App] SetAttentionState: %d, offset: %s\r\n", state, long_buf));
        /* TODO: transition when 'offset' is hit in the audio stream. */
    }
    else
    {
        configPRINTF(("[AIS App] SetAttentionState: %d (immediately)\r\n", state));
    }

    appData.prevState = appData.state;
    appData.state     = state;

    if (AIS_STATE_IDLE == appData.state)
    {
        idleCount++;
        if (AIS_STATE_SPEAKING == appData.prevState)
        {
            // Set Speaking End idle state
            set_device_idle_state(uxSpeakingEnd, kIdle);
        }
        else
        {
            // Set true IDLE state
            set_device_idle_state(uxIdle, kIdle);
        }

        /* Return to default CPU speed when Idle */
        BOARD_RevertClock();
    }
    else if (AIS_STATE_THINKING == appData.state)
    {
        thinkCount++;

        currUxState = ux_attention_get_state();
        if (currUxState != uxDisconnected)
        {
            ux_attention_set_state(uxThinking);
        }

        if (s_thinkStateTimer && (xTimerReset(s_thinkStateTimer, 0) != pdPASS))
        {
            configPRINTF(("xTimerReset failed\r\n"));
        }
    }
    else if (AIS_STATE_SPEAKING == appData.state)
    {
        speakCount++;
        ux_attention_set_state(uxSpeaking);
    }
    else if (AIS_STATE_ALERTING == appData.state)
    {
        AIS_Alerts_Trigger(0, FALSE);
        configPRINTF(("[AIS] Alerting!\r\n"));
    }
    else if (AIS_STATE_DO_NOT_DISTURB == appData.state)
    {
        // Idle state to "DO_NOT_DISTURB"
        set_device_idle_state(uxDoNotDisturb, kIdle);
    }
    else if (AIS_STATE_NOTIFICATION == appData.state)
    {
        // Idle state to "NOTIFICATION_AVAILABLE"
        set_device_idle_state(uxNotificationQueued, kIdle);
    }
    else if (AIS_STATE_INVALID == appData.state)
    {
        // Handle invalid state by returning to idle
        set_device_idle_state(uxIdle, kIdle);
    }
}

void AIS_AppCallback_SetVolume(ais_handle_t *handle, uint32_t volume, uint64_t offset, bool immediate)
{
    int32_t status    = SLN_FLASH_MGMT_OK;
    uint32_t len      = 0;
    sln_dev_cfg_t cfg = DEFAULT_CFG_VALUES;
    app_events_t event;
    char long_buf[LONG_STR_BUFSIZE] = {0};

    if (!immediate)
    {
        sln_long_to_str(offset, long_buf);
        configPRINTF(("[AIS App] SetVolume: %d, offset: %s\r\n", volume, long_buf));

        /* TODO: transition when 'offset' is hit in the audios tream. */
    }
    else
    {
        configPRINTF(("[AIS App] SetVolume: %d (immediately)\r\n", volume));
    }

    if (appData.volume != volume)
    {
        ux_attention_set_state(uxDeviceChange);
    }

    STREAMER_SetVolume(volume);
    appData.volume = volume;

    status = SLN_FLASH_MGMT_Read(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, &len);

    if (SLN_FLASH_MGMT_OK == status)
    {
        if (appData.volume != cfg.streamVolume)
        {
            cfg.streamVolume = appData.volume;
            status           = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));

            if ((SLN_FLASH_MGMT_EOVERFLOW == status) || (SLN_FLASH_MGMT_EOVERFLOW2 == status))
            {
                SLN_FLASH_MGMT_Erase(DEVICE_CONFIG_FILE_NAME);

                status = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));
            }
        }
    }
    else if (SLN_FLASH_MGMT_ENOENTRY2 == status)
    {
        // We should have an empty file so we can save a new one
        cfg.streamVolume = appData.volume;
        status           = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));
    }

    if (SLN_FLASH_MGMT_OK != status)
    {
        configPRINTF(("[AIS App] ERROR: Unable to store stream volume %d to NVM\r\n", volume));
        configPRINTF(("[AIS App] ERROR: NVM operation returned with %d\r\n", status));
    }

    event.category = kAvsEvents;
    event.event    = kVolChanged;

    // Notify app task
    while (errQUEUE_FULL == xQueueSend(s_appEventQueue, &event, (TickType_t)0))
    {
        vTaskDelay(portTICK_PERIOD_MS * 50);
        /* Send error response */
    }
}

void AIS_AppCallback_SetAlertVolume(uint32_t volume)
{
    int32_t status    = SLN_FLASH_MGMT_OK;
    uint32_t len      = 0;
    sln_dev_cfg_t cfg = DEFAULT_CFG_VALUES;
    app_events_t event;

    appData.alertVolume = volume;
    configPRINTF(("[AIS App] SetAlertVolume: %d\r\n", volume));

    status = SLN_FLASH_MGMT_Read(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, &len);

    if (SLN_FLASH_MGMT_OK == status)
    {
        if (appData.alertVolume != cfg.alertVolume)
        {
            cfg.alertVolume = appData.alertVolume;
            status          = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));

            if ((SLN_FLASH_MGMT_EOVERFLOW == status) || (SLN_FLASH_MGMT_EOVERFLOW2 == status))
            {
                SLN_FLASH_MGMT_Erase(DEVICE_CONFIG_FILE_NAME);

                status = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));
            }
        }
    }
    else if (SLN_FLASH_MGMT_ENOENTRY2 == status)
    {
        // We should have an empty file so we can save a new one
        cfg.alertVolume = appData.alertVolume;
        status          = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));
    }

    if (SLN_FLASH_MGMT_OK != status)
    {
        configPRINTF(("[AIS App] ERROR: Unable to store alert volume %d to NVM\r\n", volume));
        configPRINTF(("[AIS App] ERROR: NVM operation returned with %d\r\n", status));
    }

    event.category = kAvsEvents;
    event.event    = kAlertVolChanged;

    // Notify app task
    while (errQUEUE_FULL == xQueueSend(s_appEventQueue, &event, (TickType_t)0))
    {
        vTaskDelay(portTICK_PERIOD_MS * 50);
        /* Send error response */
    }
}

void AIS_AppCallback_SetClock(uint64_t time)
{
    /* App TODO: store clock value and use for timer/alerts. */

    appData.prevTime          = appData.currTime;
    appData.currTime          = time;
    appData.lastRefresh       = appData.currTime;
    appData.syncClockReceived = true;

    // Adjust epoch to 1970 base year
    time_t epoch       = appData.currTime - AIA_TIME_EPOCH_ADJUST;
    struct tm *timeNow = (struct tm *)pvPortMalloc(sizeof(struct tm));
    configPRINTF(("[AIS App] SetClock: %s\r\n", asctime(gmtime(&epoch))));
    vPortFree(timeNow);
    timeNow = NULL;
}

void AIS_AppCallback_SetAlert(const char *token,
                              uint64_t scheduledTime,
                              uint32_t durationInMilliseconds,
                              ais_alert_type_t type)
{
    ais_alert_t alert = {0};
    memcpy(alert.token, token, safe_strlen(token, AIS_MAX_ALERT_TOKEN_LEN_BYTES));
    alert.scheduledTime = scheduledTime;
    alert.durationMs    = durationInMilliseconds;
    alert.type          = type;
    alert.valid         = true;
    alert.idle          = true;
    app_events_t event;

    int32_t status = AIS_Alerts_SaveAlert(&alert);

    if (0 < status)
    {
        // Existing Alert has been triggered by AIS [status is alert index + 1]
        uint32_t alertIdx = status - 1;

        // Send notification to ux
        AIS_Alerts_Trigger(alertIdx, false);

        // Mark triggered in memory
        AIS_Alerts_MarkAsTriggered(alertIdx);

        configPRINTF(("[AIS App] Alert Triggered: %s\r\n", token));

        // Push token to queue to read from app task
        xQueueSend(g_alertQueue, token, (TickType_t)0);

        event.category = kAlertEvents;
        event.event    = kAlertOnlineTrigger;

        // Notify app task
        while (errQUEUE_FULL == xQueueSend(s_appEventQueue, &event, (TickType_t)0))
        {
            vTaskDelay(portTICK_PERIOD_MS * 50);
            /* Send error response */
        }
    }
    else if (0 == status)
    {
        configPRINTF(("[AIS App] SetAlert process success: %s\r\n", token));

        // Push token to queue to read from app task
        xQueueSend(g_alertQueue, token, (TickType_t)0);

        // Notify alerts task to save to NVM
        xTaskNotify(s_alertsTask, APP_EVENTS_SET_EVENT(kAlertEvents, kNewAlertSet), eSetBits);

        event.category = kAlertEvents;
        event.event    = kNewAlertSet;

        // Notify app task
        while (errQUEUE_FULL == xQueueSend(s_appEventQueue, &event, (TickType_t)0))
        {
            vTaskDelay(portTICK_PERIOD_MS * 50);
            /* Send error response */
        }
    }
    else
    {
        configPRINTF(("[AIS App] SetAlert process failure: %s\r\n", token));

        // Push token to queue to read from app task
        xQueueSend(g_alertQueue, token, (TickType_t)0);

        event.category = kAlertEvents;
        event.event    = kNewAlertSet;

        // Notify app task
        while (errQUEUE_FULL == xQueueSend(s_appEventQueue, &event, (TickType_t)0))
        {
            vTaskDelay(portTICK_PERIOD_MS * 50);
            /* Send error response */
        }
    }
}

void AIS_AppCallback_DeleteAlert(const char *token)
{
    /* App TODO: Delete alert from local memory. */
    app_events_t event;

    configPRINTF(("[AIS App] DeleteAlert: %s\r\n", token));

    size_t len     = safe_strlen(token, AIS_MAX_ALERT_TOKEN_LEN_BYTES);
    int32_t status = AIS_Alerts_MarkForDelete(token, len);

    // Push token to queue to read from app task
    xQueueSend(g_alertQueue, token, (TickType_t)0);

    if (0 == status)
    {
        // Notify alerts task to remove from NVM
        xTaskNotify(s_alertsTask, APP_EVENTS_SET_EVENT(kAlertEvents, kAlertDelete), eSetBits);

        event.category = kAlertEvents;
        event.event    = kAlertDelete;

        // Notify app task
        while (errQUEUE_FULL == xQueueSend(s_appEventQueue, &event, (TickType_t)0))
        {
            vTaskDelay(portTICK_PERIOD_MS * 50);
            /* Send error response */
        }
    }
    else
    {
        event.category = kAlertEvents;
        event.event    = kFailDelete;

        // Notify app task of failure
        while (errQUEUE_FULL == xQueueSend(s_appEventQueue, &event, (TickType_t)0))
        {
            vTaskDelay(portTICK_PERIOD_MS * 50);
            /* Send error response */
        }
    }
}

void AIS_AppCallback_OpenMicrophone(uint32_t timeout, const char *type, const char *token)
{
    /* App TODO: AIS requests mic input.
     * Respond with MicrophoneOpened within 'timeout' time, or send
     * OpenMicrophoneTimedOut. Pass the 'type' and 'token' parameters back with
     * the MicrophoneOpened request, if non-NULL. */
    audio_processing_set_state(kMicKeepOpen);
    /* Make sure CPU boost remains active */
    BOARD_BoostClock();

    // configPRINTF(("[AIS App] OpenMicrophone timeout: %d\r\n", timeout));
}

void AIS_AppCallback_CloseMicrophone(ais_handle_t *handle)
{
    /* App TODO: AIS requests close mic input. */

    /* XXX: need mutex around the changing of this in mic callback.
     * If we modify this here while state machine is updating, can become
     * non-zero before the start of another microphone send. */
    appData.bargein = false;
    audio_processing_set_state(kMicStopRecording);
}

void AIS_AppCallback_Exception(const char *code, const char *description)
{
    /* App TODO: handle exception error data. */

    configPRINTF(("[AIS App] Exception: %s: %s\r\n", code, description));
}

void AIS_AppError(ais_status_t error)
{
    /* App TODO: handle AIS error indication.
     * Microphone errors will transition AIS to IDLE state.  All others are
     * critical and require a restart of the service. */
}

void AIS_AppRegistrationInfo(ais_reg_config *registrationInfo)
{
    uint32_t status = kStatus_Success;

    /* Write shared secret to hyperflash. */
    status = SLN_FLASH_MGMT_Save(AIS_REGISTRATION_INFO_FILE_NAME, (uint8_t *)registrationInfo, sizeof(ais_reg_config));
    if (kStatus_Success == status)
    {
        configPRINTF(("[AIS App] Registration info saved to flash\r\n"));
    }
    else
    {
        configPRINTF(("[AIS App] Error: %d, Registration secret key could not be saved to flash\r\n", status));
    }
}

void AIS_Deregister(void)
{
    configPRINTF(("Starting factory reset ...\r\n"));

    /* Factory reset stuff */
#if USE_WIFI_CONNECTION
    wifi_credentials_flash_reset();
#endif /* USE_WIFI_CONNECTION */
    SLN_FLASH_MGMT_Erase(AIS_REGISTRATION_INFO_FILE_NAME);
    SLN_FLASH_MGMT_Erase(AIS_ALERT_FILE_NAME);
    SLN_FLASH_MGMT_Erase(AMZ_WW_MODEL_FILE_NAME);

#ifdef FFS_ENABLED
    aceFR_reset(ACEFR_DEEP);
#endif

    sln_reset("AIS_Deregister");
}

void AIS_AppDisconnected(ais_handle_t *handle, ais_disconnect_code_t code, const char *description)
{
    bool reconnect = false;
    /* Signal reconnection task that disconnect has occurred */
    switch (code)
    {
        case AIS_DISCONNECT_NONE:
            break;
        case AIS_DISCONNECT_INVALID_SEQUENCE:
            reconnect = reconnection_task_set_event(kReconnectInvalidSequence);
            break;

#if (defined(AIS_SPEC_REV_325) && (AIS_SPEC_REV_325 == 1))
        case AIS_DISCONNECT_MESSAGE_TAMPERED:
            reconnect = reconnection_task_set_event(kReconnectMessageTampered);
            break;
#endif

        case AIS_DISCONNECT_API_DEPRECATED:
            reconnect = reconnection_task_set_event(kReconnectAPIDeprecated);
            break;

        case AIS_DISCONNECT_ENCRYPTION_ERROR:
            reconnect = reconnection_task_set_event(kReconnectEncryptionError);
            break;

        case AIS_DISCONNECT_GOING_OFFLINE:
        default:
            reconnect = reconnection_task_set_event(kReconnectAISDisconnect);
            break;
    }

    if (reconnect)
    {
        configPRINTF(("[AIS App] Disconnected from service: %s\r\n", description));

        handle->pendDisconnectSent = true;
    }
}

ais_app_data_t *AIS_APP_GetAppData(void)
{
    return &appData;
}

/**
 * @brief Function called when the registered timer's timeout is triggered.
 *
 * @param xTimer[in]    Pointer to the timer structure
 * @return              Void
 */
static void vThinkStateTimerCallback(TimerHandle_t xTimer)
{
    if (AIS_STATE_THINKING == appData.state)
    {
        if (reconnection_task_set_event(kReconnectAISDisconnect))
        {
            /* still in thinking state when the timeout expired, trigger reconnect */
            configPRINTF(("Thinking state timeout, triggering a reconnection\r\n"));
        }
    }
}
