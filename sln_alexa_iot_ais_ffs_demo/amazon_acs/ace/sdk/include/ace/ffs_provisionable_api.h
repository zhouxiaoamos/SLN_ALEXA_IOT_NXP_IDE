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
 * @file aceFfs_provisionable_api.h
 * @brief ACS FFS Provisionable APIs
 * @details Contains APIs that can be used to control start/stop FFS
 *          provisionable process.
 */

#ifndef _ACEFFS_PROVISIONABLE_H_
#define _ACEFFS_PROVISIONABLE_H_

#include <ace/ace_status.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup FFS_DATA FFS client data types
 * @details Contains the FFS API data types
 * @{
 * @ingroup FFS
 */

/**
 * FFS provisioning mode
 */
typedef enum {
    /** Zero Touch Setup. */
    ACE_FFS_MODE_ZTS = 1 << 0,
    /** 2D Barcode Setup. */
    ACE_FFS_MODE_2D = 1 << 1,
    /** User Guided Setup. */
    ACE_FFS_MODE_UGS = 1 << 2,
} aceFfs_mode_t;

/**
 * FFS provisioning states
 */
typedef enum {
    /** Provisionable not initialized. */
    ACE_FFS_PROV_NOT_INIT = 0,
    /** Provisionable idle. */
    ACE_FFS_PROV_IDLE = 1,
    /** Beaconing, waiting for provisioner. */
    ACE_FFS_PROV_WAITING_FOR_PROVISIONER = 2,
    /** Provisioner authorizing phase. */
    ACE_FFS_PROV_AUTHORIZING_PROVISIONER = 3,
    /** Provisioner provisioning. */
    ACE_FFS_PROV_ACTIVELY_PROVISIONING = 4,
    /** Provisioning is complete (device is not registered). */
    ACE_FFS_PROV_PROVISIONING_COMPLETE = 5,
    /** Provisioning is terminated (device may/may-not be registered). */
    ACE_FFS_PROV_PROVISIONING_TERMINATED = 6,
    /** Device is registering. */
    ACE_FFS_PROV_DEVICE_REGISTERING = 7,
    /** Device is registered. */
    ACE_FFS_PROV_DEVICE_REGISTERED = 8,
    /** Provisionable not started. */
    ACE_FFS_PROV_STOPPED = 9,
} aceFfs_provisioningState_t;

/**
 * FFS provisionable shutdown reason
 */
typedef enum {
    /** No reason. */
    ACE_FFS_NONE = 0,
    /**
     * Provisioner has sent over all information required for device
     * registration. This does not indicate the device is registered.
     */
    ACE_FFS_PROVSIONING_COMPLETE = 1,
    /** Beaconing error. */
    ACE_FFS_ADV_ERROR = 2,
    /** Beaconing timeout (no provisioner has connected). */
    ACE_FFS_ADV_WDG_TIMEOUT = 3,
    /** Forced shutdown. */
    ACE_FFS_APP_FORCED_SHUTDOWN = 4,
    /** Unrecoverable fatal error. */
    ACE_FFS_FATAL_ERROR = 5,
} aceFfs_shutdownReason_t;

/**
 * FFS provisionable Wifi reason codes
 */
typedef enum {
    /** No reason. */
    ACE_FFS_WIFI_NO_REASON = 0,
    /** Fail to associate with AP. */
    ACE_FFS_WIFI_FAIL_AP_NOT_FOUND = 1,
    /** Fail to authenticate with AP (PSK error possibly). */
    ACE_FFS_WIFI_FAIL_AUTH_FAILED = 2,
    /** Fail to get IP (IP configuration related failures). */
    ACE_FFS_WIFI_FAIL_IP_FAILURE = 3,
    /** Connected to captive portal, but not authenticated yet. */
    ACE_FFS_WIFI_BEHIND_CAPTIVE_PORTAL = 4,
    /** Fail due to public network not reachable. */
    ACE_FFS_WIFI_FAIL_LIMITED_CONNECTIVITY = 5,
    /** Fail due to internal unknown error. */
    ACE_FFS_WIFI_FAIL_INTERNAL_ERROR = 6,
} aceFfs_wifiReason_t;

/**
 * FFS provisionable WiFi state
 */
typedef enum {
    /** Wifi is idle. */
    ACE_FFS_WIFI_IDLE = 0,
    /** Wifi disconnected. */
    ACE_FFS_WIFI_DISCONNECTED = 1,
    /** Wifi connected. This means network is operational. */
    ACE_FFS_WIFI_CONNECTED = 2,
    /** Wifi connecting. */
    ACE_FFS_WIFI_CONNECTING = 3,
} aceFfs_wifiState_t;

/** @brief Wi-Fi security protocol enumeration.
 */
typedef enum {
    ACE_FFS_WIFI_SECURITY_PROTOCOL_NONE = 0,     //!< Open network.
    ACE_FFS_WIFI_SECURITY_PROTOCOL_WPA_PSK = 1,  //!< WPA.
    ACE_FFS_WIFI_SECURITY_PROTOCOL_WEP = 2,      //!< WEP.
    ACE_FFS_WIFI_SECURITY_PROTOCOL_OTHER = 3,    //!< Other protocol.
    ACE_FFS_WIFI_SECURITY_PROTOCOL_UNKNOWN = 4   //!< Unknown protocol.
} aceFfs_wifiProtocol_t;

/** Max Wifi SSID name length */
#define ACE_FFS_MAX_SSID_LEN 32

/**
 * FFS provisionable Wifi network state/information.
 * (mandatory for Wifi Reconnect mode (Frustration Free Reconnect(FFR))
 * used in @ref aceFfs_startConfig_t)
 */
typedef struct {
    /**
     * Last Wifi network state reason for entering Wifi reconnect. Should be
     * one of the network error conditions in @ref aceFfs_wifiReason_t. Please
     * specify one that is best matched to the underlying Wifi error condition.
     * @ref ACE_FFS_WIFI_FAIL_AP_NOT_FOUND
     * @ref ACE_FFS_WIFI_FAIL_AUTH_FAILED
     * @ref ACE_FFS_WIFI_FAIL_IP_FAILURE
     * @ref ACE_FFS_WIFI_BEHIND_CAPTIVE_PORTAL
     * @ref ACE_FFS_WIFI_FAIL_LIMITED_CONNECTIVITY
     * @ref ACE_FFS_WIFI_FAIL_INTERNAL_ERROR
     */
    aceFfs_wifiReason_t last_network_state;

    /**
     * The network's SSID. Not required to be a null terminated string,
     * can be raw bytes e.g. for unicode
     */
    char ssid[ACE_FFS_MAX_SSID_LEN];

    /** Length of the SSID. Must be set. Length excluding null byte */
    size_t ssid_length;

    aceFfs_wifiProtocol_t security_protocol;  //!< Network security type.
} aceFfs_wifiNetworkInfo_t;

/**
 * FFS provisionable start API configuration structure
 * @note This structure should be treated as opaque. Client should use the
 * setter APIs (@ref aceFfs_initStartConfig,
 * @ref aceFfs_setStartConfigNetworkInfo) to initialize and set the specific
 * fields.
 */
typedef struct {
    uint16_t internal_flags; /* @private */

    /**
     * @private
     * Network Information is required for Wifi stress/reconnect.
     */
    aceFfs_wifiNetworkInfo_t wifiNetworkInfo;
} aceFfs_startConfig_t;

/** Default advertisement timeout (in seconds) */
#define ACE_FFS_DEFAULT_ADVERTISING_TIMEOUT 604800

/** Default Provisioning timeout (in seconds). */
#define ACE_FFS_DEFAULT_PROVISIONING_TIMEOUT 300

/**
 * FFS provisionable initialization configuration parameters
 * @warning Important note about the callbacks. Do not call
 * aceFfs_stop() and aceFfs_deinit() from the callbacks as this can cause
 * deadlock.  If these routines need to be called, be sure to do it outside the
 * context of any of the callbacks.
 * @note The return code of the callbacks are not used for now. It is
 * recommended that ACE_STATUS_OK is returned. It might be used in future to
 * allow early termination.
 *
 * All the callbacks have an @ref ace_status_t return type.
 * The callbacks are separated into two categories:
 * 1. Provision status callbacks (connection indication, registration status,
 * provisioning stopped).
 * 2. Provision information callbacks. The provisioner sent over device
 * configuration information such as locale, country code, time stamp, and
 * registration token during provisioning. These callbacks provide the
 * opportunity for user to act on them if needed.
 *
 * All callbacks are optional but it is generally useful to register for BLE
 * connection indication, registration, and stopped callbacks. For instance,
 * registration complete callback is a good place for application to signal
 * other subsystems (i.e., connect to Alexa, etc). To monitor provisioner
 * connection status, the ble_provisioner_connected() and
 * ble_provisioner_disconnected() are useful. To monitor internal FFS
 * provisionable task status, stopped() callback can be used.
 */
typedef struct {
    /** Opaque context pointer to the callbacks. */
    void* ctx;

    /* Configuration parameters */
    /**
     * Advertisement timeout (in seconds).
     * Default timeout is @ref ACE_FFS_DEFAULT_ADVERTISING_TIMEOUT seconds
     * which can be specified with value 0.
     */
    uint32_t advertising_timeout;

    /**
     * Provisioning timeout (in seconds).
     * Default timeout is @ref ACE_FFS_DEFAULT_PROVISIONING_TIMEOUT seconds
     * which can be specified with value 0.
     */
    uint32_t provisioner_timeout;

    /**
     * This indicates that client will defer its registration completion
     * callback to different task/thread and can take some time to
     * complete. We will wait for the final completion signal from
     * @ref aceFfs_sendCompletion. @ref registration_complete callback is
     * required if defer mode is enabled.
     * @note This is the preferred option of execution if one is unsure about
     * about its post registration callback's execution time.
     */
    bool defer_registration_completion;

    /* BLE connection manager callbacks. */
    /**
     * Provisioner is connected indication.
     * @param[in] ctx. opaque context pointer
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*ble_provisioner_connected)(void* ctx);

    /**
     * Provisioner is disconnected indication.
     * @param[in] ctx. opaque context pointer
     * @param[in] reason is not used right now.
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*ble_provisioner_disconnected)(void* ctx, int reason);

    /* Provisioning state callbacks. */
    /**
     * Provisioning complete. This indicates provisioner has sent
     * all information required for device registration.
     * @param[in] ctx. opaque context pointer
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*provisioning_complete)(void* ctx);

    /**
     * Current provisioner session is terminated (i.e., due to
     * provisioner disconnect, etc).
     * @param[in] ctx. opaque context pointer
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*provisioning_terminated)(void* ctx);

    /**
     * Current provisioning mode (UGS/2D/ZTS)
     * @param[in] ctx. opaque context pointer
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*state_actively_provisioning)(void* ctx, aceFfs_mode_t mode);

    /* Provisioner information callbacks (The following info are passed by the
     * provisioner). */
    /**
     * Registration token used. This is generated/sent by provisioner. This is
     * more for information purpose.
     * @param[in] ctx. opaque context pointer
     * @param[in] reg_token. registration token
     * @param[in] len. length of token
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*set_reg_token)(void* ctx, const char* reg_token, size_t len);

    /**
     * Realm information sent by the provisioner.
     * @param[in] ctx. opaque context pointer
     * @param[in] realm. realm key (ex. USAmazon)
     * @param[in] len. length of realm
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*set_realm_key)(void* ctx, const char* realm, size_t len);

    /**
     * Country code information sent by the provisioner.
     * @param[in] ctx. opaque context pointer
     * @param[in] CC. country code (ISO3166-1 alpha-2)
     * @param[in] len. length of country code
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*set_country_code)(void* ctx, const char* CC, size_t len);

    /**
     * Language locale sent by the provisioner.
     * @param[in] ctx. opaque context pointer
     * @param[in] locale. Configuration for locale setting, like "en-US", a
     * language-CountryCode combination, following standard ISO 639-1 for
     * language and ISO 3166-2 for country code.
     * @param[in] len. length of locale
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*set_language_locale)(void* ctx, const char* locale,
                                        size_t len);

    /**
     * Market place sent by the provisioner.
     * @param[in] ctx. opaque context pointer
     * @param[in] marketplace.
     * @param[in] len. length of marketplace
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*set_marketplace)(void* ctx, const char* marketplace,
                                    size_t len);

    /**
     * UTC time sent by the provisioner.
     * @param[in] ctx. opaque context pointer
     * @param[in] utc_time. ex. 2019-12-18 19:30:3
     * @param[in] len. length of utc_time
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*set_utc_time)(void* ctx, const char* utc_time, size_t len);

    /**
     * Device setup type sent by the provisioner. This is to differentiate set
     * up mode (retail vs normal, etc). Most likely not useful for 3P use cases.
     * @param[in] ctx. opaque context pointer
     * @param[in] type. device setup type.
     * @param[in] len. length of type
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*set_dev_setup_type)(void* ctx, const char* type, size_t len);

    /**
     * Registration status indication. This allows the application to perform
     * post registration operation(s) after registration is completed (success
     * or fail). In case of registration success, the application can inform
     * other services to establish cloud service connection for instance.
     *
     * Please do not perform long blocking operations in this callback.
     * If one needs to do time consuming operations in post-processing, please
     * use the async/deferred model as explained below. In addition, if the
     * application wishes to override the final FFS completion status in which
     * certain post registration operation fails, the defer mechanism can be
     * used for this purpose as well to allow final completion status override.
     *
     * Deferred registration completion can be requested through the
     * @ref defer_registration_completion during @aceFfs_init. This informs
     * the middleware not to return success or failure immediately after the
     * @ref registration_complete callback is triggered. Instead, it will wait
     * for client's signal from @ref aceFfs_sendCompletion. This will also
     * provide the client the control to decide the overall FFS provisioning
     * success. If deferred mode is requested, client must call
     * @aceFfs_sendCompletion to finish the FFS provisioning.
     *
     * Note that if registration has failed, the client will not be able to
     * override the final FFS completion status to success.
     *
     * @param[in] ctx. opaque context pointer
     * @param[in] success. @ACE_STATUS_OK on registered successfully, negative
     * on error.
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*registration_complete)(void* ctx, int success);

    /**
     * Provisioning stopped indication.
     * @param[in] ctx. opaque context pointer
     * @param[in] reason. See @ref aceFfs_shutdownReason_t
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*stopped)(void* ctx, int reason);

    /* Net event callbacks. */
    /**
     * This callback gets called for Wifi connect state reporting.
     * @note In Wifi stress/FFR mode, this will be the "completion" triggered
     * with state set to @ref ACE_FFS_WIFI_CONNECTED and reason set to @ref
     * ACE_FFS_WIFI_NO_REASON.
     * @param[in] ctx. opaque context pointer
     * @param[in] state. See @ref aceFfs_wifiState_t
     * @param[in] reason. See @ref aceFfs_wifiReason_t
     * @return ACE_STATUS_OK on success, failure code otherwise.
     */
    ace_status_t (*wifi_connect_state)(void* ctx, aceFfs_wifiState_t state,
                                       aceFfs_wifiReason_t reason);
} aceFfs_config_t;

/** @} */

/**
 * @defgroup FFS_API FFS Provisionable APIs
 * @{
 * @ingroup FFS
 */

/**
 * @brief Initialize the FFS provisionable subsystem.
 *
 * @param[in] config, configuration parameters. This is optional.
 * If no configuration is supplied, default advertisement and provisioning
 * timeout will be used which is @ref ACE_FFS_DEFAULT_ADVERTISING_TIMEOUT
 * If defer mode (@ref defer_registration_completion) is enabled,
 * @ref registration_completion callback must be provided.
 * seconds and @ref ACE_FFS_DEFAULT_PROVISIONING_TIMEOUT seconds respectively.
 * @return ACE_STATUS_OK if everything goes well.
 * @return ACE_STATUS_GENERAL_ERROR for other errors.
 */
ace_status_t aceFfs_init(aceFfs_config_t* config);

/**
 * @brief Initialize the FFS start configuration structure.
 *
 * @param[in] start_config, start configuration pointer.
 * @return ACE_STATUS_OK if everything goes well.
 * @return ACE_STATUS_BAD_PARAM for other errors.
 */
ace_status_t aceFfs_initStartConfig(aceFfs_startConfig_t* start_config);

/**
 * @brief Set the network information to the FFS start configuration structure.
 *
 * @param[in, out] start_config, start configuration pointer.
 * @param[in] network_info, network information
 * @return ACE_STATUS_OK if everything goes well.
 * @return ACE_STATUS_BAD_PARAM for other errors.
 */
ace_status_t aceFfs_setStartConfigNetworkInfo(
    aceFfs_startConfig_t* start_config,
    const aceFfs_wifiNetworkInfo_t* network_info);

/**
 * @brief Start FFS Provisioning. This starts the BLE device beaconing
 * and awaits for provisioner connection. This API is re-entrant.
 *
 * @param mode FFS provision modes (Add the individual modes together to form
 * combinations. i.e., ACE_FFS_MODE_ZTS | ACE_FFS_MODE_UGS)
 * @return ACE_STATUS_OK if everything goes well.
 * @return ACE_STATUS_UNINITIALIZED if not initialized.
 * @return ACE_STATUS_BAD_PARAM if invalid mode is provided.
 * @return ACE_STATUS_GENERAL_ERROR if start failed.
 */
ace_status_t aceFfs_start(aceFfs_mode_t mode);

/**
 * @brief Start FFS Provisioning (extended). This API is similar to
 * @ref aceFfs_start but allows further control over FFS start behavior. For
 * instance, start FFS in Wifi stress mode. Wifi reconnect mode only FFS is also
 * known as Frustration Free Reconnect (FFR). This API is re-entrant.
 *
 * @code
 * // Starting FFS in Wifi stress/reconfiguration mode for ZTS
 * aceFfs_startConfig_t sparam;
 * aceFfs_wifiNetworkInfo_t ninfo = {0};
 * ace_status_t status = aceFfs_initStartConfig(&sparam);
 *
 * // Fill in the network info into &ninfo
 * ninfo.last_network_state = ACE_FFS_WIFI_FAIL_AUTH_FAILED;
 * strncpy(ninfo.ssid, "TEST_AP", sizeof(ninfo.ssid) - 1);
 * ninfo.ssid_length = strlen(TEST_AP);
 *
 * status = aceFfs_setStartConfigNetworkInfo(&sparam, &ninfo);
 * status = aceFFS_startEx(ACE_FFS_MODE_ZTS, &sparam);
 * @endcode
 *
 * @param start_config FFS start configuration pointer (i.e start FFS in Wifi
 * stress mode). The parameters will be copied internally if specified.
 * @param mode FFS provision modes (Add the individual modes together to form
 * combinations. i.e., ACE_FFS_MODE_ZTS | ACE_FFS_MODE_UGS).
 * @note aceFfs_startEx(mode, NULL) is equivalent to aceFfs_start(mode)
 * @return ACE_STATUS_OK if everything goes well.
 * @return ACE_STATUS_UNINITIALIZED if not initialized.
 * @return ACE_STATUS_BAD_PARAM if invalid mode is provided.
 * @return ACE_STATUS_GENERAL_ERROR if start failed.
 */
ace_status_t aceFfs_startEx(aceFfs_mode_t mode,
                            const aceFfs_startConfig_t* start_config);

/**
 * @brief Stop FFS Provisioning. This will do a force stop of the provisionable
 * workflow regardless of which stage it is in. This API is re-entrant. This is
 * a synchronous operation to tear down FFS provisionable subsystem.
 * @warning Do not call this from FFS's callback context. It will deadlock.
 *
 * @return ACE_STATUS_OK if everything goes well.
 * @return ACE_STATUS_UNINITIALIZED if not initialized.
 * @return ACE_STATUS_GENERAL_ERROR if stop failed.
 */
ace_status_t aceFfs_stop(void);

/**
 * @brief De-initialize the FFS subsystem. If the provisionable is started,
 * it will do a stop first. This API is re-entrant. This is a synchronous
 * operation to tear down FFS provisionable subsystem.
 * @warning Do not call this from FFS's callback context. It will deadlock.
 *
 * @return ACE_STATUS_OK if everything goes well.
 * @return ACE_STATUS_UNINITIALIZED if not initialized.
 */
ace_status_t aceFfs_deinit(void);

/**
 * @brief Get FFS provisioning state
 *
 * @param[in,out] state. Pointer storage to hold the current FFS provisioning
 * state
 * @return ACE_STATUS_OK if everything goes well.
 */
ace_status_t aceFfs_getState(aceFfs_provisioningState_t* state);

/**
 * @brief Signal the provisioner of the final FFS completion status. This gives
 * the application the control to mark the overall FFS provisioning a success or
 * failure asynchronously. For instance, registration succeeds, but the
 * application specific post registration has failed, so the application can
 * decide to fail the overall FFS workflow.
 * @note This function is only applicable if @ref defer_registration_completion
 * is set to true during @ref aceFfs_init. The defer registration completion is
 * the recommended usage. If defer mode is enabled, application is required to
 * call this function to signal FFS completion regardless of registration
 * success or failure.
 * @note This function can only be called after the @ref
 * registration_complete callback is triggered.
 * @note This function is safe to be called from the FFS callback context.
 *
 * @param[in] completion_status. 0 indicates this is a successful
 * FFS provisioning. The success status will be reported back to the
 * provisioner. Non-zero status code will be reported back to the provisioner
 * as is. Status is ignored if registration failed.
 *
 * @return ACE_STATUS_OK if everything goes well.
 * @return ACE_STATUS_UNINITIALIZED if not initialized.
 * @return ACE_STATUS_NOT_SUPPORTED if defer registration is not enabled.
 * @return ACE_STATUS_GENERAL_ERROR if API is called at unexpected times (i.e
 * device has not reached registration)
 */
ace_status_t aceFfs_sendCompletion(int32_t completion_status);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* _ACEFFS_PROVISIONABLE_H_ */
