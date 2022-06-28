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
#include "bt_hall_gatt_helpers.h"

static BTGattServerCallbacks_t xGattServerCb;
static uint8_t ServerIf = 0;
static uint16_t conn_id = 0;
static wiced_bool_t is_connected = WICED_FALSE;
static uint32_t TransId = 0;
static BTBdaddr_t BTAddr;
static uint32_t ulGattServerIFhandle = 0;

extern uint8_t ble_gatt_database[];


static BTStatus_t prvBTRegisterServer( BTUuid_t * pxUuid );
static BTStatus_t prvBTUnregisterServer( uint8_t ucServerIf );
static BTStatus_t prvBTGattServerInit( const BTGattServerCallbacks_t * pxCallbacks );
static BTStatus_t prvAddServiceBlob( uint8_t ucServerIf,
                                     BTService_t * pxService );
static BTStatus_t prvBTConnect( uint8_t ucServerIf,
                                const BTBdaddr_t * pxBdAddr,
                                bool bIsDirect,
                                BTTransport_t xTransport );
static BTStatus_t prvBTDisconnect( uint8_t ucServerIf,
                                   const BTBdaddr_t * pxBdAddr,
                                   uint16_t usConnId );
static BTStatus_t prvBTAddService( uint8_t ucServerIf,
                                   BTGattSrvcId_t * pxSrvcId,
                                   uint16_t usNumHandles );
static BTStatus_t prvBTAddIncludedService( uint8_t ucServerIf,
                                           uint16_t usServiceHandle,
                                           uint16_t usIncludedHandle );
static BTStatus_t prvBTAddCharacteristic( uint8_t ucServerIf,
                                          uint16_t usServiceHandle,
                                          BTUuid_t * pxUuid,
                                          BTCharProperties_t xProperties,
                                          BTCharPermissions_t xPermissions );
static BTStatus_t prvBTSetVal( BTGattResponse_t * pxValue );
static BTStatus_t prvBTAddDescriptor( uint8_t ucServerIf,
                                      uint16_t usServiceHandle,
                                      BTUuid_t * pxUuid,
                                      BTCharPermissions_t xPermissions );
static BTStatus_t prvBTStartService( uint8_t ucServerIf,
                                     uint16_t usServiceHandle,
                                     BTTransport_t xTransport );
static BTStatus_t prvBTStopService( uint8_t ucServerIf,
                                    uint16_t usServiceHandle );
static BTStatus_t prvBTDeleteService( uint8_t ucServerIf,
                                      uint16_t usServiceHandle );
static BTStatus_t prvBTSendIndication( uint8_t ucServerIf,
                                       uint16_t usAttributeHandle,
                                       uint16_t usConnId,
                                       size_t xLen,
                                       uint8_t * pucValue,
                                       bool bConfirm );
static BTStatus_t prvBTSendResponse( uint16_t usConnId,
                                     uint32_t ulTransId,
                                     BTStatus_t xStatus,
                                     BTGattResponse_t * pxResponse );
static BTStatus_t prvConfigureMtu( uint8_t ucServerIf,
                                     uint16_t usMtu );
static BTStatus_t prvRemoveDevicesFromWhiteList( uint8_t ucServerIf,
                                               const BTBdaddr_t * pxBdAddr,
                                               uint32_t ulNumberOfDevices );
static BTStatus_t prvAddDevicesToWhiteList( uint8_t ucServerIf,
                                          const BTBdaddr_t * pxBdAddr,
                                          uint32_t ulNumberOfDevices );
static BTStatus_t prvReconnect( uint8_t ucServerIf,
                              const BTBdaddr_t * pxBdAddr );


static BTGattServerInterface_t xGATTserverInterface =
{
    .pxRegisterServer               = prvBTRegisterServer,
    .pxUnregisterServer             = prvBTUnregisterServer,
    .pxGattServerInit               = prvBTGattServerInit,
    .pxConnect                      = prvBTConnect,
    .pxDisconnect                   = prvBTDisconnect,
    .pxAddServiceBlob               = prvAddServiceBlob,
    .pxAddService                   = prvBTAddService,
    .pxAddIncludedService           = prvBTAddIncludedService,
    .pxAddCharacteristic            = prvBTAddCharacteristic,
    .pxSetVal                       = prvBTSetVal,
    .pxAddDescriptor                = prvBTAddDescriptor,
    .pxStartService                 = prvBTStartService,
    .pxStopService                  = prvBTStopService,
    .pxDeleteService                = prvBTDeleteService,
    .pxSendIndication               = prvBTSendIndication,
    .pxSendResponse                 = prvBTSendResponse,
    .pxRemoveDevicesFromWhiteList   = prvRemoveDevicesFromWhiteList,
    .pxAddDevicesToWhiteList        = prvAddDevicesToWhiteList,
    .pxReconnect                    = prvReconnect,
    .pxConfigureMtu                 = prvConfigureMtu
};

uint16_t get_connection_id()
{
    return conn_id;
}

wiced_bool_t get_connection_state(void)
{
    return is_connected;
}

static wiced_bt_gatt_status_t gatt_server_request_handler(wiced_bt_gatt_attribute_request_t *p_data)
{
    wiced_bt_gatt_status_t result = WICED_BT_GATT_INVALID_PDU;

    BT_LOGI("gatt_server_request_handler. conn %d, type %d\r\n", p_data->conn_id, p_data->request_type);

    switch (p_data->request_type)
    {
        case GATTS_REQ_TYPE_READ:
            if (xGattServerCb.pxRequestReadCb != NULL)
            {
                xGattServerCb.pxRequestReadCb( p_data->conn_id,
                                               TransId, &BTAddr,
                                               get_characteristic_handle(p_data->data.read_req.handle),
                                               p_data->data.read_req.offset );
                result = WICED_BT_GATT_PENDING;
            }
            break;

        case GATTS_REQ_TYPE_WRITE:
            if (xGattServerCb.pxRequestWriteCb != NULL)
            {
                xGattServerCb.pxRequestWriteCb( p_data->conn_id,
                                                TransId,
                                                &BTAddr,
                                                get_characteristic_handle(p_data->data.write_req.handle),
                                                p_data->data.write_req.offset,
                                                p_data->data.write_req.val_len,
                                                true,
                                                p_data->data.write_req.is_prep,
                                                p_data->data.write_req.p_val);
                result = WICED_BT_GATT_PENDING;
            }
            break;

        case GATTS_REQ_TYPE_PREP_WRITE:
            if (xGattServerCb.pxRequestWriteCb != NULL)
            {
                xGattServerCb.pxRequestWriteCb( p_data->conn_id,
                                                TransId,
                                                &BTAddr,
                                                get_characteristic_handle(p_data->data.write_req.handle),
                                                p_data->data.write_req.offset,
                                                p_data->data.write_req.val_len,
                                                true,
                                                p_data->data.write_req.is_prep,
                                                p_data->data.write_req.p_val);
                result = WICED_BT_GATT_SUCCESS;
            }
            break;

        case GATTS_REQ_TYPE_WRITE_EXEC:
            if (xGattServerCb.pxRequestExecWriteCb != NULL)
            {
                xGattServerCb.pxRequestExecWriteCb( p_data->conn_id, TransId, &BTAddr, p_data->data.exec_write);
                result = WICED_BT_GATT_SUCCESS;
            }
            break;

        case GATTS_REQ_TYPE_MTU:
            if (xGattServerCb.pxMtuChangedCb != NULL)
            {
                xGattServerCb.pxMtuChangedCb(p_data->conn_id, p_data->data.mtu);
                result = WICED_BT_GATT_SUCCESS;
            }
            break;

        case GATTS_REQ_TYPE_CONF:
            result =  WICED_BT_GATT_SUCCESS;
            break;

        default:
            break;
    }
    return result;
}

/*
 * Callback for various GATT events.  As this application performs only as a GATT server, some of
 * the events are ommitted.
 */
static wiced_bt_gatt_status_t gatts_callback(wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *p_data)
{
    wiced_bt_gatt_status_t result = WICED_BT_GATT_INVALID_PDU;
    wiced_bt_gatt_connection_status_t* p_status = &p_data->connection_status;

    switch (event)
    {
        case GATT_CONNECTION_STATUS_EVT:
            BT_LOGI("gatts_callback: GATT_CONNECTION_STATUS_EVT %d\r\n", p_status->connected);
            memcpy(BTAddr.ucAddress, p_status->bd_addr, btADDRESS_LEN);
            if (p_status->connected)
            {
                conn_id = p_status->conn_id;
                is_connected = WICED_TRUE;
            }
            else
            {
                conn_id = 0;
                is_connected = WICED_FALSE;
                BT_LOGI("Disconnected ! Reason %d", p_status->reason);
            }

            if (xGattServerCb.pxConnectionCb != NULL)
            {
                xGattServerCb.pxConnectionCb(p_status->conn_id, ServerIf, p_status->connected, &BTAddr);
            }

            result = WICED_BT_GATT_SUCCESS;
            break;

        case GATT_ATTRIBUTE_REQUEST_EVT:
            BT_LOGI("gatts_callback: GATT_ATTRIBUTE_REQUEST_EVT\r\n");
            result = gatt_server_request_handler(&p_data->attribute_request);
            break;

        default:
            BT_LOGI("gatts_callback: default %d\r\n", event);
            break;
    }

    return result;
}


const void * prvBTGetGattServerInterface()
{
    return &xGATTserverInterface;
}

static BTStatus_t prvBTRegisterServer( BTUuid_t * pxUuid )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    if( wiced_bt_gatt_register(gatts_callback) != WICED_BT_GATT_SUCCESS )
    {
        xStatus = eBTStatusFail;
    }

    if (xGattServerCb.pxRegisterServerCb != NULL)
    {
        xGattServerCb.pxRegisterServerCb(xStatus, ServerIf, pxUuid);
    }

    return xStatus;
}

static BTStatus_t prvBTUnregisterServer( uint8_t ucServerIf )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    if( wiced_bt_gatt_register(NULL) != WICED_BT_GATT_SUCCESS )
    {
        xStatus = eBTStatusFail;
    }

    if( xGattServerCb.pxUnregisterServerCb != NULL )
    {
        xGattServerCb.pxUnregisterServerCb( xStatus, ulGattServerIFhandle );
    }

    return xStatus;
}

static BTStatus_t prvBTGattServerInit( const BTGattServerCallbacks_t * pxCallbacks )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    if( pxCallbacks != NULL )
    {
        xGattServerCb = *pxCallbacks;
    }
    else
    {
        xStatus = eBTStatusParamInvalid;
    }

    init_gatt_database();

    return xStatus;
}

static BTStatus_t prvBTConnect( uint8_t ucServerIf,
                         const BTBdaddr_t * pxBdAddr,
                         bool bIsDirect,
                         BTTransport_t xTransport )
{
    return eBTStatusUnsupported;
}

static BTStatus_t prvBTDisconnect( uint8_t ucServerIf,
                            const BTBdaddr_t * pxBdAddr,
                            uint16_t usConnId )
{
    return eBTStatusUnsupported;
}

static BTStatus_t prvBTAddService( uint8_t ucServerIf,
                            BTGattSrvcId_t * pxSrvcId,
                            uint16_t usNumHandles )
{
    wiced_bt_gatt_status_t wwd_status = WICED_BT_GATT_SUCCESS;
    BTStatus_t xStatus = eBTStatusSuccess;
    wiced_bool_t is_primary;
    uint16_t handle = 0;

    if( pxSrvcId->xServiceType == eBTServiceTypePrimary )
    {
        is_primary = true;
    }
    else
    {
        is_primary = false;
    }

    if (pxSrvcId->xId.xUuid.ucType == eBTuuidType16)
    {
        wwd_status = add_service_16(&handle, pxSrvcId->xId.xUuid.uu.uu16, is_primary);
        BT_LOGI("Add service16 %d %02X\n\r", is_primary, (uint8_t)pxSrvcId->xId.xUuid.uu.uu16);
    }
    else if (pxSrvcId->xId.xUuid.ucType == eBTuuidType128)
    {
        wwd_status = add_service_128(&handle, pxSrvcId->xId.xUuid.uu.uu128, is_primary);
        BT_LOGI("Add service128 %d %02X\n\r", is_primary, (uint8_t)pxSrvcId->xId.xUuid.uu.uu128[0]);
    }
    else
    {
        /* We don't support eBTuuidType32 */
        xStatus = eBTStatusUnsupported;
    }

    if( wwd_status != WICED_BT_GATT_SUCCESS )
    {
        xStatus = eBTStatusFail;
    }


    if (xGattServerCb.pxServiceAddedCb != NULL)
    {
        xGattServerCb.pxServiceAddedCb(xStatus, ucServerIf, pxSrvcId, handle);
    }

    return xStatus;
}

static BTStatus_t prvBTAddIncludedService( uint8_t ucServerIf,
                                    uint16_t usServiceHandle,
                                    uint16_t usIncludedHandle )
{
    wiced_bt_gatt_status_t wwd_status = WICED_BT_GATT_SUCCESS;
    BTStatus_t xStatus = eBTStatusSuccess;
    uint16_t handle = 0;

    BT_LOGI("Add Included Service\n\r");
    wwd_status = include_serveice128(&handle, usServiceHandle, usIncludedHandle);

    if( wwd_status != WICED_BT_GATT_SUCCESS )
    {
        xStatus = eBTStatusFail;
    }

    if (xGattServerCb.pxIncludedServiceAddedCb != NULL)
    {
        xGattServerCb.pxIncludedServiceAddedCb(xStatus, ucServerIf, usServiceHandle, usIncludedHandle);
    }
    return xStatus;
}

static BTStatus_t prvBTAddCharacteristic( uint8_t ucServerIf,
                                   uint16_t usServiceHandle,
                                   BTUuid_t * pxUuid,
                                   BTCharProperties_t xProperties,
                                   BTCharPermissions_t xPermissions )
{
    wiced_bt_gatt_status_t wwd_status = WICED_BT_GATT_SUCCESS;
    BTStatus_t xStatus = eBTStatusSuccess;
    uint16_t handle = 0;
    uint8_t wwd_perm = parse_permissions(xPermissions);

    if (pxUuid->ucType == eBTuuidType16)
    {
        if (xPermissions >= eBTPermWrite)
        {
            wwd_status = add_characteristic16_writable(&handle, pxUuid->uu.uu16, xProperties, wwd_perm);
        }
        else
        {
            wwd_status = add_characteristic16(&handle, pxUuid->uu.uu16, xProperties, wwd_perm);
        }
        BT_LOGI("Add Characteristic16 %02X\n\r", (uint8_t)pxUuid->uu.uu16);
    }
    else if (pxUuid->ucType == eBTuuidType128)
    {
        if (xPermissions >= eBTPermWrite)
        {
            wwd_status = add_characteristic128_writable(&handle, pxUuid->uu.uu128, xProperties, wwd_perm);
        }
        else
        {
            wwd_status = add_characteristic128(&handle, pxUuid->uu.uu128, xProperties, wwd_perm);
        }
        BT_LOGI("Add Characteristic128 %02X\n\r", (uint8_t)pxUuid->uu.uu128[0]);
    }
    else
    {
        /* We don't support eBTuuidType32 */
        xStatus = eBTStatusUnsupported;
    }

    if( wwd_status != WICED_BT_GATT_SUCCESS )
    {
        xStatus = eBTStatusFail;
    }

    if (xGattServerCb.pxCharacteristicAddedCb != NULL)
    {
        xGattServerCb.pxCharacteristicAddedCb(xStatus, ucServerIf, pxUuid, usServiceHandle, handle);
    }

    return xStatus;
}

static BTStatus_t prvBTSetVal( BTGattResponse_t * pxValue )
{
    wiced_bt_gatt_status_t xWWDstatus = WICED_BT_GATT_SUCCESS;
    BTStatus_t xStatus = eBTStatusSuccess;
    wiced_bt_gatt_value_t *pWrite;

    pWrite = (wiced_bt_gatt_value_t *)pvPortMalloc(sizeof(wiced_bt_gatt_value_t) + pxValue->xAttrValue.xLen - 1);
    if (pWrite == NULL)
    {
        xStatus = eBTStatusFail;
    }

    if (eBTStatusSuccess == xStatus)
    {
        pWrite->handle = pxValue->usHandle;
        pWrite->offset = pxValue->xAttrValue.usOffset;
        pWrite->len = pxValue->xAttrValue.xLen;
        pWrite->auth_req = GATT_AUTH_REQ_NONE;
        memcpy(pWrite->value, pxValue->xAttrValue.pucValue, pWrite->len);

        xWWDstatus = wiced_bt_gatt_send_write( conn_id, GATT_WRITE_NO_RSP, pWrite );
        if( xWWDstatus != WICED_BT_GATT_SUCCESS )
        {
            xStatus = eBTStatusFail;
        }

        vPortFree(pWrite);
    }

    if (xGattServerCb.pxSetValCallbackCb != NULL)
    {
        xGattServerCb.pxSetValCallbackCb(xStatus, pxValue->usHandle);
    }

    return xStatus;
}

static BTStatus_t prvBTAddDescriptor( uint8_t ucServerIf,
                                      uint16_t usServiceHandle,
                                      BTUuid_t * pxUuid,
                                      BTCharPermissions_t xPermissions )
{
    wiced_bt_gatt_status_t wwd_status = WICED_BT_GATT_SUCCESS;
    BTStatus_t xStatus = eBTStatusSuccess;
    uint16_t handle = 0;
    uint8_t wwd_perm = parse_permissions(xPermissions);

    if (pxUuid->ucType == eBTuuidType16)
    {
        if (xPermissions >= eBTPermWrite)
        {
            wwd_status = add_char_descriptor16_writable(&handle, pxUuid->uu.uu16, wwd_perm);
        }
        else
        {
            wwd_status = add_char_descriptor16(&handle, pxUuid->uu.uu16, wwd_perm);
        }
        BT_LOGI("Add Descriptor16 %02X\n\r", (uint8_t)pxUuid->uu.uu16);
    }
    else if (pxUuid->ucType == eBTuuidType128)
    {
        if (xPermissions >= eBTPermWrite)
        {
            wwd_status = add_char_descriptor128_writable(&handle, pxUuid->uu.uu128, wwd_perm);
        }
        else
        {
            wwd_status = add_char_descriptor128(&handle, pxUuid->uu.uu128, wwd_perm);
        }
        BT_LOGI("Add Descriptor128 %02X\n\r", (uint8_t)pxUuid->uu.uu128[0]);
    }
    else
    {
       /* We don't support eBTuuidType32 */
        xStatus = eBTStatusUnsupported;
    }

    if( wwd_status != WICED_BT_GATT_SUCCESS )
    {
        xStatus = eBTStatusFail;
    }

    if (xGattServerCb.pxDescriptorAddedCb != NULL)
    {
        xGattServerCb.pxDescriptorAddedCb(xStatus, ucServerIf, pxUuid, usServiceHandle, handle);
    }

    return xStatus;
}

static BTStatus_t prvBTStartService( uint8_t ucServerIf,
                                     uint16_t usServiceHandle,
                                     BTTransport_t xTransport )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    wiced_bt_gatt_status_t gatt_status;

    BT_LOGI("Start Service size %ld\n\r", get_datebase_size());
    gatt_status = wiced_bt_gatt_db_init(ble_gatt_database, get_datebase_size());

    if (gatt_status != WICED_BT_GATT_SUCCESS)
    {
        xStatus =  eBTStatusFail;
    }

    if( xGattServerCb.pxServiceStartedCb != NULL )
    {
        xGattServerCb.pxServiceStartedCb(xStatus, ucServerIf, usServiceHandle);
    }

    return xStatus;
}

static BTStatus_t prvBTStopService( uint8_t ucServerIf,
                             uint16_t usServiceHandle )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    wiced_bt_gatt_status_t gatt_status;

    BT_LOGI("Stop Service\n\r");

    gatt_status = wiced_bt_gatt_db_init(NULL, 0);
    if (gatt_status != WICED_BT_GATT_SUCCESS)
    {
        xStatus =  eBTStatusFail;
    }

    if( xGattServerCb.pxServiceStoppedCb != NULL )
    {
        xGattServerCb.pxServiceStoppedCb( xStatus, ucServerIf, usServiceHandle );
    }

    return xStatus;
}

static BTStatus_t prvBTDeleteService( uint8_t ucServerIf,
                               uint16_t usServiceHandle )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    if( xGattServerCb.pxServiceDeletedCb != NULL )
    {
        xGattServerCb.pxServiceDeletedCb( eBTStatusSuccess, ucServerIf, usServiceHandle );
    }

    return xStatus;
}

static BTStatus_t prvBTSendIndication( uint8_t ucServerIf,
                                       uint16_t usAttributeHandle,
                                       uint16_t usConnId,
                                       size_t xLen,
                                       uint8_t * pucValue,
                                       bool bConfirm )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    wiced_bt_gatt_status_t xWWDstatus = WICED_BT_GATT_SUCCESS;

    if (bConfirm)
    {
        xWWDstatus = wiced_bt_gatt_send_indication(usConnId, get_value_handle(usAttributeHandle), xLen, pucValue);
    }
    else
    {
        xWWDstatus = wiced_bt_gatt_send_notification(usConnId, get_value_handle(usAttributeHandle), xLen, pucValue);
    }

    if (xWWDstatus != WICED_BT_GATT_SUCCESS)
    {
        xStatus = eBTStatusFail;
    }

    if (xGattServerCb.pxIndicationSentCb != NULL)
    {
        xGattServerCb.pxIndicationSentCb(usConnId, xStatus);
    }

    return xStatus;
}

static BTStatus_t prvBTSendResponse( uint16_t usConnId,
                                     uint32_t ulTransId,
                                     BTStatus_t xStatus,
                                     BTGattResponse_t * pxResponse )
{
    BTStatus_t xRetStatus = eBTStatusSuccess;
    wiced_bt_gatt_status_t xWWDstatus = WICED_BT_GATT_SUCCESS;

    xWWDstatus = wiced_bt_gatt_send_response( xStatus, usConnId, get_value_handle(pxResponse->xAttrValue.usHandle),
                                              pxResponse->xAttrValue.xLen, pxResponse->xAttrValue.usOffset,
                                              pxResponse->xAttrValue.pucValue);
    
    if (xWWDstatus != WICED_BT_GATT_SUCCESS)
    {
        xRetStatus = eBTStatusFail;
    }

    if (xGattServerCb.pxResponseConfirmationCb != NULL)
    {
        xGattServerCb.pxResponseConfirmationCb(xStatus, pxResponse->usHandle);
    }

    return xRetStatus;
}

static BTStatus_t prvAddServiceBlob( uint8_t ucServerIf,
                              BTService_t * pxService )
{
    return eBTStatusUnsupported;
}

static BTStatus_t prvConfigureMtu( uint8_t ucServerIf,
                                     uint16_t usMtu )
{
    BTStatus_t xRetStatus = eBTStatusSuccess;
    wiced_bt_gatt_status_t xWWDstatus = WICED_BT_GATT_SUCCESS;

    xWWDstatus = wiced_bt_gatt_configure_mtu (conn_id, usMtu);

    if (xWWDstatus != WICED_BT_GATT_SUCCESS)
    {
        xRetStatus = eBTStatusFail;
    }

    return xRetStatus;
}

static BTStatus_t prvRemoveDevicesFromWhiteList( uint8_t ucServerIf,
                                               const BTBdaddr_t * pxBdAddr,
                                               uint32_t ulNumberOfDevices )
{
    return eBTStatusUnsupported;
}

static BTStatus_t prvAddDevicesToWhiteList( uint8_t ucServerIf,
                                          const BTBdaddr_t * pxBdAddr,
                                          uint32_t ulNumberOfDevices )
{
    return eBTStatusUnsupported;
}

static BTStatus_t prvReconnect( uint8_t ucServerIf,
                              const BTBdaddr_t * pxBdAddr )
{
    return eBTStatusUnsupported;
}
