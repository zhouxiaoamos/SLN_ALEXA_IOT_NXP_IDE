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

#include "internal_device_info.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ace/hal_kv_storage.h>

#ifndef ACE_HAL_DEVICE_INFO_OS_VERSION
#define ACE_HAL_DEVICE_INFO_OS_VERSION "0.0.0"
#endif
#ifndef ACE_HAL_DEVICE_INFO_VERSION_NUMBER
#define ACE_HAL_DEVICE_INFO_VERSION_NUMBER "0000123456789"
#endif
#ifndef ACE_HAL_DEVICE_INFO_BUILD_FINGERPRINT
#define ACE_HAL_DEVICE_INFO_BUILD_FINGERPRINT "Amazon/Product:0.0/XXXXXX/000/P:userdebug/dev-keys"
#endif
#ifndef ACE_HAL_DEVICE_INFO_PRODUCT_NAME
#define ACE_HAL_DEVICE_INFO_PRODUCT_NAME "Product"
#endif
#ifndef ACE_HAL_DEVICE_INFO_BUILD_VARIANT
#define ACE_HAL_DEVICE_INFO_BUILD_VARIANT "userdebug"
#endif
#ifndef ACE_HAL_DEVICE_INFO_BUILD_TAGS
#define ACE_HAL_DEVICE_INFO_BUILD_TAGS "dev-keys"
#endif
#ifndef ACE_HAL_DEVICE_INFO_PACKAGE_NAME
#define ACE_HAL_DEVICE_INFO_PACKAGE_NAME "com.amazon.Product.FreeRTOS.os"
#endif
#ifndef ACE_HAL_DEVICE_INFO_HARDWARE_NAME
#define ACE_HAL_DEVICE_INFO_HARDWARE_NAME "XXXX"
#endif
#ifndef ACE_HAL_DEVICE_INFO_BOOTLOADER
#define ACE_HAL_DEVICE_INFO_BOOTLOADER "unknown"
#endif
#ifndef ACE_HAL_DEVICE_INFO_BUILD_DESC
#define ACE_HAL_DEVICE_INFO_BUILD_DESC "userdebug 0.0 NMAIN1 10000 dev-keys"
#endif
#ifndef ACE_HAL_DEVICE_INFO_CAP_SET_INDEX_KEY
#define ACE_HAL_DEVICE_INFO_CAP_SET_INDEX_KEY "unknown"
#endif
#ifndef ACE_HAL_DEVICE_INFO_FIRMWARE_VERSION
#define ACE_HAL_DEVICE_INFO_FIRMWARE_VERSION ACE_HAL_DEVICE_INFO_VERSION_NUMBER
#endif
#ifndef ACE_HAL_DEVICE_INFO_PLATFORM_NAME
#define ACE_HAL_DEVICE_INFO_PLATFORM_NAME ACE_HAL_DEVICE_INFO_HARDWARE_NAME
#endif
#ifndef ACE_HAL_DEVICE_INFO_MANUFACTURER_NAME
#define ACE_HAL_DEVICE_INFO_MANUFACTURER_NAME "Amazon"
#endif

#define DEVICE_INFO_MAX_LEN 128 /*need to increase this to 384 if support DSS public key*/
#define ENTRY_NOT_SUPPORTED "NA"

typedef enum {
    PROPERTY,
    KVS,
    OTHER,
} mapping_type_t;

typedef struct {
    mapping_type_t type;
    char* name;
} mapping_table_t;

// corresponds to acehal_device_info_entry_t
static const mapping_table_t entry_mapping_table[] = {
    {PROPERTY, ACE_HAL_DEVICE_INFO_OS_VERSION},           //entry 0
    {PROPERTY, ACE_HAL_DEVICE_INFO_VERSION_NUMBER},       //entry 1
    {PROPERTY, ACE_HAL_DEVICE_INFO_BUILD_FINGERPRINT},    //entry 2
    {PROPERTY, ACE_HAL_DEVICE_INFO_PRODUCT_NAME},         //entry 3
    {PROPERTY, ACE_HAL_DEVICE_INFO_BUILD_VARIANT},        //entry 4
    {PROPERTY, ACE_HAL_DEVICE_INFO_BUILD_TAGS},           //entry 5
    {PROPERTY, ACE_HAL_DEVICE_INFO_PACKAGE_NAME},         //entry 6
    {PROPERTY, ACE_HAL_DEVICE_INFO_HARDWARE_NAME},        //entry 7
    {PROPERTY, ACE_HAL_DEVICE_INFO_BOOTLOADER},           //entry 8
    {PROPERTY, ACE_HAL_DEVICE_INFO_BUILD_DESC},           //entry 9
    {KVS, "deviceData.idmeSerial"},                       //entry 10
    {KVS, "deviceData.idmeDeviceType"},                   //entry 11
    {KVS, "deviceData.idmeWifimac"},                      //entry 12
    {KVS, "deviceData.idmebtmac"},                        //entry 13
    {KVS, "deviceData.idmeMacSecret"},                    //entry 14
    {KVS, "deviceData.manufacturing"},                    //entry 15
    {KVS, "deviceData.pid"},                              //entry 16
    {KVS, "deviceData.ffsPin"},                           //entry 17
    {KVS, "deviceData.dev_hw_rev"},                       //entry 18
    {KVS, "deviceData.dss_pub_key"},                      //entry 19
    {PROPERTY, ACE_HAL_DEVICE_INFO_CAP_SET_INDEX_KEY},    //entry 20
    {OTHER, ""},                                          //entry 21
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 22
    {KVS, "deviceData.boardID"},                          //entry 23
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 24
    {PROPERTY, ACE_HAL_DEVICE_INFO_FIRMWARE_VERSION},     //entry 25
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 26
    {PROPERTY, ACE_HAL_DEVICE_INFO_PRODUCT_NAME},         //entry 27
    {PROPERTY, ACE_HAL_DEVICE_INFO_MANUFACTURER_NAME},    //entry 28
    {PROPERTY, ACE_HAL_DEVICE_INFO_PLATFORM_NAME},        //entry 29
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 30
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 31
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 32
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 33
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 34
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 35
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 36
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 37
    {KVS, "deviceData.micCal1"},                          //entry 38
    {KVS, "deviceData.micCal2"},                          //entry 39
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 40
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 41
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 42
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 43
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 44
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 45
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 46
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 47
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 48
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 49
    {KVS, "deviceData.locale"},                           //entry 50
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 51
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 52
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 53
    {KVS, "deviceData.ledParam"},                         //entry 54
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 55
    {KVS, ENTRY_NOT_SUPPORTED},                           //entry 56
    {KVS, "deviceData.bt_device_name"},                   //entry 57
    {KVS, "deviceData.client_id"},                        //entry 58
};

_Static_assert(sizeof(entry_mapping_table)/sizeof(mapping_table_t) == INVALID_ENTRY,
    "Size of entry mapping table doesn't match the allowed entries defined by ACE in hal_device_info.h");

static int aceDeviceInfoDsHal_getProp(acehal_device_info_entry_t entry, char* data, uint32_t bufSize) {
    uint32_t entrySize;
    const char *value = entry_mapping_table[entry].name;

    entrySize = strlen(value);

    if (entrySize >= bufSize) {
        return (int)ACE_STATUS_BUFFER_OVERFLOW;
    } else {
        strcpy(data, value);
        return (int)entrySize;
    }
}

static int aceDeviceInfoDsHal_getKvs(acehal_device_info_entry_t entry, char* data, uint32_t bufSize) {
    int32_t retval = aceKeyValueDsHal_get(entry_mapping_table[entry].name, data, bufSize);

    if (retval >= 0 && (uint32_t)retval > bufSize)
        return (int) ACE_STATUS_BUFFER_OVERFLOW;

    return (int)retval;
}

static int aceDeviceInfoDsHal_getOther(acehal_device_info_entry_t entry, char* data, uint32_t bufSize) {
    char tempData[32] = {0};
    uint32_t tempSize = 0;

    switch (entry) {
        case SIGN_TYPE:
            tempSize = aceDeviceInfoDsHal_getEntry(BUILD_TAGS, tempData, sizeof(tempData));
            if (tempSize > 0) {
                if (strcmp(tempData, "release-keys") == 0) {
                    if (bufSize < 8) {
                        return (int)ACE_STATUS_BUFFER_OVERFLOW;
                    }
                    strcpy(data, "release");
                } else {
                    if (bufSize < 6) {
                        return (int)ACE_STATUS_BUFFER_OVERFLOW;
                    }
                    strcpy(data, "debug");
                }
                return (int)strlen(data);
            } else {
                return (int)ACE_STATUS_DEVICE_INFO_INTERNAL_ERROR;
            }
            break;

        default:
            return (int)ACE_STATUS_DEVICE_INFO_ENTRY_NOT_SUPPORTED;
    }
    return (int)ACE_STATUS_DEVICE_INFO_INTERNAL_ERROR;
}

/**
 * Get the contents for a specified device info entry. The caller must allocate
 */
int aceDeviceInfoDsHal_getEntry(const acehal_device_info_entry_t entry, char* data, const uint32_t bufSize) {
    int entryNum = entry;

    // NULL terminate result string for error cases
    if (data) {
        data[0] = 0;
    }

    if (entryNum < 0 || entry >= INVALID_ENTRY || data == NULL || bufSize == 0)
        return (int)ACE_STATUS_BAD_PARAM;

    if (strcmp(entry_mapping_table[entry].name, ENTRY_NOT_SUPPORTED) == 0)
        return (int)ACE_STATUS_DEVICE_INFO_ENTRY_NOT_SUPPORTED;

    if (entry_mapping_table[entry].type == OTHER) {
        //other data section
        return aceDeviceInfoDsHal_getOther(entry, data, bufSize);
    } else if (entry_mapping_table[entry].type == KVS) {
        //kvs data section
        data[bufSize - 1] = 0;
        return aceDeviceInfoDsHal_getKvs(entry, data, bufSize - 1);
    } else if (entry_mapping_table[entry].type == PROPERTY) {
        //property/build time data section
        return aceDeviceInfoDsHal_getProp(entry, data, bufSize);
    } else {
        return (int) ACE_STATUS_DEVICE_INFO_INTERNAL_ERROR;
    }
}

ace_status_t internal_deviceInfo_prototype_setEntry(acehal_device_info_entry_t entry, const char* data)
{
    int entryNum = entry;
    if (entryNum < 0 || entry >= INVALID_ENTRY || data == NULL) {
        return ACE_STATUS_BAD_PARAM;
    }

    ace_status_t res = ACE_STATUS_NOT_SUPPORTED;
    if (entry_mapping_table[entry].type == KVS) {
        // note on non-prototype devices the flash map will have this partition as read only so this will fail as it should.
        res = aceKeyValueDsHal_set(entry_mapping_table[entry].name, data, strlen(data));
    }
    return res;
}

/**
 * Get the value size for a specified idme entry.
 */
int aceDeviceInfoDsHal_getEntrySize(const acehal_device_info_entry_t entry) {
    char data[DEVICE_INFO_MAX_LEN] = {0};
    return aceDeviceInfoDsHal_getEntry(entry, data, sizeof(data));
}
