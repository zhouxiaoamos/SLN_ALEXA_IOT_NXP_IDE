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
 * AFW circular buffer
 *******************************************************************************
 */

/**
* @file
*
* This is an implementation of a circular buffer
*
*/

#ifndef AFW_CIR_BUF_H_
#define AFW_CIR_BUF_H_

#include <stdint.h>
/* FreeRTOS includes */
#include <FreeRTOS.h>
#include <semphr.h>

typedef struct {
    uint8_t * buf_ptr;
    uint32_t size;
    uint32_t ridx;
    uint32_t widx;
    uint32_t free_size;
    SemaphoreHandle_t rd_mutex;
    SemaphoreHandle_t wr_mutex;
    SemaphoreHandle_t rd_data_avail_sema;
    SemaphoreHandle_t wr_space_avail_sema;
} afw_cir_buf_t;


/**
* @brief Used to initialize circular buffer
*
* @param[in] size size of the buffer
*
* @return pointer to the newly created and initialized circular buffer, NULL on failure
*
* Example Usage:
* @code
*    afw_cir_buf_t* cir_buf = afw_cir_buf_init(32);
* @endcode
*/
afw_cir_buf_t* afw_cir_buf_init(uint32_t size);

/**
* @brief Used to write data to a circular buffer. unlocked version.
*        Can be called ONLY by single-produser.
*        NOT Thread-safe
*
* @param[out] cir_buf pointer to the circular buffer
* @param[in]  data pointer to the source data buffer
* @param[in]  data_len length of data in bytes to be written to the circular buffer
*
* @return 0 on success, negative number on failure
*
* Example Usage:
* @code
*  int32_t err = afw_cir_buf_write(cir_buf, data, 32);
* @endcode
*/
int32_t afw_cir_buf_write_nolock(afw_cir_buf_t* cir_buf, const uint8_t* data, uint32_t data_len);

/**
* @brief Used to write data to a circular buffer
*        Can be called by multiple-produser.
*        Thread-safe
*
* @param[out] cir_buf pointer to the circular buffer
* @param[in]  data pointer to the source data buffer
* @param[in]  data_len length of data in bytes to be written to the circular buffer
*
* @return 0 on success, negative number on failure
*
* Example Usage:
* @code
*  int32_t err = afw_cir_buf_write(cir_buf, data, 32);
* @endcode
*/
int32_t afw_cir_buf_write(afw_cir_buf_t* cir_buf, const uint8_t* data, uint32_t data_len);

/**
* @brief Used to write data to a circular buffer. Waits indefinitely untill there's space to write
*        Can be called by multiple-producer.
*        Thread-safe
*
* @param[out] cir_buf pointer to the circular buffer
* @param[in]  data pointer to the source data buffer
* @param[in]  data_len length of data in bytes to be written to the circular buffer
* @param[in]  xTicksToWait ticks to timeout the block.
*
* @return 0 on success, negative number on failure
*
* Example Usage:
* @code
*  int32_t err = afw_cir_buf_write_block(cir_buf, data, 32);
* @endcode
*/
int32_t afw_cir_buf_write_block(afw_cir_buf_t* cir_buf, const uint8_t* data, uint32_t data_len, TickType_t xTicksToWait);

/**
* @brief Used to peek data from the circular buffer, the head for read won't move.
*        Can be called by multiple-consumer.
*        Thread-safe
*
* @param[in]  cir_buf pointer to the circular buffer
* @param[out] data pointer to the destination data buffer
* @param[in]  data_len length of data in bytes to be read from the circular buffer
*
* @return 0 on success, negative number on failure
*
* Example Usage:
* @code
*  int32_t err = afw_cir_buf_peek(cir_buf, data, 4);
* @endcode
*/
int32_t afw_cir_buf_peek(afw_cir_buf_t* cir_buf, uint8_t* data, uint32_t data_len);

/**
* @brief Used to peek data from the circular buffer. This is unlocked version. No mutex involes.
*        The head for read won't move.  Only be called by single-consumer.
*        NOT thread-safe
*
*
* @param[in]  cir_buf pointer to the circular buffer
* @param[out] data pointer to the destination data buffer
* @param[in]  data_len length of data in bytes to be read from the circular buffer
*
* @return 0 on success, negative number on failure
*
* Example Usage:
* @code
*  int32_t err = afw_cir_buf_peek(cir_buf, data, 4);
* @endcode
*/
int32_t afw_cir_buf_peek_nolock(afw_cir_buf_t *cir_buf, uint8_t *data, uint32_t data_len);

/**
* @brief Used to read data from the circular buffer.
*        Can be called by multiple-consumer.
*        Thread-safe
*
* @param[in]  cir_buf pointer to the circular buffer
* @param[out] data pointer to the destination data buffer
* @param[in]  data_len length of data in bytes to be read from the circular buffer
*
* @return 0 on success, negative number on failure
*
* Example Usage:
* @code
*  int32_t err = afw_cir_buf_read(cir_buf, data, 32);
* @endcode
*/
int32_t afw_cir_buf_read(afw_cir_buf_t* cir_buf, uint8_t* data, uint32_t data_len);

/**
* @brief Used to read data from the circular buffer. Unlocked version.
*        It should be only called for single consumer.
*        NOT thread-safe.
*
* @param[in]  cir_buf pointer to the circular buffer
* @param[out] data pointer to the destination data buffer
* @param[in]  data_len length of data in bytes to be read from the circular buffer
*
* @return 0 on success, negative number on failure
*
* Example Usage:
* @code
*  int32_t err = afw_cir_buf_read(cir_buf, data, 32);
* @endcode
*/
int32_t afw_cir_buf_read_nolock(afw_cir_buf_t *cir_buf, uint8_t *data, uint32_t data_len);

/**
* @brief Used to read data from the circular buffer. Waits indefinitely if there's no data to read.
*        Can be called by multiple-consumer.
*        Thread-safe
*
* @param[in]  cir_buf pointer to the circular buffer
* @param[out] data pointer to the destination data buffer
* @param[in]  data_len length of data in bytes to be read from the circular buffer
* @param[in]  xTicksToWait ticks to timeout the block.
*
* @return 0 on success, negative number on failure
*
* Example Usage:
* @code
*  int32_t err = afw_cir_buf_read_block(cir_buf, data, 32);
* @endcode
*/
int32_t afw_cir_buf_read_block(afw_cir_buf_t* cir_buf, uint8_t* data, uint32_t data_len, TickType_t xTicksToWait);

/**
* @brief Used to get the remaining/free space within a circular buffer. Thread-safe
*
* @param[in] cir_buf pointer to the circular buffer
*
* @return remaining space in bytes on success, negative number on failure
*
* Example Usage:
* @code
* int32_t len =  afw_cir_buf_get_rem_size(cir_buf);
* @endcode
*/
int32_t afw_cir_buf_get_rem_size(afw_cir_buf_t* cir_buf);

/**
* @brief Used to get the size of valid data within a circular buffer. Thread-safe.
*
* @param[in] cir_buf pointer to the circular buffer
*
* @return valid data in bytes on success, negative number on failure
*
* Example Usage:
* @code
* int32_t len =  afw_cir_buf_get_data_size(cir_buf);
* @endcode
*/
int32_t afw_cir_buf_get_data_size(afw_cir_buf_t* cir_buf);

/**
* @brief Used to deinit a existing circular buffer
*
* @param[in] cir_buf reference to the circular buffer pointer
*
* @return 0 on success, negative number on failure
*
* Example Usage:
* @code
* int32_t err =  afw_cir_buf_get_deinit(&cir_buf_ptr);
* @endcode
*/
int32_t afw_cir_buf_deinit(afw_cir_buf_t** cir_buf);

#endif /* AFW_UTILS_H_ */
