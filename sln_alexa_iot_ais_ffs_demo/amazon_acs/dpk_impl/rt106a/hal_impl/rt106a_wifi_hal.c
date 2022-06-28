/*
 * Copyright 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "event_groups.h"

#include "aws_wifi_hal.h"
#include "tcpip_manager.h"

#include "wwd_constants.h"
#include "wwd_events.h"
#include "wwd_management.h"
#include "wwd_network.h"
#include "wwd_structures.h"
#include "wwd_wiced.h"
#include "wwd_wifi.h"

#include "fsl_debug_console.h"


#define STA_DHCP_CLIENT_USE  true

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* TODO: ADJUST THE PRIORITY ACCORDING TO THE APPLICATION USED */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/* The priority of the Notifying task should be higher than
 * the priority of the main task(the task calling the ACE API).
 * For acs_test_runner, the main task priority is (configMAX_PRIORITIES - 10)
 */
#define NOTIFY_TASK_PRIORITY   (configMAX_PRIORITIES - 9)
#define NOTIFY_TASK_STACK_SIZE 1024

#define BIT_TASK_STARTED      (1 << 0)
#define BIT_TASK_STOPPED      (1 << 1)

/* The length of the WEP keys can be 5 or 13 */
#define WEP_KEY_LEN5        5
#define WEP_KEY_LEN13       13
/* 1B index, 1B key length, 13B key */
#define WEP_KEY_LEN         (2 + WEP_KEY_LEN13)
/* There can be wificonfigMAX_WEPKEYS number of keys at the same time */
#define WEP_KEYS_BUFF_LEN   (WEP_KEY_LEN * wificonfigMAX_WEPKEYS)

#define WPA_PASS_LEN        wificonfigMAX_PASSPHRASE_LEN

#define COUNTRY_CODE_LEN    2

typedef enum
{
    eWiFiEventExtLinkLoss = eWiFiEventMax + 1,
    eWiFiEventExtMax
} WIFIEventTypeExt_t;

typedef struct {
    wiced_ssid_t ssid;                  /**< SSID and SSID length */
    union
    {
        uint8_t wpa[WPA_PASS_LEN];      /**< WPA Password */
        uint8_t wep[WEP_KEYS_BUFF_LEN]; /**< WEP Keys */
    } password;
    uint8_t password_len;               /**< Password length */
    uint8_t channel;                    /**< Channel */
    wiced_security_t auth_type;         /**< Auth mode */
} wifi_hal_config_t;

typedef struct {
    WIFIEventHandler_t ready;
    WIFIEventHandler_t scan_done;
    WIFIEventHandler_t connected;
    WIFIEventHandler_t disconnected;
    WIFIEventHandler_t connect_failed;
    WIFIEventHandler_t ap_state_changed;
    WIFIEventHandler_t ap_station_connected;
    WIFIEventHandler_t ap_station_disconnected;
    WIFIEventHandler_t got_ip;
} wifi_cb_t;

typedef struct {
    wiced_scan_result_t *list_head;
    WIFIScanResultExt_t *full_result;
    uint16_t count;
} wifi_hal_scan_result_t;

typedef struct wifi_hal_soft_ap_connections {
    WIFIStationInfo_t mac;
    struct wifi_hal_soft_ap_connections *next;
} wifi_hal_soft_ap_connections_t;

typedef void(connection_update_fn)(bool);


extern wiced_result_t wiced_wlan_connectivity_init(void);
extern wiced_result_t register_link_events(wiced_security_t security, void *callback_fun_addr);

static WIFIReturnCode_t wifi_hal_ConvertWepHexByte(uint8_t x, uint8_t y, uint8_t *result);
static WIFIReturnCode_t wifi_hal_ConvertWepPassword(wifi_hal_config_t *wifi_conf, const WIFINetworkParamsExt_t *pxNetworkParams);
static WIFIReturnCode_t wifi_hal_ConvertHalToWiced(wifi_hal_config_t *wifi_conf, const WIFINetworkParamsExt_t *pxNetworkParams);
static WIFIReturnCode_t wifi_hal_ConvertAuthWicedToHal(WIFISecurity_t *auth_type, wiced_security_t wiced_auth_type);
static void wifi_hal_ConnectUpdateCb(wiced_bool_t event);
static bool wifi_hal_macs_equal(WIFIStationInfo_t mac1, wiced_mac_t mac2);
static bool wifi_hal_SoftApConnFindMac(wiced_mac_t mac);
static WIFIReturnCode_t wifi_hal_SoftApConnAdd(wiced_mac_t mac);
static void wifi_hal_SoftApConnDel(wiced_mac_t mac);
static void wifi_hal_SoftApConnDelAll(void);
static void wifi_hal_SoftApProcessConnEvent(wiced_bool_t event, wwd_event_reason_t reason, wiced_mac_t mac);
static void* wifi_hal_SoftApConnectionsCb(const wwd_event_header_t *event_header, const uint8_t *event_data, void *handler_user_data);
static void wifi_hal_ScanResultCb(wiced_scan_result_t **result_ptr, void *user_data, wiced_scan_status_t status);
static void wifi_hal_NotifyUpperLayerTask(void *parameters);
static WIFIReturnCode_t wifi_hal_InitNotifyTask(void);
static WIFIReturnCode_t wifi_hal_DeinitNotifyTask(void);

volatile static bool s_connectState         = false;
static wifi_cb_t wifi_callbacks;
static wifi_hal_scan_result_t s_scan_result = {0};
static wiced_scan_result_t s_scan_new_value_holder = {0};
static wiced_scan_result_t *s_scan_new_value_holder_p = &s_scan_new_value_holder;

static wifi_hal_soft_ap_connections_t *s_softApConnectionStationList = NULL;
static wiced_security_t      s_softap_security      = WICED_SECURITY_UNKNOWN;
static const wwd_event_num_t s_softap_conn_events[] = {WLC_E_ASSOC_IND, WLC_E_AUTHORIZED, WLC_E_DEAUTH_IND, WLC_E_DISASSOC_IND, WLC_E_NONE};

static TaskHandle_t       xTaskHandle_HalNotifyUp = NULL;
static EventGroupHandle_t xEventSemaphore         = NULL;
static QueueHandle_t      xEventQueue             = NULL;

/* For WEP HEX format keys, this function converts 2 ascii characters ranged from '0'-'9' or 'A'-'F'.
 * These ascii characters should be converted to hex values (example: '5' => 0x05, 'B' => 0x0B)
 * and concatenated to form 1 byte (example x = '5', y = 'B' => result = 0x5B)
 */
static WIFIReturnCode_t wifi_hal_ConvertWepHexByte(uint8_t x, uint8_t y, uint8_t *result)
{
    WIFIReturnCode_t status = eWiFiSuccess;

    if (result == NULL)
    {
        status = eWiFiFailure;
    }

    if (status == eWiFiSuccess)
    {
        if ((x >= '0') && (x <= '9'))
        {
            x = x - '0';
        }
        else if ((x >= 'A') && (x <= 'F'))
        {
            x = x - 'A' + 0x0A;
        }
        else if ((x >= 'a') && (x <= 'f'))
        {
            x = x - 'a' + 0x0A;
        }
        else
        {
            status = eWiFiFailure;
        }
    }

    if (status == eWiFiSuccess)
    {
        if ((y >= '0') && (y <= '9'))
        {
            y = y - '0';
        }
        else if ((y >= 'A') && (y <= 'F'))
        {
            y = y - 'A' + 0x0A;
        }
        else if ((y >= 'a') && (y <= 'f'))
        {
            y = y - 'a' + 0x0A;
        }
        else
        {
            status = eWiFiFailure;
        }
    }

    if (status == eWiFiSuccess)
    {
        *result = ((x << 4) | y);
    }

    return status;
}

/* For WEP security, iterate through the received keys and convert them to the expected format by WICED.
 * WICED one key format: 1B-id, 1B-key_len, 5B/13B-key
 * WICED multiple keys format: key1 key2 key3 ...
 */
static WIFIReturnCode_t wifi_hal_ConvertWepPassword(wifi_hal_config_t *wifi_conf, const WIFINetworkParamsExt_t *pxNetworkParams)
{
    WIFIReturnCode_t status = eWiFiSuccess;
    uint8_t wep_keys_len;
    uint8_t wep_key_byte;
    uint8_t key_id;
    uint8_t key_bit;

    if ((wifi_conf == NULL) || (pxNetworkParams == NULL))
    {
        status = eWiFiFailure;
    }

    if (status == eWiFiSuccess)
    {
        wep_keys_len = 0;

        for (key_id = 0; key_id < wificonfigMAX_WEPKEYS; key_id++)
        {
            switch (pxNetworkParams->xPassword.xWEP[key_id].ucLength)
            {
            case 0:
                /* This key is not set, ignore it */
                break;

            case WEP_KEY_LEN5:
            case WEP_KEY_LEN13:
                /* The key is received in plane ascii text */
                if (wep_keys_len + 2 + pxNetworkParams->xPassword.xWEP[key_id].ucLength <= WEP_KEYS_BUFF_LEN)
                {
                    wifi_conf->password.wep[wep_keys_len++] = key_id;
                    wifi_conf->password.wep[wep_keys_len++] = pxNetworkParams->xPassword.xWEP[key_id].ucLength;
                    memcpy((uint8_t *)&wifi_conf->password.wep[wep_keys_len], (uint8_t *)pxNetworkParams->xPassword.xWEP[key_id].cKey,
                            pxNetworkParams->xPassword.xWEP[key_id].ucLength);
                    wep_keys_len += pxNetworkParams->xPassword.xWEP[key_id].ucLength;
                }
                else
                {
                    status = eWiFiFailure;
                }
                break;

            case (WEP_KEY_LEN5 * 2):
            case (WEP_KEY_LEN13 * 2):
                /* The key is received in hex format */
                if (wep_keys_len + 2 + (pxNetworkParams->xPassword.xWEP[key_id].ucLength / 2) <= WEP_KEYS_BUFF_LEN)
                {
                    wifi_conf->password.wep[wep_keys_len++] = key_id;
                    wifi_conf->password.wep[wep_keys_len++] = pxNetworkParams->xPassword.xWEP[key_id].ucLength / 2;
                    for (key_bit = 0; key_bit < pxNetworkParams->xPassword.xWEP[key_id].ucLength / 2; key_bit++)
                    {
                        status = wifi_hal_ConvertWepHexByte(pxNetworkParams->xPassword.xWEP[key_id].cKey[key_bit * 2],
                                                            pxNetworkParams->xPassword.xWEP[key_id].cKey[key_bit * 2 + 1],
                                                            &wep_key_byte);
                        if (status == eWiFiSuccess)
                        {
                            wifi_conf->password.wep[wep_keys_len++] = wep_key_byte;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else
                {
                    status = eWiFiFailure;
                }
                break;

            default:
                status = eWiFiFailure;
                break;
            }

            if (status != eWiFiSuccess)
            {
                break;
            }
        }
    }

    if (status == eWiFiSuccess)
    {
        if (wep_keys_len > 0)
        {
            wifi_conf->password_len = wep_keys_len;
        }
        else
        {
            status = eWiFiFailure;
        }
    }

    return status;
}

/* Convert the ACE wifi type to WICED type */
static WIFIReturnCode_t wifi_hal_ConvertHalToWiced(wifi_hal_config_t *wifi_conf, const WIFINetworkParamsExt_t *pxNetworkParams)
{
    WIFIReturnCode_t status = eWiFiSuccess;


    if ((wifi_conf == NULL) || (pxNetworkParams == NULL))
    {
        status = eWiFiFailure;
    }

    if (status == eWiFiSuccess)
    {
        if ((pxNetworkParams->ucSSIDLength > 0) &&
            (pxNetworkParams->ucSSIDLength <= wificonfigMAX_SSID_LEN))
        {
            wifi_conf->ssid.length = pxNetworkParams->ucSSIDLength;
            memcpy((uint8_t *)wifi_conf->ssid.value, (uint8_t *)pxNetworkParams->ucSSID, wifi_conf->ssid.length);
        }
        else
        {
            status = eWiFiFailure;
        }
    }

    if (status == eWiFiSuccess)
    {
        switch (pxNetworkParams->xSecurity)
        {
        case eWiFiSecurityOpen:
            wifi_conf->password_len = 0;
            wifi_conf->password.wpa[0] = '\0';
            wifi_conf->auth_type = WICED_SECURITY_OPEN;
            break;

        case eWiFiSecurityWEP:

            status = wifi_hal_ConvertWepPassword(wifi_conf, pxNetworkParams);
            if (status == eWiFiSuccess)
            {
                wifi_conf->auth_type = WICED_SECURITY_WEP_SHARED;
            }

            break;

        case eWiFiSecurityWPA:
            if ((pxNetworkParams->xPassword.xWPA.ucLength > 0) &&
                (pxNetworkParams->xPassword.xWPA.ucLength <= wificonfigMAX_PASSPHRASE_LEN))
            {
                wifi_conf->password_len = pxNetworkParams->xPassword.xWPA.ucLength;
                memcpy((uint8_t *)wifi_conf->password.wpa, (uint8_t *)pxNetworkParams->xPassword.xWPA.cPassphrase, wifi_conf->password_len);
                wifi_conf->auth_type = WICED_SECURITY_WPA_MIXED_PSK;
            }
            else
            {
                status = eWiFiFailure;
            }
            break;

        case eWiFiSecurityWPA2:
            if ((pxNetworkParams->xPassword.xWPA.ucLength > 0) &&
                (pxNetworkParams->xPassword.xWPA.ucLength <= wificonfigMAX_PASSPHRASE_LEN))
            {
                wifi_conf->password_len = pxNetworkParams->xPassword.xWPA.ucLength;
                memcpy((uint8_t *)wifi_conf->password.wpa, (uint8_t *)pxNetworkParams->xPassword.xWPA.cPassphrase, wifi_conf->password_len);
                wifi_conf->auth_type = WICED_SECURITY_WPA2_MIXED_PSK;
            }
            else
            {
                status = eWiFiFailure;
            }
            break;

        default:
            status = eWiFiNotSupported;
            break;
        }
    }

    if (status == eWiFiSuccess)
    {
        wifi_conf->channel = pxNetworkParams->ucChannel;
    }

    return status;
}

/* Convert the authenticate type from WICED type to HAL type */
static WIFIReturnCode_t wifi_hal_ConvertAuthWicedToHal(WIFISecurity_t *auth_type, wiced_security_t wiced_auth_type)
{
    WIFIReturnCode_t status = eWiFiSuccess;

    switch (wiced_auth_type)
    {
    case WICED_SECURITY_OPEN:
        *auth_type = eWiFiSecurityOpen;
        break;

    case WICED_SECURITY_WEP_PSK:
    case WICED_SECURITY_WEP_SHARED:
        *auth_type =  eWiFiSecurityWEP;
        break;

    case WICED_SECURITY_WPA_TKIP_PSK:
    case WICED_SECURITY_WPA_AES_PSK:
    case WICED_SECURITY_WPA_MIXED_PSK:
        *auth_type = eWiFiSecurityWPA;
        break;

    case WICED_SECURITY_WPA2_AES_PSK:
    case WICED_SECURITY_WPA2_TKIP_PSK:
    case WICED_SECURITY_WPA2_MIXED_PSK:
        *auth_type = eWiFiSecurityWPA2;
        break;

    default:
        *auth_type = eWiFiSecurityNotSupported;
        status = eWiFiNotSupported;
        break;
    }

    return status;
}

/* This callback will be registered after a successful connection to an AP
 * and will be triggered when the link of the established connection will go up/down.
 * In case of link change, the upper layer will be notified.
 */
static void wifi_hal_ConnectUpdateCb(wiced_bool_t event)
{
    WIFIEvent_t wifi_event;

    switch (event)
    {
    /* Handle the link loss */
    case WICED_FALSE:
        if (s_connectState != (bool)WICED_FALSE)
        {
            configPRINTF(("wifi_hal_ConnectUpdateCb: WiFi Link lost\r\n"));
            s_connectState = (bool)WICED_FALSE;
            TCPIP_MANAGER_link_sta_down();

            /* Notify the upper layer about the Disconnect Event */
            wifi_event.xEventType = eWiFiEventExtLinkLoss;
            xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
        }
        break;

    /* Handle the link re-establishment (after a previous link loss) */
    case WICED_TRUE:
        if (s_connectState != (bool)WICED_TRUE)
        {
            /* Do nothing, let the main task to properly reconnect */
        }
        break;

    default:
        configPRINTF(("[HAL-ERROR] wifi_hal_ConnectUpdateCb: unknown event\r\n"));
        break;
    }
}

/* Compare 2 MAC entries. */
static bool wifi_hal_macs_equal(WIFIStationInfo_t mac1, wiced_mac_t mac2)
{
    bool equal;
    uint8_t i;

    equal = true;
    for (i = 0; i < wificonfigMAX_BSSID_LEN; i++)
    {
        if (mac1.ucMAC[i] != mac2.octet[i])
        {
            equal = false;
            break;
        }
    }

    return equal;
}

/* Find a specific MAC entry in the SoftAP device connection list. */
static bool wifi_hal_SoftApConnFindMac(wiced_mac_t mac)
{
    bool found;
    wifi_hal_soft_ap_connections_t *current_st;

    found = false;
    current_st = s_softApConnectionStationList;
    while (current_st != NULL)
    {
        found = wifi_hal_macs_equal(current_st->mac, mac);
        if (found == true)
        {
            break;
        }

        current_st = current_st->next;
    }

    return found;
}

/* Add a new entry to the SoftAP device connection list. */
static WIFIReturnCode_t wifi_hal_SoftApConnAdd(wiced_mac_t mac)
{
    WIFIReturnCode_t status = eWiFiSuccess;
    wifi_hal_soft_ap_connections_t *current_st;

    if (s_softApConnectionStationList == NULL)
    {
        s_softApConnectionStationList = (wifi_hal_soft_ap_connections_t*)pvPortMalloc(sizeof(wifi_hal_soft_ap_connections_t));
        if (s_softApConnectionStationList != NULL)
        {
            memcpy(s_softApConnectionStationList->mac.ucMAC, mac.octet, wificonfigMAX_BSSID_LEN);
            s_softApConnectionStationList->next = NULL;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        current_st = s_softApConnectionStationList;
        while (current_st->next != NULL)
        {
            current_st = current_st->next;
        }
        current_st->next = (wifi_hal_soft_ap_connections_t*)pvPortMalloc(sizeof(wifi_hal_soft_ap_connections_t));
        if (current_st->next != NULL)
        {
            current_st = current_st->next;
            memcpy(current_st->mac.ucMAC, mac.octet, wificonfigMAX_BSSID_LEN);
            current_st->next = NULL;
        }
        else
        {
            status = eWiFiFailure;
        }
    }

    return status;
}

/* Delete a specific MAC entry from the SoftAP device connection list.
 * If the MAC is not found, ignore.
 */
static void wifi_hal_SoftApConnDel(wiced_mac_t mac)
{
    wifi_hal_soft_ap_connections_t *prev_st;
    wifi_hal_soft_ap_connections_t *current_st;
    bool found;

    if (s_softApConnectionStationList != NULL)
    {
        current_st = s_softApConnectionStationList;

        found = wifi_hal_macs_equal(current_st->mac, mac);
        if (found == true)
        {
            s_softApConnectionStationList = s_softApConnectionStationList->next;
            vPortFree(current_st);
        }
        else
        {
            prev_st = current_st;
            current_st = current_st->next;
            while (current_st != NULL)
            {
                found = wifi_hal_macs_equal(current_st->mac, mac);
                if (found == true)
                {
                    prev_st->next = current_st->next;
                    vPortFree(current_st);
                    break;
                }

                prev_st = current_st;
                current_st = current_st->next;
            }
        }
    }
}

/* Delete the SoftAP device connection list */
static void wifi_hal_SoftApConnDelAll(void)
{
    wifi_hal_soft_ap_connections_t *current_st;

    while (s_softApConnectionStationList != NULL)
    {
        current_st = s_softApConnectionStationList;
        s_softApConnectionStationList = s_softApConnectionStationList->next;
        vPortFree(current_st);
    }
}

/* While being in SoftAP mode, handle a device connect/disconnect event.
 * If it is a connect event, check the connected devices list and if it is not
 * found notify the upper layer and add it to the list, otherwise ignore the event.
 * If it is a disconnect event, check the connected devices list and if it is
 * found notify the upper layer and delete it from the list, otherwise ignore the event.
 */
static void wifi_hal_SoftApProcessConnEvent(wiced_bool_t event, wwd_event_reason_t reason, wiced_mac_t mac)
{
    WIFIReturnCode_t status = eWiFiSuccess;
    WIFIEvent_t wifi_event;
    bool mac_connected;

    mac_connected = wifi_hal_SoftApConnFindMac(mac);

    /* Handle a device disconnection */
    if (event == WICED_FALSE)
    {
        /* Act only if previously connected */
        if (mac_connected == true)
        {
            configPRINTF(("DISCONNECTED: Reason: %d MAC: %02X %02X %02X %02X %02X %02X\r\n",
                                (uint8_t)reason, mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5]));

            wifi_hal_SoftApConnDel(mac);
            /* Notify the upper layer about the Device Disconnect from the SoftAP */
            wifi_event.xEventType                  = eWiFiEventAPStationDisconnected;
            wifi_event.xInfo.xAPStationDisconnected.xReason = eWiFiReasonUnspecified;
            memcpy(wifi_event.xInfo.xAPStationDisconnected.ucMac, mac.octet, wificonfigMAX_BSSID_LEN);
            xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
        }
    }
    /* Handle a new device connection */
    else if (event == WICED_TRUE)
    {
        /* Act only if not already connected */
        if (mac_connected == false)
        {
            configPRINTF(("CONNECTED: MAC: %02X %02X %02X %02X %02X %02X\r\n",
                                    mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5]));

            status = wifi_hal_SoftApConnAdd(mac);
            if (status != eWiFiSuccess)
            {
                configPRINTF(("[WARNING]wifi_hal_SoftApConnAdd failed\r\n"));
            }

            /* Notify the upper layer about the New Device Connect to the SoftAP */
            wifi_event.xEventType = eWiFiEventAPStationConnected;
            memcpy(wifi_event.xInfo.xAPStationConnected.ucMac, mac.octet, wificonfigMAX_BSSID_LEN);
            xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
        }
    }
}

/* This callback will be registered during a StartAP. The WiFi driver will trigger this
 * callback once a device will connect or disconnect to/from softAP. In case the softAP
 * security is OPEN (does not have a password), the connection will be notified with a
 * WLC_E_ASSOC_IND type event. If the softAP has a password, the attempt to connect
 * will be notified by WLC_E_ASSOC_IND, but the fully connection will be notified by
 * WLC_E_AUTHORIZED event, so in this case ignore the WLC_E_ASSOC_IND event.
 * The disconnection will be notified by WLC_E_DEAUTH_IND and WLC_E_DISASSOC_IND. Keep
 * both disconnection events to forwards the right disconnection reason to the upper layer.
 */
static void* wifi_hal_SoftApConnectionsCb(const wwd_event_header_t *event_header, const uint8_t *event_data, void *handler_user_data)
{
    UNUSED_PARAMETER(event_data);

    if (event_header->interface == (uint8_t)WWD_AP_INTERFACE)
    {
        switch (event_header->event_type)
        {
        case WLC_E_ASSOC_IND:
            if (s_softap_security == WICED_SECURITY_OPEN)
            {
                wifi_hal_SoftApProcessConnEvent(WICED_TRUE, event_header->reason, event_header->addr);
            }
            break;

        case WLC_E_AUTHORIZED:
            wifi_hal_SoftApProcessConnEvent(WICED_TRUE, event_header->reason, event_header->addr);
            break;

        case WLC_E_DEAUTH_IND:
        case WLC_E_DISASSOC_IND:
            wifi_hal_SoftApProcessConnEvent(WICED_FALSE, event_header->reason, event_header->addr);
            break;

        default:
            configPRINTF(("[HAL-ERROR] wifi_hal_SoftApConnectionsCb: unknown event\r\n"));
            break;
        }
    }

    return handler_user_data;
}

/* This callback will be registered during a Scan Start. The WiFi driver will trigger this
 * callback once for every BSSID Network (status = WICED_SCAN_INCOMPLETE) and 1 more time to
 * tell that the scan is completed successfully (status = WICED_SCAN_COMPLETED_SUCCESSFULLY)
 * or the scan was aborted (status = WICED_SCAN_ABORTED).
 * When the Scan is done, the upper layer will be notified.
 */
static void wifi_hal_ScanResultCb(wiced_scan_result_t **result_ptr, void *user_data, wiced_scan_status_t status)
{
    WIFIEvent_t wifi_event;
    wiced_scan_result_t *list_cursor;
    wiced_scan_result_t *scan_result_cursor;
    uint16_t i;

    switch (status)
    {
    /* The partial result about one BSSID network is inside "(*result_ptr[0]).SSID.value".
     * The total number of found networks is not known, so the partial results is
     * stored in a linked list "s_scan_result.list_head".
     * The final result vector "s_scan_result.full_result" will be written during the final
     * scan step and should be valid until a new scan occur. So if a new scan is started,
     * free the final result vector from the previous scan.
     */
    case WICED_SCAN_INCOMPLETE:
        if ((s_scan_result.count == 0) && (s_scan_result.full_result != NULL))
        {
            vPortFree(s_scan_result.full_result);
        }

        if (s_scan_result.list_head == NULL)
        {
            s_scan_result.list_head = (wiced_scan_result_t*)pvPortMalloc(sizeof(wiced_scan_result_t));
            list_cursor = s_scan_result.list_head;
        }
        else
        {
            list_cursor = s_scan_result.list_head;
            while (list_cursor->next != NULL)
            {
                list_cursor = list_cursor->next;
            }
            list_cursor->next = (wiced_scan_result_t*)pvPortMalloc(sizeof(wiced_scan_result_t));
            list_cursor       = list_cursor->next;
        }

        if (list_cursor != NULL)
        {
            s_scan_result.count++;
            memcpy(list_cursor, result_ptr[0], sizeof(wiced_scan_result_t));
            list_cursor->next = NULL;
        }
        break;

    /* All found networks are stored inside the "(*result_ptr[0]).SSID.value".
     * The "s_scan_result.full_result" vector of respective size "s_scan_result.count" is allocated.
     * The link list with partial results is freed.
     */
    case WICED_SCAN_COMPLETED_SUCCESSFULLY:
    case WICED_SCAN_ABORTED:

        if (s_scan_result.count != 0)
        {
            s_scan_result.full_result = (WIFIScanResultExt_t*)pvPortMalloc(s_scan_result.count * sizeof(WIFIScanResultExt_t));
        }

        if (s_scan_result.full_result != NULL)
        {
            for (i = 0; i < s_scan_result.count; i++)
            {
                scan_result_cursor = s_scan_result.list_head;

                s_scan_result.full_result[i].ucSSIDLength = scan_result_cursor->SSID.length;
                strncpy((char *)s_scan_result.full_result[i].ucSSID, (const char *)scan_result_cursor->SSID.value, wificonfigMAX_SSID_LEN);
                memcpy((char *)s_scan_result.full_result[i].ucBSSID, (const char *)scan_result_cursor->BSSID.octet, wificonfigMAX_BSSID_LEN);
                wifi_hal_ConvertAuthWicedToHal(&s_scan_result.full_result[i].xSecurity, scan_result_cursor->security);
                s_scan_result.full_result[i].cRSSI     = (int8_t)scan_result_cursor->signal_strength;
                s_scan_result.full_result[i].ucChannel = (int8_t)scan_result_cursor->channel;

                s_scan_result.list_head = s_scan_result.list_head->next;
                vPortFree(scan_result_cursor);
            }
        }
        else
        {
            for (i = 0; i < s_scan_result.count; i++)
            {
                scan_result_cursor      = s_scan_result.list_head;
                s_scan_result.list_head = s_scan_result.list_head->next;
                vPortFree(scan_result_cursor);
            }
            s_scan_result.count = 0;
        }

        wifi_event.xEventType                       = eWiFiEventScanDone;
        wifi_event.xInfo.xScanDone.usNumScanResults = s_scan_result.count;
        wifi_event.xInfo.xScanDone.pxScanResults    = s_scan_result.full_result;
        xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
        s_scan_result.count = 0;
        break;

    default:
        configPRINTF(("[HAL-ERROR] wifi_hal_ScanResultCb: unknown status\r\n"));
        break;
    }
}



/* HAL Layer Task to notify the upper layer about changes from current or lower layers.
 * All the callbacks registered from upper layers are called from this task.
 */
static void wifi_hal_NotifyUpperLayerTask(void *parameters)
{
    WIFIEvent_t       wifi_event;
    TCPIPReturnCode_t tcpip_status;
    wl_bss_info_t     connected_ap_info;
    wiced_security_t  connected_ap_security;
    wwd_result_t      wwd_status;
    bool              task_job_done = false;

    xEventQueue = xQueueCreate(4, sizeof(WIFIEvent_t));
    /* Notify the WiFi task that this task is initialized */
    xEventGroupSetBits(xEventSemaphore, BIT_TASK_STARTED);

    while (1)
    {
        /* WiFi Task tells to this task to stop, free the resources and wait to be deleted */
        if (task_job_done)
        {
            break;
        }

        if(xQueueReceive(xEventQueue, &wifi_event, portMAX_DELAY) == pdTRUE)
        {
            /* AMAZON PROBLEM?
             * Add this delay because the upper layers do not process correctly
             * the callbacks if they are called to soon.
             */
            vTaskDelay(100);

            switch ((uint8_t)wifi_event.xEventType)
            {
            /* READY callback is called from WIFI_HAL_Init function after it initialized all the components. */
            case eWiFiEventReady:
                /* AMAZON PROBLEM?
                 * The WIFI_HAL_Init function is called before registering the callbacks.
                 * Add this while-delay to give the WiFi task time to register the callbacks.
                 */
                while (1)
                {
                    if (wifi_callbacks.ready)
                    {
                        wifi_callbacks.ready(&wifi_event);
                        break;
                    }
                    vTaskDelay(100);
                }
                break;

            /* SCAN_DONE callback is called from "wifi_hal_ScanResultCb" after a scan is completed.
             * When the "wifi_hal_ScanResultCb" callback receives status equal to
             * WICED_SCAN_COMPLETED_SUCCESSFULLY or WICED_SCAN_COMPLETED_ABORTED, this callback should be called.
             * wifi_event variable should contain the scan result vector.
             */
            case eWiFiEventScanDone:
                if (wifi_callbacks.scan_done)
                {
                    wifi_callbacks.scan_done(&wifi_event);
                }
                break;

            /* Connected callback is called from "WIFI_HAL_StartConnectAP" after a successful connection.
             * Connected callback is called from "wifi_hal_ConnectUpdateCb" after a previously
             * connected link, was lost and now is re-established.
             * Pass to the callback the information about the current connected network.
             */
            case eWiFiEventConnected:
                if (wifi_callbacks.connected)
                {
                    wwd_status = wwd_wifi_get_ap_info(&connected_ap_info, &connected_ap_security);
                    if (wwd_status == WWD_SUCCESS)
                    {
                        wifi_event.xInfo.xConnected.xConnectionInfo.ucSSIDLength = connected_ap_info.SSID_len;
                        strncpy((char *)wifi_event.xInfo.xConnected.xConnectionInfo.ucSSID, (const char *)connected_ap_info.SSID, wificonfigMAX_SSID_LEN);
                        memcpy((char *)wifi_event.xInfo.xConnected.xConnectionInfo.ucBSSID, (const char *)connected_ap_info.BSSID.octet,
                                wificonfigMAX_BSSID_LEN);
                        wifi_event.xInfo.xConnected.xConnectionInfo.ucChannel = connected_ap_info.ctl_ch;
                        wifi_hal_ConvertAuthWicedToHal(&wifi_event.xInfo.xConnected.xConnectionInfo.xSecurity, connected_ap_security);

                        wifi_callbacks.connected(&wifi_event);
                    }
                }
                break;

            /* Disconnected callback is called from "WIFI_HAL_StartDisconnectAP" after a successful disconnection. */
            case eWiFiEventDisconnected:
                if (wifi_callbacks.disconnected)
                {
                    wifi_callbacks.disconnected(&wifi_event);
                }
                break;

            /* ConnectionFailed callback is called from "WIFI_HAL_StartConnectAP" after a failed
             * connection attempt.
             * wifi_event should contain the reason of the failure.
             */
            case eWiFiEventConnectionFailed:
                if (wifi_callbacks.connect_failed)
                {
                    wifi_callbacks.connect_failed(&wifi_event);
                }
                break;

            /* IPReady callback is called from "WIFI_HAL_StartConnectAP" after a successful connection,
             * followed by obtaining the IP address (static or using DHCP client).
             * wifi_event should contain the resulted IP address.
             */
            case eWiFiEventIPReady:
                if (wifi_callbacks.got_ip)
                {
                    tcpip_status = TCPIP_MANAGER_get_ip_only_sta_interface(wifi_event.xInfo.xIPReady.xIPAddress.ulAddress);
                    if (tcpip_status != eTCPIPSuccess)
                    {
                        wifi_event.xInfo.xIPReady.xIPAddress.ulAddress[0] = 0;
                    }

                    wifi_callbacks.got_ip(&wifi_event);
                }
                break;

            /* APStateChanged callback is called from "WIFI_HAL_StartAP" with wifi_event.xInfo.xAPStateChanged.ucState set to 1,
             * after a successful starting of AP mode.
             * APStateChanged callback is called from "WIFI_HAL_StopAP" with wifi_event.xInfo.xAPStateChanged.ucState set to 0,
             * after a successful ending of AP mode.
             */
            case eWiFiEventAPStateChanged:
                if (wifi_callbacks.ap_state_changed)
                {
                    wifi_callbacks.ap_state_changed(&wifi_event);
                }
                break;

            /* APStationConnected callback is called from "wifi_hal_SoftApProcessConnEvent" where
             * wifi_event.xInfo.xAPStationConnected.ucMac stores the MAC of the new connected device.
             */
            case eWiFiEventAPStationConnected:
                if (wifi_callbacks.ap_station_connected)
                {
                    wifi_callbacks.ap_station_connected(&wifi_event);
                }
                break;

            /* APStationDisconnected callback is called from "wifi_hal_SoftApProcessConnEvent" where
             * wifi_event.xInfo.xAPStationConnected.ucMac stores the MAC of the disconnected device.
             */
            case eWiFiEventAPStationDisconnected:
                if (wifi_callbacks.ap_station_disconnected)
                {
                    wifi_callbacks.ap_station_disconnected(&wifi_event);
                }
                break;

            /* eWiFiEventMax notifies the current task that it should stop working.
             * It is notified by the WiFi task from the "WIFI_HAL_DeInit" function.
             */
            case eWiFiEventMax:
                task_job_done = true;
                break;

            /* eWiFiEventExtLinkLoss callback is called from "wifi_hal_ConnectUpdateCb" after a previously
             * connected link is lost. Assure a proper disconnection which will trigger a eWiFiEventDisconnected event
             * which will notify the upper layer about the disconnection.
             */
            case eWiFiEventExtLinkLoss:
                WIFI_HAL_StartDisconnectAP();
                break;

            default:
                break;
            }
        }
    }

    vQueueDelete(xEventQueue);
    /* Notify the WiFi task that this task freed the resources and is ready to be deleted */
    xEventGroupSetBits(xEventSemaphore, BIT_TASK_STOPPED);
    vTaskSuspend(NULL);
}

/* Initialize the semaphore so the WiFi and HAL task can communicate.
 * Create the HAL task and wait until it is fully initialized.
 */
static WIFIReturnCode_t wifi_hal_InitNotifyTask(void)
{
    WIFIReturnCode_t status;
    BaseType_t task_status;

    if (xEventSemaphore == NULL)
    {
        xEventSemaphore = xEventGroupCreate();
        if (xEventSemaphore != NULL)
        {
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiSuccess;
    }

    if (status == eWiFiSuccess)
    {
        if (xTaskHandle_HalNotifyUp == NULL)
        {
            task_status = xTaskCreate(wifi_hal_NotifyUpperLayerTask,
                                      "wifi_hal_NotifyUpperLayerTask",
                                      NOTIFY_TASK_STACK_SIZE,
                                      (void *)NULL,
                                      NOTIFY_TASK_PRIORITY,
                                      &xTaskHandle_HalNotifyUp);
            if (task_status != pdTRUE)
            {
                status = eWiFiFailure;
            }
        }
    }

    if (status == eWiFiSuccess)
    {
        xEventGroupWaitBits(xEventSemaphore, BIT_TASK_STARTED, pdTRUE, pdFALSE, portMAX_DELAY);
    }

    return status;
}

/* Notify the HAL task that it is time to quit. Wait until the HAL task
 * frees its resources and delete it. Free the semaphore because it is not needed anymore.
 */
static WIFIReturnCode_t wifi_hal_DeinitNotifyTask(void)
{
    WIFIReturnCode_t status;
    WIFIEvent_t wifi_event;

    wifi_event.xEventType = eWiFiEventMax;
    xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
    xEventGroupWaitBits(xEventSemaphore, BIT_TASK_STOPPED, pdTRUE, pdFALSE, portMAX_DELAY);

    vTaskDelete(xTaskHandle_HalNotifyUp);
    xTaskHandle_HalNotifyUp = NULL;
    vEventGroupDelete(xEventSemaphore);
    xEventSemaphore = NULL;
    status = eWiFiSuccess;

    return status;
}



/* -------------------------------------------------------- ------------------------- */
/* -------------------------   AWS WIFI HAL API functions   ------------------------- */
/* -------------------------------------------------------- ------------------------- */

WIFIReturnCode_t WIFI_HAL_Init(void)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    TCPIPReturnCode_t tcpip_status;
    WIFIEvent_t wifi_event;

    /* Will be ignored if already ON */
    tcpip_status = TCPIP_MANAGER_init();
    if (tcpip_status == eTCPIPSuccess)
    {
        wwd_status = wiced_wlan_connectivity_init();
        if (wwd_status == WWD_SUCCESS)
        {
            status = wifi_hal_InitNotifyTask();
            if (status == eWiFiSuccess)
            {
                wifi_event.xEventType = eWiFiEventReady;
                xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
            }
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_DeInit(void)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    TCPIPReturnCode_t tcpip_status;

    /* MAYBE EXPLICIT DISCONNECTION NEEDED */

    /* Will be ignored if already OFF */
    wwd_status = wwd_management_wifi_off();
    if (wwd_status == WWD_SUCCESS)
    {
        tcpip_status = TCPIP_MANAGER_quit_sta_interface();
        if (tcpip_status == eTCPIPSuccess)
        {
            tcpip_status = TCPIP_MANAGER_quit_ap_interface();
            if (tcpip_status == eTCPIPSuccess)
            {
                status = wifi_hal_DeinitNotifyTask();
            }
            else
            {
                status = eWiFiFailure;
            }
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_SetRadio(uint8_t on)
{
    /* NEEDS TO BE IMPLEMENTED */
    return eWiFiNotSupported;
}

WIFIReturnCode_t WIFI_HAL_SetMode(WIFIDeviceModeExt_t xDeviceMode)
{
    WIFIReturnCode_t status;

    /* NEEDS TO BE IMPLEMENTED */

    switch (xDeviceMode)
    {
    case eWiFiModeStationExt:
        status = eWiFiNotSupported;
        break;

    case eWiFiModeAPExt:
        status = eWiFiNotSupported;
        break;

    default:
        status = eWiFiNotSupported;
        break;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_GetMode(WIFIDeviceModeExt_t *xDeviceMode)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;

    if (xDeviceMode != NULL)
    {
        /* Check if Station mode */
        wwd_status = wwd_wifi_is_ready_to_transceive(WWD_STA_INTERFACE);
        if (wwd_status == WWD_SUCCESS)
        {
            *xDeviceMode = eWiFiModeStationExt;
            status = eWiFiSuccess;
        }
        else
        {
            /* If not station mode, Check if AP mode */
            wwd_status = wwd_wifi_is_ready_to_transceive(WWD_AP_INTERFACE);
            if (wwd_status == WWD_SUCCESS)
            {
                *xDeviceMode = eWiFiModeAPExt;
                status = eWiFiSuccess;
            }
            else
            {
                status = eWiFiFailure;
            }
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_GetMacAddress(uint8_t *pucMac)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    wiced_mac_t mac;

    if (pucMac != NULL)
    {
        wwd_status = wwd_wifi_get_mac_address(&mac, WWD_STA_INTERFACE);
        if (wwd_status == WWD_SUCCESS)
        {
            memcpy(pucMac, mac.octet, sizeof(mac.octet));
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_SetMacAddress(uint8_t *pucMac)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    wiced_mac_t mac;

    if (pucMac != NULL)
    {
        memcpy(mac.octet, pucMac, sizeof(mac.octet));
        wwd_status = wwd_wifi_set_mac_address(mac);
        if (wwd_status == WWD_SUCCESS)
        {
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_GetIPInfo(WIFIIPConfiguration_t *pxIPInfo)
{
    WIFIReturnCode_t status;
    TCPIPReturnCode_t tcpip_status;
    tcpip_manager_ip_info_t tcpip_info;

    if (pxIPInfo != NULL)
    {
        tcpip_status = TCPIP_MANAGER_get_ip_sta_interface(&tcpip_info);
        if (tcpip_status == eTCPIPSuccess)
        {
            /* Copy IP, Netmask and Gateway */
            pxIPInfo->xIPAddress.xType        = eWiFiIPAddressTypeV4;
            pxIPInfo->xIPAddress.ulAddress[0] = tcpip_info.ip.addr;
            pxIPInfo->xNetMask.xType          = eWiFiIPAddressTypeV4;
            pxIPInfo->xNetMask.ulAddress[0]   = tcpip_info.netmask.addr;
            pxIPInfo->xGateway.xType          = eWiFiIPAddressTypeV4;
            pxIPInfo->xGateway.ulAddress[0]   = tcpip_info.gw.addr;

            /* NEEDS TO BE IMPLEMENTED */

            /* DNS1 and DNS2 */
            pxIPInfo->xDns1.xType             = eWiFiIPAddressTypeV4;
            pxIPInfo->xDns1.ulAddress[0]      = 0;
            pxIPInfo->xDns2.xType             = eWiFiIPAddressTypeV4;
            pxIPInfo->xDns2.ulAddress[0]      = 0;

            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_StartScan(WIFIScanConfig_t *pxScanConfig)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
#if 0 /* NXP TODO: Do a parameterized scan */
    wiced_ssid_t scan_ssid;
    uint16_t scan_chan[2];
#endif

    if ((pxScanConfig == NULL) || (pxScanConfig->ucBroadcast))
    {
        /* Do a generic broadcast scan */
        wwd_status = wwd_wifi_scan(WICED_SCAN_TYPE_ACTIVE, WICED_BSS_TYPE_ANY, NULL, NULL, NULL, NULL,
                (wiced_scan_result_callback_t)wifi_hal_ScanResultCb, &s_scan_new_value_holder_p, NULL, WWD_STA_INTERFACE);
        if (wwd_status == WWD_SUCCESS)
        {
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
#if 0 /* NXP TODO: Do a parameterized scan */
        scan_ssid.length = pxScanConfig->ucSSIDLength;
        strncpy((char *)scan_ssid.value, (char *)pxScanConfig->ucSSID, wificonfigMAX_SSID_LEN);
        scan_chan[0] = pxScanConfig->ucChannel;
        scan_chan[1] = 0;

        wwd_status = wwd_wifi_scan(WICED_SCAN_TYPE_ACTIVE, WICED_BSS_TYPE_ANY, &scan_ssid, NULL, scan_chan, NULL,
                (wiced_scan_result_callback_t)wifi_hal_ScanResultCb, &s_scan_new_value_holder_p, NULL, WWD_STA_INTERFACE);
        if (wwd_status == WWD_SUCCESS)
        {
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
#else
        status = eWiFiNotSupported;
#endif
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_StopScan(void)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;

    wwd_status = wwd_wifi_abort_scan();
    if (wwd_status == WWD_SUCCESS)
    {
        status = eWiFiSuccess;
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_StartConnectAP(const WIFINetworkParamsExt_t *pxNetworkParams)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    TCPIPReturnCode_t tcpip_status;
    wifi_hal_config_t wifi_conf = {0};
    WIFIEvent_t wifi_event;

    /* Parameters check */
    if (pxNetworkParams != NULL)
    {
        if (pxNetworkParams->ucSSIDLength != 0)
        {
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    /* Translate the HAL parameters type to the WICED type */
    if (status == eWiFiSuccess)
    {
        status = wifi_hal_ConvertHalToWiced(&wifi_conf, pxNetworkParams);
    }

    /* Try connect to the specified WiFi
     * In case of successful connection and successful callback registration,
     * notify the upper layer using eWiFiEventConnected.
     * In case of failed connection, notify the upper layer using eWiFiEventConnectionFailed specifying the reason.
     */
    if (status == eWiFiSuccess)
    {
        if (wifi_conf.auth_type == WICED_SECURITY_WEP_SHARED)
        {
            wwd_status = wwd_wifi_join(&wifi_conf.ssid, wifi_conf.auth_type, wifi_conf.password.wep, wifi_conf.password_len,
                                       NULL, WWD_STA_INTERFACE);
            if (wwd_status != WWD_SUCCESS)
            {
                configPRINTF(("Connect WEP_SHARED failed, trying WEP_PSK\r\n"));
                vTaskDelay(100);

                wifi_conf.auth_type = WICED_SECURITY_WEP_PSK;
                wwd_status = wwd_wifi_join(&wifi_conf.ssid, wifi_conf.auth_type, wifi_conf.password.wep, wifi_conf.password_len,
                                           NULL, WWD_STA_INTERFACE);
            }
        }
        else
        {
            wwd_status = wwd_wifi_join(&wifi_conf.ssid, wifi_conf.auth_type, wifi_conf.password.wpa, wifi_conf.password_len,
                                       NULL, WWD_STA_INTERFACE);
        }
    }

    /* In case of successful connection, register the link state callback to be notified about the link state changes.
     * If the registration fails, disconnect the wifi.
     */
    if ((status == eWiFiSuccess) && (wwd_status == WWD_SUCCESS))
    {
        wwd_status = register_link_events(wifi_conf.auth_type, wifi_hal_ConnectUpdateCb);
        if (wwd_status != WWD_SUCCESS)
        {
            wwd_wifi_leave(WWD_STA_INTERFACE);
            s_connectState = (bool)WICED_FALSE;
            status = eWiFiFailure;
        }
    }

    /* Notify the upper layer about success or failure */
    if ((status == eWiFiSuccess) && (wwd_status == WWD_SUCCESS))
    {
        s_connectState = (bool)WICED_TRUE;

        wifi_event.xEventType = eWiFiEventConnected;
        xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
    }
    else
    {
        s_connectState = (bool)WICED_FALSE;

        wifi_event.xEventType = eWiFiEventConnectionFailed;
        wifi_event.xInfo.xConnectionFailed.xReason = eWiFiReasonAuthFailed;
        xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
    }

    /* In case of successful connection, start the station interface and
     * set the static IP or a obtain a dynamic IP using DHCP client
     */
    if ((status == eWiFiSuccess) && (wwd_status == WWD_SUCCESS))
    {
        tcpip_status = TCPIP_MANAGER_start_sta_interface(STA_DHCP_CLIENT_USE, &s_connectState);
        if (tcpip_status == eTCPIPSuccess)
        {
            wifi_event.xEventType = eWiFiEventIPReady;
            xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
        }
        else
        {
            /* Let WICED to set s_connectState to WICED_FALSE if the link loss was the reason */
            vTaskDelay(100);

            /* If TCPIP_MANAGER_start_sta_interface failed because the WiFi link was lost,
             * s_connectState == WICED_FALSE and wifi_hal_ConnectUpdateCb will trigger the disconnect process.
             * Otherwise, TCPIP_MANAGER_start_sta_interface failed because of other reasons and the WiFi is still up,
             * so we have to start here a disconnection process.
             */
            if (s_connectState == (bool)WICED_TRUE)
            {
                s_connectState = (bool)WICED_FALSE;
                WIFI_HAL_StartDisconnectAP();
            }
            status = eWiFiFailure;
        }
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_StartDisconnectAP(void)
{
    WIFIReturnCode_t status = eWiFiSuccess;
    wwd_result_t wwd_status;
    TCPIPReturnCode_t tcpip_status;
    WIFIEvent_t wifi_event;

    /* Shut down the station interface and quit the DHCP client */
    tcpip_status = TCPIP_MANAGER_quit_sta_interface();
    if (tcpip_status != eTCPIPSuccess)
    {
        status = eWiFiFailure;
    }

    /* Unregister the link state callback */
    wwd_status = register_link_events(WICED_SECURITY_UNKNOWN, NULL);
    if (wwd_status != WWD_SUCCESS)
    {
        status = eWiFiFailure;
    }

    /* Disconnect from the network. If not connected to any, the leave function will also return WWD_SUCCESS */
    wwd_status = wwd_wifi_leave(WWD_STA_INTERFACE);
    if (wwd_status == WWD_SUCCESS)
    {
        s_connectState = (bool)WICED_FALSE;
        wifi_event.xEventType = eWiFiEventDisconnected;
        xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

BaseType_t WIFI_HAL_IsConnected(const WIFINetworkParamsExt_t *pxNetworkParams)
{
    BaseType_t status;

    if (pxNetworkParams == NULL)
    {
        if (s_connectState == (bool)WICED_TRUE)
        {
            status = pdTRUE;
        }
        else
        {
            status = pdFALSE;
        }
    }
    else
    {
        /* NEEDS TO BE IMPLEMENTED */
        /* NEED TO DO A SPECIFIC CHECK */
        if (s_connectState == (bool)WICED_TRUE)
        {
            status = pdTRUE;
        }
        else
        {
            status = pdFALSE;
        }
    }
    return status;
}

WIFIReturnCode_t WIFI_HAL_GetConnectionInfo(WIFIConnectionInfo_t *pxConnectionInfo)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    wl_bss_info_t    connected_ap_info;
    wiced_security_t connected_ap_security;

    if (pxConnectionInfo != NULL)
    {
        wwd_status = wwd_wifi_get_ap_info(&connected_ap_info, &connected_ap_security);
        if (wwd_status == WWD_SUCCESS)
        {
            pxConnectionInfo->ucSSIDLength = connected_ap_info.SSID_len;
            strncpy((char *)pxConnectionInfo->ucSSID, (const char *)connected_ap_info.SSID, wificonfigMAX_SSID_LEN);
            memcpy((char *)pxConnectionInfo->ucBSSID, (const char *)connected_ap_info.BSSID.octet,
                    wificonfigMAX_BSSID_LEN);
            pxConnectionInfo->ucChannel = connected_ap_info.ctl_ch;
            wifi_hal_ConvertAuthWicedToHal(&pxConnectionInfo->xSecurity, connected_ap_security);

            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_GetRSSI(int8_t *pcRSSI)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    int32_t rssi;

    if (pcRSSI != NULL)
    {
        wwd_status = wwd_wifi_get_rssi(&rssi);
        if (wwd_status == WWD_SUCCESS)
        {
            *pcRSSI = (int8_t)rssi;
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_StartAP(const WIFINetworkParamsExt_t *pxNetworkParams)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    TCPIPReturnCode_t tcpip_status;
    wifi_hal_config_t wifi_conf = {0};
    WIFIEvent_t wifi_event;

    /* Parameters check */
    if (pxNetworkParams != NULL)
    {
        if (pxNetworkParams->ucSSIDLength != 0)
        {
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    /* Translate the HAL parameters type to the WICED type */
    if (status == eWiFiSuccess)
    {
        status = wifi_hal_ConvertHalToWiced(&wifi_conf, pxNetworkParams);
    }

    /* Try to start as AP station using the credentials provided by the upper layer
     * In case of successful start, notify the upper layer using eWiFiEventAPStateChanged
     * and ucState set to 1 to specify the current AP state(ON = 1, OFF = 0).
     */
    if (status == eWiFiSuccess)
    {
        /* WICED_SECURITY_WEP_SHARED is not supported by WICED for AP mode */
        if (wifi_conf.auth_type != WICED_SECURITY_WEP_SHARED)
        {
            wwd_status = wwd_wifi_start_ap(&wifi_conf.ssid, wifi_conf.auth_type, wifi_conf.password.wpa, wifi_conf.password_len, wifi_conf.channel);
            if (wwd_status == WWD_SUCCESS)
            {
                wifi_event.xEventType = eWiFiEventAPStateChanged;
                wifi_event.xInfo.xAPStateChanged.ucState = 1;
                xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
            }
            else
            {
                status = eWiFiFailure;
            }
        }
        else
        {
            status = eWiFiNotSupported;
        }
    }

    /* Register a callback to be notified when a device connects/disconnects to/from SoftAP. */
    if (status == eWiFiSuccess)
    {
        s_softap_security = wifi_conf.auth_type;
        wwd_status = wwd_management_set_event_handler(s_softap_conn_events, wifi_hal_SoftApConnectionsCb, NULL, WWD_AP_INTERFACE);
        if (wwd_status != WWD_SUCCESS)
        {
            wwd_wifi_stop_ap();
            status = eWiFiFailure;
        }
    }

    /* In case of successful AP start, start the AP interface,
     * set the static IP and start the DHCP server
     */
    if (status == eWiFiSuccess)
    {
        tcpip_status = TCPIP_MANAGER_start_ap_interface();
        if (tcpip_status != eTCPIPSuccess)
        {
            status = eWiFiFailure;
        }
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_StopAP(void)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    TCPIPReturnCode_t tcpip_status;
    WIFIEvent_t wifi_event;

    /* Unregister the SoftAP device connect/disconnect callback. */
    wwd_status = wwd_management_set_event_handler(s_softap_conn_events, NULL, NULL, WWD_AP_INTERFACE);
    if (wwd_status == WWD_SUCCESS)
    {
        status = eWiFiSuccess;
    }
    else
    {
        status = eWiFiFailure;
    }

    /* Disconnect all connected devices */
    if (status == eWiFiSuccess)
    {
        wwd_status = wwd_wifi_deauth_all_associated_client_stas(WWD_DOT11_RC_UNSPECIFIED, WWD_AP_INTERFACE);
        if (wwd_status != WWD_SUCCESS)
        {
            status = eWiFiFailure;
        }
    }

    /* Try to stop as AP station mode.
     * In case of successful stop, notify the upper layer using eWiFiEventAPStateChanged
     * and ucState set to 0 to specify the current AP state(ON = 1, OFF = 0).
     */
    if (status == eWiFiSuccess)
    {
        wwd_status = wwd_wifi_stop_ap();
        if (wwd_status == WWD_SUCCESS)
        {
            wifi_hal_SoftApConnDelAll();

            wifi_event.xEventType = eWiFiEventAPStateChanged;
            wifi_event.xInfo.xAPStateChanged.ucState = 0;
            xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
        }
        else
        {
            status = eWiFiFailure;
        }
    }

    /* Shut down the AP interface and quit the DHCP server */
    if (status == eWiFiSuccess)
    {
        tcpip_status = TCPIP_MANAGER_quit_ap_interface();
        if (tcpip_status != eTCPIPSuccess)
        {
            status = eWiFiFailure;
        }
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_GetStationList(WIFIStationInfo_t *pxStationList, uint8_t *puStationListSize)
{
    WIFIReturnCode_t status;
    //wwd_result_t wwd_status;

    /* NEEDS TO BE IMPLEMENTED */

    /* NOT SURE HOW TO USE THIS */
    /*wwd_status = wwd_wifi_get_associated_client_list();
    if (wwd_status == WWD_SUCCESS)
    {
        status = eWiFiSuccess;
    }
    else
    {
        status = eWiFiFailure;
    }*/

    status = eWiFiNotSupported;

    return status;
}

WIFIReturnCode_t WIFI_HAL_DisconnectStation(uint8_t *pucMac)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    WIFIEvent_t wifi_event;

    if (pucMac != NULL)
    {
        wwd_status = wwd_wifi_deauth_sta((const wiced_mac_t*)pucMac, WWD_DOT11_RC_UNSPECIFIED, WWD_AP_INTERFACE);
        if (wwd_status == WWD_SUCCESS)
        {
            wifi_event.xEventType = eWiFiEventAPStationDisconnected;
            memcpy(wifi_event.xInfo.xAPStationDisconnected.ucMac, pucMac, sizeof(wifi_event.xInfo.xAPStationDisconnected.ucMac));
            wifi_event.xInfo.xAPStationDisconnected.xReason = eWiFiReasonUnspecified;
            xQueueSendToBack(xEventQueue, &wifi_event, portMAX_DELAY);
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_RegisterEvent(WIFIEventType_t xEventType, WIFIEventHandler_t xHandler)
{
    WIFIReturnCode_t status = eWiFiSuccess;

    switch (xEventType)
    {
    case eWiFiEventReady:
        wifi_callbacks.ready = xHandler;
        break;

    case eWiFiEventScanDone:
        wifi_callbacks.scan_done = xHandler;
        break;

    case eWiFiEventConnected:
        wifi_callbacks.connected = xHandler;
        break;

    case eWiFiEventDisconnected:
        wifi_callbacks.disconnected = xHandler;
        break;

    case eWiFiEventConnectionFailed:
        wifi_callbacks.connect_failed = xHandler;
        break;

    case eWiFiEventIPReady:
        wifi_callbacks.got_ip = xHandler;
        break;

    case eWiFiEventAPStateChanged:
        wifi_callbacks.ap_state_changed = xHandler;
        break;

    case eWiFiEventAPStationConnected:
        wifi_callbacks.ap_station_connected = xHandler;
        break;

    case eWiFiEventAPStationDisconnected:
        wifi_callbacks.ap_station_disconnected = xHandler;
        break;

    default:
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_SetCountryCode(const char *pcCountryCode)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;
    wwd_country_t ccode = {0};

    if (pcCountryCode != NULL)
    {
        memcpy(ccode, pcCountryCode, COUNTRY_CODE_LEN);

        wwd_status = wwd_wifi_set_ccode((wwd_country_t *)ccode);
        if (wwd_status == WWD_SUCCESS)
        {
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_GetCountryCode(char *pcCountryCode)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;

    if (pcCountryCode != NULL)
    {
        wwd_status = wwd_wifi_get_ccode((wwd_country_t *)pcCountryCode);
        if (wwd_status == WWD_SUCCESS)
        {
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_GetStatistic(WIFIStatisticInfo_t *pxStats)
{
    WIFIReturnCode_t status                = eWiFiSuccess;
    wwd_result_t wwd_status                = WWD_SUCCESS;
    wiced_counters_t wifi_stats            = {0};
    wl_bss_info_t    connected_ap_info     = {0};
    wiced_security_t connected_ap_security = {0};

    if (pxStats == NULL)
    {
        status = eWiFiFailure;
    }

    if (status == eWiFiSuccess)
    {
        wwd_status = wwd_get_counters(&wifi_stats);
        if (wwd_status != WWD_SUCCESS)
        {
            status = eWiFiFailure;
        }
    }

    if (status == eWiFiSuccess)
    {
        wwd_status = wwd_wifi_get_ap_info(&connected_ap_info, &connected_ap_security);
        if (wwd_status != WWD_SUCCESS)
        {
            status = eWiFiFailure;
        }
    }

    if (status == eWiFiSuccess)
    {
        memset(pxStats, 0, sizeof(WIFIStatisticInfo_t));

        pxStats->ulTxSuccessCount  = wifi_stats.txfrag;
        pxStats->ulTxRetryCount    = wifi_stats.txretry;
        pxStats->ulTxFailCount     = wifi_stats.txfail;

        pxStats->ulRxSuccessCount  = wifi_stats.rxfrag;
        pxStats->ulRxCRCErrorCount = wifi_stats.rxcrc;
        pxStats->ulMICErrorCount   = wifi_stats.tkipmicfaill;

        pxStats->cNoise            = connected_ap_info.phy_noise;
        pxStats->usPhyRate         = 0;
        pxStats->usTxRate          = 0;
        pxStats->usRxRate          = 0;
        pxStats->cRssi             = (int8_t)connected_ap_info.RSSI;
        pxStats->ucBandwidth       = 0;
        pxStats->ucIdleTimePer     = 0;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_GetCapability(WIFICapabilityInfo_t *pxCaps)
{
    WIFIReturnCode_t status = eWiFiSuccess;

    if (pxCaps == NULL)
    {
        status = eWiFiFailure;
    }

    if (status == eWiFiSuccess)
    {
        pxCaps->xBand               = eWiFiBand2G;
        pxCaps->xPhyMode            = eWiFiPhy11b;
        pxCaps->xBandwidth          = eWiFiBW20;
        pxCaps->ulMaxAggr           = 0;
        pxCaps->usSupportedFeatures = 0;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_SetPMMode(WIFIPMMode_t xPMModeType)
{
    WIFIReturnCode_t status;
    wwd_result_t wwd_status;

    switch (xPMModeType)
    {
    case eWiFiPMNormal:
        wwd_status = wwd_wifi_disable_powersave();
        if (wwd_status == WWD_SUCCESS)
        {
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
        break;

    case eWiFiPMLowPower:
        wwd_status = wwd_wifi_enable_powersave();
        if (wwd_status == WWD_SUCCESS)
        {
            status = eWiFiSuccess;
        }
        else
        {
            status = eWiFiFailure;
        }
        break;

    default:
        status = eWiFiFailure;
        break;
    }

    return status;
}

WIFIReturnCode_t WIFI_HAL_GetPMMode(WIFIPMMode_t *pxPMModeType)
{
    WIFIReturnCode_t status;
    uint8_t power_save_mode;

    if (pxPMModeType != NULL)
    {
        power_save_mode = wiced_wifi_get_powersave_mode();
        switch (power_save_mode)
        {
        case NO_POWERSAVE_MODE:
            *pxPMModeType = eWiFiPMNormal;
            status = eWiFiSuccess;
            break;

        case PM1_POWERSAVE_MODE:
        case PM2_POWERSAVE_MODE:
            *pxPMModeType = eWiFiPMLowPower;
            status = eWiFiSuccess;
            break;

        default:
            *pxPMModeType = eWiFiPMNotSupported;
            status = eWiFiFailure;
            break;
        }
    }
    else
    {
        status = eWiFiFailure;
    }

    return status;
}
