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
#include <stdio.h>

#include "acehal_device_info_cli.h"

#include "internal_device_info.h"
#include <ace/hal_device_info.h>

#include <stdio.h>
#include <string.h>

static ace_status_t acehal_device_info_get_cli(int32_t argc, const char** argv)
{
    char data[128] = {0};

    // gets the entry from device_info and prints it out
    #define PRINT_ENTRY(entry)                                  \
        aceDeviceInfoDsHal_getEntry(entry, data, sizeof(data)); \
        printf(#entry" = %s\n",data);                           \
        memset(data, 0, sizeof(data));

    PRINT_ENTRY(HARDWARE_NAME);
    PRINT_ENTRY(BT_DEVICE_NAME);
    PRINT_ENTRY(DEVICE_SERIAL);
    PRINT_ENTRY(DEVICE_TYPE_ID);
    PRINT_ENTRY(WIFI_MAC);
    PRINT_ENTRY(BT_MAC);
    PRINT_ENTRY(MAC_SECRET);
    PRINT_ENTRY(MANUFACTURING);
    PRINT_ENTRY(BOARDID);
    PRINT_ENTRY(PRODUCT_ID);
    PRINT_ENTRY(FFS_PIN);
    PRINT_ENTRY(LOCALE);
    PRINT_ENTRY(MICCAL1);
    PRINT_ENTRY(MICCAL2);
    PRINT_ENTRY(DSS_PUB_KEY);
    PRINT_ENTRY(LEDPARAMS);
    PRINT_ENTRY(CLIENT_ID);

    return ACE_STATUS_OK;
}

static void acehal_device_info_set(acehal_device_info_entry_t entry, const char* data)
{
    ace_status_t res = internal_deviceInfo_prototype_setEntry(entry, data);
    if (res != ACE_STATUS_OK) {
        printf("failed to set, ret: %d\n", res);
    }
    else {
        printf("set successfully, should reflect in 'get' cli\n");
    }

    return;
}

#define SET_CLI(entry) \
    static ace_status_t acehal_device_info_set##entry##_cli(int32_t argc, const char** argv) {\
        if (argc != 1) { \
            return ACE_STATUS_CLI_FUNC_ERROR; \
        } \
        acehal_device_info_set(entry, argv[0]); \
        return ACE_STATUS_OK; \
    }


SET_CLI(BT_DEVICE_NAME);
SET_CLI(DEVICE_SERIAL);
SET_CLI(DEVICE_TYPE_ID);
SET_CLI(WIFI_MAC);
SET_CLI(BT_MAC);
SET_CLI(MAC_SECRET);
SET_CLI(MANUFACTURING);
SET_CLI(BOARDID);
SET_CLI(PRODUCT_ID);
SET_CLI(FFS_PIN);
SET_CLI(LOCALE);
SET_CLI(MICCAL1);
SET_CLI(MICCAL2);
SET_CLI(DSS_PUB_KEY);
SET_CLI(LEDPARAMS);
SET_CLI(CLIENT_ID);

const aceCli_moduleCmd_t acehal_device_info_set_cli[] = {
    {"bt_name", "set Bluetooth advertisement name; args: <bt_name>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setBT_DEVICE_NAME_cli},
    {"dsn", "Set dsn; args: <dsn>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setDEVICE_SERIAL_cli},
    {"type", "set device type; args: <device_type_id>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setDEVICE_TYPE_ID_cli},
    {"wifi_mac", "set wifi mac; args: <mac>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setWIFI_MAC_cli},
    {"bt_mac", "set bt mac; args: <mac>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setBT_MAC_cli},
    {"mac_secret", "set mac secret; args: <mac>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setMAC_SECRET_cli},
    {"manufacturing", "set manufacturing Number (PSN); args: <manufacturing PSN eg: P011AL0180950221>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setMANUFACTURING_cli},
    {"board_id", "set Board ID; args: <board ID>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setBOARDID_cli},
    {"ffs_pid", "set FFS Product ID; args: <ffs_pid>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setPRODUCT_ID_cli},
    {"ffs_pin", "set FFS pin; args: <FFS pin>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setFFS_PIN_cli},
    {"locale", "set language localisation; args: <locale>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setLOCALE_cli},
    {"miccal1", "set mic calibration 1; args: <mic_cal>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setMICCAL1_cli},
    {"miccal2", "set mic calibration 2; args: <mic_cal>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setMICCAL2_cli},
    {"dss_pub_key", "set FFS pin; args: <dss pub key>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setDSS_PUB_KEY_cli},
    {"ledparams", "set led params; args: <led params>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setLEDPARAMS_cli},
    {"client_id", "set Client ID; args: <client_id>", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_setCLIENT_ID_cli},
    ACE_CLI_NULL_MODULE
};

const aceCli_moduleCmd_t acehal_device_info_cli[] = {
    {"get", "get information about device", ACE_CLI_SET_LEAF, .command.func=&acehal_device_info_get_cli},
    {"set", "sets values", ACE_CLI_SET_FUNC, .command.subCommands=acehal_device_info_set_cli},
    ACE_CLI_NULL_MODULE
};
