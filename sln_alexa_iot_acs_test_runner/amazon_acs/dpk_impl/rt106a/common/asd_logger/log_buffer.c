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
* Implementation of ram ring buffer.
*@File: asd_log_buffer.c
********************************************************************************
*/

#include "logger_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "message_buffer.h"
#include "asd_log_msg.h"
#include "log_buffer.h"
#include "asd_logger_internal_config.h"
#include "afw_cir_buf.h"

// Support single consumer + single producer
// Or  single consumer + multiple producers.
// Support ISR.
typedef struct {
    afw_stream_t stream;
    afw_cir_buf_t *cir_buf;
}asd_log_buffer_t;

static int32_t single_producer_write(afw_stream_t *logbuf_stream, const uint8_t * data, uint32_t data_len);
static int32_t multi_producers_write(afw_stream_t *logbuf_stream, const uint8_t * data, uint32_t data_len);
static int32_t single_consumer_read_one_message(afw_stream_t *logbuf_stream, uint8_t * buf, uint32_t buf_len);
static int32_t get_bytes_available(afw_stream_t *logbuf_stream);
static int32_t deinit(afw_stream_t *logbuf_stream);


static afw_stream_table_t log_buffer_stream_table_single_producer = {
    &single_producer_write,
    &single_consumer_read_one_message,
    &get_bytes_available,
    NULL,
    &deinit
};

static afw_stream_table_t log_buffer_stream_table_multi_producers = {
    &multi_producers_write,
    &single_consumer_read_one_message,
    &get_bytes_available,
    NULL,
    &deinit
};

// ============================ static functions ===============================

/**
 * @brief stream write for single producer, no mutex required. It writes the whole
 *        data or nothing.
 * @param [in] logbuf_stream: stream pointer, e.g. log buffer pointer
 * @param [in] data: data pointer for write.
 * @param [in] data_len: data length
 *
 * @return data length written, or negative for failure(afw_error).
 */
static int32_t single_producer_write(afw_stream_t *logbuf_stream, const uint8_t * data, uint32_t data_len)
{
    asd_log_buffer_t *logbuf = (asd_log_buffer_t*) logbuf_stream;
    if (!logbuf || !logbuf->cir_buf) return -AFW_EINVAL;
    int rc = afw_cir_buf_write_nolock(logbuf->cir_buf, data, data_len);
    if (rc < 0) {
        return rc;
    } else {
        return data_len;
    }
}

/**
 * @brief stream write for multiple producers, use critical section to support calling
 *        from ISR. It writes the whole data or nothing.
 * @param [in] logbuf_stream: stream pointer, e.g. log buffer pointer
 * @param [in] data: data pointer for write.
 * @param [in] data_len: data length
 *
 * @return data length written, or negative for failure (afw_error).
 */
static int32_t multi_producers_write(afw_stream_t *logbuf_stream, const uint8_t * data, uint32_t data_len)
{
    asd_log_buffer_t *logbuf = (asd_log_buffer_t*) logbuf_stream;
    if (!logbuf || !logbuf->cir_buf) return -AFW_EINVAL;

    int32_t ret = afw_cir_buf_write_block(logbuf->cir_buf, data, data_len, pdMS_TO_TICKS(ASD_LOGGER_ENQUEUE_TIMEOUT_MS));


    return ret;
}

/**
 * @brief stream read for single consumer. Make sure read one whole message or nothing.
 * @param [in] logbuf_stream: stream pointer, e.g. log buffer pointer
 * @param [out] buf: data pointer for read.
 * @param [in] buf_len: data length
 *
 * @return data length read, or negative for failure(afw_error).
 */
static int32_t single_consumer_read_one_message(afw_stream_t *logbuf_stream, uint8_t * buf, uint32_t buf_len)
{
    asd_log_buffer_t *logbuf = (asd_log_buffer_t*) logbuf_stream;
    if (!logbuf || !logbuf->cir_buf) return -AFW_EINVAL;
    //
    int available_len = afw_cir_buf_get_data_size(logbuf->cir_buf);
    // must be at least one message header length (4 bytes).
    if (available_len < (int)sizeof(asd_log_msg_t)) {
        // no available message.
        return -AFW_EIO;
    }
    asd_log_msg_t msg_header = {0};
    int ret;
    if ((ret = afw_cir_buf_peek_nolock(logbuf->cir_buf, (uint8_t*) &msg_header, sizeof(msg_header))) < 0) {
        return ret;
    }
    if (available_len < msg_header.length) {
        // not enough data read for the whole message.
        return -AFW_EIO;
    }
    if ((ret = afw_cir_buf_read_nolock(logbuf->cir_buf, buf, msg_header.length)) < 0) {
        return ret;
    }
    return msg_header.length;
}

static int32_t get_bytes_available(afw_stream_t *logbuf_stream)
{
    asd_log_buffer_t *logbuf = (asd_log_buffer_t*) logbuf_stream;
    if (!logbuf || !logbuf->cir_buf) return -AFW_EINVAL;
    return afw_cir_buf_get_data_size(logbuf->cir_buf);

}

static int32_t deinit(afw_stream_t *logbuf_stream)
{
    asd_log_buffer_t *logbuf = (asd_log_buffer_t*) logbuf_stream;
    if (!logbuf || !logbuf->cir_buf) return -AFW_EINVAL;
    afw_cir_buf_deinit(&logbuf->cir_buf);
    free(logbuf);
    return 0;
}


// ======================= API functions ================================

log_buffer_handle_t log_buffer_create(int buffer_size, uint32_t option)
{
    if (!buffer_size) return NULL;
    asd_log_buffer_t *logbuf = malloc(sizeof(asd_log_buffer_t));
    if (!logbuf) {
        LOGGER_DPRINTF("logbuf malloc failure.");
        return NULL;
    }

    logbuf->stream.table = (option & MULTI_PRODUCERS_BIT)?
        &log_buffer_stream_table_multi_producers : &log_buffer_stream_table_single_producer;
    logbuf->cir_buf = afw_cir_buf_init(buffer_size);
    if (!logbuf->cir_buf) {
        free(logbuf);
        LOGGER_DPRINTF("cir_buf malloc failure.");
        return NULL;
    }
    return (log_buffer_handle_t) logbuf;
}

int32_t log_buffer_read(log_buffer_handle_t logbuf,
                     uint8_t* buffer,
                     size_t buffer_len)
{
    asd_log_buffer_t *log_buf = (asd_log_buffer_t*) logbuf;
    if (!log_buf || !log_buf->cir_buf) return -AFW_EINVAL;
    return afw_stream_read(&log_buf->stream, buffer, buffer_len);
}

int32_t log_buffer_write(log_buffer_handle_t logbuf,
                     const uint8_t* data,
                     size_t data_len)
{
    asd_log_buffer_t *log_buf = (asd_log_buffer_t*) logbuf;
    if (!log_buf || !log_buf->cir_buf) return -AFW_EINVAL;
    return afw_stream_write(&log_buf->stream, data, data_len);
}

int32_t log_buffer_get_free_size(log_buffer_handle_t logbuf)
{
    asd_log_buffer_t *log_buf = (asd_log_buffer_t*) logbuf;
    if (!log_buf || !log_buf->cir_buf) return -AFW_EINVAL;
    return afw_stream_get_bytes_available(&log_buf->stream);
}

void log_buffer_destroy(log_buffer_handle_t logbuf)
{
    asd_log_buffer_t *log_buf = (asd_log_buffer_t*) logbuf;
    if(!logbuf) return;
    afw_stream_deinit(&log_buf->stream);
}


