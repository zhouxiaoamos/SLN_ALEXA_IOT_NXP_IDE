/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All rights reserved.
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
* Header of platform logger implmentation on freertos. The header is not exposed
* to other libraries. Only for internal use.
*@File: asd_logmgr_impl.h
********************************************************************************
*/

#pragma once

#include "logger_port.h"
#include <stdbool.h>
#include "asd_logger_if.h"
#include "asd_logger_config.h"
#include "asd_logger_internal_config.h"
#include "afw_stream.h"
#include "event_groups.h"
#include "asd_flash_buffer.h"
#include "asd_flash_region_mgr.h"
#include "asd_log_msg.h"
#include "log_request_queue.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    EventBits_t events;
    int (*events_process)(EventBits_t events, void* arg);
    uint8_t priority;
}asd_logger_events_process_t;

// Define asd_logger_t based on asd_logger_if_t. Only accessed by this lib.
typedef struct {
    //The first must be logger interface struct.
    asd_logger_if_t ifc;
    bool running;

    // -------- customized fields for logger implementation --------------------

    asd_logger_config_t config;
    TimerHandle_t       expired_cleanup_timer;
    bool                needs_timer_period_change;

    //------------------------frontend --------------------------------
    //Queue to pass log requests.
    asd_log_request_queue_t request_queue;
    afw_stream_t *istream[ASD_LOG_INPUT_STREAM_NUM];
    //events:
    EventGroupHandle_t events;
    asd_logger_events_process_t events_process[ASD_LOG_EVENT_NUM];

    //-------------------------backend -----------------------------
    afw_stream_t *ostream[ASD_LOG_OUT_STREAM_NUM];
    //log task for log service.
    TaskHandle_t task_handle;
    uint8_t scratch_pad[ASD_LOGGER_SCRATCH_PAD_SIZE];


    //Config mutex when updating
    SemaphoreHandle_t config_mutex;

    //Counters for dropped logs and data
    uint32_t log_drops;             ///< log lines dropped from last report.
    uint32_t data_drops;            ///< data bytes dropped from last report.
    uint32_t request_drops;         ///< log requests dropped from last report.
    uint32_t total_log_drops;       ///< total lines dropped from bootup.
} asd_logger_t;

/**
 * @brief   Perform freertos specific configuration updates
 *          Such as restarting tasks with new options
 * @param [in/out]handle - The logger handle.
 * @param [in]new_config - A struct containing specific configuration options
 *                  for the logmgr process
 * @return   error code indicating process success
 */
asd_logger_rc_t asd_logger_update_config(
    const asd_logger_if_t* handle, const asd_logger_config_t* new_config);

/**
 * @brief Get the console stream. singleton instance.
 * @param  none
 *
 * @return afw_stream pointer of console stream.
 */
afw_stream_t* asd_get_console_stream(void);

/**
 * @brief Send a request to logger.
 * @param [in] opts: options for vprint.
 * @param [in] fmt: print format string
 * @param [in] varg: arg list
 *
 * @return Check asd_logger_rc_t for return values.
 */
asd_logger_rc_t asd_logger_vprint_console(const asd_log_options_t* opts, const char* fmt, va_list varg);


/**
 * @brief Get the flash buffer according to log_id
 * @param [in] logger: asd_logger pointer.
 * @param [in] log_id: the log id of flash buffer.
 *
 * @return flash buffer pointer.
 */
asd_flash_buffer_t* asd_logger_get_flash_buffer(const asd_logger_t* logger, uint8_t log_id);

/**
 * @brief Get the flash manager pointer.
 * @param [in] logger: asd_logger pointer.
 *
 * @return flash manager pointer.
 */
const asd_flash_mgr_t* asd_logger_get_flash_mgr(const asd_logger_t* logger);

/**
 * @brief Decode binary log line header and write it in txt_header.
 * @param [in] bin_header: binary line header pointer.
 * @param [out] txt_header: string pointer of line header.
 * @parma [in] size: size of txt_header buffer. It should be at least
 *               ASD_LOG_LINE_HEADER_TXT_SIZE + 1;
 *
 * @return bytes written for line header, or negative for error. The returned string
 *         in txt_header is ended by '\0', but the returned byte number doesn't
 *         include null terminator.
 *         ASD_LOGGER_ERROR_OVERFLOW: txt_header buffer is too small.
 */
int32_t asd_logger_decode_line_header(
                                 const asd_log_line_header_t* bin_header,
                                 char* txt_header,
                                 uint32_t size);


/**
 * @brief Get the lines of dropped log.
 * @param [in] handle: asd_logger pointer.
 *
 * @return number of lines of dropped log.
 */
int32_t asd_logger_get_log_drops(asd_logger_if_t* handle);


/**
 * @brief Get log request queue struct.
 * @param [in] handle: asd_logger pointer.
 *
 * @return pointer to the request queue struct.
 */
asd_log_request_queue_t* asd_logger_get_request_queue(asd_logger_if_t* handle);

#ifdef __cplusplus
}
#endif



