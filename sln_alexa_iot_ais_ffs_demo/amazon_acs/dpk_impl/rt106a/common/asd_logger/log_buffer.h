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
* The header for RAM ring log buffer.
*
*@File: log_buffer.h
********************************************************************************
*/

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "asd_log_api.h"
#include "afw_stream.h"
#ifdef __cplusplus
extern "C"
{
#endif

//ram ring buffer supports single consumer only.
// single or multi producers.

enum {
    MULTI_PRODUCERS_BIT = 1,
};

typedef void* log_buffer_handle_t;

/**
 * @brief retrieve the stream pointer of log buffer. Used for stream operation.
 * @param [in] logbuf: log buffer handle
 *
 * @return stream pointer of log buffer.
 */
#define log_buffer_get_stream(logbuf) ((afw_stream_t*) logbuf)

/**
 * @brief create a log buffer.
 * @param [in] buffer_size: size of log buffer.
 * @param [in] option: multi-producer(MULTI_PRODUCERS_BIT) or single producer(0).
 *
 * @return handle of new created log buffer.
 */
log_buffer_handle_t log_buffer_create(int buffer_size, uint32_t option);


/**
 * @brief destroy a log buffer.
 * @param [in] logbuf: log buffer handle
 *
 * @return none.
 */
void log_buffer_destroy(log_buffer_handle_t logbuf);

/**
 * @brief read log buffer.
 * @param [in] logbuf: log buffer handle
 * @param [out] buffer: data pointer for filling read data.
 * @param [in] buffer_len: buffer length, max data to read.
 *
 * @return data length read, or negative for failure(afw_error).
 */
int32_t log_buffer_read(log_buffer_handle_t logbuf,
                     uint8_t* buffer,
                     size_t buffer_len);

/**
 * @brief write log buffer.
 * @param [in] logbuf: log buffer handle
 * @param [in] data: data pointer for write
 * @param [in] data_len: data length
 *
 * @return data length written, or negative for failure(afw_error).
 */
int32_t log_buffer_write(log_buffer_handle_t logbuf,
                     const uint8_t* data,
                     size_t data_len);

/**
 * @brief get the free space in log buffer.
 * @param [in] logbuf: log buffer handle
 *
 * @return available free space in log buffer, unit is byte.
 */
int32_t log_buffer_get_free_size(log_buffer_handle_t logbuf);

#ifdef __cplusplus
}
#endif
