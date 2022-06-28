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
* @brief The ACS Wi-Fi Manager provides an API for app developers to interact
* with Wi-Fi components.
*
* USAGE
* -----
*
* ACS Wi-Fi Manager
* Users must first call aceWifiMgr_init() before calling other functions.
* In addition, the Wi-Fi service must be up and running before any API calls
* (except the event register API) can be made successfully. The API
* aceWifiMgr_isWifiReady() can be used to determine if the Wi-Fi service is
* ready.
* If not ready, callers can subscribe to the Wi-Fi ready event through
* @ref EVENTMGR_API.
*
* When finished or exiting, call aceWifiMgr_deinit() to clean up.
*
* Event callbacks may be registered to receive wifi state related events
* through @ref EVENTMGR_API.
*/

#ifndef __ACE_WIFI_MGR_H__
#define __ACE_WIFI_MGR_H__
#ifdef __cplusplus
extern "C" {
#endif

// TODO change to ace_status.h in final step
#include <ace/wifi_mgr_err.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <ace/ace_config.h>

/**
 * @defgroup ACE_WIFI_DEF structs, defines, enums
 * @{
 * @ingroup ACE_WIFI
 */

/** The maximum length of an ssid */
#define ACE_WIFI_MGR_MAX_SSID_LEN 32
#define ACE_WIFI_MGR_BSSID_LEN 6
#define ACE_WIFI_MGR_INVALID_NETWORK_ID -1
#define ACE_WIFI_MGR_HOST_NAME_MAX_LEN 255

/** The maximum length of a country code */
#define ACE_WIFI_MGR_COUNTRY_CODE_LEN 4

/** The maximum length of a psk */
#define ACE_WIFI_MGR_PSK_KEY_MAX_LEN 64
/** The minimum length of a psk */
#define ACE_WIFI_MGR_PSK_KEY_MIN_LEN 8
/** The maximum length of a wep key (64 and 128 bit only) */
#define ACE_WIFI_MGR_WEP_KEY_MAX_LEN 26
/** The maximum number of wep keys */
#define ACE_WIFI_MGR_MAX_WEP_KEY_NUM 4

/**
 * Wi-Fi lower level supported feature mask.
 */
#define ACE_WIFI_MGR_WPS_SUPPORTED 0x0001
#define ACE_WIFI_MGR_ENTERPRISE_SUPPORTED 0x0002
#define ACE_WIFI_MGR_P2P_SUPPORTED 0x0003
#define ACE_WIFI_MGR_TDLS_SUPPORTED 0x0004

/** Fixed length mac address string
*/
typedef char aceWifiMgr_macAddress_t[18];

/** Fixed length Wi-Fi country code string
*/
typedef char aceWifiMgr_countryCode_t[ACE_WIFI_MGR_COUNTRY_CODE_LEN];

/** WEP key
 */
typedef struct {
    /** @ATT_META{string-not-term, size: key_length} */
    char wep_key[ACE_WIFI_MGR_WEP_KEY_MAX_LEN]; /**< WEP key */
    uint8_t key_length;                         /**< Length of the key */
} aceWifiMgr_wepKeyVal_t;

/** Auth Mode
 *  Supported authentication protocols.
 */
typedef enum {
    ACE_WIFI_MGR_AUTH_OPEN = 0, /**< Open - No Security */
    ACE_WIFI_MGR_AUTH_WEP,      /**< WEP Security */
    ACE_WIFI_MGR_AUTH_WPA,      /**< WPA Security */
    ACE_WIFI_MGR_AUTH_WPA2,     /**< WPA2 Security */
    ACE_WIFI_MGR_AUTH_EAP,      /**< IEEE802.1x Security */
    ACE_WIFI_MGR_AUTH_UNKNOWN   /**< Unknown Security */
} aceWifiMgr_authMode_t;

/**
 *  Wi-Fi profile status
 */
typedef enum {
    ACE_WIFI_MGR_CONFIG_STATUS_CURRENT =
        0, /**< Profile is the current active one */
    ACE_WIFI_MGR_CONFIG_STATUS_DISABLED, /**< Profile is disabled */
    ACE_WIFI_MGR_CONFIG_STATUS_ENABLED,  /**< Profile is enabled */
    ACE_WIFI_MGR_CONFIG_STATUS_UPDATED   /**< Profile is updated */
} aceWifiMgr_configStatus_t;

typedef enum {
    ACE_WIFI_LOG_VERBOSE = 0,
    ACE_WIFI_LOG_DEBUG,
    ACE_WIFI_LOG_INFO,
    ACE_WIFI_LOG_WARN,
    ACE_WIFI_LOG_ERROR,
    ACE_WIFI_LOG_FATAL
} aceWifiMgr_logLevel_t;

#if defined(ACE_WIFI_IEEE8021X)
/**
 * Wi-Fi configuration IEEE8021X authentication types
 */
typedef enum {
    ACE_WIFI_MGR_EAP_NONE = 0, /**< No EAP Protocol */
    ACE_WIFI_MGR_EAP_PEAP,     /**< Protected EAP Protocol */
    ACE_WIFI_MGR_EAP_TLS,      /**< EAP-Transport Layer Protocol */
    ACE_WIFI_MGR_EAP_TTLS,     /**< EAP-Tunneled Transport Layer Protocol */
} aceWifiMgr_enterpriseEap_t;

/**
 * EAP inner authentication methods
 */
typedef enum {
    ACE_WIFI_MGR_PHASE2_NONE = 0, /**< No Protocol */
    ACE_WIFI_MGR_PHASE2_MSCHAPV2, /**< Microsoft Challenge Authentication
                                     Protocol */
    ACE_WIFI_MGR_PHASE2_PAP,      /**< Password Authentication Protocol */
} aceWifiMgr_enterprisePhase2_t;

typedef struct {
    int32_t len;    /**< length of encoded certificate */
    uint8_t* p_val; /**< certificate encoded as PEM/DER format */
} aceWifiMgr_x509Cert_t;

typedef struct {
    int32_t len;                  /**< Number of certificates */
    aceWifiMgr_x509Cert_t* p_val; /**< List of certificates. */
} aceWifiMgr_x509CertList_t;

typedef struct {
    int32_t len;    /**< size of user credential */
    uint8_t* p_val; /**< value of user credential */
} aceWifiMgr_userCred_t;

typedef uint32_t aceWifiMgr_crtPolicy_t;

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
    aceWifiMgr_x509Cert_t private_key; /**< Client private RSA key in PEM/DER
                                          format. NOT encrypted. */
    aceWifiMgr_userCred_t anonymous;   /**< Unencrypted identity if eap supports
                                          tunnelled type (e.g., PEAP). */
    aceWifiMgr_userCred_t identity;    /**< User identity. */
} aceWifiMgr_enterpriseProfile_t;
#endif  // ACE_WIFI_IEEE8021X

/** IP address type
 */
typedef enum {
    ACE_WIFI_MGR_IP_TYPE_IPV4 =
        2, /**< IPV4, set to the value of AF_INET to be backward compatible */
    ACE_WIFI_MGR_IP_TYPE_IPV6 = 10, /**< IPV6, same value as AF_INET6 */
    ACE_WIFI_MGR_IP_TYPE_UNSPEC = 0 /**< uknown, same vaue as AF_UNSPEC */
} aceWifiMgr_ipType_t;

/** IP Address
 */
typedef struct {
    aceWifiMgr_ipType_t type; /**< e.g. IPV4, IPV6 */
    uint32_t ip_address[4];   /**< IP address in binary form i.e. use
                                 inet_ntop/inet_pton for conversion */
} aceWifiMgr_ipAddress_t;

/** IP Address Configuration
 */
typedef struct {
    uint8_t is_static;                 /**< 1 if static IP config, else 0 */
    aceWifiMgr_ipAddress_t ip_address; /**< IP address */
    aceWifiMgr_ipAddress_t net_mask;   /**< Network mask */
    aceWifiMgr_ipAddress_t gateway;    /**< Gateway IP address */
    aceWifiMgr_ipAddress_t dns1;       /**< First DNS server IP address */
    aceWifiMgr_ipAddress_t dns2;       /**< Second DNS server IP address */
    aceWifiMgr_ipAddress_t dns3;       /**< Third DNS server IP address */
} aceWifiMgr_ipConfiguration_t;

/** Wi-Fi Profile
 *  Describes a Wi-Fi profile
 */
typedef struct {
    /** @ATT_META{string-not-term, size: ssid_length} */
    /** The network's SSID. NOT a null terminated string, can be raw
        bytes e.g. for unicode */
    char ssid[ACE_WIFI_MGR_MAX_SSID_LEN];
    /** Length of the SSID. MUST be set instead of a null terminator */
    uint8_t ssid_length;
    /** @ATT_META{string-not-term, size: psk_length} */
    char psk[ACE_WIFI_MGR_PSK_KEY_MAX_LEN]; /**< Pre-shared key for use with
                                                 WPA-PSK, NOT null terminated.*/
    uint8_t psk_length; /**< Length of the psk. MUST be set instead of a null
                           terminator */
    /** WEP keys, up to four keys, for WEP networks */
    aceWifiMgr_wepKeyVal_t wep_keys[ACE_WIFI_MGR_MAX_WEP_KEY_NUM];
    uint8_t wep_key_index; /**< Default WEP key index, ranging from 0 to 3 */
    /** The authentication protocol supported by this config */
    aceWifiMgr_authMode_t auth_mode;
    /** Whether this network is hidden 0 = false, 1 = true.
         A hidden network does not broadcast its SSID, so an
         SSID-specific probe request must be used for scans. */
    uint8_t hidden_ssid;
    aceWifiMgr_configStatus_t status; /**< Current status of this network
                                           configuration entry */
    uint8_t is_tethered;              /**< Whether this network is tethered */
#if defined(ACE_WIFI_IEEE8021X)
    aceWifiMgr_enterpriseProfile_t eap; /**< 802.1X configuration */
#endif
#if defined(ACE_WIFI_STATIC_IP)
    aceWifiMgr_ipConfiguration_t ip_config; /**< IP configuration */
#endif
#if defined(ACE_WIFI_PROFILE_PRIORITY)
    uint8_t priority; /**< Determines the preference given to a network
                           during auto connection attempt. */
#endif
} aceWifiMgr_profile_t;

/** Wi-Fi profile list
 *  List of configured networks. Used by aceWifiMgr_getConfiguredProfiles()
 */
typedef struct {
    uint16_t length;
    /** @ATT_META{size: length} */
    aceWifiMgr_profile_t list[ACE_WIFI_MAX_CONFIGURED_NETWORKS];
} aceWifiMgr_profileList_t;

/** Scan Result.
 *  Basic results of a scan request. It doesn't have bssid and freq info.
 */
typedef struct {
    /** @ATT_META{string-not-term, size: ssid_length} */
    char ssid[ACE_WIFI_MGR_MAX_SSID_LEN]; /**< The network's SSID. NOT a null
                                            terminated string, can be raw bytes
                                            e.g. for unicode */
    uint8_t ssid_length; /**< Length of the SSID. Must be set in place of
                              a null terminator */
    aceWifiMgr_authMode_t auth_mode; /**< The authentication protocol supported
                                          by this network */
    int8_t rssi; /**< The detected signal level in dBm, also known
                      as the RSSI */
} aceWifiMgr_scanResultItem_t;

/** Scan results list
 *  List of scan results. Used by aceWifiMgr_getScanResultList()
 */
typedef struct {
    uint16_t length;
    /** @ATT_META{size: length} */
    aceWifiMgr_scanResultItem_t list[ACE_WIFI_MAX_SCAN_RESULTS];
} aceWifiMgr_scanResults_t;

/** Detailed Scan Result
 *  Detailed results of a scan request, this is an extension of
 * aceWifiMgr_scanResult_t,
 *  includes bssid and freq info.
 */
typedef struct {
    /** The network's SSID. NOT a null terminated string, can be raw bytes
        e.g. for unicode */
    char ssid[ACE_WIFI_MGR_MAX_SSID_LEN];
    uint8_t bssid[ACE_WIFI_MGR_BSSID_LEN]; /**< BSSID of the network */
    uint8_t ssid_length; /**< Length of the SSID. Must be set in place of a null
                              terminator */
    aceWifiMgr_authMode_t auth_mode; /**< The authentication protocol supported
                                          by this network */
    int8_t rssi;        /**< The detected signal level in dBm, also known
                             as the RSSI */
    uint16_t frequency; /**< Frequency */
} aceWifiMgr_detailedScanResultItem_t;

/** Detailed scan results list
 *  List of detailed scan results. Used by
 * aceWifiMgr_getDetailedScanResultList()
 */
typedef struct {
    uint16_t length;
    aceWifiMgr_detailedScanResultItem_t list[ACE_WIFI_MAX_SCAN_RESULTS];
} aceWifiMgr_detailedScanResults_t;

/** Network State
 *  Describes the current state of the network. When registered for network
 *  state change events, the state will only be published at the moment of the
 *  state change. e.g. NET_STATE_DISCONNECTED will only be published if the
 *  device is already connected. Similarly, NET_STATE_CONNECTED will only be
 *  published if the device was not already in the connected state.
 */
typedef enum {
    ACE_WIFI_MGR_NET_STATE_CONNECTING =
        0,                            /**< The device is currently connecting */
    ACE_WIFI_MGR_NET_STATE_CONNECTED, /**< The device is connected */
    ACE_WIFI_MGR_NET_STATE_DISCONNECTING, /**< The device is disconnecting */
    ACE_WIFI_MGR_NET_STATE_DISCONNECTED,  /**< The device is disconnected */
    ACE_WIFI_MGR_NET_STATE_UNKNOWN        /**< The network state is unknown */
} aceWifiMgr_networkState_t;

/** Disconnect reasons
 *  Reason for ACE_WIFI_MGR_NET_STATE_DISCONNECTED event
 */
typedef enum {
    ACE_WIFI_MGR_DISCONNECT_ASSOC_FAILURE = 0, /**< Cannot connect to AP */
    ACE_WIFI_MGR_DISCONNECT_AUTH_FAILURE,      /**< Password, Key etc */
    ACE_WIFI_MGR_DISCONNECT_IP_FAILURE,        /**< Cannot obtain IP address */
    ACE_WIFI_MGR_DISCONNECT_LOCALLY_GENERATED, /**< Locally generated device
                                                  side disconnect */
    ACE_WIFI_MGR_DISCONNECT_UNDEFINED          /**< Undefined reason */
} aceWifiMgr_disconnectReason_t;

/** Captive Portal State
 *  Describes the current state of the captive portal.
 */
typedef enum {
    /** The device has authenticated with captive portal */
    ACE_WIFI_MGR_CAPTIVE_PORTAL_AUTHENTICATED = 0,
    ACE_WIFI_MGR_CAPTIVE_PORTAL_CONNECTED, /**< The device is connected to
                                                captive portal */
    ACE_WIFI_MGR_CAPTIVE_PORTAL_FAILED,    /**< The device has failed to check
                                                captive portal */
    ACE_WIFI_MGR_CAPTIVE_PORTAL_UNKNOWN /**< The captive portal state is unknown
                                           */
} aceWifiMgr_captivePortalState_t;

/** Wi-Fi Connection Info
 *  Describes the currently connected Wi-Fi network, or the disconnection
 *  reason code if the profile is disconnected.
 */
typedef struct {
    /** BSSID of the connected network */
    uint8_t bssid[ACE_WIFI_MGR_BSSID_LEN];
    /** @ATT_META{string-not-term, size: ssid_length} */
    char ssid[ACE_WIFI_MGR_MAX_SSID_LEN]; /**< The network's SSID. NOT a null
                                            terminated string, can be raw bytes
                                            e.g. for unicode */
    uint8_t ssid_length; /**< Length of the SSID. MUST be set instead of a null
                              terminator */
    /** The authentication protocol supported by this network */
    aceWifiMgr_authMode_t auth_mode;
    aceWifiMgr_ipAddress_t ip_address; /**< IP address */
    uint16_t frequency; /**< Frequency of the connected network */
    aceWifiMgr_disconnectReason_t reason; /**< Reason code for a disconnect */
} aceWifiMgr_wifiConnectionInfo_t;

/** Soft AP State
 *  State events types that indicate the state of the soft AP. When registered
 *  for soft AP state change events, the state will only be  published at the
 *  moment of the state change.
 */
typedef enum {
    ACE_WIFI_MGR_SOFTAP_UP = 0,    /**< soft AP created */
    ACE_WIFI_MGR_SOFTAP_DOWN,      /**< soft AP removed */
    ACE_WIFI_MGR_SOFTAP_CONNECTED, /**< a device has connected to the soft AP */
    ACE_WIFI_MGR_SOFTAP_DISCONNECTED /**< a device has disconnected to the soft
                                        AP */
} aceWifiMgr_softAPState_t;

/**
 * @brief Event code describing the type of a Wi-Fi profile update
 */
typedef enum {
    ACE_WIFI_MGR_PROFILE_ADDED = 0, /**< A Wi-Fi profile was added */
    ACE_WIFI_MGR_PROFILE_DELETED,   /**< A Wi-Fi profile was deleted */
    ACE_WIFI_MGR_PROFILE_CHANGED    /**< A Wi-Fi profile was updated */
} aceWifiMgr_profileUpdateEvent_t;

/**
 * @brief Event that indicates that an update occurred on a Wi-Fi profile
 */
typedef struct {
    char ssid[ACE_WIFI_MGR_MAX_SSID_LEN]; /**< The network's SSID. NOT a null
                                            terminated string, can be raw bytes
                                            e.g. for unicode */
    uint8_t ssid_length; /**< Length of the SSID. Must be set in place of
                              a null terminator */
    aceWifiMgr_authMode_t auth_mode;      /**< The authentication protocol
                                               supported by this network */
    aceWifiMgr_profileUpdateEvent_t type; /**< The type of the profile update */
} aceWifiMgr_profileUpdateEventInfo_t;

/**
 * @brief Wi-Fi core statistics data.
 */
typedef struct {
    uint32_t tx_success_count;   /**< No of TX success */
    uint32_t tx_retry_count;     /**< No of TX tx packet retries */
    uint32_t tx_fail_count;      /**< No of TX failure */
    uint32_t rx_success_count;   /**< No of RX success */
    uint32_t rx_crc_error_count; /**< No of RX CRC */
    uint32_t mic_error_count;    /**< No of Mic error count */
    uint32_t nss;                /**< No of spatial streams (MIMO)  */
    int8_t noise;                /**< noise */
    int8_t noise2;               /**< noise stream2 */
    uint16_t phy_rate;           /**< Max phy rate */
    uint16_t tx_rate;            /**< tx rate */
    uint16_t rx_rate;            /**< rx rate */
    int8_t rssi;                 /**< RSSI */
    int8_t rssi2;                /**< RSSI stream2 */
    int8_t brssi;                /**< Beacon RSSI  */
    uint8_t channel;             /**< Channel */
    uint8_t bandwidth;           /**< Bandwidth */
    uint8_t idle_time_per;       /**< Percent of idle time */
    uint8_t signal_bar;          /**< Signal strength bars */
} aceWifiMgr_statisticsData_t;

/**
 * @brief Wi-Fi band.
 */
typedef enum {
    ACE_WIFI_MGR_BAND_2G = 0,     /**< 2.4G */
    ACE_WIFI_MGR_BAND_5G,         /**< 5G */
    ACE_WIFI_MGR_BAND_DUAL,       /**< dual band */
    ACE_WIFI_MGR_BAND_UNSUPPORTED /**< unsupported band */
} aceWifiMgr_band_t;

/**
 * @brief Wi-Fi phymode.
 */
typedef enum {
    ACE_WIFI_MGR_PHY_11B = 0,    /**< 11B */
    ACE_WIFI_MGR_PHY_11G,        /**< 11G */
    ACE_WIFI_MGR_PHY_11N,        /**< 11N */
    ACE_WIFI_MGR_PHY_11AC,       /**< 11AC */
    ACE_WIFI_MGR_PHY_11AX,       /**< 11AX */
    ACE_WIFI_MGR_PHY_UNSUPPORTED /**< Unsupported phy */
} aceWifiMgr_phyMode_t;

/**
 * @brief Wi-Fi bandwidth.
 */
typedef enum {
    ACE_WIFI_MGR_BW_20 = 0,
    ACE_WIFI_MGR_BW_40,
    ACE_WIFI_MGR_BW_80,
    ACE_WIFI_MGR_BW_160,
    ACE_WIFI_MGR_BW_UNSUPPORTED
} aceWifiMgr_bandwidth_t;

/**
 * @brief Wi-Fi device power management modes.
 *
 * Device power management modes supported.
 */
typedef enum {
    ACE_WIFI_MGR_PM_NORMAL = 0, /**< Normal mode. */
    ACE_WIFI_MGR_PM_LOW,        /**< Low Power mode. */
    ACE_WIFI_MGR_PM_ALWAYS_ON,  /**< Always On mode. */
    ACE_WIFI_MGR_PM_UNSUPPORTED /**< Unsupported PM mode. */
} aceWifiMgr_PMMode_t;

/**
 * @brief Wi-Fi capability info.
 */
typedef struct {
    aceWifiMgr_band_t band;           /**< band ex 2.4g/5g/dual */
    aceWifiMgr_phyMode_t phy_mode;    /**< Phymode ex 11b,11g */
    aceWifiMgr_bandwidth_t bandwidth; /**< Bandwidth ex 20M */
    uint32_t max_aggr;                /**< Max aggregation */
    uint16_t supported_features; /**< Supported feature, ex wps/p2p/enterprise,
                                     in bit mask aceWifiMgr_WPS_SUPPORTED|xxx */
} aceWifiMgr_capabilityData_t;

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event registration.
 *
 * Callback function type to receive Wi-Fi ready events
 *
 * @param ctx: client data, passed in by register function.
 */
typedef void (*aceWifiMgr_wifiReadyEventCallback_t)(void* ctx);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event registration.
 *
 * Callback function type to receive scan done events
 *
 * @param ctx: client data, passed in by register function.
 */
typedef void (*aceWifiMgr_scanDoneEventCallback_t)(void* ctx);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event registration.
 *
 * Callback function type to receive soft AP state change events
 *
 * @param ap_state: the current soft AP state
 * @param ctx: client data, passed in by register function.
 */
typedef void (*aceWifiMgr_softAPStateEventCallback_t)(
    aceWifiMgr_softAPState_t ap_state, void* ctx);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event registration.
 *
 * Callback function type to receive Wi-Fi network state change events
 *
 * @param wifi_state: the current Wi-Fi network state
 * @param ctx: client data, passed in by register function.
 * @param wifi_info: info about the current Wi-Fi network
 */
typedef void (*aceWifiMgr_wifiStateEventCallback_t)(
    aceWifiMgr_networkState_t wifi_state,
    aceWifiMgr_wifiConnectionInfo_t wifi_info, void* ctx);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event registration.
 *
 * Callback function type to receive captive portal event
 *
 * @param captive_state Captive portal event
 * @param ctx: client data, passed in by register function.
 */
typedef void (*aceWifiMgr_captivePortalEventCallback_t)(
    aceWifiMgr_captivePortalState_t captive_state, void* ctx);

#if defined(ACE_WIFI_PROFILE_EVENT)
/**
 * @deprecated - Refer to @ref EVENTMGR_API for event registration.
 *
 * Callback function type to receive profile update events
 *
 * @param profile_update_info: The reference to the {@link
 * aceWifiMgr_profileUpdateEventInfo_t}
 * @param ctx: client data, passed in by register function
 */
typedef void (*aceWifiMgr_profileUpdatedEventCallback_t)(
    aceWifiMgr_profileUpdateEventInfo_t profile_update_info, void* ctx);
#endif

/** @} */  // ACE_WIFI_DEF

/**
 * @defgroup ACE_WIFI_API Public API
 * @{
 * @ingroup ACE_WIFI
 */

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Initialize aceWifiMgr to enable API functions
 * This MUST be called before other aceWifiMgr API calls.
 *
 * Note: This function registers an application into the ACS IPC system so that
 * it can communicate with the Wi-Fi service. If the Wi-Fi service is not ready,
 * subsequent calls to the API will fail until it is. Users may call
 * aceWifiMgr_isWifiReady or register Wi-Fi ready event through @ref
 * EVENTMGR_API
 * to check if the Wi-Fi service is up. This should only be an issue on early
 * start if an app starts before the Wi-Fi service.
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_init(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Deinitialize aceWifiMgr and cleanup resources
 * This must be called before exiting. All registered callbacks must also be
 * deregistered separately with the corresponding deregister functions.
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_deinit(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Check if Wi-Fi is enabled and ready
 *
 * @return uint8_t: 1 if Wi-Fi is ready, else 0
 */
uint8_t aceWifiMgr_isWifiReady(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Request a full scan for access points. To receive the scan complete event,
 * register ACE_EVENTMGR_EVENT_SCAN_DONE event through ace event mgr
 * API @ref EVENTMGR_API.
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_startScan(void);

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
 * aceWifiMgr_getScanResultList(), otherwise the list may be empty, since it is
 * not
 * cached long term.
 *
 * @param[in, out] results: Pointer to valid memory of size {@link
 * aceWifiMgr_scanResults_t},
 *                 allocated by the caller.
 *                 Caller can pass in previous scan results or empty results,
 * results->length
 *                 indicates the number of the scan results passed in. Wi-Fi
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
ace_status_t aceWifiMgr_getScanResultList(aceWifiMgr_scanResults_t* results);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get a list of raw scan results, which is not filtered from the original scan
 * results from Wi-Fi driver.
 * To ensure an up to date list, the caller should
 * call aceWifiMgr_startScan() and wait for the scan done event before calling
 * aceWifiMgr_getDetailedScanResultList(), otherwise the list may be empty,
 * since it is not
 * cached long term.
 *
 * @param[out] results: Pointer to valid memory of size {@link
 * aceWifiMgr_detailedScanResults_t},
 *                 allocated by the caller. results->length will be set to the
 *                 total number of scan results returned in results->list.
 *                 If there are no scan results available, the length will be 0.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getDetailedScanResultList(
    aceWifiMgr_detailedScanResults_t* results);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get a list of configured Wi-Fi profiles.
 *
 * @param[out] profiles: Pointer to valid memory of size
 *                  {@link * aceWifiMgr_profileList_t},
 *                  allocated by the caller. profiles->length will be set
 *                  to the total number of configured profiles returned.
 *                  If there are no configured profiles, the length will be 0.
 *                  @Note This list does not contain password info.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getConfiguredProfiles(
    aceWifiMgr_profileList_t* profiles);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Add a new Wi-Fi network profile to the set of configured profiles. This
 * profile
 * will not be automatically saved -- to save it to file, call {@link
 * aceWifiMgr_saveConfig}.
 *
 * @param profile: Wi-Fi profile to be added. Must be allocated and populated by
 * the caller.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_addProfile(aceWifiMgr_profile_t* profile);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Remove the specified Wi-Fi profile from the list of the configured profiles.
 * This
 * change will not be automatically saved -- to save it to file, call {@link
 * aceWifiMgr_saveConfig}.
 *
 * @param ssid: Wi-Fi profile to be removed.
 * @param auth_mode: The auth mode of the Profile.
 *        When set to ACE_WIFI_MGR_AUTH_UNKNOWN, then any auth mode can apply.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_removeProfile(const char* ssid,
                                      aceWifiMgr_authMode_t auth_mode);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Save all the currently configured profiles to file.
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_saveConfig(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Enable all configured Wi-Fi profiles -- allow auto connect
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_enableAllNetworks(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Disable all configured Wi-Fi network profiles. This will also disconnect from
 * the
 * currently connected network. The disconnected state will be published in
 * aceWifiMgr_networkState_t for callbacks registered for network state change
 * events ACE_EVENTMGR_GROUP_WIFI_STATE, through @ref EVENTMGR_API.
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_disableAllNetworks(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Enable and connect to a specified Wi-Fi profile. The connection status will
 * be
 * published in aceWifiMgr_networkState_t for callbacks registered for network
 * state change events ACE_EVENTMGR_GROUP_WIFI_STATE, through ace event mgr
 * API @ref EVENTMGR_API.
 *
 * @param ssid: The SSID to connect to.
 * @param auth_mode: The auth mode of the AP to connect to.
 *        When set to ACE_WIFI_MGR_AUTH_UNKNOWN, then any auth mode can apply.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_connect(const char* ssid,
                                aceWifiMgr_authMode_t auth_mode);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Disconnect current connected network. The connection status will be
 * published in aceWifiMgr_networkState_t for callbacks registered for network
 * state change events ACE_EVENTMGR_GROUP_WIFI.
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_disconnect(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Create a soft AP. Callbacks can be registered through @ref EVENTMGR_API
 * to receive event ACE_EVENTMGR_GROUP_WIFI_SOFTAP_STATE when the AP is created
 * or removed.
 *
 * @param ssid: SSID of the AP to be created.
 *              The HAL may define a default SSID if NULL is given. Otherwise a
 * valid string
 *              is required.
 * @param password: Predefined password. Can be NULL for an open AP.
 * @param frequency: AP frequency, or 0.
 *              If given as 0, the HAL is expected to pick a default frequency.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_createSoftAP(const char* ssid, const char* password,
                                     uint16_t frequency);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Remove the current soft AP. Callbacks registered through @ref EVENTMGR_API
 * to receive event ACE_EVENTMGR_GROUP_WIFI_SOFTAP_STATE when the AP is created
 * or removed.
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_removeSoftAP(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the current Wi-Fi network state.
 *
 * @return aceWifiMgr_networkState_t: the current Wi-Fi network state
 */
aceWifiMgr_networkState_t aceWifiMgr_getNetworkState(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the currently connected network's Wi-Fi info. The caller must allocate
 * enough space for {@link aceWifiMgr_wifiConnectionInfo_t}. If the network is
 not currently connected,
 * this function will return @ref ACE_STATUS_GENERAL_ERROR.
 *
 * @param[out] ret_conn_info: Pointer to an allocated {@link
 aceWifiMgr_wifiConnectionInfo_t},
                              to be populated by aceWifiMgr.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 failure
 */
ace_status_t aceWifiMgr_getCurrentConnectionInfo(
    aceWifiMgr_wifiConnectionInfo_t* ret_conn_info);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the currently connected network's ip info. The caller must allocate
 * enough space for {@link aceWifiMgr_ipConfiguration_t}.
 *
 * @param[out] ret_ip_info: Pointer to an allocated
 * aceWifiMgr_ipConfiguration_t, to be populated by aceWifiMgr.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getIpConfigInfo(
    aceWifiMgr_ipConfiguration_t* ret_ip_info);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the Wi-Fi statistic info. The caller must allocate enough space
 * for {@link aceWifiMgr_statisticsData_t}.
 *
 * @param[out] ret_stat_info: Pointer to an allocated
 * aceWifiMgr_statisticsData_t, to be populated by aceWifiMgr.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getStatisticsDataInfo(
    aceWifiMgr_statisticsData_t* ret_stat_info);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the Wi-Fi capability info. The caller must allocate enough space
 * for {@link aceWifiMgr_capabilityData_t}.
 *
 * @param[out] ret_cap_info: Pointer to an allocated
 * aceWifiMgr_capabilityData_t,
 *                         to be populated by aceWifiMgr.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getCapabilityDataInfo(
    aceWifiMgr_capabilityData_t* ret_cap_info);

#if defined(ACE_WIFI_TCP_TUNNEL)
/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Enable TCP tunnel
 *
 * @param src_ip: Source IP - IPV4 dot notation e.g. 192.168.1.1
 * @param src_port: Source port
 * @param dest_ip: Destination IP - IPV4 dot notation e.g. 192.168.1.1
 * @param dest_port: Destination port
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_enableTcpTunnel(const char* src_ip, uint16_t src_port,
                                        const char* dest_ip,
                                        uint16_t dest_port);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Disable TCP tunnel
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_disableTcpTunnel(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Enable NAT
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_enableNAT(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Disable NAT
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_disableNAT(void);
#endif

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Evaluate if the current network is in a captive portal. This is an async
 * call.
 * The captive portal status will be published in
 * aceWifiMgr_captivePortalState_t
 * for any callbacks registered with event
 * ACE_EVENTMGR_GROUP_CAPTIVE_PORTAL_STATE
 * through @ref EVENTMGR_API.
 * This function should only be called if the device is connected to a network,
 * otherwise the check will fail.
 * Note:
 *     Be extra cause when using this API. It should be only used to check for
 * captive portal status.
 * it is too heavy to be used to detect if internet is connected.
 *
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_evaluateCaptive(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the current wifi captive portal state.
 *
 * @return aceWifiMgr_captivePortalState_t: current wifi captive portal state
 */
aceWifiMgr_captivePortalState_t aceWifiMgr_getCaptivePortalState(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the mac address
 *
 * @param[out] address: @ATT_META{string} this will be set to the mac address in
 * the format XX:XX:XX:XX:XX:XX with null terminator
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getMacAddress(aceWifiMgr_macAddress_t address);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Set the country code
 *
 * @param code: @ATT_META{string} country code to set, must be a 2-3 char null
 * terminated string
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_setCountryCode(const char* code);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get the country code
 *
 * @param country: fixed length buffer to hold the country code
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure
 */
ace_status_t aceWifiMgr_getCountryCode(aceWifiMgr_countryCode_t country);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Get curent acs Wi-Fi log level
 *
 * @return aceWifiMgr_logLevel_t: this should be consistent with ACE log levels
 */
aceWifiMgr_logLevel_t aceWifiMgr_getLogLevel(void);

/**
 * @ATT_API{ACE_WIFI ACE_TEST}
 * Set desired acs Wi-Fi log level
 *
 * @param priority: this should be consistent with ACE log levels
 */
void aceWifiMgr_setLogLevel(aceWifiMgr_logLevel_t priority);

#if defined(ACE_WIFI_PROFILE_EVENT)
/**
 * @ATT_API{ACE_WIFI_ACE_TEST}
 *
 * This method can be used to get the string representation of a
 * {@link aceWifiMgr_profileUpdateEvent_t} object
 *
 * @param type: The type of profile update event
 * @param typeStr: The character array to hold the string representation. Must
 * be allocated by the caller and passed in
 * @param len: The length of the {@link typeStr}. If the length is insufficient
 * to hold the string representation, an error is returned and the parameter
 * is set to the minimum length required. The minimum length required is
 * {@link k_profileUpdateEventAsStrMinLength} + 1
 * @return ace_status_t: ACE_STATUS_OK on success, the corresponding
 * error code on failure
 */
ace_status_t aceWifiMgr_getProfileUpdateEventAsStr(
    aceWifiMgr_profileUpdateEvent_t type, char* typeStr, size_t* len);
#endif

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 ACE_EVENTMGR_EVENT_WIFI_READY
 *               registration.
 *
 * Register a callback function for Wi-Fi ready events.
 * This API is allowed to be used even before Wi-Fi svc is ready.
 * Note: inside callback, do not call any Wi-Fi API, do not do heavy work.
 *       Calling Wi-Fi API inside the callback could cause deadlock in Wi-Fi
 svc.
 *
 * @param event_callback: callback to register
 * @param ctx: client data, will be passed back in the callback function, can be
 NULL.
               Note: Client needs to keep this data pointer not being released.
 * @return ace_status_t: Return ACE_STATUS_OK on registration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_registerWifiReadyEvent(
    aceWifiMgr_wifiReadyEventCallback_t event_callback, void* ctx);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 * ACE_EVENTMGR_EVENT_WIFI_READY
 *               deregistration.
 *
 * Deregister a callback function from Wi-Fi ready events.
 *
 * @param event_callback: callback to deregister
 * @return ace_status_t: Return ACE_STATUS_OK on deregistration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_deregisterWifiReadyEvent(
    aceWifiMgr_wifiReadyEventCallback_t event_callback);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 *               ACE_EVENTMGR_EVENT_SCAN_DONE registration.
 *
 * Register a callback function for scan done events.
 * This API is allowed to be used even before Wi-Fi svc is ready.
 * Note: inside callback, do not do heavy work, do not call any Wi-Fi API
 including
 *       deregister. Calling Wi-Fi API inside the callback could cause deadlock
 in Wi-Fi svc.
 *
 * @param event_callback: callback to register
 * @param ctx: client data, will be passed back in the callback function, can be
 NULL.
               Note: Client needs to keep this data pointer not being released.
 * @return ace_status_t: Return ACE_STATUS_OK on registration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_registerScanDoneEvent(
    aceWifiMgr_scanDoneEventCallback_t event_callback, void* ctx);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 *               ACE_EVENTMGR_EVENT_SCAN_DONE deregistration.
 *
 * Deregister a callback function from scan done events.
 *
 * @param event_callback: callback to deregister
 * @return ace_status_t: Return ACE_STATUS_OK on deregistration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_deregisterScanDoneEvent(
    aceWifiMgr_scanDoneEventCallback_t event_callback);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 *               ACE_EVENTMGR_GROUP_WIFI_SOFTAP_STATE registration.
 *
 * Register a callback function for soft AP state change events.
 * This API is allowed to be used even before Wi-Fi svc is ready.
 * Note: inside callback, do not do heavy work, do not call any Wi-Fi API
 including
 *       deregister. Calling Wi-Fi API inside the callback could cause deadlock
 *       in Wi-Fi svc.
 *
 * @param event_callback: callback to register
 * @param ctx: client data, will be passed back in the callback function, can be
 NULL.
               Note: Client needs to keep this data pointer not being released.
 * @return ace_status_t: Return ACE_STATUS_OK on registration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_registerSoftAPStateEvent(
    aceWifiMgr_softAPStateEventCallback_t event_callback, void* ctx);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 *               ACE_EVENTMGR_GROUP_WIFI_SOFTAP_STATE deregistration.
 *
 * Deregister a callback function from soft AP state change events.
 *
 * @param event_callback: callback to deregister
 * @return ace_status_t: Return ACE_STATUS_OK on deregistration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_deregisterSoftAPStateEvent(
    aceWifiMgr_softAPStateEventCallback_t event_callback);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 *               ACE_EVENTMGR_GROUP_WIFI_STATE registration.
 *
 * Register a callback function for Wi-Fi network state change events.
 * This API is allowed to be used even before Wi-Fi svc is ready.
 * Note: inside callback, do not do heavy work, do not call any Wi-Fi API
 including
 *       deregister. Calling Wi-Fi API inside the callback could cause deadlock
 in Wi-Fi svc.
 *
 * @param event_callback: callback to register
 * @param ctx: client data, will be passed back in the callback function, can be
 NULL.
               Note: Client needs to keep this data pointer not being released.
 * @return ace_status_t: Return ACE_STATUS_OK on registration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_registerWifiStateEvent(
    aceWifiMgr_wifiStateEventCallback_t event_callback, void* ctx);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 *               ACE_EVENTMGR_GROUP_WIFI_STATE deregistration.
 *
 * Deregister a callback function from Wi-Fi network state change events.
 *
 * @param event_callback: callback to deregister
 * @return ace_status_t: Return ACE_STATUS_OK on deregistration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_deregisterWifiStateEvent(
    aceWifiMgr_wifiStateEventCallback_t event_callback);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 *               ACE_EVENTMGR_GROUP_CAPTIVE_PORTAL_STATE registration.
 *
 * Register a callback function for captive portal events.
 * This API is allowed to be used even before Wi-Fi svc is ready.
 * Note: inside callback, do not do heavy work, do not call any Wi-Fi API
 including
 *       deregister. Calling Wi-Fi API inside the callback could cause deadlock
 in Wi-Fi svc.
 *
 * @param event_callback: callback to register
 * @param ctx: client data, will be passed back in the callback function, can be
 NULL.
               Note: Client needs to keep this data pointer not being released.
 * @return ace_status_t: Return ACE_STATUS_OK on registration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_registerCaptivePortalEvent(
    aceWifiMgr_captivePortalEventCallback_t event_callback, void* ctx);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 *               ACE_EVENTMGR_GROUP_CAPTIVE_PORTAL_STATE deregistration.
 *
 * Deregister a callback function from captive portal events.
 *
 * @param event_callback: callback to deregister
 * @return ace_status_t: Return ACE_STATUS_OK on deregistration success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_deregisterCaptivePortalEvent(
    aceWifiMgr_captivePortalEventCallback_t event_callback);

#if defined(ACE_WIFI_PROFILE_EVENT)
/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 *               ACE_EVENTMGR_GROUP_WIFI_PROFILE_UPDATED registration.
 *
 * Register a callback function for {@link aceWifiMgr_profileUpdateEvent_t}
 * events.
 * This API is allowed to be used even before Wi-Fi svc is ready.
 * Note: inside callback, do not do heavy work, do not call any Wi-Fi API
 * including
 *       deregister. Calling Wi-Fi API inside the callback could cause deadlock
 * in Wi-Fi svc.
 *
 * @param event_callback: The callback to register
 * @param ctx: The client data to be passed in when the callback is invoked, can
 * be NULL.
 * @return ace_status_t: Return ACE_STATUS_OK on registration
 * success, an error code in case of failure
 */
ace_status_t aceWifiMgr_registerProfileUpdatedEvent(
    aceWifiMgr_profileUpdatedEventCallback_t event_callback, void* ctx);

/**
 * @deprecated - Refer to @ref EVENTMGR_API for event
 *               ACE_EVENTMGR_GROUP_WIFI_PROFILE_UPDATED deregistration.
 *
 * Deregister a callback function for {@link aceWifiMgr_profileUpdateEvent_t}
 * events
 *
 * @param event_callback: The callback to deregister
 * @return ace_status_t: Return ACE_STATUS_OK on deregistration
 * success, an error code in case of failure
 */
ace_status_t aceWifiMgr_deregisterProfileUpdatedEvent(
    aceWifiMgr_profileUpdatedEventCallback_t event_callback);
#endif

#if defined(ACE_WIFI_PROFILE_UPDATE)
#if defined(ACE_WIFI_IEEE8021X)
/**
 * Verifies that any Wi-Fi keystore slot has a corresponding
 * network configuration of wpa_supplicant. Removes abandoned slots.
 *
 * @return ace_status_t: Return ACE_STATUS_OK on backup success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_synchronizeConfig(void);
#endif

/**
 * Restore original Wi-Fi network profile.
 *
 * @param ssid: Wi-Fi profile to be restored.
 * @param auth_mode: The auth mode of the Profile.
 *        When set to ACE_WIFI_MGR_AUTH_UNKNOWN, then any auth mode can apply.

 * @return ace_status_t: Return ACE_STATUS_OK on backup success,
 *                             ACE_STATUS_GENERAL_ERROR on failure.
 */
ace_status_t aceWifiMgr_restoreProfile(const char* ssid,
                                       aceWifiMgr_authMode_t auth_mode);

/**
 * Update existing Wi-Fi network profile. This profile
 * will not be automatically saved -- to save it to file, call
 * aceWifiMgr_saveConfig().
 *
 * @param config: Wi-Fi configuration to be added. Must be allocated and
 * populated by the caller.
 * @return ace_status_t: ACE_STATUS_OK on success, ACE_STATUS_GENERAL_ERROR on
 * failure.
 */
ace_status_t aceWifiMgr_updateProfile(aceWifiMgr_profile_t* profile);

#endif  // ACE_WIFI_PROFILE_UPDATE

/** @} */  // ACE_WIFI_API

// Temporary build flag to control if Wi-Fi mgr legacy
// structure or API are allowed to be used.
// Right now, Wi-Fi mgr and Wi-Fi svc code have all
// changed to new structure/API, do not allow
// legacy to be used anymore.
#ifndef WIFI_MW_NO_LEGACY
// Legacy Wi-Fi mgr API, to be removed
#include <ace/wifi_mgr_legacy.h>
#endif

#ifdef __cplusplus
}
#endif

#endif  // __ACE_WIFI_MGR_H__
