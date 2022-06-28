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

/** @file
 *  @brief Implements the hal_kv_storage.h interface.
 */

#include <ace/hal_kv_storage.h>
#include "afw_def.h"
#include "flash_map.h"
#include "afw_kvs.h"
#include "flash_manager.h"
#include "asd_log_platform_api.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifdef UNIT_TESTS
#include <mock_flash_manager.h>
#endif

#define DOT '.'

static ace_status_t convert_return_code(int32_t afw_ret) {
    switch(afw_ret){
        case AFW_OK:
            return ACE_STATUS_OK;
        case -AFW_ENOENT:
        case -AFW_ENODEV:
        case -AFW_EBADF:
            return ACE_STATUS_FAILURE_UNKNOWN_FILE;
        case -AFW_EINVAL:
            return ACE_STATUS_BAD_PARAM;
        case -AFW_ENOMEM:
            return ACE_STATUS_BUFFER_OVERFLOW;
        case -AFW_EBUSY:
            return ACE_STATUS_BUSY;
        case -AFW_EUFAIL:
        case -AFW_EINTRL:
        default:
            return ACE_STATUS_GENERAL_ERROR;
    }
}

static bool is_group_valid(const char **group) {
    if (!group) {
        return false;
    }

    // the default group is as if no group
    if (*group && !strncmp(ACE_KVS_DS_DEFAULT_GROUP, *group, strlen(*group) + 1)) {
        *group = NULL;
    }

    // If no group, then we use default group so this is valid case
    if (!*group) {
        return true;
    }

    static fm_flash_partition_t *kvsShared = NULL;
    static fm_flash_partition_t *kvsBackup = NULL;

    fm_flash_partition_t *part = fm_flash_get_partition(*group);
    if (!__atomic_load_n(&kvsShared, __ATOMIC_SEQ_CST)) {
        __atomic_store_n(&kvsShared,
                         fm_flash_get_partition(FLASH_PARTITION_KVS_SHARED),
                         __ATOMIC_SEQ_CST);
    }
    if (!__atomic_load_n(&kvsBackup, __ATOMIC_SEQ_CST)) {
        __atomic_store_n(&kvsBackup,
                         fm_flash_get_partition(FLASH_PARTITION_KVS_BACKUP),
                         __ATOMIC_SEQ_CST);
    }

    // Check for invalid partitions
    if (!part ||
        part == __atomic_load_n(&kvsShared, __ATOMIC_SEQ_CST) ||
        part == __atomic_load_n(&kvsBackup, __ATOMIC_SEQ_CST)) {
        return false;
    }

    //Check that partition is within KVS boundaries
    if (afw_kvs_is_kvs_partition(part->offset)) {
        return true;
     }

    // group was a valid partition in the flashmap but not available for KVS usage
    return false;
}

static const char* getKeyName(const char *name, char **group)
{
    char *dotLoc = strchr(name, DOT);
    const char *key = dotLoc ? dotLoc + 1 : name;

    if (key == name){
        *group = NULL;
    } else {
        strncpy(*group, name, dotLoc - name);

        if (!is_group_valid((const char**)group)) {
            return NULL;
        }
    }

    return key;
}

static ace_status_t ace_kvs_set_wrapper(const char *name, const void *data, const size_t len, bool encrypted) {
    int32_t afw_ret;
    ace_status_t ace_ret;
    char _namespace[AFW_KVS_MAX_NAMESPACE_LEN + 1] = {0};
    char *namespace = _namespace;   // This way namespace can be set to NULL
    const char *key = getKeyName(name, &namespace);

    if (!key) {
        return ACE_STATUS_FAILURE_UNKNOWN_FILE;
    }

    afw_ret = afw_kvs_set(namespace, key, data, len, encrypted);
    ace_ret = convert_return_code(afw_ret);

    if (AFW_OK != afw_ret) {
        ASD_LOG_E(ace_hal, "Failed to set %s with error (%d:%s)", name, ace_ret, afw_strerror(afw_ret));
    }

    return ace_ret;
}

void aceKeyValueDsHal_db_init(const char *name) {
    (void)name;
}

ace_status_t aceKeyValueDsHal_set(const char *name, const void *data, const size_t len) {
    return ace_kvs_set_wrapper(name, data, len, false);
}

ace_status_t aceKeyValueDsHal_setWithEncryption(const char *name, const void *data, const size_t len) {
    return ace_kvs_set_wrapper(name, data, len, true);
}

ace_status_t aceKeyValueDsHal_remove(const char *name) {
    int32_t afw_ret;
    ace_status_t ace_ret;
    char _namespace[AFW_KVS_MAX_NAMESPACE_LEN + 1] = {0};
    char *namespace = _namespace;   // This way namespace can be set to NULL
    const char *key = getKeyName(name, &namespace);

    if (!key) {
        return ACE_STATUS_FAILURE_UNKNOWN_FILE;
    }

    afw_ret = afw_kvs_delete(namespace, key);
    ace_ret = afw_ret == -AFW_ENOENT ? ACE_STATUS_OK : convert_return_code(afw_ret);

    if (AFW_OK != afw_ret) {
        ASD_LOG_E(ace_hal, "Failed to remove %s with error code (%d:%s)", name, ace_ret, afw_strerror(afw_ret));
    }

    return ace_ret;
}

int aceKeyValueDsHal_get(const char *name, void *const data, const size_t len) {
    int32_t afw_ret;
    ace_status_t ace_ret;
    char _namespace[AFW_KVS_MAX_NAMESPACE_LEN + 1] = {0};
    char *namespace = _namespace;   // This way namespace can be set to NULL
    const char *key = getKeyName(name, &namespace);

    if (!key) {
        return ACE_STATUS_FAILURE_UNKNOWN_FILE;
    }

    memset (data, 0, len);
    afw_ret = afw_kvs_get(namespace, key, data, len);
    ace_ret = convert_return_code(afw_ret);

    if (0 > afw_ret) {
        if (-AFW_ENOENT != afw_ret) {
            ASD_LOG_E(ace_hal, "Failed to get value for %s with error code (%d:%s)", name, ace_ret, afw_strerror(afw_ret));
        } else {
            ASD_LOG_D(ace_hal, "The key %s does not exist", name);
        }
        return ace_ret;
    }

    return (int)afw_ret;
}

int aceKeyValueDsHal_getValueSize(const char *name) {
    int32_t afw_ret;
    ace_status_t ace_ret;
    char _namespace[AFW_KVS_MAX_NAMESPACE_LEN + 1] = {0};
    char *namespace = _namespace;   // This way namespace can be set to NULL
    const char *key = getKeyName(name, &namespace);

    if (!key) {
        return ACE_STATUS_FAILURE_UNKNOWN_FILE;
    }

    afw_ret = afw_kvs_get_size(namespace, key);
    ace_ret = convert_return_code(afw_ret);
    if (0 > afw_ret) {
        if (-AFW_ENOENT != afw_ret) {
            ASD_LOG_E(ace_hal, "Failed to get value size for %s with error code (%d:%s)", name, ace_ret, afw_strerror(afw_ret));
        } else {
            ASD_LOG_D(ace_hal, "The key %s does not exist", name);
        }
        return ace_ret;
    }

    return (int)afw_ret;
}

ace_status_t aceKeyValueDsHal_eraseGroup(const char *groupName) {
    int32_t afw_ret;
    ace_status_t ace_ret;

    if (!is_group_valid(&groupName)) {
        return ACE_STATUS_FAILURE_UNKNOWN_FILE;
    }

    afw_ret = afw_kvs_delete_namespace(groupName);
    ace_ret = convert_return_code(afw_ret);
    if (AFW_OK != afw_ret) {
        ASD_LOG_E(ace_hal, "Failed to erase group %s with error code (%d:%s)", groupName, ace_ret, afw_strerror(afw_ret));
    }

    return ace_ret;
}

ace_status_t aceKeyValueDsHal_listKeys(
    const char* group_name, aceKeyValueDsHal_listKeys_cb listKeys_cb) {

    if (!is_group_valid(&group_name)) {
        return ACE_STATUS_FAILURE_UNKNOWN_FILE;
    }

    int afw_ret = afw_kvs_get_all_keys_with_callback(group_name, NULL, 0, listKeys_cb);
    return convert_return_code(afw_ret);
}

