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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <ace/ace_config.h>
#include <ace/hal_wifi.h>
#include <ace/hal_kv_storage.h>

#include <FreeRTOS.h>
#include "asd_log_platform_api.h"
#include "iot_wifi.h"
#include "hal_wifi_private.h"

static char* WIFI_KEY_PROFILE_V1 = "WifiCred.profile1";

// These are defined in board specific header wifi_api.h
#define WIFI_MAX_LENGTH_OF_SSID 32
#define WIFI_LENGTH_PASSPHRASE  64
#define WIFI_LENGTH_PMK 32

// V1 format
typedef enum {
    WIFI_SEC_TYPE_WPA2_AES_NETWORK,
    WIFI_SEC_TYPE_WPA_WPA2_NETWORK,
    WIFI_SEC_TYPE_WPA_LEGACY_NETWORK,
    WIFI_SEC_TYPE_UNSUPPORTED_WPA_NETWORK,
    WIFI_SEC_TYPE_WEP_NETWORK,
    WIFI_SEC_TYPE_OPEN_NETWORK,
    WIFI_SEC_TYPE_NOT_SUPPORTED = UINT8_MAX,
} afw_wifi_net_type_t;

// Wifi profile info struct
typedef struct {
    afw_wifi_net_type_t     wifi_sec_type;
    uint8_t                 ssid[WIFI_MAX_LENGTH_OF_SSID];
    uint8_t                 ssid_len;
    char                    passphrase[WIFI_LENGTH_PASSPHRASE + 1];
    uint8_t                 pmk[WIFI_LENGTH_PMK];
    uint8_t                 valid;
} wifi_profile_info_t;

void aceWifiHal_profile_v1_to_v2(aceWifiHal_config_t *profile, int32_t *version) {
    int32_t retVal;
    wifi_profile_info_t profile_v1;  // Note v1 profile has only 1 entry

    retVal = aceKeyValueDsHal_get(WIFI_KEY_PROFILE_V1, &profile_v1, sizeof(profile_v1));
    if (retVal == sizeof(profile_v1)) {
        ASD_LOG_E(wifi_hal, "V1 profile found");

        profile->ssidLength = profile_v1.ssid_len;
        memcpy(profile->ssid, profile_v1.ssid, profile->ssidLength);
        profile->pskLength = strlen(profile_v1.passphrase);
        memcpy(profile->psk, profile_v1.passphrase, profile->pskLength);
        profile->status = aceWifiHal_CONFIG_STATUS_ENABLED;

        switch(profile_v1.wifi_sec_type) {
            case WIFI_SEC_TYPE_WPA2_AES_NETWORK:
            case WIFI_SEC_TYPE_WPA_WPA2_NETWORK:
                profile->authMode = aceWifiHal_AUTH_MODE_WPA2_PSK;
                break;
            case WIFI_SEC_TYPE_WPA_LEGACY_NETWORK:
                profile->authMode = aceWifiHal_AUTH_MODE_WPA_PSK;
                break;
            case WIFI_SEC_TYPE_WEP_NETWORK:
                profile->authMode = aceWifiHal_AUTH_MODE_WEP;
                break;
            case WIFI_SEC_TYPE_OPEN_NETWORK:
                profile->authMode = aceWifiHal_AUTH_MODE_OPEN;
                break;
            default:
                profile->authMode = aceWifiHal_AUTH_MODE_MAX;  //Invalid autho mode
                break;
        }
        *version = WIFI_PROFILE_VERSION_V1;
    }
}

void aceWifiHal_profile_remove_v1() {
    aceKeyValueDsHal_remove(WIFI_KEY_PROFILE_V1);
}
