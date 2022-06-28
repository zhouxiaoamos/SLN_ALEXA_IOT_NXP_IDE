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
* log request function implementation.
*@File: log_request.c
********************************************************************************
*/


#include <assert.h>
#include <string.h>
#include "log_request.h"
#include "asd_logger_if.h"
#include "log_request_queue.h"

int32_t log_request_send_and_wait_completion(asd_log_request_t* request, uint32_t timeout_tick)
{
    //blocking on request.
    int32_t rc = 0;
    rc = asd_logger_send_request(asd_logger_get_handle(), request, timeout_tick);
    if (rc < 0) {
        return rc;
    }

    //wait for completion.
    rc = asd_logger_wait_request_completion(
               asd_logger_get_handle(), request, timeout_tick);
    if (rc < 0){
        return rc;
    }

    return ASD_LOG_REQUEST_RC(request);
}

int32_t asd_logger_flush_log(uint32_t timeout_tick)
{
    asd_log_request_t* request = (asd_log_request_t*) asd_log_request_alloc();
    if (!request) return ASD_LOGGER_ERROR_NO_RESOURCE;
    // log id doesn't matter. It will flush all logs in buffer.
    ASD_LOG_REQUEST_INIT(request, LOG_REQUEST_FLUSH_LOG, 0, 0, NULL);
    int32_t rc = log_request_send_and_wait_completion(request, timeout_tick);
    asd_log_request_free(request);
    return rc;
}




