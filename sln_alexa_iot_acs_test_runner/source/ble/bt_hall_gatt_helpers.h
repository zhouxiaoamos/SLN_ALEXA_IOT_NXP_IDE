/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _HALL_GATT_HELPERS_H_
#define _HALL_GATT_HELPERS_H_

#include "bt_hal_gatt_types.h"

#define BLE_GAT_DATABASE_MAX_SIZE 512
#define MAX_ATRIBUTE_SIZE 46
#define UUID128SIZE 16
#define HANDLE_VALUE_DATABASE_SIZE BLE_GAT_DATABASE_MAX_SIZE / MAX_ATRIBUTE_SIZE

typedef struct {
    uint16_t data[HANDLE_VALUE_DATABASE_SIZE];
    uint8_t size;
}handle_value_database_t;

typedef struct
{
    BTIOtypes_t xPropertyIO;
    bool bBondable;
    bool bSecureConnectionOnly;
    uint32_t ulMtu;
} BTProperties_t;

#pragma pack (1)

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint16_t type;
    uint16_t service;
}service_uuid16_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint16_t type;
    uint8_t service[UUID128SIZE];
}service_uuid128_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint16_t gatt_uuid_include_service;
    uint16_t service_handle;
    uint16_t end_group_handle;
    uint16_t service;
}include_service_uuid16_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint16_t gatt_uuid_include_service;
    uint16_t service_handle;
    uint16_t end_group_handle;
}include_service_uuid128_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint16_t gatt_uuid_char_declare;
    uint8_t properties;
    uint16_t handle_value;
    uint16_t uuid;
    uint16_t handle_value_copy;
    uint8_t permission;
    uint8_t LEGATTDB_size;
    uint16_t uuid_copy;
}characteristic_uuid16_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint16_t gatt_uuid_char_declare;
    uint8_t properties;
    uint16_t handle_value;
    uint8_t uuid[UUID128SIZE];
    uint16_t handle_value_copy;
    uint8_t permission;
    uint8_t LEGATTDB_size;
    uint8_t uuid_copy[UUID128SIZE];
}characteristic_uuid128_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint16_t gatt_uuid_char_declare;
    uint8_t properties;
    uint16_t handle_value;
    uint16_t uuid;
    uint16_t handle_value_copy;
    uint8_t permission;
    uint8_t LEGATTDB_size;
    uint8_t zero;
    uint16_t uuid_copy;
}characteristic_uuid16_writable_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint16_t gatt_uuid_char_declare;
    uint8_t properties;
    uint16_t handle_value;
    uint8_t uuid[UUID128SIZE];
    uint16_t handle_value_copy;
    uint8_t permission;
    uint8_t LEGATTDB_size;
    uint8_t zero;
    uint8_t uuid_copy[UUID128SIZE];
}characteristic_uuid128_writable_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint8_t zero;
    uint16_t uuid;
}char_descriptor_uuid16_writable_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint16_t uuid;
}char_descriptor_uuid16_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint8_t zero;
    uint8_t uuid[UUID128SIZE];
}char_descriptor_uuid128_writable_t;

typedef struct {
    uint16_t handle;
    uint8_t perm;
    uint8_t size;
    uint8_t uuid[UUID128SIZE];
}char_descriptor_uuid128_t;

uint16_t get_characteristic_handle(uint16_t handle);
uint16_t get_value_handle(uint16_t handle);
uint8_t add_service_16(uint16_t* handle, uint16_t service, wiced_bool_t is_primary);
uint8_t add_service_128(uint16_t* handle, uint8_t *service, wiced_bool_t is_primary);
uint8_t include_serveice16(uint16_t* handle, uint16_t service_handle, uint16_t end_group_handle, uint16_t service);
uint8_t include_serveice128(uint16_t* handle, uint16_t service_handle, uint16_t end_group_handle);
uint8_t add_characteristic16(uint16_t* handle, uint16_t uuid, uint8_t properties, uint8_t permission);
uint8_t add_characteristic128(uint16_t* handle, uint8_t* uuid, uint8_t properties, uint8_t permission);
uint8_t add_characteristic16_writable(uint16_t* handle, uint16_t uuid, uint8_t properties, uint8_t permission);
uint8_t add_characteristic128_writable(uint16_t* handle, uint8_t* uuid, uint8_t properties, uint8_t permission);
uint8_t add_char_descriptor16(uint16_t* handle, uint16_t uuid, uint8_t permission);
uint8_t add_char_descriptor16_writable(uint16_t* handle, uint16_t uuid, uint8_t permission);
uint8_t add_char_descriptor128(uint16_t* handle, uint8_t* uuid, uint8_t permission);
uint8_t add_char_descriptor128_writable(uint16_t* handle, uint8_t* uuid, uint8_t permission);
void init_gatt_database();
uint8_t parse_permissions(BTCharPermissions_t permissions);
uint32_t get_datebase_size();
uint16_t get_connection_id();
wiced_bool_t get_connection_state(void);


#endif /*_HALL_GATT_HELPERS_H_ */
