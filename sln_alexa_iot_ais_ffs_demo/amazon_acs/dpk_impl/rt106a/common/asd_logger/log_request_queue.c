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
*@File: log_request_queue.c
********************************************************************************
*/


#include <assert.h>
#include <string.h>
#include "log_request.h"
#include "asd_logger_config.h"
#include "asd_logger_impl.h"
#include "afw_utils.h"

static int32_t asd_log_request_get_index(asd_log_request_queue_t* q, asd_log_request_t* request)
{
    if (!q || !request) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    int32_t index = ARRAY_INDEX(q->pool_requests, request);
    assert((index < q->pool_mgr.size) && (index >= 0));
    return index;
}


//// ==========================================================================

int32_t asd_log_request_queue_init(asd_log_request_queue_t* q)
{
    int32_t ret = ASD_LOGGER_OK;
    if (!q) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    memset(q, 0, sizeof(*q));
    // initialize request queue, request memory pool, and completion group.
    q->incoming_queue = xQueueCreate(ASD_LOGGER_REQUEST_QUEUE_LENGTH,
                                         sizeof(asd_log_request_t*));
    if (!q->incoming_queue) {
        ret = ASD_LOGGER_ERROR_MALLOC;
        goto error;
    }

    q->completion_group = xEventGroupCreate();
    if (!q->completion_group) {
        ret = ASD_LOGGER_ERROR_MALLOC;
        goto error;
    }
    ret = asd_mempool_mgr_init(&q->pool_mgr, ASD_LOG_REQEUST_POOL_ITEM_NUM);
    if(ret != ASD_LOGGER_OK)
        goto error;

    return ASD_LOGGER_OK;
error:
    asd_log_request_queue_deinit(q);
    return ret;
}

void asd_log_request_queue_deinit(asd_log_request_queue_t* q)
{
    if (!q) return;

    if (q->incoming_queue) {
        vQueueDelete(q->incoming_queue);
    }
    if (q->completion_group) {
        vEventGroupDelete(q->completion_group);
    }
    memset(q, 0, sizeof(*q));
}

asd_log_request_t* asd_log_request_alloc(void)
{
    asd_log_request_queue_t *q = asd_logger_get_request_queue(asd_logger_get_handle());
    if (!q) return NULL;

    int32_t index = asd_mempool_index_alloc(&q->pool_mgr);
    if (ASD_LOGGER_ERROR_NO_RESOURCE == index) {
        printf("request pool is empty.");
    }
    if (index < 0) return NULL;
    assert(index < q->pool_mgr.size);
    asd_log_request_t *request = &q->pool_requests[index];
    memset(request, 0, sizeof(*request));
    request->refcount = 1;
    return request;
}

void asd_log_request_free(asd_log_request_t* request)
{
    uint32_t oldcount = __atomic_fetch_sub(&request->refcount, 1, __ATOMIC_SEQ_CST);
    assert(oldcount > 0);

    if (oldcount == 1) {
        asd_log_request_queue_t *q = asd_logger_get_request_queue(asd_logger_get_handle());
        if (!q) return;
        int32_t index = asd_log_request_get_index(q, request);
        asd_mempool_index_free(&q->pool_mgr, (uint32_t) index);
    }
}

uint32_t asd_log_request_add_ref(asd_log_request_t* request)
{
    assert(request->refcount > 0);
    return __atomic_add_fetch(&request->refcount, 1, __ATOMIC_SEQ_CST);
}


int32_t asd_log_request_cancel(asd_log_request_queue_t* q,
                                           asd_log_request_t* request)
{
    if (!q || !request) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    asd_log_request_status_t expected = LOG_REQUEST_STATUS_CANCEL_ALLOWED;

    if (__atomic_compare_exchange_n(&request->status, &expected,
             LOG_REQUEST_STATUS_CANCELING,
             false, /* strong variation */
             __ATOMIC_SEQ_CST,
             __ATOMIC_SEQ_CST)) {
        // cancel success.
        LOGGER_DPRINTF("Canceling op %d ... ", request->op);
        return ASD_LOGGER_OK;
    }

    // The request is holding by log task for some shared resource.
    // Can not cancel until complete.
    LOGGER_DPRINTF("Canceling op %d. but not allowed, wait for request completion infinitely.",
                  request->op);
    return asd_log_request_wait_completion(q, request, portMAX_DELAY);
}

bool asd_log_request_is_canceling(const asd_log_request_t* request)
{
    if (!request) return false;

    return __atomic_load_n(&request->status, __ATOMIC_SEQ_CST) == LOG_REQUEST_STATUS_CANCELING;
}

asd_log_request_status_t asd_log_request_hold_resource(asd_log_request_t* request)
{
    if (!request) return LOG_REQUEST_STATUS_CANCEL_ALLOWED;

    asd_log_request_status_t old_status =
        __atomic_exchange_n(&request->status, LOG_REQUEST_STATUS_CANCEL_NOT_ALLOWED,
                                    __ATOMIC_SEQ_CST);
    if (old_status == LOG_REQUEST_STATUS_CANCELING) {
        // it is canceling, don't process the request.
        // recover the old status.
        __atomic_store_n(&request->status, old_status, __ATOMIC_SEQ_CST);

    }
    return old_status;
}

void asd_log_request_unhold_resource(asd_log_request_t* request)
{
    if (!request) return;

    __atomic_store_n(&request->status, LOG_REQUEST_STATUS_CANCEL_ALLOWED, __ATOMIC_SEQ_CST);
}

int32_t asd_log_request_complete(asd_log_request_queue_t *q, asd_log_request_t* request)
{
    if (!request || !q) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    //thread safe.
    asd_log_request_status_t old_status =
               __atomic_exchange_n(&request->status, LOG_REQUEST_STATUS_COMPLETED,
                                    __ATOMIC_SEQ_CST);
    switch (old_status) {
        case LOG_REQUEST_STATUS_CANCEL_ALLOWED:
        case LOG_REQUEST_STATUS_CANCEL_NOT_ALLOWED: {
            if (request->flags & ASD_LOG_REQEUST_FLAG_DONT_RESPONSE) {
                //skip response
                LOGGER_DPRINTF("%d Completed. but no response required.", request->op);
                break;
            }
            //normal case.
            if (!q) return ASD_LOGGER_ERROR_UNINTIALIZED;
            int32_t index = asd_log_request_get_index(q, request);
            LOGGER_DPRINTF("Sent completion response for op %d.", request->op);
            //notify requester completion.
            xEventGroupSetBits(q->completion_group, BIT(index));

            break;
        }
        case LOG_REQUEST_STATUS_CANCELING: {
            // canceled.
            // no need to notify requester, since it is canceled already.
            LOGGER_DPRINTF("%d Canceled. No response required.", request->op);
            break;
        }
        default:
            return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    }
    // This just reduces the ref counter.
    asd_log_request_free(request);
    return ASD_LOGGER_OK;

}

int32_t asd_log_request_send(asd_log_request_queue_t* q,
                                asd_log_request_t* request, uint32_t timeout_tick)
{
    if (!request || !q) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    int32_t index = asd_log_request_get_index(q, request);
    xEventGroupClearBits(q->completion_group, BIT(index));
    request->status = LOG_REQUEST_STATUS_CANCEL_ALLOWED;
    ASD_LOG_REQUEST_RC(request) = ASD_LOGGER_OK;
    //add ref counter before send it to queue.
    asd_log_request_add_ref(request);
    if (xQueueSend(q->incoming_queue, &request, timeout_tick) != pdTRUE) {
        ASD_LOG_REQUEST_RC(request) = ASD_LOGGER_ERROR_QUEUE;
        //reduce ref counter if enqueue fails.
        asd_log_request_free(request);
        return ASD_LOGGER_ERROR_QUEUE;
    }
    LOGGER_DPRINTF("Sent request op %d.", request->op);
    return ASD_LOGGER_OK;
}

int32_t asd_log_request_wait_completion(
                   asd_log_request_queue_t* q,
                   asd_log_request_t* request, uint32_t timeout_tick)
{
    if (!request || !q) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    // Should not wait for completion if request is set "dont response".
    if (0 != (request->flags & ASD_LOG_REQEUST_FLAG_DONT_RESPONSE)) {
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    }

    int32_t index = asd_log_request_get_index(q, request);
    LOGGER_DPRINTF("Wait for completion op %d.", request->op);
    EventBits_t uxBits = xEventGroupWaitBits(
            q->completion_group,
            BIT(index),
            pdTRUE,
            pdFALSE,
            timeout_tick);
    if (uxBits & BIT(index)) {
        LOGGER_DPRINTF("Get Completion op %d.", request->op);
        return ASD_LOGGER_OK;
    } else {
        //timeout, let's cancel the request if possible.
        asd_log_request_cancel(q, request);

        return ASD_LOGGER_ERROR_TIMEOUT;
    }
}





