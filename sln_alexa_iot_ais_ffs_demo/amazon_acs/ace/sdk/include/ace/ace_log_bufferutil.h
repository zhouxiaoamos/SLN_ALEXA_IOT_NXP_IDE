/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

/**
 * @file ace_log_bufferutil.h
 * @brief ACE Log BufferUtil defines API allowing application to control
 * logging buffer operation
 * @addtogroup ACE_LOG_BUFFERUTIL Logging BufferUtil APIs
 * @{
 * @brief   ACE Log BufferUtil defines API allowing application to control
 * logging buffer operation
 * @ingroup ACE_LOG
 */

#ifndef ACE_LOG_BUFFERUTIL_H
#define ACE_LOG_BUFFERUTIL_H

#include <ace/ace_log.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup ACE_LOG_BUFFERUTIL_API_DEFINE Data structures, Defines and Enums
 * @{
 * @ingroup ACE_LOG_BUFFERUTIL
 */

/**
 * @brief Log buffer output destinations those may support related APIs
 */
typedef enum {
    /** Console output */
    ACE_LOG_DEST_CONSOLE = 0,
    /** Number of buffer output destinations IDs */
    ACE_LOG_DEST_NUM = 1
} aceLog_dest_t;

/**
 * @brief Log buffer output destinations states
 */
typedef enum {
    /** Disable logging to this output destination */
    ACE_LOG_DEST_STATE_OFF = 0,
    /** Enable logging to this output destination */
    ACE_LOG_DEST_STATE_ON = 1,
    /** Default state of this output destination as decided
      * by the platform implementation. This is used only by the
      * aceLog_setBufferOutputDestState API.
      */
    ACE_LOG_DEST_STATE_DEFAULT = 2
} aceLog_destState_t;

/** @} */

/**
 * @defgroup ACE_LOG_BUFFERUTIL_API APIs
 * @{
 * @ingroup ACE_LOG_BUFFERUTIL
 */

/**
 * @brief: Flush (clear) logs in the buffer to persistent storage if appropriate
 * @param[in] timeout_ms Timeout in milliseconds. The implementation should
 * return within the time set by timeout_ms. If timeout_ms is set to 0,
 * the function will not return to the caller until the function is
 * completed.
 * @return ACE_STATUS_OK on success, otherwise one of the error codes of
 * ace_status_t.
 * @retval ACE_STATUS_TIMEOUT if the operation can not be completed when the
 * timeout expires.
 */
ace_status_t aceLog_flushAllBuffers(uint32_t timeout_ms);

/**
 * @brief: Set the state for an output destination of a log buffer
 * @param[in] buffer_id Buffer ID of the logging buffer of interest
 * @param[in] stream_dest Desired output destination as defined in
 * aceLog_dest_t
 * @param[in] state Intended state of the stream_dest as defined in
 * aceLog_destState_t
 * @return ACE_STATUS_OK on success, otherwise one of the error codes of
 * ace_status_t.
 */
ace_status_t aceLog_setBufferOutputDestState(aceLog_buffer_t buffer_id,
                                             aceLog_dest_t stream_dest,
                                             aceLog_destState_t state);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ACE_LOG_BUFFERUTIL_H */

/** @} */
