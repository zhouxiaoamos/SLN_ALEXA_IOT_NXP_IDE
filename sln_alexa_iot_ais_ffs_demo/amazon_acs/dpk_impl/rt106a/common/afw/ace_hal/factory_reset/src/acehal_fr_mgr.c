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
/*
 * ACE HAL Factory Reset Driver
 */

#include <stdio.h>
#include <string.h>
#include <ace/hal_fr.h>
#include <afw_error.h>
#include <ace/hal_kv_storage.h>
#include "asd_log_platform_api.h"
#include "acehal_fr_mgr.h"
#include "flash_map.h"
#include "flash_manager.h"



typedef struct {
    aceHalFr_user_callback_t cb;
    void* arg;
} callback_ctx_t;

typedef struct {
    callback_ctx_t event_callbacks[ACE_FR_EVENT_NUM];

} aceFrHal_config_t;

static aceFrHal_config_t s_fr_conf;

// This is the value written to KVS, once factory reset completes.
static const char s_factory_reset_completion_value[] =
    "Factory reset is completed successfully.";


// This should be defined in flash_map.h, so that project can override
#ifndef FACTORY_RESET_PARTITIONS_TO_ERASE
// use the default partition list for erase.
const char * factory_reset_erase_partitions[] = {
#ifdef FLASH_PARTITION_ROLL_CRED_IOTCRT
       FLASH_PARTITION_ROLL_CRED_IOTCRT,
#endif
#ifdef FLASH_PARTITION_ROLL_CRED_WIFI
       FLASH_PARTITION_ROLL_CRED_WIFI,
#endif
#ifdef FLASH_PARTITION_ROLL_CRED_USER_ACCT_KEY
       FLASH_PARTITION_ROLL_CRED_USER_ACCT_KEY,
#endif
#ifdef FLASH_PARTITION_ROLL_CRED_DEVICE_INFO
       FLASH_PARTITION_ROLL_CRED_DEVICE_INFO,
#endif
#ifdef FLASH_PARTITION_ROLL_CRED_CUST_KEY
       FLASH_PARTITION_ROLL_CRED_CUST_KEY,
#endif
#ifdef FLASH_PARTITION_MAPLITE
       FLASH_PARTITION_MAPLITE,
#endif
#ifdef FLASH_PARTITION_METRICS
       FLASH_PARTITION_METRICS,
#endif
#ifdef FLASH_PARTITION_ROLL_CRED_IOTKEY
       FLASH_PARTITION_ROLL_CRED_IOTKEY,
#endif
};
#else
const char * factory_reset_erase_partitions[] = {
       FACTORY_RESET_PARTITIONS_TO_ERASE
};
#endif

static int flash_erase_partition(const char* part_name)
{
    int ret;
    struct fm_flash_partition *flash;

    flash = fm_flash_get_partition(part_name);
    if (!flash) {
        ASD_LOG_E(ace_hal,
            "Failed to get partition: %s", part_name);
        return -AFW_ENOENT;
    }

    ret = fm_flash_erase(flash, FM_BYPASS_CLIENT_ID);
    if (ret != 0) {
        ASD_LOG_E(ace_hal,
            "Erase failure (%d): %s %s", ret, afw_strerror(ret), part_name);
    } else {
        ASD_LOG_I(ace_hal,
            "Erased partition %s.", part_name);
    }

    return ret;
}

/****************************  APIs *********************************/
void aceFrHalMgr_init(void)
{
    if (aceFrHalMgr_is_resetting()) {
        // if factory reset is in progress, erase all user partitions.
        ASD_LOG_I(ace_hal, "Resume Factory reset.");

        // The partition erase is executed early stage of boot up.
        aceFrHalMgr_erase_user_partitions();
        int rc = aceFrHalMgr_set_breadcrumb(false);
        if ( rc == ACE_STATUS_OK) {
            ASD_LOG_I(ace_hal, "Factory reset is completed successfully.");
        } else {
            ASD_LOG_E(ace_hal, "Factory reset completion fails, rc = %d.", rc);
        }
    }
}

int aceFrHalMgr_erase_user_partitions(void)
{
    int num = sizeof(factory_reset_erase_partitions) / sizeof(factory_reset_erase_partitions[0]);
    /* format flash partition */
    for (int i = 0; i < num; i++) {
        flash_erase_partition(factory_reset_erase_partitions[i]);
    }
    return 0;
}

bool aceFrHalMgr_is_resetting(void)
{

    int len = aceKeyValueDsHal_getValueSize(ACE_KVS_FACTORY_RESET_COMPLETION_NAME);

    if (len != sizeof(s_factory_reset_completion_value)) {
        // bad FR completion value length.
        return true;
    }
    char factory_reset_completion[sizeof(s_factory_reset_completion_value)] = {0};
    len = aceKeyValueDsHal_get(ACE_KVS_FACTORY_RESET_COMPLETION_NAME,
            factory_reset_completion, sizeof(factory_reset_completion));
    if (len != sizeof(s_factory_reset_completion_value)) {
        return true;
    }

    // if completion flag is written, return false.
    return memcmp(s_factory_reset_completion_value, factory_reset_completion,
        sizeof(factory_reset_completion)) != 0;
}


int aceFrHalMgr_set_breadcrumb(bool enable)
{
    if (enable){
        // start of factory reset, remove ACE_KVS_FACTORY_RESET_COMPLETION_NAME
        // to inidcate a FR is in progress.
        ace_status_t ret = aceKeyValueDsHal_remove(ACE_KVS_FACTORY_RESET_COMPLETION_NAME);
        if (ret < 0) {
            // if this fails, the partition is corrupt or something else bad, so go nuclear.
            // (it does not fail if the key just simply doesn't exist)
            // (We dont always do this because it can be slow)

            ASD_LOG_I(ace_hal,
                     "factory reset failed to remove completion breadcrumb %d so erasing partition",
                     ret);
            // All keys of default (NULL) group are now savedin shard KVS
            // partition, and it is common across projects.
            // if this is asynchronous wait for callback....
            ret = flash_erase_partition(FLASH_PARTITION_KVS_SHARED);
        }
        return ret;

    } else {
        // end of factory reset. Write ACE_KVS_FACTORY_RESET_COMPLETION_NAME to
        // indicate a FR completes successfully.
        return aceKeyValueDsHal_set(ACE_KVS_FACTORY_RESET_COMPLETION_NAME,
            s_factory_reset_completion_value, sizeof(s_factory_reset_completion_value));
    }
}

//all callback should run in sync mode.
int aceFrHalMgr_execute_callback(ace_fr_event_t event)
{
    if (((uint32_t) event) >= ACE_FR_EVENT_NUM) return ACE_STATUS_BAD_PARAM;
    if (s_fr_conf.event_callbacks[event].cb) {
        return s_fr_conf.event_callbacks[event].cb(s_fr_conf.event_callbacks[event].arg);
    }
    return ACE_STATUS_OK;
}

int aceFrHalMgr_register_user_callback(ace_fr_event_t event, aceHalFr_user_callback_t cb, void* arg)
{
    if (((uint32_t) event) >= ACE_FR_EVENT_NUM) return ACE_STATUS_BAD_PARAM;
    if ( cb && s_fr_conf.event_callbacks[event].cb) {
        // Not allow to override a callback.
        return ACE_STATUS_BAD_PARAM;
    }
    // only set or clear is allowed.
    // cb == NULL or s_fr_conf.event_callbacks[event].cb == NULL
    s_fr_conf.event_callbacks[event].arg = arg;
    s_fr_conf.event_callbacks[event].cb= cb;
    return ACE_STATUS_OK;
}


