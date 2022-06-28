/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All rights reserved.
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

#ifndef EVENTMGR_EVENTS_H
#define EVENTMGR_EVENTS_H

/**
 * @file  eventmgr_events.h
 * @brief Supported events by EventMgr
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup EVENTMGR_EVENTS
 * @{
 */

/**
 * @brief MAP event group
 * @details Group id for module id: ACE_MODULE_MAPLITE "MAP events"
 */
#define ACE_EVENTMGR_GROUP_MAP (1)

/**
 * @brief Device register event
 * @details Event id for module id: ACE_MODULE_MAPLITE, group id: ACE_EVENTMGR_GROUP_MAP
 */
#define ACE_EVENTMGR_MAP_EVENT_REGISTER (0x1)

/**
 * @brief Device deregister event
 * @details Event id for module id: ACE_MODULE_MAPLITE, group id: ACE_EVENTMGR_GROUP_MAP
 */
#define ACE_EVENTMGR_MAP_EVENT_DEREGISTER (0x2)

/**
 * @brief Access token updated event
 * @details Event id for module id: ACE_MODULE_MAPLITE, group id: ACE_EVENTMGR_GROUP_MAP
 */
#define ACE_EVENTMGR_MAP_EVENT_ACCESS_TOKEN_UPDATED (0x4)

/**
 * @brief Device name changed event
 * @details Event id for module id: ACE_MODULE_MAPLITE, group id: ACE_EVENTMGR_GROUP_MAP
 */
#define ACE_EVENTMGR_MAP_EVENT_DEVICE_NAME_UPDATED (0x8)

/**
 * @brief Invalid device event
 * @details Event id for module id: ACE_MODULE_MAPLITE, group id: ACE_EVENTMGR_GROUP_MAP
 */
#define ACE_EVENTMGR_MAP_EVENT_INVALID_DEVICE (0x10)

/**
 * @brief Group Id of the test events group
 * @details Group id for module id: ACE_MODULE_EVENTMGR "For testing purpose only"
 */
#define ACE_EVENTMGR_GROUP_TEST (1)

/**
 * @brief Test event 01
 * @details Event id for module id: ACE_MODULE_EVENTMGR, group id: ACE_EVENTMGR_GROUP_TEST
 */
#define ACE_EVENTMGR_EVENT_TEST_01 (0x1)

/**
 * @brief Test event 02
 * @details Event id for module id: ACE_MODULE_EVENTMGR, group id: ACE_EVENTMGR_GROUP_TEST
 */
#define ACE_EVENTMGR_EVENT_TEST_02 (0x2)

/**
 * @brief Test event 03
 * @details Event id for module id: ACE_MODULE_EVENTMGR, group id: ACE_EVENTMGR_GROUP_TEST
 */
#define ACE_EVENTMGR_EVENT_TEST_03 (0x4)

/**
 * @brief Group Id of the smoke test events group
 * @details Group id for module id: ACE_MODULE_EVENTMGR "For testing purpose only"
 */
#define ACE_EVENTMGR_GROUP_SMOKE_TEST (2)

/**
 * @brief Smoke test 01
 * @details Event id for module id: ACE_MODULE_EVENTMGR, group id: ACE_EVENTMGR_GROUP_SMOKE_TEST
 */
#define ACE_EVENTMGR_EVENT_ST_01 (0x1)

/**
 * @brief Smoke test 02
 * @details Event id for module id: ACE_MODULE_EVENTMGR, group id: ACE_EVENTMGR_GROUP_SMOKE_TEST
 */
#define ACE_EVENTMGR_EVENT_ST_02 (0x2)

/**
 * @brief Smoke test 03
 * @details Event id for module id: ACE_MODULE_EVENTMGR, group id: ACE_EVENTMGR_GROUP_SMOKE_TEST
 */
#define ACE_EVENTMGR_EVENT_ST_03 (0x4)

/**
 * @brief Group of the general events such as wifi ready
 * @details Group id for module id: ACE_MODULE_WIFI "WiFi events"
 */
#define ACE_EVENTMGR_GROUP_WIFI_GENERAL (1)

/**
 * @brief WiFi has done initialization
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_GENERAL
 */
#define ACE_EVENTMGR_EVENT_WIFI_READY (0x1)

/**
 * @brief Scan was done
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_GENERAL
 */
#define ACE_EVENTMGR_EVENT_SCAN_DONE (0x2)

/**
 * @brief Describes the current wifi state
 * @details Group id for module id: ACE_MODULE_WIFI "WiFi events"
 */
#define ACE_EVENTMGR_GROUP_WIFI_STATE (2)

/**
 * @brief WiFi is connecting
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_STATE
 */
#define ACE_EVENTMGR_EVENT_WIFI_STATE_CONNECTING (0x1)

/**
 * @brief WiFi is connected
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_STATE
 */
#define ACE_EVENTMGR_EVENT_WIFI_STATE_CONNECTED (0x2)

/**
 * @brief WiFi is disconnecting
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_STATE
 */
#define ACE_EVENTMGR_EVENT_WIFI_STATE_DISCONNECTING (0x4)

/**
 * @brief WiFi is disconnected
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_STATE
 */
#define ACE_EVENTMGR_EVENT_WIFI_STATE_DISCONNECTED (0x8)

/**
 * @brief WiFi state is unknown
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_STATE
 */
#define ACE_EVENTMGR_EVENT_WIFI_STATE_UNKNOWN (0x10)

/**
 * @brief Describes the current state of the captive portal
 * @details Group id for module id: ACE_MODULE_WIFI "WiFi events"
 */
#define ACE_EVENTMGR_GROUP_CAPTIVE_PORTAL_STATE (3)

/**
 * @brief The device has authenticated with captive portal
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_CAPTIVE_PORTAL_STATE
 */
#define ACE_EVENTMGR_EVENT_CAPTIVE_PORTAL_AUTHENTICATED (0x1)

/**
 * @brief The device is connected to a captive portal AP
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_CAPTIVE_PORTAL_STATE
 */
#define ACE_EVENTMGR_EVENT_CAPTIVE_PORTAL_CONNECTED (0x2)

/**
 * @brief The device has failed to check captive portal
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_CAPTIVE_PORTAL_STATE
 */
#define ACE_EVENTMGR_EVENT_CAPTIVE_PORTAL_FAILED (0x4)

/**
 * @brief Describes the current softap state
 * @details Group id for module id: ACE_MODULE_WIFI "WiFi events"
 */
#define ACE_EVENTMGR_GROUP_WIFI_SOFTAP_STATE (4)

/**
 * @brief Soft AP created
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_SOFTAP_STATE
 */
#define ACE_EVENTMGR_EVENT_SOFTAP_UP (0x1)

/**
 * @brief Soft AP removed
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_SOFTAP_STATE
 */
#define ACE_EVENTMGR_EVENT_SOFTAP_DOWN (0x2)

/**
 * @brief A device has connected to the soft AP
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_SOFTAP_STATE
 */
#define ACE_EVENTMGR_EVENT_SOFTAP_CONNECTED (0x4)

/**
 * @brief A device has disconnected to the soft AP
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_SOFTAP_STATE
 */
#define ACE_EVENTMGR_EVENT_SOFTAP_DISCONNECTED (0x8)

/**
 * @brief Describes the profile updated events
 * @details Group id for module id: ACE_MODULE_WIFI "WiFi events"
 */
#define ACE_EVENTMGR_GROUP_WIFI_PROFILE_UPDATED (5)

/**
 * @brief Profile was added
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_PROFILE_UPDATED
 */
#define ACE_EVENTMGR_EVENT_PROFILE_ADDED (0x1)

/**
 * @brief Profile was deleted
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_PROFILE_UPDATED
 */
#define ACE_EVENTMGR_EVENT_PROFILE_DELETED (0x2)

/**
 * @brief Profile was changed
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_PROFILE_UPDATED
 */
#define ACE_EVENTMGR_EVENT_PROFILE_CHANGED (0x4)

/**
 * @brief Describes the wifi ffs events
 * @details Group id for module id: ACE_MODULE_WIFI "WiFi events"
 */
#define ACE_EVENTMGR_GROUP_WIFI_FFS (6)

/**
 * @brief Ffs provisionee found
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_FFS
 */
#define ACE_EVENTMGR_EVENT_PROVISIONEE_FOUND (0x1)

/**
 * @brief Ffs provisionee connected
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_FFS
 */
#define ACE_EVENTMGR_EVENT_PROVISIONEE_CONNECTED (0x2)

/**
 * @brief Ffs provisionee disconnected
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_FFS
 */
#define ACE_EVENTMGR_EVENT_PROVISIONEE_DISCONNECTED (0x4)

/**
 * @brief Ffs provisionee removed
 * @details Event id for module id: ACE_MODULE_WIFI, group id: ACE_EVENTMGR_GROUP_WIFI_FFS
 */
#define ACE_EVENTMGR_EVENT_PROVISIONEE_REMOVED (0x8)

#define ACE_EVENTMGR_MODULE_LIST \
    { \
        ACE_MODULE_MAPLITE, \
        ACE_MODULE_EVENTMGR, \
        ACE_MODULE_WIFI, \
    }

#define ACE_EVENTMGR_NUM_MODULES (3)

/** @} */
#ifdef __cplusplus
}
#endif

#endif  // EVENTMGR_EVENTS_H
