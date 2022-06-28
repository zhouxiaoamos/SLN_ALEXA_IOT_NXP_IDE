/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <ace/aceBT_log.h>
#include "wiced_bt_gatt.h"
#include "bt_hal_manager_types.h"
#include "sdpdefs.h"
#include "bt_hall_gatt_helpers.h"

__attribute__((section(".ocram_non_cacheable_bss"))) uint8_t ble_gatt_database[BLE_GAT_DATABASE_MAX_SIZE];
__attribute__((section(".ocram_non_cacheable_bss"))) handle_value_database_t handle_values;

static uint32_t database_size = 0;
static uint8_t handle_cnt = 0;

static void generate_handle(uint16_t* handle, wiced_bool_t is_handle_value)
{
    /* If the handle is not 0 we don't need to generate a new one, it has a static value */
    if (*handle == 0)
    {
        /* Generate an unique handle starting from 0x0100
         * All the static values are smaller then 0x0100
         */
        *handle = (uint16_t)(handle_cnt | 0x0100);
        handle_cnt++;

        if (is_handle_value == WICED_TRUE)
        {
            if (handle_values.size < HANDLE_VALUE_DATABASE_SIZE)
            {
                handle_values.data[handle_values.size] = *handle;
                handle_values.size++;
            }
            else
            {
                /* It should never get here if the math is right */
                BT_LOGE("Handle value database full\n\r");
            }
        }
    }
}

/* The ACS upper layer only uses the characteristic handle, but the wiced stack
 * on callbacks uses the value handle so we need a function to get the characteristic handle
 * for that value handle
 */
uint16_t get_characteristic_handle(uint16_t handle)
{
    uint16_t characteristic_handle = handle;

    /* Check is the handle is a value handle */
    for (int i = 0; i < handle_values.size; i++)
    {
        if (handle_values.data[i] == handle)
        {
            /* The value handle is always characteristic_handle + 1 */
            characteristic_handle = handle - 1;
        }
    }

    BT_LOGI("get_characteristic_handle: Receive 0x%x, sent 0x%x", handle, characteristic_handle);
    return characteristic_handle;
}

uint16_t get_value_handle(uint16_t handle)
{
    int16_t value_handle = handle + 1;
    wiced_bool_t is_value_handle = WICED_FALSE;

    /* Check is the handle + 1 is a value handle */
    for (int i = 0; i < handle_values.size; i++)
    {
        if (handle_values.data[i] == value_handle)
        {
            is_value_handle = WICED_TRUE;
            break;
        }
    }

    /* If handle + 1 is a NOT value handle return the handle as it is */
    if (is_value_handle == WICED_FALSE)
    {
        value_handle = value_handle - 1;
    }

    BT_LOGI("get_value_handle: Receive 0x%x, sent 0x%x", handle, value_handle);
    return value_handle;
}

uint8_t add_service_16(uint16_t* handle, uint16_t service, wiced_bool_t is_primary)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    service_uuid16_t service16;
    uint8_t entry_size = sizeof(service16);

    if (handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        service16.handle = *handle;
        service16.perm = LEGATTDB_PERM_READABLE;
        service16.size = 4;
        service16.type = (is_primary == 1) ? GATT_UUID_PRI_SERVICE : GATT_UUID_SEC_SERVICE;
        service16.service = service;
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &service16, entry_size);
        database_size += entry_size;
    }

    return status;
}

uint8_t add_service_128(uint16_t* handle, uint8_t *service, wiced_bool_t is_primary)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    service_uuid128_t service128;
    uint8_t entry_size = sizeof(service128);

    if (service != NULL && handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        service128.handle = *handle;
        service128.perm = LEGATTDB_PERM_READABLE;
        service128.size = 18;
        service128.type = (is_primary == 1) ? GATT_UUID_PRI_SERVICE : GATT_UUID_SEC_SERVICE;
        memcpy(service128.service, service, bt128BIT_UUID_LEN);
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &service128, entry_size);
        database_size += entry_size;
    }

    return status;
}

uint8_t include_serveice16(uint16_t* handle, uint16_t service_handle, uint16_t end_group_handle, uint16_t service)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    include_service_uuid16_t service16;
    uint8_t entry_size = sizeof(service16);

    if (handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        service16.handle = *handle;
        service16.perm = LEGATTDB_PERM_READABLE;
        service16.size = 8;
        service16.gatt_uuid_include_service = GATT_UUID_INCLUDE_SERVICE;
        service16.service_handle = service_handle;
        service16.end_group_handle = end_group_handle;
        service16.service = service;
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &service16, entry_size);
        database_size += entry_size;
    }

    return status;
}

uint8_t include_serveice128(uint16_t* handle, uint16_t service_handle, uint16_t end_group_handle)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    include_service_uuid128_t service128;
    uint8_t entry_size = sizeof(service128);

    if (handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        service128.handle = *handle;
        service128.perm = LEGATTDB_PERM_READABLE;
        service128.size = 6;
        service128.gatt_uuid_include_service = GATT_UUID_INCLUDE_SERVICE;
        service128.service_handle = service_handle;
        service128.end_group_handle = end_group_handle;
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &service128, entry_size);
        database_size += entry_size;
    }

    return status;
}

uint8_t add_characteristic16(uint16_t* handle, uint16_t uuid, uint8_t properties, uint8_t permission)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    characteristic_uuid16_t characteristic16;
    uint8_t entry_size = sizeof(characteristic16);
    uint16_t handle_value = 0;

    if (handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        generate_handle(&handle_value, WICED_TRUE);
        characteristic16.handle = *handle;
        characteristic16.perm = LEGATTDB_PERM_READABLE;
        characteristic16.size = 0x07;
        characteristic16.gatt_uuid_char_declare = GATT_UUID_CHAR_DECLARE;
        characteristic16.properties = properties;
        characteristic16.handle_value = handle_value;
        characteristic16.uuid = uuid;
        characteristic16.handle_value_copy = handle_value;
        characteristic16.permission = permission;
        characteristic16.LEGATTDB_size = LEGATTDB_UUID16_SIZE;
        characteristic16.uuid_copy = uuid;
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &characteristic16, entry_size);
        database_size += entry_size;
    }

    return status;
}

uint8_t add_characteristic128(uint16_t* handle, uint8_t* uuid, uint8_t properties, uint8_t permission)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    characteristic_uuid128_t characteristic128;
    uint8_t entry_size = sizeof(characteristic128);
    uint16_t handle_value = 0;

    if (uuid != NULL && handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        generate_handle(&handle_value, WICED_TRUE);
        characteristic128.handle = *handle;
        characteristic128.perm = LEGATTDB_PERM_READABLE;
        characteristic128.size = 21;
        characteristic128.gatt_uuid_char_declare = GATT_UUID_CHAR_DECLARE;
        characteristic128.properties = properties;
        characteristic128.handle_value = handle_value;
        memcpy(characteristic128.uuid, uuid, bt128BIT_UUID_LEN);
        characteristic128.handle_value_copy = handle_value;
        characteristic128.permission = permission | LEGATTDB_PERM_SERVICE_UUID_128;
        characteristic128.LEGATTDB_size = LEGATTDB_UUID128_SIZE;
        memcpy(characteristic128.uuid_copy, uuid, bt128BIT_UUID_LEN);
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &characteristic128, entry_size);
        database_size += entry_size;
    }

    return status;
}


uint8_t add_characteristic16_writable(uint16_t* handle, uint16_t uuid, uint8_t properties, uint8_t permission)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    characteristic_uuid16_writable_t characteristic16;
    uint8_t entry_size = sizeof(characteristic16);
    uint16_t handle_value = 0;

    if (handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        generate_handle(&handle_value, WICED_TRUE);
        characteristic16.handle = *handle;
        characteristic16.perm = LEGATTDB_PERM_READABLE;
        characteristic16.size = 0x07;
        characteristic16.gatt_uuid_char_declare = GATT_UUID_CHAR_DECLARE;
        characteristic16.properties = properties;
        characteristic16.handle_value = handle_value;
        characteristic16.uuid = uuid;
        characteristic16.handle_value_copy = handle_value;
        characteristic16.permission = permission;
        characteristic16.LEGATTDB_size = LEGATTDB_UUID16_SIZE;
        characteristic16.zero = 0;
        characteristic16.uuid_copy = uuid;
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &characteristic16, entry_size);
        database_size += entry_size;
    }

    return status;
}

uint8_t add_characteristic128_writable(uint16_t* handle, uint8_t* uuid, uint8_t properties, uint8_t permission)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    characteristic_uuid128_writable_t characteristic128;
    uint8_t entry_size = sizeof(characteristic128);
    uint16_t handle_value = 0;

    if (uuid != NULL && handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        generate_handle(&handle_value, WICED_TRUE);
        characteristic128.handle = *handle;
        characteristic128.perm = LEGATTDB_PERM_READABLE;
        characteristic128.size = 21;
        characteristic128.gatt_uuid_char_declare = GATT_UUID_CHAR_DECLARE;
        characteristic128.properties = properties;
        characteristic128.handle_value = handle_value;
        memcpy(characteristic128.uuid, uuid, bt128BIT_UUID_LEN);
        characteristic128.handle_value_copy = handle_value;
        characteristic128.permission = permission | LEGATTDB_PERM_SERVICE_UUID_128;
        characteristic128.LEGATTDB_size = LEGATTDB_UUID128_SIZE;
        characteristic128.zero = 0;
        memcpy(characteristic128.uuid_copy, uuid, bt128BIT_UUID_LEN);
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &characteristic128, entry_size);
        database_size += entry_size;
    }

    return status;
}

uint8_t add_char_descriptor16(uint16_t* handle, uint16_t uuid, uint8_t permission)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    char_descriptor_uuid16_t descriptor16;
    uint8_t entry_size = sizeof(descriptor16);

    if (handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        descriptor16.handle = *handle;
        descriptor16.perm = permission;
        descriptor16.size = LEGATTDB_UUID16_SIZE;
        descriptor16.uuid = uuid;
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &descriptor16, entry_size);
        database_size += entry_size;
    }

    return status;
}

uint8_t add_char_descriptor16_writable(uint16_t* handle, uint16_t uuid, uint8_t permission)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    char_descriptor_uuid16_writable_t descriptor16;
    uint8_t entry_size = sizeof(descriptor16);

    if (handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        descriptor16.handle = *handle;
        descriptor16.perm = permission;
        descriptor16.size = LEGATTDB_UUID16_SIZE;
        descriptor16.zero = 0;
        descriptor16.uuid = uuid;
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &descriptor16, entry_size);
        database_size += entry_size;
    }

    return status;
}


uint8_t add_char_descriptor128(uint16_t* handle, uint8_t* uuid, uint8_t permission)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    char_descriptor_uuid128_t descriptor128;
    uint8_t entry_size = sizeof(descriptor128);

    if (uuid != NULL && handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        descriptor128.handle = *handle;
        descriptor128.perm = permission | LEGATTDB_PERM_SERVICE_UUID_128;
        descriptor128.size = LEGATTDB_UUID128_SIZE;
        memcpy(descriptor128.uuid, uuid, bt128BIT_UUID_LEN);
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &descriptor128, entry_size);
        database_size += entry_size;
    }

    return status;
}

uint8_t add_char_descriptor128_writable(uint16_t* handle, uint8_t* uuid, uint8_t permission)
{
    uint8_t status = WICED_BT_GATT_SUCCESS;
    char_descriptor_uuid128_writable_t descriptor128;
    uint8_t entry_size = sizeof(descriptor128);

    if (uuid != NULL && handle != NULL)
    {
        generate_handle(handle, WICED_FALSE);
        descriptor128.handle = *handle;
        descriptor128.perm = permission | LEGATTDB_PERM_SERVICE_UUID_128;
        descriptor128.size = LEGATTDB_UUID128_SIZE;
        descriptor128.zero = 0;
        memcpy(descriptor128.uuid, uuid, bt128BIT_UUID_LEN);
    }
    else
    {
        status = WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if the entry fits in the database */
    if ( (WICED_BT_GATT_SUCCESS == status) &&
         ((database_size + entry_size) > BLE_GAT_DATABASE_MAX_SIZE) )
    {
        status = WICED_BT_GATT_DB_FULL;
    }

    /* Add it to the database */
    if (WICED_BT_GATT_SUCCESS == status)
    {
        memcpy(ble_gatt_database + database_size, &descriptor128, entry_size);
        database_size += entry_size;
    }

    return status;
}

void init_gatt_database()
{
    /* Reset the size and the handle_cnt */
    database_size = 0;
    handle_cnt = 0;
    handle_values.size = 0;
}

uint8_t parse_permissions(BTCharPermissions_t permissions)
{
    uint8_t wwd_perm = 0x00;

    if (((permissions & eBTPermRead) != 0) || ((permissions & eBTPermReadEncrypted) != 0))
    {
        wwd_perm = wwd_perm | LEGATTDB_PERM_READABLE;
    }

    if (((permissions & eBTPermReadEncryptedMitm) != 0))
    {
        wwd_perm = wwd_perm | LEGATTDB_PERM_AUTH_READABLE;
    }

    if (((permissions & eBTPermWrite) != 0) || ((permissions & eBTPermWriteEncrypted) != 0) ||
       ((permissions & eBTPermWriteSigned) != 0))
    {
        wwd_perm = wwd_perm | LEGATTDB_PERM_WRITE_CMD | LEGATTDB_PERM_WRITE_REQ | LEGATTDB_PERM_RELIABLE_WRITE;
    }

    if (((permissions & eBTPermWriteEncryptedMitm) != 0) || ((permissions & eBTPermWriteSignedMitm) != 0))
    {
        wwd_perm = wwd_perm | LEGATTDB_PERM_WRITABLE | LEGATTDB_PERM_RELIABLE_WRITE;
    }

    return wwd_perm;
}

uint32_t get_datebase_size(void)
{
    return database_size;
}

