/*
 * Copyright 2018-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _AISV2_APP_H_
#define _AISV2_APP_H_

#include "aisv2.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define AIS_APP_SMART_HOME_QUEUE_COUNT (6U)

#define AIS_APP_TIMER_INTERVAL_MSEC              (10000U) /* Internal timer ticking to execute timed events */
#define AIS_APP_TIME_SYNC_INTERVAL_SEC           (40U)    /* Re-sync time every 40 seconds */
#define AIS_APP_TIMER_SYNC_CLOCK_TIMEOUT_MSEC    (30000U) /* Timeout for receiving sync clock directive is 30 seconds */
#define AIS_APP_MQTT_DISCONNECT_TIMEOUT_MSEC     (30000U) /* Timeout for MQTT Disconnect is 30 seconds */
#define AIS_APP_MQTT_RECONNECT_TIMEOUT_MSEC      (90000U) /* Timeout for MQTT Reconnect is 90 seconds */
#define AIS_APP_MQTT_AIS_CONNECT_TIMEOUT_MSEC    (60000U) /* Timeout for AIS Connection events is 60 seconds */
#define AIS_APP_MQTT_OTA_DISCONNECT_TIMEOUT_MSEC (30000U) /* Timeout for OTA MQTT Disconnection is 30 seconds */
#define AIS_APP_MQTT_OTA_CONNECT_TIMEOUT_MSEC    (60000U) /* Timeout for OTA MQTT Connection is 60 seconds */
#define AIS_APP_NET_RECV_CHECK_MSEC              (1000U)  /* Time between checking that NetRecv task is still running */
#define AIS_APP_NET_RECV_CONSECUTIVE_TIMES       (2U) /* Consecutive NetRecv task not running in order to start reconnect */
#define AIS_APP_TIMER_INTERVAL_SEC               (AIS_APP_TIMER_INTERVAL_MSEC / 1000)
#define AIS_APP_THINK_STATE_TIMEOUT_MSEC         (10000U)
#define AIS_APP_MAX_SPEAKER_MARKERS              (3U) /* Maximum not replied speaker markers kept in history */
#define AIS_APP_WARNINGS_THRESHOLD_MSEC          (2000U) /* Minimum time between two underrun or overrun warning messages  */
#define AIS_APP_MIC_PUBLISHING_HEAP_THRESHOLD    (25000U) /* Stop Mic Publishing when heap below this threshold */

#define AIA_CLIENT_ID                    "amzn1.application-oa2-client.2e2e9ee683a249d78711d07bdeaa3af8"
#define clientcredentialIOT_PRODUCT_NAME "17865680665"

/** @brief This types are used by vProcessVolume
 *
 *  They are used to determine if it increases or decreases the volume
 */
typedef enum _app_volume_direction
{
    kVolumeUp   = (1),
    kVolumeDown = (-1)
} ais_app_volume_direction_t;

/** @brief This is the main structure of the app
 *
 */
typedef struct
{
    ais_state_t state;     /**< The current state of the app */
    ais_state_t prevState; /**< The previous state of the app */

    uint64_t speakerOffsetStart;       /**< The start offset of the speaker */
    uint64_t speakerOffsetEnd;         /**< The end offset of the speaker */
    uint64_t speakerOffsetWritten;     /**< The offset of the last thing written to the streamer */
    uint32_t speakerOpusFramesFlushed; /**< Remember how much was flushed at stop to use at the next interaction */
    echoMarker_t speakerMarker[AIS_APP_MAX_SPEAKER_MARKERS]; /**< The speaker markers sent by the service used to check
                                                                the progress of the playback */
    bool speakerOpen;                                        /**< Tells if the speaker is open */
    bool speakerOpenSent; /**< Tells if we notified the service that the speaker is open so we can avoid sending
                             multiple SpeakerOpened messages during a playback */
    bool bargein;         /**< Tells if we are in a barge in situation */

    ais_buffer_state_t
        prevSpeakerBufferState; /**< Used for better transition handling, saves the previous speaker buffer state */
    ais_buffer_state_t speakerBufferState; /**< Holds the current speaker buffer state */
    uint32_t overrunSequence;              /**< Holds record of last overrun sequence until received again */

    uint64_t prevTime;    /**< Saves the previous time sent by a sync clock */
    uint64_t currTime;    /**< Saves the current time sent by a sync clock */
    uint64_t lastRefresh; /**< Saves the time of the last refresh */

    bool syncClockReceived; /**< Tells if we receive a sync clock */

    TickType_t previousTickTime;    /**< Saves the previous time that microphone data was sent to the service */
    TickType_t underrunWarningTime; /**< Saves the time stamp for when an underrun warning was sent to the service */
    TickType_t overrunWarningTime;  /**< Saves the time stamp for when an overrun warning was sent to the service */
    int32_t volume;                 /**< Saves the current volume of the device */
    uint32_t alertVolume;           /**< Saves the current alert volume */
} ais_app_data_t;

/** @brief Grabs the ais_app_data structure
 *
 *  @return ais_app_data_t structure or NULL
 */
ais_app_data_t *AIS_APP_GetAppData(void);

/*!
 * @brief Pass the main app event queue handle to the audio processing task
 * @param *handle Reference to the main processing event queue
 */
void AIS_APP_set_queue_handle(QueueHandle_t *handle);

#endif
