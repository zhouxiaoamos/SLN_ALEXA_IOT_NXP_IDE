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
/*
 * @file ace_conn_mgr.h
 *
 * @brief The ACE connectivity manager provides an api for app developers to
 * manage http connections
 */

#ifndef _ACE_CONN_MGR_H_
#define _ACE_CONN_MGR_H_

#include <ace/ace.h>
#include <toolchain/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ACE_ACM_DEF structs, defines, enums
 * @{
 * @ingroup ACE_ACM
 */

#define ACM_LOG_LEVEL ACM_LOG_DEBUG

#if WEB_SOCKETS_VERSION == 4
/**
 * @deprecated - Refer to lib web socket api for the new support.
 */
deprecated__ typedef enum acmHeaderTokenIndex {
    ACM_HEADER_TOKEN_GET_URI,
    ACM_HEADER_TOKEN_POST_URI,
#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_OPTIONS_URI,
#endif
    ACM_HEADER_TOKEN_HOST,
    ACM_HEADER_TOKEN_CONNECTION,
    ACM_HEADER_TOKEN_UPGRADE,
    ACM_HEADER_TOKEN_ORIGIN,
#if defined(LIBWEBSOCKETS_WITH_WS)
    ACM_HEADER_TOKEN_DRAFT,
#endif
    ACM_HEADER_TOKEN_CHALLENGE,
#if defined(LIBWEBSOCKETS_WITH_WS)
    ACM_HEADER_TOKEN_EXTENSIONS,
    ACM_HEADER_TOKEN_KEY1,
    ACM_HEADER_TOKEN_KEY2,
    ACM_HEADER_TOKEN_PROTOCOL,
    ACM_HEADER_TOKEN_ACCEPT,
    ACM_HEADER_TOKEN_NONCE,
#endif
    ACM_HEADER_TOKEN_HTTP,
#if !defined(LIBWEBSOCKETS_WITHOUT_H2)
    ACM_HEADER_TOKEN_HTTP2_SETTINGS,
#endif
    ACM_HEADER_TOKEN_HTTP_ACCEPT,
#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_HTTP_AC_REQUEST_HEADERS,
#endif
    ACM_HEADER_TOKEN_HTTP_IF_MODIFIED_SINCE,
    ACM_HEADER_TOKEN_HTTP_IF_NONE_MATCH,
    ACM_HEADER_TOKEN_HTTP_ACCEPT_ENCODING,
    ACM_HEADER_TOKEN_HTTP_ACCEPT_LANGUAGE,
    ACM_HEADER_TOKEN_HTTP_PRAGMA,
    ACM_HEADER_TOKEN_HTTP_CACHE_CONTROL,
    ACM_HEADER_TOKEN_HTTP_AUTHORIZATION,
    ACM_HEADER_TOKEN_HTTP_COOKIE,
    ACM_HEADER_TOKEN_HTTP_CONTENT_LENGTH,
    ACM_HEADER_TOKEN_HTTP_CONTENT_TYPE,
    ACM_HEADER_TOKEN_HTTP_DATE,
    ACM_HEADER_TOKEN_HTTP_RANGE,
#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    !defined(LIBWEBSOCKETS_WITHOUT_H2) || defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_HTTP_REFERER,
#endif
#if defined(LIBWEBSOCKETS_WITH_WS)
    ACM_HEADER_TOKEN_KEY,
    ACM_HEADER_TOKEN_VERSION,
    ACM_HEADER_TOKEN_SWORIGIN,
#endif
#if !defined(LIBWEBSOCKETS_WITHOUT_H2)
    ACM_HEADER_TOKEN_HTTP_COLON_AUTHORITY,
    ACM_HEADER_TOKEN_HTTP_COLON_METHOD,
    ACM_HEADER_TOKEN_HTTP_COLON_PATH,
    ACM_HEADER_TOKEN_HTTP_COLON_SCHEME,
    ACM_HEADER_TOKEN_HTTP_COLON_STATUS,
#endif

#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    !defined(LIBWEBSOCKETS_WITHOUT_H2) || defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_HTTP_ACCEPT_CHARSET,
#endif
    ACM_HEADER_TOKEN_HTTP_ACCEPT_RANGES,
#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    !defined(LIBWEBSOCKETS_WITHOUT_H2) || defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_HTTP_ACCESS_CONTROL_ALLOW_ORIGIN,
#endif
    ACM_HEADER_TOKEN_HTTP_AGE,
    ACM_HEADER_TOKEN_HTTP_ALLOW,
    ACM_HEADER_TOKEN_HTTP_CONTENT_DISPOSITION,
    ACM_HEADER_TOKEN_HTTP_CONTENT_ENCODING,
    ACM_HEADER_TOKEN_HTTP_CONTENT_LANGUAGE,
    ACM_HEADER_TOKEN_HTTP_CONTENT_LOCATION,
    ACM_HEADER_TOKEN_HTTP_CONTENT_RANGE,
    ACM_HEADER_TOKEN_HTTP_ETAG,
    ACM_HEADER_TOKEN_HTTP_EXPECT,
    ACM_HEADER_TOKEN_HTTP_EXPIRES,
    ACM_HEADER_TOKEN_HTTP_FROM,
    ACM_HEADER_TOKEN_HTTP_IF_MATCH,
    ACM_HEADER_TOKEN_HTTP_IF_RANGE,
    ACM_HEADER_TOKEN_HTTP_IF_UNMODIFIED_SINCE,
    ACM_HEADER_TOKEN_HTTP_LAST_MODIFIED,
    ACM_HEADER_TOKEN_HTTP_LINK,
    ACM_HEADER_TOKEN_HTTP_LOCATION,
#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    !defined(LIBWEBSOCKETS_WITHOUT_H2) || defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_HTTP_MAX_FORWARDS,
    ACM_HEADER_TOKEN_HTTP_PROXY_AUTHENTICATE,
    ACM_HEADER_TOKEN_HTTP_PROXY_AUTHORIZATION,
#endif
    ACM_HEADER_TOKEN_HTTP_REFRESH,
    ACM_HEADER_TOKEN_HTTP_RETRY_AFTER,
    ACM_HEADER_TOKEN_HTTP_SERVER,
    ACM_HEADER_TOKEN_HTTP_SET_COOKIE,
#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    !defined(LIBWEBSOCKETS_WITHOUT_H2) || defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_HTTP_STRICT_TRANSPORT_SECURITY,
#endif
    ACM_HEADER_TOKEN_HTTP_TRANSFER_ENCODING,
#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    !defined(LIBWEBSOCKETS_WITHOUT_H2) || defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_HTTP_USER_AGENT,
    ACM_HEADER_TOKEN_HTTP_VARY,
    ACM_HEADER_TOKEN_HTTP_VIA,
    ACM_HEADER_TOKEN_HTTP_WWW_AUTHENTICATE,
#endif
#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_PATCH_URI,
    ACM_HEADER_TOKEN_PUT_URI,
    ACM_HEADER_TOKEN_DELETE_URI,
#endif
    ACM_HEADER_TOKEN_HTTP_URI_ARGS,
#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_PROXY,
    ACM_HEADER_TOKEN_HTTP_X_REAL_IP,
#endif
    ACM_HEADER_TOKEN_HTTP1_0,
    ACM_HEADER_TOKEN_X_FORWARDED_FOR,
    ACM_HEADER_TOKEN_CONNECT,
    ACM_HEADER_TOKEN_HEAD_URI,
#if defined(LIBWEBSOCKETS_WITH_HTTP_UNCOMMON_HEADERS) || \
    !defined(LIBWEBSOCKETS_WITHOUT_H2) || defined (SS_ENABLE_PROXY)
    ACM_HEADER_TOKEN_TE,
    ACM_HEADER_TOKEN_REPLAY_NONCE,
#endif
#if !defined(LIBWEBSOCKETS_WITHOUT_H2)
    ACM_HEADER_TOKEN_COLON_PROTOCOL,
#endif
    ACM_HEADER_TOKEN_X_AUTH_TOKEN,
} acmHeaderTokenIndex_t;
#else  // LWS3
/**
 * @deprecated - Refer to lib web socket api for the new support.
 */
deprecated__ typedef enum acmHeaderTokenIndex {
    ACM_HEADER_TOKEN_GET_URI,
    ACM_HEADER_TOKEN_POST_URI,
    ACM_HEADER_TOKEN_OPTIONS_URI,
    ACM_HEADER_TOKEN_HOST,
    ACM_HEADER_TOKEN_CONNECTION,
    ACM_HEADER_TOKEN_UPGRADE,
    ACM_HEADER_TOKEN_ORIGIN,
    ACM_HEADER_TOKEN_DRAFT,
    ACM_HEADER_TOKEN_CHALLENGE,
    ACM_HEADER_TOKEN_EXTENSIONS,
    ACM_HEADER_TOKEN_KEY1,
    ACM_HEADER_TOKEN_KEY2,
    ACM_HEADER_TOKEN_PROTOCOL,
    ACM_HEADER_TOKEN_ACCEPT,
    ACM_HEADER_TOKEN_NONCE,
    ACM_HEADER_TOKEN_HTTP,
    ACM_HEADER_TOKEN_HTTP2_SETTINGS,
    ACM_HEADER_TOKEN_HTTP_ACCEPT,
    ACM_HEADER_TOKEN_HTTP_AC_REQUEST_HEADERS,
    ACM_HEADER_TOKEN_HTTP_IF_MODIFIED_SINCE,
    ACM_HEADER_TOKEN_HTTP_IF_NONE_MATCH,
    ACM_HEADER_TOKEN_HTTP_ACCEPT_ENCODING,
    ACM_HEADER_TOKEN_HTTP_ACCEPT_LANGUAGE,
    ACM_HEADER_TOKEN_HTTP_PRAGMA,
    ACM_HEADER_TOKEN_HTTP_CACHE_CONTROL,
    ACM_HEADER_TOKEN_HTTP_AUTHORIZATION,
    ACM_HEADER_TOKEN_HTTP_COOKIE,
    ACM_HEADER_TOKEN_HTTP_CONTENT_LENGTH,
    ACM_HEADER_TOKEN_HTTP_CONTENT_TYPE,
    ACM_HEADER_TOKEN_HTTP_DATE,
    ACM_HEADER_TOKEN_HTTP_RANGE,
    ACM_HEADER_TOKEN_HTTP_REFERER,
    ACM_HEADER_TOKEN_KEY,
    ACM_HEADER_TOKEN_VERSION,
    ACM_HEADER_TOKEN_SWORIGIN,
    ACM_HEADER_TOKEN_HTTP_COLON_AUTHORITY,
    ACM_HEADER_TOKEN_HTTP_COLON_METHOD,
    ACM_HEADER_TOKEN_HTTP_COLON_PATH,
    ACM_HEADER_TOKEN_HTTP_COLON_SCHEME,
    ACM_HEADER_TOKEN_HTTP_COLON_STATUS,
    ACM_HEADER_TOKEN_HTTP_ACCEPT_CHARSET,
    ACM_HEADER_TOKEN_HTTP_ACCEPT_RANGES,
    ACM_HEADER_TOKEN_HTTP_ACCESS_CONTROL_ALLOW_ORIGIN,
    ACM_HEADER_TOKEN_HTTP_AGE,
    ACM_HEADER_TOKEN_HTTP_ALLOW,
    ACM_HEADER_TOKEN_HTTP_CONTENT_DISPOSITION,
    ACM_HEADER_TOKEN_HTTP_CONTENT_ENCODING,
    ACM_HEADER_TOKEN_HTTP_CONTENT_LANGUAGE,
    ACM_HEADER_TOKEN_HTTP_CONTENT_LOCATION,
    ACM_HEADER_TOKEN_HTTP_CONTENT_RANGE,
    ACM_HEADER_TOKEN_HTTP_ETAG,
    ACM_HEADER_TOKEN_HTTP_EXPECT,
    ACM_HEADER_TOKEN_HTTP_EXPIRES,
    ACM_HEADER_TOKEN_HTTP_FROM,
    ACM_HEADER_TOKEN_HTTP_IF_MATCH,
    ACM_HEADER_TOKEN_HTTP_IF_RANGE,
    ACM_HEADER_TOKEN_HTTP_IF_UNMODIFIED_SINCE,
    ACM_HEADER_TOKEN_HTTP_LAST_MODIFIED,
    ACM_HEADER_TOKEN_HTTP_LINK,
    ACM_HEADER_TOKEN_HTTP_LOCATION,
    ACM_HEADER_TOKEN_HTTP_MAX_FORWARDS,
    ACM_HEADER_TOKEN_HTTP_PROXY_AUTHENTICATE,
    ACM_HEADER_TOKEN_HTTP_PROXY_AUTHORIZATION,
    ACM_HEADER_TOKEN_HTTP_REFRESH,
    ACM_HEADER_TOKEN_HTTP_RETRY_AFTER,
    ACM_HEADER_TOKEN_HTTP_SERVER,
    ACM_HEADER_TOKEN_HTTP_SET_COOKIE,
    ACM_HEADER_TOKEN_HTTP_STRICT_TRANSPORT_SECURITY,
    ACM_HEADER_TOKEN_HTTP_TRANSFER_ENCODING,
    ACM_HEADER_TOKEN_HTTP_USER_AGENT,
    ACM_HEADER_TOKEN_HTTP_VARY,
    ACM_HEADER_TOKEN_HTTP_VIA,
    ACM_HEADER_TOKEN_HTTP_WWW_AUTHENTICATE,
    ACM_HEADER_TOKEN_PATCH_URI,
    ACM_HEADER_TOKEN_PUT_URI,
    ACM_HEADER_TOKEN_DELETE_URI,
    ACM_HEADER_TOKEN_HTTP_URI_ARGS,
    ACM_HEADER_TOKEN_PROXY,
    ACM_HEADER_TOKEN_HTTP_X_REAL_IP,
    ACM_HEADER_TOKEN_HTTP1_0,
    ACM_HEADER_TOKEN_X_FORWARDED_FOR,
    ACM_HEADER_TOKEN_CONNECT,
    ACM_HEADER_TOKEN_HEAD_URI,
    ACM_HEADER_TOKEN_TE,
    ACM_HEADER_TOKEN_REPLAY_NONCE,
    ACM_HEADER_TOKEN_COLON_PROTOCOL,
    ACM_HEADER_TOKEN_X_AUTH_TOKEN,
} acmHeaderTokenIndex_t;
#endif

/**
 * @brief ACM status code
 */
typedef enum acmReturnCode {
    eAcmOk = ACE_STATUS_OK,

    /* input/output related */
    eAcmInvalidParameter = ACE_STATUS_BAD_PARAM,
    eAcmInvalidPayload = ACE_STATUS_BAD_PARAM,
    eAcmMessageTooLarge = ACE_STATUS_PARAM_OUT_OF_RANGE,
    eAcmOverflow = ACE_STATUS_BUFFER_OVERFLOW,
    eAcmInsufficientMemory = ACE_STATUS_OUT_OF_MEMORY,
    eAcmQueueFull = ACE_STATUS_OUT_OF_RESOURCES,
    eAcmRetry = ACE_STATUS_TRY_AGAIN,

    /* internal error  */
    eAcmNetworkErr = ACE_STATUS_NO_NET,
    eAcmConnectionErr = ACE_STATUS_NET_CONNECTION_ERROR,
    eAcmTimeoutErr = ACE_STATUS_NET_CONNECTION_TIMEOUT_ERROR,
    eAcmSendAbort = ACE_STATUS_NET_TRANSMIT_ABORT_ERROR,
    eAcmRecvAbort = ACE_STATUS_NET_RECEIVE_ABORT_ERROR,
    eAcmStreamErr = ACE_STATUS_GENERAL_ERROR,
    eAcmAuthenticateErr = ACE_STATUS_GENERAL_ERROR,
    eAcmTlsErr = ACE_STATUS_GENERAL_ERROR,
    eAcmUserCallbackErr = ACE_STATUS_GENERAL_ERROR,
    eAcmProtoErr = ACE_STATUS_GENERAL_ERROR,
    eAcmInternalErr = ACE_STATUS_GENERAL_ERROR,

    /* other */
    eAcmFatal = ACE_STATUS_UNRECOVERABLE,
    eAcmBusy = ACE_STATUS_BUSY,
    eAcmTryAgain = ACE_STATUS_TRY_AGAIN,
    eAcmDataExist = ACE_STATUS_ALREADY_EXISTS
} acmReturnCode_t;

/**
 * @deprecated - Refer to lib web socket api for the new support.
 */
deprecated__ typedef enum acmHttpStatus {
    ACM_HTTP_STATUS_CONTINUE = 100,

    ACM_HTTP_STATUS_OK = 200,
    ACM_HTTP_STATUS_NO_CONTENT = 204,
    ACM_HTTP_STATUS_PARTIAL_CONTENT = 206,

    ACM_HTTP_STATUS_MOVED_PERMANENTLY = 301,
    ACM_HTTP_STATUS_FOUND = 302,
    ACM_HTTP_STATUS_SEE_OTHER = 303,
    ACM_HTTP_STATUS_NOT_MODIFIED = 304,

    ACM_HTTP_STATUS_BAD_REQUEST = 400,
    ACM_HTTP_STATUS_UNAUTHORIZED,
    ACM_HTTP_STATUS_PAYMENT_REQUIRED,
    ACM_HTTP_STATUS_FORBIDDEN,
    ACM_HTTP_STATUS_NOT_FOUND,
    ACM_HTTP_STATUS_METHOD_NOT_ALLOWED,
    ACM_HTTP_STATUS_NOT_ACCEPTABLE,
    ACM_HTTP_STATUS_PROXY_AUTH_REQUIRED,
    ACM_HTTP_STATUS_REQUEST_TIMEOUT,
    ACM_HTTP_STATUS_CONFLICT,
    ACM_HTTP_STATUS_GONE,
    ACM_HTTP_STATUS_LENGTH_REQUIRED,
    ACM_HTTP_STATUS_PRECONDITION_FAILED,
    ACM_HTTP_STATUS_REQ_ENTITY_TOO_LARGE,
    ACM_HTTP_STATUS_REQ_URI_TOO_LONG,
    ACM_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
    ACM_HTTP_STATUS_REQ_RANGE_NOT_SATISFIABLE,
    ACM_HTTP_STATUS_EXPECTATION_FAILED,

    ACM_HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    ACM_HTTP_STATUS_NOT_IMPLEMENTED,
    ACM_HTTP_STATUS_BAD_GATEWAY,
    ACM_HTTP_STATUS_SERVICE_UNAVAILABLE,
    ACM_HTTP_STATUS_GATEWAY_TIMEOUT,
    ACM_HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED,
} acmHttpStatus_t;

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * Callback reason to pass to @acmHttpClient_request_retry. See api for detail.
 * This list can be extended.
 */
deprecated__ typedef enum acmRetryReason {
    ACM_CALLBACK_REASON_DEFAULT = 0,
    ACM_CALLBACK_REASON_BUF_ALLOC_FAIL,
    ACM_CALLBACK_REASON_NOTIFY_DISPATCHER_FAIL,
    ACM_CALLBACK_REASON_ERROR,
} acmCallbackReason_t;

/**
 * @deprecated - Refer to lib web socket api for the new support.
 */
deprecated__ typedef int acmHandle_t;

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * Callback order should not be changed here. All callback types are ordered in
 * the same order clients should expect them. Note that not all callbacks are
 * guaranteed to get called, but the order in which they are called is
 * guaranteed. (e.g. Connection error happens after ACM_CALLBACK_ESTABLISHED,
 * the next callback getting called will be ACM_CALLBACK_TIMEOUT or
 * ACM_CALLBACK_CLOSE)
 */
deprecated__ typedef enum acmCallback {
    ACM_CALLBACK_INIT_CONNECTION = (1 << 0),
    ACM_CALLBACK_APPEND_HEADER = (1 << 1),
    ACM_CALLBACK_WRITE = (1 << 2),
    ACM_CALLBACK_ESTABLISHED = (1 << 3),
    ACM_CALLBACK_READ_READY = (1 << 4),
    ACM_CALLBACK_COMPLETE = (1 << 5),
    ACM_CALLBACK_TIMEOUT = (1 << 6),
    ACM_CALLBACK_CLOSE = (1 << 7),
} acmCallback_t;

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Callback for ACM to retrieve access token from user chosen provider.
 *
 * @param[out] token_buf - Pointer to ACM allocated memory to receive token.
 *
 * @param[in out] token_buf_len - Buffer size of allocated memory. Callback
 *                should replace with retrieved token length.
 */
deprecated__ typedef ace_status_t (*acmHttpClient_get_access_token_cb)(
    char* token_buf, uint32_t* token_buf_len);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Callback used when ACM starts to service a connection.
 *
 * @param[in] handle - Unique identifier associated with this request.
 *
 */
deprecated__ typedef void (*acmHttpClient_init_connection_cb)(
    acmHandle_t handle);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Callback used when ACM asks for HTTP header content.
 *
 * @param[in] handle - Unique identifier associated with this request.
 *
 * @param[out] is_last_token - set this to 1 when all headers have been added.
 *                             [Deprecated]
 *
 * @par Example application code in this callback
 * @code
 *       acmHttpClient_add_header(handle, ACM_HEADER_TOKEN_HTTP_AUTHORIZATION,
 *                                (char *)aws_token, strlen(aws_token));
 *       acmHttpClient_add_header(handle, ACM_HEADER_TOKEN_CONTENT_LENGTH,
 *                                "145", 3);
 *       *is_last_token = 1;
 * @endcode
 */
deprecated__ typedef void (*acmHttpClient_append_header_cb)(acmHandle_t handle,
                                                            int* is_last_token);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Callback used when ACM notifies a write can proceed on this connection
 *
 * @param[in] handle - Unique identifier associated with this request.
 *
 * @par Example application code in this callback
 * @code
 *          // prepare write buffer first
 *
 *          acmHttpClient_write(handle,
 *                             (char *)write_buf,
 *                             (int)write_len,
 *                             1);
 * @endcode
 */

deprecated__ typedef void (*acmHttpClient_write_cb)(acmHandle_t handle);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Callback used when ACM notifies when connection is established.
 *
 * @param[in] handle - Unique identifier associated with this request.
 *
 * @param[in] rc - Return code of the connection status.
 *
 * @param[in] status - Current HTTP status code.
 */
deprecated__ typedef void (*acmHttpClient_established_cb)(
    acmHandle_t handle, acmReturnCode_t rc, acmHttpStatus_t status);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Callback used when ACM notifies a read is ready on the connection.
 *
 * @param[in] handle - Unique identifier associated with this request.
 *
 * @param[in] rc - Return code used for retry.
 *
 * @param[in] status - Current HTTP status code.
 *
 * @par Example application code in this callback
 * @code
 *          // obtain read buffer first
 *
 *          acmHttpClient_read(handle,
 *                             (char *)read_buf,
 *                             (int *)read_len);
 * @endcode
 */
deprecated__ typedef void (*acmHttpClient_read_ready_cb)(
    acmHandle_t handle, acmReturnCode_t rc, acmHttpStatus_t status);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Callback used when ACM notifies when connection is completed.
 *
 * @param[in] handle - Unique identifier associated with this request.
 *
 * @param[in] rc - Return code of the connection status.
 *
 * @param[in] status - Current HTTP status code.
 */
deprecated__ typedef void (*acmHttpClient_complete_cb)(acmHandle_t handle,
                                                       acmReturnCode_t rc,
                                                       acmHttpStatus_t status);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Callback used when ACM notifies when connection is closed.
 *
 * @param[in] handle - Unique identifier associated with this request.
 *
 * @param[in] rc - Return code of the connection status.
 */
deprecated__ typedef void (*acmHttpClient_close_cb)(acmHandle_t handle,
                                                    acmReturnCode_t rc);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Callback used when ACM notifies when connection timeout happens.
 *
 * @param[in] handle - Unique identifier associated with this request.
 *
 * @param[in] rc - Return code of the connection timeout.
 */
deprecated__ typedef void (*acmHttpClient_timeout_cb)(acmHandle_t handle,
                                                      acmReturnCode_t rc);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * ACM callbacks
 */
deprecated__ typedef struct acmHttpClient_callback {
    /**< NULL: no access toekn or user handle token
         not NULL: require acm to request token through
         this callback. ACM will attach access token
         before calling append header callback */
    acmHttpClient_get_access_token_cb get_access_token_cb;
    acmHttpClient_init_connection_cb init_connection_cb;
    /* Header: auth token, content-type, etc. */
    acmHttpClient_append_header_cb append_header_cb;
    /* RX */
    acmHttpClient_read_ready_cb read_ready_cb;
    /* TX */
    acmHttpClient_write_cb write_cb;
    /* Connection status */
    acmHttpClient_complete_cb complete_cb;
    acmHttpClient_established_cb established_cb;
    acmHttpClient_close_cb close_cb;
    acmHttpClient_timeout_cb timeout_cb;
} acmHttpClient_callback_t;

/**
 * @deprecated - Refer to lib web socket api for the new support.
 */
deprecated__ typedef enum acmHttpClient_method {
    ACMHTTPCLIENT_METHOD_GET,
    ACMHTTPCLIENT_METHOD_POST,
} acmHttpClient_method_t;

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * ACM HTTP request definition
 */
deprecated__ typedef struct acmHttpClient_request {
    /**
     * REQUIRED: App info
     * Appy to: H1, H2
     */
    char* appname;   /**< application name as a string */
    int appname_len; /**< application name length */

    /**
     * REQUIRED: Endpoint config
     * Apply to: H1, H2
     */
    char* address;   /**< remote address, e.g.,
              "avs-alexa-na.amazon.com" */
    int address_len; /**< remote address length */
    char* path;      /**< uri path, e.g., "/v20160207/directives" */
    int path_len;    /**< uri path length */
    int port;        /**< remote port number */
    int is_tls;      /**< 0: non-TLS, 1: TLS */
    acmHttpClient_method_t method;
    /**< acmHttpClient_method_t value for GET or POST */

    /**
     * OPTIONAL: Connection timeout
     * Apply to: H1, H2
     */
    int timeout; /**< timeout for connection in sec */

    /**
     * REQUIRED (for TLS): certificate chain
     * Apply to: H1, H2
     */
    const char* ca_cert; /**< certificate store for this connection */
    int ca_cert_len;     /**< certificate store size */

    /**
     * OPTIONAL: HTTP2 stream priority
     * Apply to: H2
     */
    int priority;          /**< H2 stream priority, value between 1-256 */
    acmHandle_t dependent; /**< H2 stream dependency on another stream */

    /**
     * REQUIRED: define callbacks
     * Apply to: H1, H2
     */
    acmHttpClient_callback_t* cb_group;
    /**< callback group definition for this connection */

    /**
     * OPTIONAL: client private data
     * Apply to: H1, H2
     */
    void* priv; /**< user provided priv data, used in cbs */
} acmHttpClient_request_t;

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * ACM configuration parameters
 */
deprecated__ typedef struct acmHttpClient_conf {
    int max_reqs_supp;  /**< maximum number of ACM requests allowed */
    int num_reqs;       /**< number of ACM requests waiting to be served */
    int num_conns;      /**< number of ACM requests currently being served */
    int app_timeout_s;  /**< Default max time (s) a request can be served by ACM
                         */
    int idle_timeout_s; /**< Idle (No RX/TX/callbacks) time (s) after which
                           connection is closed */
    int max_req_hdr_buf_len;  /**< maximum request header buf size allowed */
    int max_resp_hdr_buf_len; /**< maximum response header buf size allowed */
} acmHttpClient_conf_t;
/** @} */  // ACE_ACM_DEF

/**
 * @defgroup ACE_ACM_API Public API
 * @{
 * @ingroup ACE_ACM
 */

/**
 * @brief Intialize acm.
 *
 * @param[in] none.
 *
 * @param[out] status - status of acm initialization.
 *
 * @return eAcmOk if successfully initalized acm, failure code otherwise.
 *
 */
acmReturnCode_t acm_init(void);

#if WEB_SOCKETS_VERSION == 4
#if defined(SS_ENABLE_PROXY)
#if defined(LWS_SS_USE_SSPC)
#define acm_init acm_sspc_init
#endif /* LWS_SS_USE_SSPC */
/**
 * @brief Intialize acm for lws SS client process.
 *
 * @param[in] none.
 *
 * @param[out] status - status of acm initialization.
 *
 * @return eAcmOk if successfully initalized acm, failure code otherwise.
 *
 */
acmReturnCode_t acm_sspc_init(void);
#endif /* SS_ENABLE_PROXY */
#endif /* lws4 */

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Send http request to acm.
 *
 * @param[in] req - acmHttpClient request containing info for an http request.
 *
 * @param[out] handle - Unique identifier associated with this request.
 *
 * @return eAcmOk if successfully submitted a request, failure code otherwise.
 *
 */
deprecated__ acmReturnCode_t acmHttpClient_submit_request_async(
    acmHttpClient_request_t* req, acmHandle_t* handle);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief cancel pending request or on going connection in acm. cancel should
 *        not be used during close_cb or timeout_cb as during those callbacks
 *        connection is already closed. ACM will call close_cb after connection
 *        is canceled.
 *
 * @param[in] handle - handle previously returned by calling
 *          acmHttpClient_submit_request_async
 *
 * @return eAcmOk if successfully, failure code otherwise.
 *
 */
deprecated__ acmReturnCode_t acmHttpClient_cancel_request_async(
    acmHandle_t handle);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief read data from the network.
 *
 * @param[in] handle - identifier of the connection.
 *
 * @param[in] buf - client read data buffer pointer.
 *
 * @param[in out] len - pointer to max data buffer size, filled with data
 *                      read size upon return.
 *
 * @return eAcmOk if read successfully, failure code otherwise.
 *
 */
deprecated__ acmReturnCode_t acmHttpClient_read(acmHandle_t handle, char* buf,
                                                int* len);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief read header from response.
 *
 * @param[in] handle - identifier of the connection.
 *
 * @param[in] buf - client read header buffer pointer.
 *
 * @param[in] len - header buffer size.
 *
 * @param[in] h - index of the header to retreive.
 *
 * @return eAcmOk if read successfully, failure code otherwise.
 *
 */
deprecated__ acmReturnCode_t acmHttpClient_read_header(acmHandle_t handle,
                                                       char* buf, int len,
                                                       acmHeaderTokenIndex_t h);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief Request a write callback when socket becomes available.
 *
 * @param[in] handle - identifier of the connection wants to write to.
 *
 * @return eAcmOk if write successfully, failure code otherwise.
 *
 */
deprecated__ acmReturnCode_t acmHttpClient_prepare_write(acmHandle_t handle);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief write data to the network.
 *
 * @param[in] handle - identifier of the connection.
 *
 * @param[in] buf - client write data buffer pointer.
 *
 * @param[in] len - length of data to write.
 *
 * @param[in] is_complete - if there are more data to write. Setting this to
 *                          zero will trigger acm to send more write callback
 *                          when socket is ready.
 *
 * @return eAcmOk if write successfully, failure code otherwise.
 *
 */
deprecated__ acmReturnCode_t acmHttpClient_write(acmHandle_t handle, char* buf,
                                                 int len, int is_complete);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief append http header using well-defined tokens
 *
 * @param[in] handle - identifier of the connection to write to.
 *
 * @param[in] token_index - http token type.
 *
 * @param[in] buf - http header buffer pointer. Do not include token name.
 *
 * @param[in] len - length of header to write.
 *
 * @par Example application code in this callback
 * @code
 *          acmHttpClient_add_header(handle,
 *                           ACM_HEADER_TOKEN_HTTP_AUTHORIZATION,
 *                                   (char *)aws_token, strlen(aws_token));
 *          acmHttpClient_add_header(handle, ACM_HEADER_TOKEN_CONTENT_LENGTH,
 *                                   "145", 3);
 * @endcode
 */
deprecated__ acmReturnCode_t acmHttpClient_add_header(
    acmHandle_t handle, acmHeaderTokenIndex_t token_index,
    const unsigned char* buf, int len);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief append custom http header
 *
 * @param[in] handle - identifier of the connection to write to.
 *
 * @param[in] name - custom http header name. Check ":" requirement for your
 *           header.
 *
 * @param[in] value - custom http header value.
 *
 * @param[in] len - length of http header value to write.
 *
 * @par Example application code in this callback
 * @code
 *          acmHttpClient_add_header_custom(handle, "X-DSN:",
 *                                  "XXYYZZ", 6);
 * @endcode
 */
deprecated__ acmReturnCode_t
acmHttpClient_add_header_custom(acmHandle_t handle, const unsigned char* name,
                                const unsigned char* value, int len);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief return per connection client private data pointer.
 *
 * @param[in out] priv - pointer to returned client private data pointer.
 *
 */
deprecated__ acmReturnCode_t acmHttpClient_get_client_data(acmHandle_t handle,
                                                           void** priv);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief request acm to retry a given callback, the connection will hold until
 *.       retry is completed.
 *
 * @param[in] handle - identifier of the connection.
 *
 * @param[in] retry_callback - the callback client wants to retry
 *
 * @param[in] retry_after - acm will schedule retry after retry_after usec.
 *                           Time in microsecond.
 *
 * @param[in] retry_reason - reason for requesting this retry. Client can
 *                           retrieve this reason by calling
 *                           @acmHttpClient_get_retry_reason in the callback
 *
 */
deprecated__ acmReturnCode_t acmHttpClient_request_retry(
    acmHandle_t handle, acmCallback_t retry_callback, uint32_t retry_after,
    acmCallbackReason_t retry_reason);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief get retry reason to determine if the callback is a retry and why the
 *        retry is called.
 *
 * @param[in] handle - identifier of the connection.
 *
 * @param[out] retry_reason - pointer to returned retry reason.
 *
 */
deprecated__ acmReturnCode_t acmHttpClient_get_retry_reason(
    acmHandle_t handle, acmCallbackReason_t* retry_reason);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief get access token buffer associated with this request handle.
 *
 * @param[in] handle - identifier of the connection.
 *
 * @param[out] buf - pointer to returned token buffer pointer. Return NULL if
 * token doesn't exist.
 *
 * @param[out] buf_len - pointer to returned token buffer length. Return 0 if
 * token doesn't exist.
 *
 */
deprecated__ acmReturnCode_t acmHttpClient_get_access_token_buffer(
    acmHandle_t handle, char** buf, uint32_t* buf_len);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief turn off receive path
 *
 * @param[in] handle - identifier of the connection.
 *
 */
deprecated__ void acmHttpClient_rxflow_off(acmHandle_t handle);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief turn on receive path
 *
 * @param[in] handle - identifier of the connection.
 *
 */
deprecated__ void acmHttpClient_rxflow_on(acmHandle_t handle);

/**
 * @deprecated - Refer to lib web socket api for the new support.
 *
 * @brief retrieve ACM configured parameter values
 *
 * @param[in] - acmHttpClient_conf_t container to get configuration values.
 *
 */
deprecated__ void acmHttpClient_get_config(acmHttpClient_conf_t* conf);

#if WEB_SOCKETS_VERSION == 4
/**
 * @brief retrieve system netwokring eventloop context
 *
 * @param[in] - context container to get struct lws_context*.
 *
 */
acmReturnCode_t acmHttpClient_get_context(void** context);

/**
 * @brief initialize update access token feature
 *
 * @param[in] - context for attaching the token.
 *
 */
acmReturnCode_t acmSecureStreams_update_token_init(void* context);
#endif
/** @} */  // ACE_ACM_API

#ifdef __cplusplus
}
#endif

#endif
