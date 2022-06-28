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
 * AFW Key Value Store (KVS) Interface
 *******************************************************************************
 */

/**
 * @file afw_kvs.h
 *
 * Provides an interface store/retrieve Key Value Pairs
 */
#pragma once

#include "stdlib.h"
#include "afw_error.h"
#include "stdint.h"
#include "stdbool.h"
#include <flash_manager.h>

#include "sys/queue.h"

// #define KVS_DEBUG_PRI_1
// #define KVS_DEBUG_PRI_2

#ifndef AMAZON_TESTS_ENABLE
#define _KVS_DEBUG_MSG_PRINT(_str, ...) \
    printf("KVS_L| %s | %s: %d (%s) | "#_str"\n", pcTaskGetName(NULL), &__FILE__[69], __LINE__, __func__, ##__VA_ARGS__)
#else
#define _KVS_DEBUG_MSG_PRINT(_str, ...) \
    printf("KVS_L| %s | %s: %d (%s) | "#_str"\n", pcTaskGetName(NULL), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif

#ifdef KVS_DEBUG_PRI_1
#define KVS_DEBUG_1(_str, ...) _KVS_DEBUG_MSG_PRINT(_str, ##__VA_ARGS__)
#else
#define KVS_DEBUG_1(_str, ...)
#endif

#ifdef KVS_DEBUG_PRI_2
#define KVS_DEBUG_2(_str, ...) _KVS_DEBUG_MSG_PRINT(_str, ##__VA_ARGS__)
#else
#define KVS_DEBUG_2(_str, ...)
#endif

#define AFW_KVS_MAX_KEY_LEN 48
#define AFW_KVS_MAX_NAMESPACE_LEN 16
#define AFW_KVS_MAX_VALUE_LEN 7680
#define MAX_MODULE_LEN      32

/**
 * @brief Set a value in Key Value Store
 *
 * Stores a value for the key in the corresponding namespace. If
 * the value exists already, that entry will be invalidated and
 * the new entry will be appended to the end of the partition.
 * If the partition is full, a cleanup will be attempted first.
 * Operation is thread safe.
 *
 * @param namespace: Must by <= 16 bytes
 * @param key: Must be <= 48 bytes
 * @param value: Byte array Must be <= 8130 bytes
 * @param val_len: Size of value object passed in
 * @param encrypted: Boolean for storing value encrypted
 *
 * @return int32_t AFW_OK or negative afw error code
 */
int32_t afw_kvs_set(const char *namespace,
                    const char *key,
                    const void *value,
                    uint16_t val_len,
                    bool encrypted);

/**
 * @brief Retrieved a value for a corresponding namespace and key
 *
 * Operation is thread safe.
 *
 * @param namespace: group that key/value belong to
 * @param key: Corresponding to value
 * @param value: Buffer to store value into
 * @param val_len: length of the buffer provided. val_len must
 *                be less than the actual size of the buffer
 *
 * @return int32_t Returns size of value returned or error code
 */
int32_t afw_kvs_get(const char *namespace,
                    const char *key,
                    void *value,
                    uint16_t val_len);

/**
 * @brief Return value size for corresponding namespace and key
 *
 * Operation is thread safe.
 *
 * @param namespace: group that key/value belong to
 * @param key: Corresponding to value
 *
 * @return int32_t Length of corresponding value or error code
 */
int32_t afw_kvs_get_size(const char* namespace,
                         const char* key);

/**
 * @brief Populate buffer with all keys for a given namespace
 *
 * Caller provides a buffer that is zero-initialized. Keys will be
 * placed contiguously in buffer and delineated by a null character
 *
 * Operation is thread safe.
 *
 * The buffer and key_cb are optional arguments. If they are both there, both will
 * be used. If only one is there only that one will be used. If neither is there,
 * will return early and error
 *
 * @param namespace: group that key/value belong to
 * @param buffer: Zero-initialized buffer to fill with keys
 * @param size: size of buffer
 * @param key_cb: a callback that will be called with each key, modelled from aceKeyValueDsHal_listKeys_cb
 *
 * @return int32_t Length of corresponding value or error code
 */
typedef int (*afw_key_cb)(const char* key_name);
int32_t afw_kvs_get_all_keys_with_callback(const char *namespace,
                                           char *buffer,
                                           uint32_t size,
                                           afw_key_cb key_cb);
int32_t afw_kvs_get_all_keys(const char *namespace,
                             char *buffer,
                             uint32_t size);


/**
 * @brief Delete a specific entry
 *
 * Operation is thread safe.
 *
 * @param namespace: group that key/value belong to
 * @param key: Corresponding to value
 *
 * @return int32_t AFW_OK or negative afw error code
 */
int32_t afw_kvs_delete(const char *namespace,
                       const char *key);

/**
 * Delete all entries corresponding to a namespace
 *
 * Operation is thread safe.
 *
 * @param namespace: target for which all entries are to be deleted.
 *                   If the partition is not the shared partition,
 *                   the entire partition will be deleted.
 *                   If it is the shared partition, then the function
 *                   will walk through the partition, entry-by-entry,
 *                   and delete the individual entry (they will
 *                   persist in flash until cleaned up)
 *
 * @return int32_t AFW_OK or negative afw error code
 */
int32_t afw_kvs_delete_namespace(const char *namespace);

/**
 * @brief removes all invalidated entries
 *
 * Defragment the partition corresponding to a namespace by
 * removing the entries that have been invalidated. This is
 * automatically triggered by set if the entry being stored
 * won't fit in the partition. Operation is thread safe.
 *
 * @param namespace: Used to find corresponding partition to
 *                 cleanup
 *
 * @return int32_t AFW_OK or negative afw error code
 */
int32_t afw_kvs_cleanup(const char *namespace);

bool afw_kvs_is_kvs_partition(unsigned long offset);

/**
 * @brief Initialize KVS module
 *
 * @return AFW_OK or negative afw error code
 */
int32_t afw_kvs_init(unsigned char *mbedtls_aes_key);

int32_t afw_kvs_deinit(void);

/* TODO: FWPLATFORM-984: Cleanup KVS 1.0 Artifacts */
typedef struct afw_kvs_s {} afw_kvs_t;
