/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All rights reserved.
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
 * @File map_api.h
 *
 * @brief ACS Device Registration APIs and Structures
 *  ACS MAP middleware implements communication protocols to communicate
 *  with backend servers for supporting 1P device registration/deregistration,
 *  token and registration information management.
 *  ACS MAP APIs can be directly used by other ACS middleware components
 *  and Apps. A typical usage for device register/deregister can be done
 *  in sequence of function calls:
 *      1. aceMap_init()                 --> To init the ACS MAP
 *      2. aceMap_registerDevice()       --> Perform device registration
 *      3. aceMap_getRegistrationState() --> Monitor current state
 *      4. aceMap_updateAccessToken()    --> If caller wants to update token
 *      5. aceMap_deregisterDevice()     --> Perform device deregistration
 *      6. aceMap_deinit()               --> Deinit when exit (must be called)
 *  For details, please refer to each function's description below.
 */

#ifndef _ACE_MAP_API_H_
#define _ACE_MAP_API_H_

#include <ace/ace_config.h>
#include <ace/ace_status.h>
#include <ace/dispatcher_core.h>
#include <stddef.h>
#include <stdint.h>

#if defined(ACE_EVENTMGR_EVENTS_V1_ENABLE)
#include <ace/ace_events.h>
#endif
#include <ace/eventmgr_events.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ACE_MAP_DATA Map client data types
 * @{
 * @ingroup AceMap
 */

/** Maximum length of register public/private code */
#define ACE_MAP_REG_CODE_LENGTH 128

/** Maximum length of account id buffer */
#define ACE_MAP_ACCOUNT_ID_LENGTH 128

/** Maximum length of access/refresh token */
#define ACE_MAP_TOKEN_LENGTH 2048

/** Maximum length of device name */
#define ACE_MAP_DEVICE_NAME_LENGTH 256

/** Maximum length of challenge reason */
#define ACE_MAP_CHALLENGE_REASON_LENGTH 128

/** Maximum length of challenge method */
#define ACE_MAP_CHALLENGE_METHOD_LENGTH 128

/**
 * @brief Panda endpoint enum types
 */
typedef enum {
    ACE_MAP_PROD = 1, /**< endpoint type PRODUCTION */
    ACE_MAP_GAMMA,    /**< endpoint type GAMMA */
    ACE_MAP_BETA,     /**< endpoint type BETA */
} aceMap_endpointType_t;

/**
 * @brief Map register challenge info
 */
typedef struct {
    char challenge_reason[ACE_MAP_CHALLENGE_REASON_LENGTH];
                                 /**< Register challenge reason */
    char challenge_method[ACE_MAP_CHALLENGE_METHOD_LENGTH];
                                 /**< Register challenge required
                                      authentication method */
} aceMap_challengeInfo_t;

/**
 * @brief Map registration error info
 */
typedef struct {
    ace_status_t status;        /**< Device register status, based on
                                     this status, user can further retrieve
                                     error_info in union struct below. */
    union {
        aceMap_challengeInfo_t challenge_info;
                                /**< Register error challenge info. This info
                                     will be available if status equals to
                                     ACE_STATUS_REGISTRATION_ACCOUNT_CHALLENGED
                                     */
    } error_info;               /**< Union for register error info */
} aceMap_regErrorInfo_t;

/**
 * @brief Device Information data type required for registration
 */
typedef struct {
    const char* country_code;   /**< country code:
                                     US(United States - default),
                                     UK(United Kingdom),
                                     DE(Germany),
                                     FR(France),
                                     IT(Italy),
                                     ES(Spain),
                                     JP(Japan),
                                     MX(Mexico),
                                     CA(Canada), more found in ISO 3166-2
                                     spec) */
    aceMap_endpointType_t endpoint_type;
                                /**< endpoint type as defined
                                     in @ref aceMap_endpointType_t */
    const char* email;          /**< email for device registration */
    const char* password;       /**< password for device registration */
    const char* language_locale;
                                /**< language locale for device registration */
    const char* device_model;   /**< device model, example value: EchoDevice,
                                     it is an optional field */
    const char* app_version;    /**< a version number for the app, this is
                                     modeled as a string, but it's expected
                                     to be an integer, example value "0.1",
                                     it is an optional field */
    const char* app_name;       /**< a name for the app,
                                     example value: AlexTestApp,
                                     it is an optional field */
    const char* os_version;     /**< os version, this is modeled as a string,
                                     but it's expected to be an integer,
                                     example value: "1.0",
                                     it is an optional field */
    const char* software_version;
                                /**< device software version, example
                                     value "1.0", it is an optional field */
    aceMap_regErrorInfo_t* register_error_info;
                                /**< This is optional. The pointer to caller
                                     allocated memory for caller to receive
                                     device registration error info, such as
                                     registration challenged info
                                     @ref aceMap_challengeInfo_t.
                                     Map will fill in the error info during
                                     device register. In sync mode, caller gets
                                     error info after aceMap_registerDevice()
                                     function returns. In async mode,
                                     the error info will be available after
                                     caller receiving
                                     register_device_complete() callback. Refer
                                     to @ref aceMap_registerDevice() for more
                                     details. */
} aceMap_regInfo_t;

/**
 * @brief Linkcode for code based linking (CBL).
 */
typedef struct {
    char public_code[ACE_MAP_REG_CODE_LENGTH];  /**< public code */
    char private_code[ACE_MAP_REG_CODE_LENGTH]; /**< private code */
} aceMap_cblCode_t;

/**
 * @brief Device registration code
 */
typedef struct {
    bool is_preauth_code;           /**< 0: not a preauth code
                                         1: a preauth code */
    union {
        aceMap_cblCode_t cbl_code;  /**< @ref aceMap_cblCode_t,
                                         using it when @ref is_preauth_code
                                         is 0 */
        char preauth_cbl_code[ACE_MAP_REG_CODE_LENGTH];
                                    /**< pre-authorized code based linking
                                        (CBL) code, using it when @ref
                                        is_preauth_code is 1 */
    };
    int polling_interval;           /**< code polling interval */
    int expires_in;                 /**< code will expire in (second) */
} aceMap_regCode_t;

/**
 * @brief Device registration state
 */
typedef enum {
    ACE_MAP_DEVICE_NOT_REGISTERED = 0,  /**< Device is not registered */
    ACE_MAP_DEVICE_REGISTERING,         /**< Device is registering */
    ACE_MAP_DEVICE_REGISTERED,          /**< Device is registered */
    ACE_MAP_DEVICE_REGISTRATION_UNKNOWN = 0xEF,
                                        /**< Device state is unknown */
} aceMap_registrationState_t;

/**
 * @brief Device deregistration reason
 */
typedef enum {
    ACE_MAP_DEREGISTRATION_NO_REASON = 0,         /**< Device deregister due
                                                       to no reason */
    ACE_MAP_DEREGISTRATION_REMOTE,                /**< Device deregister due
                                                       to request from
                                                       remote server */
    ACE_MAP_DEREGISTRATION_REREGISTRATION,        /**< Device deregister due
                                                       to reregistration */
    ACE_MAP_DEREGISTRATION_FACTORY_RESET_SHALLOW, /**< Device deregister due
                                                       to factory-reset
                                                       (shallow) */
    ACE_MAP_DEREGISTRATION_FACTORY_RESET_DEEP,    /**< Device deregister due
                                                       to factory-reset
                                                       (deep) */
    ACE_MAP_DEREGISTRATION_LOCAL,                 /**< Device deregister only
                                                       remove local
                                                       registration data */
} aceMap_deregistrationReason_t;

/**
 * @brief Map event type
 */
typedef enum {
    ACE_MAP_EVENT_REGISTER = ACE_EVENTMGR_MAP_EVENT_REGISTER,
                                                /**< Device is registered */
    ACE_MAP_EVENT_DEREGISTER = ACE_EVENTMGR_MAP_EVENT_DEREGISTER,
                                                /**< Device is deregistered */
    ACE_MAP_EVENT_ACCESS_TOKEN_UPDATED =
        ACE_EVENTMGR_MAP_EVENT_ACCESS_TOKEN_UPDATED,
                                                /**< Access token is updated */
    ACE_MAP_EVENT_DEVICE_NAME_UPDATED =
        ACE_EVENTMGR_MAP_EVENT_DEVICE_NAME_UPDATED,
                                                /**< Device name is updated */
    ACE_MAP_EVENT_INVALID_DEVICE =  ACE_EVENTMGR_MAP_EVENT_INVALID_DEVICE,
                                                /**< Invalid device detected.
                                                     In case device was
                                                     deregistered from Cloud
                                                     when device is offline,
                                                     after that when device
                                                     back online and update
                                                     token, MAP will receive
                                                     “Invalid Device” in
                                                     response from backend */
} aceMap_eventType_t;

/**
 * @brief Map event reason
 */
typedef enum {
    ACE_MAP_EVENT_NO_REASON = 0,                    /**< Device is deregistered
                                                         with no reason */
    ACE_MAP_EVENT_DEREGISTER_REMOTE,                /**< Device is deregistered
                                                         due to request from
                                                         remote server */
    ACE_MAP_EVENT_DEREGISTER_REGISTRATION,          /**< Device is deregistered
                                                         due to registration
                                                         (i.e OOBE) */
    ACE_MAP_EVENT_DEREGISTER_FACTORY_RESET_SHALLOW, /**< Device is deregistered
                                                         due to factory-reset
                                                         (shallow) */
    ACE_MAP_EVENT_DEREGISTER_FACTORY_RESET_DEEP,    /**< Device is deregistered
                                                         due to factory-reset
                                                         (deep) */
    ACE_MAP_EVENT_DEREGISTER_LOCAL,                 /**< Device is deregistered
                                                         by only remove local
                                                         registration data */
    ACE_MAP_EVENT_REGISTER_ACCOUNT_ADDED,           /**< Device is registered
                                                         due to registration */
    ACE_MAP_EVENT_REGISTER_ACCOUNT_TRANSFERRED,     /**< Device is registered
                                                         due to account
                                                         transfer */
    ACE_MAP_EVENT_ACCOUNT_REMOVED,                  /**< Device account is
                                                         removed */
} aceMap_eventReason_t;

/**
 * @brief Map event parameters
 */
typedef struct {
    aceMap_eventType_t type;     /**< Map event type as defined
                                      in @ref aceMap_eventType_t */
    aceMap_eventReason_t reason; /**< Map event reason as defined in
                                      @ref aceMap_eventReason_t */
} aceMap_eventParams_t;

/**
 * @brief Map callback types
 */
typedef struct {
    /**
     * @brief Register device complete callback function
     * Refer to @ref aceMap_registerDevice() for more details.
     *
     * @param[in] ctx The callback context
     * @param[in] status The status of completion
     */
    void (*register_device_complete)(void* ctx, ace_status_t status);

    /**
     * @brief Deregister device complete callback function
     *
     * @param[in] ctx The callback context
     * @param[in] status The status of completion
     */
    void (*deregister_device_complete)(void* ctx, ace_status_t status);

    /**
     * @brief Get code pair callback function
     *
     * @param[in] ctx The callback context
     * @param[in] code_pair The pointer to received code pair
     * @param[in] status The status of completion
     */
    void (*get_code_pair_complete)(void* ctx, const aceMap_regCode_t* code_pair,
                                   ace_status_t status);

    /**
     * @brief Get preauth code callback function
     *
     * @param[in] ctx The callback context
     * @param[in] preauth_code The pointer to received preauth code
     * @param[in] status The status of completion
     */
    void (*get_preauth_code_complete)(void* ctx,
                                      const aceMap_regCode_t* preauth_code,
                                      ace_status_t status);

    /**
     * @brief Update token callback function
     *
     * @param[in] ctx The callback context
     * @param[in] token The pointer to received token
     * @param[in] token_len The length of token buffer
     * @param[in] status The status of completion
     */
    void (*update_token_complete)(void* ctx, const char* token,
                                  size_t token_len, ace_status_t status);

    /**
     * @brief Get device name callback function
     *
     * @param[in] ctx The callback context
     * @param[in] device_name The pointer to receive device name
     * @param[in] device_name_len The length of device name buffer
     * @param[in] status The status of completion
     */
    void (*get_device_name_complete)(void* ctx, const char* device_name,
                                     size_t device_name_len,
                                     ace_status_t status);

    /**
     * @brief Get transferred credentials callback function
     *
     * @param[in] ctx The callback context
     * @param[in] status The status of completion
     */
    void (*get_transferred_credentials_complete)(void* ctx,
                                                 ace_status_t status);

    /**
     * @brief callback context
     */
    void* ctx;
} aceMap_callbacks_t;

/**
 * @brief Init data struct
 */
typedef struct {
    aceMap_callbacks_t* callbacks;  /**< For synchronous mode, caller
                                         must set callback to NULL;
                                         For asynchronous mode, caller
                                         needs to provide corresponding
                                         callback functions */
    aceDispatcher_dispatchHandle_t* disp_handle;
                                    /**< Caller can pass in dispatcher handle.
                                         If set this handle to NULL, MAP
                                         will use LIFE_CYCLE dispatcher or
                                         create a dispatcher internally
                                         depending on the configuration */
} aceMap_initData_t;

/** @brief Opaque handle used for accessing aceMap APIs */
typedef void* aceMap_handle_t;

/** @} */

/**
 * @defgroup ACE_MAP_API Map client APIs
 * @{
 * @ingroup AceMap
 */

/**
 * @brief Initializes the MAP client. Needs to be called before calling
 * any other MAP APIs. Each caller must call this init function once to
 * get initialized map_handle. MAP malloc the map_handle internally, caller
 * must call @ref aceMap_deinit() to ensure MAP to free the map_handle when
 * exit.
 *
 * @param[in] init_data Caller allocated pointer to init data structure.
 * @param[out] map_handle MAP handle for caller to invoke other MAP APIs.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_init(const aceMap_initData_t* init_data,
                         aceMap_handle_t** map_handle);

/**
 * @brief Deinitializes the MAP client. Caller must call @ref aceMap_deinit()
 * to ensure MAP to free the map_handle when exit.
 *
 * @param[in] map_handle MAP handle returned by @ref aceMap_init().
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_deinit(aceMap_handle_t** map_handle);

/**
 * @brief Perform necessary actions to generate linkcode for code based
 * linking (CBL).
 * In case of error, retry needs to be done with exponential backoff. See
 * User Guide @ref maplite_user_guide_3 for more details.
 *
 * @param[in] map_handle MAP handle returned by @ref aceMap_init().
 * @param[in] info The pointer to @ref aceMap_regInfo_t. The following need to
 * be filled in before calling: country_code, endpoint_type, language_locale.
 * The device_model, app_name, app_version, os_version are optional.
 * The @ref info pointer can not be freed until the @ref
 * get_code_pair_complete() callback is received.
 * @param[out] code_pair For sync mode, a pointer to caller allocated memory
 * to save the code pair; For async mode, code pair data is returned in @ref
 * get_code_pair_complete() callback.
 * @param[in] timeout_ms Time out value in ms. Value 0: the waiting time is
 * infinite.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_generateLinkCode(const aceMap_handle_t* map_handle,
                                     aceMap_regInfo_t* info,
                                     aceMap_regCode_t* code_pair,
                                     uint32_t timeout_ms);

/**
 * @brief Perform necessary actions to generate a pre-authorized registration
 * code.
 * In case of error, retry needs to be done with exponential backoff. See
 * User Guide @ref maplite_user_guide_3 for more details.
 *
 * @param[in] map_handle MAP handle returned by @ref aceMap_init().
 * @param[out] preauth_code For sync mode, a pointer to caller allocated memory
 * to save the preauth code; For async mode, preauth code data is returned in
 * @ref get_preauth_code_complete() callback.
 * @param[in] timeout_ms Time out value in ms. Value 0: the waiting time is
 * infinite.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_generatePreAuthorizedLinkCode(
    const aceMap_handle_t* map_handle, aceMap_regCode_t* preauth_code,
    uint32_t timeout_ms);

/**
 * @brief Perform necessary actions to register device with the backend server.
 * In case of error, retry needs to be done with exponential backoff. See
 * User Guide @ref maplite_user_guide_3 for more details.
 *
 * In async mode, after user calls aceMap_registerDevice(), the
 * @ref register_device_complete() callback will always be called upon device
 * registration completion, whether it is successfully completed or not.
 * The status parameter in register_device_complete() gives the status of
 * completion. E.g. If registration is challenged, device_register_complete()
 * gets called with status ACE_STATUS_REGISTRATION_ACCOUNT_CHALLENGED.
 * User can then parse the error info @ref aceMap_challengeInfo_t to determine
 * which step to take next, such as using CBL or MFA to register the device
 * again via aceMap_registerDevice().
 * In sync mode, if registration is challenged, aceMap_registerDevice() will
 * return with status ACE_STATUS_REGISTRATION_ACCOUNT_CHALLENGED.
 *
 * @param[in] map_handle MAP handle returned by @ref aceMap_init().
 * @param[in] info The pointer to caller allocated @ref aceMap_regInfo_t.
 * Following need to be filled in before calling: country_code,
 * endpoint_type, language_locale. The device_model, app_name, app_version,
 * os_version are optional. Provide username/password for mac_sec registration.
 * The @ref info pointer can not be freed until the @ref
 * register_device_complete() callback is received.
 * @param[in] code_pair The pointer to @ref aceMap_regCode_t that contains
 * linkcode for registration. Set contents to {0} if using username/password
 * registration.
 * @param[in] timeout_ms Time out value in ms. Value 0: the waiting time is
 * infinite.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_registerDevice(const aceMap_handle_t* map_handle,
                                   aceMap_regInfo_t* info,
                                   aceMap_regCode_t* code_pair,
                                   uint32_t timeout_ms);

/**
 * @brief Perform necessary actions to deregister device with the backend
 * server.
 * In case of error, retry needs to be done with exponential backoff. See
 * User Guide @ref maplite_user_guide_3 for more details.
 *
 * @param[in] map_handle MAP handle returned by @ref aceMap_init().
 * @param[in] reason The reason to deregister device. @ref
 * aceMap_deregistrationReason_t for available reason.
 * @param[in] timeout_ms Time out value in ms. Value 0: the waiting time is
 * infinite.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_deregisterDevice(const aceMap_handle_t* map_handle,
                                     aceMap_deregistrationReason_t reason,
                                     uint32_t timeout_ms);

/**
 * @brief Get current device registration state.
 *
 * @param[out] state The pointer to current device registration state. @ref
 * aceMap_registrationState_t for available states.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_getRegistrationState(aceMap_registrationState_t* state);

/**
 * @brief Get current device refresh token.
 *
 * @param[out] token The pointer to caller allocated memory to receive token.
 * @param[in, out] token_len Buffer size of allocated memory. Must >=
 * ACE_MAP_TOKEN_LENGTH
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_getRefreshToken(char* token, size_t* token_len);

/**
 * @brief Get current device access token.
 * Retrieves the access token from local cached memory. If caller doesn't
 * provide @ref token and @ref token_len buffers, this API can be used for
 * access token validity check.
 * If access token is not expired, it will return ACE_STATUS_OK, and if
 * @ref token and @ref token_len buffers are provided (not NULL), then token
 * value and size will be filled in @ref token and @ref token_len buffers.
 * If the token is expired, it will return ACE_STATUS_ACCESS_TOKEN_EXPIRED,
 * and no values will be filled into @ref token and @ref token_len buffers.
 * The caller should call @ref aceMap_updateAccessToken() API to get a
 * valid token.
 *
 * @param[in, out] token The pointer to caller allocated memory to receive
 * token. It also can be NULL pointer if just for token validity check.
 * @param[in, out] token_len Buffer size of allocated memory.
 * Must >= ACE_MAP_TOKEN_LENGTH. It can be NULL pointer if just for
 * token validity check.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_getAccessToken(char* token, size_t* token_len);

/**
 * @brief Update access token from refresh token.
 * In case of error, retry needs to be done with exponential backoff. See
 * User Guide @ref maplite_user_guide_3 for more details.
 *
 * @param[in] map_handle MAP handle returned by @ref aceMap_init().
 * @param[in, out] token The pointer to caller allocated memory to receive
 * token. It also can be NULL pointer if using MAP async mode.
 * @param[in, out] token_len Buffer size of allocated memory.
 * Must >= ACE_MAP_TOKEN_LENGTH. It also can be NULL pointer if using
 * MAP async mode.
 * @param[in] timeout_ms Time out value in ms. Value 0: the waiting time is
 * infinite.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_updateAccessToken(const aceMap_handle_t* map_handle,
                                      char* token, size_t* token_len,
                                      uint32_t timeout_ms);

/**
 * @brief Get registered account ID.
 *
 * @param[out] account_id The pointer to caller allocated memory to save
 * @ref account_id.
 * @param[in, out] account_length Buffer size of allocated memory. Must >=
 * @ref ACE_MAP_ACCOUNT_ID_LENGTH. The result length of @ref account_id will
 * be saved here.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_getAccountId(char* account_id, uint32_t* account_length);

/**
 * @brief Get device name with option of force update.
 * If force update is on, it gets device name from backend service, otherwise
 * it gets cached device name. If device name changed, then an
 * ACE_MAP_EVENT_DEVICE_NAME_UPDATED event will be emitted.
 * For async mode if force update is on, the device name and name length will
 * be returned in @ref get_device_name_complete() callback function; For sync
 * mode, the device name and length will be returned in this function's
 * parameters @ref device_name and @ref name_length.
 * In case of error, retry needs to be done with exponential backoff. See
 * User Guide @ref maplite_user_guide_3 for more details.
 *
 * @param[in] map_handle MAP handle returned by @ref aceMap_init().
 * This parameter is used when force_update is on, otherwise user can set it
 * to NULL.
 * @param[in, out] device_name [in]:User allocated buffer pointer to store
 * device name, the buffer length should be less or equal to
 * @ref ACE_MAP_DEVICE_NAME_LENGTH. [out]:Device name buffer will be null
 * terminated. This parameter is used when force_update is NOT on, otherwise
 * user can set it to NULL.
 * @param[in, out] name_length [in]:Buffer length of device_name; [out]:Length
 * of device name. This parameter is used when force_update is NOT on,
 * otherwise user can set it to NULL.
 * @param[in] force_update true: get updated device name from backend service;
 * false: get cached device name.
 * @param[in] timeout_ms Time out value in ms. Value 0: the waiting time is
 * infinite. This parameter is used when force_update is on; otherwise it is
 * un-used and caller can set it to 0.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_getDeviceName(const aceMap_handle_t* map_handle,
                                  char* device_name, size_t* name_length,
                                  bool force_update, uint32_t timeout_ms);

/**
 * @brief Get transferred device's new credentials.
 * This API should only be used once the device has been remote transferred
 * initiated from the cloud side. Do not call this if your device has not
 * been transferred. If new device credentials are obtained successfully,
 * then both ACE_MAP_EVENT_REGISTER and ACE_MAP_EVENT_ACCESS_TOKEN_UPDATED
 * will be emitted.
 * In case of error, retry needs to be done with exponential backoff. See
 * User Guide @ref maplite_user_guide_3 for more details.
 *
 * @param[in] map_handle MAP handle returned by @ref aceMap_init().
 * @param[in] timeout_ms Time out value in ms. Value 0: the waiting time is
 * infinite.
 * @return ACE_STATUS_OK on success, other @ref ace_status_t values on error.
 */
ace_status_t aceMap_getTransferredDeviceCredentials(
    const aceMap_handle_t* map_handle, uint32_t timeout_ms);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /*_ACE_MAP_API_H_*/
