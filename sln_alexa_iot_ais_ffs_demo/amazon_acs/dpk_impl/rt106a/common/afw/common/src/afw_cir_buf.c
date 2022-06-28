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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include <portmacro.h>
#include "semphr.h"
#include "afw_cir_buf.h"
#include "afw_error.h"
#include "afw_utils.h"


#define CIR_BUF_DEBUG 0


afw_cir_buf_t* afw_cir_buf_init(uint32_t size)
{
    if (size <= 0)
        return NULL;

    afw_cir_buf_t *cir_buf = (afw_cir_buf_t*)calloc(1, sizeof(afw_cir_buf_t));
    if (!cir_buf)
        return NULL;

    cir_buf->buf_ptr = (uint8_t*)malloc(size);
    if (!cir_buf->buf_ptr) {
        free(cir_buf);
        return NULL;
    }
    cir_buf->rd_mutex = xSemaphoreCreateMutex();
    if (!cir_buf->rd_mutex) {
        afw_cir_buf_deinit(&cir_buf);
        return NULL;
    }

    cir_buf->wr_mutex = xSemaphoreCreateMutex();
    if (!cir_buf->wr_mutex) {
        afw_cir_buf_deinit(&cir_buf);
        return NULL;
    }

    cir_buf->rd_data_avail_sema = xSemaphoreCreateBinary();
    if (!cir_buf->rd_data_avail_sema) {
        afw_cir_buf_deinit(&cir_buf);
        return NULL;
    }
    cir_buf->wr_space_avail_sema = xSemaphoreCreateBinary();
    if (!cir_buf->wr_space_avail_sema) {
        afw_cir_buf_deinit(&cir_buf);
        return NULL;
    }

    cir_buf->size = size;
    cir_buf->free_size = size;
    cir_buf->ridx = 0;
    cir_buf->widx = 0;

    return cir_buf;
}

int32_t afw_cir_buf_write_nolock(afw_cir_buf_t *cir_buf, const uint8_t *data, uint32_t data_len)
{
    uint32_t copy_len, rem_data_len;
    const uint8_t* src_ptr;
    uint32_t free_len;
    if (!(cir_buf) || !(data) || !(data_len))
        return -AFW_EINVAL;

    __atomic_load(&cir_buf->free_size, &free_len, __ATOMIC_SEQ_CST);

    if (data_len > free_len) {
        // no space for write whole data.
        return -AFW_EIO;
    }

    rem_data_len = data_len;

    do {
        copy_len = ( (cir_buf->widx + rem_data_len) > cir_buf->size ) ? (cir_buf->size - cir_buf->widx) :
                   rem_data_len;
        src_ptr = data + (data_len - rem_data_len);
        memcpy(&cir_buf->buf_ptr[cir_buf->widx], src_ptr, copy_len);
        rem_data_len = rem_data_len - copy_len;
        cir_buf->widx = (cir_buf->widx + copy_len) % cir_buf->size;
    } while (rem_data_len);

    __atomic_sub_fetch(&cir_buf->free_size, data_len, __ATOMIC_SEQ_CST);

    if (afw_cir_buf_get_data_size(cir_buf)) {
        SEMA_GIVE(cir_buf->rd_data_avail_sema);
    }
#if CIR_BUF_DEBUG
    printf("write: ridx = %d, widx = %d, free_space = %d\r\n", cir_buf->ridx, cir_buf->widx,
           cir_buf->free_size);
#endif
    return 0;
}

int32_t afw_cir_buf_write_block(afw_cir_buf_t* cir_buf, const uint8_t* data, uint32_t data_len, TickType_t xTicksToWait)
{
    int32_t rc = -AFW_EINVAL;

    if (!(cir_buf) || !(data) || !(data_len))
        return -AFW_EINVAL;

    if (data_len > cir_buf->size)
        return -AFW_EIO;

    BaseType_t sema_retv;
    while(1) {
        SEMA_TAKE(cir_buf->wr_mutex, xTicksToWait, sema_retv);
        if (sema_retv != pdTRUE) {
            rc = -AFW_ETIMEOUT;
            break;
        }
        rc = afw_cir_buf_write_nolock(cir_buf, data, data_len);

        if (rc == -AFW_EIO) {
            // no space for the whole data write.
            SEMA_GIVE(cir_buf->wr_mutex);
            /* Wait on free space to write */
            SEMA_TAKE(cir_buf->wr_space_avail_sema, xTicksToWait, sema_retv);
            if (sema_retv != pdTRUE) {
                rc = -AFW_ETIMEOUT;
                break;
            }
        } else {

            SEMA_GIVE(cir_buf->wr_mutex);
            break;
        }
    }

    return rc;
}

int32_t afw_cir_buf_write(afw_cir_buf_t* cir_buf, const uint8_t* data, uint32_t data_len)
{
    int32_t rc;

    if (!(cir_buf) || !(data) || !(data_len))
        return -AFW_EINVAL;

    BaseType_t sema_retv;
    SEMA_TAKE(cir_buf->wr_mutex, portMAX_DELAY, sema_retv);
    rc = afw_cir_buf_write_nolock(cir_buf, data, data_len);
    SEMA_GIVE(cir_buf->wr_mutex);
    return rc;
}

int32_t afw_cir_buf_peek_nolock(afw_cir_buf_t *cir_buf, uint8_t *data, uint32_t data_len)
{
    uint32_t copy_len, rem_data_len;
    uint8_t* dst_ptr;
    uint32_t free_len;
    if (!(cir_buf) || !(data) || !(data_len))
        return -AFW_EINVAL;

    __atomic_load(&cir_buf->free_size, &free_len, __ATOMIC_SEQ_CST);
    if (data_len > (cir_buf->size - free_len)) {
        // no enough data to read.
        return -AFW_EIO;
    }

    uint32_t ridx = cir_buf->ridx;
    rem_data_len = data_len;
    do {
        copy_len = ( (ridx + rem_data_len) > cir_buf->size ) ? (cir_buf->size - ridx) :
                   rem_data_len;
        dst_ptr = data + (data_len - rem_data_len);
        memcpy(dst_ptr, &cir_buf->buf_ptr[ridx], copy_len);
        rem_data_len = rem_data_len - copy_len;
        ridx = (ridx + copy_len) % cir_buf->size;
    } while (rem_data_len);
    return 0;
}


int32_t afw_cir_buf_peek(afw_cir_buf_t* cir_buf, uint8_t* data, uint32_t data_len)
{
    int32_t rc;

    if (!(cir_buf) || !(data) || !(data_len))
        return -AFW_EINVAL;

    BaseType_t sema_retv;
    SEMA_TAKE(cir_buf->rd_mutex, portMAX_DELAY, sema_retv);
    rc = afw_cir_buf_peek_nolock(cir_buf, data, data_len);
    SEMA_GIVE(cir_buf->rd_mutex);

    return rc;
}

int32_t afw_cir_buf_read_nolock(afw_cir_buf_t *cir_buf, uint8_t *data, uint32_t data_len)
{

    if (!(cir_buf) || !(data) || !(data_len))
        return -AFW_EINVAL;

    int32_t rc = afw_cir_buf_peek_nolock(cir_buf, data, data_len);
    if (rc < 0) return rc;

    // update the free_size and ridx;
    cir_buf->ridx = (cir_buf->ridx + data_len) % cir_buf->size;
    __atomic_add_fetch(&cir_buf->free_size, data_len, __ATOMIC_SEQ_CST);

    if (afw_cir_buf_get_rem_size(cir_buf)) {
        SEMA_GIVE(cir_buf->wr_space_avail_sema);
    }
#if CIR_BUF_DEBUG
    printf("read: ridx = %d, widx = %d, free_space = %d\r\n", cir_buf->ridx, cir_buf->widx,
           cir_buf->free_size);
#endif
    return 0;
}


int32_t afw_cir_buf_read_block(afw_cir_buf_t* cir_buf, uint8_t* data, uint32_t data_len, TickType_t xTicksToWait)
{
    int32_t rc = -AFW_EINVAL;

    if (!(cir_buf) || !(data) || !(data_len))
        return -AFW_EINVAL;

    if (data_len > cir_buf->size)
        return -AFW_EIO;
    BaseType_t sema_retv;
    while (1) {
        SEMA_TAKE(cir_buf->rd_mutex, xTicksToWait, sema_retv);
        if (sema_retv != pdTRUE) {
            rc = -AFW_ETIMEOUT;
            break;
        }
        rc = afw_cir_buf_read_nolock(cir_buf, data, data_len);

        if (rc == -AFW_EIO) {
            // no enough data available.
            SEMA_GIVE(cir_buf->rd_mutex);
            /* Wait on data. If there's data to read wake up */
            SEMA_TAKE(cir_buf->rd_data_avail_sema, xTicksToWait, sema_retv);
            if (sema_retv != pdTRUE) {
                rc = -AFW_ETIMEOUT;
                break;
            }
        } else {
            SEMA_GIVE(cir_buf->rd_mutex);
            break;
        }
    }
    return rc;
}

int32_t afw_cir_buf_read(afw_cir_buf_t* cir_buf, uint8_t* data, uint32_t data_len)
{
    int32_t rc;

    if (!(cir_buf) || !(data) || !(data_len))
        return -AFW_EINVAL;
    BaseType_t sema_retv;
    SEMA_TAKE(cir_buf->rd_mutex, portMAX_DELAY, sema_retv);
    rc = afw_cir_buf_read_nolock(cir_buf, data, data_len);
    SEMA_GIVE(cir_buf->rd_mutex);

    return rc;
}

int32_t afw_cir_buf_get_rem_size(afw_cir_buf_t* cir_buf)
{
    if (!cir_buf)
        return -AFW_EINVAL;
    uint32_t free_len;
    __atomic_load(&cir_buf->free_size, &free_len, __ATOMIC_SEQ_CST);

    return free_len;
}

int32_t afw_cir_buf_get_data_size(afw_cir_buf_t* cir_buf)
{
    if (!cir_buf)
        return -AFW_EINVAL;
    uint32_t free_len;
    __atomic_load(&cir_buf->free_size, &free_len, __ATOMIC_SEQ_CST);
    return (int32_t)(cir_buf->size - free_len);
}

int32_t afw_cir_buf_deinit(afw_cir_buf_t** cir_buf)
{
    if (!*(cir_buf))
        return -AFW_EINVAL;

    if ((*cir_buf)->rd_mutex) {
        vSemaphoreDelete((*cir_buf)->rd_mutex);
    }
    if ((*cir_buf)->wr_mutex) {
        vSemaphoreDelete((*cir_buf)->wr_mutex);
    }
    if ((*cir_buf)->rd_data_avail_sema) {
        vSemaphoreDelete((*cir_buf)->rd_data_avail_sema);
    }
    if ((*cir_buf)->wr_space_avail_sema) {
        vSemaphoreDelete((*cir_buf)->wr_space_avail_sema);
    }
    free((*cir_buf)->buf_ptr);
    free(*cir_buf);

    *cir_buf = NULL;

    return 0;
}
