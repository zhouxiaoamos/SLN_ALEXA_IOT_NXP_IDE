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
/*******************************************************************************
 * AFW Platform Events
 *
 *******************************************************************************
 */

#ifndef AFW_EVENT_DEFS_H
#define AFW_EVENT_DEFS_H

/* Events associated with the AFW Platform. Application layer events should
 * start at AFW_EVENT_MAX. */
typedef enum {
    AFW_EVENT_TEST = 0,

    AFW_EVENT_BUTTON_0_DOWN,
    AFW_EVENT_BUTTON_0_UP,
    AFW_EVENT_BUTTON_0_SPRESS,
    AFW_EVENT_BUTTON_0_NPRESS,
    AFW_EVENT_BUTTON_0_LPRESS,
    AFW_EVENT_BUTTON_0_XLPRESS,
    AFW_EVENT_BUTTON_0_INACTIVITY,

    AFW_EVENT_BUTTON_1_DOWN,
    AFW_EVENT_BUTTON_1_UP,
    AFW_EVENT_BUTTON_1_SPRESS,
    AFW_EVENT_BUTTON_1_NPRESS,
    AFW_EVENT_BUTTON_1_LPRESS,
    AFW_EVENT_BUTTON_1_XLPRESS,
    AFW_EVENT_BUTTON_1_INACTIVITY,

    AFW_EVENT_LED_OFF,
    AFW_EVENT_LED_BLINK,
    AFW_EVENT_LED_PATTERN,
    AFW_EVENT_LED_CUS_PATTERN,

    AFW_EVENT_BT_STATE_CHANGE, // MSB 16 indicates State, LSB 16 bits indicates any cause

    /* --------------------------------------------------------------------------------
     * Reserving a few positions for the WIFI/Network stack related events
     * --------------------------------------------------------------------------------
     */
    AFW_EVENT_WIFI_READY,
    AFW_EVENT_WIFI_CONNECTED_STA,
    AFW_EVENT_WIFI_CONNECTED_AP,
    AFW_EVENT_WIFI_DISCONNECTED,
    AFW_EVENT_WIFI_SCAN_COMPLETE,
    AFW_EVENT_WIFI_CONNECTED_STA_TIMEOUT,

    /*
     * Events related to provisioning
     */
    AFW_PROV_CFG_CONNECTION_EVENT_ID,
    AFW_PROV_CFG_EVENT_ID,
    AFW_PROV_CFG_TERMINATE_EVENT_ID,

    /*
     * Events related to registration
     */
    AFW_REGISTRATION_CREDS_SET,
    AFW_REGISTRATION_PROV_DONE,
    AFW_REGISTRATION_COMPLETE,
    AFW_REGISTRATION_FAILED,

    /*
     * Events related to server communications (transactions)
     */
    AFW_TRANSACTION_COMPLETE,
    AFW_TRANSACTION_FAILED,

    /*
     * Misc Events
     */
    AFW_EVENT_MORSE_PATTERN_DETECTED,
    AFW_EVENT_HEALTH_CHECK,
    AFW_EVENT_POWER_OFF,
    AFW_EVENT_REBOOT,
    AFW_EVENT_START_BLE_BEACON,

    AFW_PROV_CONN_WIFI,
    AFW_PROV_CONN_BLE,
    AFW_EVENT_SEND_OTA_STATUS,
    AFW_PROV_EVENT_FFS_SHUTDOWN_IND,

    AFW_EVENT_MAX
} afw_event_def_t;

#endif
