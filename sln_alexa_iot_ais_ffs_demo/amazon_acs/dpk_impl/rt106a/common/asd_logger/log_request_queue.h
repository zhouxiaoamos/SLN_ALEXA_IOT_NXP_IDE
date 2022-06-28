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
* The header for log request.
*
*@File: log_request_queue.h
********************************************************************************
*/

#pragma once

#include "logger_port.h"
#include <stdint.h>
#include <stdbool.h>
#include <event_groups.h>
#include "log_request.h"
#include "asd_mempool_mgr.h"
#include "asd_logger_internal_config.h"
#include "asd_log_reader.h"
#include "asd_log_eraser.h"

#ifdef __cplusplus
extern "C"
{
#endif

// mempool provides 4 request items more than log request queue, so that
// clients can allocate and hold a bit more requests than request queue.
#define ASD_LOG_REQEUST_POOL_ITEM_NUM     (ASD_LOGGER_REQUEST_QUEUE_LENGTH + 4)

#if ASD_MEMPOOL_SIZE_MAX < ASD_LOG_REQEUST_POOL_ITEM_NUM
#error "ASD_LOG_REQEUST_POOL_ITEM_NUM must smaller than ASD_MEMPOOL_SIZE_MAX!!!"
#endif

typedef struct {
    QueueHandle_t incoming_queue;        ///< queue for log request.
    EventGroupHandle_t completion_group; ///< event group for request completion.
    asd_mempool_mgr_t  pool_mgr;
    asd_log_request_t  pool_requests[ASD_LOG_REQEUST_POOL_ITEM_NUM];
} asd_log_request_queue_t;

/**
 * @brief Initialize log request queue.
 *
 * @param [in/out] q: queue of log request.
 *
 * @return Negative for failure, otherwise success. See ASD_LOGGER_ERROR...
 */
int32_t asd_log_request_queue_init(asd_log_request_queue_t* q);
/**
 * @brief deinitailize log request queue.
 *        It is blocking mode. Don't use this in ISR.
 * @param [in] q: queue of log request.
 *
 * @return None.
 */
void asd_log_request_queue_deinit(asd_log_request_queue_t* q);

/**
 * @brief Allocate a log request from mempool. Its reference counter is set to 1.
 *        Thread safe and lockless.
 *
 * @return Pointer to an allocated log request. NULL if mempool is empty.
 */
asd_log_request_t* asd_log_request_alloc(void);

/**
 * @brief Free a log request. It reduces the referecne counter by 1, and free
 *        it to mempool freelist ONLY if reference counter is reduced to 0.
 *        Thread safe, and lockless.
 * @param [in] request: pointer of request to release.
 *
 * @return None.
 */
void asd_log_request_free(asd_log_request_t* request);

/**
 * @brief Cancel a request.
 *        It tries cancel the request, if possible.
 *        If request is allowed to cancel (status == LOG_REQUEST_STATUS_CANCEL_ALLOWED),
 *        it sets status to LOG_REQUEST_STATUS_CANCELING.
 *        If request is not allowed to cancel (status != LOG_REQUEST_STATUS_CANCEL_ALLOWED),
 *        it will block on completion wait.
 *        Thread safe.
 * @param [in] q: log request queue struct.
 * @param [in/out] request: pointer of request.
 *
 * @return Negative for failure, otherwise success.
 */
int32_t asd_log_request_cancel(asd_log_request_queue_t* q,
                                           asd_log_request_t* request);

/**
 * @brief Check if log request status is LOG_REQUEST_STATUS_CANCELING.
 *
 * @param [in] request: pointer of request.
 *
 * @return status == LOG_REQUEST_STATUS_CANCELING.
 */
bool asd_log_request_is_canceling(const asd_log_request_t* request);


/**
 * @brief Set log request status as LOG_REQUEST_STATUS_CANCEL_NOT_ALLOWED, if
 *        not canceling request. if request is canceling, do nothing.
 *        Thread safe.
 *
 * @param [in/out] request: pointer of request.
 *
 * @return old status before the API.
 */
asd_log_request_status_t asd_log_request_hold_resource(asd_log_request_t* request);

/**
 * @brief Set log request status as LOG_REQUEST_STATUS_CANCEL_ALLOWED.
 *        Thread safe.
 *
 * @param [in/out] request: pointer of request.
 *
 * @return none.
 */
void asd_log_request_unhold_resource(asd_log_request_t* request);


/**
 * @brief Process request is completed. If request is canceled or flag as
 *        ASD_LOG_REQEUST_FLAG_DONT_RESPONSE , don't response to client.
 *        otherwise, send response for completion.
 *
 * @param [in] q: log request queue struct.
 * @param [in/out] request: pointer of request.
 *
 * @return ASD_LOGGER_OK for success, otherwise negative.
 */
int32_t asd_log_request_complete(asd_log_request_queue_t *q, asd_log_request_t* request);


/**
 * @brief Send request to log request queue.
 *        It clears the response bit for the request, and add referece count
 *        for request.
 *
 * @param [in] q: log request queue struct.
 * @param [in] request: pointer of request.
 * @param [in] timeout_tick: tick of enqueue timeout.
 *
 * @return ASD_LOGGER_OK for success.
 *         ASD_LOGGER_ERROR_QUEUE for enqueue timeout.
 */
int32_t asd_log_request_send(asd_log_request_queue_t* q,
                                asd_log_request_t* request, uint32_t timeout_tick);

/**
 * @brief Block on waiting completion from log request handling.
 *
 * @param [in] q: log request queue struct.
 * @param [in] request: pointer of request.
 * @param [in] timeout_tick: tick of wait timeout.
 *
 * @return ASD_LOGGER_OK for success.
 *         ASD_LOGGER_ERROR_TIMEOUT for wait timeout.
 */
int32_t asd_log_request_wait_completion(
                   asd_log_request_queue_t* q,
                   asd_log_request_t* request, uint32_t timeout_tick);

#ifdef __cplusplus
}
#endif
