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
* log eraser implementation.
*@File: asd_log_eraser.c
********************************************************************************
*/

#include "logger_port.h"
#include <assert.h>
#include <string.h>
#include "log_request.h"
#include "asd_logger_if.h"
#include "asd_log_eraser.h"
#include "log_request_queue.h"
#include "asd_log_msg.h"


int32_t asd_logger_erase_flash_log(asd_log_id_t log_id, uint32_t timeout_tick)
{
    asd_log_request_t* req = asd_log_request_alloc();
    if (!req) return ASD_LOGGER_ERROR_NO_RESOURCE;

    ASD_LOG_REQUEST_INIT(req, LOG_REQUEST_ERASE_LOG, log_id, 0, NULL);

    int32_t rc = log_request_send_and_wait_completion(req, timeout_tick);
    //always free request, it will reduce ref counter.
    asd_log_request_free(req);

    return rc;
}

int32_t asd_logger_erase_all_flash_log(uint32_t timeout_tick)
{
    asd_logger_erase_flash_log(ASD_LOG_ID_MAIN, timeout_tick);
    asd_logger_erase_flash_log(ASD_LOG_ID_DSP, timeout_tick);
    asd_logger_erase_flash_log(ASD_LOG_ID_3P, timeout_tick);
    return ASD_LOGGER_OK;
}

bool main_is_expired_cb(union asd_eraser_cb_params* cb_args, void* data)
{
    asd_log_line_header_t* log_header = (asd_log_line_header_t*)data;
    return cb_args->expiration_timestamp_ms >= log_header->timestamp_ms;
}

int32_t asd_logger_erase_expired_flash_log(asd_log_id_t log_id, uint32_t timeout_tick, uint64_t expiration_timestamp_ms)
{
    static asd_log_eraser_t eraser;
    asd_log_request_t* req = asd_log_request_alloc();
    if (!req) return ASD_LOGGER_ERROR_NO_RESOURCE;
    ASD_LOG_REQUEST_INIT(req, LOG_REQUEST_ERASE_EXPIRED_LOG, log_id,
        ASD_LOG_REQEUST_FLAG_DONT_RESPONSE, &eraser);

    eraser.should_erase_data_cb = &main_is_expired_cb;
    eraser.callback_params.expiration_timestamp_ms = expiration_timestamp_ms;
    int32_t rc = asd_logger_send_request(asd_logger_get_handle(), req, timeout_tick);
    //always free request, it will reduce ref counter.
    asd_log_request_free(req);
    return rc;
}
