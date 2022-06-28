/*
 * Copyright 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/* FreeRTOS kernel includes */
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "event_groups.h"

/* Wifi includes*/
#include "wifi_management.h"
#include "wifi_credentials.h"

/* Ace wifi includes */
#include <ace/wifi_mgr.h>

/* Ais demo includes */
#include "reconnection_task.h"
#include "ux_attention_system.h"

#define WIFI_GET_STATE_RETRIES 1000

typedef enum _wifi_event
{
    WIFI_EVENT_SERVICE_READY = (1 << 0U),
    WIFI_EVENT_CONNECTED     = (1 << 1U),
    WIFI_EVENT_DISCONNECTED  = (1 << 2U),
} wifi_event_t;

static EventGroupHandle_t xEventWifiState = NULL;

/**
 * @brief This callback is triggered when the WiFi service has been fully initialized.
 *
 * @param ctx[in] application argument passed during registration
 */
static void network_service_ready_cb(void *ctx)
{
    if (xEventWifiState != NULL)
    {
        xEventGroupSetBits(xEventWifiState, WIFI_EVENT_SERVICE_READY);
    }
    else
    {
        configPRINTF(("[ERROR] xEventWifiState is NULL and the main task cannot be woken up\r\n"));
    }
}

/**
 * @brief This callback is triggered when a wifi connection event occurred.
 *
 * @param netState[in] event type
 *        wifiInfo[in] corresponding wifi connection information
 *        ctx[in]      application argument passed during registration
 */
static void network_event_cb(aceWifiMgr_networkState_t netState, aceWifiMgr_wifiInfo_t wifiInfo, void *ctx)
{
    /* This print is very useful for debugging purposes */
    configPRINTF(("network_event_cb: %d\r\n", netState));

    if (netState == NET_STATE_CONNECTED)
    {
        configPRINTF(("connected to ssid: %s\r\n", wifiInfo.ssid));

        if (xEventWifiState != NULL)
        {
            xEventGroupSetBits(xEventWifiState, WIFI_EVENT_CONNECTED);
        }
    }
    else if (netState == NET_STATE_DISCONNECTED)
    {
        reconnection_task_set_event(kReconnectNetworkLoss);
        if (xEventWifiState != NULL)
        {
            xEventGroupSetBits(xEventWifiState, WIFI_EVENT_DISCONNECTED);
        }
    }
}

wifi_status_t wifi_init(void)
{
    wifi_status_t status = kWiFiStatus_Success;
    aceWifiMgr_error_t status_wifi;
    EventBits_t wifi_conn_bits;

    /* Inform the user that the WiFi setup has started */
    ux_attention_set_state(uxWiFiSetup);

    if (xEventWifiState == NULL)
    {
        xEventWifiState = xEventGroupCreate();
        if (xEventWifiState == NULL)
        {
            configPRINTF(("[ERROR] Could not create EventGroup\r\n"));
            status = kWiFiStatus_Fail;
        }
    }
    else
    {
        xEventGroupClearBits(xEventWifiState,
                             (WIFI_EVENT_SERVICE_READY | WIFI_EVENT_CONNECTED | WIFI_EVENT_DISCONNECTED));
    }

    if (status == kWiFiStatus_Success)
    {
        status_wifi = aceWifiMgr_init();
        if (status_wifi != aceWifiMgr_ERROR_SUCCESS)
        {
            configPRINTF(("[ERROR] aceWifiMgr_init error code: %d\r\n", status_wifi));
            status = kWiFiStatus_Fail;
        }
    }

    if (status == kWiFiStatus_Success)
    {
        /* register a callback for wifi service ready event */
        status_wifi = aceWifiMgr_registerWifiReadyEvent(network_service_ready_cb, NULL);
        if (status_wifi != aceWifiMgr_ERROR_SUCCESS)
        {
            configPRINTF(("[ERROR] aceWifiMgr_registerWifiReadyEvent error code: %d\r\n", status_wifi));
            status = kWiFiStatus_Fail;
        }
    }

    if (status == kWiFiStatus_Success)
    {
        /* There is a chance that the wifi service has been already initialized before we registered
         * the callback. Don't wait for the WIFI_EVENT_SERVICE_READY if the service is already running */
        if (aceWifiMgr_isWifiReady() == 0)
        {
            /* Wait wifi to be fully ready */
            wifi_conn_bits =
                xEventGroupWaitBits(xEventWifiState, WIFI_EVENT_SERVICE_READY, pdTRUE, pdTRUE, portMAX_DELAY);
            if ((wifi_conn_bits & WIFI_EVENT_SERVICE_READY) == 0)
            {
                configPRINTF(("[ERROR] WiFi service start failed\r\n"));
                status = kWiFiStatus_Fail;
            }
        }
    }

    if (status == kWiFiStatus_Success)
    {
        /* register a callback for the network state changes events */
        status_wifi = aceWifiMgr_registerNetworkStateEvent(network_event_cb, NULL);
        if (status_wifi != aceWifiMgr_ERROR_SUCCESS)
        {
            configPRINTF(("[ERROR] aceWifiMgr_registerNetworkStateEvent error code: %d\r\n", status_wifi));
            status = kWiFiStatus_Fail;
        }
    }

    if (status == kWiFiStatus_Success)
    {
        if (wifi_credentials_sync() != kWiFiStatus_Success)
        {
            configPRINTF(("Could not sync WiFi credentials in KVS and Flash_management\r\n"));
        }
    }

    return status;
}

wifi_status_t wifi_connect(bool ux_attention)
{
    wifi_status_t status = kWiFiStatus_Success;
    aceWifiMgr_error_t status_wifi;
    aceWifiMgr_configList_t networks = {0};
    EventBits_t wifi_conn_bits;

    /* Stores the WiFi SSID as a printable string. Added one extra byte for the NULL terminator. */
    char ssid_str[MAX_SSID_LEN + 1] = {0};

    if ((aceWifiMgr_isWifiReady() == 0) || (xEventWifiState == NULL))
    {
        status = kWiFiStatus_Fail;
    }

    if (status == kWiFiStatus_Success)
    {
        /* It is supposed we got here fully disconnected.
         * Reset the event bits for safety.
         */
        xEventGroupClearBits(xEventWifiState, (WIFI_EVENT_CONNECTED | WIFI_EVENT_DISCONNECTED));

        status_wifi = aceWifiMgr_getConfiguredNetworks(&networks);
        if (status_wifi != aceWifiMgr_ERROR_SUCCESS)
        {
            configPRINTF(("[ERROR] aceWifiMgr_getConfiguredNetworks error code: %d\r\n", status_wifi));
            status = kWiFiStatus_Fail;
        }
    }

    if (status == kWiFiStatus_Success)
    {
        if (networks.length == 0)
        {
            configPRINTF(("[ERROR] WiFi profile is missing\r\n"));
            status = kWiFiStatus_MissingProfile;
        }
    }

    if (status == kWiFiStatus_Success)
    {
        /* Connect to wifi.
         * On a network state change, network_event_cb will be triggered
         * Check the event bit to understand the wifi status.
         */
        while (1)
        {
            if (ux_attention == true)
            {
                ux_attention_set_state(uxWiFiSetup);
            }

            memset(ssid_str, 0, MAX_SSID_LEN + 1);
            memcpy(ssid_str, networks.list[0].ssid, networks.list[0].ssidLength);
            status_wifi = aceWifiMgr_connect(ssid_str, networks.list[0].authMode);
            if (status_wifi == aceWifiMgr_ERROR_SUCCESS)
            {
                wifi_conn_bits = xEventGroupWaitBits(xEventWifiState, (WIFI_EVENT_CONNECTED | WIFI_EVENT_DISCONNECTED),
                                                     pdTRUE, pdFALSE, portMAX_DELAY);
                if (wifi_conn_bits & WIFI_EVENT_CONNECTED)
                {
                    if (ux_attention == true)
                    {
                        ux_attention_set_state(uxAccessPointFound);
                    }
                    break;
                }
                else if (wifi_conn_bits & WIFI_EVENT_DISCONNECTED)
                {
                    if (ux_attention == true)
                    {
                        ux_attention_set_state(uxNoAccessPoint);

                        /* Let the user see the error LED's */
                        vTaskDelay(2000);
                    }
                    configPRINTF(("Could not connect to the network, trying again.\r\n"));
                }
                else
                {
                    if (ux_attention == true)
                    {
                        ux_attention_set_state(uxNoAccessPoint);
                    }
                    configPRINTF(("[ERROR] WiFi connection failed\r\n"));
                    status = kWiFiStatus_Fail;
                    break;
                }
            }
            else
            {
                configPRINTF(("[ERROR] aceWifiMgr_connect error code: %d\r\n", status_wifi));
                status = kWiFiStatus_Fail;
                break;
            }
        }
    }

    return status;
}

wifi_status_t wifi_reconnect(void)
{
    wifi_status_t status = kWiFiStatus_Success;
    aceWifiMgr_error_t status_wifi;
    uint16_t retries;

    /* In order to be safe, restart the existing connection. */
    status_wifi = aceWifiMgr_disconnect();
    if (status_wifi == aceWifiMgr_ERROR_SUCCESS)
    {
        for (retries = 0; retries < WIFI_GET_STATE_RETRIES; retries++)
        {
            vTaskDelay(10);
            if (aceWifiMgr_getNetworkState() == NET_STATE_DISCONNECTED)
            {
                break;
            }
        }
        if (aceWifiMgr_getNetworkState() == NET_STATE_DISCONNECTED)
        {
            configPRINTF(("WiFi disconnected\r\n"));
        }
        else
        {
            configPRINTF(("[ERROR] WiFi disconnection failed\r\n"));
            status = kWiFiStatus_Fail;
        }
    }
    else
    {
        configPRINTF(("[ERROR] aceWifiMgr_disconnect error code: %d\r\n", status_wifi));
        status = kWiFiStatus_Fail;
    }

    if (status == kWiFiStatus_Success)
    {
        vTaskDelay(10);
        status = wifi_connect(false);
    }

    return status;
}
