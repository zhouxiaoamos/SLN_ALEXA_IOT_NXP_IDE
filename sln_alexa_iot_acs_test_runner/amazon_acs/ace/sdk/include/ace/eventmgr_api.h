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
#ifndef EVENTMGR_API_H
#define EVENTMGR_API_H

/**
 * @file  eventmgr_api.h
 * @brief EventMgr API for publish-subscribe events.
 */

#include <ace/ace_modules.h>
#include <ace/ace_status.h>
#include <ace/eventmgr_events.h>
#include <ace/eventmgr_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup EVENTMGR_API
 * @{
 */

/**
 * @brief Register an event publisher
 * @details Register as an event publisher for a module. The handle obtained can
 * be used to publish any event under any group of this module.
 *
 * For ACE modules, the param `module_id` is an unique id defined in
 * ace/ace_modules.h. Please refer to the module's user guide and doxygen for
 * information about the id.
 *
 * For applications, the param `module_id` is defined in application's
 * own headers. Please refer to @ref EVENTMGR_USER_GUIDE under
 * `Application event definition` section for information about defining the id.
 *
 * @param[in]  module_id Module id of the publisher.
 * @param[out] handle    Handle to the publisher context to be used for
 *                       publishing events.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_registerPublisher(aceModules_moduleId_t module_id,
                                           aceEventMgr_publishHandle_t* handle);

/**
 * @brief Register an event publisher with additional features
 * @details Extended version of @ref aceEventMgr_registerPublisher which
 * supports additional features such as publish notifications. The handle
 * obtained can be used to publish any event under any group of this module.
 *
 * For ACE modules, the param `module_id` is an unique id defined in
 * ace/ace_modules.h. Please refer to the module's user guide and doxygen for
 * information about the id.
 *
 * For applications, the param `module_id` is defined in application's
 * own headers. Please refer to @ref EVENTMGR_USER_GUIDE under
 * `Application event definition` section for information about defining the id.
 *
 * @note Use @ref aceEventMgr_setPublishNotifications to set up `params`.
 *
 * @param[in]  module_id Module id of the publisher.
 * @param[in]  params    Publisher params that contains callback information for
 *                       notifying after all subscribers receive the published
 *                       event.
 * @param[out] handle    Handle to the publisher context to be used for
 *                       publishing events.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_registerPublisherEx(
    aceModules_moduleId_t module_id, aceEventMgr_publisherParams_t* params,
    aceEventMgr_publishHandle_t* handle);

/**
 * @brief Set up publish notifications params
 * @details Helper function to set up publisher params for
 * @ref aceEventMgr_registerPublisherEx.
 *
 * @code
 *  ace_status_t status = aceEventMgr_setPublishNotifications(
 *           publish_notify_cb, &pubs_ctx,
 *           ACE_EVENTMGR_PUBLISHER_FLAGS_PUBLISH_COMPLETE, &publisher_params);
 *
 *  status = aceEventMgr_registerPublisherEx(
 *                         TEST_MODULE01, &publisher_params, &pub_handle);
 * @endcode
 *
 * @warning The callback function is treated like an ISR. It should use least
 * amount of CPU cycles and stack resources. For stack usage, the maximum
 * allowed value is 1024 bytes.
 *
 * @param[in]  func    Callback function for notification.
 * @param[in]  ctx     User specified context.
 * @param[in]  flags   Supported publisher notification features e.g.
 *                     @ref ACE_EVENTMGR_PUBLISHER_FLAGS_PUBLISH_COMPLETE.
 * @param[out] params  Pointer to @ref aceEventMgr_publisherParams_t to be used
 *                     in @ref aceEventMgr_registerPublisherEx. Zero initialize
 *                     the struct before passing in.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_setPublishNotifications(
    aceEventMgr_callbackFunc_t func, void* ctx,
    aceEventMgr_publisherFlags_t flags, aceEventMgr_publisherParams_t* params);

/**
 * @brief Deregister an event publisher
 *
 * @param[in] handle Handle to the publisher context.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_deregisterPublisher(
    aceEventMgr_publishHandle_t* handle);

/**
 * @brief Register an event subscriber
 * @details Register an event subscriber callback per group of a module. The
 * handle obtained can be used to subscribe to only one group of the module.
 *
 * For ACE modules, the param `module_id` is an unique id defined in
 * ace/ace_modules.h. Please refer to the module's user guide and doxygen for
 * information about the id.
 *
 * For applications, the param `module_id` is defined in application's
 * own headers. Please refer to @ref EVENTMGR_USER_GUIDE under
 * `Application event definition` section for information about defining the id.
 *
 * @warning `EventMgr` _does not_ filter out callback functions if the same
 * function is registered more than once. Each registered instance will receive
 * a call back when the subscribed event is published.
 *
 * @note Use @ref aceEventMgr_setCallbackSubscribeParams to set up `params`.
 *
 * @param[in]  module_id Module id of the module publishing the event.
 * @param[in]  params    Subscriber callback params that contains the
 *                       information for calling back when an event is
 *                       published.
 * @param[out] handle    Handle to the subscriber context to be used for
 *                       subscribing to events.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_registerSubscriber(
    aceModules_moduleId_t module_id, aceEventMgr_subscribeParams_t* params,
    aceEventMgr_subscribeHandle_t* handle);

/**
 * @brief Set up subscriber callback params
 * @details Helper function to set up subscriber params for
 * @ref aceEventMgr_registerSubscriber.
 *
 * @code
 *  ace_status_t status = aceEventMgr_setCallbackSubscribeParams(
 *           &subs_callback, &subs_ctx, &subs_params);
 *
 *  status = aceEventMgr_registerSubscriber(
 *           TEST_MODULE01, &subscriber_params, &subs_handle);
 * @endcode
 *
 * @warning The callback function is treated like an ISR. It should use least
 * amount of CPU cycles and stack resources. For stack usage, the maximum
 * allowed value is 1024 bytes.
 *
 * @param[in]  func    Callback function.
 * @param[in]  ctx     User specified context.
 * @param[out] params  Pointer to @ref aceEventMgr_subscribeParams_t to be used
 *                     in @ref aceEventMgr_registerSubscriber. Zero initialize
 *                     the struct before passing in.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_setCallbackSubscribeParams(
    aceEventMgr_callbackFunc_t func, void* ctx,
    aceEventMgr_subscribeParams_t* params);

/**
 * @brief Deregister an event subscriber
 *
 * @param[in] handle Handle to the subscriber context.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_deregisterSubscriber(
    aceEventMgr_subscribeHandle_t* handle);

/**
 * @brief Publish an event
 *
 * For ACE modules, the params `group_id` and `event_id` are defined by the
 * module in it's event YAML file. Please refer to @ref EVENTMGR_USER_GUIDE
 * under `Events definition` section for information about defining these ids.
 *
 * For applications, the params `group_id` and `event_id` are defined in
 * application's own headers. Please refer to @ref EVENTMGR_USER_GUIDE under
 * `Application event definition` section for information about defining these
 * ids.
 *
 * @note The length of the data that can be published is limited to
 *       @ref ACE_EVENTMGR_MAX_PUBLISH_DATA_SIZE value to support low memory
 *       devices.
 *
 * @warning Publishers should use the group id and event id to signal events
 *          and avoid sending data unless required. This is because the
 *          library will have to make a copy of the data for every publish.
 *          Such copies are expensive in low memory devices.
 *
 * @note Use @ref aceEventMgr_setPublishParams to set up `params`.
 *
 * @param[in] handle   Handle to the publisher context.
 * @param[in] group_id Group id of the event group.
 * @param[in] event_id Event id.
 * @param[in] params   Params containing data for the published event.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_publish(aceEventMgr_publishHandle_t* handle,
                                 aceEventMgr_groupId_t group_id,
                                 aceEventMgr_eventId_t event_id,
                                 aceEventMgr_publishParams_t* params);

/**
 * @brief Set up publish params
 * @details Helper function to set up publish params for
 * @ref aceEventMgr_publish.
 *
 * @code
 *  ace_status_t status = aceEventMgr_setPublishParams(&data, length,
 *           &publish_params);
 *
 *  status = aceEventMgr_publish(&pub_handle, TEST_GROUP_ID, TEST_MODULE_ID,
 *           &publish_params);
 * @endcode
 *
 * @param[in]  data    Data for the published event.
 * @param[in]  length  Length of `data`. The max length is limited to @ref
                       ACE_EVENTMGR_MAX_PUBLISH_DATA_SIZE value.
 * @param[out] params  Pointer to @ref aceEventMgr_publishParams_t to be used
 *                     in @ref aceEventMgr_publish. Zero initialize
 *                     the struct before passing in.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_setPublishParams(void* data, size_t length,
                                          aceEventMgr_publishParams_t* params);

/**
 * @brief Subscribe for event(s)
 *
 * For ACE modules, the params `group_id` and `event_id` are defined by the
 * module. Please refer to the module's user guide and doxygen for information
 * about these ids.
 *
 * For applications, the params `group_id` and `event_id` are defined in
 * application's own headers. Please refer to application documentation for
 * information about these ids.
 *
 * @param[in] handle    Handle to the subscriber context.
 * @param[in] group_id  Group id of the event group.
 * @param[in] event_ids Event ids.
 *                      Supports multiple event id subscription by ORing event
 *                      ids together. Or, can subscribe to all events supported
 *                      by the group id by passing 0. Ignored if group id is 0.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_subscribe(aceEventMgr_subscribeHandle_t* handle,
                                   aceEventMgr_groupId_t group_id,
                                   aceEventMgr_eventId_t event_ids);

/**
 * @brief Unsubscribe from event(s)
 *
 * For ACE modules, the params `group_id` and `event_id` are defined by the
 * module. Please refer to the module's user guide and doxygen for information
 * about these ids.
 *
 * For applications, the params `group_id` and `event_id` are defined in
 * application's own headers. Please refer to application documentation for
 * information about these ids.
 *
 * @param[in] handle    Handle to the subscriber context.
 * @param[in] group_id  Group id of the event group.
 * @param[in] event_ids Event ids.
 *                      Supports multiple event id unsubscription by ORing event
 *                      ids together. Or, can unsubscribe to all events
 *                      supported by the group id by passing 0. Ignored if
 *                      group id is 0.
 *
 * @return ACE_STATUS_OK on success or an appropriate ACE status error code.
 */
ace_status_t aceEventMgr_unsubscribe(aceEventMgr_subscribeHandle_t* handle,
                                     aceEventMgr_groupId_t group_id,
                                     aceEventMgr_eventId_t event_ids);

/** @} */
#ifdef __cplusplus
}
#endif

#endif  // EVENTMGR_API_H