/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All rights reserved.
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
 * Smarthome Main Task
 *******************************************************************************
 */
#include <stdint.h>
/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "app_cli.h"

#include <asd_cli.h>
#include <asd_log_platform_api.h>
#include <asd_logger_if.h>
#include <asd_logger_config.h>

#include <ace/ace.h>

#ifdef ACE_COMPONENT_hal_device_info
#include <ace/hal_device_info.h>
#endif

#ifdef ACE_COMPONENT_hal_kv_storage
#include <ace/hal_kv_storage.h>
#endif

#ifdef ACE_COMPONENT_hal_dha
#include <ace/hal_dha.h>
#endif

#include <ace/ace_main.h>
#include <ace/ace_config.h>
#include <ace/ace_log.h>

#ifdef ACE_COMPONENT_libace_connectivity_manager
#include <ace/ace_conn_mgr.h>
#endif

#ifdef ACE_WIFI_MIDDLEWARE
#include <ace/wifi_mgr.h>
#include <ace/ace_wifi_svc.h>
extern void aceWifiMonitor_start(void);
#endif

#ifdef ACE_MAP
#include <ace/map_api.h>
#endif
#include <ace/version.h>
#include <assert.h>
#include "flash_manager.h"
#include "platform_key.h"
#include "asd_debug.h"
#include "asd_tools_handler.h"

#include "afw_kvs.h"
#include "afw_kvs_str.h"
#include "flash_map.h"

#include "afw_flash_api.h"

#include <mbedtls/threading.h>
#include "ace_threading_alt.h"

#ifdef ACE_COMPONENT_hal_factoryreset
#include "acehal_fr_mgr.h"
#endif

#include "rt106a_device_info_setup.h"

#ifdef AMAZON_BOOTLOADER_SECUREBOOT_ENABLE
#include "secure_boot_header.h"

const secure_boot_header_t gAmazon_SecureBootHeader = {{0}};
#endif

#ifndef TEST_ABORT
void TEST_ABORT() { return; }
#endif

#define MAIN_TASK_STACKSIZE (8 * 1024)
#define MAIN_TASK_PRIORITY ((configMAX_PRIORITIES - 10) | portPRIVILEGE_BIT)
#define MAIN_TASK_DELAY_TIME (10 / portTICK_PERIOD_MS)

#define ASD_CLI_TASK_STACKSIZE 4096 * sizeof(portSTACK_TYPE)

asd_log_create_module(main, ASD_LOG_LEVEL_DEFAULT,
                      ASD_LOG_PLATFORM_STREAM_BM_DEFAULT);

static struct fm_flash_partition partitions[] = {
#ifdef FLASH_PARTITION_LOADER_BIN
    {
        .name = FLASH_PARTITION_LOADER_BIN,
        .size = BOOTLOADER_IMAGE_LENGTH,
        .offset = BOOTLOADER_BASE,
        .flags = FM_FLASH_UNSECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_BOOTCTL_1
    {
        .name = FLASH_PARTITION_BOOTCTL_1,
        .size = BOOTCTL_LENGTH,
        .offset = BOOTCTL_SECTION_1_BASE,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_BOOTCTL_2
    {
        .name = FLASH_PARTITION_BOOTCTL_2,
        .size = BOOTCTL_LENGTH,
        .offset = BOOTCTL_SECTION_2_BASE,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_APP_IMAGE_A
    {
        .name = FLASH_PARTITION_APP_IMAGE_A,
        .size = APP_IMAGE_A_LENGTH,
        .offset = APP_IMAGE_A_BASE,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_APP_IMAGE_B
    {
        .name = FLASH_PARTITION_APP_IMAGE_B,
        .size = APP_IMAGE_B_LENGTH,
        .offset = APP_IMAGE_B_BASE,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_PERSIST_CRED_DD
    {
        .name = FLASH_PARTITION_PERSIST_CRED_DD,
        .size = FLASH_PERSIST_CRED_DD_SIZE,
        .offset = FLASH_PERSIST_CRED_DD_ADDR,
        .encrypted = 1,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_PERSIST_CRED_DHAKEY
    {
        .name = FLASH_PARTITION_PERSIST_CRED_DHAKEY,
        .size = FLASH_PERSIST_CRED_DHAKEY_SIZE,
        .offset = FLASH_PERSIST_CRED_DHAKEY_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE,
    },
    {
        .name = FLASH_PARTITION_PERSIST_CRED_DHACRT,
        .size = FLASH_PERSIST_CRED_DHACRT_SIZE,
        .offset = FLASH_PERSIST_CRED_DHACRT_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE,
    },
    {
        .name = FLASH_PARTITION_PERSIST_CRED_DHACRT2,
        .size = FLASH_PERSIST_CRED_DHACRT2_SIZE,
        .offset = FLASH_PERSIST_CRED_DHACRT2_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_DEVICE_CALIB
    {
        .name = FLASH_PARTITION_DEVICE_CALIB,
        .size = FLASH_PERSIST_DEVICE_CALIB_SIZE,
        .offset = FLASH_PERSIST_DEVICE_CALIB_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_ROLL_CRED_IOTKEY
    {
        .name = FLASH_PARTITION_ROLL_CRED_IOTKEY,
        .size = FLASH_ROLL_CRED_IOTKEY_SIZE,
        .offset = FLASH_ROLL_CRED_IOTKEY_ADDR,
        .encrypted = 1,
        .flags = AFW_FLASH_UNSECURED_WRITEABLE | AFW_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_ROLL_CRED_IOTCRT
    {
        .name = FLASH_PARTITION_ROLL_CRED_IOTCRT,
        .size = FLASH_ROLL_CRED_IOTCRT_SIZE,
        .offset = FLASH_ROLL_CRED_IOTCRT_ADDR,
        .flags = AFW_FLASH_UNSECURED_WRITEABLE | AFW_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_ROLL_CRED_CUSTID
    {
        .name = FLASH_PARTITION_ROLL_CRED_CUSTID,
        .size = FLASH_ROLL_CRED_CUSTID_SIZE,
        .offset = FLASH_ROLL_CRED_CUSTID_ADDR,
        .encrypted = 1,
        .flags = AFW_FLASH_UNSECURED_WRITEABLE | AFW_FLASH_SECURED_WRITEABLE,
    },
#endif

#ifdef FLASH_PARTITION_ROLL_CRED_CUST_KEY
    {
        .name = FLASH_PARTITION_ROLL_CRED_CUST_KEY,
        .size = FLASH_ROLL_CRED_CUST_KEY_SIZE,
        .offset = FLASH_ROLL_CRED_CUST_KEY_ADDR,
        .encrypted = 1,
        .flags = AFW_FLASH_UNSECURED_WRITEABLE | AFW_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_ROLL_CRED_USER_ACCT_KEY
    {
        .name = FLASH_PARTITION_ROLL_CRED_USER_ACCT_KEY,
        .size = FLASH_ROLL_CRED_USER_ACCT_SIZE,
        .offset = FLASH_ROLL_CRED_USER_ACCT_ADDR,
        .encrypted = 1,
        .flags = AFW_FLASH_UNSECURED_WRITEABLE | AFW_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_ROLL_CRED_DEVICE_INFO
    {
        .name = FLASH_PARTITION_ROLL_CRED_DEVICE_INFO,
        .size = FLASH_ROLL_CRED_DEVICE_INFO_SIZE,
        .offset = FLASH_ROLL_CRED_DEVICE_INFO_ADDR,
        .encrypted = 1,
        .flags = AFW_FLASH_UNSECURED_WRITEABLE | AFW_FLASH_SECURED_WRITEABLE,
    },
#endif

#ifdef FLASH_PARTITION_ROLL_CRED_WIFI
    {
        .name = FLASH_PARTITION_ROLL_CRED_WIFI,
        .size = FLASH_ROLL_CRED_WIFI_SIZE,
        .offset = FLASH_ROLL_CRED_WIFI_ADDR,
        .encrypted = 1,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_MAPLITE
    {
        .name = FLASH_PARTITION_MAPLITE,
        .size = FLASH_MAPLITE_SIZE,
        .offset = FLASH_MAPLITE_ADDR,
        .encrypted = 1,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_THERMAL
    {
        .name = FLASH_PARTITION_THERMAL,
        .size = FLASH_THERMAL_DATA_SIZE,
        .offset = FLASH_THERMAL_DATA_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_KVS_BACKUP
    {
        .name = FLASH_PARTITION_KVS_BACKUP,
        .size = FLASH_KVS_BACKUP_SIZE,
        .offset = FLASH_KVS_BACKUP_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_KVS_SHARED
    {
        .name = FLASH_PARTITION_KVS_SHARED,
        .size = FLASH_KVS_SHARED_SIZE,
        .offset = FLASH_KVS_SHARED_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_LOG
    {
        .name = FLASH_PARTITION_LOG,
        .size = LOG_LENGTH,
        .offset = LOG_BASE,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
    {
        .name = FLASH_PARTITION_LOG_META,
        .size = LOG_META_SIZE,
        .offset = LOG_META_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
    {
        .name = FLASH_PARTITION_LOG_MAIN,
        .size = LOG_MAIN_SIZE,
        .offset = LOG_MAIN_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
    {
        .name = FLASH_PARTITION_LOG_CRASH,
        .size = LOG_CRASH_LOG_SIZE,
        .offset = LOG_CRASH_LOG_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#ifdef FLASH_PARTITION_LOG_3P
    {
        .name = FLASH_PARTITION_LOG_3P,
        .size = LOG_3P_SIZE,
        .offset = LOG_3P_ADDR,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif

#ifdef FLASH_PARTITION_METRICS
    {
        .name = FLASH_PARTITION_METRICS,
        .size = METRICS_LENGTH,
        .offset = METRICS_BASE,
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,
    },
#endif
#if defined(AMAZON_FLASH_MAP_EXT_ENABLE) && defined(FLASH_PARTITION_EXTENTION)
    /* Add extended flash partitions by the App */
    FLASH_PARTITION_EXTENTION
#endif
};

extern void ace_app_main(void* parameters);

bool is_device_locked(void) { return false; }

#define DEBUG_BUF_LEN 128

// TODO check if really needed - global flash offset memory mapped variable
uintptr_t _g_global_flash_offset;

#ifndef MAPLITE_KV_AWS_IOT_THING_NAME
#define MAPLITE_KV_AWS_IOT_THING_NAME ACE_MAPLITE_DS_GROUP ".aws_iot_thing_name"
#endif
static void print_debug_info(void) {
    int retval = 0;
    char buf[DEBUG_BUF_LEN] = {0};
    ACE_LOGI(ACE_LOG_ID_MAIN, "main", "====================================");
    ACE_LOGI(ACE_LOG_ID_MAIN, "main", "ACS version: [%s]", ACS_VERSION_STRING);

#ifdef ACE_COMPONENT_hal_device_info
    retval =
        aceDeviceInfoDsHal_getEntry(DEVICE_SERIAL, (char*)buf, DEBUG_BUF_LEN);
    ACE_LOGI(ACE_LOG_ID_MAIN, "main", "Device serial Number: [%s]",
             (retval < 0) ? "ERROR" : buf);
    retval =
        aceDeviceInfoDsHal_getEntry(DEVICE_TYPE_ID, (char*)buf, DEBUG_BUF_LEN);
    ACE_LOGI(ACE_LOG_ID_MAIN, "main", "Device Type: [%s]",
             (retval < 0) ? "ERROR" : buf);
#endif

#ifdef ACE_COMPONENT_hal_dha
    uint8_t* cert_chain = NULL;
    size_t cert_chain_len = 0;
    retval = aceDhaHal_get_field(ACE_DHA_CERTIFICATE_CHAIN, (void**)&cert_chain,
                                 &cert_chain_len);
    if (retval < 0) {
        ACE_LOGI(ACE_LOG_ID_MAIN, "main", "Device certs not found");
        goto print_return;
    }
    free(cert_chain);
    ACE_LOGI(ACE_LOG_ID_MAIN, "main", "DHA cert is present, size: %d bytes",
             cert_chain_len);
#endif

#ifdef ACE_MAP
    aceMap_registrationState_t state;
    ace_status_t status = aceMap_getRegistrationState(&state);
    if (status == ACE_STATUS_OK) {
        ACE_LOGI(ACE_LOG_ID_MAIN, "main", "Device is%s registered, state: %d",
                 (state == ACE_MAP_DEVICE_REGISTERED) ? "" : " not", state);
    } else {
        ACE_LOGE(ACE_LOG_ID_MAIN, "main",
                 "Unable to retrieve device reg. state,  Error %d", status);
    }
#endif

print_return:
    ACE_LOGI(ACE_LOG_ID_MAIN, "main", "====================================");
}

static void _main_task(void* parameters) {
    unsigned char* kvs_aes_key;

    // give usb cdc a chance
    vTaskDelay(1000);

    mbedtls_threading_set_alt(
        &threading_mutex_init_freertos, &threading_mutex_free_freertos,
        &threading_mutex_lock_freertos, &threading_mutex_unlock_freertos);

    // init asd logger.
    asd_logger_init(asd_logger_get_handle(), asd_logger_get_default_config());

    // afw_kvs initializations.
    // Must before wifi, ace_main(), which use kvs
    configASSERT(platform_key_setup_kvs_key(&kvs_aes_key) == 0);
    configASSERT(afw_kvs_init(kvs_aes_key) == 0);

#ifdef ACE_COMPONENT_hal_factoryreset
    // init factory reset setting. resume factory reset if it is in progress.
    // must be called after kvs init, and before any service using kvs.
    aceFrHalMgr_init();
#endif
    // setup ace
    ace_main();

    if (asd_rtos_cli_init(ACE_CLI_MODULE_CMDS, ASD_CLI_TASK_STACKSIZE) != 0) {
        ACE_LOGI(ACE_LOG_ID_SYSTEM, "main", "Could not initialize CLI\r\n");
    }

    ACE_LOGI(ACE_LOG_ID_SYSTEM, "main", "%s\r\n", "Hello World...");
    ASD_LOG_I(main, "This is info printed from %s", "_main_task");

    aceCli_setIsLocked(is_device_locked);

#ifdef ACE_WIFI_MIDDLEWARE
    ace_status_t err = ACE_STATUS_GENERAL_ERROR;
    err = aceWifiHal_init(NULL);
    if (err != ACE_STATUS_OK) {
        ACE_LOGE(ACE_LOG_ID_SYSTEM, "main",
                 "Error in initializing WiFi HAL, reason: %d", err);
    }
#endif

#ifdef ACE_COMPONENT_libace_connectivity_manager
    // init ACM
    acmReturnCode_t ret_code = acm_init();
    if (ret_code != eAcmOk) {
        ACE_LOGE(ACE_LOG_ID_MAIN, "main", "Failed to init ACM %d", ret_code);
    }
#endif

#ifdef ACE_COMPONENT_libace_sys_toolbox
    init_heapstats_callbacks();
#endif
    print_debug_info();
    
    // NXP TODO maybe move somewhere else
    device_info_set_kvs_entries();

    ace_app_main(parameters);
    // Should not return from here
    ASD_LOG_I(main, "Info: Returned from app_main!");
    vTaskDelete(NULL);
}

//==============================================================================

int create_main_task(void) {
    // clang-format off
    return (xTaskCreate(_main_task,
                        "main",
                        MAIN_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                        (void*)NULL,
                        MAIN_TASK_PRIORITY,
                        NULL) == pdPASS ?
            0 : -1);
    // clang-format on
}

//==============================================================================

/* Things to be initialized at the beginning of main() */
void system_early_init(void) {
    // Initialize Flash Manger as early as possible.
    configASSERT(fm_flash_init(partitions, ARRAY_SIZE(partitions)) == 0);
    // asd tools/debug init must after fm_flash_init(), because
    // asd_crashdump_init() uses flash partition.
    asd_tools_init();

    time_utils_init();
}

//==============================================================================
