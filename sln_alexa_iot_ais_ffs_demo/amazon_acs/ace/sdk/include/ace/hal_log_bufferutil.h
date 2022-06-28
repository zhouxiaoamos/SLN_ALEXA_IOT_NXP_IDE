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
 * @file hal_log_bufferutil.h
 * @brief ACE Log BufferUtil Hal defines API implementations from the platform
 * to enable the ACE logging buffer control capabilities.
*/

#ifndef HAL_LOG_BUFFERUTIL_H
#define HAL_LOG_BUFFERUTIL_H

#include <stdint.h>
#include <stdlib.h>
#include <ace/ace_status.h>
#include <ace/hal_log.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup HAL_LOG_BUFFERUTIL_DS Log HAL BufferUtil Data Structures and Enum
 * @{
 * @ingroup ACE_HAL_LOG
 */

/**
 * @brief Log buffer output destinations that may or may not be supported by
 * the platform implementation.
 */
typedef enum {
    /** Console output */
    ACE_LOGHAL_DEST_CONSOLE = 0,
    /** Number of buffer output destinations IDs */
    ACE_LOGHAL_DEST_NUM = 1
} aceLogHal_dest_t;

/**
 * @brief Log buffer output destinations states
 */
typedef enum {
    /** Disable logging to this output destination */
    ACE_LOGHAL_DEST_STATE_OFF = 0,
    /** Enable logging to this output destination */
    ACE_LOGHAL_DEST_STATE_ON = 1,
    /** Default state of this output destination as decided
      * by the HAL implementation. This is used only by the
      * aceLogHal_setBufferOutputDestState API.
      */
    ACE_LOGHAL_DEST_STATE_DEFAULT = 2
} aceLogHal_destState_t;

/** @} */

/**
 * @defgroup HAL_LOG_BUFFERUTIL_APIs Log HAL BufferUtil APIs
 * @{
 * @ingroup ACE_HAL_LOG
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
ace_status_t aceLogHal_flushAllBuffers(uint32_t timeout_ms);

/**
 * @brief: Set the state for an output destination of a log buffer
 * @param[in] buffer_id Buffer ID of the logging buffer of interest
 * @param[in] stream_dest Desired output destination as defined in
 * aceLogHal_dest_t
 * @param[in] state Intended state of the stream_dest as defined in
 * aceLogHal_destState_t
 * @return ACE_STATUS_OK on success, otherwise one of the error codes of
 * ace_status_t.
 */
ace_status_t aceLogHal_setBufferOutputDestState(aceLogHal_id_t buffer_id,
                                                aceLogHal_dest_t stream_dest,
                                                aceLogHal_destState_t state);

/** @} */

#ifdef __cplusplus
}
#endif

#endif  // HAL_LOG_BUFFERUTIL_H
