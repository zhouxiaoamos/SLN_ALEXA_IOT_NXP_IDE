/*
 * Copyright 2018-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "osa_common.h"
#include "fsl_common.h"

#include "ais_streamer.h"
#include "streamer_pcm.h"
#include "af_error.h"

#define APP_STREAMER_MSG_QUEUE     "app_queue"
#define STREAMER_TASK_NAME         "Streamer"
#define STREAMER_MESSAGE_TASK_NAME "StreamerMessage"

#define STREAMER_TASK_STACK_SIZE         1536
#define STREAMER_MESSAGE_TASK_STACK_SIZE 512
#define STREAMER_DEFAULT_VOLUME          60

/*! @brief local OPUS file internal structure definition */
typedef struct _streamer_local_file
{
    uint32_t len;
    uint8_t *file;
} streamer_local_file_t;

ringbuf_t *audioBuffer;

/* Declaration of locale OPUS file used for playing OPUS audio locally */
static streamer_local_file_t local_active_file_desc;

static uint32_t _STREAMER_ReadLocalFile(uint8_t *buffer, uint32_t size);

/* internal mutex for accessing the audio buffer */
static OsaMutex audioBufMutex;

/*!
 * @brief Streamer task for communicating messages
 *
 * This function is the entry point of a task that is manually created by
 * STREAMER_Create.  It listens on a message queue and receives status updates
 * about errors, audio playback state and position.  The application can make
 * use of this data.
 *
 * @param arg Data to be passed to the task
 */
static void STREAMER_MessageTask(void *arg)
{
    OsaMq mq;
    STREAMER_MSG_T msg;
    streamer_handle_t *handle;
    bool exit_thread = false;
    int ret;

    handle = (streamer_handle_t *)arg;

    configPRINTF(("[STREAMER] Message Task started\r\n"));

    ret = osa_mq_open(&mq, APP_STREAMER_MSG_QUEUE, STREAMER_MSG_SIZE, true);
    if (ERRCODE_NO_ERROR != ret)
    {
        configPRINTF(("osa_mq_open failed: %d\r\n", ret));
        return;
    }

    do
    {
        ret = osa_mq_receive(&mq, (void *)&msg, STREAMER_MSG_SIZE, 0, NULL);
        if (ret != ERRCODE_NO_ERROR)
        {
            configPRINTF(("osa_mq_receiver error: %d\r\n", ret));
            continue;
        }

        switch (msg.id)
        {
            case STREAM_MSG_ERROR:
                configPRINTF(("STREAM_MSG_ERROR %d\r\n", msg.errorcode));

                if (handle->pvExceptionCallback != NULL)
                {
                    handle->pvExceptionCallback();
                }

                break;
            case STREAM_MSG_EOS:
                configPRINTF(("STREAM_MSG_EOS\r\n"));
                xSemaphoreTake(audioBufMutex, portMAX_DELAY);
                if (local_active_file_desc.file != NULL)
                {
                    /* Stop the streamer so we don't send speaker closed
                     * Don't flush the streamer just in case there is pending data */
                    handle->audioPlaying = false;
                    streamer_set_state(handle->streamer, 0, STATE_NULL, true);
                    local_active_file_desc.file = NULL;
                }
                else
                {
                    /* Indicate to other software layers that playing has ended. */
                    handle->eos = true;
                }
                xSemaphoreGive(audioBufMutex);
                break;
            case STREAM_MSG_UPDATE_POSITION:
                configPRINTF(("STREAM_MSG_UPDATE_POSITION\r\n"));
                configPRINTF(("  position: %d ms\r\n", msg.event_data));
                break;
            case STREAM_MSG_CLOSE_TASK:
                configPRINTF(("STREAM_MSG_CLOSE_TASK\r\n"));
                exit_thread = true;
                break;
            default:
                break;
        }

    } while (!exit_thread);

    osa_mq_close(&mq);
    osa_mq_destroy(APP_STREAMER_MSG_QUEUE);
}

int STREAMER_Read(uint8_t *data, uint32_t size)
{
    uint32_t bytes_read = 0;

    /* The streamer reads blocks of data but the decoder only decodes frames
       This means there could be incomplete frames the decoder has got but the
       application could interrupt before sending the full frame.

       The following ensures that only full OPUS frames are given to the streamer
       so we can ensure a transition without Error 252*/
    size = (size - (size % STREAMER_PCM_OPUS_FRAME_SIZE));

    xSemaphoreTake(audioBufMutex, portMAX_DELAY);

    /* If a local sound is being played at the moment, read frames from that source and not from the ring buffer;
       local sounds take precedence over AIS streaming;
       Also check if the destination buffer is NULL - this happens when we want to clear data from the ringbuffer */
    if ((local_active_file_desc.file != NULL) && (data != NULL))
    {
        if (local_active_file_desc.len == -1)
        {
            local_active_file_desc.file = NULL;
        }
        else if (local_active_file_desc.len == 0)
        {
            local_active_file_desc.len = -1;
        }
        else if (local_active_file_desc.len != 0)
        {
            bytes_read = _STREAMER_ReadLocalFile(data, size);
        }
    }

    if ((local_active_file_desc.file == NULL) || (data == NULL))
    {
        bytes_read += ringbuf_read(audioBuffer, data, size);
    }

    xSemaphoreGive(audioBufMutex);

    if (bytes_read != size)
    {
        /* Don't print this warning under normal conditions.
         * * Excessive calls can clog the logging and hide other messages. */
        /*
               configPRINTF(("[STREAMER WARN] read underrun: size: %d, read: %d\r\n", size, bytes_read));
        */
    }

    return bytes_read;
}

int STREAMER_Write(uint8_t *data, uint32_t size)
{
    uint32_t written;

    xSemaphoreTake(audioBufMutex, portMAX_DELAY);
    written = ringbuf_write(audioBuffer, data, size);
    xSemaphoreGive(audioBufMutex);

    if (written != size)
    {
        configPRINTF(("[STREAMER ERR] write overflow: size %d, written %d\r\n", size, written));
    }

    return written;
}

uint32_t STREAMER_GetQueued(streamer_handle_t *handle)
{
    uint32_t bufSize;

    xSemaphoreTake(audioBufMutex, portMAX_DELAY);
    bufSize = ringbuf_get_occupancy(audioBuffer);
    xSemaphoreGive(audioBufMutex);

    return bufSize;
}

uint32_t STREAMER_GetQueuedRaw(streamer_handle_t *handle)
{
    /* Size of frame is also writen into ringbuffer. It should be taken into consideration */
    return STREAMER_GetQueued(handle) / STREAMER_PCM_OPUS_FRAME_SIZE * STREAMER_PCM_OPUS_DATA_SIZE;
}

uint32_t STREAMER_SetLocalSound(streamer_handle_t *handle, uint8_t *file, uint32_t len)
{
    uint32_t status = kStatus_Fail;

    xSemaphoreTake(audioBufMutex, portMAX_DELAY);

    if (local_active_file_desc.file == NULL)
    {
        local_active_file_desc.file = file;
        local_active_file_desc.len  = len;
    }
    status = kStatus_Success;

    xSemaphoreGive(audioBufMutex);

    return status;
}

uint32_t STREAMER_GetQueuedNotBlocking(streamer_handle_t *handle)
{
    return ringbuf_get_occupancy(audioBuffer);
}

bool STREAMER_IsPlaying(streamer_handle_t *handle)
{
    return handle->audioPlaying;
}

void STREAMER_SetVolume(uint32_t volume)
{
    /* Protect against an uninitialized volume */
    if (volume == -1)
    {
        volume = STREAMER_DEFAULT_VOLUME;
    }

    streamer_pcm_set_volume(volume);
}

void STREAMER_Start(streamer_handle_t *handle)
{
    configPRINTF(("[STREAMER] start playback\r\n"));

    handle->audioPlaying = true;
    streamer_set_state(handle->streamer, 0, STATE_PLAYING, true);
}

uint32_t STREAMER_Stop(streamer_handle_t *handle)
{
    uint32_t flushedSize = 0;

    configPRINTF(("[STREAMER] stop playback\r\n"));

    handle->audioPlaying = false;
    streamer_set_state(handle->streamer, 0, STATE_NULL, true);

    flushedSize = STREAMER_GetQueued(handle);

    /* Flush input ringbuffer. */
    xSemaphoreTake(audioBufMutex, portMAX_DELAY);

    local_active_file_desc.file = NULL;
    local_active_file_desc.len  = -1;

    ringbuf_clear(audioBuffer);
    xSemaphoreGive(audioBufMutex);

    return flushedSize;
}

void STREAMER_Pause(streamer_handle_t *handle)
{
    configPRINTF(("[STREAMER] pause playback\r\n"));

    handle->audioPlaying = false;
    streamer_set_state(handle->streamer, 0, STATE_PAUSED, true);

    /* Don't clear out input ringbuffer so play can be resumed. */
}

status_t STREAMER_Create(streamer_handle_t *handle, streamer_decoder_t decoder)
{
    STREAMER_CREATE_PARAM params;
    ELEMENT_PROPERTY_T prop;
    OsaThread msg_thread;
    OsaThreadAttr thread_attr;
    int ret;

    audioBufMutex = xSemaphoreCreateMutex();
    if (!audioBufMutex)
    {
        return kStatus_Fail;
    }

    audioBuffer = ringbuf_create(AWS_AUDIO_BUFFER_SIZE);
    if (!audioBuffer)
    {
        return kStatus_Fail;
    }

    /* Create message process thread */
    osa_thread_attr_init(&thread_attr);
    osa_thread_attr_set_name(&thread_attr, STREAMER_MESSAGE_TASK_NAME);
    osa_thread_attr_set_stack_size(&thread_attr, STREAMER_MESSAGE_TASK_STACK_SIZE);
    ret = osa_thread_create(&msg_thread, &thread_attr, STREAMER_MessageTask, (void *)handle);
    osa_thread_attr_destroy(&thread_attr);
    if (ERRCODE_NO_ERROR != ret)
    {
        return kStatus_Fail;
    }

    /* Create streamer */
    strcpy(params.out_mq_name, APP_STREAMER_MSG_QUEUE);
    params.stack_size    = STREAMER_TASK_STACK_SIZE;
    params.pipeline_type = STREAM_PIPELINE_NETBUF;
    params.task_name     = STREAMER_TASK_NAME;
    params.in_dev_name   = "";
    params.out_dev_name  = "";

    handle->streamer = streamer_create(&params);
    if (!handle->streamer)
    {
        return kStatus_Fail;
    }

    prop.prop = PROP_NETBUFSRC_SET_CALLBACK;
    prop.val  = (uintptr_t)STREAMER_Read;

    streamer_set_property(handle->streamer, prop, true);

    prop.prop = PROP_DECODER_DECODER_TYPE;
    if (decoder == AIS_DECODER_OPUS)
    {
        prop.val = DECODER_TYPE_OPUS;
    }
    else if (decoder == AIS_DECODER_MP3)
    {
        prop.val = DECODER_TYPE_MP3;
    }
    else
    {
        return kStatus_Fail;
    }

    streamer_set_property(handle->streamer, prop, true);

    handle->audioPlaying = false;
    handle->eos          = false;

    return kStatus_Success;
}

void STREAMER_Destroy(streamer_handle_t *handle)
{
    streamer_destroy(handle->streamer);

    vSemaphoreDelete(audioBufMutex);
    ringbuf_destroy(audioBuffer);
}

void STREAMER_Init(void)
{
    /* Initialize OSA*/
    osa_init();

    /* Initialize logging */
    init_logging();

    add_module_name(LOGMDL_STREAMER, "STREAMER");

    /* Uncomment below to turn on full debug logging for the streamer. */
    // set_debug_module(0xffffffff);
    // set_debug_level(0xff);

    /* Initialize streamer PCM management library. */
    streamer_pcm_init();
}

static uint32_t _STREAMER_ReadLocalFile(uint8_t *buffer, uint32_t size)
{
    uint32_t read_size = size;

    /* If the requested size is greater than what's left */
    if (size >= local_active_file_desc.len)
    {
        read_size = local_active_file_desc.len;
    }

    memcpy(buffer, local_active_file_desc.file, read_size);

    /* If the len hasn't hit 0, shift the pointer */
    if (0 != (read_size - local_active_file_desc.len))
    {
        local_active_file_desc.file += read_size;
    }

    local_active_file_desc.len -= read_size;

    return read_size;
}
