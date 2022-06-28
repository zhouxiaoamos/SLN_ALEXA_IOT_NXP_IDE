/*
 * Amazon FreeRTOS
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */
/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdio.h>
#include <ace/bt_hal_manager.h>
#include <ace/bt_hal_manager_types.h>
#include <ace/bt_hal_manager_adapter_ble.h>
#include <ace/bt_hal_gatt_server.h>
#include <ace/aceBT_log.h>
#include <bt_hal_manager_types.h>
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_cfg.h"
#include "bt_target.h"
#include "wiced_bt_stack.h"
#include "gattdefs.h"
#include "sdpdefs.h"
#include "wiced_bt_dev.h"
#include "fsl_debug_console.h"
#include "wiced_rtos.h"
#include "platform_imxrt.h"
#include "bt_hall_gatt_helpers.h"
#include "hal_kv_storage.h"

#define KVS_BOND_KEY "bt_bond_keys"

static EventGroupHandle_t xEventGapSemaphore = NULL;


extern wiced_bt_cfg_settings_t wiced_bt_cfg_settings;
extern BTProperties_t xProperties;
extern const wiced_bt_cfg_buf_pool_t wiced_bt_cfg_buf_pools[];

const void * prvBTGetGattServerInterface();
const void * prvGetLeAdapter();

static BTCallbacks_t xBTCallbacks;

BTStatus_t prvBTManagerInit( const BTCallbacks_t * pxCallbacks );
BTStatus_t prvBtManagerCleanup( void );
BTStatus_t prvBTEnable( uint8_t ucGuestMode );
BTStatus_t prvBTDisable();
BTStatus_t prvBTGetAllDeviceProperties();
BTStatus_t prvBTGetDeviceProperty( BTPropertyType_t xType );
BTStatus_t prvBTSetDeviceProperty( const BTProperty_t * pxProperty );
BTStatus_t prvBTGetAllRemoteDeviceProperties( BTBdaddr_t * pxRemoteAddr );
BTStatus_t prvBTGetRemoteDeviceProperty( BTBdaddr_t * pxRemoteAddr,
                                         BTPropertyType_t type );
BTStatus_t prvBTSetRemoteDeviceProperty( BTBdaddr_t * pxRemoteAddr,
                                         const BTProperty_t * property );
BTStatus_t prvBTPair( const BTBdaddr_t * pxBdAddr,
                      BTTransport_t xTransport,
                      bool bCreateBond );
BTStatus_t prvBTCreateBondOutOfBand( const BTBdaddr_t * pxBdAddr,
                                     BTTransport_t xTransport,
                                     const BTOutOfBandData_t * pxOobData );
BTStatus_t prvBTCancelBond( const BTBdaddr_t * pxBdAddr );
BTStatus_t prvBTRemoveBond( const BTBdaddr_t * pxBdAddr );
BTStatus_t prvBTGetConnectionState( const BTBdaddr_t * pxBdAddr,
                                    bool * bConnectionState );
BTStatus_t prvBTPinReply( const BTBdaddr_t * pxBdAddr,
                          uint8_t ucAccept,
                          uint8_t ucPinLen,
                          BTPinCode_t * pxPinCode );
BTStatus_t prvBTSspReply( const BTBdaddr_t * pxBdAddr,
                          BTSspVariant_t xVariant,
                          uint8_t ucAccept,
                          uint32_t ulPasskey );
BTStatus_t prvBTReadEnergyInfo();
BTStatus_t prvBTDutModeConfigure( bool bEnable );
BTStatus_t prvBTDutModeSend( uint16_t usOpcode,
                             uint8_t * pucBuf,
                             size_t xLen );
BTStatus_t prvBTLeTestMode( uint16_t usOpcode,
                            uint8_t * pucBuf,
                            size_t xLen );
BTStatus_t prvBTConfigHCISnoopLog( bool bEnable );
BTStatus_t prvBTConfigClear();
BTStatus_t prvBTReadRssi( const BTBdaddr_t * pxBdAddr );
BTStatus_t prvBTGetTxpower( const BTBdaddr_t * pxBdAddr,
                            BTTransport_t xTransport );
const void * prvGetClassicAdapter();

static BTInterface_t xBTinterface =
{
    .pxBtManagerInit                = prvBTManagerInit,
    .pxBtManagerCleanup             = prvBtManagerCleanup,
    .pxEnable                       = prvBTEnable,
    .pxDisable                      = prvBTDisable,
    .pxGetAllDeviceProperties       = prvBTGetAllDeviceProperties,
    .pxGetDeviceProperty            = prvBTGetDeviceProperty,
    .pxSetDeviceProperty            = prvBTSetDeviceProperty,
    .pxGetAllRemoteDeviceProperties = prvBTGetAllRemoteDeviceProperties,
    .pxGetRemoteDeviceProperty      = prvBTGetRemoteDeviceProperty,
    .pxSetRemoteDeviceProperty      = prvBTSetRemoteDeviceProperty,
    .pxPair                         = prvBTPair,
    .pxCreateBondOutOfBand          = prvBTCreateBondOutOfBand,
    .pxCancelBond                   = prvBTCancelBond,
    .pxRemoveBond                   = prvBTRemoveBond,
    .pxGetConnectionState           = prvBTGetConnectionState,
    .pxPinReply                     = prvBTPinReply,
    .pxSspReply                     = prvBTSspReply,
    .pxReadEnergyInfo               = prvBTReadEnergyInfo,
    .pxDutModeConfigure             = prvBTDutModeConfigure,
    .pxDutModeSend                  = prvBTDutModeSend,
    .pxLeTestMode                   = prvBTLeTestMode,
    .pxConfigHCISnoopLog            = prvBTConfigHCISnoopLog,
    .pxConfigClear                  = prvBTConfigClear,
    .pxReadRssi                     = prvBTReadRssi,
    .pxGetTxpower                   = prvBTGetTxpower,
    .pxGetClassicAdapter            = prvGetClassicAdapter,
    .pxGetLeAdapter                 = prvGetLeAdapter,
};

const BTInterface_t * BTGetBluetoothInterface()
{
    return &xBTinterface;
}

static BTBondState_t bond_state_set(wiced_bt_device_link_keys_t new_link_keys)
{
    BTBondState_t result = eBTbondStateBonded;
    int kvs_read_result;
    ace_status_t kvs_write_result;
    wiced_bt_device_link_keys_t old_link_keys;

    kvs_read_result = aceKeyValueDsHal_get(KVS_BOND_KEY, (void *)&old_link_keys, sizeof(wiced_bt_device_link_keys_t));
    if (kvs_read_result != sizeof(wiced_bt_device_link_keys_t))
    {
        kvs_write_result = aceKeyValueDsHal_set(KVS_BOND_KEY, (const void *)&new_link_keys, sizeof(wiced_bt_device_link_keys_t));
        if (kvs_write_result != ACE_STATUS_OK)
        {
            BT_LOGE("Bond set failed: %d\r\n", kvs_write_result);
            result = eBTbondStateNone;
        }
    }
    else
    {
        if (memcmp(&old_link_keys, &new_link_keys, sizeof(wiced_bt_device_link_keys_t)) != 0)
        {
            kvs_write_result = aceKeyValueDsHal_set(KVS_BOND_KEY, (const void *)&new_link_keys, sizeof(wiced_bt_device_link_keys_t));
            if (kvs_write_result != ACE_STATUS_OK)
            {
                BT_LOGE("Bond set failed: %d\r\n", kvs_write_result);
                result = eBTbondStateNone;
            }
        }
    }

    return result;
}

static BTStatus_t bond_state_get(wiced_bt_device_link_keys_t *link_keys)
{
    BTStatus_t result = eBTStatusSuccess;
    int kvs_read_result;
    wiced_bt_device_link_keys_t tmp_link_keys;

    kvs_read_result = aceKeyValueDsHal_get(KVS_BOND_KEY, (void *)&tmp_link_keys, sizeof(wiced_bt_device_link_keys_t));
    if (kvs_read_result == sizeof(wiced_bt_device_link_keys_t))
    {
        memcpy(link_keys, &tmp_link_keys, sizeof(wiced_bt_device_link_keys_t));
        result = eBTStatusSuccess;
    }
    else
    {
        if (kvs_read_result != ACE_STATUS_FAILURE_UNKNOWN_FILE)
        {
            BT_LOGE("Bond get failed: %d\r\n", kvs_read_result);
        }
        result = eBTStatusFail;
    }

    return result;
}

static BTStatus_t bond_state_clear(void)
{
    BTStatus_t result = eBTStatusSuccess;
    ace_status_t kvs_clear_result;

    kvs_clear_result = aceKeyValueDsHal_remove(KVS_BOND_KEY);
    if (kvs_clear_result == ACE_STATUS_OK)
    {
        result = eBTStatusSuccess;
    }
    else
    {
        BT_LOGE("Bond clear failed: %d\r\n", kvs_clear_result);
        result = eBTStatusFail;
    }

    return result;
}

static BTStatus_t parse_pairing_complete_status(wiced_result_t status)
{
    BTStatus_t result = eBTStatusFail;

    if (status == WICED_SUCCESS)
    {
        result = eBTStatusSuccess;
    }

    return result;
}

wiced_result_t wiced_stack_management_callback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t result = WICED_BT_SUCCESS;
    wiced_bt_dev_ble_pairing_info_t *p_info;
    wiced_bt_ble_advert_mode_t *p_mode;
    uint16_t conn_id;
    BTBondState_t bond_set_result;
    static wiced_bt_device_link_keys_t new_link_keys;
    BTStatus_t bond_state_get_result;

    switch (event)
    {
        /* Bluetooth  stack enabled */
        case BTM_ENABLED_EVT:
            BT_LOGI("wiced_stack_management_callback: BTM_ENABLED_EVT\r\n");
            xEventGroupSetBits(xEventGapSemaphore, (1 << BTM_ENABLED_EVT));
            break;

        case BTM_DISABLED_EVT:
            BT_LOGI("wiced_stack_management_callback: BTM_DISABLED_EVT\r\n");
            xEventGroupSetBits(xEventGapSemaphore, (1 << BTM_DISABLED_EVT));
            break;

        case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
            p_mode = &p_event_data->ble_advert_state_changed;
            BT_LOGI("wiced_stack_management_callback: BTM_BLE_ADVERT_STATE_CHANGED_EVT %d\r\n", (uint8_t)*p_mode);
            break;

        case BTM_SECURITY_REQUEST_EVT:
            BT_LOGI("wiced_stack_management_callback: BTM_SECURITY_REQUEST_EVT\r\n");
            if (xBTCallbacks.pxSspRequestCb != NULL)
            {
                xBTCallbacks.pxSspRequestCb((BTBdaddr_t * )p_event_data->security_request.bd_addr, NULL, 0, eBTsspVariantConsent, 0);
            }
            wiced_bt_ble_security_grant(p_event_data->security_request.bd_addr, WICED_BT_SUCCESS);
            break;

        case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
            BT_LOGI("wiced_stack_management_callback: BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT\r\n");
            p_event_data->pairing_io_capabilities_ble_request.local_io_cap = BTM_IO_CAPABILITIES_BLE_DISPLAY_AND_KEYBOARD_INPUT;
            p_event_data->pairing_io_capabilities_ble_request.oob_data     = BTM_OOB_NONE;
            p_event_data->pairing_io_capabilities_ble_request.auth_req     = BTM_LE_AUTH_REQ_SC_MITM_BOND;
            p_event_data->pairing_io_capabilities_ble_request.max_key_size = 0x10;
            p_event_data->pairing_io_capabilities_ble_request.init_keys    = BTM_LE_KEY_PENC | BTM_LE_KEY_PID;
            p_event_data->pairing_io_capabilities_ble_request.resp_keys    = BTM_LE_KEY_PENC | BTM_LE_KEY_PID;
            break;

        case BTM_USER_CONFIRMATION_REQUEST_EVT:
            BT_LOGI("wiced_stack_management_callback: BTM_USER_CONFIRMATION_REQUEST_EVT just works: %d  key: %ld\r\n",
                    p_event_data->user_confirmation_request.just_works, p_event_data->user_confirmation_request.numeric_value);
            if (xBTCallbacks.pxSspRequestCb != NULL)
            {
                xBTCallbacks.pxSspRequestCb((BTBdaddr_t * )p_event_data->user_confirmation_request.bd_addr, NULL, 0, eBTsspVariantPasskeyConfirmation, p_event_data->user_confirmation_request.numeric_value);
            }
            wiced_bt_dev_confirm_req_reply(WICED_BT_SUCCESS, p_event_data->user_confirmation_request.bd_addr);
            break;

        case BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
            BT_LOGI("wiced_stack_management_callback: BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT\r\n");
            bond_state_get_result = bond_state_get(&p_event_data->paired_device_link_keys_request);
            if (bond_state_get_result != eBTStatusSuccess)
            {
                result = WICED_BT_ERROR;
            }
            break;

        case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
            BT_LOGI("wiced_stack_management_callback: BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT\r\n");
            memcpy(&new_link_keys, &p_event_data->paired_device_link_keys_update, sizeof(wiced_bt_device_link_keys_t));
            break;

        case BTM_PAIRING_COMPLETE_EVT:
            p_info = &p_event_data->pairing_complete.pairing_complete_info.ble;
            if (xBTCallbacks.pxPairingStateChangedCb != NULL)
            {
                /* Accept only Authenticated Key */
                if (p_info->sec_level == 4)
                {
                    bond_set_result = bond_state_set(new_link_keys);
                    xBTCallbacks.pxPairingStateChangedCb(parse_pairing_complete_status(p_event_data->pairing_complete.pairing_complete_info.ble.status),
                                                     (BTBdaddr_t * )p_event_data->pairing_complete.bd_addr,
                                                     bond_set_result,
                                                     p_info->sec_level,
                                                     p_info->reason);
                }
                else
                {
                    if (get_connection_state())
                    {
                        conn_id = get_connection_id();
                        wiced_bt_gatt_disconnect(conn_id);
                    }

                    xBTCallbacks.pxPairingStateChangedCb(eBTStatusFail,
                                                     (BTBdaddr_t * )p_event_data->pairing_complete.bd_addr,
                                                     eBTbondStateNone,
                                                     eBTSecLevelNoSecurity,
                                                     eBTauthFailInsuffSecurity);
                }
            }
            BT_LOGI("wiced_stack_management_callback: BTM_PAIRING_COMPLETE_EVT BondingStatus: %d, stat: %d, reason: %d, sec: %d, cancel: %d \r\n",
                    p_event_data->pairing_complete.bonding_status,
                    p_info->status,
                    p_info->reason,
                    p_info->sec_level,
                    p_info->is_pair_cancel);
            break;

        default:
            BT_LOGI("wiced_stack_management_callback: unknown %d\r\n", event);
            break;
    }

    return result;
}

BTStatus_t prvBTManagerInit( const BTCallbacks_t * pxCallbacks )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    if (pxCallbacks != NULL)
    {
        xBTCallbacks = *pxCallbacks;
    }
    else
    {
        xStatus = eBTStatusParamInvalid;
    }

    if (xStatus == eBTStatusSuccess)
    {
        if (wiced_bt_cfg_settings.device_name != NULL)
        {
            vPortFree(wiced_bt_cfg_settings.device_name);
            wiced_bt_cfg_settings.device_name = NULL;
        }

        if (xEventGapSemaphore == NULL)
        {
            xEventGapSemaphore = xEventGroupCreate();
            if (xEventGapSemaphore == NULL)
            {
                xStatus = eBTStatusNoMem;
            }
        }
    }

    return xStatus;
}

BTStatus_t prvBtManagerCleanup( void )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    if (wiced_bt_cfg_settings.device_name != NULL)
    {
        vPortFree(wiced_bt_cfg_settings.device_name);
        wiced_bt_cfg_settings.device_name = NULL;
    }

    if (xEventGapSemaphore != NULL)
    {
        vEventGroupDelete(xEventGapSemaphore);
        xEventGapSemaphore = NULL;
    }

    return xStatus;
}

BTStatus_t prvBTEnable( uint8_t ucGuestMode )
{
    BTStatus_t xStatus;
    wiced_result_t xWWDstatus = WICED_BT_SUCCESS;
    static bool initialized = false;

    /* Due to an error in LPUART_RTOS_Deinit(fsl_lpuart_freertos.c) we can not call wiced_bt_stack_deinit.
     * Due to this error, we initialize the bt_stack only once.
     * The error is: xSemaphoreGive(handle->rxSemaphore); triggers configASSERT because the rxSemaphore is
     * not held by the current task.
     */
    if (initialized == false)
    {
        ble_pwr_on();

        xWWDstatus = wiced_bt_stack_init(wiced_stack_management_callback, &wiced_bt_cfg_settings, wiced_bt_cfg_buf_pools);
        if (xWWDstatus == WICED_BT_SUCCESS)
        {
            xEventGroupWaitBits(xEventGapSemaphore, (1 << BTM_ENABLED_EVT), pdTRUE, pdFALSE, portMAX_DELAY);
            initialized = true;
            xStatus = eBTStatusSuccess;
        }
        else
        {
            xStatus = eBTStatusFail;
        }
    }
    else
    {
        xStatus = eBTStatusSuccess;
    }

    if((xStatus == eBTStatusSuccess) && (xBTCallbacks.pxDeviceStateChangedCb != NULL))
    {
        xBTCallbacks.pxDeviceStateChangedCb( eBTstateOn );
    }

    return xStatus;
}

BTStatus_t prvBTDisable()
{
    BTStatus_t xStatus = eBTStatusSuccess;
    uint16_t conn_id;

    if (get_connection_state())
    {
        conn_id = get_connection_id();
        wiced_bt_gatt_disconnect(conn_id);
    }

    /* Due to an error in LPUART_RTOS_Deinit(fsl_lpuart_freertos.c) we can not call wiced_bt_stack_deinit.
     * The error is: xSemaphoreGive(handle->rxSemaphore); triggers configASSERT because the rxSemaphore is
     * not held by the current task.
     */
#if 0
    wiced_result_t xWWDstatus = WICED_BT_SUCCESS;
    xWWDstatus = wiced_bt_stack_deinit();
    if (xWWDstatus == WICED_BT_SUCCESS)
    {
        xEventGroupWaitBits(xEventGapSemaphore, (1 << BTM_DISABLED_EVT), pdTRUE, pdFALSE, portMAX_DELAY);
        ble_pwr_off();
        BT_LOGI("wiced_bt_stack_deinit Success\n\r");
        xStatus = eBTStatusSuccess;
    }
    else
    {
        BT_LOGI("wiced_bt_stack_deinit Fail\n\r");
        xStatus = eBTStatusFail;
    }
#endif

    if((xStatus == eBTStatusSuccess) && (xBTCallbacks.pxDeviceStateChangedCb != NULL))
    {
        xBTCallbacks.pxDeviceStateChangedCb(eBTstateOff);
    }

    return xStatus;
}


BTStatus_t prvGetBondableDeviceList( void )
{
    return eBTStatusUnsupported;
}

BTStatus_t prvBTGetAllDeviceProperties()
{
    return eBTStatusUnsupported;
}

BTStatus_t prvBTGetDeviceProperty( BTPropertyType_t xType )
{
    uint8_t* name = NULL;
    BTStatus_t xStatus = eBTStatusFail;
    BTProperty_t xReturnedProperty;

    wiced_bt_device_link_keys_t bond_key;

    xReturnedProperty.xType = xType;
    switch( xType )
    {
        case eBTpropertyAdapterBondedDevices:
            xStatus = bond_state_get(&bond_key);
            if (xStatus == eBTStatusSuccess)
            {
                xReturnedProperty.xLen  = sizeof(bond_key.bd_addr);
                xReturnedProperty.pvVal = bond_key.bd_addr;
            }
            else
            {
                xReturnedProperty.xLen  = 0;
                xReturnedProperty.pvVal = NULL;
            }
            xStatus = eBTStatusSuccess;
            break;

        case eBTpropertyBdname:
            xStatus = eBTStatusSuccess;
            name = wiced_bt_cfg_settings.device_name;

            if( name )
            {
                xReturnedProperty.xLen = strlen((const char*)name);
                xReturnedProperty.xType = eBTpropertyBdname;
                xReturnedProperty.pvVal = ( void * ) name;
            }
            else
            {
                xStatus = eBTStatusFail;
            }
            break;

        case eBTpropertyBdaddr:
            xStatus = eBTStatusSuccess;
            wiced_bt_device_address_t bd_addr;
            wiced_bt_dev_read_local_addr (bd_addr);
            xReturnedProperty.xLen = BD_ADDR_LEN;
            xReturnedProperty.xType = eBTpropertyBdaddr;
            xReturnedProperty.pvVal = bd_addr;
            break;

        case eBTpropertyTypeOfDevice:
            xStatus = eBTStatusUnsupported;
            break;

        case eBTpropertyLocalMTUSize:
            xStatus = eBTStatusUnsupported;
            break;

        case eBTpropertyBondable:
            xStatus = eBTStatusSuccess;
            xReturnedProperty.xLen = sizeof( bool );
            xReturnedProperty.xType = eBTpropertyBondable;
            xReturnedProperty.pvVal = ( void * ) &xProperties.bBondable;
            break;

        case eBTpropertyIO:
            xStatus = eBTStatusSuccess;
            xReturnedProperty.xLen = sizeof( BTIOtypes_t );
            xReturnedProperty.xType = eBTpropertyIO;
            xReturnedProperty.pvVal = ( void * ) &xProperties.xPropertyIO;
            break;

        case eBTpropertySecureConnectionOnly:
            xStatus = eBTStatusSuccess;
            xReturnedProperty.xLen = sizeof( bool );
            xReturnedProperty.xType = eBTpropertySecureConnectionOnly;
            xReturnedProperty.pvVal = ( void * ) &xProperties.bSecureConnectionOnly;
            break;

        default:
            xStatus = eBTStatusUnsupported;
    }

    if ((xStatus == eBTStatusSuccess) && (xBTCallbacks.pxAdapterPropertiesCb != NULL))
    {
        xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, &xReturnedProperty);
    }

    return xStatus;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTGetAllRemoteDeviceProperties( BTBdaddr_t * pxRemoteAddr )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTGetRemoteDeviceProperty( BTBdaddr_t * pxRemoteAddr,
                                         BTPropertyType_t type )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTSetRemoteDeviceProperty( BTBdaddr_t * pxRemoteAddr,
                                         const BTProperty_t * property )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTPair( const BTBdaddr_t * pxBdAddr,
                      BTTransport_t xTransport,
                      bool bCreateBond )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTCreateBondOutOfBand( const BTBdaddr_t * pxBdAddr,
                                     BTTransport_t xTransport,
                                     const BTOutOfBandData_t * pxOobData )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTCancelBond( const BTBdaddr_t * pxBdAddr )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTRemoveBond( const BTBdaddr_t * pxBdAddr )
{
    BTStatus_t xStatus;

    xStatus = bond_state_clear();
    if (xBTCallbacks.pxPairingStateChangedCb != NULL)
    {
        xBTCallbacks.pxPairingStateChangedCb(xStatus,
                                             (BTBdaddr_t *)pxBdAddr,
                                             eBTbondStateNone,
                                             eBTSecLevelNoSecurity,
                                             eBTauthSuccess);
    }

    return xStatus;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTGetConnectionState( const BTBdaddr_t * pxBdAddr,
                                    bool * bConnectionState )
{
    return 0;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTPinReply( const BTBdaddr_t * pxBdAddr,
                          uint8_t ucAccept,
                          uint8_t ucPinLen,
                          BTPinCode_t * pxPinCode )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTSspReply( const BTBdaddr_t * pxBdAddr,
                          BTSspVariant_t xVariant,
                          uint8_t ucAccept,
                          uint32_t ulPasskey )
{
    BTStatus_t xStatus;

    /* These are already handled by the BT stack inside wiced_stack_management_callback */
    switch( xVariant )
    {
        case eBTsspVariantPasskeyConfirmation:
        case eBTsspVariantPasskeyEntry:
        case eBTsspVariantConsent:
        case eBTsspVariantPasskeyNotification:
            xStatus = eBTStatusSuccess;
            break;

        default:
            xStatus = eBTStatusFail;
            break;
    }

    return xStatus;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTReadEnergyInfo()
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTDutModeConfigure( bool bEnable )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTDutModeSend( uint16_t usOpcode,
                             uint8_t * pucBuf,
                             size_t xLen )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTLeTestMode( uint16_t usOpcode,
                            uint8_t * pucBuf,
                            size_t xLen )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTConfigHCISnoopLog( bool bEnable )
{
    return eBTStatusUnsupported;
}


/*-----------------------------------------------------------*/

BTStatus_t prvBTConfigClear()
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

void rssi_callback(void *p_data)
{
    BTStatus_t xStatus = eBTStatusSuccess;
    wiced_bt_dev_rssi_result_t *result = p_data;
    BTBdaddr_t pxBda;
    uint32_t ulRssi = 0;

    if (result != NULL)
    {
        memcpy(pxBda.ucAddress, result->rem_bda, btADDRESS_LEN);
        ulRssi = result->rssi;

        if (result->status != WICED_BT_SUCCESS)
        {
            xStatus = eBTStatusFail;
        }
    }
    else
    {
        xStatus = eBTStatusFail;
    }

    if (xBTCallbacks.pxReadRssiCb != NULL)
    {
        xBTCallbacks.pxReadRssiCb(&pxBda, ulRssi, xStatus);
    }
}

BTStatus_t prvBTReadRssi( const BTBdaddr_t * pxBdAddr )
{
    wiced_result_t status;
    BTStatus_t xStatus = eBTStatusSuccess;

    status = wiced_bt_dev_read_rssi((uint8_t *)pxBdAddr->ucAddress, BT_TRANSPORT_LE, rssi_callback);

    if (status != WICED_BT_PENDING)
    {
        xStatus = eBTStatusFail;
    }

    return xStatus;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTGetTxpower( const BTBdaddr_t * pxBdAddr,
                            BTTransport_t xTransport )
{
    return eBTStatusUnsupported;
}

/*-----------------------------------------------------------*/

BTStatus_t prvBTSetDeviceProperty( const BTProperty_t * pxProperty )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    switch( pxProperty->xType )
    {
        case eBTpropertyBdname:
            /* Free the previous allocated memory for name */
            if (wiced_bt_cfg_settings.device_name != NULL)
            {
                vPortFree(wiced_bt_cfg_settings.device_name);
                wiced_bt_cfg_settings.device_name = NULL;
            }

            wiced_bt_cfg_settings.device_name = (uint8_t *)pvPortMalloc(pxProperty->xLen + 1);
            if (wiced_bt_cfg_settings.device_name != NULL)
            {
                memcpy(wiced_bt_cfg_settings.device_name, pxProperty->pvVal, pxProperty->xLen);
                wiced_bt_cfg_settings.device_name[pxProperty->xLen] = '\0';
                xStatus = eBTStatusSuccess;
            }
            else
            {
                xStatus = eBTStatusNoMem;
            }
            break;

        case eBTpropertyLocalMTUSize:
            if (pxProperty->xLen == 2)
            {
                wiced_bt_cfg_settings.gatt_cfg.max_mtu_size = ((uint16_t *)pxProperty->pvVal)[0];
                xStatus = eBTStatusSuccess;
            }
            else
            {
                xStatus = eBTStatusParamInvalid;
            }
            break;

        default:
            xStatus = eBTStatusUnsupported;
    }

    if ((xStatus == eBTStatusSuccess) && (xBTCallbacks.pxAdapterPropertiesCb != NULL))
    {
        xBTCallbacks.pxAdapterPropertiesCb(xStatus, 1, (BTProperty_t *)pxProperty);
    }

    return xStatus;
}

/*-----------------------------------------------------------*/

const void * prvGetClassicAdapter()
{
    return NULL;
}
