/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All rights reserved.
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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <ace/ace_config.h>
#include <ace/hal_wifi.h>

#include "asd_log_platform_api.h"

#include <ace/hal_kv_storage.h>

#include <FreeRTOS.h>
#include "iot_wifi.h"
#include "aws_wifi_ext.h"
#include "hal_wifi_private.h"

#ifndef WIFI_MAX_NUMBER_OF_STA
#define WIFI_MAX_NUMBER_OF_STA  2
#endif

//
// aceWifiSvc event callback function
//
static aceWifiHal_eventHandler_t svcEventHandler;

// local function declare
static aceWifiHal_error_t aceWifiHal_mapChannelToFreq(uint8_t channel, uint16_t* frequency);
static aceWifiHal_authMode_t aceWifiHal_convertAuthMode(WIFISecurity_t xSecurity);

static const char* obfuscateSsid(const char* ssid) {
#ifdef DEBUG
    return ssid;
#else
    return "xxx";
#endif
}

void aceWifiHal_registerEventHandler(aceWifiHal_eventHandler_t handler) {
    svcEventHandler = handler;
}

static aceWifiHal_error_t aceWifiHal_dispatchEvent(aceWifiHal_eventType_t type, uint8_t* data, uint32_t dataLen) {
    ASD_LOG_I(wifi_hal, "aceWifiHal_dispatchEvent: type %d", type);

    if (svcEventHandler != NULL) {
        return svcEventHandler(type, data, dataLen);
    }
    return aceWifiHal_ERROR_FAILURE;
}

//
// Profile management functions
// TODO: move it to svc?
//

// To avoid exposing priority info to user app, create another structure here
typedef struct aceWifiHal_RTOS_config {
    aceWifiHal_config_t mgrConfig;
    int priority;                       /**< Priority of the Network */
} aceWifiHal_RTOS_config_t;

// Current version of aceWifiHal_config_t. We must check WIFI_KEY_VERSION
// stored in KVS against this value and perform conversion of profile_array
// if necessary
#define WIFI_PROFILE_VERSION    1

static aceWifiHal_RTOS_config_t profile_array[ACE_WIFI_MAX_CONFIGURED_NETWORKS];

/*Current network state, initialized to DISCONNECTED*/
static aceWifiHal_networkState_t networkState = aceWifiHal_NET_STATE_DISCONNECTED;

#ifdef ACE_WIFI_HAL_AUTO_CONNECT
/*Define MAX retry to connect in HL-HAL*/
#define HAL_WIFI_MAX_RETRY_AUTO_CONNECT 1
/*Auto connect retry count*/
static int autoConnectRetryCount = 0;
/*The flag of disconnect from remove network connected/connecting*/
static bool disconnectFromRemoveNetwork = false;
#endif

// Save current connected ip address and wifi info
static aceWifiHal_wifiInfo_t sWifiInfo;

// WiFi KVS keys must have the format "WifiCred.xxx"
static char* WIFI_KEY_PROFILE = "WifiCred.profile";
static char* WIFI_KEY_VERSION = "WifiCred.version";

/* Initialize the profile array, to determine of the profile[i] is empty or not,
   check if "ssidLength == 0"
 *
 */
static aceWifiHal_error_t aceWifiHal_initializeProfile(void) {
    memset(profile_array, 0, sizeof(profile_array));
    for (int i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        profile_array[i].priority = -1;
    }
    return aceWifiHal_ERROR_SUCCESS;
}

/* Find the highest priority, profile can be in any status, enabled or disabled.
 * Return -1 if no valid profile can be found
 *
 */
static int aceWifiHal_findHighestPriority(void) {
    int priority = -1;

    for (int i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        if (profile_array[i].mgrConfig.ssidLength != 0 && profile_array[i].priority > priority) {
            priority = profile_array[i].priority;
        }
    }
    return priority;
}
/* Find the highest priority profile among enabled ones
 * Return NULL if no valid profile can be found
 *
 */
static aceWifiHal_RTOS_config_t* aceWifiHal_findHighestPriorityEnabledProfile(void) {
    aceWifiHal_RTOS_config_t* profile = NULL;
    int priority = -1;

    for (int i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        if (profile_array[i].mgrConfig.ssidLength != 0 &&
            profile_array[i].mgrConfig.status != aceWifiHal_CONFIG_STATUS_DISABLED &&
            profile_array[i].priority > priority) {
            profile = &profile_array[i];
            priority = profile->priority;
        }
    }

    return profile;
}

/* Set the new profile, move content from config into profile array
 * Need to edit if we add more element into profile structure
 */
static aceWifiHal_error_t aceWifiHal_setConfig(aceWifiHal_RTOS_config_t* profile, const aceWifiHal_config_t* config, int priority) {

     // as all the fields are fixed length, will do the whole memory copy
     memcpy(&(profile->mgrConfig), config, sizeof(aceWifiHal_config_t));
     profile->priority = priority;
     // The new profile status might be set as "0", which is then aceWifiHal_CONFIG_STATUS_CURRENT
     // need to set it to be "enabled" instead
     profile->mgrConfig.status = aceWifiHal_CONFIG_STATUS_ENABLED;
     return aceWifiHal_ERROR_SUCCESS;
}

/* Reset all profile priorities from 0, 1, 2, ..., (ACE_WIFI_MAX_CONFIGURED_NETWORKS-1).
 */
static void aceWifiHal_resetAllProfilesPriority(aceWifiHal_RTOS_config_t profilesArray[])
{
    int profilePriority[ACE_WIFI_MAX_CONFIGURED_NETWORKS] = {0};
    int i, j;

    ASD_LOG_I(wifi_hal, "aceWifiHal_resetAllProfilesPriority...");

    /* Record numbers that the priority lower than mine.
     */
    for (i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        if (profilesArray[i].mgrConfig.ssidLength == 0) {
            continue;
        }
        for (j = 0; j < ACE_WIFI_MAX_CONFIGURED_NETWORKS; j++) {
            if (profilesArray[j].mgrConfig.ssidLength == 0) {
                continue;
            }
            if (profilesArray[j].priority < profilesArray[i].priority) {
                profilePriority[i]++;
            }
        }
    }

    /*Reset all priority from 0, 1, 2, ..., (N-1)*/
    for (i = 0; i  < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        if (profilesArray[i].mgrConfig.ssidLength == 0) {
            continue;
        }
        profilesArray[i].priority = profilePriority[i];
    }

    return;
}

#ifdef ACE_WIFI_HAL_AUTO_CONNECT
/* Set current profile priority to 0, Others profile priority + 1.
 */
static void aceWifiHal_reduceCurrentProfilePriority(void)
{
    int index = ACE_WIFI_MAX_CONFIGURED_NETWORKS;
    bool needAdjust = false;
    int i;

    ASD_LOG_I(wifi_hal, "aceWifiHal_reduceCurrentProfilePriority...");

    for (i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        if (0 == profile_array[i].mgrConfig.ssidLength) {
            continue;
        }
        /* Set current profile priority to 0, Others profile priority + 1*/
        if (profile_array[i].mgrConfig.ssidLength == sWifiInfo.ssidLength && \
            0 == strncmp(profile_array[i].mgrConfig.ssid, sWifiInfo.ssid, sWifiInfo.ssidLength) && \
            profile_array[i].mgrConfig.authMode == sWifiInfo.authMode) {
            profile_array[i].priority = 0;
        } else {
            profile_array[i].priority = profile_array[i].priority + 1;
        }
        if (profile_array[i].priority >= (INT_MAX - 1)) {
            needAdjust = true;
        }
    }
    /* If highest priority is >= (INT_MAX - 1), reset all profiles priority*/
    if (needAdjust) {
        aceWifiHal_resetAllProfilesPriority(profile_array);
    }

    return;
}

/* Save current profile priority to flash if profile already saved.
 */
static aceWifiHal_error_t aceWifiHal_saveHighestPriority(void)
{
    aceWifiHal_RTOS_config_t profiles[ACE_WIFI_MAX_CONFIGURED_NETWORKS] = {0};
    int priority = -1;
    int current = -1;
    int32_t retVal;
    int i;

    // Get all profiles from KVS
    retVal = aceKeyValueDsHal_get(WIFI_KEY_PROFILE, profiles, sizeof(profiles));
    if (retVal != sizeof(profiles)) {
        ASD_LOG_E(wifi_hal, "%s: KVS get profiles failed", __FUNCTION__);
        return aceWifiHal_ERROR_FAILURE;
    }

    for (i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        if (profiles[i].mgrConfig.ssidLength == 0) {
            continue;
        }
        if (profiles[i].priority > priority) {
            priority = profiles[i].priority;
        }
        if (profiles[i].mgrConfig.ssidLength == sWifiInfo.ssidLength &&
            strncmp(profiles[i].mgrConfig.ssid, sWifiInfo.ssid, sWifiInfo.ssidLength) == 0 &&
            profiles[i].mgrConfig.authMode == sWifiInfo.authMode) {
            current = i;
        }
    }

    if (current == -1) {
        ASD_LOG_E(wifi_hal, "%s: User did not save current profile", __FUNCTION__);
        return aceWifiHal_ERROR_FAILURE;
    }

    if (profiles[current].priority == priority) {
        ASD_LOG_I(wifi_hal, "%s: Current profile priority is already the highest", __FUNCTION__);
        return aceWifiHal_ERROR_SUCCESS;
    }

    profiles[current].priority = priority + 1;
    if (profiles[current].priority >= (INT_MAX - 1)) {
        aceWifiHal_resetAllProfilesPriority(profiles);
    }

    // Save all profiles to KVS
    retVal = aceKeyValueDsHal_setWithEncryption(WIFI_KEY_PROFILE, profiles, sizeof(profiles));
    if (retVal != ACE_STATUS_OK) {
        ASD_LOG_E(wifi_hal, "%s: KVS set profiles failed", __FUNCTION__);
        return aceWifiHal_ERROR_FAILURE;
    }

    return aceWifiHal_ERROR_SUCCESS;
}
#endif

/* Make connect config */
static void makeConnectConfig(const char* ssid,
                              uint8_t ssidLength,
                              uint8_t* bssid,
                              aceWifiHal_authMode_t authMode,
                              aceWifiHal_connectConfig_t* retConfig) {
    memset(retConfig, 0, sizeof(aceWifiHal_connectConfig_t));
    retConfig->ssidLength = ssidLength;
    memcpy(retConfig->ssid, ssid, aceWifiHal_MAX_SSID_LEN);
    if (bssid) {
        memcpy(retConfig->bssid, bssid, aceWifiHal_BSSID_LEN);
    }
    retConfig->authMode = authMode;
}

/* Auto-connect to the highest priority profile
 * TODO: sync algorithm with Linux
 */
static aceWifiHal_error_t aceWifiHal_autoConnect(void) {
    ASD_LOG_I(wifi_hal, "aceWifiHal_autoConnect()");
    aceWifiHal_RTOS_config_t* profile = aceWifiHal_findHighestPriorityEnabledProfile();
    if (profile != NULL) {
        aceWifiHal_connectConfig_t config;
        makeConnectConfig(profile->mgrConfig.ssid,
                          profile->mgrConfig.ssidLength,
                          NULL,
                          profile->mgrConfig.authMode,
                          &config);
        return aceWifiHal_connect(&config);
    }
    return aceWifiHal_ERROR_FAILURE;
}

static uint8_t aceWifiHal_btoh(char ch) {
    if (ch >= '0' && ch <= '9') {
        return (ch - '0');
    }
    if (ch >= 'A' && ch <= 'F') {
        return (ch - 'A' + 0xa);
    }
    if (ch >= 'a' && ch <= 'f') {
        return (ch - 'a' + 0xa);
    }
    return 0;
}

static void aceWifiHal_atoh(char* src, uint8_t* dest, int8_t destLength) {
    for (int8_t i = 0; i < destLength; i++) {
        dest[i] = (aceWifiHal_btoh(src[i*2]) << 4) + (aceWifiHal_btoh(src[i*2+1]));
    }
}

//
// Private HAL functions
//
static int8_t aceWifiHal_init_done = false;
static int8_t aceWifiHal_init_done_need_notify = false;

static void aceWifiHal_init_done_cb_(WIFIEvent_t * xEvent) {
    ASD_LOG_I(wifi_hal, "aceWifiHal_init_done");
    aceWifiHal_init_done = true;

    // As Wifi hal init is invoked outside of ace wifi svc,
    // can't guarantee that ace wifi svc is up when wifi is ready.
    // Notify the wifi ready event then in hal_start, which
    // is triggered in wifi svc hsm and is guranteed that svc is up.
    if (aceWifiHal_init_done_need_notify) {
        aceWifiHal_init_done_need_notify = false;
        aceWifiHal_dispatchEvent(aceWifiHal_wifiReadyEvent, NULL, 0);
/* Modified by NXP: ACE_WIFI_ENABLE_MW_AUTO_CONNECT is currently not supported in the ACS.
 * After the ACS issue is fixed, change back the below "#ifndef ACE_WIFI_ENABLE_MW_AUTO_CONNECT_NXP"
 * to the original "#ifndef ACE_WIFI_ENABLE_MW_AUTO_CONNECT" and also replace the
 * ACE_WIFI_ENABLE_MW_AUTO_CONNECT_NXP define to the ACE_WIFI_ENABLE_MW_AUTO_CONNECT define
 * inside the Preprocessor section of the project Properties. */
#ifndef ACE_WIFI_ENABLE_MW_AUTO_CONNECT_NXP
        aceWifiHal_autoConnect();
#endif
    }
}

static void aceWifiHal_scan_done_cb_(WIFIEvent_t* event) {
    ASD_LOG_I(wifi_hal, "aceWifiHal_scan_done");
    aceWifiHal_dispatchEvent(aceWifiHal_scanDoneEvent, NULL, 0);
}

static void aceWifiHal_connected_cb_(WIFIEvent_t* event) {
    ASD_LOG_I(wifi_hal, "aceWifiHal_connected");

    // save wifiInfo for ext ip ready event
    memset(&sWifiInfo, 0, sizeof(sWifiInfo));
    memcpy(&sWifiInfo.bssid, &(event->xInfo.xConnected.xConnectionInfo.ucBSSID), wificonfigMAX_BSSID_LEN);
    memcpy(&sWifiInfo.ssid, &(event->xInfo.xConnected.xConnectionInfo.ucSSID), wificonfigMAX_SSID_LEN);
    sWifiInfo.ssidLength = event->xInfo.xConnected.xConnectionInfo.ucSSIDLength;
    aceWifiHal_mapChannelToFreq(event->xInfo.xConnected.xConnectionInfo.ucChannel, &sWifiInfo.frequency);
    sWifiInfo.authMode = aceWifiHal_convertAuthMode(event->xInfo.xConnected.xConnectionInfo.xSecurity);

    aceWifiHal_networkStateInfo_t networkStateInfo;
    memcpy(&networkStateInfo.wifiInfo, &sWifiInfo, sizeof(networkStateInfo.wifiInfo));
    networkStateInfo.networkState = aceWifiHal_NET_STATE_L2_CONNECTED;
    aceWifiHal_dispatchEvent(aceWifiHal_networkStateEvent,
                             (uint8_t*)&networkStateInfo,
                             sizeof(networkStateInfo));

    networkState = aceWifiHal_NET_STATE_L2_CONNECTED;
#ifdef ACE_WIFI_HAL_AUTO_CONNECT
    /* L2 connected, record current network state, clear connection failed count*/
    autoConnectRetryCount = 0;
#endif
}

static void aceWifiHal_ap_connected_cb_(WIFIEvent_t* event) {
    ASD_LOG_I(wifi_hal, "aceWifiHal_ap_connected");
    aceWifiHal_softAPState_t softAPState;
    memset(&softAPState, 0, sizeof(aceWifiHal_softAPState_t));
    softAPState = aceWifiHal_SOFTAP_CONNECTED;
    aceWifiHal_dispatchEvent(aceWifiHal_softAPStateEvent,
                             (uint8_t*)&softAPState,
                             sizeof(aceWifiHal_softAPState_t));
}

static void aceWifiHal_disconnected_cb_(WIFIEvent_t* event) {
    ASD_LOG_I(wifi_hal, "aceWifiHal_disconnected");
    aceWifiHal_networkStateInfo_t networkStateInfo;
    memset(&networkStateInfo, 0, sizeof(aceWifiHal_networkStateInfo_t));
    networkStateInfo.networkState = aceWifiHal_NET_STATE_DISCONNECTED;
    memcpy(&networkStateInfo.wifiInfo, &sWifiInfo, sizeof(networkStateInfo.wifiInfo));
    aceWifiHal_dispatchEvent(aceWifiHal_networkStateEvent,
                             (uint8_t*)&networkStateInfo,
                             sizeof(networkStateInfo));
#ifdef ACE_WIFI_HAL_AUTO_CONNECT
    aceWifiHal_networkState_t prevNetworkState = networkState;
#endif
    networkState = aceWifiHal_NET_STATE_DISCONNECTED;
#ifdef ACE_WIFI_HAL_AUTO_CONNECT
    ASD_LOG_D(wifi_hal, "%s:state=%u,reason=%u,retry=%u,flag=%s", __FUNCTION__, \
    prevNetworkState, sWifiInfo.reason, autoConnectRetryCount, disconnectFromRemoveNetwork ? "true" : "false");

    /* When networkState is OBTAINEDIP or L2_CONNECTED,
     * the flag of disconnect is true or disconnect reason is not LOCALLY_GENERATED,
     * continue to connect.
     */
    if ((prevNetworkState == aceWifiHal_NET_STATE_OBTAINEDIP || prevNetworkState == aceWifiHal_NET_STATE_L2_CONNECTED) && \
        (disconnectFromRemoveNetwork || sWifiInfo.reason != aceWifiHal_DISCONNECT_LOCALLY_GENERATED)) {

        if (disconnectFromRemoveNetwork) {
            disconnectFromRemoveNetwork = false;
            aceWifiHal_autoConnect();
            return;
        }

        autoConnectRetryCount++;
        aceWifiHal_reconnect();
    }
#endif
}

static void aceWifiHal_ap_disconnected_cb_(WIFIEvent_t* event) {
    ASD_LOG_I(wifi_hal, "aceWifiHal_ap_disconnected");
    aceWifiHal_softAPState_t softAPState;
    memset(&softAPState, 0, sizeof(aceWifiHal_softAPState_t));
    softAPState = aceWifiHal_SOFTAP_DISCONNECTED;
    aceWifiHal_dispatchEvent(aceWifiHal_softAPStateEvent,
                             (uint8_t*)&softAPState,
                             sizeof(aceWifiHal_softAPState_t));
}

static aceWifiHal_disconnectReason_t aceWifiHal_convertDisconnectReason(WIFIReason_t halReason) {
    aceWifiHal_disconnectReason_t aceReason;

    switch (halReason) {
        case eWiFiReasonAuthExpired:
        case eWiFiReasonAuthFailed:
        case eWiFiReason4WayTimeout:
        case eWiFiReason4WayIEDiffer:
        case eWiFiReason4WayFailed:
        case eWiFiReasonAKMPInvalid:
        case eWiFiReasonPairwiseCipherInvalid:
        case eWiFiReasonGroupCipherInvalid:
        case eWiFiReasonRSNVersionInvalid:
        case eWiFiReasonRSNCapInvalid:
        case eWiFiReasonGroupKeyUpdateTimeout:
        case eWiFiReasonCipherSuiteRejected:
        case eWiFiReason8021XAuthFailed:
            aceReason = aceWifiHal_DISCONNECT_AUTH_FAILURE;
            break;
        case eWiFiReasonAssocExpired:
        case eWiFiReasonAssocTooMany:
        case eWiFiReasonAssocPowerCapBad:
        case eWiFiReasonAssocSupChanBad:
        case eWiFiReasonAssocFailed:
            aceReason = aceWifiHal_DISCONNECT_ASSOC_FAILURE;
            break;
        default:
            aceReason = aceWifiHal_DISCONNECT_UNDEFINED;
    }
    return aceReason;
}

static void aceWifiHal_connection_failed_cb_(WIFIEvent_t* event) {
#ifdef ACE_WIFI_HAL_AUTO_CONNECT
    aceWifiHal_disconnectReason_t prevReason = sWifiInfo.reason;
    aceWifiHal_networkState_t prevNetworkState = networkState;
#endif
    if (sWifiInfo.reason != aceWifiHal_DISCONNECT_LOCALLY_GENERATED) {
        sWifiInfo.reason = aceWifiHal_convertDisconnectReason(event->xInfo.xDisconnected.xReason);
        ASD_LOG_E(wifi_hal, "aceWifiHal_connection_failed: reason %d", event->xInfo.xDisconnected.xReason);
    } else {
        ASD_LOG_E(wifi_hal, "aceWifiHal_connection_failed: reason LOCALLY_GENERATED");
    }

    aceWifiHal_networkStateInfo_t networkStateInfo;
    memset(&networkStateInfo, 0, sizeof(aceWifiHal_networkStateInfo_t));
    networkStateInfo.networkState = aceWifiHal_NET_STATE_DISCONNECTED;
    memcpy(&networkStateInfo.wifiInfo, &sWifiInfo, sizeof(networkStateInfo.wifiInfo));
    aceWifiHal_dispatchEvent(aceWifiHal_networkStateEvent,
                             (uint8_t*)&networkStateInfo,
                             sizeof(networkStateInfo));

    networkState = aceWifiHal_NET_STATE_DISCONNECTED;
#ifdef ACE_WIFI_HAL_AUTO_CONNECT
    ASD_LOG_D(wifi_hal, "%s:state=%u,reason=%u,retry=%u,flag=%s", __FUNCTION__, \
    prevNetworkState, prevReason, autoConnectRetryCount, disconnectFromRemoveNetwork ? "true" : "false");

    /* When networkState is CONNECTING,
     * the flag of disconnect is true or reason is not LOCALLY_GENERATED,
     * continue to connect.
     */
    if (prevNetworkState == aceWifiHal_NET_STATE_CONNECTING && \
        (disconnectFromRemoveNetwork || prevReason != aceWifiHal_DISCONNECT_LOCALLY_GENERATED)) {

        if (disconnectFromRemoveNetwork) {
            disconnectFromRemoveNetwork = false;
            autoConnectRetryCount = 0;
            aceWifiHal_autoConnect();
            return;
        }

        /*When retry count is up to MAX, connect to other profile, otherwise reconnect current profile.*/
        if (autoConnectRetryCount >= HAL_WIFI_MAX_RETRY_AUTO_CONNECT) {
            autoConnectRetryCount = 0;
            aceWifiHal_reduceCurrentProfilePriority();
            aceWifiHal_autoConnect();
        } else {
            autoConnectRetryCount++;
            aceWifiHal_reconnect();
        }
    } else {
        autoConnectRetryCount = 0;
    }
#endif
}

static void aceWifiHal_ip_ready_cb_(WIFIEvent_t* event) {

    ASD_LOG_I(wifi_hal, "aceWifiHal_ip_ready_cb_()");
    // check the ip address if, all 0, then not ip ready
    // ipv4 only right now
    if (event->xInfo.xIPReady.xIPAddress.ulAddress[0] == 0) {
        return;
    }

    ASD_LOG_I(wifi_hal, "IP ready");

    // Save Ip info.
    WIFIIPAddress_t *inIp = &event->xInfo.xIPReady.xIPAddress;
    memset(&sWifiInfo.ipAddress, 0, sizeof(sWifiInfo.ipAddress));
    if (inIp->xType == eWiFiIPAddressTypeV4) {
        sWifiInfo.ipAddress.type = aceWifiHal_IP_TYPE_IPV4;
    } else {
        sWifiInfo.ipAddress.type = aceWifiHal_IP_TYPE_IPV6;
    }
    memcpy(&sWifiInfo.ipAddress.ipAddress, &inIp->ulAddress, sizeof(sWifiInfo.ipAddress.ipAddress));

    aceWifiHal_networkStateInfo_t networkStateInfo;
    networkStateInfo.networkState = aceWifiHal_NET_STATE_OBTAINEDIP;
    memcpy(&networkStateInfo.wifiInfo, &sWifiInfo, sizeof(networkStateInfo.wifiInfo));

    // DO not call afr wifi api inside callback, let upper layer to call getConnectionInfo().

    aceWifiHal_dispatchEvent(aceWifiHal_networkStateEvent,
                             (uint8_t*)&networkStateInfo,
                             sizeof(networkStateInfo));

    networkState = aceWifiHal_NET_STATE_OBTAINEDIP;
#ifdef ACE_WIFI_HAL_AUTO_CONNECT
    aceWifiHal_saveHighestPriority();
#endif
}

static void aceWifiHal_softAP_state_cb_(WIFIEvent_t* event) {
    bool isUp = event->xInfo.xAPStateChanged.ucState == 1;
    ASD_LOG_I(wifi_hal, "aceWifiHal_softAP %s", (isUp ? "UP" : "DOWN"));
    aceWifiHal_softAPState_t softAPState;
    memset(&softAPState, 0, sizeof(aceWifiHal_softAPState_t));
    softAPState = isUp? aceWifiHal_SOFTAP_UP : aceWifiHal_SOFTAP_DOWN;
    aceWifiHal_dispatchEvent(aceWifiHal_softAPStateEvent,
                             (uint8_t*)&softAPState,
                             sizeof(aceWifiHal_softAPState_t));
}

static aceWifiHal_error_t aceWifiHal_mapFreqToChannel(uint16_t frequency, uint8_t* channel) {
    aceWifiHal_error_t err = aceWifiHal_ERROR_SUCCESS;

    if (frequency >= 2412 && frequency <= 2484) {
        ASD_LOG_I(wifi_hal, "Map to 2GHz channels");
        if ((frequency - 2412) % 5 == 0) {
            *channel = 1 + (frequency - 2412) / 5;
        } else if (frequency == 2484) {
            *channel = 14;
        } else {
            ASD_LOG_E(wifi_hal, "Cannot map 2GHz frquency %d to channel", frequency);
            err = aceWifiHal_ERROR_FAILURE;
        }
    } else if (frequency >= 5180 && frequency <= 5825) {
        ASD_LOG_I(wifi_hal, "Map to 5GHz channels");
        if (frequency >= 5180 && frequency <= 5320 && (frequency - 5180) % 20 == 0) {
            *channel = 36 + ((frequency - 5180) / 20) * 4;
        } else if (frequency >= 5500 && frequency <= 5700 && (frequency - 5500) % 20 == 0) {
            *channel = 100 + ((frequency - 5500) / 20) * 4;
        } else if (frequency >= 5745 && frequency <= 5825 && (frequency - 5745) % 20 == 0) {
            *channel = 149 + ((frequency - 5745) / 20) * 4;
        } else {
            ASD_LOG_E(wifi_hal, "Cannot map 5GHz frquency %d to channel", frequency);
            err = aceWifiHal_ERROR_FAILURE;
        }
    } else {
        ASD_LOG_E(wifi_hal, "Cannot map frequency %d to channel", frequency);
        err = aceWifiHal_ERROR_FAILURE;
    }
    return err;
}

static aceWifiHal_error_t aceWifiHal_mapChannelToFreq(uint8_t channel, uint16_t* frequency) {
    if (channel == 14) {
        *frequency = 2484;
    } else if (channel >= 1 && channel <= 13) {
        *frequency = 2412 + (channel - 1) * 5;
    } else if (channel >= 36 && channel <= 64) {
        *frequency = 5180 + (channel - 36) / 4 * 20;
    } else if (channel >= 100 && channel <= 140) {
        *frequency = 5500 + (channel - 100) / 4 * 20;
    } else if (channel >= 149 && channel <= 165) {
        *frequency = 5745 + (channel - 149) / 4 * 20;
    } else {
        ASD_LOG_E(wifi_hal, "Non-exist channel: %d", channel);
        return aceWifiHal_ERROR_FAILURE;
    }
    return aceWifiHal_ERROR_SUCCESS;
}

static bool aceWifiHal_isHidden(const uint8_t* p_ssid, uint8_t length) {

    if (length == 0) {
        return true;
    }

    // TODO right now just check ascii case, will need to support unicode.
    const uint8_t* curr = p_ssid;
    const uint8_t* end = p_ssid + wificonfigMAX_SSID_LEN - 1;
    while ((*curr == '\0') && (curr < end)) {
        curr++;
    }
    if (curr == end) {
        return true;
    }
    return false;
}

//
// Public HAL functions
//
static bool init_invoked = false;
aceWifiHal_error_t aceWifiHal_init(aceWifiHal_config_t* conf) {

    // If wifi init is invoked already, do not do init again
    if (init_invoked) {
        return aceWifiHal_ERROR_SUCCESS;
    }
    init_invoked = true;

    /* User initial the parameters for wifi initial process,  system will determin
     * which wifi operation mode will be started , and adopt which settings for the
     * specific mode while wifi initial process is running
     */
    WIFI_Init();

    // Register WiFi events
    WIFI_RegisterEvent(eWiFiEventReady, aceWifiHal_init_done_cb_);
    WIFI_RegisterEvent(eWiFiEventScanDone, aceWifiHal_scan_done_cb_);
    WIFI_RegisterEvent(eWiFiEventConnected, aceWifiHal_connected_cb_);
    WIFI_RegisterEvent(eWiFiEventDisconnected, aceWifiHal_disconnected_cb_);
    WIFI_RegisterEvent(eWiFiEventConnectionFailed, aceWifiHal_connection_failed_cb_);
    WIFI_RegisterEvent(eWiFiEventAPStateChanged, aceWifiHal_softAP_state_cb_);
    WIFI_RegisterEvent(eWiFiEventAPStationConnected, aceWifiHal_ap_connected_cb_);
    WIFI_RegisterEvent(eWiFiEventAPStationDisconnected, aceWifiHal_ap_disconnected_cb_);

    // Register ip events
    WIFI_RegisterEvent(eWiFiEventIPReady, aceWifiHal_ip_ready_cb_);

    ASD_LOG_I(wifi_hal, "aceWifiHal_init success");
    return aceWifiHal_ERROR_SUCCESS;
}


aceWifiHal_error_t aceWifiHal_start(void) {
    int32_t retVal;
    int32_t version = -1;    // -1 indicates wifi profile doesn't exist

    aceWifiHal_initializeProfile(); // Initialize the buffer before load config
    retVal = aceKeyValueDsHal_get(WIFI_KEY_VERSION, &version, sizeof(version));
    if (version == WIFI_PROFILE_VERSION) {
        ASD_LOG_I(wifi_hal, "Profile version in KVS is %u", version);

        // Read profiles into memory
        retVal = aceKeyValueDsHal_get(WIFI_KEY_PROFILE, profile_array, sizeof(profile_array));
        if (retVal != sizeof(profile_array)) {
            ASD_LOG_E(wifi_hal, "KVS get profile failed");
            return aceWifiHal_ERROR_FAILURE;
        }

        for (int i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
            // at initialize, set all the profiles to be enabled.
            profile_array[i].mgrConfig.status = aceWifiHal_CONFIG_STATUS_ENABLED;
        }
    }
    else {
#ifdef V1_TO_V2_MIGRATION_ENABLE
        // If V1 profile exist, migrate to V2 profile and update version
        aceWifiHal_profile_v1_to_v2(&(profile_array[0].mgrConfig), &version);
        if (version == WIFI_PROFILE_VERSION_V1) {
            // priority is not defined in old profile.
            profile_array[0].priority = 0;
        }
#endif
        // Future profile migration can be put here

        // Store new profiles to kvs
        retVal = aceKeyValueDsHal_setWithEncryption(WIFI_KEY_PROFILE, profile_array, sizeof(profile_array));
        if (retVal != ACE_STATUS_OK) {
            ASD_LOG_E(wifi_hal, "KVS set profile failed");
            return aceWifiHal_ERROR_FAILURE;
        }

#ifdef V1_TO_V2_MIGRATION_ENABLE
        if (version == WIFI_PROFILE_VERSION_V1) {
            aceWifiHal_profile_remove_v1();
        }
#endif

        // Last step, update profile version to current number
        version = WIFI_PROFILE_VERSION;
        ASD_LOG_I(wifi_hal, "Initialize KVS for profile version %u", version);
        retVal = aceKeyValueDsHal_setWithEncryption(WIFI_KEY_VERSION, &version, sizeof(version));
        if (retVal != ACE_STATUS_OK) {
            ASD_LOG_E(wifi_hal, "KVS set version failed");
            return aceWifiHal_ERROR_FAILURE;
        }
    }

    if (aceWifiHal_init_done) {
        aceWifiHal_dispatchEvent(aceWifiHal_wifiReadyEvent, NULL, 0);
/* Modified by NXP: ACE_WIFI_ENABLE_MW_AUTO_CONNECT is currently not supported in the ACS.
 * After the ACS issue is fixed, change back the below "#ifndef ACE_WIFI_ENABLE_MW_AUTO_CONNECT_NXP"
 * to the original "#ifndef ACE_WIFI_ENABLE_MW_AUTO_CONNECT" and also replace the
 * ACE_WIFI_ENABLE_MW_AUTO_CONNECT_NXP define to the ACE_WIFI_ENABLE_MW_AUTO_CONNECT define
 * inside the Preprocessor section of the project Properties. */
#ifndef ACE_WIFI_ENABLE_MW_AUTO_CONNECT_NXP
        aceWifiHal_autoConnect();
#endif
    } else {
        // Hal is still in init, notify once it's ready.
        aceWifiHal_init_done_need_notify = true;
    }

    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_startScan(aceWifiHal_scanConfig_t* config) {
    WIFIScanConfig_t scanConfig;
    WIFIReturnCode_t retCode;
    uint8_t i;

    memset(&scanConfig, 0, sizeof(scanConfig));

    // Allow NULL config, treat it as broadcast scan only
    if (config) {
        scanConfig.ucBroadcast = config->broadcast;
        scanConfig.ucTargetedScanItemNum = config->targetedScanItemNum > wificonfigMAX_TARGETED_SCAN_NUM ? wificonfigMAX_TARGETED_SCAN_NUM : config->targetedScanItemNum;
        // Cover the case that upper layer does not set either ucBroadcast
        // or ucTargetedScanItemNum, for backward compatible.
        if ((config->broadcast == 0) && (config->targetedScanItemNum == 0)) {
            scanConfig.ucBroadcast = 1;
        }

        if (scanConfig.ucTargetedScanItemNum) {
            for (i = 0; i < scanConfig.ucTargetedScanItemNum; i++) {
                scanConfig.xScanList[i].ucSSIDLength
                     = config->scanList[i].ssidLength > wificonfigMAX_SSID_LEN ?
                       wificonfigMAX_SSID_LEN : config->scanList[i].ssidLength;
                memcpy(scanConfig.xScanList[i].ucSSID, config->scanList[i].ssid,
                       scanConfig.xScanList[i].ucSSIDLength);
            }
        }

        scanConfig.ucChannelNum
                     = config->channelNum > wificonfigMAX_SCAN_CHANNEL_NUM ?
                       wificonfigMAX_SCAN_CHANNEL_NUM : config->channelNum;
        if (scanConfig.ucChannelNum > 0) {
            memcpy(scanConfig.ucChannel, config->channelList, scanConfig.ucChannelNum);
        }
    }

    ASD_LOG_I(wifi_hal, "Init scan");

    retCode = WIFI_StartScan(&scanConfig);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "SDK scan init failed");
        return aceWifiHal_ERROR_FAILURE;
    }
    return aceWifiHal_ERROR_SUCCESS;
}

static aceWifiHal_authMode_t aceWifiHal_convertAuthMode(WIFISecurity_t xSecurity) {
    aceWifiHal_authMode_t authMode;

    switch(xSecurity) {
        case eWiFiSecurityOpen:
            authMode = aceWifiHal_AUTH_MODE_OPEN;
            break;
        case eWiFiSecurityWEP:
            authMode = aceWifiHal_AUTH_MODE_WEP;
            break;
        case eWiFiSecurityWPA:
            authMode = aceWifiHal_AUTH_MODE_WPA_PSK;
            break;
        case eWiFiSecurityWPA2:
            authMode = aceWifiHal_AUTH_MODE_WPA2_PSK;
            break;
        default:
            authMode = aceWifiHal_AUTH_MODE_MAX;
    }
    return authMode;
}

/* Get scan results. In returned scan results, duplicated (keep the strongest rssi)
   and empty ssid (length=0) are removed.
   For the client needs to get the entire raw scan result, use getDetailedScanResults()
   Note:  Passed in results can be empty or already contains previous scan results.
          If it contains previous scan results already, will be merged with the new
          scan results, filtering out the duplicated ssid.
          if scanResultList->length is 0, then passed in scan results is empty */
aceWifiHal_error_t aceWifiHal_getScanResults(aceWifiHal_scanResultList_t* scanResultList) {
    WIFIReturnCode_t retCode;
    aceWifiHal_scanResult_t* result;
    const WIFIScanResultExt_t* item;  // points to the new result item
    uint16_t count;
    uint16_t i;
    uint16_t retResultLength = scanResultList->length;
    aceWifiHal_scanResult_t tempResult;

    ASD_LOG_I(wifi_hal, "aceWifiHal_getScanResults, in len = %d", scanResultList->length);
    // if the length passed in is invalid, reset to 0
    if (scanResultList->length > ACE_WIFI_MAX_SCAN_RESULTS) {
        retResultLength = scanResultList->length = 0;
    }

    retCode = WIFI_GetScanResults(&item, &count);
    if (retCode != eWiFiSuccess) {
        return aceWifiHal_ERROR_FAILURE;
    }
    if (count > ACE_WIFI_MAX_SCAN_RESULTS) {
        count = ACE_WIFI_MAX_SCAN_RESULTS;
    }

    // Replace the duplicated ones with higher rssi.
    // For new items, if array is not full, add to the end (avoid memory copy/moving)
    // If array is full, replace the lowest the rssi item with the new one.
    // for each item in new driver scan results:
    for (i = 0; i < count; i++, item++) {
        //ASD_LOG_I(wifi_hal, "new result [%d] ssid=%s, len=%d, rssi=%d", i, item->ucSSID, item->ucSSIDLength, item->cRSSI);
        // Filter out hidden and duplicate ssid in scan results
        if (aceWifiHal_isHidden(item->ucSSID, item->ucSSIDLength) == false) {
            bool found = false;
            aceWifiHal_authMode_t itemAuthMode = aceWifiHal_convertAuthMode(item->xSecurity);
            uint16_t lowestRssiPos = retResultLength? (retResultLength - 1) : 0;
            // check if this ssid is a duplicate, iterate through the in orig scanresult.
            for (int k = 0; k < retResultLength; k++) {
                // Original array is sorted, the last item in the array
                // should be the lowest rssi. However, due to the replace of
                // items (duplicate with higher rssi) and new items added,
                // not sorted anymore.
                if ((retResultLength == ACE_WIFI_MAX_SCAN_RESULTS) &&
                     scanResultList->list[k].rssi < scanResultList->list[lowestRssiPos].rssi) {
                    lowestRssiPos = k;  //pos to replace item when array is full
                }
                if ((item->ucSSIDLength == scanResultList->list[k].ssidLength) &&
                    !memcmp(item->ucSSID, scanResultList->list[k].ssid, item->ucSSIDLength) &&
                    (itemAuthMode == scanResultList->list[k].authMode)) {
                    // is duplicate
                    if (item->cRSSI > scanResultList->list[k].rssi) {
                        scanResultList->list[k].rssi = item->cRSSI;
                    }
                    found = true;
                    break;
                }
            }
            if (found == false) {
                uint16_t insertPos = retResultLength;
                if (retResultLength == ACE_WIFI_MAX_SCAN_RESULTS) {
                    if (item->cRSSI < scanResultList->list[lowestRssiPos].rssi) {
                        // In the rssi same case, replace the old one with new one.
                        continue;
                    }
                    insertPos = lowestRssiPos;
                } else {
                    retResultLength++;
                }

                result = &scanResultList->list[insertPos];
                memset(result, 0, sizeof(aceWifiHal_scanResult_t));
                memcpy(result->ssid, item->ucSSID, item->ucSSIDLength);
                result->ssidLength = item->ucSSIDLength;
                result->authMode = itemAuthMode;
                result->rssi = item->cRSSI;
                //ASD_LOG_I(wifi_hal, "add pos =%d, ssid=%s, len=%d, rssi=%d", insertPos, result->ssid, result->ssidLength, result->rssi);
            }
        }
    }


    // Sorting
    if (retResultLength > 0) {
        for (i = (retResultLength - 1); i > 0; i--) {
            bool changed = false;
            for (int k = 0; k < i; k++) {
                if (scanResultList->list[k].rssi < scanResultList->list[k + 1].rssi) {
                    memcpy(&tempResult, &scanResultList->list[k], sizeof(tempResult));
                    memcpy(&scanResultList->list[k], &scanResultList->list[k + 1], sizeof(tempResult));
                    memcpy(&scanResultList->list[k + 1], &tempResult, sizeof(tempResult));
                    changed = true;
                }
            }
            if (changed == false) {
                break;
            }
        }
    }

    ASD_LOG_I(wifi_hal, "scan result SSID number: %d", retResultLength);
    scanResultList->length = retResultLength;
    return aceWifiHal_ERROR_SUCCESS;
}

/* Get scan results, which is the original unfiltered scan list from the driver
*/
aceWifiHal_error_t aceWifiHal_getDetailedScanResults(aceWifiHal_detailedScanResultList_t* scanResultList) {
    aceWifiHal_detailedScanResult_t* result;
    const WIFIScanResultExt_t* item;
    uint16_t count;
    WIFIReturnCode_t retCode;
    uint16_t i;
    uint16_t j;

    retCode = WIFI_GetScanResults(&item, &count);
    if (retCode != eWiFiSuccess) {
        return aceWifiHal_ERROR_FAILURE;
    }
    if (count > ACE_WIFI_MAX_SCAN_RESULTS) {
        count = ACE_WIFI_MAX_SCAN_RESULTS;
    }

    for (i = 0, j = 0; i < count; i++, item++) {
        result = &scanResultList->list[j];

        // Do not filter out duplicated ssid in scan results
        memcpy(result->ssid, item->ucSSID, item->ucSSIDLength);
        result->ssidLength = item->ucSSIDLength;
        result->authMode = aceWifiHal_convertAuthMode(item->xSecurity);
        result->rssi = item->cRSSI;
        memcpy(result->bssid, item->ucBSSID, aceWifiHal_BSSID_LEN);
        aceWifiHal_mapChannelToFreq(item->ucChannel, &(result->frequency));
        j++;
    }

    ASD_LOG_I(wifi_hal, "detailed scan result SSID number: %d", j);
    scanResultList->length = j;
    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_connect(const aceWifiHal_connectConfig_t *inConfig) {

    char ssid[inConfig->ssidLength + 1];
    ssid[inConfig->ssidLength] = '\0';
    memcpy(ssid, inConfig->ssid, inConfig->ssidLength);
    ASD_LOG_I(wifi_hal, "aceWifiHal_connect %s auth=%d", obfuscateSsid(ssid), inConfig->authMode);

    aceWifiHal_RTOS_config_t* profile;
    int i;

    // FIXME: compile-time flags in aws_wifi_config.h vs Makefile???
    for (i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        profile = &profile_array[i];
        if ((profile->mgrConfig.ssidLength > 0) &&
            (profile->mgrConfig.ssidLength == inConfig->ssidLength) &&
            strncmp(inConfig->ssid, profile->mgrConfig.ssid, profile->mgrConfig.ssidLength) == 0) {
            // If the authMode is not specified, pick the first one that matches ssid.
            if ((inConfig->authMode != aceWifiHal_AUTH_MODE_MAX) &&
                (profile->mgrConfig.authMode != inConfig->authMode)) {
                    continue;
            }
            break;
        }
    }
    if (i == ACE_WIFI_MAX_CONFIGURED_NETWORKS) {
        ASD_LOG_E(wifi_hal, "Did not find SSID in profiles");
        return aceWifiHal_ERROR_FAILURE;
    }

    // Promote the priority of connecting profile, even though it may fail
    // TODO: check this against Linux behavior
    int highest = aceWifiHal_findHighestPriority();
    if (highest == -1) {
        profile->priority = 1;
    } else {
        profile->priority = highest + 1;
    }
    if (profile->priority >= (INT_MAX - 1)) {
        aceWifiHal_resetAllProfilesPriority(profile_array);
    }

    WIFIReturnCode_t retCode;
    WIFINetworkParamsExt_t networkParams;
    memset(&networkParams, 0, sizeof(WIFINetworkParamsExt_t));

    memcpy(networkParams.ucSSID, profile->mgrConfig.ssid, aceWifiHal_MAX_SSID_LEN);
    memcpy(networkParams.ucBSSID, inConfig->bssid, wificonfigMAX_BSSID_LEN);
    networkParams.ucSSIDLength = profile->mgrConfig.ssidLength;

    switch (profile->mgrConfig.authMode) {
        case aceWifiHal_AUTH_MODE_WPA_PSK:
            networkParams.xSecurity = eWiFiSecurityWPA;
            memcpy(networkParams.xPassword.xWPA.cPassphrase,
                   profile->mgrConfig.psk,
                   profile->mgrConfig.pskLength);
            networkParams.xPassword.xWPA.ucLength = profile->mgrConfig.pskLength;
            break;
        case aceWifiHal_AUTH_MODE_WPA2_PSK:
            networkParams.xSecurity = eWiFiSecurityWPA2;
            memcpy(networkParams.xPassword.xWPA.cPassphrase,
                   profile->mgrConfig.psk,
                   profile->mgrConfig.pskLength);
            networkParams.xPassword.xWPA.ucLength = profile->mgrConfig.pskLength;
            break;
        case aceWifiHal_AUTH_MODE_WEP:
            networkParams.xSecurity = eWiFiSecurityWEP;
            networkParams.ucWEPKeyIndex = profile->mgrConfig.wepKeyIndex;
            for (i = 0; i < 4; i++) {
                aceWifiHal_wepKey_t* inKey = &profile->mgrConfig.wepKeys[i];
                WIFIWEPKey_t* outKey = &networkParams.xPassword.xWEP[i];
                outKey->ucLength = inKey->keyLength;
                memcpy(outKey->cKey, inKey->wepKey, inKey->keyLength);
            }
            break;
        case aceWifiHal_AUTH_MODE_OPEN:
            networkParams.xSecurity = eWiFiSecurityOpen;
            break;
        default:
            ASD_LOG_E(wifi_hal, "Unknown auth mode - %d", profile->mgrConfig.authMode);
            return aceWifiHal_ERROR_FAILURE;
    }

    if (WIFI_IsConnectedExt(&networkParams)) {
         // If this profile is connected already, ignore
         ASD_LOG_I(wifi_hal, "already connected to the same AP");
         return aceWifiHal_ERROR_SUCCESS;
    }

#ifdef ACE_WIFI_HAL_AUTO_CONNECT
    /* When new connect is coming, but now is in connecting,
     * If new connect is same to current, directly return SUCCESS.
     * If new connect is different to current, disconnect current connect, then connect to new.
     */
    if (networkState == aceWifiHal_NET_STATE_CONNECTING) {
        if (profile->mgrConfig.status == aceWifiHal_CONFIG_STATUS_CURRENT) {
            ASD_LOG_I(wifi_hal, "Is connecting this profile!");
            return aceWifiHal_ERROR_SUCCESS;
        } else {
            ASD_LOG_I(wifi_hal, "Disconnect the last profile!");
            aceWifiHal_disconnect();
        }
    }
#endif

    retCode = WIFI_StartConnectAP(&networkParams);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "aceWifiHal_connect StartConnectAP failed %d", retCode);
        return aceWifiHal_ERROR_FAILURE;
    }

    for (i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        // reset the previous connected profile status to enabled
        if (profile_array[i].mgrConfig.status == aceWifiHal_CONFIG_STATUS_CURRENT) {
            profile_array[i].mgrConfig.status = aceWifiHal_CONFIG_STATUS_ENABLED;
        }
    }
    profile->mgrConfig.status = aceWifiHal_CONFIG_STATUS_CURRENT;

    // TODO, Just temporary, remove once rtos ace wifi statemachine is ready
    memset(&sWifiInfo, 0, sizeof(sWifiInfo));
    memcpy(&sWifiInfo.ssid, profile->mgrConfig.ssid, aceWifiHal_MAX_SSID_LEN);
    sWifiInfo.ssidLength = profile->mgrConfig.ssidLength;
    sWifiInfo.authMode = profile->mgrConfig.authMode;
    sWifiInfo.reason = aceWifiHal_DISCONNECT_UNDEFINED;

    aceWifiHal_networkStateInfo_t networkStateInfo;
    memcpy(&networkStateInfo.wifiInfo, &sWifiInfo, sizeof(networkStateInfo.wifiInfo));
    networkStateInfo.networkState = aceWifiHal_NET_STATE_CONNECTING;
    aceWifiHal_dispatchEvent(aceWifiHal_networkStateEvent,
                             (uint8_t*)&networkStateInfo,
                             sizeof(networkStateInfo));

    networkState = aceWifiHal_NET_STATE_CONNECTING;

    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_disconnect(void) {
    ASD_LOG_I(wifi_hal, "aceWifiHal_disconnect");
    WIFIReturnCode_t retCode;

    sWifiInfo.reason = aceWifiHal_DISCONNECT_LOCALLY_GENERATED;
    retCode = WIFI_StartDisconnect();
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "WIFI_StartDisconnect failed %d", retCode);
        return aceWifiHal_ERROR_FAILURE;
    }

    return aceWifiHal_ERROR_SUCCESS;
}

/*
 * Reconnect to the currently active access point, if we are currently
 * disconnected.
*/
aceWifiHal_error_t aceWifiHal_reconnect(void) {
    int i = 0;
    ASD_LOG_I(wifi_hal, "aceWifiHal_reconnect");

    // Note, this api is for the case when wifi is disconnected
    // already. For the case wifi is connected already, should have
    // another api called "aceWifiHal_reassociate()".
    if (WIFI_IsConnected()) {
        ASD_LOG_E(wifi_hal, "aceWifiHal_reconnect(), wifi is connected already");
        return aceWifiHal_ERROR_FAILURE;
    }

    // Get the current
    for (i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        /*The length of ssid is 0, skip ...*/
        if (profile_array[i].mgrConfig.ssidLength == 0) {
            continue;
        }
        if (profile_array[i].mgrConfig.status == aceWifiHal_CONFIG_STATUS_CURRENT) {
            aceWifiHal_connectConfig_t config;
            makeConnectConfig(profile_array[i].mgrConfig.ssid,
                              profile_array[i].mgrConfig.ssidLength,
                              NULL,
                              profile_array[i].mgrConfig.authMode,
                              &config);
            return aceWifiHal_connect(&config);
        }
    }
    ASD_LOG_E(wifi_hal, "aceWifiHal_reconnect(), no active profile");
    return aceWifiHal_ERROR_FAILURE;
}

aceWifiHal_networkState_t aceWifiHal_getNetworkState(void) {
    return networkState;
}

static void aceWifiHal_printStaInfo(aceWifiHal_wifiInfo_t* wifiInfo) {
    ASD_LOG_D(wifi_hal, "WiFi is connected:\n");
    ASD_LOG_D(wifi_hal, " Frequency: %d", wifiInfo->frequency);
#ifdef DEBUG
    ASD_LOG_D(wifi_hal, ", BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
            wifiInfo->bssid[0],
            wifiInfo->bssid[1],
            wifiInfo->bssid[2],
            wifiInfo->bssid[3],
            wifiInfo->bssid[4],
            wifiInfo->bssid[5]);
#else
    ASD_LOG_D(wifi_hal, ", BSSID:xxx");
#endif
    ASD_LOG_D(wifi_hal, ", ssidLen: %d, SSID: %.*s", wifiInfo->ssidLength, obfuscateSsid(wifiInfo->ssid));
    ASD_LOG_D(wifi_hal, "\n");
}

aceWifiHal_error_t aceWifiHal_getConnectionInfo(aceWifiHal_wifiInfo_t* wifiInfo) {
    WIFIConnectionInfo_t info;
    WIFIReturnCode_t retCode;

    if (!WIFI_IsConnected()) {
        ASD_LOG_E(wifi_hal, "Wifi is not Connected");
        return aceWifiHal_ERROR_FAILURE;
    }

    if (wifiInfo == NULL) {
        ASD_LOG_E(wifi_hal, "aceWifiHal_getConnectionInfo NULL param");
        return aceWifiHal_ERROR_INVALID_PARAM;
    }

    retCode = WIFI_GetConnectionInfo(&info);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "aceWifiHal_getConnectionInfo failed");
        return aceWifiHal_ERROR_FAILURE;
    }

    memcpy(wifiInfo->ssid, info.ucSSID, info.ucSSIDLength);
    wifiInfo->ssidLength = info.ucSSIDLength;
    memcpy(wifiInfo->bssid, info.ucBSSID, aceWifiHal_BSSID_LEN);
    aceWifiHal_mapChannelToFreq(info.ucChannel, &(wifiInfo->frequency));
    wifiInfo->authMode = aceWifiHal_convertAuthMode(info.xSecurity);

    ASD_LOG_I(wifi_hal, "Frequency info: %d", wifiInfo->frequency);

    memcpy(&wifiInfo->ipAddress, &sWifiInfo.ipAddress, sizeof(wifiInfo->ipAddress));

    aceWifiHal_printStaInfo(wifiInfo);
    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_createAP(aceWifiHal_apConfig_t* config)
{
    WIFINetworkParamsExt_t networkParams;
    WIFIReturnCode_t retCode;

    // TODO: Implement default AP, ssid would be Amazon-XXX (XXX is the last three
    // digit of DSN), psk would be non-secure.
    if ((config->frequency == 0) && (config->ssidLength == 0)) {
        ASD_LOG_E(wifi_hal, "Default soft AP is not supported yet");
        return aceWifiHal_ERROR_FAILURE;
    }

    if (config->frequency == 0) {
        ASD_LOG_E(wifi_hal, "No frequency specified, use the default one #2437");
        config->frequency = 2437;
    }

    memcpy(networkParams.ucSSID, config->ssid, config->ssidLength);
    networkParams.ucSSIDLength = config->ssidLength;

    if (config->pskLength == 0) {
        networkParams.xSecurity = eWiFiSecurityOpen;
    }
    else {
        networkParams.xSecurity = eWiFiSecurityWPA2;
        memcpy(&networkParams.xPassword.xWPA.cPassphrase, config->psk, config->pskLength);
        networkParams.xPassword.xWPA.ucLength = config->pskLength;
    }
    aceWifiHal_mapFreqToChannel(config->frequency, &networkParams.ucChannel);

    retCode = WIFI_ConfigureAPExt(&networkParams);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "aceWifiHal_createAP ConfigureAP failed");
        return aceWifiHal_ERROR_FAILURE;
    }

    // StartAP is a sync call
    retCode = WIFI_StartAP();
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "aceWifiHal_createAP StartAP failed");
        return aceWifiHal_ERROR_FAILURE;
    }
    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_removeAP(void) {
    WIFIReturnCode_t retCode;

    // StopAP is a sync call
    retCode = WIFI_StopAP();
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "aceWifiHal_removeAP failed");
        return aceWifiHal_ERROR_FAILURE;
    }
    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_getStaList(aceWifiHal_apStaList_t* staList,
                                         uint32_t* staListSize)
{
    aceWifiHal_apStaList_t* iter = staList;
    uint8_t cnt = WIFI_MAX_NUMBER_OF_STA;
    uint32_t i;
    WIFIReturnCode_t retCode;

    // FIXME: harmonize aceWifiHal_apStaList_t with WIFIStationInfo_t to avoid malloc
    WIFIStationInfo_t* list = (WIFIStationInfo_t*)malloc(cnt * sizeof(WIFIStationInfo_t));

    retCode = WIFI_GetStationList(list, &cnt);
    if (retCode != eWiFiSuccess) {
        free(list);
        ASD_LOG_E(wifi_hal, "aceWifiHal_getStaList failed");
        return aceWifiHal_ERROR_FAILURE;
    }

    if (*staListSize > cnt) *staListSize = cnt;

    for (i = 0, iter->next = NULL; i < *staListSize;) {
        memcpy(iter->staInfo.mac, list->ucMAC, aceWifiHal_BSSID_LEN);

        // TODO: fill in IP info

        if (++i == *staListSize) break;

        iter->next = iter + 1;
        iter = iter->next;
        iter->next = NULL;

        list++;
    }
    free(list);

    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_disconnectSta(uint8_t* mac) {
    WIFIReturnCode_t retCode;

    retCode = WIFI_StartDisconnectStation(mac);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "aceWifiHal_disconnectSta failed");
        return aceWifiHal_ERROR_FAILURE;
    }
    return aceWifiHal_ERROR_SUCCESS;
}

/* Profile Management */

/* Read config file, read the whole file at one time
 * Read the config file into the profile array
 */
aceWifiHal_error_t aceWifiHal_getConfiguredNetworks(aceWifiHal_configList_t* configList) {
    int8_t curCfg = 0;
    memset(configList, 0, sizeof(aceWifiHal_configList_t));
    for (int i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        if (profile_array[i].mgrConfig.ssidLength > 0) {
            memcpy(&(configList->list[curCfg]),
                   &(profile_array[i].mgrConfig),
                   sizeof(aceWifiHal_config_t));
            curCfg++;
        }
    }
    configList->length = curCfg;
    return aceWifiHal_ERROR_SUCCESS;
}

/* Write config file, write the whole file at one time
 * Write the profile array into the config file
 */
aceWifiHal_error_t aceWifiHal_saveConfig(void) {
    int32_t retVal;

#ifdef ACE_WIFI_HAL_AUTO_CONNECT
    aceWifiHal_resetAllProfilesPriority(profile_array);
#endif
    // Save all profiles to KVS
    retVal = aceKeyValueDsHal_setWithEncryption(WIFI_KEY_PROFILE, profile_array, sizeof(profile_array));
    if (retVal != ACE_STATUS_OK) {
        ASD_LOG_E(wifi_hal, "KVS set profile failed");
        return aceWifiHal_ERROR_FAILURE;
    }
    return aceWifiHal_ERROR_SUCCESS;
}

/* Add a new profile
 * Add new profile into profile array
 */
aceWifiHal_error_t aceWifiHal_addNetwork(const aceWifiHal_config_t* config) {
    /* find if there is a empty profile which we can overwrite */
    int cur_priority = -1; /* record the current highest priority */
    int cur_priority_index = -1; /* record the index for current highest priority */
    int valid_profile_n = 0; /* record current profile count */
    int lowest_priority = -1; /* record current lowest priority */
    int same_ssid_index = -1; /* record the profile index with same SSID */
    int empty_ssid_index = -1; /* record 1st empty profile index */
#ifdef ACE_WIFI_HAL_AUTO_CONNECT
    int lowest_priority_index = -1; /* record the lowest priority profile index */
#endif

    for (int i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        if (profile_array[i].mgrConfig.ssidLength > 0) {
            valid_profile_n++;
            if (profile_array[i].priority > cur_priority) {
                cur_priority = profile_array[i].priority;
                cur_priority_index = i;
            }

            if (lowest_priority < 0 || profile_array[i].priority < lowest_priority) {
                lowest_priority = profile_array[i].priority;
#ifdef ACE_WIFI_HAL_AUTO_CONNECT
                lowest_priority_index = i;
#endif
            }

            if (config->ssidLength == profile_array[i].mgrConfig.ssidLength &&
                strncmp(config->ssid, profile_array[i].mgrConfig.ssid, config->ssidLength) == 0 &&
                config->authMode == profile_array[i].mgrConfig.authMode)
                same_ssid_index = i;
        } else if (empty_ssid_index == -1) {
            empty_ssid_index = i;
        }
    }

    ASD_LOG_I(wifi_hal, "Current highest priority: %d", cur_priority);
    ASD_LOG_I(wifi_hal, "Current profile count: %d", valid_profile_n);

    /* Overflow handle, when cur priority is >= (INT_MAX-1), reset all priority from 0, 1...valid_profile_n-1 */
    if (cur_priority >= (INT_MAX - 1)) {
        aceWifiHal_resetAllProfilesPriority(profile_array);
    }

    /* Update valid info and priority info for the new profile */
    int priority_new = (cur_priority_index == -1) ? 0 : (profile_array[cur_priority_index].priority + 1);

    /* Set the profile into profile array */
    /* If same SSID exist, need to update the content */
    if (same_ssid_index != -1) {
        // keep the current status
        aceWifiHal_configStatus_t curStatus = profile_array[same_ssid_index].mgrConfig.status;
        aceWifiHal_setConfig(&(profile_array[same_ssid_index]), config, priority_new);
        profile_array[same_ssid_index].mgrConfig.status = curStatus;
        ASD_LOG_I(wifi_hal, "Found same SSID with profile index: %d", same_ssid_index);
        return aceWifiHal_ERROR_SUCCESS;
    }

    if (valid_profile_n == ACE_WIFI_MAX_CONFIGURED_NETWORKS) {
#ifdef ACE_WIFI_HAL_AUTO_CONNECT
        /* Remove the lowest priority profile, then add new profile. */
        char ssid[aceWifiHal_MAX_SSID_LEN + 1] = {0};

        ASD_LOG_I(wifi_hal, "Remove the lowest priority profile with index: %d", lowest_priority_index);
        memcpy(ssid, profile_array[lowest_priority_index].mgrConfig.ssid, profile_array[lowest_priority_index].mgrConfig.ssidLength);
        aceWifiHal_removeNetwork(ssid, profile_array[lowest_priority_index].mgrConfig.authMode);

        ASD_LOG_I(wifi_hal, "Add new profile with index: %d", lowest_priority_index);
        return aceWifiHal_setConfig(&(profile_array[lowest_priority_index]), config, priority_new);
#else
        // profile array is full, we will not delete the profile without notifying the
        // customer, customer will need to remove the network manually before adding new
        // one.
        ASD_LOG_I(wifi_hal, "Profile array is full, can't add any new one");
        return aceWifiHal_ERROR_FAILURE;
#endif
    }

    ASD_LOG_I(wifi_hal, "Found 1st empty profile slot with index: %d", empty_ssid_index);
    return aceWifiHal_setConfig(&(profile_array[empty_ssid_index]), config, priority_new);
}

/* Remove a profile with same SSID from Profile array:
 * 'remove' by setting valid as 0
 */
aceWifiHal_error_t aceWifiHal_removeNetwork(const char* ssid, aceWifiHal_authMode_t authMode) {
    for (int i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        aceWifiHal_RTOS_config_t* profile = &profile_array[i];
        uint8_t ssidLength = profile->mgrConfig.ssidLength;

        if (ssidLength > 0 && (profile->mgrConfig.ssidLength == strlen(ssid)) &&
            strncmp(ssid, profile->mgrConfig.ssid, ssidLength) == 0) {
            // If the authMode is not specified, pick the first one that matches ssid.
            if ((authMode != aceWifiHal_AUTH_MODE_MAX) &&
                (profile->mgrConfig.authMode != authMode)) {
                    continue;
            }
            ASD_LOG_I(wifi_hal, "Found SSID with profile index %d\n", i);

            // If ssid matches the currently connected AP, disconnect
            if (aceWifiHal_getNetworkState() != aceWifiHal_NET_STATE_DISCONNECTED) {
                WIFIConnectionInfo_t info;
                WIFIReturnCode_t retCode;

                retCode = WIFI_GetConnectionInfo(&info);
                if (retCode != eWiFiSuccess) {
                    ASD_LOG_W(wifi_hal, "Get removed network info failed %u", retCode);
                }
                else {
                    if (info.ucSSIDLength == ssidLength &&
                        strncmp((char*)info.ucSSID, ssid, ssidLength) == 0 &&
                        aceWifiHal_convertAuthMode(info.xSecurity) == profile->mgrConfig.authMode) {
                        ASD_LOG_I(wifi_hal, "Disconnect from AP");
#ifdef ACE_WIFI_HAL_AUTO_CONNECT
                        /* When network state is not DISCONNECTED,
                         * sets flag, changes profile status to DISABLED.
                         */
                        disconnectFromRemoveNetwork = true;
                        profile->mgrConfig.status = aceWifiHal_CONFIG_STATUS_DISABLED;
#endif
                        aceWifiHal_disconnect();
                    }
                }
            }

            // Remove the profile
            memset(profile, 0, sizeof(aceWifiHal_RTOS_config_t));
            return aceWifiHal_ERROR_SUCCESS;
        }
    }

    ASD_LOG_E(wifi_hal, "Did not find SSID in profiles");
    return aceWifiHal_ERROR_FAILURE;
}

aceWifiHal_error_t aceWifiHal_enableNetwork(const char* ssid, aceWifiHal_authMode_t type) {
    // TODO
    ASD_LOG_I(ace_hal, "TODO implementation");
    return aceWifiHal_ERROR_FAILURE;
}

aceWifiHal_error_t aceWifiHal_disableNetwork(const char* ssid, aceWifiHal_authMode_t type) {
    // TODO
    ASD_LOG_I(ace_hal, "TODO implementation");
    return aceWifiHal_ERROR_FAILURE;
}

aceWifiHal_error_t aceWifiHal_enableAllNetworks(void) {
    ASD_LOG_I(wifi_hal, "Enabling all networks...");
    for (int i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        if (profile_array[i].mgrConfig.ssidLength != 0 &&
            profile_array[i].mgrConfig.status == aceWifiHal_CONFIG_STATUS_DISABLED) {
            profile_array[i].mgrConfig.status = aceWifiHal_CONFIG_STATUS_ENABLED; /* Set this profile as enabled */
        }
    }
#ifndef ACE_WIFI_ENABLE_MW_AUTO_CONNECT
    return aceWifiHal_autoConnect();
#else
    return aceWifiHal_ERROR_SUCCESS;
#endif
}

aceWifiHal_error_t aceWifiHal_disableAllNetworks(void) {
    ASD_LOG_I(wifi_hal, "Disabling all networks...");
    uint8_t status;

    for (int i = 0; i < ACE_WIFI_MAX_CONFIGURED_NETWORKS; i++) {
        profile_array[i].mgrConfig.status = aceWifiHal_CONFIG_STATUS_DISABLED;
    }

    // If there is AP connected, disconnect
#ifndef ACE_WIFI_ENABLE_MW_AUTO_CONNECT
    aceWifiHal_disconnect();
#endif

    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_enableTcpTunnel(const char* srcIp, uint16_t srcPort,
                                              const char* destIp, uint16_t destPort) {
    //TODO: enable later
    ASD_LOG_I(wifi_hal, "TODO implementation");
    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_disableTcpTunnel(void) {
    //TODO: enable later
    ASD_LOG_I(wifi_hal, "TODO implementation");
    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_enableNAT(void) {
    //TODO: enable later
    ASD_LOG_I(wifi_hal, "TODO implementation");
    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_disableNAT(void) {
    //TODO: enable later
    ASD_LOG_I(wifi_hal, "TODO implementation");
    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_getMacAddress(aceWifiHal_macAddress_t* macAddress) {
    uint8_t address[aceWifiHal_BSSID_LEN];
    WIFIReturnCode_t retCode;

    retCode = WIFI_GetMAC(address);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "WIFI GetMAC failed");
        return aceWifiHal_ERROR_FAILURE;
    }

    sprintf((char*)macAddress, "%02x:%02x:%02x:%02x:%02x:%02x",
            address[0], address[1], address[2], address[3], address[4], address[5]);
    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_setCountryCode(const char* country) {
    WIFIReturnCode_t retCode;

    retCode = WIFI_SetCountryCode(country);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "WIFI SetCountry failed");
        return aceWifiHal_ERROR_FAILURE;
    }
    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_getCountryCode(char* country, uint32_t length) {
    WIFIReturnCode_t retCode;

    /* The buffer should be at least 4 bytes */
    if (length < aceWifiHal_COUNTRY_CODE_LEN) {
        ASD_LOG_E(wifi_hal, "WIFI GetCountry failed: buffer %d bytes", length);
        return aceWifiHal_ERROR_FAILURE;
    }

    memset(country, 0, length);
    retCode = WIFI_GetCountryCode(country);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "WIFI GetCountry failed");
        return aceWifiHal_ERROR_FAILURE;
    }
    return aceWifiHal_ERROR_SUCCESS;
}

static aceWifiHal_PMMode_t aceWifiHal_convertPMModeToAce(WIFIPMMode_t powerMode) {
    switch(powerMode) {
        case eWiFiPMNormal:
            return aceWifiHal_PM_NORMAL;
        case eWiFiPMLowPower:
            return aceWifiHal_PM_LOW;
        case eWiFiPMAlwaysOn:
            return aceWifiHal_PM_ALWAYS_ON;
        default:
            return aceWifiHal_PM_MAX;
    }
}

static WIFIPMMode_t aceWifiHal_convertPMModetoHal(aceWifiHal_bandwidth_t powerMode) {
    switch(powerMode) {
        case aceWifiHal_PM_NORMAL:
            return eWiFiPMNormal;
        case aceWifiHal_PM_LOW:
            return eWiFiPMLowPower;
        case aceWifiHal_PM_ALWAYS_ON:
            return eWiFiPMAlwaysOn;
        default:
            return aceWifiHal_PM_MAX;
    }
}

aceWifiHal_error_t aceWifiHal_setPowerMode(aceWifiHal_PMMode_t powerMode) {
    WIFIReturnCode_t retCode;
    uint32_t ulOptionValue = 0;

    retCode = WIFI_SetPMMode(aceWifiHal_convertPMModetoHal(powerMode),&ulOptionValue);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "WIFI SetPMMode failed: %d", powerMode);
        return aceWifiHal_ERROR_FAILURE;
    }

    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_getPowerMode(aceWifiHal_PMMode_t* powerMode) {
    WIFIReturnCode_t retCode;
    uint32_t ulOptionValue = 0;

    WIFIPMMode_t pmMode = eWiFiPMNotSupported;
    retCode = WIFI_GetPMMode(&pmMode, ( void * ) &ulOptionValue);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "WIFI GetPMMode failed: %d", pmMode);
        return aceWifiHal_ERROR_FAILURE;
    }

    *powerMode = aceWifiHal_convertPMModeToAce(pmMode);

    return aceWifiHal_ERROR_SUCCESS;
}

static aceWifiHal_band_t aceWifiHal_convertAddrType(WIFIIPAddressType_t xAddrType) {
    switch(xAddrType) {
        case eWiFiIPAddressTypeV4:
            return aceWifiHal_IP_TYPE_IPV4;
        case eWiFiIPAddressTypeV6:
            return aceWifiHal_IP_TYPE_IPV6;
        case eWiFiIPAddressTypeNotSupported:
            return aceWifiHal_IP_TYPE_UNSPEC;
        default:
            return aceWifiHal_IP_TYPE_UNSPEC;
    }
}

aceWifiHal_error_t aceWifiHal_getIpInfo(aceWifiHal_ipConfiguration_t* retIpInfo) {

    WIFIIPConfiguration_t ipInfo;
    memset(&ipInfo, 0, sizeof(WIFIIPConfiguration_t));
    WIFIReturnCode_t retCode = WIFI_GetIPInfo(&ipInfo);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "WIFI_GetIPInfo failed");
        return aceWifiHal_ERROR_FAILURE;
    }

    memset(retIpInfo, 0, sizeof(aceWifiHal_ipConfiguration_t));
    retIpInfo->ipAddress.type = aceWifiHal_convertAddrType(ipInfo.xIPAddress.xType);
    memcpy(&(retIpInfo->ipAddress.ipAddress), &(ipInfo.xIPAddress.ulAddress), sizeof(retIpInfo->ipAddress.ipAddress));

    retIpInfo->netMask.type = aceWifiHal_convertAddrType(ipInfo.xNetMask.xType);
    memcpy(&(retIpInfo->netMask.ipAddress), &(ipInfo.xNetMask.ulAddress), sizeof(retIpInfo->netMask.ipAddress));

    retIpInfo->gateway.type = aceWifiHal_convertAddrType(ipInfo.xGateway.xType);
    memcpy(&(retIpInfo->gateway.ipAddress), &(ipInfo.xGateway.ulAddress), sizeof(retIpInfo->gateway.ipAddress));

    retIpInfo->dns1.type = aceWifiHal_convertAddrType(ipInfo.xDns1.xType);
    memcpy(&(retIpInfo->dns1.ipAddress), &(ipInfo.xDns1.ulAddress), sizeof(retIpInfo->dns1.ipAddress));

    retIpInfo->dns2.type = aceWifiHal_convertAddrType(ipInfo.xDns2.xType);
    memcpy(&(retIpInfo->dns2.ipAddress), &(ipInfo.xDns2.ulAddress), sizeof(retIpInfo->dns2.ipAddress));

    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_getCoreWifiStats(aceWifiHal_statistic_t* retStatsInfo) {
    WIFIStatisticInfo_t statsInfo;
    memset(&statsInfo, 0, sizeof(WIFIStatisticInfo_t));
    WIFIReturnCode_t retCode = WIFI_GetStatistic(&statsInfo);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "WIFI_GetGetStatistic failed");
        return aceWifiHal_ERROR_FAILURE;
    }

    retStatsInfo->txSuccessCount = statsInfo.ulTxSuccessCount;
    retStatsInfo->txRetryCount = statsInfo.ulTxRetryCount;
    retStatsInfo->txFailCount = statsInfo.ulTxFailCount;
    retStatsInfo->rxSuccessCount = statsInfo.ulRxSuccessCount;
    retStatsInfo->rxCRCErrorCount = statsInfo.ulRxCRCErrorCount;
    retStatsInfo->MICErrorCount = statsInfo.ulMICErrorCount;
    retStatsInfo->noise = statsInfo.cNoise;
    retStatsInfo->phyRate = statsInfo.usPhyRate;
    retStatsInfo->txRate = statsInfo.usTxRate;
    retStatsInfo->rxRate = statsInfo.usRxRate;
    retStatsInfo->rssi = statsInfo.cRssi;
    retStatsInfo->bandwidth = statsInfo.ucBandwidth;
    retStatsInfo->idleTimePer = statsInfo.ucIdleTimePer;

    if (WIFI_IsConnected()) {
        WIFIConnectionInfo_t info;
        retCode = WIFI_GetConnectionInfo(&info);
        if (retCode == eWiFiSuccess) {
            retStatsInfo->channel = info.ucChannel;
        } else {
            ASD_LOG_E(wifi_hal, "WIFI GetConnectionInfo failed, ret=%d", retCode);
        }
    }

#if 0
    // local debug
    ASD_LOG_I(wifi_hal, "WifiStatistic: txSuccessCount=%lu, txRetryCount=%lu, txFailCount=%lu, rxSuccessCount=%lu, rxCRCErrorCount=%lu, MICErrorCount=%lu, noise=%d, phyRate=%u, txRate=%u, rxRate=%u, rssi=%d, ch=%d, bandwidth=%u, idleTimePer=%u)", retStatsInfo->txSuccessCount, retStatsInfo->txRetryCount, retStatsInfo->txFailCount, retStatsInfo->rxSuccessCount, retStatsInfo->rxCRCErrorCount, retStatsInfo->MICErrorCount, retStatsInfo->noise, retStatsInfo->phyRate, retStatsInfo->txRate, retStatsInfo->rxRate, retStatsInfo->rssi, retStatsInfo->channel, retStatsInfo->bandwidth, retStatsInfo->idleTimePer);
#endif

    return aceWifiHal_ERROR_SUCCESS;
}

static aceWifiHal_band_t aceWifiHal_convertBand(WIFIBand_t xBand) {
    switch(xBand) {
        case eWiFiBand2G:
            return aceWifiHal_BAND_2G;
        case eWiFiBand5G:
            return aceWifiHal_BAND_5G;
        case eWiFiBandDual:
            return aceWifiHal_BAND_DUAL;
        default:
            return aceWifiHal_BAND_MAX;
    }
}

static aceWifiHal_phyMode_t aceWifiHal_convertPhyMode(WIFIPhyMode_t xPhy) {
    switch(xPhy) {
        case eWiFiPhy11b:
            return aceWifiHal_PHY_11B;
        case eWiFiPhy11g:
            return aceWifiHal_PHY_11G;
        case eWiFiPhy11n:
            return aceWifiHal_PHY_11N;
        case eWiFiPhy11ac:
            return aceWifiHal_PHY_11AC;
        case eWiFiPhy11ax:
            return aceWifiHal_PHY_11AX;
        default:
            return aceWifiHal_PHY_MAX;
    }
}

static aceWifiHal_bandwidth_t aceWifiHal_convertBandwidth(WIFIBandwidth_t xBandwidth) {
    switch(xBandwidth) {
        case eWiFiBW20:
            return aceWifiHal_BW_20;
        case eWiFiBW40:
            return aceWifiHal_BW_40;
        case eWiFiBW80:
            return aceWifiHal_BW_80;
        case eWiFiBW160:
            return aceWifiHal_BW_160;
        default:
            return aceWifiHal_AUTH_MODE_MAX;
    }
}

static aceWifiHal_PMMode_t aceWifiHal_convertPMMode(WIFIPMMode_t xPMMode) {
    switch(xPMMode) {
        case eWiFiPMNormal:
            return aceWifiHal_PM_NORMAL;
        case eWiFiPMLowPower:
            return aceWifiHal_PM_LOW;
        case eWiFiPMAlwaysOn:
            return aceWifiHal_PM_ALWAYS_ON;
        default:
            return aceWifiHal_PM_MAX;
    }
}

static uint16_t aceWifiHal_convertSupportedFeatures(uint16_t inSupportedFeatures) {

    uint16_t retSupportedFeatures = 0;

    if (inSupportedFeatures & WIFI_WPS_SUPPORTED) {
        retSupportedFeatures |= aceWifiHal_WPS_SUPPORTED;
    }

    if (inSupportedFeatures & WIFI_ENTERPRISE_SUPPORTED) {
        retSupportedFeatures |= aceWifiHal_ENTERPRISE_SUPPORTED;
    }

    if (inSupportedFeatures & WIFI_P2P_SUPPORTED) {
        retSupportedFeatures |= aceWifiHal_P2P_SUPPORTED;
    }

    if (inSupportedFeatures & WIFI_TDLS_SUPPORTED) {
        retSupportedFeatures |= aceWifiHal_TDLS_SUPPORTED;
    }
    return retSupportedFeatures;
}

aceWifiHal_error_t aceWifiHal_getWifiCapability(aceWifiHal_capability_t* retCapsInfo) {
    WIFICapabilityInfo_t capsInfo;
    memset(&capsInfo, 0, sizeof(WIFICapabilityInfo_t));
    memset(retCapsInfo, 0, sizeof(aceWifiHal_capability_t));
    WIFIReturnCode_t retCode = WIFI_GetCapability(&capsInfo);
    if (retCode != eWiFiSuccess) {
        ASD_LOG_E(wifi_hal, "WIFI_GetCapability failed");
        return aceWifiHal_ERROR_FAILURE;
    }

    retCapsInfo->band = aceWifiHal_convertBand(capsInfo.xBand);
    retCapsInfo->phyMode = aceWifiHal_convertPhyMode(capsInfo.xPhyMode);
    retCapsInfo->bandwidth = aceWifiHal_convertBandwidth(capsInfo.xBandwidth);

    retCapsInfo->maxAggr = capsInfo.ulMaxAggr;
    retCapsInfo->supportedFeatures = aceWifiHal_convertSupportedFeatures(capsInfo.usSupportedFeatures);

#if 0
    // local debug
    ASD_LOG_I(wifi_hal, "WifiCapability: band=%d, phyMode=%d, bandwidth=%d, maxAggr=%lu, supportedFeatures=%d", retCapsInfo->band, retCapsInfo->phyMode, retCapsInfo->bandwidth, retCapsInfo->maxAggr, retCapsInfo->supportedFeatures);
#endif

    return aceWifiHal_ERROR_SUCCESS;
}

aceWifiHal_error_t aceWifiHal_startIpProvisioning(
        aceWifiHal_ipConfiguration_t* ipConfig) {
    (void)ipConfig;
    return aceWifiHal_ERROR_NOT_SUPPORTED;
}

aceWifiHal_error_t aceWifiHal_stopIpProvisioning(void) {
    return aceWifiHal_ERROR_NOT_SUPPORTED;
}

