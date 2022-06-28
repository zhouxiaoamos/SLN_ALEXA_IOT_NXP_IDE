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
 * @file       ace_sys_stats.h
 * @brief      Ace system stats APIs
 *
 * This file defines APIs that provide system statistics(heap) instrumentation
 * to be queried by sys_toolbox, the APIs are RTOS oriented
 */

#ifndef ACE_SYS_STATS_H
#define ACE_SYS_STATS_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <ace/ace_status.h>

#ifndef ACE_SYS_MAX_TASK_NAME
#define ACE_SYS_MAX_TASK_NAME 8
#endif

#ifndef ACE_SYS_PROVIDER_NAME_LEN_MAX
#define ACE_SYS_PROVIDER_NAME_LEN_MAX 16
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Statistics for heap usage
 */
typedef struct _HeapStats {
    // Total heap size in bytes
    size_t  total;

    // Free heap size in bytes
    size_t  free;

    // Min free heap size in bytes since last rebase
    // represents the minimal heap free value since:
    // - device booted or
    // - last rebase call rebaseHeapStats()
    size_t  minfree;
} aceSys_heapStats_t;

/**
 * @brief       Task heap usage record
 * @details     The structure represents the heap usage breakdown on a task basis.
 *              Each record tracks aggregated heap size allocated by a particular
 *              task, the task may or may not be alive as long as the heap chunks
 *              allocated are not yet all freed up.
 */
typedef struct __taskHeapRecord_t {
    // Name of task the usage stats belong to
    char        taskname[ACE_SYS_MAX_TASK_NAME];

    // ID of task the usage stats belong to
    uint32_t    taskId;

    // Allocated heap size in bytes still in use
    size_t      alloc;

    // Peak heap size in bytes since last rebase
    size_t      peak;

    // Max chunk size allocated by the task since last rebase
    size_t      max;

    // Min chunk size allocated by the task since last rebase
    size_t      min;
} aceSys_taskHeapRecord_t;

/**
 * @brief       Heap chunk information
 */
typedef struct __heapChunkInfo_t {
    // If the chunk is allocated or free
    bool        inUse;

    // Name of task if chunk is allocated
    // all '0' if chunk is free
    char        taskname[ACE_SYS_MAX_TASK_NAME];

    // ID of task the usage stats belong to
    // 0 if chunk is free
    uint32_t    taskId;

    // Start address of the chunk
    uintptr_t   startAddr;

    // Size of the chunk
    size_t      size;
} aceSys_heapChunkInfo_t;

/**
 * @brief       Heap statistics provider
 * @details     The structure contains heap statistics query operations
 *              provided by a certain heap implementation
 */
typedef struct aceSys_heapStatsProvider {
    /**
     * @brief       Get heap stats callback
     *
     * @param[out]  stats   Pointer to a receiving object of type aceSys_heapStats_t
     *
     * @return ACE_STATUS_OK if stats is filled in successfully
     *         ACE_STATUS_NOT_SUPPORTED if operation not supported
     *         relevant error code from ace_status_t otherwise
     */
    ace_status_t (*getHeapStats)(aceSys_heapStats_t* stats);

    /**
     * @brief       Retrieve task heap record
     * @details     Retrieve heap task records in an iterator style, starts with 'begin'
     *              as true, and keeps receiving next heap task record with 'begin'
     *              as false until ACE_STATUS_STOPPED returned.
     *              The order of task heap record retrieving doesn't matter.
     *              The function will be called inside critical section.
     *
     * @param[out]      record  Pointer to a receiving object of type aceSys_taskHeapRecord_t
     * @param[in]       begin   true to start visiting the first record
     *                          false to continue with current position
     *
     * @return ACE_STATUS_OK if record is filled successfully
     *         ACE_STATUS_NOT_SUPPORTED if operation not supported
     *         ACE_STATUS_STOPPED if no more record to iterate
     *         relevant error code from ace_status_t otherwise
     */
    ace_status_t (*nextTaskHeapRecord)(aceSys_taskHeapRecord_t* record, const bool begin);

    /**
     * @brief       Retrieve heap chunk information
     * @details     Retrieve information on all heap chunks in an iterator style, starts with 'begin'
     *              as true, and keeps receiving next chunk information with 'begin'
     *              as false until ACE_STATUS_STOPPED returned.
     *              The order of heap chunks is expected to be the same with chunk layout inside heap
     *              The function will be called inside critical section.
     *
     * @param[out]      record  Pointer to a receiving object of type aceSys_heapChunkInfo_t
     * @param[in]       begin   true to start visiting the first chunk
     *                          false to continue with current position
     *
     * @return ACE_STATUS_OK if record is filled successfully
     *         ACE_STATUS_NOT_SUPPORTED if operation not supported
     *         ACE_STATUS_STOPPED if no more record to iterate
     *         relevant error code from ace_status_t otherwise
     */
    ace_status_t (*nextHeapChunkInfo)(aceSys_heapChunkInfo_t* chunkInfo, const bool begin);

    /**
     * @brief       Rebase statistic data for heap stats
     * @details     The function sets below heap stats value:
     *              - sets 'minfree' value to 'free'
     *              - sets 'peak' value to 'alloc'
     *              - sets 'max' value to 0
     *              - sets 'min' value to 0
     *
     * @return ACE_STATUS_OK if successful
     *         ACE_STATUS_NOT_SUPPORTED if operation not supported
     */
    ace_status_t (*rebaseHeapStats)(void);
} aceSys_heapStatsProvider_t;


/**
 * @brief       Register a heap stats provider
 * @details     Register heap stats provider with available callback functions implemented,
 *              shallow copy will be created for provider and name.
 *
 * @param[in]   provider  Pointer to heap stats provider with callbacks provided
 * @param[in]   name      Provider name in string no longer than ACE_SYS_PROVIDER_NAME_LEN_MAX
 *
 * @return ACE_STATUS_OK if successful, else error code from ace_status_t
 */
ace_status_t aceSys_registerHeapStatsProvider(const aceSys_heapStatsProvider_t* provider,
                                              const char* name);

/**
 * @brief       Retrieve a registered heap stats provider with known name
 * @details     Checks registered heap_stats_providers to find one that matches
 *              the passed in name. If one is found, will set provider pointer
 *
 * @param[out]  provider   Pointer to heap stats provider with matching name
 *                         if one is found.
 * @param[in]   name       Provider name in string no longer than ACE_SYS_PROVIDER_NAME_LEN_MAX.
 *                         If the name is NULL, will return the first available provider.
 *
 * @return ACE_STATUS_OK if successful, else error code from ace_status_t
 */
ace_status_t aceSys_retrieveHeapStatsProvider(aceSys_heapStatsProvider_t* provider,
    const char* name);

#ifdef __cplusplus
}
#endif

#endif // ACE_SYS_STATS_H

