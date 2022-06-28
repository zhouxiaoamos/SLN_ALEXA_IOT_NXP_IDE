/*
 * Copyright 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "FreeRTOSConfig.h"
#include "hal_wifi.h"
#include "hal_device_info.h"
#include "internal_device_info.h"
#include "wiced_bt_cfg.h"
#include "portable.h"

#ifdef FFS_ENABLED
#include "aisv2_app.h"
#endif /* FFS_ENABLED */

#ifdef WICED_BLUETOOTH_PLATFORM
    extern wiced_bt_cfg_settings_t wiced_bt_cfg_settings;
#endif

#define DEVICE_INFO_UNKNWON_VALUE "unknown"
#define DEVICE_INFO_DEFAULT_MAX_LEN 128

#define ACE_DEVICE_INFO_DEV_HW_REV_VALUE "1"

#define MAC_NIBBLE_LENGTH 12

/* Convert MAC format "12:34:56:ab:cd:ef" to "123456ABCDEF" */
static void device_info_convert_wifi_mac_format(char *field_value, aceWifiHal_macAddress_t mac)
{
    uint8_t src_index = 0;
    uint8_t dst_index = 0;

    if (field_value != NULL)
    {
        for (dst_index = 0; dst_index < MAC_NIBBLE_LENGTH; dst_index++)
        {
            if (mac[src_index] >= 'a' && mac[src_index] <= 'z')
            {
                field_value[dst_index] = mac[src_index++] - 32;
            }
            else
            {
                field_value[dst_index] = mac[src_index++];
            }

            if (dst_index % 2 == 1)
            {
                src_index++;
            }
        }
        field_value[MAC_NIBBLE_LENGTH] = '\0';
    }
}

/* Check if the entry already exists in KVS.
 * If the entry does not exist or its value is different from the new one, store the new value,
 * otherwise (the stored value is equal to the new one) do nothing.
 */
static void device_info_set_kvs_entry(acehal_device_info_entry_t key, char *data_write, uint16_t max_entry_len, bool replace)
{
    int rc;
    ace_status_t status;
    char *data_read;

    if (data_write != NULL)
    {
        data_read = (char *)pvPortMalloc(max_entry_len + 1);
        if (data_read != NULL)
        {
            memset(data_read, 0, max_entry_len + 1);
            rc = aceDeviceInfoDsHal_getEntry(key, data_read, max_entry_len + 1);
            if (rc >= 0)
            {
                /* Replace the existing entry if the entry should not be set by the provisioning process
                 * and the already stored value is different from the new one.
                 */
                if (replace == true)
                {
                    if (strncmp(data_read, data_write, max_entry_len) != 0)
                    {
                        status = internal_deviceInfo_prototype_setEntry(key, data_write);
                        if (status != ACE_STATUS_OK)
                        {
                            configPRINTF(("[FAIL] For key %d, setEntry failed with %d\r\n", key, status));
                        }
                    }
                }
            }
            else if (rc == ACE_STATUS_BUFFER_OVERFLOW)
            {
                /* Replace the existing entry if the entry should not be set by the provisioning process.
                 * The already stored value is different because the size differs (overflow occurred while reading).
                 */
                if (replace == true)
                {
                    status = internal_deviceInfo_prototype_setEntry(key, data_write);
                    if (status != ACE_STATUS_OK)
                    {
                        configPRINTF(("[FAIL] For key %d, setEntry failed with %d\r\n", key, status));
                    }
                }
            }
            else if (rc == ACE_STATUS_FAILURE_UNKNOWN_FILE)
            {
                /* If no entry was stored before, store the new value that will be overwritten in need
                 * by the provisioning process.
                 */
                status = internal_deviceInfo_prototype_setEntry(key, data_write);
                if (status != ACE_STATUS_OK)
                {
                    configPRINTF(("[FAIL] For key %d, setEntry failed with %d\r\n", key, status));
                }
            }
            else
            {
                configPRINTF(("[FAIL] For key %d, getEntry failed with %d\r\n", key, rc));
            }
            vPortFree(data_read);
        }
        else
        {
            configPRINTF(("[FAIL] For key %d, data_read pvPortMalloc fail\r\n", key));
        }
    }
    else
    {
        configPRINTF(("[FAIL] For key %d, data_write pointer is NULL\r\n", key));
    }
}

/* Check the provided entry value.
 * If there is no entry or the entry length is too long,
 * write the default "unknown" entry.
 */
static void device_info_check_and_set_kvs_entry(acehal_device_info_entry_t key, char *field_value, uint16_t max_entry_len, bool replace)
{
    char *data_write = NULL;

    data_write = (char *)pvPortMalloc(max_entry_len + 1);
    if (data_write != NULL)
    {
        memset(data_write, 0, max_entry_len + 1);
        if (field_value != NULL)
        {
            if (strlen(field_value) <= max_entry_len)
            {
                strncpy(data_write, field_value, max_entry_len);
            }
            else
            {
                configPRINTF(("[FAIL] key %d field_value length too long %d > %d\r\n", key, strlen(field_value), max_entry_len));
                strncpy(data_write, DEVICE_INFO_UNKNWON_VALUE, max_entry_len);
            }
        }
        else
        {
            /* The value could not be extracted from the component.
             * In case the extraction method was already implemented,
             * another err message has been already displayed.
             */
            strncpy(data_write, DEVICE_INFO_UNKNWON_VALUE, max_entry_len);
        }

        device_info_set_kvs_entry(key, data_write, max_entry_len, replace);
        vPortFree(data_write);
    }
    else
    {
        configPRINTF(("[FAIL] For key %d, data_write pvPortMalloc fail\r\n", key));
    }
}

static void device_info_get_serial(char **serial_number)
{
    /* Device Serial Number entry should be set by the provisioning process.
     * In case the provisioning process was not done yet, fill the entry with "unknown" value.
     */
    if (serial_number != NULL)
    {
        *serial_number = NULL;
    }
}

static void device_info_get_type_id(char **type_id)
{
    /* Device Type ID entry should be set by the provisioning process.
     * In case the provisioning process was not done yet, fill the entry with "unknown" value.
     */
    if (type_id != NULL)
    {
        *type_id = NULL;
    }
}

static void device_info_get_wifi_mac(char **mac)
{
    aceWifiHal_macAddress_t acs_mac;
    aceWifiHal_error_t status;

    if (mac != NULL)
    {
        status = aceWifiHal_getMacAddress(&acs_mac);
        if (status == aceWifiHal_ERROR_SUCCESS)
        {
            *mac = (char *)pvPortMalloc(MAC_NIBBLE_LENGTH + 1);
            if (*mac != NULL)
            {
                memset(*mac, 0, MAC_NIBBLE_LENGTH + 1);
                device_info_convert_wifi_mac_format(*mac, acs_mac);
            }
            else
            {
                configPRINTF(("[FAIL] mac pvPortMalloc fail\r\n"));
            }
        }
        else
        {
            configPRINTF(("[FAIL] aceWifiHal_getMacAddress fail\r\n"));
            *mac = NULL;
        }
    }
}

static void device_info_get_bt_mac(char **bt_mac)
{
    /* TODO */
    if (bt_mac != NULL)
    {
        *bt_mac = NULL;
    }
}

static void device_info_get_mac_secret(char **mac_secret)
{
    /* TODO */
    if (mac_secret != NULL)
    {
        *mac_secret = NULL;
    }
}

static void device_info_get_manufacturing(char **manufacturing)
{
    if (manufacturing != NULL)
    {
#ifdef ACE_HAL_DEVICE_INFO_MANUFACTURER_NAME
        *manufacturing = (char *)pvPortMalloc(strlen(ACE_HAL_DEVICE_INFO_MANUFACTURER_NAME) + 1);
        if (*manufacturing != NULL)
        {
            memcpy(*manufacturing, ACE_HAL_DEVICE_INFO_MANUFACTURER_NAME, strlen(ACE_HAL_DEVICE_INFO_MANUFACTURER_NAME));
            (*manufacturing)[strlen(ACE_HAL_DEVICE_INFO_MANUFACTURER_NAME)] = '\0';
        }
#else
        *manufacturing = NULL;
#endif /* ACE_HAL_DEVICE_INFO_MANUFACTURER_NAME */
    }
}

static void device_info_get_product_id(char **product_id)
{
    /* Product ID (FFS_PID) entry should be set by the provisioning process.
     * In case the provisioning process was not done yet, fill the entry with "unknown" value.
     */
    if (product_id != NULL)
    {
        *product_id = NULL;
    }
}

static void device_info_get_ffs_pin(char **ffs_pin)
{
    /* FFS Pin entry should be set by the provisioning process.
     * In case the provisioning process was not done yet, fill the entry with "unknown" value.
     */
    if (ffs_pin != NULL)
    {
        *ffs_pin = NULL;
    }
}

static void device_info_get_dev_hw_rev(char **dev_hw_pin)
{
    if (dev_hw_pin != NULL)
    {
#ifdef ACE_DEVICE_INFO_DEV_HW_REV_VALUE
        *dev_hw_pin = (char *)pvPortMalloc(strlen(ACE_DEVICE_INFO_DEV_HW_REV_VALUE) + 1);
        if (*dev_hw_pin != NULL)
        {
            memcpy(*dev_hw_pin, ACE_DEVICE_INFO_DEV_HW_REV_VALUE, strlen(ACE_DEVICE_INFO_DEV_HW_REV_VALUE));
            (*dev_hw_pin)[strlen(ACE_DEVICE_INFO_DEV_HW_REV_VALUE)] = '\0';
        }
#else
        *dev_hw_pin = NULL;
#endif /* ACE_DEVICE_INFO_DEV_HW_REV_VALUE */
    }
}

static void device_info_get_dss_pub_key(char **dss_pub_key)
{
    /* Device DSS public key entry should be set by the provisioning process.
     * In case the provisioning process was not done yet, fill the entry with "unknown" value.
     */
    if (dss_pub_key != NULL)
    {
        *dss_pub_key = NULL;
    }
}

static void device_info_get_bt_device_name(char **bt_device_name)
{
    /* BT Device Name entry is currently set by the provisioning process.
     * In case the provisioning process was not done yet,
     * fill the entry with the BT name extracted from the WICED module.
     */
    if (bt_device_name != NULL)
    {
#ifdef WICED_BLUETOOTH_PLATFORM
        if (wiced_bt_cfg_settings.device_name != NULL)
        {
            *bt_device_name = (char *)pvPortMalloc(strlen((char *)wiced_bt_cfg_settings.device_name) + 1);
            if (*bt_device_name != NULL)
            {
                memset(*bt_device_name, 0, strlen((char *)wiced_bt_cfg_settings.device_name) + 1);
                strncpy(*bt_device_name, (char *)wiced_bt_cfg_settings.device_name, strlen((char *)wiced_bt_cfg_settings.device_name));
            }
            else
            {
                configPRINTF(("[FAIL] bt_device_name pvPortMalloc fail\r\n"));
            }
        }
        else
        {
            *bt_device_name = NULL;
        }
#else
        *bt_device_name = NULL;
#endif /* WICED_BLUETOOTH_PLATFORM */
    }
}

static void device_info_client_id(char **client_id)
{
    if (client_id != NULL)
    {
#ifdef AIA_CLIENT_ID
        *client_id = (char *)pvPortMalloc(strlen(AIA_CLIENT_ID) + 1);
        if (*client_id != NULL)
        {
            memcpy(*client_id, AIA_CLIENT_ID, strlen(AIA_CLIENT_ID));
            (*client_id)[strlen(AIA_CLIENT_ID)] = '\0';
        }
#else
        *client_id = NULL;
#endif /* AIA_CLIENT_ID */
    }
}

/* Free the field_value pointer in case it was allocated. */
static void device_info_free(char **field_value)
{
    if (field_value != NULL)
    {
        if (*field_value != NULL)
        {
            vPortFree(*field_value);
            *field_value = NULL;
        }
    }
}

void device_info_set_kvs_entries(void)
{
    char *field_value = NULL;

    /* ACE_DEVICE_INFO_DEVICE_SERIAL */
    device_info_get_serial(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_DEVICE_SERIAL, field_value, DEVICE_INFO_DEVICE_SERIAL_MAX_LEN, false);
    device_info_free(&field_value);

    /* ACE_DEVICE_INFO_DEVICE_TYPE_ID */
    device_info_get_type_id(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_DEVICE_TYPE_ID, field_value, DEVICE_INFO_DEVICE_TYPE_ID_MAX_LEN, false);
    device_info_free(&field_value);

    /* ACE_DEVICE_INFO_WIFI_MAC */
    device_info_get_wifi_mac(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_WIFI_MAC, field_value, DEVICE_INFO_WIFI_MAC_MAX_LEN, true);
    device_info_free(&field_value);

    /* TODO: ACE_DEVICE_INFO_BT_MAC */
    device_info_get_bt_mac(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_BT_MAC, field_value, DEVICE_INFO_BT_MAC_MAX_LEN, true);
    device_info_free(&field_value);

    /* TODO: ACE_DEVICE_INFO_MAC_SECRET */
    device_info_get_mac_secret(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_MAC_SECRET, field_value, DEVICE_INFO_BT_MAC_MAX_LEN, true);
    device_info_free(&field_value);

    /* ACE_DEVICE_INFO_MANUFACTURING */
    device_info_get_manufacturing(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_MANUFACTURING, field_value, DEVICE_INFO_MANUFACTURING_MAX_LEN, true);
    device_info_free(&field_value);

    /* ACE_DEVICE_INFO_PRODUCT_ID */
    device_info_get_product_id(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_PRODUCT_ID, field_value, DEVICE_INFO_PRODUCT_ID_MAX_LEN, false);
    device_info_free(&field_value);

    /* ACE_DEVICE_INFO_FFS_PIN */
    device_info_get_ffs_pin(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_FFS_PIN, field_value, DEVICE_INFO_FFS_PIN_MAX_LEN, false);
    device_info_free(&field_value);

    /* ACE_DEVICE_INFO_DEV_HW_REV */
    device_info_get_dev_hw_rev(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_DEV_HW_REV, field_value, DEVICE_INFO_DEV_HW_REV_MAX_LEN, true);
    device_info_free(&field_value);

    /* ACE_DEVICE_INFO_DSS_PUB_KEY */
    device_info_get_dss_pub_key(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_DSS_PUB_KEY, field_value, DEVICE_INFO_DSS_PUB_KEY_MAX_LEN, false);
    device_info_free(&field_value);

    /* ACE_DEVICE_INFO_BT_DEVICE_NAME */
    device_info_get_bt_device_name(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_BT_DEVICE_NAME, field_value, DEVICE_INFO_BT_DEVICE_NAME_MAX_LEN, false);
    device_info_free(&field_value);

    /* ACE_DEVICE_INFO_CLIENT_ID */
    device_info_client_id(&field_value);
    device_info_check_and_set_kvs_entry(ACE_DEVICE_INFO_CLIENT_ID, field_value, DEVICE_INFO_CLIENT_ID_NAME_MAX_LEN, true);
    device_info_free(&field_value);
}
