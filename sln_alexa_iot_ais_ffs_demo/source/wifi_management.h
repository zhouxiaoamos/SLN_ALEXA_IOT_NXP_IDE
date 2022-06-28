/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef WIFI_MANAGEMENT_H_
#define WIFI_MANAGEMENT_H_

#include <stdbool.h>

typedef enum _wifi_status
{
    kWiFiStatus_Success,
    kWiFiStatus_Fail,
    kWiFiStatus_MissingProfile,
} wifi_status_t;

/**
 * @brief   Initialize wifi manager and register the callbacks.
 *
 * @return  kWiFiStatus_Success:        Wifi initialized.
 *          kWiFiStatus_Fail:           Wifi not initialized.
 */
wifi_status_t wifi_init(void);

/**
 * @brief   Connect to the first wifi profile available in KVS.
 *          If no error occurs, retry for indefinite times.
 *
 * @param   ux_attention: Specifies if the state should be notified by LED.
 *
 * @return  kWiFiStatus_Success:        Wifi connected
 *          kWiFiStatus_Fail:           Wifi could not connect to network.
 *          kWiFiStatus_MissingProfile: No wifi profile found in KVS.
 */
wifi_status_t wifi_connect(bool ux_attention);

/**
 * @brief   Should be called after a disconnection was registered.
 *          Disconnect from the current network.
 *          Connect to the first wifi profile available in KVS (by calling wifi_connect).
 *
 * @return  kWiFiStatus_Success:        Wifi connected
 *          kWiFiStatus_Fail:           Wifi failed to initialize or could not connect to network.
 *          kWiFiStatus_MissingProfile: No wifi profile found in KVS.
 */
wifi_status_t wifi_reconnect(void);

#endif /* WIFI_MANAGEMENT_H_ */
