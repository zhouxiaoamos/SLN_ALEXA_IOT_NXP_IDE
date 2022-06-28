/*
 * Copyright 2020-2021 Amazon.com, Inc. or its affiliates. All rights reserved.
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
#ifndef EVENTMGR_TYPES_H
#define EVENTMGR_TYPES_H

/**
 * @file  eventmgr_types.h
 * @brief Data types use in the EventMgr API
 */

#include <stdint.h>
#include <string.h>
#include <ace/ace_config.h>
#include <ace/ace_modules.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup EVENTMGR_TYPES
 * @{
 */

/** Module Id starting value for usage by applications */
#define ACE_EVENTMGR_APP_MODULE_ID_START (ACE_MODULE_MAX + 1)

/** Module Id max value supported for usage by applications */
#define ACE_EVENTMGR_APP_MODULE_ID_END (ACE_EVENTMGR_APP_MODULE_ID_START + 255)

/** Subscriber type values - callback function */
#define ACE_EVENTMGR_SUBSCRIBE_TYPE_CALLBACK (1)

/** Publisher flags for notifications - publish complete notification */
#define ACE_EVENTMGR_PUBLISHER_FLAGS_PUBLISH_COMPLETE (1)

/** Group Id type */
typedef uint8_t aceEventMgr_groupId_t;

/** %Event Id type */
typedef uint32_t aceEventMgr_eventId_t;

/** Subscriber type */
typedef uint8_t aceEventMgr_subscribeType_t;

/**
 * @brief Publisher flags
 * @details Supports bitwise OR the supported flags -
 *          e.g. @ref  ACE_EVENTMGR_PUBLISHER_FLAGS_PUBLISH_COMPLETE.
 */
typedef uint16_t aceEventMgr_publisherFlags_t;

/**
 * @brief %Event params type passed to the callback function
 */
typedef struct aceEventMgr_eventParams_s {
    aceModules_moduleId_t module_id; /**< Publisher module id */
    aceEventMgr_groupId_t group_id;  /**< Group id for the event */
    aceEventMgr_eventId_t event_id;  /**< %Event id */
    size_t length;                   /**< Length of the event data */
    void* data;                      /**< %Event data */
} aceEventMgr_eventParams_t;

/**
 * @brief Callback function prototype
 *
 * @param[in] params The occurred event info for the registered callback.
 * @param[in] ctx    User provided context info.
 */
typedef void (*aceEventMgr_callbackFunc_t)(aceEventMgr_eventParams_t* params,
                                           void* ctx);

/*
 * The following structs are to be treated as opaque as their fields are for
 * internal use only.
 */

/**
 * @brief Callback params
 * @details Use @ref aceEventMgr_setCallbackSubscribeParams to set up this
 * struct.
 *
 * @note This struct should be treated as opaque as the fields are for internal
 * use only.
 */
typedef struct aceEventMgr_callbackParams_s {
    aceEventMgr_callbackFunc_t func; /**< @private */
    void* ctx;                       /**< @private */
} aceEventMgr_callbackParams_t;

/**
 * @brief Params that are passed for the registering subscribers
 * @details Use @ref aceEventMgr_setCallbackSubscribeParams to set up this
 * struct.
 *
 * @note This struct should be treated as opaque as the fields are for internal
 * use only.
 */
typedef struct aceEventMgr_subscribeParams_s {
    aceEventMgr_subscribeType_t type; /**< @private */
    union {
        aceEventMgr_callbackParams_t cb; /**< @private */
    };
} aceEventMgr_subscribeParams_t;

/**
 * @brief Publish params containing event data
 * @details Use @ref aceEventMgr_setPublishParams to set up this struct.
 *
 * @note This struct should be treated as opaque as the fields are for internal
 * use only.
 */
typedef struct aceEventMgr_publishParams_s {
    void* data;    /**< @private */
    size_t length; /**< @private */
} aceEventMgr_publishParams_t;

/**
 * @brief Subscriber handle
 * @details This handle is initialized by @ref aceEventMgr_registerSubscriber.
 *
 * @note This struct should be treated as opaque as the fields are for internal
 * use only.
 */
typedef struct aceEventMgr_subscribeHandle_s {
    union {
        struct {
            uint16_t publisher_index;  /**< @private */
            uint16_t subscriber_index; /**< @private */
        };
        void* client_info; /**< @private */
    };
} aceEventMgr_subscribeHandle_t;

/**
 * @brief Publisher handle
 * @details This handle is initialized by @ref aceEventMgr_registerPublisher or
 * @ref aceEventMgr_registerPublisherEx
 *
 * @note This struct should be treated as opaque as the fields are for internal
 * use only.
 */
typedef struct aceEventMgr_publishHandle_s {
    union {
        struct {
            uint16_t publisher_index;  /**< @private */
            uint16_t subscriber_index; /**< @private */
        };
        void* client_info; /**< @private */
    };
} aceEventMgr_publishHandle_t;

/**
 * @brief Publisher params containing notification information
 * @details Use @ref aceEventMgr_setPublishNotifications to set up this struct.
 *
 * @note This struct should be treated as opaque as the fields are for internal
 * use only.
 */
typedef struct aceEventMgr_publisherParams_s {
    aceEventMgr_publisherFlags_t flags; /**< @private */
    aceEventMgr_subscribeType_t type;   /**< @private */
    union {
        aceEventMgr_callbackParams_t cb; /**< @private */
    };
} aceEventMgr_publisherParams_t;

/** @} */
#ifdef __cplusplus
}
#endif

#endif  // EVENTMGR_TYPES_H