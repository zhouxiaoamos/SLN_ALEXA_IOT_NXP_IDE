/*
 * Amazon FreeRTOS
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <event_groups.h>

#include "iot_wifi.h"
#include "aws_wifi_ext.h"
#include "aws_wifi_hal.h"
#include "aws_wifi_log.h"

#ifndef AMAZON_WIFI_HAL_STUB
#include "lwip/err.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#endif

//
// Internal semaphore for higher-layer synchronous calls
//
static SemaphoreHandle_t wifiSemaphore_;
static StaticSemaphore_t wifiSemaphoreBuffer_;
static const TickType_t wifiSemaphoreWaitTicks_ = pdMS_TO_TICKS(wificonfigMAX_SEMAPHORE_WAIT_TIME_MS);

static inline void WIFI_InitSemaphore( void )
{
    wifiSemaphore_ = xSemaphoreCreateBinaryStatic(&wifiSemaphoreBuffer_);
    configASSERT(wifiSemaphore_);

    // Make semaphore available by default
    xSemaphoreGive(wifiSemaphore_);
}

static inline bool WIFI_TakeSemaphore( void )
{
    return xSemaphoreTake(wifiSemaphore_, wifiSemaphoreWaitTicks_) == pdTRUE;
}

static inline WIFIReturnCode_t WIFI_GiveSemaphore( WIFIReturnCode_t retCode )
{
    xSemaphoreGive(wifiSemaphore_);
    return retCode;
}

//
// Event group for synchronization with lower-layer events
//
#define WIFI_EVENT_INIT_DONE         (1 << 0)
#define WIFI_EVENT_SCAN_DONE         (1 << 1)
#define WIFI_EVENT_CONNECTED         (1 << 2)
#define WIFI_EVENT_DISCONNECTED      (1 << 3)
#define WIFI_EVENT_AP_STATE_CHANGED  (1 << 4)
#define WIFI_EVENT_IP_READY          (1 << 5)
#define WIFI_EVENT_CONNECTION_FAILED (1 << 6)

#define MAX_SSID_SEARCH_COUNT 10
static uint32_t gSsidNotFound;
static EventGroupHandle_t wifiEvent_;
static StaticEventGroup_t wifiEventBuffer_;
static const TickType_t wifiEventWaitTicks_ = pdMS_TO_TICKS(wificonfigMAX_EVENT_WAIT_TIME_MS);

static void WIFI_InitEvent( void )
{
    wifiEvent_ = xEventGroupCreateStatic(&wifiEventBuffer_);
    configASSERT(wifiEvent_);
}

static void WIFI_ClearEvent( const EventBits_t bits )
{
    xEventGroupClearBits(wifiEvent_, bits);
}

static void WIFI_PostEvent( const EventBits_t bits )
{
    // Signal that event bits have occured
    xEventGroupSetBits(wifiEvent_, bits);
}

static bool WIFI_WaitEvent( const EventBits_t bits )
{
    EventBits_t waitBits;

    // Wait for any bit in bits. No auto-clear---user must call WIFI_ClearEvent() first!
    waitBits = xEventGroupWaitBits(wifiEvent_, bits, pdFALSE, pdFALSE, wifiEventWaitTicks_);
    return (waitBits & bits) != 0;
}

static EventBits_t WIFI_WaitGroupEvent( const EventBits_t bits )
{
    EventBits_t waitBits;

    // Wait for any bit in bits. No auto-clear---user must call WIFI_ClearEvent() first!
    waitBits = xEventGroupWaitBits(wifiEvent_, bits, pdFALSE, pdFALSE, wifiEventWaitTicks_);
    return waitBits;
}

//
// Internal data structures
//
static WIFIScanResultExt_t* wifiScanResults_;
static uint16_t wifiNumScanResults_;
static WIFINetworkParamsExt_t wifiAPConfig_;

#ifndef AMAZON_WIFI_HAL_STUB
#ifndef WIFI_PING_TIMEOUT_MS
#define WIFI_PING_TIMEOUT_MS   1000
#endif

#ifndef WIFI_PING_DATA_SIZE
#define WIFI_PING_DATA_SIZE  32
#endif

#ifndef WIFI_PING_MAX_DATA_SIZE
#define WIFI_PING_MAX_DATA_SIZE 1400
#endif

#define WIFI_PING_ICMP_HDR_SIZE (sizeof(struct icmp_echo_hdr))
#define WIFI_PING_IP_ICMP_HDR_SIZE (sizeof(struct ip_hdr) + WIFI_PING_ICMP_HDR_SIZE)
static struct timeval wifiPingTimeout_ =
{
    .tv_sec = WIFI_PING_TIMEOUT_MS / 1000,
    .tv_usec = (WIFI_PING_TIMEOUT_MS % 1000) * 1000
};
static const uint16_t wifiPingId_ = 0xAFAF;
#endif

//
// Internal helper functions
//
static WIFIReturnCode_t WIFI_ValidateNetworkParams( const WIFINetworkParamsExt_t * pxNetworkParams )
{
    uint8_t length;
    uint8_t i;

    if (pxNetworkParams->ucSSIDLength > 32)
    {
        AWS_WIFI_LOGE("WIFI ssid length %u invalid\n", pxNetworkParams->ucSSIDLength);
        return eWiFiFailure;
    }

    switch (pxNetworkParams->xSecurity)
    {
        case eWiFiSecurityOpen:
            break;
        case eWiFiSecurityWPA:
        case eWiFiSecurityWPA2:
            length = pxNetworkParams->xPassword.xWPA.ucLength;
            if (length < 8 || length > 64)
            {
                AWS_WIFI_LOGE("WIFI psk length %u invalid\n", length);
                return eWiFiFailure;
            }
            break;
        case eWiFiSecurityWEP:
            for (i = 0; i < wificonfigMAX_WEPKEYS; i++)
            {
                length = pxNetworkParams->xPassword.xWEP[i].ucLength;
                if (!(length == 5 || length == 10 || length == 13 || length == 26))
                {
                    // Not all WEP keys are required, so reject only the used key
                    AWS_WIFI_LOGE("WIFI wep key %u length %u invalid\n", i, length);
                    if (i == pxNetworkParams->ucWEPKeyIndex)
                    {
                        return eWiFiFailure;
                    }
                }
            }
            break;
        default:
            return eWiFiNotSupported;
    }
    return eWiFiSuccess;
}

static WIFIReturnCode_t WIFI_ConvertNetworkParams( const WIFINetworkParams_t * pxNetworkParams,
                                                   WIFINetworkParamsExt_t * pxNetworkParamsExt )
{
    WIFIReturnCode_t retCode;

    if( NULL == pxNetworkParams || NULL == pxNetworkParams->pcSSID )
    {
        return eWiFiFailure;
    }

    if( NULL == pxNetworkParams->pcPassword &&
            eWiFiSecurityOpen != pxNetworkParams->xSecurity )
    {
        return eWiFiFailure;
    }

    // Sanity check to prevent buffer overflow
    if (pxNetworkParams->ucSSIDLength > wificonfigMAX_SSID_LEN ||
        pxNetworkParams->ucPasswordLength > wificonfigMAX_PASSPHRASE_LEN)
    {
        AWS_WIFI_LOGE("WIFI ssid length %d or psk length %d invalid\n",
              pxNetworkParams->ucSSIDLength, pxNetworkParams->ucPasswordLength);
        return eWiFiFailure;
    }
    memset(pxNetworkParamsExt, 0, sizeof(WIFINetworkParamsExt_t));
    memcpy(pxNetworkParamsExt->ucSSID, pxNetworkParams->pcSSID, pxNetworkParams->ucSSIDLength);
    pxNetworkParamsExt->ucSSIDLength = pxNetworkParams->ucSSIDLength;
    pxNetworkParamsExt->xSecurity = pxNetworkParams->xSecurity;
    switch (pxNetworkParams->xSecurity)
    {
        case eWiFiSecurityOpen:
            break;
        case eWiFiSecurityWEP:
            return eWiFiNotSupported;
        case eWiFiSecurityWPA:
        case eWiFiSecurityWPA2:
            if (pxNetworkParams->ucPasswordLength < 8)
            {
                AWS_WIFI_LOGE("WIFI psk length %d invalid\n", pxNetworkParams->ucPasswordLength);
                return eWiFiFailure;
            }
            memcpy(pxNetworkParamsExt->xPassword.xWPA.cPassphrase,
                   pxNetworkParams->pcPassword,
                   pxNetworkParams->ucPasswordLength);
            pxNetworkParamsExt->xPassword.xWPA.ucLength = pxNetworkParams->ucPasswordLength;
            break;
        default:
            return eWiFiFailure;
            break;
    }
    pxNetworkParamsExt->ucChannel = pxNetworkParams->cChannel;
    return eWiFiSuccess;
}

static void WIFI_ShowScanResults( void )
{
#if 0
    WIFIScanResultExt_t* result = wifiScanResults_;
    char ssid[33];
    uint16_t i;

    for (i = 0; i < wifiNumScanResults_; i++, result++)
    {
        if (result->ucSSIDLength > 32)
        {
            AWS_WIFI_LOGD("WIFI ShowScanResults invalid SSID length %u\n", result->ucSSIDLength);
        }
        else
        {
            memset(ssid, 0, sizeof(ssid));
            memcpy(ssid, result->ucSSID, result->ucSSIDLength);
            AWS_WIFI_LOGD("[%2d]: Channel=%2u, BSSID=%02X:%02X:%02X:%02X:%02X:%02X, "
                  "RSSI=%d, Security=%u, Len=%2u, SSID=%s\n",
                  i,
                  result->ucChannel,
                  result->ucBSSID[0],
                  result->ucBSSID[1],
                  result->ucBSSID[2],
                  result->ucBSSID[3],
                  result->ucBSSID[4],
                  result->ucBSSID[5],
                  result->cRSSI,
                  result->xSecurity,
                  result->ucSSIDLength,
                  ssid);
        }
    }
#endif
}

//
// Internal event handlers
//
static WIFIEventHandler_t wifiEventHandlers_[ eWiFiEventMax ];
static bool wifiInitDone_ = false;

static void WIFI_InitDoneCallback( WIFIEvent_t * xEvent )
{

    wifiInitDone_ = true;

    WIFI_PostEvent(WIFI_EVENT_INIT_DONE);
    if (wifiEventHandlers_[eWiFiEventReady])
    {
        wifiEventHandlers_[eWiFiEventReady](xEvent);
    }
}

static void WIFI_ScanDoneCallback( WIFIEvent_t * xEvent )
{
    WIFIReturnCode_t retCode;

    wifiScanResults_ = xEvent->xInfo.xScanDone.pxScanResults;
    wifiNumScanResults_ = xEvent->xInfo.xScanDone.usNumScanResults;
    WIFI_ShowScanResults();

    AWS_WIFI_LOGI("%s scan results num = %d\n", __FUNCTION__, wifiNumScanResults_);

    WIFI_PostEvent(WIFI_EVENT_SCAN_DONE);
    if (wifiEventHandlers_[eWiFiEventScanDone])
    {
        wifiEventHandlers_[eWiFiEventScanDone](xEvent);
    }
}

static void WIFI_ConnectedCallback( WIFIEvent_t * xEvent )
{

    WIFI_PostEvent(WIFI_EVENT_CONNECTED);
    if (wifiEventHandlers_[eWiFiEventConnected])
    {
        wifiEventHandlers_[eWiFiEventConnected](xEvent);
    }
}

static void WIFI_DisconnectedCallback( WIFIEvent_t * xEvent )
{

    WIFI_PostEvent(WIFI_EVENT_DISCONNECTED);
    if (wifiEventHandlers_[eWiFiEventDisconnected])
    {
        wifiEventHandlers_[eWiFiEventDisconnected](xEvent);
    }
}

static void WIFI_APStateChangedCallback( WIFIEvent_t * xEvent )
{

    WIFI_PostEvent(WIFI_EVENT_AP_STATE_CHANGED);
    if (wifiEventHandlers_[eWiFiEventAPStateChanged])
    {
        wifiEventHandlers_[eWiFiEventAPStateChanged](xEvent);
    }
}

static void WIFI_IPReadyCallback( WIFIEvent_t * xEvent )
{

    WIFI_PostEvent(WIFI_EVENT_IP_READY);
    if (wifiEventHandlers_[eWiFiEventIPReady])
    {
        wifiEventHandlers_[eWiFiEventIPReady](xEvent);
    }
}

static void WIFI_ConnectionFailedCallback( WIFIEvent_t * xEvent )
{
    if (xEvent->xInfo.xConnectionFailed.xReason == eWiFiReasonAPNotFound) {
        if(++gSsidNotFound >= MAX_SSID_SEARCH_COUNT) {
            WIFI_PostEvent(WIFI_EVENT_CONNECTION_FAILED);
        }
    }
    else {  // All other reasons treat it as failure
        WIFI_PostEvent(WIFI_EVENT_CONNECTION_FAILED);
    }

    if (wifiEventHandlers_[eWiFiEventConnectionFailed])
    {
        wifiEventHandlers_[eWiFiEventConnectionFailed](xEvent);
    }
}


WIFIReturnCode_t WIFI_RegisterEvent( WIFIEventType_t xEventType,
                                     WIFIEventHandler_t xHandler )
{
    WIFIEvent_t event;
    WIFIReturnCode_t retCode;

    if (xEventType >= eWiFiEventMax)
    {
        AWS_WIFI_LOGE("WIFI RegisterEvent %u invalid\n", xEventType);
        return eWiFiFailure;
    }
    wifiEventHandlers_[xEventType] = xHandler;

    switch (xEventType)
    {
        // These events are used by the sync API so will be registered automatically
        case eWiFiEventReady:
            if (wifiInitDone_)
            {
                // WIFI init has already happened
                event.xEventType = eWiFiEventReady;
                xHandler(&event);
            }
            break;
        case eWiFiEventScanDone:
        case eWiFiEventConnected:
        case eWiFiEventDisconnected:
        case eWiFiEventAPStateChanged:
        case eWiFiEventIPReady:
            break;
        default:
            // All other events are passed to WIFI_HAL layer
            retCode = WIFI_HAL_RegisterEvent(xEventType, xHandler);
            if (retCode != eWiFiSuccess)
            {
                AWS_WIFI_LOGE("WIFI RegisterEvent %u failed\n", xEventType);
                return eWiFiFailure;
            }
    }
    return eWiFiSuccess;
}

//
// AWS WIFI API functions (published, synchronous)
//
WIFIReturnCode_t WIFI_On( void )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (!wifiInitDone_)
    {
        return WIFI_Init();
    }

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_SetRadio(1);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI On failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI On timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_Off( void )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_SetRadio(0);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI Off failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI Off timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_ConnectAP( const WIFINetworkParams_t * const pxNetworkParams )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFINetworkParamsExt_t paramsExt;
    WIFIReturnCode_t retCode;
    BaseType_t retVal;

    memset(&paramsExt, 0, sizeof(WIFINetworkParamsExt_t));
    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_ConvertNetworkParams(pxNetworkParams, &paramsExt);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI ConnectAP conversion failed\n");
            return WIFI_GiveSemaphore(eWiFiFailure);
        }

        // If already connected to the same AP, do nothing
        if (WIFI_HAL_IsConnected(&paramsExt))
        {
            AWS_WIFI_LOGI("WIFI ConnectAP already connected\n");
            return WIFI_GiveSemaphore(eWiFiSuccess);
        }

        // Clear event before the async procedure to avoid race condition!
        WIFI_ClearEvent(WIFI_EVENT_CONNECTED | WIFI_EVENT_CONNECTION_FAILED | WIFI_EVENT_IP_READY);
        gSsidNotFound = 0;

        retCode = WIFI_HAL_StartConnectAP(&paramsExt);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI ConnectAP failed\n");
        }
        else
        {
            EventBits_t event;
            event = WIFI_WaitGroupEvent(WIFI_EVENT_CONNECTED | WIFI_EVENT_CONNECTION_FAILED);
            if(event & WIFI_EVENT_CONNECTED) {
                if(WIFI_WaitEvent(WIFI_EVENT_IP_READY)) {
                    AWS_WIFI_LOGD("WIFI ConnectAP succeess\n");
                }
                else {
                    AWS_WIFI_LOGI("WIFI ConnectAP cannot obtain IP\n");
                    retCode = eWiFiTimeout;
                }
            }
            else if (event & WIFI_EVENT_CONNECTION_FAILED) {
                retCode = eWiFiFailure;
            }
            else {
                AWS_WIFI_LOGI("WIFI ConnectAP event timeout\n");
                retCode = eWiFiTimeout;
            }
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI ConnectAP timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_Disconnect( void )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        if(WIFI_HAL_IsConnected(NULL)) {
            // Clear event before the async procedure to avoid race condition!
            WIFI_ClearEvent(WIFI_EVENT_DISCONNECTED);
            retCode = WIFI_HAL_StartDisconnectAP();
            if (retCode != eWiFiSuccess)
            {
                AWS_WIFI_LOGE("WIFI Disconnect failed\n");
            }
            else
            {
                if (WIFI_WaitEvent(WIFI_EVENT_DISCONNECTED))
                {
                    AWS_WIFI_LOGD("WIFI Disconnect succeess\n");
                }
                else
                {
                    AWS_WIFI_LOGI("WIFI Disconnect event timeout\n");
                    retCode = eWiFiTimeout;
                }
            }
        }
        else {  // Already disconnected
            retCode = eWiFiSuccess;
        }
        return WIFI_GiveSemaphore(retCode);
    }

    AWS_WIFI_LOGI("WIFI Disconnect timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_Reset( void )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    return eWiFiNotSupported;
}

WIFIReturnCode_t WIFI_SetMode( WIFIDeviceMode_t xDeviceMode )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIDeviceModeExt_t modeExt;
    WIFIReturnCode_t retCode;

    // Convert to the extended mode
    switch (xDeviceMode)
    {
        case eWiFiModeStation:
            modeExt = eWiFiModeStationExt;
            break;
        case eWiFiModeAP:
            modeExt = eWiFiModeAP;
            break;
        default:
            AWS_WIFI_LOGI(" WIFI SetMode %u not supported\n", xDeviceMode);
            return eWiFiNotSupported;
    }

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_SetMode(modeExt);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI SetMode failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI SetMode timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetMode( WIFIDeviceMode_t* pxDeviceMode )
{
    WIFIDeviceModeExt_t modeExt;
    WIFIReturnCode_t retCode;

    if (pxDeviceMode == NULL) {
        AWS_WIFI_LOGE("WIFI GetMode failed because of NULL pointer\n");
        return eWiFiFailure;
    }

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_GetMode(&modeExt);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI GetMode failed\n");
        }
        else
        {
            // Convert from the extended mode
            switch (modeExt)
            {
                case eWiFiModeStationExt:
                    *pxDeviceMode = eWiFiModeStation;
                    break;
                case eWiFiModeAPExt:
                    *pxDeviceMode = eWiFiModeAP;
                    break;
                default:
                    AWS_WIFI_LOGI(" WIFI GetMode %u not supported\n", modeExt);
                    *pxDeviceMode = eWiFiModeNotSupported;
            }
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI GetMode timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_NetworkAdd( const WIFINetworkProfile_t * const pxNetworkProfile,
                                  uint16_t * pusIndex )
{
    // Profile management is better done in the upper layer
    return eWiFiNotSupported;
}

WIFIReturnCode_t WIFI_NetworkGet( WIFINetworkProfile_t * pxNetworkProfile,
                                  uint16_t usIndex )
{
    // Profile management is better done in the upper layer
    return eWiFiNotSupported;
}

WIFIReturnCode_t WIFI_NetworkDelete( uint16_t usIndex )
{
    // Profile management is better done in the upper layer
    return eWiFiNotSupported;
}

#ifndef AMAZON_WIFI_HAL_STUB
WIFIReturnCode_t WIFI_Ping( uint8_t * pucIPAddr, uint16_t usCount, uint32_t ulIntervalMS )
{
    WIFIReturnCode_t retCode;
    WIFIPingResults_t results;

    retCode = WIFI_PingExt(pucIPAddr, usCount, WIFI_PING_DATA_SIZE, ulIntervalMS, &results);
    if (retCode != eWiFiSuccess)
    {
        AWS_WIFI_LOGE("WIFI_Ping failed\n");
        return eWiFiFailure;
    }
    // If one ping reply is received, it means the link is up so we treat it as success
    return results.usSuccessCount > 0 ? eWiFiSuccess : eWiFiFailure;
}
#endif

WIFIReturnCode_t WIFI_GetIP( uint8_t * pucIPAddr )
{
    WIFIReturnCode_t retCode;

    if(pucIPAddr == NULL) {
        AWS_WIFI_LOGE("WIFI GetIP failed because of NULL pointer\n");
        return eWiFiFailure;
    }

    if (WIFI_TakeSemaphore())
    {
        WIFIIPConfiguration_t ipInfo;
        retCode = WIFI_HAL_GetIPInfo(&ipInfo);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI GetIP failed\n");
        } else {
            // This API doesn't show the length of addr
            // Do IPV4 here.
            memcpy(pucIPAddr, &(ipInfo.xIPAddress.ulAddress), 4);
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI GetIP timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetMAC( uint8_t * pucMac )
{
    WIFIReturnCode_t retCode;

    if (pucMac == NULL) {
        AWS_WIFI_LOGE("WIFI GetMAC failed because of NULL pointer\n");
        return eWiFiFailure;
    }

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_GetMacAddress(pucMac);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI GetMAC failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI GetMAC timeout\n");
    return eWiFiTimeout;
}

#ifndef AMAZON_WIFI_HAL_STUB
// This function is thread-safe and runs in the caller thread, so we don't need
// semaphore protection. Also, only IPv4 is supported in this version
WIFIReturnCode_t WIFI_GetHostIP( char * pcHost, uint8_t * pucIPAddr )
{
    WIFIReturnCode_t retCode;
    err_t retVal;
    struct addrinfo hints;
    struct addrinfo* result;

   if (pcHost == NULL || pucIPAddr == NULL)
    {
        AWS_WIFI_LOGE("WIFI GetHostIP params invalid\n");
        return eWiFiFailure;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    retVal = getaddrinfo(pcHost, NULL, &hints, &result);
    if (retVal != 0 || result == NULL)
    {
        AWS_WIFI_LOGE("WIFI GetHostIP getaddrinfo failed: %d\n", retVal);
        retCode = eWiFiFailure;
    }
    else
    {
        for (struct addrinfo* p = result; p != NULL; p = p->ai_next)
        {
            if (p->ai_family == AF_INET)
            {
                struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
                memcpy(pucIPAddr, &ipv4->sin_addr.s_addr, sizeof(ipv4->sin_addr.s_addr));

#ifdef DEBUG
                AWS_WIFI_LOGD("WIFI GetHostIP got IPv4 address %d.%d.%d.%d\n",
                      pucIPAddr[0], pucIPAddr[1], pucIPAddr[2], pucIPAddr[3]);
#else
                AWS_WIFI_LOGD("WIFI GetHostIP got IPv4 address\n");
#endif
                // FIXME: pick the first address or the last???
            }
            else
            {
                // NOTE: IPv6 is not supported in this version
                AWS_WIFI_LOGI("WIFI GetHostIP got IPv6 address\n");
            }
        }
        freeaddrinfo(result);
        retCode = eWiFiSuccess;
    }
    return retCode;
}
#else
WIFIReturnCode_t WIFI_GetHostIP( char * pcHost, uint8_t * pucIPAddr )
{
    return eWiFiNotSupported;
}
#endif

// This function does not allow indication of the actual number of scan results,
// so it is up to the caller to mark the end of pxBuffer when the actual number
// of scan results is smaller than ucNumNetworks
WIFIReturnCode_t WIFI_Scan( WIFIScanResult_t * pxBuffer,
                            uint8_t ucNumNetworks )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIScanResultExt_t* result;
    WIFIReturnCode_t retCode;
    uint8_t i;

    if(pxBuffer == NULL) {
        AWS_WIFI_LOGE("%s Scan failed due to invalid parameters\n", __FUNCTION__);
        return eWiFiFailure;
    }

    if (WIFI_TakeSemaphore())
    {
        // Invalidate cached scan results
        wifiNumScanResults_ = 0;

        // Clear event before the async procedure to avoid race condition!
        WIFI_ClearEvent(WIFI_EVENT_SCAN_DONE);
        retCode = WIFI_HAL_StartScan(NULL);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI Scan failed to start\n");
        }
        else
        {
            if (WIFI_WaitEvent(WIFI_EVENT_SCAN_DONE))
            {
                if (ucNumNetworks > wifiNumScanResults_)
                {
                    ucNumNetworks = wifiNumScanResults_;
                }
                for (i = 0; i < ucNumNetworks; i++)
                {
                    result = &wifiScanResults_[i];
                    memcpy(pxBuffer->cSSID, result->ucSSID, result->ucSSIDLength);
                    pxBuffer->cSSID[result->ucSSIDLength] = 0;  // Null termination
                    memcpy(pxBuffer->ucBSSID, result->ucBSSID, wificonfigMAX_BSSID_LEN);
                    pxBuffer->xSecurity = result->xSecurity;
                    pxBuffer->cRSSI = result->cRSSI;
                    pxBuffer->cChannel = result->ucChannel;
                    pxBuffer++;
                }
                WIFI_ShowScanResults();
            }
            else
            {
                AWS_WIFI_LOGI("WIFI Scan event timeout\n");
                retCode = WIFI_HAL_StopScan();
                if (retCode != eWiFiSuccess)
                {
                    AWS_WIFI_LOGE("WIFI Scan failed to stop\n");
                }
                wifiNumScanResults_ = 0;
            }
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI Scan timeout\n");
    retCode = WIFI_HAL_StopScan();
    if (retCode != eWiFiSuccess)
    {
        AWS_WIFI_LOGE("WIFI Scan failed to stop\n");
    }
    wifiNumScanResults_ = 0;
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_StartAP( void )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        // Clear event before the async procedure to avoid race condition!
        WIFI_ClearEvent(WIFI_EVENT_AP_STATE_CHANGED);

        // We don't maintain AP state machine here. Upper layer has that responsibility
        retCode = WIFI_HAL_StartAP(&wifiAPConfig_);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI StartAP failed\n");
        }
        else
        {
            if (WIFI_WaitEvent(WIFI_EVENT_AP_STATE_CHANGED))
            {
                AWS_WIFI_LOGD("WIFI StartAP succeess\n");
            }
            else
            {
                AWS_WIFI_LOGI("WIFI StartAP event timeout\n");
                retCode = eWiFiTimeout;
            }
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI StartAP timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_StopAP( void )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        // Clear event before the async procedure to avoid race condition!
        WIFI_ClearEvent(WIFI_EVENT_AP_STATE_CHANGED);

        // We don't maintain AP state machine here. Upper layer has that responsibility
        retCode = WIFI_HAL_StopAP();
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI StopAP failed\n");
        }
        else
        {
            if (WIFI_WaitEvent(WIFI_EVENT_AP_STATE_CHANGED))
            {
                AWS_WIFI_LOGD("WIFI StopAP succeess\n");
            }
            else
            {
                AWS_WIFI_LOGI("WIFI StopAP event timeout\n");
                retCode = eWiFiTimeout;
            }
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI StopAP timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_ConfigureAP( const WIFINetworkParams_t * const pxNetworkParams )
{
    WIFINetworkParamsExt_t paramsExt;
    WIFIReturnCode_t retCode;

    retCode = WIFI_ConvertNetworkParams(pxNetworkParams, &paramsExt);
    if (retCode != eWiFiSuccess)
    {
        AWS_WIFI_LOGE("WIFI COnfigureAP conversion failed\n");
        return eWiFiFailure;
    }
    return WIFI_ConfigureAPExt(&paramsExt);
}

WIFIReturnCode_t WIFI_SetPMMode( WIFIPMMode_t xPMModeType,
                                 const void * pvOptionValue )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (pvOptionValue == NULL) {
        AWS_WIFI_LOGE("Invalid parameter\n");
        return eWiFiFailure;
    }

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_SetPMMode(xPMModeType);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI SetPMMode failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI SetPMMode timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetPMMode( WIFIPMMode_t * pxPMModeType,
                                 void * pvOptionValue )
{
    WIFIReturnCode_t retCode;

    if (pvOptionValue == NULL || pxPMModeType == NULL) {
        AWS_WIFI_LOGE("Invalid parameters\n");
        return eWiFiFailure;
    }

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_GetPMMode(pxPMModeType);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI GetPMMode failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI GetPMMode timeout\n");
    return eWiFiTimeout;
}

BaseType_t WIFI_IsConnected( void )
{
    return WIFI_HAL_IsConnected(NULL);
}

BaseType_t WIFI_IsConnectedExt( const WIFINetworkParamsExt_t * pxNetworkParams )
{
    BaseType_t retVal;

    if (WIFI_TakeSemaphore())
    {
        retVal = WIFI_HAL_IsConnected(pxNetworkParams);
        WIFI_GiveSemaphore(eWiFiSuccess);
        return retVal;
    }
    AWS_WIFI_LOGI("WIFI IsConnected timeout\n");
    return pdFALSE;
}

//
// AWS WIFI extended API functions
//
WIFIReturnCode_t WIFI_Init( void )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;
    TickType_t tickCount;

    // In case of repeated initialization calls
    if (wifiInitDone_) return eWiFiSuccess;

    // Create semaphore for higher layer
    WIFI_InitSemaphore();

    // Create event for lower layer
    WIFI_InitEvent();

    if (WIFI_TakeSemaphore())
    {
        // Clear event before the async procedure to avoid race condition!
        WIFI_ClearEvent(WIFI_EVENT_INIT_DONE);

        // Register events necessary for sync operations
        // Must be done before initialization

#define CHECK_REGISTER(type, handler) retCode = WIFI_HAL_RegisterEvent(type, handler); \
        if (retCode != eWiFiSuccess){ AWS_WIFI_LOGE("WIFI register %d failed: %d", type, retCode); }

        CHECK_REGISTER(eWiFiEventReady, WIFI_InitDoneCallback);
        CHECK_REGISTER(eWiFiEventScanDone, WIFI_ScanDoneCallback);
        CHECK_REGISTER(eWiFiEventConnected, WIFI_ConnectedCallback);
        CHECK_REGISTER(eWiFiEventDisconnected, WIFI_DisconnectedCallback);
        CHECK_REGISTER(eWiFiEventAPStateChanged, WIFI_APStateChangedCallback);
        CHECK_REGISTER(eWiFiEventIPReady, WIFI_IPReadyCallback);
        CHECK_REGISTER(eWiFiEventConnectionFailed, WIFI_ConnectionFailedCallback);
#undef CHECK_REGISTER

        // Initialize WiFi and IP layers
        retCode = WIFI_HAL_Init();
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI Init failed");
            return WIFI_GiveSemaphore(retCode);
        }

#if 0   //Due to an issue, do not do this for now
        // Use tickCount to detect whether WIFI_Init() is called in main() or not
        // If so, tickCount will not advance and WaitEvent() will timeout immediately
        tickCount = xTaskGetTickCount();
        if (!WIFI_WaitEvent(WIFI_EVENT_INIT_DONE))
        {
            if (tickCount < xTaskGetTickCount())
            {
                AWS_WIFI_LOGD("WIFI Init event timeout\n");
                retCode = eWiFiTimeout;
            }
        }
#endif
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI Init timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_SetModeExt( WIFIDeviceModeExt_t xDeviceMode )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_SetMode(xDeviceMode);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI SetModeExt failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI SetModeExt timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetModeExt( WIFIDeviceModeExt_t * pxDeviceMode )
{
    WIFIReturnCode_t retCode;

    if (pxDeviceMode == NULL) {
        AWS_WIFI_LOGE("WIFI GetModeExt failed because of NULL pointer\n");
        return eWiFiFailure;
    }

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_GetMode(pxDeviceMode);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI GetModeExt failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI GetModeExt timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_StartScan( WIFIScanConfig_t * pxScanConfig )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        // Invalidate cached scan results
        wifiNumScanResults_ = 0;

        retCode = WIFI_HAL_StartScan(pxScanConfig);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI StartScan failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI StartScan timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetScanResults( const WIFIScanResultExt_t ** pxBuffer,
                                      uint16_t * ucNumNetworks )
{
    AWS_WIFI_LOGD("%s, numScanResults %u\n", __FUNCTION__, wifiNumScanResults_);

    if (WIFI_TakeSemaphore())
    {
        *pxBuffer = wifiScanResults_;
        *ucNumNetworks = wifiNumScanResults_;

        WIFI_ShowScanResults();
        return WIFI_GiveSemaphore(eWiFiSuccess);
    }
    AWS_WIFI_LOGI("WIFI GetScanResults timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_StartConnectAP( const WIFINetworkParamsExt_t * pxNetworkParams )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    retCode = WIFI_ValidateNetworkParams(pxNetworkParams);
    if (retCode != eWiFiSuccess)
    {
        AWS_WIFI_LOGE("WIFI StartConnectAP validation failed\n");
        return eWiFiFailure;
    }

    if (WIFI_TakeSemaphore())
    {
        // If already connected to the same AP, do nothing
        if (WIFI_HAL_IsConnected(pxNetworkParams))
        {
            AWS_WIFI_LOGI("WIFI StartConnectAP already connected\n");
            retCode = eWiFiSuccess;
        }
        else
        {
            retCode = WIFI_HAL_StartConnectAP(pxNetworkParams);
            if (retCode != eWiFiSuccess)
            {
                AWS_WIFI_LOGE("WIFI StartConnectAP failed\n");
            }
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI StartConnectAP timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_StartDisconnect( void )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_StartDisconnectAP();
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI StartDisconnect failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI StartDisconnect timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetConnectionInfo( WIFIConnectionInfo_t * pxConnectionInfo )
{
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_GetConnectionInfo(pxConnectionInfo);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI GetConnectionInfo failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI GetConnectionInfo timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetIPInfo( WIFIIPConfiguration_t * pxIPInfo )
{
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_GetIPInfo(pxIPInfo);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI Get IP info failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI Get IP info timeout\n");
    return eWiFiTimeout;
}

#ifndef AMAZON_WIFI_HAL_STUB
// This function is thread-safe and runs in the caller thread, so we don't need
// semaphore protection. Also, only IPv4 is supported in this version
WIFIReturnCode_t WIFI_PingExt( uint8_t * pucIPAddr,
                               uint16_t usCount,
                               uint16_t usLength,
                               uint32_t ulIntervalMS,
                               WIFIPingResults_t * pxResults )
{
    int sock;
    struct sockaddr_in addr;
    int addrSize;
    char* buffer;
    struct ip_hdr* ip;
    struct icmp_echo_hdr* icmp;
    uint16_t sendSeqNum = 0;
    uint16_t recvSeqNum;
    uint32_t sendTime;
    uint32_t pingTime;
    uint32_t pingTimeTotal = 0;
    int retVal;
    int i;

    if (pucIPAddr == NULL || usCount == 0 || usLength == 0 || usLength > WIFI_PING_MAX_DATA_SIZE ||
        ulIntervalMS == 0 || pxResults == NULL)
    {
        AWS_WIFI_LOGE("WIFI Ping params invalid\n");
        return eWiFiFailure;
    }
    memset(pxResults, 0, sizeof(WIFIPingResults_t));
    pxResults->ulMinTimeMS = INT_MAX;

    sock = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
    if (sock < 0)
    {
        AWS_WIFI_LOGE("WIFI Ping socket failed\n");
        return eWiFiFailure;
    }

    buffer = (char *)mem_malloc(WIFI_PING_IP_ICMP_HDR_SIZE + usLength);
    if (buffer == NULL)
    {
        AWS_WIFI_LOGE("WIFI Ping mem_malloc failed\n");
        close(sock);
        return eWiFiFailure;
    }
    ip = (struct ip_hdr *)buffer;

    while (usCount--)
    {
        // Prepare ping request
        icmp = (struct icmp_echo_hdr *)(buffer + sizeof(struct ip_hdr));
        ICMPH_TYPE_SET(icmp, ICMP_ECHO);
        ICMPH_CODE_SET(icmp, 0);
        icmp->chksum = 0;
        icmp->id = wifiPingId_;
        icmp->seqno  = htons(++sendSeqNum);
        for (i = 0; i < usLength; i++)
        {
            buffer[WIFI_PING_IP_ICMP_HDR_SIZE + i] = (char)i;
        }
        icmp->chksum = inet_chksum(icmp, WIFI_PING_ICMP_HDR_SIZE + usLength);

        // Send ping request (IP header is not included)
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = pucIPAddr[0] + (pucIPAddr[1] << 8) +
                               (pucIPAddr[2] << 16) + (pucIPAddr[3] << 24);
        addrSize = sizeof(addr);
        sendTime = sys_now();
        retVal = sendto(sock, icmp, WIFI_PING_ICMP_HDR_SIZE + usLength, 0,
                             (struct sockaddr*)&addr, addrSize);
        if (retVal != (int)(WIFI_PING_ICMP_HDR_SIZE + usLength))
        {
            AWS_WIFI_LOGE("WIFI Ping sendto failed %d\n", retVal);
            break;
        }

        pingTime = 0;
        do
        {
            // Set the receive timeout
            wifiPingTimeout_.tv_sec = (ulIntervalMS - pingTime) / 1000;
            wifiPingTimeout_.tv_usec = ((ulIntervalMS - pingTime) % 1000) * 1000;
            retVal = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &wifiPingTimeout_, sizeof(struct timeval));
            if (retVal != 0)
            {
                AWS_WIFI_LOGE("WIFI Ping setsockopt failed %d\n", retVal);
                mem_free(buffer);
                close(sock);
                return eWiFiSuccess;
            }

            // Receive ping response (IP header is included)
            retVal = recvfrom(sock, buffer, WIFI_PING_IP_ICMP_HDR_SIZE + usLength, 0,
                               (struct sockaddr*)&addr, (socklen_t*)&addrSize);
            pingTime = sys_now() - sendTime;
            if (retVal >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr)))
            {
                icmp = (struct icmp_echo_hdr *)(buffer + (IPH_HL(ip) * 4));
                recvSeqNum = ntohs(icmp->seqno);
                if (ICMPH_TYPE(icmp) == ICMP_ER && icmp->id == wifiPingId_ && recvSeqNum == sendSeqNum)
                {
                    AWS_WIFI_LOGI("WIFI Ping success, seq=%u, time=%lu ms\n", recvSeqNum, pingTime);
                    pingTimeTotal += pingTime;
                    pxResults->usSuccessCount++;

                    if (pxResults->ulMinTimeMS > pingTime)
                    {
                         pxResults->ulMinTimeMS = pingTime;
                    }
                    if (pxResults->ulMaxTimeMS < pingTime)
                    {
                        pxResults->ulMaxTimeMS = pingTime;
                    }
                    break;
                }
            }
            else if (retVal < 0 && errno == EWOULDBLOCK)
            {
                // Receive timeout from lwip_recvfrom (retVal == -1 && errno == EWOULDBLOCK(11))
                AWS_WIFI_LOGI("WIFI Ping timeout, seq=%u, time=%lu\n", sendSeqNum, pingTime);
                pxResults->usTimeoutCount++;
                break;
            }

            // Check for timeout to avoid infinite loops
            if (pingTime >= ulIntervalMS)
            {
                AWS_WIFI_LOGI("WIFI Ping timeout, seq=%u, time=%lu\n", sendSeqNum, pingTime);
                pxResults->usTimeoutCount++;
                break;
            }
        } while (1);

        // Wait for the next ping request
        if (pingTime < ulIntervalMS )
        {
            sys_msleep(ulIntervalMS - pingTime);
        }
    }
    if (pxResults->usSuccessCount != 0)
    {
        pxResults->ulAvgTimeMS = pingTimeTotal / pxResults->usSuccessCount;
    }

    mem_free(buffer);
    close(sock);
    return eWiFiSuccess;
}
#else
WIFIReturnCode_t WIFI_PingExt( uint8_t * pucIPAddr,
                               uint16_t usCount,
                               uint16_t usLength,
                               uint32_t ulIntervalMS,
                               WIFIPingResults_t * pxResults )
{
    return eWiFiNotSupported;
}
#endif  // AMAZON_WIFI_HAL_STUB

WIFIReturnCode_t WIFI_GetRSSI( int8_t * pcRSSI )
{
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        if (WIFI_HAL_IsConnected(NULL))
        {
            retCode = WIFI_HAL_GetRSSI(pcRSSI);
            if (retCode != eWiFiSuccess)
            {
                AWS_WIFI_LOGE("WIFI GetRSSI failed\n");
            }
        }
        else
        {
            AWS_WIFI_LOGE("WIFI GetRSSI not connected\n");
            retCode = eWiFiFailure;
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI GetRSSI timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_ConfigureAPExt( WIFINetworkParamsExt_t * pxNetworkParams )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_ValidateNetworkParams(pxNetworkParams);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI ConfigureAPExt validation failed\n");
        }
        else
        {
            // Set default channel if not specified
            if (pxNetworkParams->ucChannel == 0)
            {
                pxNetworkParams->ucChannel = wificonfigACCESS_POINT_CHANNEL;
            }
            memcpy(&wifiAPConfig_, pxNetworkParams, sizeof(wifiAPConfig_));
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI ConfigureAPExt timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetStationList( WIFIStationInfo_t * pxStationList,
                                      uint8_t * puStationListSize )
{
    WIFIReturnCode_t retCode;

    if(pxStationList == NULL || puStationListSize == NULL || *puStationListSize == 0) {
        return eWiFiFailure;
    }

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_GetStationList(pxStationList, puStationListSize);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI GetStationList failed\n");
            return eWiFiFailure;
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI GetStationList timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_StartDisconnectStation( uint8_t * pucMac )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_DisconnectStation(pucMac);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI StartDisconnectStation failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI StartDisconnectStation timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_SetMAC( uint8_t * pucMac )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_SetMacAddress(pucMac);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI SetMAC failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI SetMAC timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_SetCountryCode( const char * pcCountryCode )
{
    AWS_WIFI_LOGD("%s\n", __FUNCTION__);
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_SetCountryCode(pcCountryCode);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI SetCountryCode failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI SetCountryCode timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetCountryCode( char * pcCountryCode )
{
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_GetCountryCode(pcCountryCode);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI GetCountryCode failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI GetCountryCode timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetStatistic( WIFIStatisticInfo_t * pxStatistics )
{
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_GetStatistic(pxStatistics);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI GetStatistic failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGI("WIFI GetStatistic timeout\n");
    return eWiFiTimeout;
}

WIFIReturnCode_t WIFI_GetCapability( WIFICapabilityInfo_t * pxCapabilities)
{
    WIFIReturnCode_t retCode;

    if (WIFI_TakeSemaphore())
    {
        retCode = WIFI_HAL_GetCapability(pxCapabilities);
        if (retCode != eWiFiSuccess)
        {
            AWS_WIFI_LOGE("WIFI GetCapability failed\n");
        }
        return WIFI_GiveSemaphore(retCode);
    }
    AWS_WIFI_LOGE("WIFI GetCapability timeout\n");
    return eWiFiTimeout;
}

/* NXP: TODO, check if really needed */
WIFIReturnCode_t WIFI_RegisterNetworkStateChangeEventCallback( IotNetworkStateChangeEventCallback_t xCallback )
{
    return eWiFiNotSupported;
}
