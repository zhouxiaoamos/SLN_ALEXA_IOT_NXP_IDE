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
 * @file bluetooth_ble_gatt_server_api.h
 *
 * @brief ACE Bluetooth GATT Server API definition header file to host GATT
 * Services.
 */

#ifndef BT_BLE_GATT_SERVER_API_H
#define BT_BLE_GATT_SERVER_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <ace/bluetooth_defines.h>
#include <ace/bluetooth_ble_defines.h>
#include <ace/bluetooth_session_api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ACE_BLE_CB_GATT_SERVER GATT Server callbacks
 * @{
 * @ingroup ACE_BLE_CB
 */

/**
 * @brief Callback when the gatt service is added and started\n
 * Triggered on @ref aceBT_bleRegisterGattServer
 *
 * @param[in] gatts_handle gatt instance handle
 * @param[in] status status of the service addition
 */
typedef void (*on_ble_gatt_service_added_callback)(
    aceBT_bleGattInstanceHandle gatts_handle, aceBT_status_t status);

/**
 * @brief Callback when the remote client request GATT characteristics write
 *
 * @param[in] conn_handle connection instance handle
 * @param[in] request_id request transaction id
 * @param[in] prepared_write Delayed write, mostly followed by an execute to
 * commit the write
 * @param[in] resp Response type
 * @param[in] char_value Characteristics value that needs to be written
 * @return None
 */
typedef void (*on_ble_gatt_characteristis_write_req_callback)(
    aceBT_bleConnHandle conn_handle, int request_id, bool prepared_write,
    aceBT_responseType_t resp, aceBT_bleGattCharacteristicsValue_t char_value);

/**
 * @brief Callback is invoked when the remote device need to read data from the
 * gatt server. @ref aceBT_bleGattRecord_t contained in
 * @ref aceBT_bleGattCharacteristicsValue_t contains the uuid to which the read
 * is requested
 *
 * @param[in] conn_handle connection instance handle
 * @param[in] request_id request transaction id
 * @param[in] char_value characteristics value that needs to be read
 * @return None
 */
typedef void (*on_ble_gatt_characteristis_read_req_callback)(
    aceBT_bleConnHandle conn_handle, int request_id,
    aceBT_bleGattCharacteristicsValue_t char_value);

/**
 * @brief Callback when the remote client writes descriptor to update the
 * existing GATT characteristics.\n
 * @ref aceBT_bleGattDescriptor_t contained in
 * @ref aceBT_bleGattCharacteristicsValue_t contains the uuid to which the
 * write is requested
 *
 * @param[in] conn_handle connection instance handle
 * @param[in] request_id request transaction id
 * @param[in] prepared_write Delayed write, mostly follow by an execute to
 * commit the write
 * @param[in] resp Response type
 * @param[in] char_value characteristics value that needs to be read
 * @return None
 */
typedef void (*on_ble_gatt_descriptor_write_req_callback)(
    aceBT_bleConnHandle conn_handle, int request_id, bool prepared_write,
    aceBT_responseType_t resp, aceBT_bleGattCharacteristicsValue_t char_value);

/**
 * @brief Callback when the remote client read descriptor to update the
 * existing GATT characteristics.\n
 * @ref aceBT_bleGattDescriptor_t contained in
 * @ref aceBT_bleGattCharacteristicsValue_t contains the uuid to which the read
 * is requested
 *
 * @param[in] conn_handle connection instance handle
 * @param[in] request_id request transaction id
 * @param[in] desc_value GATT descriptor value
 * @return None
 */
typedef void (*on_ble_gatt_descriptor_read_callback)(
    aceBT_bleConnHandle conn_handle, int request_id,
    aceBT_bleGattDescriptor_t desc_value);

/**
 * @brief Callback when the remote client writes descriptor to update the
 * existing GATT characteristics.\n
 * @ref aceBT_bleGattDescriptor_t contains the @ref aceBT_bleGattRecord_t
 * which had the handle to uniquely identidy the gatt descriptor.
 *
 * @param[in] conn_handle connection instance handle
 * @param[in] request_id request transaction id
 * @param[in] prepared_write Delayed write, mostly follow by an execute to
 * commit the write
 * @param[in] resp Response type
 * @param[in] desc_value GATT descriptor value
 * @return None
 */
typedef void (*on_ble_gatt_descriptor_write_callback)(
    aceBT_bleConnHandle conn_handle, int request_id, bool prepared_write,
    aceBT_responseType_t resp, aceBT_bleGattDescriptor_t desc_value);

/**
 * @brief Callback when the remote client read descriptor to update the
 * existing GATT characteristics.\n
 * @ref aceBT_bleGattDescriptor_t contained in
 * @ref aceBT_bleGattCharacteristicsValue_t contains the uuid to which the read
 * is requested
 *
 * @param[in] conn_handle connection instance handle
 * @param[in] request_id request transaction id
 * @param[in] chars_value GATT characteristics value
 * @return None
 */
typedef void (*on_ble_gatt_descriptor_read_req_callback)(
    aceBT_bleConnHandle conn_handle, int request_id,
    aceBT_bleGattCharacteristicsValue_t chars_value);

/**
 * @brief Callback when the remote client executes the delayed write
 *
 * @param[in] conn_handle connection instance handle
 * @param[in] request_id request transaction id
 * @param[in] execute execute write
 * @return None
 */
typedef void (*on_ble_gatt_execute_write_req_callback)(
    aceBT_bleConnHandle conn_handle, int request_id, bool execute);

/**
 * @brief Callback is invoked when the mtu is changed
 *
 * @param[in] conn_handle connection instance handle
 * @param[in] mtu updated mtu
 * @return None
 */
typedef void (*on_ble_gatt_mtu_changed_callback)(
    aceBT_bleConnHandle conn_handle, int mtu);

/**
 * @brief Callback is invoked when the notification is sent to the remote device
 *
 * @param[in] conn_handle connection instance handle
 * @param[in] status status of notification send operation
 * @return None
 */
typedef void (*on_ble_gatt_notification_sent_callback)(
    aceBT_bleConnHandle conn_handle, aceBT_status_t status);

/**
 * @brief Callback is invoked to notify the GATT server connection status
 *
 * @param[in] conn_handle connection instance handle
 * @param[in] connected Connection state. 0 means disconnected, 1 is connected
 * @return None
 */
typedef void (*on_ble_gatt_conn_indication_callback)(
    aceBT_bleConnHandle conn_handle, int connected);

/**
 * @brief Callback for notifying that there is update on the white list
 *
 * @param[in] p_device Address of the Remote device.
 * @param[in] is_added true if device added false if device not in white list
 */
typedef void (*on_ble_whitelist_changed_callback)(
    const aceBT_bdAddr_t* p_device, bool is_added);

/**
 *  @brief Gatt server Data structures
 */
typedef struct {
    /** Size of the struct */
    size_t size;
    /** Service added callback */
    on_ble_gatt_service_added_callback on_ble_gatts_service_registered_cb;
    /** Characteristic write request callback */
    on_ble_gatt_characteristis_write_req_callback
        on_ble_write_characteristics_cb;
    /** MTU changed callback*/
    on_ble_gatt_mtu_changed_callback on_ble_mtu_changed_cb;
    /** Characteristic read request callback */
    on_ble_gatt_characteristis_read_req_callback
        on_ble_read_characteristics_req_cb;
    /** Execute write request callback */
    on_ble_gatt_execute_write_req_callback on_ble_gatt_execute_write_req_cb;
    /** Descriptor write request callback */
    on_ble_gatt_descriptor_write_req_callback
        on_ble_gatt_descriptor_write_req_cb;
    /** Descriptor read request callback */
    on_ble_gatt_descriptor_read_req_callback on_ble_gatt_descriptor_read_req_cb;
    /** Notification sent callback */
    on_ble_gatt_notification_sent_callback on_ble_notification_sent_cb;
    /** Indication callback */
    on_ble_gatt_conn_indication_callback on_ble_gatt_conn_indication_cb;
    /** Descriptor write callback */
    on_ble_gatt_descriptor_write_callback on_ble_gatt_descriptor_write_cb;
    /** Descriptor read callback */
    on_ble_gatt_descriptor_read_callback on_ble_gatt_descriptor_read_cb;
    /** Whitelist changed callback */
    on_ble_whitelist_changed_callback on_ble_whitelist_changed_cb;
} aceBT_bleGattServerCallbacks_t;

/** @} */

/**
 * @defgroup ACE_BLE_API_GATT_SERVER GATT Server APIs
 * @{
 * @ingroup ACE_BLE_API
 */

/**
 * @brief Utility function to init and create a GATT server.
 *
 * @param[out] gatt_service pointer to gatt service. The pointer memory will be
 * allocated by this API call.
 * @param[in] uuid uuid value
 * @param[in] service_type Primary or secondary service
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_NOMEM if memory allocation fails
 */
aceBT_status_t aceBT_bleCreateGattService(
    aceBT_bleGattsService_t** gatt_service, aceBT_uuid_t uuid,
    aceBT_bleGattServiceType_t service_type);

/**
 * @brief Utility function to Add Included Service
 *
 * @param[in] gatt_service gatt service
 * @param[in] uuid uuid value
 * @param[in] service_type Primary or secondary service
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_NOMEM if memory allocation fails
 */

aceBT_status_t aceBT_bleAddIncludedService(
    aceBT_bleGattsService_t* gatt_service, aceBT_uuid_t uuid,
    aceBT_bleGattServiceType_t service_type);

/**
 * @brief Utility function to add Characteristics
 *
 * @param[in] gatt_service gatt instance handle
 * @param[in] att_prop attribute property
 * @param[in] att_perm attribute permission can be OR-ed
 * @param[in] att_uuid uuid value
 * @param[in] format format of characteristic
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_NOMEM if memory allocation fails
 */
aceBT_status_t aceBT_bleAddCharacteristics(
    aceBT_bleGattsService_t* gatt_service,
    aceBT_bleGattAttributeProperty att_prop,
    aceBT_bleGattAttributePermission att_perm, aceBT_uuid_t att_uuid,
    aceBT_bleGattAttributeFormat format);

/**
 * @brief Utility function to add Characteristics with descriptors
 *
 * @param[in] gatt_service gatt instance handle
 * @param[in] att_prop attribute property
 * @param[in] att_perm attribute permission can be OR-ed
 * @param[in] att_uuid uuid
 * @param[in] format format of characteristic
 * @param[in] descriptor_perm permission of descriptor
 * @param[in] descriptor_uuid descriptor uuid
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_NOMEM if memory allocation fails
 */
aceBT_status_t aceBT_bleAddCharacteristicsWithDesc(
    aceBT_bleGattsService_t* gatt_service,
    aceBT_bleGattAttributeProperty att_prop,
    aceBT_bleGattAttributePermission att_perm, aceBT_uuid_t att_uuid,
    aceBT_bleGattAttributeFormat format,
    aceBT_bleGattAttributePermission descriptor_perm,
    aceBT_uuid_t descriptor_uuid);

/**
 * @brief Utility function to add multiple descriptor array to a Characteristics
 *
 * @param[in] gatt_service gatt instance handle
 * @param[in] att_prop attribute property
 * @param[in] att_perm attribute permission can be OR-ed
 * @param[in] att_uuid uuid value
 * @param[in] format format of characteristic
 * @param[in] desc_arr Array of GATT descriptors
 * @param[in] no_descriptors Number of GATT descriptors
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_NOMEM if memory allocation fails
 */
aceBT_status_t aceBT_bleAddCharacteristicsWithDescArray(
    aceBT_bleGattsService_t* gatt_service,
    aceBT_bleGattAttributeProperty att_prop,
    aceBT_bleGattAttributePermission att_perm, aceBT_uuid_t att_uuid,
    aceBT_bleGattAttributeFormat format, aceBT_bleGattDescriptor_t* desc_arr,
    uint8_t no_descriptors);

/**
 * @brief Utility function to add Characteristics by Handle
 *
 * @param[in] gatt_service gatt instance handle
 * @param[in] att_prop attribute property
 * @param[in] att_uuid uuid value
 * @param[in] handle GATT handle
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_NOMEM if memory allocation fails
 */
aceBT_status_t aceBT_bleAddCharacteristicsByHandle(
    aceBT_bleGattsService_t* gatt_service,
    aceBT_bleGattAttributeProperty att_prop, aceBT_uuid_t att_uuid,
    uint16_t handle);

/**
 * @brief Utility to add Characteristics
 *
 * @param[in] gatt_service gatt instance handle
 * @param[in] value GATT Characteristic value to be added
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_NOMEM if memory allocation fails
 */

aceBT_status_t aceBT_bleAddCharacteristicsByValue(
    aceBT_bleGattsService_t* gatt_service,
    aceBT_bleGattCharacteristicsValue_t value);

/**
 * @brief API to register gatt server. This is a synchronous call and returns
 * the status immediately.\n
 * This function should be called before invoking any gatt interfaces.\n
 * Invoking this api multiple times using the same session would result in
 * error.\n
 * Triggers @ref on_ble_gatt_service_added_callback
 *
 * @param[in] session_handle session handle
 * @param[in] callbacks Pointer to callback struct
 * aceBT_bleGattServerCallbacks_t
 * @param[in] gatt_service gatt service
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_NOMEM if memory allocation fails
 */
aceBT_status_t aceBT_bleRegisterGattServer(
    aceBT_sessionHandle session_handle,
    aceBT_bleGattServerCallbacks_t* callbacks,
    aceBT_bleGattsService_t* gatt_service);

/**
 * @brief API to deregister gatt server. This is a synchronous call returns the
 * status immediately. This function should be called before invoking any gatt
 * interfaces.\n
 * Invoking this API multiple times using the same session would result in
 * error.
 *
 * @param[in] session_handle session handle
 * @return @ref ACEBT_STATUS_SUCCESS if success
 */
aceBT_status_t aceBT_bleDeRegisterGattServer(
    aceBT_sessionHandle session_handle);

/**
 * @brief API to add GATT service. Use the create service help to create the
 * service and add the service to the list of already hosted services.
 *
 * @param[in] session_handle session handle
 * @param[in] gatt_service gatt service of type gatt_service
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_PARM_INVALID if session_handle is invalid
 */
aceBT_status_t aceBT_bleAddGattService(aceBT_sessionHandle session_handle,
                                       aceBT_bleGattsService_t* gatt_service);

/**
 * @brief API to remove GATT service
 *
 * @param[in] gatts_handle gatt instance generated when service is added
 * @return @ref ACEBT_STATUS_SUCCESS if success
 */
aceBT_status_t aceBT_bleRemoveService(aceBT_bleGattInstanceHandle gatts_handle);

/**
 * @brief Function to free one or more GATT service(s) that are allocated
 * in calls to @ref aceBT_bleCreateGattService.
 *
 * @param[in] gatt_services is a pointer to one or more GATT service(s)
 *            to be freed by this API call. The server allocates this memory
 *            and must take care of freeing up the memory.
 * @param[in] no_svc is the number of GATT services to be freed
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_PARM_INVALID if request contains invalid parameters
 */
aceBT_status_t aceBT_bleGattFreeServices(
    aceBT_bleGattsService_t** gatt_services, uint8_t no_svc);

/**
 * @brief API to send a GATT server response. All the parameters related
 * to the service need to be filled before invoking this function.
 * @ref on_ble_gatt_conn_indication_cb will be invoked once a client connects
 * this GATT server, @ref aceBT_bleConnHandle is returned as part of
 * the callback. The connection handle returned should
 * be used to perform any operation on this service.
 * Invoking this api multiple time for the same session would result in error
 *
 * @param[in] session_handle Session handle
 * @param[in] conn_handle Connection instance handle
 * @param[in] response_pdu GATT response PDU
 * @return ACEBT_STATUS_SUCCESS if success
 */
aceBT_status_t aceBT_bleGattSendResponse(aceBT_sessionHandle session_handle,
                                         aceBT_bleConnHandle conn_handle,
                                         aceBT_bleGattResp_t response_pdu);

/**
 * @brief Configure MTU for GATT server  to update the existing GATT
 * characteristics
 *
 * @param[in] gatts_handle gatt instance handle
 * @param[in] max_mtu max MTU - Default is 512
 * @return @ref ACEBT_STATUS_SUCCESS if success
 */
aceBT_status_t aceBT_bleGattConfigureMTU(
    aceBT_bleGattInstanceHandle gatts_handle, uint16_t max_mtu);

/**
 * @brief Notify the characteristice changed
 *
 * @param[in] session_handle Session Handle received from open call
 * @param[in] conn_handle ble connection handle
 * @param[in] characteristics chanacteristic value for which contains the
 * notification
 * @param[in] confirm request a response from the server to send confirmation on
 * receiving the notification
 * @return @ref ACEBT_STATUS_SUCCESS if success
 */
aceBT_status_t aceBT_bleGattNotifyCharacteristicsChanged(
    aceBT_sessionHandle session_handle, aceBT_bleConnHandle conn_handle,
    aceBT_bleGattCharacteristicsValue_t characteristics, bool confirm);

/**
 * @brief API to retrieve the GATT Service pointer handle based on a GATT handle
 * ID.
 *
 * @param[in] session_handle Session Handle received from open call
 * @param[in] gatts_handle gatt instance handle
 * @param[out] gatt_service Pointer to GATT server service handle
 * @return @ref ACEBT_STATUS_SUCCESS if success
 */
aceBT_status_t aceBT_bleGetGattService(aceBT_sessionHandle session_handle,
                                       aceBT_bleGattInstanceHandle gatts_handle,
                                       aceBT_bleGattsService_t** gatt_service);

/**
 * @brief Function to reconnect to the GATT client this would trigger a directed
 * advertisment if the device is added to the white list
 *
 * @param[in] session_handle Session handle received when opening the session
 * @param[in] p_device Pointer to aceBT_bdAddr_t of the device to which the
 * connection needs to be initiated
 * @return @ref ACEBT_STATUS_SUCCESS if success
 */
aceBT_status_t aceBT_bleReconnect(aceBT_sessionHandle session_handle,
                                  aceBT_bdAddr_t* p_device);

/**
 * @brief Function to add the devices to whitelist
 *
 * @param[in] session_handle Session handle received when opening the session
 * @param[in] p_devices Array of addresses of Remote devices
 * @param[in] num_of_devices Number of devices in previous param's pointer array
 * @return @ref ACEBT_STATUS_SUCCESS if success
 */
aceBT_status_t aceBT_bleAddWhiteListDevices(aceBT_sessionHandle session_handle,
                                            aceBT_bdAddr_t* p_devices,
                                            uint32_t num_of_devices);

/**
 * @brief Function to remove the devices to whitelist
 *
 * @param[in] session_handle Session handle received when opening the session
 * @param[in] p_devices Array of addresses of Remote devices
 * @param[in] num_of_devices Number of devices in previous param's pointer array
 * @return @ref ACEBT_STATUS_SUCCESS if success
 */
aceBT_status_t aceBT_bleRemoveWhiteListedDevices(
    aceBT_sessionHandle session_handle, aceBT_bdAddr_t* p_devices,
    uint32_t num_of_devices);

/**
 * @brief Function to disconnect GATT Server
 *
 * @param[in] session_handle BLE Session handle
 * @param[in] conn_handle BLE connection handle
 * @param[in] p_device Pointer to aceBT_bdAddr_t for the device to be
 * disconnected.
 * @return @ref ACEBT_STATUS_SUCCESS if success; otherwise, appropriate
 * aceBT_status_t error.
 */
aceBT_status_t aceBT_bleServerDisconnect(aceBT_sessionHandle session_handle,
                                         aceBT_bleConnHandle conn_handle,
                                         aceBT_bdAddr_t* p_device);

/**
 * @brief Function to retrieve connection handle for a provided BT Mac address
 *
 * @param[in] conn_handle Connection Handle
 * @param[out] p_device MAC Address of remote device
 * @return @ref ACEBT_STATUS_SUCCESS if success; otherwise, appropriate
 * aceBT_status_t error.
 */
aceBT_status_t aceBT_bleGetBdaddr(aceBT_bleConnHandle conn_handle,
                                  aceBT_bdAddr_t* p_device);

/**
 * @brief Utility function to add a characteristic
 *
 * @param[in] gatt_service gatt instance handle
 * @param[in] att_prop attribute property
 * @param[in] att_uuid uuid value
 * @param[in] handle handle for the new characteristic
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_NOMEM if memory allocation fails
 */
aceBT_status_t aceBT_bleAddCharacteristicsByHandle(
    aceBT_bleGattsService_t* gatt_service,
    aceBT_bleGattAttributeProperty att_prop, aceBT_uuid_t att_uuid,
    uint16_t handle);

/**
 * @brief Utility function to update the primary service
 *
 * @param[in,out] gatt_service gatt instance handle
 * @param[in] uuid uuid value
 * @return @ref ACEBT_STATUS_SUCCESS if success
 */
aceBT_status_t aceBT_bleUpdatePrimaryService(
    aceBT_bleGattsService_t* gatt_service, aceBT_uuid_t uuid);

/**
 * @brief Utility function to add characteristic with descriptor service by
 * handle.
 *
 * @param[in] gatt_service gatt instance handle
 * @param[in] att_prop attribute property
 * @param[in] chars_handle handle for the new characteristic
 * @param[in] att_uuid uuid value
 * @param[in] desc_prop Descriptor property
 * @param[in] descriptor_uuid Descriptor uuid value
 * @param[in] desc_handle Descriptor handle
 * @return @ref ACEBT_STATUS_SUCCESS if success
 * @return @ref ACEBT_STATUS_NOMEM if memory allocation fails
 */
aceBT_status_t aceBT_bleAddCharsWithDescByHandle(
    aceBT_bleGattsService_t* gatt_service,
    aceBT_bleGattAttributeProperty att_prop, uint16_t chars_handle,
    aceBT_uuid_t att_uuid, aceBT_bleGattAttributeProperty desc_prop,
    aceBT_uuid_t descriptor_uuid, uint16_t desc_handle);

/** @} */

#ifdef __cplusplus
}
#endif

#endif  // BT_BLE_GATT_SERVER_API_H
