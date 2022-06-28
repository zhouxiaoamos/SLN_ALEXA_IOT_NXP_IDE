/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All rights reserved.
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
 * AFW Stream Interface
 *
 *******************************************************************************
 */

/**
 * @file
 *
 * Provides an interface to Streaming modules.
 *
 */

#ifndef AFW_STREAM_H
#define AFW_STREAM_H

#include "stdlib.h"
#include "afw_error.h"
#include "stdint.h"
#include "sys/queue.h"

typedef enum {
    AFW_STREAM_READY_TO_READ, /**< Stream is ready to be read */
    AFW_STREAM_READY_TO_WRITE, /**< Stream is ready to be written */
    AFW_STREAM_DATA_LOST_DUE_TO_FULL_BUFFER, /**< Stream data lost due to full buffer */
    AFW_STREAM_DISCONNECTED, /**< Stream is disconnected */
} afw_stream_event_t;

typedef struct afw_stream_table_s afw_stream_table_t;
typedef struct afw_stream_s afw_stream_t;

/**
 * @brief Write data to the module
 *
 * @param afw_stream A reference to the Module to be used
 * @param data A buffer containing the data to be written
 * @param data_len length of data
 * @returns bytes written to stream (<=data_len)  on success,
 *          an afw_error value on failure
 *
 * @remark thread-safe
 */
static inline int32_t afw_stream_write(afw_stream_t *afw_stream,
                                    const uint8_t * data,
                                    uint32_t data_len);

/**
 * @brief Read data from the module
 *
 * @param afw_stream A reference to the Module to be used
 * @param buffer A buffer for storing the data read from the stream
 * @param buffer_len length of buffer
 * @returns 0 on success, an afw_error value on failure
 *
 * @remark thread-safe
 */
static inline int32_t afw_stream_read(afw_stream_t *afw_stream,
                                    uint8_t * buffer,
                                    uint32_t buffer_len);

/**
 * @brief Get the number of bytes available to read. Reads of this number of bytes or
 * fewer will not block.
 *
 * @param afw_stream A reference to the Module to be used
 * @returns The number of bytes available, or an afw_error value on failure
 *
 * @remark thread-safe
 */
static inline int32_t afw_stream_get_bytes_available(afw_stream_t *afw_stream);

/**
 * @brief Register a callback to be called whenever the stream state changes
 *
 * @param afw_stream A reference to the Module to be used
 * @param callback A function pointer for the callback to be used
 * @param context A context object to be passed back to the callback when called
 * @return 0 on success, an afw_error on failure
 *
 * @remark thread-safe
 */
static inline int32_t afw_stream_register_event_callback(
                                afw_stream_t *afw_stream,
                                void (*callback)(void *context, afw_stream_event_t event),
                                void *context);

/**
 * @brief De-initializes the module
 *
 * This will deinitialize any resources and clear all members of the Module instance to zero.
 *
 * @param afw_led A reference to the Module to be de-initialized
 * @return 0 on success, an afw_error on failure
 */
static inline int32_t afw_stream_deinit(afw_stream_t *afw_stream);

/**
 * @brief Add Stream module to database
 *
 * @param Name to use in database
 * @param Reference to handle containing func table etc.
 * @return 0 on success, an afw_error on failure
 */
 int32_t afw_stream_db_add(char* name, afw_stream_t *afw_stream);

 /**
  * @brief Remove Stream module from database
  *
  * @param Reference to handle containing func table etc.
  * @return 0 on success, an afw_error on failure
  */
 int32_t afw_stream_db_remove(afw_stream_t *afw_stream);

 /**
  * @brief Get Stream module from database
  *
  * @param Name to look up in database
  * @param Reference to store returning Stream hndl
  * @return 0 on success, an afw_error on failure
  */
 int32_t afw_stream_db_get(char* name, afw_stream_t **afw_stream);

/*************************************************************************************************/
/********************************* INTERNAL ******************************************************/
/*************************************************************************************************/
/********** Do not rely on the following implementation details in application code***************/
/*************************************************************************************************/
struct afw_stream_s {
    afw_stream_table_t* table;
    uint32_t db_entry_hash;
    TAILQ_ENTRY(afw_stream_s) entry;
};

struct afw_stream_table_s {
    int32_t (*write)(afw_stream_t *afw_stream,
                            const uint8_t * data,
                            uint32_t data_len);
    int32_t (*read)(afw_stream_t *afw_stream,
                            uint8_t * buffer,
                            uint32_t buffer_len);
    int32_t (*get_bytes_available)(afw_stream_t *afw_stream);
    int32_t (*register_event_callback)(
                            afw_stream_t *afw_stream,
                            void (*callback)(void *context, afw_stream_event_t event),
                            void *context);
    int32_t (*deinit)(afw_stream_t *afw_stream);
};

static inline int32_t afw_stream_write(afw_stream_t *afw_stream,
                                        const uint8_t * data,
                                        uint32_t data_len)
{
    if (afw_stream == NULL || afw_stream->table == NULL)
        return -AFW_EINVAL;
    if(afw_stream->table->write == NULL)
        return -AFW_ENOTIMPL;

    return afw_stream->table->write(afw_stream, data, data_len);
}

static inline int32_t afw_stream_read(afw_stream_t *afw_stream,
                                        uint8_t * buffer,
                                        uint32_t buffer_len)
{
    if (afw_stream == NULL || afw_stream->table == NULL)
        return -AFW_EINVAL;
    if(afw_stream->table->read == NULL)
        return -AFW_ENOTIMPL;

    return afw_stream->table->read(afw_stream, buffer, buffer_len);
}

static inline int32_t afw_stream_get_bytes_available(afw_stream_t *afw_stream)
{
    if (afw_stream == NULL || afw_stream->table == NULL)
        return -AFW_EINVAL;
    if(afw_stream->table->get_bytes_available == NULL)
        return -AFW_ENOTIMPL;

    return afw_stream->table->get_bytes_available(afw_stream);
}

static inline int32_t afw_stream_register_event_callback(
                                        afw_stream_t *afw_stream,
                                        void (*callback)(void *context, afw_stream_event_t event),
                                        void *context)
{
    if (afw_stream == NULL || afw_stream->table == NULL)
        return -AFW_EINVAL;
    if(afw_stream->table->register_event_callback == NULL)
        return -AFW_ENOTIMPL;

    return afw_stream->table->register_event_callback(afw_stream, callback, context);
}

static inline int32_t afw_stream_deinit(afw_stream_t *afw_stream)
{
    if (afw_stream == NULL || afw_stream->table == NULL)
        return -AFW_EINVAL;
    if(afw_stream->table->deinit == NULL)
        return -AFW_ENOTIMPL;

    return afw_stream->table->deinit(afw_stream);
}

#endif /* AFW_STREAM_H */
