/*
 * Copyright 2018-2020 Amazon.com, Inc. or its affiliates. All rights reserved.
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

/**
* @file wifi_mgr.h
*
* @brief This file serves as shim layer for the wifi manager legacy api.
*        with wifi components. These api are to be deprecated.
*        Never include this file.
*/

#ifndef __ACE_WIFI_MGR_LEGACY_H__
#define __ACE_WIFI_MGR_LEGACY_H__

#ifdef __cplusplus
extern "C" {
#endif

// The only reason to include it here is due to unit test
// that uses the old api need to include wifi_mgr_legacy.h
#include <ace/wifi_mgr.h>

/**
 * @cond DEPRECATED
 * @deprecated Please use the new symbols.
 * @{
 */

/** The maximum length of an ssid */
#define MAX_SSID_LEN ACE_WIFI_MGR_MAX_SSID_LEN
#define BSSID_LEN ACE_WIFI_MGR_BSSID_LEN
#define INVALID_NETWORK_ID ACE_WIFI_MGR_INVALID_NETWORK_ID
#define HOST_NAME_MAX_LEN ACE_WIFI_MGR_HOST_NAME_MAX_LEN

/** The maximum length of a country code */
#define COUNTRY_CODE_LEN ACE_WIFI_MGR_COUNTRY_CODE_LEN

/** The maximum length of a psk */
#define KEY_MAX_LEN ACE_WIFI_MGR_PSK_KEY_MAX_LEN
/** The minimum length of a psk */
#define KEY_MIN_LEN ACE_WIFI_MGR_PSK_KEY_MIN_LEN
/** The maximum length of a wep key (64 and 128 bit only) */
#define WEP_MAX_LEN ACE_WIFI_MGR_WEP_KEY_MAX_LEN
/** The maximum number of wep keys */
#define MAX_WEP_KEY_NUM ACE_WIFI_MGR_MAX_WEP_KEY_NUM

/**
 * Wifi lower level supported feature mask.
 */
#define aceWifiMgr_WPS_SUPPORTED ACE_WIFI_MGR_WPS_SUPPORTED
#define aceWifiMgr_ENTERPRISE_SUPPORTED ACE_WIFI_MGR_ENTERPRISE_SUPPORTED
#define aceWifiMgr_P2P_SUPPORTED ACE_WIFI_MGR_P2P_SUPPORTED
#define aceWifiMgr_TDLS_SUPPORTED ACE_WIFI_MGR_TDLS_SUPPORTED

/** WEP key
 */
typedef struct {
    /** @ATT_META{string-not-term, size: keyLength} */
    char wepKey[WEP_MAX_LEN]; /**< WEP key */
    uint8_t keyLength;        /**< Length of the key */
} aceWifiMgr_wepKey_t;

/** Auth Mode
 *  Supported authentication protocols.
 */
#define aceWifiMgr_AUTH_OPEN ACE_WIFI_MGR_AUTH_OPEN /**< Open - No Security */
#define aceWifiMgr_AUTH_WEP ACE_WIFI_MGR_AUTH_WEP   /**< WEP Security */
#define aceWifiMgr_AUTH_WPA ACE_WIFI_MGR_AUTH_WPA   /**< WPA Security */
#define aceWifiMgr_AUTH_WPA2 ACE_WIFI_MGR_AUTH_WPA2 /**< WPA2 Security */
#define aceWifiMgr_AUTH_EAP ACE_WIFI_MGR_AUTH_EAP   /**< IEEE802.1x Security */
#define aceWifiMgr_eWiFiSecurityNotSupported \
    ACE_WIFI_MGR_AUTH_UNKNOWN /**< Unknown Security */

/**
 *  Wifi profile status
 */
#define aceWifiMgr_CONFIG_STATUS_CURRENT ACE_WIFI_MGR_CONFIG_STATUS_CURRENT
#define aceWifiMgr_CONFIG_STATUS_DISABLED ACE_WIFI_MGR_CONFIG_STATUS_DISABLED
#define aceWifiMgr_CONFIG_STATUS_ENABLED ACE_WIFI_MGR_CONFIG_STATUS_ENABLED
#define aceWifiMgr_CONFIG_STATUS_UPDATED ACE_WIFI_MGR_CONFIG_STATUS_UPDATED

#if defined(ACE_WIFI_IEEE8021X)
/**
 * WiFi configuration IEEE8021X authentication types
 */
#define aceWifiMgr_EAP_NONE ACE_WIFI_MGR_EAP_NONE /**< No EAP Protocol */
#define aceWifiMgr_EAP_PEAP ACE_WIFI_MGR_EAP_PEAP /**< Protected EAP Protocol \
                                                     */
#define aceWifiMgr_EAP_TLS \
    ACE_WIFI_MGR_EAP_TLS /**< EAP-Transport Layer Protocol */
#define aceWifiMgr_EAP_TTLS \
    ACE_WIFI_MGR_EAP_TTLS /**< EAP-Tunneled Transport Layer Protocol */

/**
 * EAP inner authentication methods
 */
#define aceWifiMgr_PHASE2_NONE ACE_WIFI_MGR_PHASE2_NONE /**< No Protocol */
#define aceWifiMgr_PHASE2_MSCHAPV2                                       \
    ACE_WIFI_MGR_PHASE2_MSCHAPV2 /**< Microsoft Challenge Authentication \
                                    Protocol */
#define aceWifiMgr_PHASE2_PAP \
    ACE_WIFI_MGR_PHASE2_PAP /**< Password Authentication Protocol */

typedef struct {
    aceWifiMgr_enterpriseEap_t
        type; /**< The Extensible Authentication Protocol method used  */
    aceWifiMgr_enterprisePhase2_t
        phase2; /**< The inner authentication method used */
    aceWifiMgr_crtPolicy_t
        policy; /**< Contains certificate validation rules. */
    aceWifiMgr_x509CertList_t
        ca_certs; /**< List of CA certificates in PEM/DER format */
    aceWifiMgr_x509Cert_t cert; /**< Client certificate in PEM/DER format*/
    aceWifiMgr_x509Cert_t
        pkey; /**< Client private RSA key in PEM/DER format. NOT encrypted. */
    aceWifiMgr_userCred_t anonymous; /**< Unencrypted identity if eap supports
                                        tunnelled type (e.g., PEAP). */
    aceWifiMgr_userCred_t identity;  /**< User identity. */
} aceWifiMgr_enterpriseConfig_t;
#endif  // ACE_WIFI_IEEE8021X

#if 0
/** IP address type
 */
/**< IPV4, set to the value of AF_INET to be backward compatible */
#define ACE_WIFI_IP_TYPE_IPV4 ACE_WIFI_MGR_IP_TYPE_IPV4
/**< IPV6, same value as AF_INET6 */
#define ACE_WIFI_IP_TYPE_IPV6 ACE_WIFI_MGR_IP_TYPE_IPV6
/**< uknown, same vaue as AF_UNSPEC */
#define ACE_WIFI_IP_TYPE_UNSPEC ACE_WIFI_MGR_IP_TYPE_UNSPEC
#endif

/** IP address type
 */
typedef enum {
    ACE_WIFI_IP_TYPE_IPV4 =
        ACE_WIFI_MGR_IP_TYPE_IPV4, /**< IPV4, set to the value of AF_INET to be
                                      backward compatible */
    ACE_WIFI_IP_TYPE_IPV6 =
        ACE_WIFI_MGR_IP_TYPE_IPV6, /**< IPV6, same value as AF_INET6 */
    ACE_WIFI_IP_TYPE_UNSPEC =
        ACE_WIFI_MGR_IP_TYPE_UNSPEC /**< uknown, same vaue as AF_UNSPEC */
} aceWifi_ipType_t;

/** IP Address
 */
typedef struct {
    aceWifi_ipType_t type; /**< e.g. IPV4, IPV6 */
    uint32_t ipAddress[4]; /**< IP address in binary form i.e. use
                              inet_ntop/inet_pton for conversion */
} aceWifi_ipAddress_t;

/** IP Address Configuration
 */
typedef struct {
    uint8_t isStatic;              /**< 1 if static IP config, else 0 */
    aceWifi_ipAddress_t ipAddress; /**< IP address */
    aceWifi_ipAddress_t netMask;   /**< Network mask */
    aceWifi_ipAddress_t gateway;   /**< Gateway IP address */
    aceWifi_ipAddress_t dns1;      /**< First DNS server IP address */
    aceWifi_ipAddress_t dns2;      /**< Second DNS server IP address */
    aceWifi_ipAddress_t dns3;      /**< Third DNS server IP address */
} aceWifiMgr_IpConfiguration_t;

/** Wifi Configuration
 *  Describes a configured network.
 */
typedef struct {
    /** @ATT_META{string-not-term, size: ssidLength} */
    char ssid[MAX_SSID_LEN]; /**< The network's SSID. NOT a null terminated
                                string, can be raw bytes e.g. for unicode */
    uint8_t ssidLength; /**< Length of the SSID. MUST be set instead of a null
                           terminator */
    /** @ATT_META{string-not-term, size: pskLength} */
    char psk[KEY_MAX_LEN]; /**< Pre-shared key for use with WPA-PSK, NOT null
                              terminated. */
    uint8_t pskLength;     /**< Length of the psk. MUST be set instead of a null
                              terminator */
    aceWifiMgr_wepKey_t wepKeys[MAX_WEP_KEY_NUM]; /**< WEP keys, up to four
                                                     keys, for WEP networks */
    uint8_t wepKeyIndex; /**< Default WEP key index, ranging from 0 to 3 */
    aceWifiMgr_authMode_t
        authMode; /**< The authentication protocol supported by this config */
    uint8_t
        hiddenSsid; /**< Whether this network is hidden - 0 = false, 1 = true.
                         A hidden network does not broadcast its SSID, so an
                         SSID-specific probe request must be used for scans. */
    aceWifiMgr_configStatus_t
        status; /**< Current status of this network configuration entry */
    uint8_t isTethered; /**< Whether this network is tethered */
#if defined(ACE_WIFI_IEEE8021X)
    aceWifiMgr_enterpriseConfig_t eap; /**< 802.1X configuration */
#endif
#if defined(ACE_WIFI_STATIC_IP)
    aceWifiMgr_IpConfiguration_t ipConfig; /**< IP configuration */
#endif
#if defined(ACE_WIFI_PROFILE_PRIORITY)
    uint8_t priority; /**< Determines the preference given to a network
                            during auto connection attempt. */
#endif
} aceWifiMgr_config_t;

/** Configured networks list
 *  List of configured networks. Used by aceWifiMgr_getConfiguredNetworks()
 */
typedef struct {
    uint16_t length;
    /** @ATT_META{size: length} */
    aceWifiMgr_config_t list[ACE_WIFI_MAX_CONFIGURED_NETWORKS];
} aceWifiMgr_configList_t;

/** Scan Result.
 *  Basic results of a scan request. It doesn't have bssid and freq info.
 */
typedef struct {
    /** @ATT_META{string-not-term, size: ssidLength} */
    char ssid[MAX_SSID_LEN]; /**< The network's SSID. NOT a null terminated
                                string, can be raw bytes e.g. for unicode */
    uint8_t
        ssidLength; /**< Length of the SSID. Must be set in place of a null
                       terminator */
    aceWifiMgr_authMode_t
        authMode; /**< The authentication protocol supported by this network */
    int8_t
        rssi; /**< The detected signal level in dBm, also known as the RSSI */
} aceWifiMgr_scanResult_t;

/** Scan results list
 *  List of scan results. Used by aceWifiMgr_getScanResults()
 */
typedef struct {
    uint16_t length;
    /** @ATT_META{size: length} */
    aceWifiMgr_scanResult_t list[ACE_WIFI_MAX_SCAN_RESULTS];
} aceWifiMgr_scanResultList_t;

/** Detailed Scan Result
 *  Detailed results of a scan request, this is an extension of
 * aceWifiMgr_scanResult_t,
 *  includes bssid and freq info.
 */
typedef struct {
    char ssid[MAX_SSID_LEN];  /**< The network's SSID. NOT a null terminated
                                 string, can be raw bytes e.g. for unicode */
    uint8_t bssid[BSSID_LEN]; /**< BSSID of the network */
    uint8_t
        ssidLength; /**< Length of the SSID. Must be set in place of a null
                       terminator */
    aceWifiMgr_authMode_t
        authMode; /**< The authentication protocol supported by this network */
    int8_t
        rssi; /**< The detected signal level in dBm, also known as the RSSI */
    uint16_t frequency; /**< Frequency */
} aceWifiMgr_detailedScanResult_t;

/** Detailed scan results list
 *  List of detailed scan results. Used by aceWifiMgr_getDetailedScanResults()
 */
typedef struct {
    uint16_t length;
    aceWifiMgr_detailedScanResult_t list[ACE_WIFI_MAX_SCAN_RESULTS];
} aceWifiMgr_detailedScanResultList_t;

/** Network State
 *  Describes the current state of the network. When registered for network
 *  state change events, the state will only be published at the moment of the
 *  state change. e.g. NET_STATE_DISCONNECTED will only be published if the
 *  device is already connected. Similarly, NET_STATE_CONNECTED will only be
 *  published if the device was not already in the connected state.
 */
#define NET_STATE_CONNECTING                                                  \
    ACE_WIFI_MGR_NET_STATE_CONNECTING /**< The device is currently connecting \
                                         */
#define NET_STATE_CONNECTED \
    ACE_WIFI_MGR_NET_STATE_CONNECTED /**< The device is connected */
#define NET_STATE_DISCONNECTING \
    ACE_WIFI_MGR_NET_STATE_DISCONNECTING /**< The device is disconnecting */
#define NET_STATE_DISCONNECTED \
    ACE_WIFI_MGR_NET_STATE_DISCONNECTED /**< The device is disconnected */
#define NET_STATE_UNKNOWN \
    ACE_WIFI_MGR_NET_STATE_UNKNOWN /**< The network state is unknown */

/** Disconnect reasons
 *  Reason for NET_STATE_DISCONNECTED event
 */
#define DISCONNECT_ASSOC_FAILURE \
    ACE_WIFI_MGR_DISCONNECT_ASSOC_FAILURE /**< Cannot connect to AP */
#define DISCONNECT_AUTH_FAILURE \
    ACE_WIFI_MGR_DISCONNECT_AUTH_FAILURE /**< Password, Key etc */
#define DISCONNECT_IP_FAILURE \
    ACE_WIFI_MGR_DISCONNECT_IP_FAILURE /**< Cannot obtain IP address */
#define DISCONNECT_LOCALLY_GENERATED                                        \
    ACE_WIFI_MGR_DISCONNECT_LOCALLY_GENERATED /**< Locally generated device \
                                                 side disconnect */
#define DISCONNECT_UNDEFINED \
    ACE_WIFI_MGR_DISCONNECT_UNDEFINED /**< Undefined reason */

/** Captive Portal State
 *  Describes the current state of the captive portal.
 */
#define CAPTIVE_PORTAL_AUTHENTICATED                                        \
    ACE_WIFI_MGR_CAPTIVE_PORTAL_AUTHENTICATED /**< The device has           \
                                                 authenticated with captive \
                                                 portal */
#define CAPTIVE_PORTAL_CONNECTED                                          \
    ACE_WIFI_MGR_CAPTIVE_PORTAL_CONNECTED /**< The device is connected to \
                                             captive portal */
#define CAPTIVE_PORTAL_FAILED                                              \
    ACE_WIFI_MGR_CAPTIVE_PORTAL_FAILED /**< The device has failed to check \
                                          captive portal */

/** Wifi Info
 *  Describes the currently connected WiFi network.
 */
typedef struct {
    uint8_t bssid[BSSID_LEN]; /**< BSSID of the connected network */
    /** @ATT_META{string-not-term, size: ssidLength} */
    char ssid[MAX_SSID_LEN]; /**< The network's SSID. NOT a null terminated
                                string, can be raw bytes e.g. for unicode */
    uint8_t ssidLength; /**< Length of the SSID. MUST be set instead of a null
                           terminator */
    aceWifiMgr_authMode_t
        authMode; /**< The authentication protocol supported by this network */
    aceWifi_ipAddress_t ipAddress; /**< IP address */
    uint16_t frequency;            /**< Frequency of the connected network */
    aceWifiMgr_disconnectReason_t reason; /**< Reason code for a disconnect */
} aceWifiMgr_wifiInfo_t;

/** Soft AP State
 *  State events types that indicate the state of the soft AP. When registered
 *  for soft AP state change events, the state will only be  published at the
 *  moment of the state change.
 */
#define SOFTAP_UP ACE_WIFI_MGR_SOFTAP_UP     /**< soft AP created */
#define SOFTAP_DOWN ACE_WIFI_MGR_SOFTAP_DOWN /**< soft AP removed */
#define SOFTAP_CONNECTED                                                     \
    ACE_WIFI_MGR_SOFTAP_CONNECTED /**< a device has connected to the soft AP \
                                     */
#define SOFTAP_DISCONNECTED                                                \
    ACE_WIFI_MGR_SOFTAP_DISCONNECTED /**< a device has disconnected to the \
                                        soft AP */

/**
 * @brief Event that indicates that an update occurred on a WiFi profile
 */
typedef struct aceWifiMgr_profileUpdateInfo {
    char ssid[MAX_SSID_LEN]; /**< The network's SSID. NOT a null terminated
                                string, can be raw bytes e.g. for unicode */
    uint8_t
        ssidLength; /**< Length of the SSID. Must be set in place of a null
                       terminator */
    aceWifiMgr_authMode_t
        authMode; /**< The authentication protocol supported by this network */
    aceWifiMgr_profileUpdateEvent_t type; /**< The type of the profile update */
} aceWifiMgr_profileUpdateInfo_t;

/**
 * @brief Wifi core statistic info.
 */
typedef struct {
    uint32_t txSuccessCount;  /**< No of TX success */
    uint32_t txRetryCount;    /**< No of TX tx packet retries */
    uint32_t txFailCount;     /**< No of TX failure */
    uint32_t rxSuccessCount;  /**< No of RX success */
    uint32_t rxCRCErrorCount; /**< No of RX CRC */
    uint32_t MICErrorCount;   /**< No of Mic error count */
    uint32_t nss;             /**< No of spatial streams (MIMO)  */
    int8_t noise;             /**< noise */
    int8_t noise2;            /**< noise stream2 */
    uint16_t phyRate;         /**< Max phy rate */
    uint16_t txRate;          /**< tx rate */
    uint16_t rxRate;          /**< rx rate */
    int8_t rssi;              /**< RSSI */
    int8_t rssi2;             /**< RSSI stream2 */
    int8_t brssi;             /**< Beacon RSSI  */
    uint8_t channel;          /**< Channel */
    uint8_t bandwidth;        /**< Bandwidth */
    uint8_t idleTimePer;      /**< Percent of idle time */
    uint8_t signal_bar;       /**< Signal strength bars */
} aceWifiMgr_statistic_t;

/**
 * @brief Wifi band.
 */
#define aceWifiMgr_BAND_2G ACE_WIFI_MGR_BAND_2G     /**< 2.4G */
#define aceWifiMgr_BAND_5G ACE_WIFI_MGR_BAND_5G     /**< 5G */
#define aceWifiMgr_BAND_DUAL ACE_WIFI_MGR_BAND_DUAL /**< dual band */
#define aceWifiMgr_BAND_MAX \
    ACE_WIFI_MGR_BAND_UNSUPPORTED /**< unsupported band */

/**
 * @brief Wifi phymode.
 */
#define aceWifiMgr_PHY_11B ACE_WIFI_MGR_PHY_11B         /**< 11B */
#define aceWifiMgr_PHY_11G ACE_WIFI_MGR_PHY_11G         /**< 11G */
#define aceWifiMgr_PHY_11N ACE_WIFI_MGR_PHY_11N         /**< 11N */
#define aceWifiMgr_PHY_11AC ACE_WIFI_MGR_PHY_11AC       /**< 11AC */
#define aceWifiMgr_PHY_11AX ACE_WIFI_MGR_PHY_11AX       /**< 11AX */
#define aceWifiMgr_PHY_MAX ACE_WIFI_MGR_PHY_UNSUPPORTED /**< Unsupported phy \
                                                           */

/**
 * @brief Wifi bandwidth.
 */
#define aceWifiMgr_BW_20 ACE_WIFI_MGR_BW_20
#define aceWifiMgr_BW_40 ACE_WIFI_MGR_BW_40
#define aceWifiMgr_BW_80 ACE_WIFI_MGR_BW_80
#define aceWifiMgr_BW_160 ACE_WIFI_MGR_BW_160
#define aceWifiMgr_BW_MAX ACE_WIFI_MGR_BW_UNSUPPORTED

/**
 * @brief Wi-Fi device power management modes.
 *
 * Device power management modes supported.
 */
#define aceWifiMgr_PM_NORMAL ACE_WIFI_MGR_PM_NORMAL /**< Normal mode. */
#define aceWifiMgr_PM_LOW ACE_WIFI_MGR_PM_LOW       /**< Low Power mode. */
#define aceWifiMgr_PM_ALWAYS_ON \
    ACE_WIFI_MGR_PM_ALWAYS_ON /**< Always On mode. */
#define aceWifiMgr_PM_MAX \
    ACE_WIFI_MGR_PM_UNSUPPORTED /**< Unsupported PM mode. */

/**
 * @brief Wifi capability info.
 */
typedef struct {
    aceWifiMgr_band_t band;           /**< band ex 2.4g/5g/dual */
    aceWifiMgr_phyMode_t phyMode;     /**< Phymode ex 11b,11g */
    aceWifiMgr_bandwidth_t bandwidth; /**< Bandwidth ex 20M */
    uint32_t maxAggr;                 /**< Max aggregation */
    uint16_t supportedFeatures; /**< Supported feature, ex wps/p2p/enterprise,
                                     in bit mask aceWifiMgr_WPS_SUPPORTED|xxx */
} aceWifiMgr_capability_t;

/**
 * Callback function type to receive network state change events
 *
 * @param netState: the current network state
 * @param ctx: client data, passed in by register function.
 * @param wifiInfo: info about the current network
 */
typedef void (*aceWifiMgr_networkStateEventCallback_t)(
    aceWifiMgr_networkState_t netState, aceWifiMgr_wifiInfo_t wifiInfo,
    void* ctx);

#if defined(ACE_WIFI_PROFILE_EVENT)
/**
 * Callback function type to receive profile update events
 *
 * @param profileUpdateInfo: The reference to the {@link
 * aceWifiMgr_profileUpdateInfo_t}
 * @param ctx: client data, passed in by register function
 */
typedef void (*aceWifiMgr_profileUpdateEventCallback_t)(
    aceWifiMgr_profileUpdateInfo_t profileUpdateInfo, void* ctx);
#endif

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get a list of scan results.
 * This list contains the filtered scan results which does not have duplicate
 * ssid
 * (with same security) or 'empty' / wildcard ssids which contain null bytes or
 * have
 * a length of 0.
 *
 * Passed in results can be empty or already contains previous scan results.
 * If it contains previous scan results already, will be merged with the new
 * scan results, filtering out the duplicated ssid.
 *
 * To ensure an up to date list, the caller should
 * call aceWifiMgr_startScan() and wait for the scan done event before calling
 * aceWifiMgr_getScanResults(), otherwise the list may be empty, since it is not
 * cached long term.
 *
 * @param[in, out] results: Pointer to valid memory of size
 * aceWifiMgr_scanResultList_t,
 *                 allocated by the caller.
 *                 Caller can pass in previous scan results or empty results,
 * results->length
 *                 indicates the number of the scan results passed in. Wifi
 * manager
 *                 will merge the results with the new scan results, if the
 * length is not 0.
 *                 At return, results->length will be set to the total number of
 * scan results
 *                 returned in results->list. If there are no scan results
 * available,
 *                 the length will be 0.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getScanResults(aceWifiMgr_scanResultList_t* results);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get a list of raw scan results, which is not filtered from the original scan
 * results from wifi driver.
 * To ensure an up to date list, the caller should
 * call aceWifiMgr_startScan() and wait for the scan done event before calling
 * aceWifiMgr_getScanResults(), otherwise the list may be empty, since it is not
 * cached long term.
 *
 * @param[out] results: Pointer to valid memory of size
 * aceWifiMgr_scanResultList_t,
 *                 allocated by the caller. results->length will be set to the
 *                 total number of scan results returned in results->list.
 *                 If there are no scan results available, the length will be 0.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getDetailedScanResults(
    aceWifiMgr_detailedScanResultList_t* results);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get a list of configured networks.
 *
 * @param[out] networks: Pointer to valid memory of size aceWifiMgr_config_t,
 *                  allocated by the caller. networks->length will be set to the
 *                  total number of configured networks returned in
 * results->list.
 *                  If there are no configured networks, the length will be 0.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getConfiguredNetworks(
    aceWifiMgr_configList_t* networks);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Add a new wifi network profile to the set of configured networks. This
 * profile
 * will not be automatically saved -- to save it to file, call
 * aceWifiMgr_saveConfig().
 *
 * @param config: Wifi configuration to be added. Must be allocated and
 * populated by the caller.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_addNetwork(aceWifiMgr_config_t* config);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Remove the specified wifi profile from the list of configured networks. This
 * change will not be automatically saved -- to save it to file, call
 * aceWifiMgr_saveConfig().
 *
 * @param ssid: Wifi profile to be removed.
 * @param authMode: The authmode of the Profile.
 *        When set to aceWifiMgr_eWiFiSecurityNotSupported, then any authmode
 * can apply.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_removeNetwork(const char* ssid,
                                      aceWifiMgr_authMode_t authMode);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the currently connected network's wifi info. The caller must allocate
 * enough space for aceWifiMgr_wifiInfo_t. If the network is not currently
 * connected,
 * this function will return @ref ACE_STATUS_GENERAL_ERROR.
 *
 * @param[out] retWifiInfo: Pointer to an allocated aceWifiMgr_wifiInfo_t, to be
 * populated by aceWifiMgr.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getConnectionInfo(aceWifiMgr_wifiInfo_t* retWifiInfo);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the currently connected network's ip info. The caller must allocate
 * enough space for aceWifiMgr_IpConfiguration_t.
 *
 * @param[out] retIpInfo: Pointer to an allocated aceWifiMgr_IpConfiguration_t,
 * to be populated by aceWifiMgr.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getIpInfo(aceWifiMgr_IpConfiguration_t* retIpInfo);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the wifi statistic info. The caller must allocate enough space
 * for aceWifiMgr_statistic_t.
 *
 * @param[out] retStatInfo: Pointer to an allocated aceWifiMgr_statistic_t, to
 * be populated by aceWifiMgr.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getStatisticInfo(aceWifiMgr_statistic_t* retStatInfo);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the wifi capability info. The caller must allocate enough space
 * for aceWifiMgr_capability_t.
 *
 * @param[out] retCapInfo: Pointer to an allocated aceWifiMgr_capability_t, to
 * be populated by aceWifiMgr.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getCapabilityInfo(aceWifiMgr_capability_t* retCapInfo);

/**
 * Register a callback function for network state change events.
 * This api is allowed to be used even before wifi svc is ready.
 * Note: inside callback, do not do heavy work, do not call any wifi api
 including
 *       deregister. Calling wifi api inside the callback could cause deadlock
 in wifi svc.
 *
 * @param networkStateEventCallback: callback to register
 * @param ctx: client data, will be passed back in the callback function, can be
 NULL.
               Note: Client needs to keep this data pointer not being released.
 * @return ace_status_t: Return ACE_STATUS_OK on registration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_registerNetworkStateEvent(
    aceWifiMgr_networkStateEventCallback_t networkStateEventCallback,
    void* ctx);

/**
 * Deregister a callback function from network state change events.
 *
 * @param networkStateEventCallback: callback to deregister
 * @return ace_status_t: Return ACE_STATUS_OK on deregistration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_deregisterNetworkStateEvent(
    aceWifiMgr_networkStateEventCallback_t networkStateEventCallback);

#if defined(ACE_WIFI_PROFILE_EVENT)
/**
 * Register a callback function for {@link aceWifiMgr_profileUpdateInfo_t}
 * events.
 * This api is allowed to be used even before wifi svc is ready.
 * Note: inside callback, do not do heavy work, do not call any wifi api
 * including
 *       deregister. Calling wifi api inside the callback could cause deadlock
 * in wifi svc.
 *
 * @param profileUpdateEventCallback: The callback to register
 * @param ctx: The client data to be passed in when the callback is invoked, can
 * be NULL.
 * @return ace_status_t: Return ACE_STATUS_OK on registration
 * success, an error code in case of failure
 */
ace_status_t aceWifiMgr_registerProfileUpdateEvent(
    aceWifiMgr_profileUpdateEventCallback_t profileUpdateEventCallback,
    void* ctx);

/**
 * Deregister a callback function for {@link aceWifiMgr_profileUpdateInfo_t}
 * events
 *
 * @param profileUpdateEventCallback: The callback to deregister
 * @return ace_status_t: Return ACE_STATUS_OK on deregistration
 * success, an error code in case of failure
 */
ace_status_t aceWifiMgr_deregisterProfileUpdateEvent(
    aceWifiMgr_profileUpdateEventCallback_t profileUpdateEventCallback);
#endif

#if defined(ACE_WIFI_PROFILE_UPDATE)
/**
 * Restore original Wifi configuration.
 *
 * @param ssid: Wifi profile to be restored.
 * @param authMode: The authmode of the Profile.
 *        When set to aceWifiMgr_eWiFiSecurityNotSupported, then any authmode
 can apply.

 * @return ace_status_t: Return ACE_STATUS_OK on backup success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_restoreConfig(const char* ssid,
                                      aceWifiMgr_authMode_t authMode);
/**
 * Update existing wifi network profile. This profile
 * will not be automatically saved -- to save it to file, call
 * aceWifiMgr_saveConfig().
 *
 * @param config: Wifi configuration to be added. Must be allocated and
 * populated by the caller.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure.
 */
ace_status_t aceWifiMgr_updateNetwork(aceWifiMgr_config_t* config);

#endif  // ACE_WIFI_PROFILE_UPDATE

/**
 * @}
 * @endcond
 */  // cond DEPRECATED

#ifdef __cplusplus
}
#endif

#endif  // __ACE_WIFI_MGR_LEGACY_H__
