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
* The header for memory pool.
*
*@File: asd_mempool_mgr.h
********************************************************************************
*/

#pragma once

#include "logger_port.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif
// mempool manager only support up to 32 items in a pool. The implementation
// use a uint32_t for the bitmap of free list, which limit the maximum size to 32.
#define ASD_MEMPOOL_SIZE_MAX     (32)

typedef struct {
    int8_t size;          ///< size of mempool, how many items in the pool.
    int8_t free_count;    ///< count of free items currently in the pool.
    uint32_t free_bits;   ///< bit map of free items in the pool.
} asd_mempool_mgr_t;

/**
 * @brief Initialize mempool manager. The pool support up to ASD_MEMPOOL_SIZE_MAX items.
 *        The manager doesn't allocate the actual memory pool. It just manages
 *        the pool, e.g. provide API to allocate/free index of item. The memory pool
 *        is allocated seperately by user, and it can be an array or memory block.
 *
 * @param [in/out] pool: memory pool manager pointer.
 * @param [in] num: number of items managed by pool manager.
 *
 * @return Negative for failure, ASD_LOGGER_OK for success. See ASD_LOGGER_ERROR...
 */
int32_t asd_mempool_mgr_init(asd_mempool_mgr_t* pool, uint8_t num);



/**
 * @brief Provide an item index that map to an item from memory pool.
 *
 * @param [in] pool: memory pool manager pointer.
 *
 * @return ASD_LOGGER_ERROR_UNINTIALIZED: pool is not initialized;
 *         ASD_LOGGER_ERROR_NO_RESOURCE: pool is empty, no free item;
 *         >=0: index of item allocated.
 */
int32_t asd_mempool_index_alloc(asd_mempool_mgr_t* pool);

/**
 * @brief Free the item index that map to an item from memory pool.
 *
 * @param [in] pool: memory pool manager pointer.
 * @param [in] index: the item index that map to an item from memory pool. The
 *                    index must be a number in [0, pool->size-1].
 *
 * @return None.
 */
void asd_mempool_index_free(asd_mempool_mgr_t* pool, uint32_t index);


/**
 * @brief Reset pool manager, free all indexes.
 *
 * @param [in] pool: memory pool manager pointer.
 *
 * @return None.
 */
void asd_mempool_mgr_reset(asd_mempool_mgr_t* pool);

#ifdef __cplusplus
}
#endif
