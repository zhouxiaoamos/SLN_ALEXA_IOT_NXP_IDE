/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file. This file is a
 * Modifiable File, as defined in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */
/*******************************************************************************
* Implement the real logger interfaces used by asd_logger_if.
*@File: asd_logger_impl.c
********************************************************************************
*/

#include "logger_port.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <timers.h>
#include <assert.h>
#include "asd_log_api.h"
#include "asd_logger_if.h"
#include "asd_log_platform_api.h"
#include "log_request.h"
#include "asd_flash_region_mgr.h"
#include "asd_logger_impl.h"
#include "log_buffer.h"
#include "asd_log_msg.h"
#include "asd_log_reader.h"
#include "asd_log_eraser.h"
#include "asd_crashdump.h"
#include "time_utils.h"
#include <portmacro.h>


#define ASD_LOG_TIME_FORMAT  "%Y-%m-%d %H:%M:%S"

#define ASD_LOG_LEVEL_TABLE  \
    TABLE_ENTRY(ASD_LOG_LEVEL_DEBUG, 'D')    \
    TABLE_ENTRY(ASD_LOG_LEVEL_INFO, 'I')     \
    TABLE_ENTRY(ASD_LOG_LEVEL_WARN, 'W')     \
    TABLE_ENTRY(ASD_LOG_LEVEL_ERROR, 'E')    \
    TABLE_ENTRY(ASD_LOG_LEVEL_FATAL, 'F')    \
    TABLE_ENTRY(ASD_LOG_LEVEL_NUM,   'U')   /*UNKNOWN*/

static const char asd_log_level_char[] = {
#define TABLE_ENTRY(a, b) b,
    ASD_LOG_LEVEL_TABLE
#undef     TABLE_ENTRY
};

#define IS_VALID_ASD_LOG_LEVEL(L)   (((uint32_t)(L)) < ASD_LOG_LEVEL_NUM)
#define IS_VALID_ASD_STREAM_BITMAP(_bm) (_bm != 0xFF)

#define ASD_LOG_LEVEL2CHAR(L) (IS_VALID_ASD_LOG_LEVEL(L)? \
              asd_log_level_char[L] : asd_log_level_char[ASD_LOG_LEVEL_NUM])


#define section_foreach_entry(section_name, type_t, elem)    \
extern type_t __start_##section_name;                        \
extern type_t __end_##section_name;                          \
    for (type_t *elem = &__start_##section_name;             \
        elem < &__end_##section_name;                        \
        ++elem)


//static asd_logger uninitialized.
static asd_logger_t g_logger;

static int32_t console_write(afw_stream_t *afw_stream,
                            const uint8_t * data,
                            uint32_t data_len);

static asd_logger_rc_t asd_logger_init_interface(asd_logger_if_t* handle);
static asd_logger_rc_t asd_logger_frontend_register_stream(const asd_logger_if_t* handle,
                                         asd_logger_stream_direction_t direction,
                                         afw_stream_t *stream, uint8_t index);
static asd_logger_rc_t logger_create_task(asd_logger_t* logger);
static int process_log_requests(EventBits_t events, void* arg);
static int process_input_stream_main(EventBits_t events, void* arg);
static int process_input_stream_dsp(EventBits_t events, void* arg);
static void asd_logger_cleanup(asd_logger_if_t* handle);
static int32_t log_request_handler(asd_logger_t* logger, asd_log_request_t* req);
static void asd_logger_construct_log_line_header(
                asd_log_msg_t* logmsg, const asd_log_options_t* options );



asd_logger_if_t* asd_logger_get_handle(void)
{
    return (asd_logger_if_t*) &g_logger;
}

static void asd_logger_flash_log_expiration_check_cb(TimerHandle_t arg)
{
    asd_logger_t *logger = (asd_logger_t*)asd_logger_get_handle();
    struct timespec ts;
    int32_t rc = ASD_LOGGER_OK;

    if (!clock_isset()) {
        LOGGER_DPRINTF("Clock not set or unable to access clock ... bypassing expiration check.");
        rc = ASD_LOGGER_ERROR;
    }
    else
    {
        uint64_t expiration_timestamp_ms = clock_gettime_ms();
        expiration_timestamp_ms -= SEC_TO_MSEC(logger->config.flash_log_lifetime_seconds);
        LOGGER_DPRINTF("Check Started: Expiration time_stamp %lu seconds", (uint32_t)MSEC_TO_SEC(expiration_timestamp_ms));
        rc = asd_logger_erase_expired_flash_log(ASD_LOG_ID_MAIN, 0, expiration_timestamp_ms);
    }

    if (rc != ASD_LOGGER_OK)
    {
        LOGGER_DPRINTF("Encountered issues erasing expired logs. Retrying in a few minutes.");
        logger->needs_timer_period_change = true;

        if (xTimerChangePeriod(logger->expired_cleanup_timer,
            pdMS_TO_TICKS(SEC_TO_MSEC(logger->config.flash_log_expire_retry_seconds)), 0) == pdFAIL)
        {
            LOGGER_DPRINTF("Having problems changing the time period for expiration.");
        }
    }
    else if (logger->needs_timer_period_change)
    {
        if (xTimerChangePeriod(logger->expired_cleanup_timer,
            pdMS_TO_TICKS(SEC_TO_MSEC(logger->config.flash_log_expire_check_timer_interval_seconds)), 0) == pdPASS)
        {
            logger->needs_timer_period_change = false;
        }
        else
        {
            LOGGER_DPRINTF("Having problems changing the time period for expiration.");
        }
    }
}

static void asd_logger_setup_expiration_log_check(asd_logger_t* logger)
{
    LOGGER_DPRINTF("Setup expired flash log cleanup every %lu seconds.",
        logger->config.flash_log_expire_check_timer_interval_seconds);

    logger->needs_timer_period_change = true;
    logger->expired_cleanup_timer = xTimerCreate("flash_expiration_check",
        pdMS_TO_TICKS(SEC_TO_MSEC(logger->config.flash_log_expire_first_check_seconds)),
        pdTRUE,
        NULL,
        &asd_logger_flash_log_expiration_check_cb);

    if (xTimerStart(logger->expired_cleanup_timer, portMAX_DELAY) == pdFAIL)
    {
        LOGGER_DPRINTF("Unable to start expired cleanup timer.");
    }
}

asd_logger_rc_t asd_logger_init(asd_logger_if_t* handle,
                const asd_logger_config_t* config)
{
    asd_logger_rc_t ret = ASD_LOGGER_OK;
    if (!handle || !config)
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    asd_logger_t *logger = (asd_logger_t*) handle;

    if (handle->initialized) {
        //already initailzed. report error.
        LOGGER_DPRINTF("Already initialized. Please deinit first, then init");
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    }

    memset(logger, 0, sizeof(asd_logger_t));

    logger->config_mutex = xSemaphoreCreateMutex();
    memcpy(&logger->config, config, sizeof(asd_logger_config_t));

    //allocate request queue.
    ret = asd_log_request_queue_init(&logger->request_queue);
    if (ret != ASD_LOGGER_OK) {
        goto failure_cleanup;
    }
    //allocate main input buffer.
    log_buffer_handle_t main_logbuf = log_buffer_create(
                                             config->main_input_buffer_size,
                                             MULTI_PRODUCERS_BIT);
    if (!main_logbuf) {
        ret = ASD_LOGGER_ERROR_MALLOC;
        goto failure_cleanup;
    }

    afw_stream_t* main_stream = log_buffer_get_stream(main_logbuf);

    //register main input stream.
    if ((ret = asd_logger_frontend_register_stream(handle,
                                                   ASD_LOGGER_STREAM_INPUT,
                                                   main_stream,
                                                   ASD_LOG_INPUT_STREAM_MAIN))
        != ASD_LOGGER_OK ) {
        //delete main input buffer.
        log_buffer_destroy(main_logbuf);
        goto failure_cleanup;
    }

    //set output streams:
    if (config->stream_bm & ASD_LOG_OPTION_BIT_STREAM_CONSOLE) {
        //get static console stream.
        afw_stream_t* console_stream = asd_get_console_stream();
        // register console stream.
        if ((ret = asd_logger_frontend_register_stream(handle,
                                                   ASD_LOGGER_STREAM_OUTPUT,
                                                   console_stream,
                                                   ASD_LOG_STREAM_CONSOLE))
            != ASD_LOGGER_OK) {
            goto failure_cleanup;
        }
    }
    if (config->stream_bm & ASD_LOG_OPTION_BIT_STREAM_FLASH) {
        asd_flash_mgr_t* flashmgr = asd_flash_mgr_init();
        if (!flashmgr) {
            ret = ASD_LOGGER_ERROR_FLASH;
            LOGGER_DPRINTF("Flash manager init failed.");
            goto failure_cleanup;
        }

        afw_stream_t* flash_stream = asd_flash_mgr_get_stream(flashmgr);
        // register console stream.
        if ((ret = asd_logger_frontend_register_stream(handle,
                                                       ASD_LOGGER_STREAM_OUTPUT,
                                                       flash_stream,
                                                       ASD_LOG_STREAM_FLASH))
            != ASD_LOGGER_OK) {
            asd_flash_mgr_deinit(flashmgr);
            goto failure_cleanup;
        }
    }

    logger->events = xEventGroupCreate();
    if (!logger->events) {
        ret = ASD_LOGGER_ERROR_MALLOC;
        goto failure_cleanup;
    }
    // init events processes.
    logger->events_process[0].events = ASD_LOG_EVENT_BIT_LOG_REQUEST;
    logger->events_process[0].events_process = process_log_requests;
    logger->events_process[0].priority = 10;
    //TODO: setup DSP log streaming. FWPLATFORM-433
    logger->events_process[1].events = ASD_LOG_EVENT_BIT_STREAM_DSP;
    logger->events_process[1].events_process = process_input_stream_dsp;
    logger->events_process[1].priority = 2;
    logger->events_process[2].events = ASD_LOG_EVENT_BIT_STREAM_MAIN;
    logger->events_process[2].events_process = process_input_stream_main;
    logger->events_process[2].priority = 0;

    if ((ret = logger_create_task(logger)) != ASD_LOGGER_OK) {
        goto failure_cleanup;
    }
    // intialize interface at the last step.
    if ((ret = asd_logger_init_interface(&logger->ifc)) != ASD_LOGGER_OK) {
        goto failure_cleanup;
    }

    asd_logger_setup_expiration_log_check(logger);

    return ret;

failure_cleanup:
    asd_logger_cleanup(handle);
    return ret;
}


void asd_logger_deinit(asd_logger_if_t* handle)
{
    if (!handle || !handle->initialized) return;
    asd_logger_cleanup(handle);
}

asd_logger_rc_t asd_logger_update_config(
    const asd_logger_if_t* handle, const asd_logger_config_t* new_config)
{
    if (handle == NULL || new_config == NULL) {
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    }
    asd_logger_t* logger = (asd_logger_t*)(handle);
    if(handle->initialized == false)
        return ASD_LOGGER_ERROR_UNINTIALIZED;
    if (pdTRUE == xSemaphoreTake(logger->config_mutex, portMAX_DELAY)) {
        logger->config = *new_config;
        xSemaphoreGive(logger->config_mutex);
        return ASD_LOGGER_OK;
    } else {
        return ASD_LOGGER_ERROR_SEMAPHORE;
    }
}

void asd_logger_dump_module_filters(asd_logger_if_t* handle)
{
    asd_logger_t* logger = (asd_logger_t*) handle;
    if (!handle || !handle->initialized) return;


    printf("module_name\t\tlevel\toutput_stream_bitmap\n");
    printf("* global\t\t%c\t%d" ASD_LINE_ENDING, ASD_LOG_LEVEL2CHAR(logger->config.level),
          logger->config.stream_bm);
    section_foreach_entry(asd_log_module_filters, asd_log_control_block_t, pFilter) {
        printf("%-24s%c\t%d\n", pFilter->module_name, ASD_LOG_LEVEL2CHAR(pFilter->level),
            pFilter->ostream_bm);
    }

}

asd_logger_rc_t asd_logger_vprint_console(const asd_log_options_t* opts,
                                          const char* fmt, va_list varg)
{
    int32_t rc = ASD_LOGGER_OK;
    char date[32];
    struct tm t;
    uint64_t ts_ms = clock_gettime_ms();
    time_t secs = (time_t)(ts_ms/1000);
    localtime_r(&secs, &t);
    strftime(date, sizeof(date), ASD_LOG_TIME_FORMAT, &t);

    printf("[%s.%03u %c %3lu %08lx]%s:",
        date, (unsigned int)(ts_ms%1000), /*Fill in the milliseconds*/
        ASD_LOG_LEVEL2CHAR(opts->level),
        uxTaskGetTaskNumber(xTaskGetCurrentTaskHandle()),
        opts->pc,
        opts->tag);
    if (opts->func_name) {
        //print the function name and line number, if available.
        printf("%.*s:%d:", ASD_LOGGER_FUNC_NAME_LENGTH_MAX,
                                    opts->func_name, opts->line_no);
    }
    vprintf(fmt, varg);
    printf(ASD_LINE_ENDING);
    return rc;
}

afw_stream_t* asd_get_console_stream(void)
{
    static afw_stream_table_t console_stream_table = {
            console_write, NULL, NULL, NULL, NULL};
    static afw_stream_t console_stream = {
        .table = &console_stream_table,
    };
    return &console_stream;
}

int32_t asd_logger_decode_line_header(
                                 const asd_log_line_header_t* bin_header,
                                 char* txt_header,
                                 uint32_t size)
{
    if (!bin_header || !txt_header || size < (ASD_LOG_LINE_HEADER_TXT_SIZE + 1))
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    int32_t total_len = 0;
    *txt_header++ = '[';
    size--;
    total_len = 1;
    time_t secs = bin_header->timestamp_ms/1000;
    struct tm t;
    //First print localtime:
    localtime_r(&secs, &t);
    int len = strftime(txt_header, size,
                       ASD_LOG_TIME_FORMAT, &t);
    if (NOT_IN_EXCLUSIVE_RANGE(len, 0, (int)size))
        return ASD_LOGGER_ERROR_OVERFLOW;
    total_len += len;
    size -= len;
    txt_header += len;

    len = snprintf(txt_header, size,
        ".%03d %c %3u %08lx]",
        (int)(bin_header->timestamp_ms%1000), /*Fill in the milliseconds*/
        ASD_LOG_LEVEL2CHAR(bin_header->level),
        bin_header->task_id,
        bin_header->pc);
    if (NOT_IN_EXCLUSIVE_RANGE(len, 0, (int)size))
        return ASD_LOGGER_ERROR_OVERFLOW;
    total_len += len;

    return total_len;
}

int32_t asd_logger_get_log_drops(asd_logger_if_t* handle)
{
    if (!handle || !handle->initialized) return 0;
    asd_logger_t *logger = (asd_logger_t*) handle;
    return logger->total_log_drops;

}

asd_log_request_queue_t* asd_logger_get_request_queue(asd_logger_if_t* handle)
{
    if (!handle || !handle->initialized) return NULL;
    asd_logger_t *logger = (asd_logger_t*) handle;
    return &logger->request_queue;
}


// -----------------  static functions -----------------

static void asd_logger_cleanup(asd_logger_if_t *handle)
{
    //clean up resources.
    if (!handle) return;
    asd_logger_t *logger = (asd_logger_t*) handle;
    memset(&logger->ifc, 0, sizeof(logger->ifc));

    if (logger->config_mutex != NULL) {
        xSemaphoreTake(logger->config_mutex, portMAX_DELAY);
        xSemaphoreGive(logger->config_mutex);
        vSemaphoreDelete(logger->config_mutex);
    }

    asd_log_request_queue_deinit(&logger->request_queue);

    for (int i = 0; i < ASD_LOG_INPUT_STREAM_NUM; i++) {
        if (logger->istream[i])
            afw_stream_deinit(logger->istream[i]);
    }
    for (int i = 0; i < ASD_LOG_OUT_STREAM_NUM; i++) {
        if (logger->ostream[i])
            afw_stream_deinit(logger->ostream[i]);
    }
    if (logger->events) {
        vEventGroupDelete(logger->events);
    }
    memset(logger, 0, sizeof(asd_logger_t));
}


static void events_process(asd_logger_t * logger, EventBits_t events)
{

    for (int i = 0; i < ASD_LOG_EVENT_NUM &&
                    logger->events_process[i].events_process; i++) {
        //TODO: process according to priority. FWPLATFORM-433
        if (logger->events_process[i].events & events) {
            logger->events_process[i].events_process(
                     logger->events_process[i].events & events, logger);
        }
    }
}


/***
 * logger_task
 *
 * Purpose:
 *      Run the logger task
 *      Should never return
 * Return:
 *      Nothing
 */
static void logger_task(void* arg)
{
    asd_logger_t* logger = (asd_logger_t*) arg;
    if (!logger) {
        printf("NULL logger!!! Exit %s" ASD_LINE_ENDING, __FUNCTION__);
        return;
    }
    EventBits_t uxBits;
    const TickType_t xTicksToWait = portMAX_DELAY;

    while(logger->running) {

        uxBits = xEventGroupWaitBits(
            logger->events,
            ASD_LOG_EVENT_BIT_ALL,
            pdTRUE,
            pdFALSE,
            xTicksToWait);
        events_process(logger, uxBits);
    }
}

const asd_flash_mgr_t* asd_logger_get_flash_mgr(const asd_logger_t* logger)
{
    return (asd_flash_mgr_t*) logger->ostream[ASD_LOG_STREAM_FLASH];
}

asd_flash_buffer_t* asd_logger_get_flash_buffer(const asd_logger_t* logger, uint8_t log_id)
{
    return asd_flash_mgr_get_buffer_by_log_id(asd_logger_get_flash_mgr(logger), log_id);
}

static int process_output_streams(asd_logger_t* logger, const uint8_t *data, int data_len)
{
    int rc;
    const asd_log_msg_t* msg = (const asd_log_msg_t*) data;
    for (int i = 0; i < ASD_LOG_OUT_STREAM_NUM; i++) {
        if (!logger->ostream[i] ||
            (logger->config.stream_bm & msg->stream_bitmap & (1<<i)) == 0)
            continue;

        if ((rc = afw_stream_write(logger->ostream[i], data, data_len)) < 0) {
            LOGGER_DPRINTF("out stream[%d] error: %d %s", i, rc, afw_strerror(rc));
            //go next stream
        }
    }
    return ASD_LOGGER_OK;
}

static void print_drop_count(asd_logger_t* logger, uint32_t drop_count, const char* counter_name)
{
    if (!drop_count)  return;
    asd_log_msg_t* logmsg = (asd_log_msg_t*) logger->scratch_pad;
    asd_log_options_t options = {
        .version = ASD_LOG_OPTION_VERSION,
        .id = ASD_LOG_ID_MAIN,
        .level = ASD_LOG_LEVEL_WARN,
        .tag = NULL,
        .pc = 0,
        .more_options = ASD_LOG_PLATFORM_STREAM_BM_DEFAULT,
    };

    asd_logger_construct_log_line_header(logmsg, &options);
    asd_log_line_header_t *line = (asd_log_line_header_t *) logmsg->data;
    int size = ASD_LOGGER_SCRATCH_PAD_SIZE - sizeof(asd_log_line_header_t)
                     -sizeof(asd_log_msg_t);
    //print a warning message for drop counter.
    int plen = snprintf((char*) line->data, size, "Warning: %ld %s dropped", drop_count, counter_name);
    if (NOT_IN_EXCLUSIVE_RANGE(plen, 0, size)) {
        plen = size -1;
    }

    logmsg->length = plen + sizeof(asd_log_line_header_t)
                     + sizeof(asd_log_msg_t);
    //write the warning message directly in output streams
    process_output_streams(logger, logger->scratch_pad, logmsg->length);
}

static int process_log_requests(EventBits_t events, void* arg)
{
    asd_log_request_t *request;
    asd_logger_t* logger = (asd_logger_t*) arg;
    //read request in non-blocking mode.
    while (xQueueReceive(logger->request_queue.incoming_queue, &request, 0) == pdPASS) {
        log_request_handler(logger, request);
    }
    uint32_t drop_count = __atomic_exchange_n(&logger->request_drops, 0, __ATOMIC_SEQ_CST);
    print_drop_count(logger, drop_count, "log requests");
    return ASD_LOGGER_OK;
}

static int process_input_stream_main(EventBits_t events, void* arg)
{
    asd_logger_t* logger = (asd_logger_t*) arg;
    int rc;
    //drain all messages from input stream.
    while ((rc = afw_stream_read(logger->istream[ASD_LOG_INPUT_STREAM_MAIN],
                       logger->scratch_pad, sizeof(logger->scratch_pad))) > 0) {
        process_output_streams(logger, logger->scratch_pad, rc);
    }
    //check if there is any log line dropped.
    uint32_t drop_count = __atomic_exchange_n(&logger->log_drops, 0, __ATOMIC_SEQ_CST);
    print_drop_count(logger, drop_count, "log lines");
    // afw_cir_buf returns -AFW_EIO when no data available. Treat it as no error.
    if (rc != -AFW_EIO && rc < 0) {
        LOGGER_DPRINTF("main input stream read error: %s", afw_strerror(rc));
        return rc;
    }
    return ASD_LOGGER_OK;
}

static int process_input_stream_dsp(EventBits_t events, void* logger)
{
    //Setup DSP log streaming.
    printf("Not implemented: process dsp stream" ASD_LINE_ENDING);
    return ASD_LOGGER_OK;
}

static asd_logger_rc_t logger_create_task(
    asd_logger_t* logger)
{
    asd_logger_rc_t error_code = ASD_LOGGER_OK;

    if(logger == NULL)
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    logger->running = true;

    //create lower priority task to monitor queu
    if (pdPASS != xTaskCreate(logger_task, "logd",
                          logger->config.task_stack_size / sizeof(portSTACK_TYPE),
                          (void*)logger,
                          logger->config.task_priority | portPRIVILEGE_BIT,
                          &(logger->task_handle))) {
        error_code = ASD_LOGGER_ERROR_TASK;
        logger->running = false;
    }

    return error_code;
}

// register can not be run in ISR. Only task can call it.
static asd_logger_rc_t asd_logger_frontend_register_stream(
            const asd_logger_if_t* handle,
            asd_logger_stream_direction_t direction,
            afw_stream_t *stream, uint8_t index)
{
    assert(!xPortIsInsideInterrupt());
    asd_logger_rc_t rc = ASD_LOGGER_OK;
    asd_logger_t* logger = (asd_logger_t*) handle;
    if (!handle) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    if (pdTRUE != xSemaphoreTake(logger->config_mutex, portMAX_DELAY)) {
        return ASD_LOGGER_ERROR_SEMAPHORE;
    }
    if (direction == ASD_LOGGER_STREAM_INPUT) {
        if ( index < ASD_LOG_INPUT_STREAM_NUM) {
           logger->istream[index] = stream;
        } else {
            rc = ASD_LOGGER_ERROR_INVALID_PARAMETERS;
        }
    } else {
        if (index < ASD_LOG_OUT_STREAM_NUM) {
            logger->ostream[index] = stream;
        } else {
            rc = ASD_LOGGER_ERROR_INVALID_PARAMETERS;
        }
    }
    xSemaphoreGive(logger->config_mutex);
    return rc;
}

static asd_logger_rc_t asd_logger_frontend_write(
    const asd_logger_if_t* handle, const asd_log_options_t* options,
    const uint8_t* data, int32_t length)
{
    asd_logger_t* logger = (asd_logger_t*) handle;
    if (!handle || !handle->initialized) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    //TODO: pack header. FWPLATFORM-433
    if (afw_stream_write(logger->istream[ASD_LOG_INPUT_STREAM_MAIN], data, length) < 0) {
        //error to push in buffer.
        __atomic_add_fetch(&logger->data_drops, 1, __ATOMIC_SEQ_CST);
        return ASD_LOGGER_ERROR_OVERFLOW;
    }
    xEventGroupSetBits(logger->events, ASD_LOG_EVENT_BIT_STREAM_MAIN);
    return ASD_LOGGER_OK;
}

static void asd_logger_construct_log_line_header(
                asd_log_msg_t* logmsg, const asd_log_options_t* options )
{
    //set message header
    ASD_LOG_MSG_SET(logmsg, options->more_options, options->id, ASD_LOG_DATA_TYPE_LOG_TXT);
    //set log line header
    asd_log_line_header_t *line = (asd_log_line_header_t *) logmsg->data;

    ASD_LOG_LINE_HEADER_SET(line, clock_gettime_ms(), options->level,
        uxTaskGetTaskNumber(xTaskGetCurrentTaskHandle()), options->pc);
}

//Frontend vprint.
static asd_logger_rc_t asd_logger_frontend_vprint(
    const asd_logger_if_t* handle, const asd_log_options_t* options,
    const char* fmt, va_list vargs)
{

    asd_logger_t* logger = (asd_logger_t*) handle;
    if (!handle || !handle->initialized) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    // Use stack space for log data. Don't allocate heap for it.
    uint8_t data[sizeof(asd_log_line_header_t) + ASD_LOG_LINE_MAXSIZE + sizeof(asd_log_msg_t)] = {0};

    asd_log_msg_t *logmsg = (asd_log_msg_t*) data;
    asd_log_line_header_t *line = (asd_log_line_header_t *) logmsg->data;
    asd_logger_construct_log_line_header(logmsg, options);

    //sprintf line:
    int bytes_per_print = 0;
    int size = ASD_LOG_LINE_MAXSIZE;
    char *pline = (char*) line->data;
    if (options->tag) {
        //printf tag:
        bytes_per_print = snprintf(pline, size,"%s:", options->tag);
        if (NOT_IN_EXCLUSIVE_RANGE(bytes_per_print, 0, size)) {
            return ASD_LOGGER_ERROR_LINE_LENGTH_LIMIT;
        }
    }
    size -= bytes_per_print;
    pline += bytes_per_print;

    if (options->func_name) {
        //print the function name and line number, if available.
        bytes_per_print = snprintf(pline, size,
                                                "%.*s:%d:", ASD_LOGGER_FUNC_NAME_LENGTH_MAX,
                                                options->func_name, options->line_no);
        if (NOT_IN_EXCLUSIVE_RANGE(bytes_per_print, 0, size)) {
            // string is truncated due to the limit of buffer.
            bytes_per_print = size - 1;
        }
        size -= bytes_per_print;
        pline += bytes_per_print;
    }
    //sprintf line fmt and args.
    bytes_per_print = vsnprintf(pline, size, fmt, vargs);
    if (NOT_IN_EXCLUSIVE_RANGE(bytes_per_print, 0, size)) {
        // string is truncated due to the limit of buffer.
        bytes_per_print = size - 1;
    }
    size -= bytes_per_print;
    pline += bytes_per_print;
    logmsg->length = sizeof(data) - size;

    //Push log msg to main stream/log buffer.
    if (afw_stream_write(logger->istream[ASD_LOG_INPUT_STREAM_MAIN],
                         data, logmsg->length) < 0) {
        //error to push in buffer. count the dropped lines.
        __atomic_add_fetch(&logger->log_drops, 1, __ATOMIC_SEQ_CST);
        __atomic_add_fetch(&logger->total_log_drops, 1, __ATOMIC_SEQ_CST);
        return ASD_LOGGER_ERROR_OVERFLOW;
    }
    if (xPortIsInsideInterrupt()) {
        BaseType_t _pxHigherPriorityTaskWoken;
        xEventGroupSetBitsFromISR(logger->events, ASD_LOG_EVENT_BIT_STREAM_MAIN,
               &_pxHigherPriorityTaskWoken);
        portYIELD_FROM_ISR_WRAP(_pxHigherPriorityTaskWoken);
    } else {
        xEventGroupSetBits(logger->events, ASD_LOG_EVENT_BIT_STREAM_MAIN);
    }
    return ASD_LOGGER_OK;
}

static bool asd_logger_frontend_option_filter(
    const asd_logger_if_t* handle, const asd_log_control_block_t* module_filter,
    asd_log_options_t* options)
{
    asd_logger_t* logger = (asd_logger_t*) handle;
    if (!handle || !options) return false;

    uint32_t ostream_bm = (logger->config.stream_bm & options->more_options);

    //check global level first:
    if (logger->config.level > options->level)
        return false;

    //check module filter:
    if (module_filter){
        //check module level
        if (module_filter->level > options->level) return false;
        //update output stream bitmap
        ostream_bm &= module_filter->ostream_bm;
    }

    // no output stream, don't write the log.
    if (!ostream_bm) return false;

    //clear the bitmap of output streams
    options->more_options &= (~((1<<ASD_LOG_STREAM_NUM) - 1)) | ostream_bm;

    // Only write log, if
    // 1. output stream is allowed for global filter in asd logger,
    //    AND module filter, AND options
    // 2. level is no less than global filter AND module filter.
    return true;
}


/***
 * asd_logger_frontend_set_log_level
 *
 * Purpose:
 *      Set log level for the whole log system.
 * Param:
 *      [in]handle - The logger handle that tracks this logging instance
 *      [in]level - The minimum log level for the log system.
 * Return:
 *      error code indicating process success
 */
static asd_logger_rc_t asd_logger_frontend_set_log_filter(
                           const asd_logger_if_t* handle, const char* module_name,
                           const asd_log_control_block_t* filter)
{
    asd_logger_t* logger = (asd_logger_t*) handle;
    if (!handle || !handle->initialized || !filter) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    if (pdTRUE != xSemaphoreTake(logger->config_mutex, portMAX_DELAY)) {
        return ASD_LOGGER_ERROR_SEMAPHORE;
    }
    bool found = false;
    if (module_name){
        section_foreach_entry(asd_log_module_filters, asd_log_control_block_t, pFilter) {

            if (strcmp(pFilter->module_name, module_name) &&
                strcmp(module_name, "*")) continue;
            //If module_name is filter module name, or module_name is "*", set the filter for log module.
            // "*" is to apply the filter for all log module.

            if (IS_VALID_ASD_LOG_LEVEL(filter->level)) {
                pFilter->level = filter->level;
            }
            if (IS_VALID_ASD_STREAM_BITMAP(filter->ostream_bm)) {
                pFilter->ostream_bm = filter->ostream_bm;
            }
            found = true;
        }
    }

    if ((!module_name) || (!strcmp(module_name, "*"))){
        //apply global logger filter if module name is NULL or "*"
        if (IS_VALID_ASD_LOG_LEVEL(filter->level)) {
            logger->config.level = filter->level;
        }
        if (IS_VALID_ASD_STREAM_BITMAP(filter->ostream_bm)) {
            logger->config.stream_bm = filter->ostream_bm;
        }
        found = true;
    }
    xSemaphoreGive(logger->config_mutex);
    return found? ASD_LOGGER_OK : ASD_LOGGER_ERROR_INVALID_PARAMETERS;
}

static asd_logger_rc_t asd_logger_frontend_get_log_filter(
                           const asd_logger_if_t* handle, const char* module_name,
                           asd_log_control_block_t* filter)
{
    asd_logger_t* logger = (asd_logger_t*) handle;
    if (!handle || !handle->initialized || !filter) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    if (pdTRUE != xSemaphoreTake(logger->config_mutex, portMAX_DELAY)) {
        return ASD_LOGGER_ERROR_SEMAPHORE;
    }

    bool found = false;
    if (module_name){
        section_foreach_entry(asd_log_module_filters, asd_log_control_block_t, pFilter) {
            if (strcmp(pFilter->module_name, module_name)) continue;
            //Find the module. Update the filter.
            filter->level = pFilter->level;
            filter->ostream_bm = pFilter->ostream_bm;
            found = true;
        }
    } else {
        filter->module_name = module_name;
        filter->level = logger->config.level;
        filter->ostream_bm = logger->config.stream_bm;
        found = true;
    }
    xSemaphoreGive(logger->config_mutex);

    return found? ASD_LOGGER_OK : ASD_LOGGER_ERROR_INVALID_PARAMETERS;
}

static int32_t asd_logger_frontend_send_request(const asd_logger_if_t* handle,
                          asd_log_request_t* request, uint32_t timeout_tick)
{
    asd_logger_t* logger = (asd_logger_t*) handle;
    if (!request || !handle || !handle->initialized)
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    if (asd_log_request_send(&logger->request_queue, request, timeout_tick) != ASD_LOGGER_OK) {
        __atomic_add_fetch(&logger->request_drops, 1, __ATOMIC_SEQ_CST);
    } else {
        //notify event for log request to backend.
        xEventGroupSetBits(logger->events, ASD_LOG_EVENT_BIT_LOG_REQUEST);
    }

    return ASD_LOG_REQUEST_RC(request);
}

static int32_t asd_logger_frontend_wait_request_completion(const asd_logger_if_t* handle,
                          asd_log_request_t* request, uint32_t timeout_tick)
{
    if (!request || !handle || !handle->initialized)
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    asd_logger_t* logger = (asd_logger_t*) handle;
    return asd_log_request_wait_completion(&logger->request_queue, request, timeout_tick);
}

// Initialize logger interface. This is called at the end of successful asd_logger_init().
static asd_logger_rc_t asd_logger_init_interface(asd_logger_if_t* handle)
{
    handle->write = asd_logger_frontend_write;
    handle->vprint = asd_logger_frontend_vprint;
    handle->option_filter = asd_logger_frontend_option_filter;
    handle->register_stream = asd_logger_frontend_register_stream;
    handle->set_log_filter = asd_logger_frontend_set_log_filter;
    handle->get_log_filter = asd_logger_frontend_get_log_filter;
    handle->send_request = asd_logger_frontend_send_request;
    handle->wait_completion = asd_logger_frontend_wait_request_completion;
    handle->initialized = 1;
    return ASD_LOGGER_OK;
}


static asd_logger_rc_t asd_logger_backend_console_print_oneline(const asd_log_msg_t *log_msg)
{
    if(!log_msg) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    assert(log_msg->type == ASD_LOG_DATA_TYPE_LOG_TXT);
    char header[ASD_LOG_LINE_HEADER_TXT_SIZE+1]={0};
    const asd_log_line_header_t* line = (const asd_log_line_header_t*) log_msg->data;
    asd_logger_decode_line_header(line, header, ASD_LOG_LINE_HEADER_TXT_SIZE + 1);

    printf("%.*s%.*s"ASD_LINE_ENDING,
        ASD_LOG_LINE_HEADER_TXT_SIZE,
        header,
        GET_LOG_LINE_BODY_LENGTH(log_msg),
        (char*)line->data);

    return ASD_LOGGER_OK;
}

static int32_t console_write(afw_stream_t *afw_stream,
                            const uint8_t * data,
                            uint32_t data_len)
{
    if (!afw_stream || !data) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    return asd_logger_backend_console_print_oneline(
                             (const asd_log_msg_t*) data);
}

static uint32_t count_decoded_text_length(uint32_t bin_length, uint32_t line_num)
{
    return bin_length + line_num*(ASD_LOG_LINE_HEADER_SIZE_DELTA + sizeof(ASD_LINE_ENDING)-1);
}

static int32_t compute_remaining_log_length(asd_logger_t* logger, asd_log_reader_t* reader)
{
    int32_t rc = 0;
    if (!logger || !reader) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    if (reader->log_id >= ASD_LOG_ID_NUM) return ASD_LOGGER_ERROR;
    switch (reader->log_id) {
    case ASD_LOG_ID_CRASH_LOG:
        rc = asd_crashdump_get_total_frame_length();
        if (rc < 0)  break;
        rc -= reader->read_len;
        break;
    default: {
            uint32_t data_length;
            uint32_t frame_num;
            rc = asd_flash_buffer_get_data_length_and_frame_num_from_frame(
                                         asd_logger_get_flash_buffer(logger, reader->log_id),
                                         &reader->frame_info, &data_length, &frame_num);
            if (rc < 0) break;
            if (data_length < reader->offset_in_frame) {
                rc = ASD_LOGGER_ERROR_FLASH;
                break;
            }
            data_length -= reader->offset_in_frame;
            if (reader->offset_in_frame != 0) {
                //the current frame header is not included.
                frame_num--;
            }
            rc = count_decoded_text_length(data_length, frame_num);
            break;
        }
    }
    return rc;
}

static int32_t log_eraser_check_and_erase_main(asd_flash_buffer_t* flashbuf, asd_log_eraser_t* eraser) {
    asd_frame_info_t frame_info;
    asd_frame_info_t next_frame_info = {0};

    if (!eraser || !eraser->should_erase_data_cb)
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    int32_t rc = asd_flash_buffer_get_first_frame_info(flashbuf, &frame_info);

    asd_logger_t* logger = (asd_logger_t*)asd_logger_get_handle();
    uint8_t* data = logger->scratch_pad;
    asd_log_line_header_t* log_header = (asd_log_line_header_t*)data;

    LOGGER_DPRINTF("Started log eraser - main log");

    int counter = 0;
    while (rc == 0) {
        rc = asd_flash_buffer_read_payload(
                flashbuf,
                &frame_info,
                0,
                data,
                sizeof(logger->scratch_pad));
        if (rc > 0)
        {
             // Keep going through frames until we hit a payload where we shouldn't erase data
             // Future frames should have more recent data.
             // (Excluding reboot where the clock goes back to the epoch until SNTP is successful.
             //  It is ok to not clean this data out, it will get cleaned out eventually).
            if (!eraser->should_erase_data_cb(&eraser->callback_params, (void *)log_header)) {
                rc = asd_flash_buffer_erase_before(flashbuf, frame_info.start_pos);
                LOGGER_DPRINTF("Erasing main log sectors prior to frame_info count %d RC: %d", counter, (int)rc);
                return rc;
            }
        }

        rc = asd_flash_buffer_get_next_frame_info(
                flashbuf,
                &frame_info,
                &next_frame_info);

        frame_info = next_frame_info;
        counter++;
    }

    LOGGER_DPRINTF("Completed log eraser. Processed %d data.", counter);

    // If we reached the end of the buffer, we erase everything.
    // If we encountered other errors we should just return that rc value.
    return (rc == ASD_FLASH_BUFFER_ENO_BYTES) ? asd_flash_buffer_erase_all(flashbuf) : rc;
}

// handle individual log request.
static int32_t log_request_handler(asd_logger_t* logger, asd_log_request_t* req)
{
    int32_t rc = ASD_LOGGER_OK;

    assert(req);
    if (asd_log_request_is_canceling(req)) {
        //skip process and free the request. no response back.
        return asd_log_request_complete(&logger->request_queue, req);
    }

    switch (req->op) {
    case LOG_REQUEST_SET_POSITION_START:
        if (LOG_REQUEST_STATUS_CANCELING == asd_log_request_hold_resource(req)) {
            //skip process and free the request. no response back.
            break;
        }
        if (req->log_id == ASD_LOG_ID_CRASH_LOG) {
            rc = asd_crashdump_get_first_frame(&req->reader->frame_info);
        } else {
            rc = asd_flash_buffer_get_first_frame_info(
                  asd_logger_get_flash_buffer(logger, req->log_id),
                  &req->reader->frame_info);
        }
        break;

    case LOG_REQUEST_SET_POSITION_UNREAD:
        if (LOG_REQUEST_STATUS_CANCELING == asd_log_request_hold_resource(req)) {
            //skip process and free the request. no response back.
            break;
        }
        if (req->log_id == ASD_LOG_ID_CRASH_LOG) {
            rc = asd_crashdump_get_first_frame(&req->reader->frame_info);
        } else {
            rc = asd_flash_buffer_get_first_unread_frame_info(
                  asd_logger_get_flash_buffer(logger, req->log_id),
                  &req->reader->frame_info);
        }
        break;

    case LOG_REQUEST_SET_POSITION_END:
        if (LOG_REQUEST_STATUS_CANCELING == asd_log_request_hold_resource(req)) {
            //skip process and free the request. no response back.
            break;
        }
        if (req->log_id == ASD_LOG_ID_CRASH_LOG) {
            rc = asd_crashdump_get_first_frame(&req->reader->frame_info);
        } else {
            rc = asd_flash_buffer_get_last_frame_info(
                  asd_logger_get_flash_buffer(logger, req->log_id),
                  &req->reader->frame_info);
            if (rc == ASD_FLASH_BUFFER_OK) {
                req->reader->offset_in_frame = req->reader->frame_info.length;
            }
        }
        break;

    case LOG_REQUEST_READ_LOG:
        if (LOG_REQUEST_STATUS_CANCELING == asd_log_request_hold_resource(req)) {
            //skip process and free the request. no response back.
            break;
        }
        rc = log_reader_backend_read_log_from_flash(
                         asd_logger_get_flash_mgr(logger), req->reader);
        break;

    case LOG_REQUEST_ERASE_EXPIRED_LOG:
        if (req->log_id == ASD_LOG_ID_MAIN) {
            rc = log_eraser_check_and_erase_main(
                asd_logger_get_flash_buffer(logger, req->log_id), req->eraser);
        }
        else {
            rc = ASD_LOGGER_ERROR_INVALID_PARAMETERS;
        }
        break;

    case LOG_REQUEST_ERASE_LOG:
        if (req->log_id == ASD_LOG_ID_CRASH_LOG) {
            rc = asd_crashdump_erase();
        } else {
            rc = asd_flash_buffer_erase_all(
                      asd_logger_get_flash_buffer(logger, req->log_id));
        }
        break;
    case LOG_REQUEST_GET_DATA_LENGTH: {
        if (LOG_REQUEST_STATUS_CANCELING == asd_log_request_hold_resource(req)) {
            //skip process and free the request. no response back.
            break;
        }
        rc = compute_remaining_log_length(logger, req->reader);
        break;
    }

    case LOG_REQUEST_MARK_READ_FLAG:
        if (req->log_id == ASD_LOG_ID_CRASH_LOG) {
            rc = asd_crashdump_erase();
        } else {
            if (LOG_REQUEST_STATUS_CANCELING == asd_log_request_hold_resource(req)) {
                //skip process and free the request. no response back.
                break;
            }
            uint32_t pos = req->reader->offset_in_frame + req->reader->frame_info.start_pos;
            asd_log_request_unhold_resource(req);
            rc = asd_flash_buffer_mark_read_before(
                 asd_logger_get_flash_buffer(logger, req->log_id),
                 pos);
        }
        break;
    case LOG_REQUEST_FLUSH_LOG:
        events_process(logger, ASD_LOG_EVENT_BIT_STREAM_MAIN);
        rc = ASD_LOGGER_OK;
        break;

    default:
        rc = ASD_LOGGER_ERROR_INVALID_PARAMETERS;
        break;
    }

    req->rc = rc;
    asd_log_request_complete(&logger->request_queue, req);
    return ASD_LOGGER_OK;
}



