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
 * Copyright 2020-2021 NXP.
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
#include "bt_hall_gatt_helpers.h"
#include "fsl_trng.h"

#define BLE_ADV_FIELDS_MAX_CNT 6

BTBleAdapterCallbacks_t xBTBleAdapterCallbacks;
extern wiced_bt_cfg_settings_t wiced_bt_cfg_settings;
const void * prvBTGetGattServerInterface();
BTProperties_t xProperties;

static BTStatus_t prvBTBleAdapterInit( const BTBleAdapterCallbacks_t * pxCallbacks );
static BTStatus_t prvBTRegisterBleApp( BTUuid_t * pxAppUuid );
static BTStatus_t prvBTUnregisterBleApp( uint8_t ucAdapterIf );
static BTStatus_t prvBTGetBleAdapterProperty( BTBlePropertyType_t xType );
static BTStatus_t prvBTSetBleAdapterProperty( const BTBleProperty_t * pxProperty );
static BTStatus_t prvBTGetallBleRemoteDeviceProperties( BTBdaddr_t * pxRremoteAddr );
static BTStatus_t prvBTGetBleRemoteDeviceProperty( BTBdaddr_t * pxRemoteAddr,
                                                   BTBleProperty_t xType );
static BTStatus_t prvBTSetBleRemoteDeviceProperty( BTBdaddr_t * pxRemoteAddr,
                                                   const BTBleProperty_t * pxProperty );
static BTStatus_t prvBTScan( bool bStart );
static BTStatus_t prvBTConnect( uint8_t ucAdapterIf,
                                const BTBdaddr_t * pxBdAddr,
                                bool bIsDirect,
                                BTTransport_t ulTransport );
static BTStatus_t prvBTDisconnect( uint8_t ucAdapterIf,
                                   const BTBdaddr_t * pxBdAddr,
                                   uint16_t usConnId );
static BTStatus_t prvBTStartAdv( uint8_t ucAdapterIf );
static BTStatus_t prvBTStopAdv( uint8_t ucAdapterIf );
static BTStatus_t prvBTReadRemoteRssi( uint8_t ucAdapterIf,
                                       const BTBdaddr_t * pxBdAddr );
static BTStatus_t prvBTScanFilterParamSetup( BTGattFiltParamSetup_t xFiltParam );
static BTStatus_t prvBTScanFilterAddRemove( uint8_t ucAdapterIf,
                                            uint32_t ulAction,
                                            uint32_t ulFiltType,
                                            uint32_t ulFiltIndex,
                                            uint32_t ulCompanyId,
                                            uint32_t ulCompanyIdMask,
                                            const BTUuid_t * pxUuid,
                                            const BTUuid_t * pxUuidMask,
                                            const BTBdaddr_t * pxBdAddr,
                                            char cAddrType,
                                            size_t xDataLen,
                                            char * pcData,
                                            size_t xMaskLen,
                                            char * pcMask );
static BTStatus_t prvBTScanFilterEnable( uint8_t ucAdapterIf,
                                         bool bEnable );
static BTTransport_t prvBTGetDeviceType( const BTBdaddr_t * pxBdAddr );
static BTStatus_t prvBTSetAdvData( uint8_t ucAdapterIf,
                                   BTGattAdvertismentParams_t * pxParams,
                                   uint16_t usManufacturerLen,
                                   char * pcManufacturerData,
                                   uint16_t usServiceDataLen,
                                   char * pcServiceData,
                                   BTUuid_t * pxServiceUuid,
                                   size_t xNbServices );
static BTStatus_t prvBTSetAdvRawData( uint8_t ucAdapterIf,
                                      uint8_t * pucData,
                                      uint8_t ucLen );
static BTStatus_t prvBTConnParameterUpdateRequest( const BTBdaddr_t * pxBdAddr,
                                                   uint32_t ulMinInterval,
                                                   uint32_t ulMaxInterval,
                                                   uint32_t ulLatency,
                                                   uint32_t ulTimeout );
static BTStatus_t prvBTSetScanParameters( uint8_t ucAdapterIf,
                                          uint32_t ulScanInterval,
                                          uint32_t ulScanWindow );
static BTStatus_t prvBTMultiAdvEnable( uint8_t ucAdapterIf,
                                       BTGattAdvertismentParams_t * pxAdvParams );
static BTStatus_t prvBTMultiAdvUpdate( uint8_t ucAdapterIf,
                                       BTGattAdvertismentParams_t * pxAdvParams );
static BTStatus_t prvBTMultiAdvSetInstData( uint8_t ucAdapterIf,
                                            bool bSetScanRsp,
                                            bool bIncludeName,
                                            bool bInclTxpower,
                                            uint32_t ulAppearance,
                                            size_t xManufacturerLen,
                                            char * pcManufacturerData,
                                            size_t xServiceDataLen,
                                            char * pcServiceData,
                                            BTUuid_t * pxServiceUuid,
                                            size_t xNbServices );
static BTStatus_t prvBTMultiAdvDisable( uint8_t ucAdapterIf );
static BTStatus_t prvBTBatchscanCfgStorage( uint8_t ucAdapterIf,
                                            uint32_t ulBatchScanFullMax,
                                            uint32_t ulBatchScanTruncMax,
                                            uint32_t ulBatchScanNotifyThreshold );
static BTStatus_t prvBTBatchscanEndBatchScan( uint8_t ucAdapterIf,
                                              uint32_t ulScanMode,
                                              uint32_t ulScanInterval,
                                              uint32_t ulScanWindow,
                                              uint32_t ulAddrType,
                                              uint32_t ulDiscardRule );
static BTStatus_t prvBTBatchscanDisBatchScan( uint8_t ucAdapterIf );
static BTStatus_t prvBTBatchscanReadReports( uint8_t ucAdapterIf,
                                             uint32_t ulScanMode );
static BTStatus_t prvBTSetPreferredPhy( uint16_t usConnId,
                                        uint8_t ucTxPhy,
                                        uint8_t ucRxPhy,
                                        uint16_t usPhyOptions );
static BTStatus_t prvBTReadPhy( uint16_t usConnId,
                                BTReadClientPhyCallback_t xCb );
static const void * prvBTGetGattClientInterface( void );
static BTStatus_t prvBTBleAdapterInit( const BTBleAdapterCallbacks_t * pxCallbacks );

BTBleAdapter_t xBTLeAdapter =
{
    .pxBleAdapterInit                  = prvBTBleAdapterInit,
    .pxRegisterBleApp                  = prvBTRegisterBleApp,
    .pxUnregisterBleApp                = prvBTUnregisterBleApp,
    .pxGetBleAdapterProperty           = prvBTGetBleAdapterProperty,
    .pxSetBleAdapterProperty           = prvBTSetBleAdapterProperty,
    .pxGetallBleRemoteDeviceProperties = prvBTGetallBleRemoteDeviceProperties,
    .pxGetBleRemoteDeviceProperty      = prvBTGetBleRemoteDeviceProperty,
    .pxSetBleRemoteDeviceProperty      = prvBTSetBleRemoteDeviceProperty,
    .pxScan                            = prvBTScan,
    .pxConnect                         = prvBTConnect,
    .pxDisconnect                      = prvBTDisconnect,
    .pxStartAdv                        = prvBTStartAdv,
    .pxStopAdv                         = prvBTStopAdv,
    .pxReadRemoteRssi                  = prvBTReadRemoteRssi,
    .pxScanFilterParamSetup            = prvBTScanFilterParamSetup,
    .pxScanFilterAddRemove             = prvBTScanFilterAddRemove,
    .pxScanFilterEnable                = prvBTScanFilterEnable,
    .pxGetDeviceType                   = prvBTGetDeviceType,
    .pxSetAdvData                      = prvBTSetAdvData,
    .pxSetAdvRawData                   = prvBTSetAdvRawData,
    .pxConnParameterUpdateRequest      = prvBTConnParameterUpdateRequest,
    .pxSetScanParameters               = prvBTSetScanParameters,
    .pxMultiAdvEnable                  = prvBTMultiAdvEnable,
    .pxMultiAdvUpdate                  = prvBTMultiAdvUpdate,
    .pxMultiAdvSetInstData             = prvBTMultiAdvSetInstData,
    .pxMultiAdvDisable                 = prvBTMultiAdvDisable,
    .pxBatchscanCfgStorage             = prvBTBatchscanCfgStorage,
    .pxBatchscanEndBatchScan           = prvBTBatchscanEndBatchScan,
    .pxBatchscanDisBatchScan           = prvBTBatchscanDisBatchScan,
    .pxBatchscanReadReports            = prvBTBatchscanReadReports,
    .pxSetPreferredPhy                 = prvBTSetPreferredPhy,
    .pxReadPhy                         = prvBTReadPhy,
    .ppvGetGattClientInterface         = prvBTGetGattClientInterface,
    .ppvGetGattServerInterface         = prvBTGetGattServerInterface,
};

static BTStatus_t prvBTBleAdapterInit( const BTBleAdapterCallbacks_t * pxCallbacks )
{
    //TODO: Not sure if the stack should be initialized here or by the xBTinterface
     /* set default properties */
    BTStatus_t xStatus = eBTStatusSuccess;

    xProperties.bBondable = true;
    xProperties.bSecureConnectionOnly = true;
    xProperties.xPropertyIO = 0; //No IO

    if( pxCallbacks != NULL )
    {
        xBTBleAdapterCallbacks = *pxCallbacks;
    }
    else
    {
        xStatus = eBTStatusFail;
    }

    return xStatus;
}

BTStatus_t prvBTRegisterBleApp( BTUuid_t * pxAppUuid )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    if (xBTBleAdapterCallbacks.pxRegisterBleAdapterCb != NULL)
    {
        xBTBleAdapterCallbacks.pxRegisterBleAdapterCb( xStatus, 0, pxAppUuid );
    }

    return xStatus;
}

BTStatus_t prvBTUnregisterBleApp( uint8_t ucAdapterIf )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    return xStatus;
}

BTStatus_t prvBTGetBleAdapterProperty( BTBlePropertyType_t xType )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    /*if (xBTBleAdapterCallbacks.pxRegisterBleAdapterCb != NULL)
    {
        xBTBleAdapterCallbacks.pxRegisterBleAdapterCb( eBTStatusSuccess, 0, pxAppUuid );
    }*/

    return xStatus;
}

BTStatus_t prvBTSetBleAdapterProperty( const BTBleProperty_t * pxProperty )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    /*if (xBTBleAdapterCallbacks.pxBleAdapterPropertiesCb != NULL)
    {
        xBTBleAdapterCallbacks.pxBleAdapterPropertiesCb(xStatus, pxProperty->xLen, pxProperty);
    }*/

    return xStatus;
}

BTStatus_t prvBTGetallBleRemoteDeviceProperties( BTBdaddr_t * pxRremoteAddr )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTGetBleRemoteDeviceProperty( BTBdaddr_t * pxRemoteAddr,
                                            BTBleProperty_t xType )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTSetBleRemoteDeviceProperty( BTBdaddr_t * pxRemoteAddr,
                                            const BTBleProperty_t * pxProperty )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTScan( bool bStart )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTConnect( uint8_t ucAdapterIf,
                         const BTBdaddr_t * pxBdAddr,
                         bool bIsDirect,
                         BTTransport_t ulTransport )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTDisconnect( uint8_t ucAdapterIf,
                            const BTBdaddr_t * pxBdAddr,
                            uint16_t usConnId )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    wiced_result_t result;

    result = wiced_bt_gatt_disconnect(usConnId);
    if (result != WICED_BT_SUCCESS)
    {
        xStatus = eBTStatusFail;
    }

    return xStatus;
}

static BTStatus_t prvBTStartAdv( uint8_t ucAdapterIf )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    wiced_result_t result;

    result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH, BLE_ADDR_PUBLIC, NULL);
    if (result != WICED_BT_SUCCESS)
    {
        xStatus = eBTStatusFail;
    }

    if (xBTBleAdapterCallbacks.pxAdvStatusCb != NULL)
    {
        xBTBleAdapterCallbacks.pxAdvStatusCb(xStatus, ucAdapterIf, true);
    }

    return xStatus;
}

static BTStatus_t prvBTStopAdv( uint8_t ucAdapterIf )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    wiced_result_t result;

    result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_OFF, BLE_ADDR_PUBLIC, NULL);
    if (result != WICED_BT_SUCCESS)
    {
        xStatus = eBTStatusFail;
    }

    if (xBTBleAdapterCallbacks.pxAdvStatusCb != NULL)
    {
        xBTBleAdapterCallbacks.pxAdvStatusCb(xStatus, ucAdapterIf, false);
    }

    return xStatus;
}

BTStatus_t prvBTReadRemoteRssi( uint8_t ucAdapterIf,
                                const BTBdaddr_t * pxBdAddr )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTScanFilterParamSetup( BTGattFiltParamSetup_t xFiltParam )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTScanFilterAddRemove( uint8_t ucAdapterIf,
                                     uint32_t ulAction,
                                     uint32_t ulFiltType,
                                     uint32_t ulFiltIndex,
                                     uint32_t ulCompanyId,
                                     uint32_t ulCompanyIdMask,
                                     const BTUuid_t * pxUuid,
                                     const BTUuid_t * pxUuidMask,
                                     const BTBdaddr_t * pxBdAddr,
                                     char cAddrType,
                                     size_t xDataLen,
                                     char * pcData,
                                     size_t xMaskLen,
                                     char * pcMask )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTScanFilterEnable( uint8_t ucAdapterIf,
                                  bool bEnable )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTTransport_t prvBTGetDeviceType( const BTBdaddr_t * pxBdAddr )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

static BTStatus_t prvBTSetAdvData( uint8_t ucAdapterIf,
                                   BTGattAdvertismentParams_t * pxParams,
                                   uint16_t usManufacturerLen,
                                   char * pcManufacturerData,
                                   uint16_t usServiceDataLen,
                                   char * pcServiceData,
                                   BTUuid_t * pxServiceUuid,
                                   size_t xNbServices )
{
    BTStatus_t xStatus                   = eBTStatusSuccess;
    wiced_result_t result                = WICED_BT_SUCCESS;
    wiced_bt_ble_advert_elem_t *adv_elem = NULL;
    uint8_t ble_advertisement_flag_value = (BTM_BLE_GENERAL_DISCOVERABLE_FLAG | BTM_BLE_BREDR_NOT_SUPPORTED);
    uint8_t num_elem                     = 0;
    uint8_t num_scan_elem                = 0;
    uint8_t bt_addr[BD_ADDR_LEN]         = {0};
    status_t bt_addr_gen_status;

    if (pxParams == NULL)
    {
        result = WICED_BT_ERROR;
    }

    if (result == WICED_BT_SUCCESS)
    {
        adv_elem = pvPortMalloc(sizeof(wiced_bt_ble_advert_elem_t) * (BLE_ADV_FIELDS_MAX_CNT + xNbServices));
        if (adv_elem != NULL)
        {
            memset(adv_elem, 0, (sizeof(wiced_bt_ble_advert_elem_t) * (BLE_ADV_FIELDS_MAX_CNT + xNbServices)));
        }
        else
        {
            result = WICED_BT_ERROR;
        }
    }

    if (result == WICED_BT_SUCCESS)
    {
        if (wiced_bt_cfg_settings.device_name != NULL)
        {
            if (pxParams->ucName.xType == BTGattAdvNameShort)
            {
                adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_NAME_SHORT;
                adv_elem[num_elem].len         = pxParams->ucName.ucShortNameLen;
                adv_elem[num_elem].p_data      = (uint8_t *)wiced_bt_cfg_settings.device_name;
                num_elem++;
            }
            else if (pxParams->ucName.xType == BTGattAdvNameComplete)
            {
                adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
                // The BLE stack has a length limit so we cut the name if it's too long
                if (pxParams->ucName.ucShortNameLen > 17)
                {
                    adv_elem[num_elem].len = 17;
                }
                else
                {
                    adv_elem[num_elem].len = pxParams->ucName.ucShortNameLen;
                }
                adv_elem[num_elem].p_data = (uint8_t *)wiced_bt_cfg_settings.device_name;
                num_elem++;
            }
        }

        if (pxParams->bIncludeTxPower)
        {
            adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_TX_POWER;
            adv_elem[num_elem].len         = 1;
            adv_elem[num_elem].p_data      = (uint8_t *)&pxParams->ucTxPower;
            num_elem++;
        }

        if (pxParams->ulAppearance != 0)
        {
            adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_APPEARANCE;
            adv_elem[num_elem].len         = 4;
            adv_elem[num_elem].p_data      = (uint8_t *)&pxParams->ulAppearance;
            num_elem++;
        }

        if (pxParams->bSetScanRsp == false)
        {
            adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_FLAG;
            adv_elem[num_elem].len         = 1;
            adv_elem[num_elem].p_data      = &ble_advertisement_flag_value;
            num_elem++;
        }

        if ((usManufacturerLen > 0) && (pcManufacturerData != NULL))
        {
            adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_MANUFACTURER;
            adv_elem[num_elem].len         = usManufacturerLen;
            adv_elem[num_elem].p_data      = (uint8_t *)pcManufacturerData;
            num_elem++;
        }

        if ((usServiceDataLen > 0) && (pcServiceData != NULL))
        {
            adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_SERVICE_DATA;
            adv_elem[num_elem].len         = usServiceDataLen;
            adv_elem[num_elem].p_data      = (uint8_t *)pcServiceData;
            num_elem++;
        }

        for (num_scan_elem = 0; num_scan_elem < xNbServices; num_scan_elem++)
        {
            if (pxServiceUuid[num_scan_elem].ucType == eBTuuidType128)
            {
                adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_128SRV_COMPLETE;
                adv_elem[num_elem].len         = bt128BIT_UUID_LEN;
                adv_elem[num_elem].p_data      = (uint8_t *)pxServiceUuid[num_scan_elem].uu.uu128;
            }
            else if (pxServiceUuid[num_scan_elem].ucType == eBTuuidType32)
            {
                adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_32SRV_COMPLETE;
                adv_elem[num_elem].len         = 4;
                adv_elem[num_elem].p_data      = (uint8_t *)&pxServiceUuid[num_scan_elem].uu.uu32;
            }
            else if (pxServiceUuid[num_scan_elem].ucType == eBTuuidType16)
            {
                adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_16SRV_COMPLETE;
                adv_elem[num_elem].len         = 2;
                adv_elem[num_elem].p_data      = (uint8_t *)&pxServiceUuid[num_scan_elem].uu.uu16;
            }

            num_elem++;
        }

        if (pxParams->bSetScanRsp)
        {
            result = wiced_bt_ble_set_raw_scan_response_data(num_elem, adv_elem);
        }
        else
        {
            result = wiced_bt_ble_set_raw_advertisement_data(num_elem, adv_elem);
        }
    }

    vPortFree(adv_elem);

    if (result == WICED_BT_SUCCESS)
    {
        if (pxParams->xAddrType == BTAddrTypeRandom)
        {
            bt_addr_gen_status = TRNG_GetRandomData(((TRNG_Type *)TRNG_BASE), bt_addr, BD_ADDR_LEN);
            if (bt_addr_gen_status == kStatus_Success)
            {
                wiced_bt_set_local_bdaddr(bt_addr);
            }
            else
            {
                result = WICED_BT_GATT_ERROR;
                BT_LOGE("Get random MAC for BT failed, %d!\n\r", bt_addr_gen_status);
            }
        }
    }

    if (result != WICED_BT_SUCCESS)
    {
        xStatus = eBTStatusFail;
        BT_LOGE("Set advertisement data fail, code %d!\n\r", result);
    }

    if (xBTBleAdapterCallbacks.pxSetAdvDataCb != NULL)
    {
        xBTBleAdapterCallbacks.pxSetAdvDataCb(xStatus);
    }

    return xStatus;
}

static BTStatus_t prvBTSetAdvRawData( uint8_t ucAdapterIf,
                                      uint8_t * pucData,
                                      uint8_t ucLen )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTConnParameterUpdateRequest( const BTBdaddr_t * pxBdAddr,
                                            uint32_t ulMinInterval,
                                            uint32_t ulMaxInterval,
                                            uint32_t ulLatency,
                                            uint32_t ulTimeout )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTSetScanParameters( uint8_t ucAdapterIf,
                                   uint32_t ulScanInterval,
                                   uint32_t ulScanWindow )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTMultiAdvEnable( uint8_t ucAdapterIf,
                                BTGattAdvertismentParams_t * pxAdvParams )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTMultiAdvUpdate( uint8_t ucAdapterIf,
                                BTGattAdvertismentParams_t * pxAdvParams )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTMultiAdvSetInstData( uint8_t ucAdapterIf,
                                     bool bSetScanRsp,
                                     bool bIncludeName,
                                     bool bInclTxpower,
                                     uint32_t ulAppearance,
                                     size_t xManufacturerLen,
                                     char * pcManufacturerData,
                                     size_t xServiceDataLen,
                                     char * pcServiceData,
                                     BTUuid_t * pxServiceUuid,
                                     size_t xNbServices )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTMultiAdvDisable( uint8_t ucAdapterIf )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTBatchscanCfgStorage( uint8_t ucAdapterIf,
                                     uint32_t ulBatchScanFullMax,
                                     uint32_t ulBatchScanTruncMax,
                                     uint32_t ulBatchScanNotifyThreshold )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTBatchscanEndBatchScan( uint8_t ucAdapterIf,
                                       uint32_t ulScanMode,
                                       uint32_t ulScanInterval,
                                       uint32_t ulScanWindow,
                                       uint32_t ulAddrType,
                                       uint32_t ulDiscardRule )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTBatchscanDisBatchScan( uint8_t ucAdapterIf )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTBatchscanReadReports( uint8_t ucAdapterIf,
                                      uint32_t ulScanMode )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTSetPreferredPhy( uint16_t usConnId,
                                 uint8_t ucTxPhy,
                                 uint8_t ucRxPhy,
                                 uint16_t usPhyOptions )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

BTStatus_t prvBTReadPhy( uint16_t usConnId,
                         BTReadClientPhyCallback_t xCb )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

const void * prvBTGetGattClientInterface()
{
    return NULL;
}

const void * prvGetLeAdapter()
{
    return &xBTLeAdapter;
}
