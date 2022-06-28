/*
 * Amazon FreeRTOS
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */


/**
 * @file aws_wifi_ext.h
 * @brief Wi-Fi Interface extensions.
 */

#ifndef _AWS_WIFI_EXT_H_
#define _AWS_WIFI_EXT_H_

#include "iot_wifi.h"

/**
 * @brief Wifi lower level supported feature mask.
 */
#define WIFI_WPS_SUPPORTED           0x0001
#define WIFI_ENTERPRISE_SUPPORTED    0x0002
#define WIFI_P2P_SUPPORTED           0x0004
#define WIFI_TDLS_SUPPORTED          0x0008

#ifndef wificonfigMAX_TARGETED_SCAN_NUM
#define wificonfigMAX_TARGETED_SCAN_NUM 1
#endif

#ifndef wificonfigMAX_SCAN_CHANNEL_NUM
#define wificonfigMAX_SCAN_CHANNEL_NUM 5 // Put 5 here as default, if more than
                                         // that, might as well do full channel scan.
#endif

/**
 * @brief Wi-Fi device modes extension.
 *
 * Wi-Fi Device mode supported.
 *
 * @note This is an extension of WIFI_DeviceMode_t. We cannot change the published
 * WIFI_DeviceMode_t, so we have to define a new list here. It will be combined with
 * the original WIFI_DeviceMode_t in a future revision of iot_wifi.h.
 */
typedef enum
{
    eWiFiModeStationExt,        /**< Station mode. */
    eWiFiModeAPExt,             /**< SoftAP mode. */
    eWiFiModeP2PExt,            /**< P2P mode. */
    eWiFiModeAPStationExt,      /**< AP+Station (repeater) mode. */
    eWiFiModeNotSupportedExt,   /**< Unsupported mode. */
} WIFIDeviceModeExt_t;

/**
 * @brief Wi-Fi WEP keys (64- and 128-bit keys only)
 */
typedef struct
{
    char cKey[ wificonfigMAX_WEPKEY_LEN ];  /**< WEP key. */
    uint8_t ucLength;                       /**< Key length. */
} WIFIWEPKey_t;

/**
 * @brief Wi-Fi WPA/WPA2 passphrase
 */
typedef struct
{
    char cPassphrase[ wificonfigMAX_PASSPHRASE_LEN ];   /**< WPA passphrase (char or hex). */
    uint8_t ucLength;                                   /**< Passphrase length. */
} WIFIWPAPassphrase_t;

/**
 * @brief Parameters passed to the WIFI_ConnectAP API for connection.
 *
 * @note This is an extension of WIFINetworkParams_t. For now we cannot change the
 * published WIFINetworkParams_t, so we have to define a new struct here. It will
 * be combined with the original WIFINetworkParams_t in a future revision of iot_wifi.h.
 */
typedef struct
{
    uint8_t ucSSID[ wificonfigMAX_SSID_LEN ];       /**< SSID of the Wi-Fi network. */
    uint8_t ucSSIDLength;                           /**< SSID length. */
    uint8_t ucBSSID[ wificonfigMAX_BSSID_LEN ];     /**< BSSID of the Wi-Fi network. */
    WIFISecurity_t xSecurity;                       /**< Wi-Fi Security. */
    union
    {
        WIFIWEPKey_t xWEP[ wificonfigMAX_WEPKEYS ]; /**< WEP keys. */
        WIFIWPAPassphrase_t xWPA;                   /**< WPA/WPA2 passphrase. */
    } xPassword;
    uint8_t ucWEPKeyIndex;                          /**< Default WEP key index. */
    uint8_t ucChannel;                              /**< Channel number. */
} WIFINetworkParamsExt_t;

/**
 * @brief Wi-Fi scan item.
 */
typedef struct
{
    uint8_t ucSSID[wificonfigMAX_SSID_LEN]; /**< SSID for targeted scan. */
    uint8_t ucSSIDLength;                   /**< SSID length */
} WIFIScanItem_t;

/**
 * @brief Wi-Fi scan request configuration.
 * For broadcast only, set broadcast to be 1, usTargetedScanItemNum to be 0.
 */
typedef struct
{
    uint8_t ucBroadcast;                 /**< include broadcast scan */
    uint8_t ucTargetedScanItemNum;        /**< Number of targeted scan items */
    WIFIScanItem_t xScanList[wificonfigMAX_TARGETED_SCAN_NUM];  /**< targeted scan item list */
    uint8_t ucChannelNum;                 /**< Number of channels to be scanned. O means full scan */
    uint8_t ucChannel[wificonfigMAX_SCAN_CHANNEL_NUM];    /**< channels to be scanned */
} WIFIScanConfig_t;

/**
 * @brief Wi-Fi scan results extension.
 *
 * Structure to store the Wi-Fi scan results.
 *
 * @note This is an extension of WIFIScanResult_t. For now we cannot change the
 * published WIFIScanResult_t, so we have to define new struct here. It will be
 * combined with the original WIFIScanResult_t in the next iot_wifi.h revision.
 */
typedef struct
{
    uint8_t ucSSID[ wificonfigMAX_SSID_LEN ];   /**< SSID of the Wi-Fi network. */
    uint8_t ucSSIDLength;                       /**< SSID length. */
    uint8_t ucBSSID[ wificonfigMAX_BSSID_LEN ]; /**< BSSID of the Wi-Fi network. */
    WIFISecurity_t xSecurity;                   /**< Wi-Fi Security. */
    int8_t cRSSI;                               /**< Signal strength. */
    uint8_t ucChannel;                          /**< Channel info. */
} WIFIScanResultExt_t;

/**
 * @brief Wi-Fi info of the connected AP.
 *
 * Structure to store info of the connected AP.
 */
typedef struct
{
    uint8_t ucSSID[ wificonfigMAX_SSID_LEN ];   /**< SSID of the Wi-Fi network. */
    uint8_t ucSSIDLength;                       /**< SSID length. */
    uint8_t ucBSSID[ wificonfigMAX_BSSID_LEN ]; /**< BSSID of the Wi-Fi network. */
    WIFISecurity_t xSecurity;                   /**< Wi-Fi Security. */
    uint8_t ucChannel;                          /**< Channel info. */
} WIFIConnectionInfo_t;

/**
* @brief Wi-Fi station IP address type.
*/
typedef enum
{
    eWiFiIPAddressTypeV4,
    eWiFiIPAddressTypeV6,
    eWiFiIPAddressTypeNotSupported,
} WIFIIPAddressType_t;

/**
* @brief Wi-Fi station IP address format.
*/
typedef struct
{
    WIFIIPAddressType_t xType;  /**< IP address type */
    uint32_t ulAddress[4];      /**< IP address in binary form, use inet_ntop/inet_pton for conversion */
} WIFIIPAddress_t;

/**
 * @brief IP address configuration.
 */
typedef struct
{
    WIFIIPAddress_t xIPAddress;   /**< IP address */
    WIFIIPAddress_t xNetMask;     /**< Network mask */
    WIFIIPAddress_t xGateway;     /**< Gateway IP address */
    WIFIIPAddress_t xDns1;        /**< First DNS server IP address */
    WIFIIPAddress_t xDns2;        /**< Second DNS server IP address */
} WIFIIPConfiguration_t;

/**
 * @brief Ping results.
 *
 * Structure to store the results of an ICMP ping operation.
 */
typedef struct
{
    uint16_t usSuccessCount;    /**< Number of good ping replies */
    uint16_t usDroppedCount;    /**< Number of out-of-sequence ping replies */
    uint16_t usTimeoutCount;    /**< Number of timeouts */
    uint32_t ulMinTimeMS;       /**< Minimum ping reply time in milliseconds */
    uint32_t ulMaxTimeMS;       /**< Maximum ping reply time in milliseconds */
    uint32_t ulAvgTimeMS;       /**< Average ping reply time in milliseconds */
} WIFIPingResults_t;

/**
 * @brief Wi-Fi SoftAP connected station info.
 */
typedef struct
{
    uint8_t ucMAC[ wificonfigMAX_BSSID_LEN ];   /**< MAC address of Wi-Fi station */
} WIFIStationInfo_t;

/**
 * @brief Wi-Fi Statistic info.
 */
typedef struct
{
    uint32_t ulTxSuccessCount;   /**< Number of TX successes, 0 if unavailable */
    uint32_t ulTxRetryCount;     /**< Number of TX retries, 0 if unavailable */
    uint32_t ulTxFailCount;      /**< Number of TX failures, 0 if unavailable */
    uint32_t ulRxSuccessCount;   /**< Number of RX successes, 0 if unavailable */
    uint32_t ulRxCRCErrorCount;  /**< Number of RX FCS errors, 0 if unavailable */
    uint32_t ulMICErrorCount;    /**< Number of MIC errors, 0 if unavailable */
    int8_t cNoise;               /**< Average noise level in dBm, -128 if unavailable */
    uint16_t usPhyRate;          /**< Maximum used PHY rate, 0 if unavailable */
    uint16_t usTxRate;           /**< Average used TX rate, 0 if unavailable */
    uint16_t usRxRate;           /**< Average used RX rate, 0 if unavailable */
    int8_t cRssi;                /**< Average beacon RSSI in dBm, -128 if unavailable */
    uint8_t ucBandwidth;         /**< Average used bandwidth, 0 if unavailable */
    uint8_t ucIdleTimePer;       /**< Percent of idle time, 0 if unavailable */
} WIFIStatisticInfo_t;

/**
 * @brief Wi-Fi band.
 */
typedef enum {
    eWiFiBand2G = 0,    /**< 2.4G band */
    eWiFiBand5G,        /**< 5G band */
    eWiFiBandDual,      /**< Dual band */
    eWiFiBandMax        /**< Unsupported */
} WIFIBand_t;

/**
 * @brief Wi-Fi PHY mode.
 */
typedef enum {
    eWiFiPhy11b = 0,    /**< 11B */
    eWiFiPhy11g,        /**< 11G */
    eWiFiPhy11n,        /**< 11N */
    eWiFiPhy11ac,       /**< 11AC */
    eWiFiPhy11ax,       /**< 11AX */
    eWiFiPhyMax,        /**< Unsupported */
} WIFIPhyMode_t;

/**
 * @brief Wi-Fi bandwidth.
 */
typedef enum {
    eWiFiBW20 = 0,      /**< 20MHz only */
    eWiFiBW40,          /**< Max 40MHz */
    eWiFiBW80,          /**< Max 80MHz */
    eWiFiBW160,         /**< Max 80+80 or 160MHz */
    eWiFiBWMax          /**< Unsupported */
} WIFIBandwidth_t;

typedef struct
{
    WIFIBand_t xBand;             /**< Supported band */
    WIFIPhyMode_t xPhyMode;       /**< Supported PHY mode */
    WIFIBandwidth_t xBandwidth;   /**< Supported bandwidth */
    uint32_t ulMaxAggr;           /**< Max aggregation length, 0 if no aggregation support */
    uint16_t usSupportedFeatures; /**< Supported features bitmap, e.g., WIFI_WPS_SUPPORTED */
} WIFICapabilityInfo_t;


/**
 * @brief Initialize the WiFi subsystem.
 *
 * @return eWiFiSuccess if Wi-Fi was initialized successfully, failure code otherwise.
 *
 * @note This function is implicitly called in WIFI_On(), as defined in iot_wifi.h.
 */
WIFIReturnCode_t WIFI_Init( void );

/**
 * @brief Sets the Wi-Fi mode.
 *
 * @param[in] xDeviceMode - Mode of the device.
 *
 * @return eWiFiSuccess if Wi-Fi mode was set successfully, failure code otherwise.
 *
 * @note This is an extension of WIFI_SetMode(). We cannot change the published API
 * now, so we have to define a new function here. It will be combined with the original
 * WIFI_SetMode() in a future revision of iot_wifi.h.
 */
WIFIReturnCode_t WIFI_SetModeExt( WIFIDeviceModeExt_t xDeviceMode );

/**
 * @brief Get the Wi-Fi mode.
 *
 * @param[out] pxDeviceMode - Mode of the device.
 *
 * @return eWiFiSuccess if Wi-Fi mode was successfully retrieved, failure code otherwise.
 *
 * @note This is an extension of WIFI_GetMode(). We cannot change the published API
 * now, so we have to define a new function here. It will be combined with the original
 * WIFI_GetMode() in a future revision of iot_wifi.h.
 */
WIFIReturnCode_t WIFI_GetModeExt( WIFIDeviceModeExt_t * pxDeviceMode );

/**
 * @brief Start a Wi-Fi scan.
 *
 * This is an asynchronous call, the result will be notified by an event.
 *
 * @param[in] pxScanConfig - Parameters for scan, NULL if default scan
 * (i.e. broadcast scan on all channels).
 *
 * @return eWiFiSuccess if scan was started successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_StartScan( WIFIScanConfig_t * pxScanConfig );

/**
 * @brief Get Wi-Fi scan results. Scan results is sorted in rssi order.
 *
 * @param[out] pxBuffer - Handle to the READ ONLY scan results buffer.
 * @param[out] ucNumNetworks - Actual number of networks in the scan results.
 *
 * @return eWiFiSuccess if the scan results were got successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_GetScanResults( const WIFIScanResultExt_t ** pxBuffer,
                                      uint16_t * ucNumNetworks );

/**
 * @brief Connect to the Wi-Fi Access Point (AP) specified in the input.
 *
 * This is an asynchronous call, the result will be notified by an event.
 *
 * The Wi-Fi should stay connected when the specified AP is the same as the
 * currently connected AP. Otherwise, the Wi-Fi should disconnect and connect
 * to the specified AP. If the specified AP has invalid parameters, the Wi-Fi
 * should be disconnected.
 *
 * @param[in] pxNetworkParams - Configuration of the targeted AP.
 *
 * @return eWiFiSuccess if connection was started successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_StartConnectAP( const WIFINetworkParamsExt_t * pxNetworkParams );

/**
 * @brief Wi-Fi station disconnects from AP.
 *
 * This is an asynchronous call. The result will be notified by an event.
 *
 * @return eWiFiSuccess if disconnection was started successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_StartDisconnect( void );

/**
 * @brief Check if the Wi-Fi is connected and the AP configuration matches the query.
 *
 * param[in] pxNetworkParams - Network parameters to query, if NULL then just check the
 * Wi-Fi link status.
 *
 * @return pdTRUE if Wi-Fi is connected, pdFalse otherwise.

 */
BaseType_t WIFI_IsConnectedExt( const WIFINetworkParamsExt_t * pxNetworkParams );

/**
 * @brief Get Wi-Fi info of the connected AP.
 *
 * This is a synchronous call.
 *
 * @param[out] pxConnectionInfo - Wi-Fi info of the connected AP.
 *
 * @return eWiFiSuccess if connection info was got successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_GetConnectionInfo( WIFIConnectionInfo_t * pxConnectionInfo );

/**
 * @brief Get IP configuration (IP address, NetworkMask, Gateway and
 *        DNS server addresses).
 *
 * This is a synchronous call.
 *
 * @param[out] pxIPInfo - Current IP configuration.
 *
 * @return eWiFiSuccess if connection info was got successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_GetIPInfo( WIFIIPConfiguration_t * pxIPInfo );

/**
 * @brief Ping an IP address and get results.
 *
 * @param[in] pucIPAddr - Ping target IP address (binary array, not string).
 * @param[in] usCount - Ping request count.
 * @param[in] usLength - Ping data length (max 1400, IP/ICMP headers excluded).
 * @param[in] ulIntervalMS - Ping interval in milliseconds.
 * @param[out] xResults - Structure to store ping results.
 *
 * @return eWiFiSuccess if there were no socket or memory errors, failure code otherwise.
 */
WIFIReturnCode_t WIFI_PingExt( uint8_t * pucIPAddr,
                               uint16_t usCount,
                               uint16_t usLength,
                               uint32_t ulIntervalMS,
                               WIFIPingResults_t * pxResults );

/**
 * @brief Get the RSSI of the connected AP.
 *
 * This only works when the station is connected.
 *
 * @param[out] pcRSSI - RSSI of the connected AP.
 *
 * @return eWiFiSuccess if RSSI was got successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_GetRSSI( int8_t * pcRSSI );

/**
 * @brief Configure SoftAP.
 *
 * @param[in] pxNetworkParams - Network parameters to configure AP.
 *
 * @return eWiFiSuccess if SoftAP was successfully configured, failure code otherwise.
 *
 * @note This is an extension of WIFI_ConfigureAP(). We cannot change the published API
 * now, so we have to define a new function here. It will be combined with the original
 * WIFI_ConfigureAP() in a future revision of iot_wifi.h.
 */
WIFIReturnCode_t WIFI_ConfigureAPExt( WIFINetworkParamsExt_t * pxNetworkParams );

/**
 * @brief SoftAP mode get connected station list.
 *
 * @param[out] pxStationList - Buffer for station list.
 * @param[out] pcStationListSize - Number of connected stations.
 *
 * @return eWiFiSuccess if stations were got successfully (manybe none),
 * failure code otherwise.
 */
WIFIReturnCode_t WIFI_GetStationList( WIFIStationInfo_t* pxStationList,
                                      uint8_t * pcStationListSize );

/**
 * @brief AP mode disconnecting a station.
 *
 * This is an asynchronous call, the result will be notified by an event.
 *
 * @param[in] pucMac - MAC Address of the station to be disconnected.
 *
 * @return eWiFiSuccess if disconnection was started successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_StartDisconnectStation( uint8_t* pucMac );

/**
 * @brief Set Wi-Fi MAC addresses.
 *
 * The given MAC address will become the station MAC address. The AP MAC address
 * (i.e. BSSID) will be the same MAC address but with the local bit set.
 *
 * @param[in] pucMac - Station MAC address.
 *
 * @return eWiFiSuccess if MAC address was set successfully, failure code otherwise.
 *
 * @note On some platforms the change of MAC address can only take effect after reboot.
 */
WIFIReturnCode_t WIFI_SetMAC( uint8_t * pucMac );

/**
 * @brief Set country based configuration (including channel list, power table)
 *
 * @param[in] pcCountryCode - Country code string (e.g. "US", "CN").
 *
 * @return eWiFiSuccess if Country Code is set successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_SetCountryCode( const char* pcCountryCode );

/**
 * @brief Get the currently configured country code.
 *
 * @param[out] pcCountryCode - String to hold the country code, must be at least 4 bytes.
 *
 * @return eWiFiSuccess if Country Code is retrieved successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_GetCountryCode( char * pcCountryCode );

/**
 * @brief Get the WiFi statistics.
 *
 * @param[out] pxStats - Structure to hold the WiFi statistics.
 * @return eWiFiSuccess if Country Code is retrieved successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_GetStatistic( WIFIStatisticInfo_t * pxStats );

/**
 * @brief Get the WiFi capability.
 *
 * @param[out] pxCaps - Structure to hold the WiFi capabilities.
 * @return eWiFiSuccess if Country Code is retrieved successfully, failure code otherwise.
 */
WIFIReturnCode_t WIFI_GetCapability( WIFICapabilityInfo_t * pxCaps);

/**
 * @brief Wi-Fi protocol reason codes.
 */
typedef enum
{
    eWiFiReasonUnspecified,             /**< Unspecified error */
    eWiFiReasonAPNotFound,              /**< Cannot find the target AP. */
    eWiFiReasonAuthExpired,             /**< Previous auth invalid. */
    eWiFiReasonAuthLeaveBSS,            /**< Deauth leaving BSS. */
    eWiFiReasonAuthFailed,              /**< All other AUTH errors. */
    eWiFiReasonAssocExpired,            /**< Disassoc due to inactivity. */
    eWiFiReasonAssocTooMany,            /**< AP is overloaded. */
    eWiFiReasonAssocPowerCapBad,        /**< Power capability unacceptable. */
    eWiFiReasonAssocSupChanBad,         /**< Supported channel unacceptable. */
    eWiFiReasonAssocFailed,             /**< All other ASSOC errors. */
    eWiFiReasonIEInvalid,               /**< Management frame IE invalid. */
    eWiFiReasonMICFailure,              /**< MIC failure detected. */
    eWiFiReason4WayTimeout,             /**< 4WAY handshake timeout. */
    eWiFiReason4WayIEDiffer,            /**< 4WAY handshake IE error. */
    eWiFiReason4WayFailed,              /**< All other 4WAY errors. */
    eWiFiReasonAKMPInvalid,             /**< AKMP invalid. */
    eWiFiReasonPairwiseCipherInvalid,   /**< Pairwise cipher invalid. */
    eWiFiReasonGroupCipherInvalid,      /**< Group cipher invalid. */
    eWiFiReasonRSNVersionInvalid,       /**< RSN version invalid. */
    eWiFiReasonRSNCapInvalid,           /**< RSN capability invalid. */
    eWiFiReasonGroupKeyUpdateTimeout,   /**< Group key update timeout. */
    eWiFiReasonCipherSuiteRejected,     /**< Cipher violates security policy. */
    eWiFiReason8021XAuthFailed,         /**< 802.1X auth errors. */
    eWiFiReasonBeaconTimeout,           /**< Beacon timeout. */
    eWiFiReasonLinkFailed,              /**< All other link errors. */
    eWiFiReasonDHCPExpired,             /**< DHCP license expired. */
    eWiFiReasonDHCPFailed,              /**< All other DHCP errors. */
    eWiFiReasonMax
} WIFIReason_t;

/**
 * @brief Wi-Fi event types.
 */
typedef enum
{
    eWiFiEventReady,                    /**< Wi-Fi is initialized or was reset in the lower layer. */
    eWiFiEventScanDone,                 /**< Scan is finished. */
    eWiFiEventConnected,                /**< Station is connected to the AP. */
    eWiFiEventDisconnected,             /**< Station is disconnected from the AP. */
    eWiFiEventConnectionFailed,         /**< Station connection has failed. */
    eWiFiEventIPReady,                  /**< DHCP is successful. */
    eWiFiEventAPStateChanged,           /**< SoftAP state changed. */
    eWiFiEventAPStationConnected,       /**< SoftAP got a new station. */
    eWiFiEventAPStationDisconnected,    /**< SoftAP lost a new station. */
    eWiFiEventWPSSuccess,               /**< WPS is completed successfully. */
    eWiFiEventWPSFailed,                /**< WPS has failed. */
    eWiFiEventWPSTimeout,               /**< WPS has timeout. */
    eWiFiEventRxDone,                   /**< A frame is received in monitor mode. */
    eWiFiEventTxDone,                   /**< A frame is transmitted for packet injection. */
    eWiFiEventMax
} WIFIEventType_t;

/**
 * @brief Wi-Fi event info for WI-FI ready.
 */
typedef struct
{
    /* No data. */
} WiFiEventInfoReady_t;

/**
 * @brief Wi-Fi event info for scan done.
 */
typedef struct
{
    WIFIScanResultExt_t* pxScanResults;
    uint16_t usNumScanResults;
} WiFiEventInfoScanDone_t;

/**
 * @brief Wi-Fi event info for station connected to AP.
 */
typedef struct
{
    WIFIConnectionInfo_t xConnectionInfo;
} WiFiEventInfoConnected_t;

/**
 * @brief Wi-Fi event info for station disconnected from AP.
 */
typedef struct
{
    WIFIReason_t xReason;       /**< Reason code for station disconnection. */
} WiFiEventInfoDisconnected_t;

/**
 * @brief Wi-Fi event info for station connection failure.
 */
typedef struct
{
    WIFIReason_t xReason;       /**< Reason code for connection failure. */
} WiFiEventInfoConnectionFailed_t;

/**
 * @brief Wi-Fi event info for IP ready.
 */
typedef struct
{
    WIFIIPAddress_t xIPAddress; /**< Station IP address from DHCP. */
} WiFiEventInfoIPReady_t;

/**
 * @brief Wi-Fi event info for AP state change.
 */
typedef struct
{
    uint8_t ucState;            /**< AP state: 0 = DOWN, 1 = UP. */
} WiFiEventInfoAPStateChanged_t;

/**
 * @brief Wi-Fi event info for AP got a connected station.
 */
typedef struct
{
    uint8_t ucMac[ wificonfigMAX_BSSID_LEN ];   /**< MAC address of connected station. */
} WiFiEventInfoAPStationConnected_t;

/**
 * @brief Wi-Fi event info for AP got a disconnected station.
 */
typedef struct
{
    uint8_t ucMac[ wificonfigMAX_BSSID_LEN ];   /**< MAC address of disconnected station. */
    WIFIReason_t xReason;                       /**< Reason code for the disconnection. */
} WiFiEventInfoAPStationDisconnected_t;

/**
 * @brief Wi-Fi event info for receiving a frame in monitor mode (or normal mode
 * with RX filter).
 */
typedef struct
{
    uint8_t* pucData;       /**< Data buffer of received raw frame. */
    uint32_t ulLength;      /**< Length of the raw frame. */
} WiFiEventInfoRxDone_t;

/**
 * @brief Wi-Fi event info for finishing transmitting an injection frame.
 */
typedef struct
{
    /* No data. */
} WiFiEventInfoTxDone_t;

/**
 * @brief Wi-Fi combined event data structure.
 */
typedef struct
{
    WIFIEventType_t xEventType;
    union
        {
        WiFiEventInfoReady_t xReady;
        WiFiEventInfoScanDone_t xScanDone;
        WiFiEventInfoConnected_t xConnected;
        WiFiEventInfoIPReady_t xIPReady;
        WiFiEventInfoDisconnected_t xDisconnected;
        WiFiEventInfoConnectionFailed_t xConnectionFailed;
        WiFiEventInfoAPStateChanged_t xAPStateChanged;
        WiFiEventInfoAPStationConnected_t xAPStationConnected;
        WiFiEventInfoAPStationDisconnected_t xAPStationDisconnected;
        WiFiEventInfoRxDone_t xRxDone;
        WiFiEventInfoTxDone_t xTxDone;
    } xInfo;
} WIFIEvent_t;

/**
 * @brief Wi-Fi event handler definition.
 *
 * @param[in] xEvent - Wi-Fi event data structure.
 *
 * @return None.
 */
typedef void (*WIFIEventHandler_t)( WIFIEvent_t * xEvent );

/**
 * @brief Register a Wi-Fi event Handler.
 *
 * @param[in] xEventType - Wi-Fi event type.
 * @param[in] xHandler - Wi-Fi event handler.
 *
 * @return eWiFiSuccess if registration is successful, failure code otherwise.
 */
WIFIReturnCode_t WIFI_RegisterEvent( WIFIEventType_t xEventType,
                                     WIFIEventHandler_t xHandler );

#endif /* _AWS_WIFI_H_ */
