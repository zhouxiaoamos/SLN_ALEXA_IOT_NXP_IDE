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
*@File: log_request_mempool.c
********************************************************************************
*/


#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "log_request.h"
#include "asd_mempool_mgr.h"
#include "asd_log_api.h"

int32_t asd_mempool_mgr_init(asd_mempool_mgr_t* pool, uint8_t num)
{
    if (num > ASD_MEMPOOL_SIZE_MAX || !pool)
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    pool->free_bits = BIT(num) - 1;
    pool->free_count = (int8_t) num;
    pool->size = (int8_t) num;
    return ASD_LOGGER_OK;
}

int32_t asd_mempool_index_alloc(asd_mempool_mgr_t* pool)
{
    if (!pool || !pool->size)
        return ASD_LOGGER_ERROR_UNINTIALIZED;
    if (__atomic_fetch_sub(&pool->free_count, 1, __ATOMIC_SEQ_CST) <= 0) {
        // there is no available item free, recover the free counter.
        __atomic_add_fetch(&pool->free_count, 1, __ATOMIC_SEQ_CST);
        return ASD_LOGGER_ERROR_NO_RESOURCE;
    }
    assert(__builtin_popcount(pool->free_bits) > 0);
    int32_t index = 0;
    uint32_t freebits = 0;
    do {
        freebits = __atomic_load_n(&pool->free_bits, __ATOMIC_SEQ_CST);
        index = __builtin_ffs(freebits);
        // pool->free_count should guarantee at least one available item in pool.
        assert(index > 0);
        // convert to the index based on 0.
        index --;
        freebits = __atomic_fetch_and(&pool->free_bits, ~ BIT(index), __ATOMIC_SEQ_CST);
        // Try more if "index" was already allocated by other task before
        // __atomic_fetch_and().
    } while ((freebits & BIT(index)) == 0);
    return index;

}

void asd_mempool_index_free(asd_mempool_mgr_t* pool, uint32_t index)
{
    if (!pool || !pool->size || index >= (uint32_t) pool->size)
        return;
    uint32_t freebits = __atomic_fetch_or(&pool->free_bits, BIT(index), __ATOMIC_SEQ_CST);
    // Before free, the bit should not be set. This should not happen.
    assert((freebits & BIT(index)) == 0);

    __atomic_add_fetch(&pool->free_count, 1, __ATOMIC_SEQ_CST);
    assert(pool->free_count <= pool->size);
    assert(__builtin_popcount(pool->free_bits) <= (int) pool->size);
}

void asd_mempool_mgr_reset(asd_mempool_mgr_t* pool)
{
    if (!pool || !pool->size)
        return;
    asd_mempool_mgr_init(pool, pool->size);
}

