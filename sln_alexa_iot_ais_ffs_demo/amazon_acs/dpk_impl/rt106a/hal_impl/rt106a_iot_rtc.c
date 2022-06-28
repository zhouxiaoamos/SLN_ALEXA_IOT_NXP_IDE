/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdint.h>
#include <stddef.h>
#include "iot_rtc.h"
#include "fsl_snvs_hp.h"
#include "rt106a_iot_log.h"
#include "FreeRTOS.h"

#define iotRTC_BASE_YEAR            ( 1900U )

typedef struct Rtc_Ctxt_s
{
    SNVS_Type *pxRtcHandleBase;    /* RTC base register */
    IotRtcCallback_t  xCallback;  /* callback */
    void* pvUserContext;          /* user context to provide in callback */
    uint8_t ucInitialized;        /* Initialized flag */
    uint8_t ucStatus;             /* Internal timer status */
} Rtc_Ctxt_t;

static Rtc_Ctxt_t xRtcCtxt =
{
    .pxRtcHandleBase = SNVS,
    .xCallback = NULL,
    .pvUserContext = NULL,
    .ucInitialized = 0,
    .ucStatus = eRtcTimerStopped
};

void SNVS_HP_WRAPPER_IRQHandler(void)
{
    if (SNVS_HP_RTC_GetStatusFlags(SNVS) & kSNVS_RTC_AlarmInterruptFlag)
    {
        SNVS_HP_RTC_ClearStatusFlags(SNVS, kSNVS_RTC_AlarmInterruptFlag);

        if (xRtcCtxt.xCallback)
        {
            xRtcCtxt.ucStatus =  eRtcTimerAlarmTriggered;
            xRtcCtxt.xCallback(xRtcCtxt.ucStatus, xRtcCtxt.pvUserContext);
        }
    }
}

IotRtcHandle_t iot_rtc_open(int32_t lInstance)
{
    IotRtcHandle_t   xHandle = NULL;
    snvs_hp_rtc_config_t snvsRtcConfig;

    /*
     * Check if its already open.
     * Note that this does not take care of race conditions.
     */
    if(lInstance != 0 || xRtcCtxt.ucInitialized == 1)
    {
        return NULL;
    }
    xRtcCtxt.ucInitialized = 1;
    xHandle = (void*)&xRtcCtxt;

    /*
     * Initialzie the RTC hardware.
     * reset RTC, clock and turn on the oscillator
     */
    SNVS_HP_RTC_GetDefaultConfig(&snvsRtcConfig);
    SNVS_HP_RTC_Init(SNVS, &snvsRtcConfig);
    NVIC_SetPriority(SNVS_HP_WRAPPER_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY-1);
    EnableIRQ(SNVS_HP_WRAPPER_IRQn);

    return xHandle;
}

int32_t iot_rtc_set_datetime(IotRtcHandle_t const pxRtcHandle,
                            const IotRtcDatetime_t * pxDatetime)
{
    snvs_hp_rtc_datetime_t xDatetime;
    Rtc_Ctxt_t *ptxt = (Rtc_Ctxt_t *)pxRtcHandle;

    if (pxRtcHandle == NULL || pxDatetime == NULL)
    {
        return IOT_RTC_INVALID_VALUE;
    }

    xDatetime.year = pxDatetime->usYear + iotRTC_BASE_YEAR;
    xDatetime.month =  pxDatetime->ucMonth + 1;
    xDatetime.day =  pxDatetime->ucDay;
    xDatetime.hour = pxDatetime->ucHour;
    xDatetime.minute = pxDatetime->ucMinute;
    xDatetime.second = pxDatetime->ucSecond;

    if (kStatus_Success != SNVS_HP_RTC_SetDatetime(ptxt->pxRtcHandleBase, &xDatetime))
    {
        return IOT_RTC_SET_FAILED;
    }

    SNVS_HP_RTC_StartTimer(ptxt->pxRtcHandleBase);

    ptxt->ucStatus = eRtcTimerRunning;

    return IOT_RTC_SUCCESS;
}

int32_t iot_rtc_get_datetime(IotRtcHandle_t const pxRtcHandle,
                                    IotRtcDatetime_t * pxDatetime)
{
    snvs_hp_rtc_datetime_t xDatetime;
    Rtc_Ctxt_t *ptxt = (Rtc_Ctxt_t *)pxRtcHandle;

    if (pxRtcHandle == NULL || pxDatetime == NULL)
    {
        return IOT_RTC_INVALID_VALUE;
    }

    if(ptxt->ucStatus == eRtcTimerStopped)
    {
        return IOT_RTC_NOT_STARTED;
    }

    SNVS_HP_RTC_GetDatetime(ptxt->pxRtcHandleBase, &xDatetime);

    pxDatetime->usYear = xDatetime.year - iotRTC_BASE_YEAR;
    pxDatetime->ucMonth = xDatetime.month - 1;
    pxDatetime->ucDay = xDatetime.day;
    pxDatetime->ucHour = xDatetime.hour;
    pxDatetime->ucMinute = xDatetime.minute;
    pxDatetime->ucSecond = xDatetime.second;

    return IOT_RTC_SUCCESS;
}

void iot_rtc_set_callback(IotRtcHandle_t const pxRtcHandle,
                                IotRtcCallback_t xCallback,
                                void * pvUserContext)
{
    if ((pxRtcHandle != NULL) && (xCallback != NULL))
    {
        ((Rtc_Ctxt_t *)pxRtcHandle)->xCallback     = xCallback;
        ((Rtc_Ctxt_t *)pxRtcHandle)->pvUserContext = pvUserContext;
    }
}

int32_t iot_rtc_ioctl(IotRtcHandle_t const pxRtcHandle,
                        IotRtcIoctlRequest_t xRequest,
                        void * const pvBuffer)
{
    snvs_hp_rtc_datetime_t xDatetime;
    IotRtcDatetime_t *pIotDatetime = NULL;
    Rtc_Ctxt_t *ptxt = (Rtc_Ctxt_t *)pxRtcHandle;

    if (pxRtcHandle == NULL)
    {
        return IOT_RTC_INVALID_VALUE;
    }

    switch(xRequest) {
    case eSetRtcAlarm:
        if (pvBuffer == NULL)
        {
            return IOT_RTC_INVALID_VALUE;
        }
        if(ptxt->ucStatus == eRtcTimerStopped)
        {
            return IOT_RTC_NOT_STARTED;
        }
        pIotDatetime = (IotRtcDatetime_t *)(pvBuffer);
        xDatetime.year =   pIotDatetime->usYear + iotRTC_BASE_YEAR;
        xDatetime.month =  pIotDatetime->ucMonth + 1;
        xDatetime.day =    pIotDatetime->ucDay;
        xDatetime.hour =   pIotDatetime->ucHour;
        xDatetime.minute = pIotDatetime->ucMinute;
        xDatetime.second = pIotDatetime->ucSecond;

        if (0 != SNVS_HP_RTC_SetAlarm(ptxt->pxRtcHandleBase, &xDatetime))
        {
            return IOT_RTC_SET_FAILED;
        }

        SNVS_HP_RTC_EnableInterrupts(SNVS, kSNVS_RTC_AlarmInterrupt);
        break;

    case eGetRtcAlarm:
        if (pvBuffer == NULL)
        {
            return IOT_RTC_INVALID_VALUE;
        }

        SNVS_HP_RTC_GetAlarm(ptxt->pxRtcHandleBase, &xDatetime);
        pIotDatetime = (IotRtcDatetime_t *)(pvBuffer);
        pIotDatetime->usYear = xDatetime.year - iotRTC_BASE_YEAR;
        pIotDatetime->ucMonth = xDatetime.month - 1;
        pIotDatetime->ucDay = xDatetime.day;
        pIotDatetime->ucHour = xDatetime.hour;
        pIotDatetime->ucMinute = xDatetime.minute;
        pIotDatetime->ucSecond = xDatetime.second;
        break;

    case eCancelRtcAlarm:
        SNVS_HP_RTC_DisableInterrupts(ptxt->pxRtcHandleBase, kSNVS_RTC_AlarmInterrupt);
        break;

     case eSetRtcWakeupTime:
        if (ptxt->ucStatus == IOT_RTC_NOT_STARTED) {
            return IOT_RTC_NOT_STARTED;
        }

        return IOT_RTC_FUNCTION_NOT_SUPPORTED;
        break;

    case eGetRtcWakeupTime:
        if (ptxt->ucStatus == IOT_RTC_NOT_STARTED) {
            return IOT_RTC_NOT_STARTED;
        }

        return IOT_RTC_FUNCTION_NOT_SUPPORTED;
        break;

    case eGetRtcStatus:
        *(IotRtcStatus_t *)pvBuffer = ptxt->ucStatus;
        break;

    default:
        return IOT_RTC_INVALID_VALUE;
        break;

    }
    return IOT_RTC_SUCCESS;
}

int32_t iot_rtc_close( IotRtcHandle_t const pxRtcHandle)
{
    Rtc_Ctxt_t *ptxt = (Rtc_Ctxt_t *)pxRtcHandle;
    if(pxRtcHandle == NULL)
    {
        return IOT_RTC_INVALID_VALUE;
    }

    if(ptxt->ucInitialized == 0)
    {
        return IOT_RTC_INVALID_VALUE;
    }

    DisableIRQ(SNVS_HP_WRAPPER_IRQn);
    SNVS_HP_RTC_DisableInterrupts(ptxt->pxRtcHandleBase, kSNVS_RTC_AlarmInterrupt);
    SNVS_HP_RTC_Deinit(ptxt->pxRtcHandleBase);

    ptxt->xCallback = NULL;
    ptxt->pvUserContext = NULL;
    ptxt->ucInitialized = 0;
    ptxt->ucStatus = eRtcTimerStopped;

    return IOT_RTC_SUCCESS;
}
