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
*@File: log_request.h
********************************************************************************
*/

#pragma once

#include "logger_port.h"
#include <stdint.h>
#include <stdbool.h>
#include "asd_log_api.h"
#include "asd_log_reader.h"
#include "asd_log_eraser.h"

#ifdef __cplusplus
extern "C"
{
#endif

// The request doesn't need response, so request handler will not send completion.
// The requester sends request and forget it. request handler will free the request
// at the end.
#define ASD_LOG_REQEUST_FLAG_DONT_RESPONSE            (1 << 0)

#ifndef BIT
#define BIT(n)                  ((uint32_t) 1 << (n))
#endif

#ifndef BITS
// It returns bits 1 for all bits between index m to n (0-based).
// For example, m = 1, n =3 return = 1110b

#define BITS(m,n)               (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))
#endif

typedef enum {
    LOG_REQUEST_READ_LOG,
    LOG_REQUEST_SET_POSITION_START,
    LOG_REQUEST_SET_POSITION_UNREAD,
    LOG_REQUEST_SET_POSITION_END,
    LOG_REQUEST_GET_DATA_LENGTH,
    LOG_REQUEST_MARK_READ_FLAG,
    LOG_REQUEST_ERASE_LOG,
    LOG_REQUEST_ERASE_EXPIRED_LOG,
    LOG_REQUEST_FLUSH_LOG,
} asd_log_request_op_t;

typedef enum {
    LOG_REQUEST_STATUS_CANCEL_ALLOWED,  ///< This status allow cancel the request
                                        ///< 1. request is still in queue
                                        ///< or
                                        ///< 2. reqeust doesn't involve any shared resource (buffer)

    LOG_REQUEST_STATUS_CANCEL_NOT_ALLOWED, ///< This status doesn't not allow cancel request.
                                           ///< request is being processed, and involve
                                           ///< shared resource (for example, buffer)
    LOG_REQUEST_STATUS_CANCELING,      ///< request is being cancelled. there will be no
                                       ///< response returning.
    LOG_REQUEST_STATUS_COMPLETED,      ///< request is processed completely. A response
                                       ///< will be sent to requester.
} asd_log_request_status_t;

typedef struct asd_log_request{
    asd_log_request_op_t  op;    ///< log request operation
    asd_log_request_status_t status; ///< request status
    uint8_t  flags;             ///< flags, see ASD_LOG_REQEUST_FLAG_...
    asd_log_id_t    log_id;     ///< log id
    uint32_t     refcount;      ///< reference counter
    int32_t      rc;            ///< return code of the request process.
    union {
        asd_log_reader_t *reader;
        asd_log_eraser_t *eraser;
        void* arg;
    };
} asd_log_request_t;

#define ASD_LOG_REQUEST_INIT(_req, _op, _logid, _flags, _arg) do { \
            (_req)->op = _op;                                      \
            (_req)->rc = 0;                                        \
            (_req)->log_id = _logid;                               \
            (_req)->flags = _flags;                                \
            (_req)->status = LOG_REQUEST_STATUS_CANCEL_ALLOWED;    \
            (_req)->arg = (void*) _arg;                            \
        } while(0)

#define ASD_LOG_REQUEST_RC(_req)  ((_req)->rc)

/**
 * @brief Send log request to backend, and wait for request process completion.
 *        It is blocking mode. Don't use this in ISR.
 * @param [in/out] request: pointer of request.
 * @param [in] timeout_tick: timeout in tick.
 *
 * @return Negative for failure, otherwise success.
 */
int32_t log_request_send_and_wait_completion(asd_log_request_t* request, uint32_t timeout_tick);

#ifdef __cplusplus
}
#endif
