/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "stdint.h"
#include <iot_timer.h>
#include "fsl_pit.h"
#include "rt106a_iot_log.h"
#include "FreeRTOS.h"

#define MS_TO_US    1000
typedef enum {
    TIMER_OPENED = 1,
    TIMER_RUNNING,
    TIMER_STOPPED
} timer_status_t;

static const char *TAG = "rt106a-iot-timer";
#define TIMER0_INSTANCE     1
#define TIMER1_INSTANCE     2

typedef struct {
    uint64_t timeout_us;
    uint64_t start_time;
    timer_status_t status;
    void (*func)(void *arg);
    void *arg;
    uint8_t timer_index;
} timer_ctx_t;

/*
 * RT106A has one PIT module with 4 channels
 * Timer0 configured with kPIT_Chnl_1/kPIT_Chnl_2/kPIT_Chnl_3. Timer Precision is 1us.
 * Overtimer is 0xFFFFFFFFFFFFFFFF/1000/1000/60/60/24/365 = 584942 years
 * Timer1 configured with kPIT_Chnl_3. So far only timer0 is supported
 */
static timer_ctx_t *pgtimer[1] = {NULL};
#define TIMER_NUM (sizeof(pgtimer)/sizeof(pgtimer[0]))

static uint64_t read_timer0()
{
    uint32_t timer0 = 0, timer1 = 0, timer2 = 0, timer0_1 = 0;

    timer2 = PIT_GetCurrentTimerCount(PIT, kPIT_Chnl_3);
    timer1 = PIT_GetCurrentTimerCount(PIT, kPIT_Chnl_2);
    timer0 = PIT_GetCurrentTimerCount(PIT, kPIT_Chnl_1);
    timer0_1 = PIT_GetCurrentTimerCount(PIT, kPIT_Chnl_1);

    if (timer0_1 > timer0) { //timer0 overflow
        if (timer1 == 0) {
            timer2--; //timer2 never overflow
        } else {
            timer1--;
        }
    }

    timer2 = 0xFFFFFFFF - timer2;
    timer1 = 0xFFFFFFFF - timer1;
     /*
      * 6144 is the threshold of channel0. timer resolution is 1us.
      * timer interrupt period is 256. In future if CPU load needs to be reduce,
      * we may increase the period of interrupt to 32us/64us/128us/256us ...
      * Here we do not use USEC_TO_COUNT(256, CLOCK_GetFreq(kCLOCK_IpgClk) to instead of 33792, since
      * I would not like consume much time in interrupt context.
      */
    timer0 = (33792 - timer0) / 132;

    return (((uint64_t)timer2 << 32 | (uint64_t)timer1) << 8) + timer0;
}

static void timer_cb(timer_ctx_t *tm_ctx)
{
    if (tm_ctx->func) {
        tm_ctx->func(tm_ctx->arg);
    }
}

void timer_IRQHandler(void)
{
    uint64_t current_time = 0;
    PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
    timer_ctx_t * pctx = pgtimer[0];
    if (pctx->timeout_us != 0) {
        current_time = read_timer0();
        if (current_time > pctx->start_time) {
            if ((current_time - pctx->start_time) >= pctx->timeout_us) {
                pctx->timeout_us = 0;
                timer_cb(pctx);
            }
        }
    }
}

static void init_timer0(timer_ctx_t *tm_ctx)
{
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_3, 0xFFFFFFFF);
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_2, 0xFFFFFFFF);
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, USEC_TO_COUNT(256, CLOCK_GetFreq(kCLOCK_IpgClk)) - 1);

    PIT_SetTimerChainMode(PIT, kPIT_Chnl_2, 1);
    PIT_SetTimerChainMode(PIT, kPIT_Chnl_3, 1);

    PIT_EnableInterrupts(PIT, kPIT_Chnl_1, kPIT_TimerInterruptEnable);

    tm_ctx->status = TIMER_OPENED;
}

static void start_timer0()
{
    PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
    PIT_EnableInterrupts(PIT, kPIT_Chnl_1, kPIT_TimerInterruptEnable);
    PIT_StartTimer(PIT, kPIT_Chnl_3);
    PIT_StartTimer(PIT, kPIT_Chnl_2);
    PIT_StartTimer(PIT, kPIT_Chnl_1);
}

static void stop_timer0()
{
    PIT_DisableInterrupts(PIT, kPIT_Chnl_1, kPIT_TimerInterruptEnable);
    PIT_StopTimer(PIT, kPIT_Chnl_1);
    PIT_StopTimer(PIT, kPIT_Chnl_2);
    PIT_StopTimer(PIT, kPIT_Chnl_3);
}

IotTimerHandle_t iot_timer_open( int32_t lTimerInstance )
{
    // needs to  work on instance 1 from time_utils.c
    if (lTimerInstance != TIMER0_INSTANCE) {
        RT106A_LOGD(TAG, "Invalid instance of timer");
        return NULL;
    }

    if(pgtimer[lTimerInstance-1] != NULL)
        return NULL;

    timer_ctx_t *tm_ctx = (timer_ctx_t *) pvPortMalloc(sizeof(timer_ctx_t));
    if (tm_ctx == NULL) {
        RT106A_LOGD(TAG, "Could not allocate memory for the timer");
        return NULL;
    }
    memset(tm_ctx, 0, sizeof(*tm_ctx));

    tm_ctx->timer_index = lTimerInstance;

    pit_config_t pitConfig;

    /*
     * pitConfig.enableRunInDebug = false;
     */
    PIT_GetDefaultConfig(&pitConfig);

    /* Init pit module */
    PIT_Init(PIT, &pitConfig);
    NVIC_SetPriority(PIT_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY-1);
    NVIC_EnableIRQ(PIT_IRQn);

    init_timer0(tm_ctx);
    pgtimer[0] = tm_ctx;

    return (IotTimerHandle_t) tm_ctx;
}

void iot_timer_set_callback( IotTimerHandle_t const pxTimerHandle,
                             IotTimerCallback_t xCallback,
                             void *pvUserContext )
{
    if (pxTimerHandle == NULL) {
        RT106A_LOGD(TAG, "Invalid Timer Handler %s", __func__);
        return;
    }
    timer_ctx_t *tm_ctx = (timer_ctx_t *) pxTimerHandle;

    tm_ctx->func = xCallback;
    tm_ctx->arg = pvUserContext;
}

int32_t iot_timer_start( IotTimerHandle_t const pxTimerHandle )
{
    if (pxTimerHandle == NULL) {
        RT106A_LOGD(TAG, "Invalid Timer Handler %s", __func__);
        return IOT_TIMER_INVALID_VALUE;
    }

    timer_ctx_t *tm_ctx = (timer_ctx_t *) pxTimerHandle;

    if (tm_ctx->status == TIMER_RUNNING) {
        return IOT_TIMER_SUCCESS;
    }

    if (tm_ctx->timer_index != TIMER0_INSTANCE) {
        return IOT_TIMER_INVALID_VALUE;
    }

    start_timer0();
    tm_ctx->start_time = read_timer0();
    tm_ctx->status = TIMER_RUNNING;

    return IOT_TIMER_SUCCESS;
}

int32_t iot_timer_stop( IotTimerHandle_t const pxTimerHandle )
{
    if (pxTimerHandle == NULL) {
        RT106A_LOGD(TAG, "Invalid Timer Handler %s", __func__);
        return IOT_TIMER_INVALID_VALUE;
    }
    timer_ctx_t *tm_ctx = (timer_ctx_t *) pxTimerHandle;

    if(tm_ctx->status != TIMER_RUNNING) {
        return IOT_TIMER_NOT_RUNNING;
    }

    stop_timer0();
    tm_ctx->status = TIMER_STOPPED;

    return IOT_TIMER_SUCCESS;
}

int32_t iot_timer_get_value( IotTimerHandle_t const pxTimerHandle,
                             uint64_t *ulMicroSeconds )
{
    if (pxTimerHandle == NULL) {
        RT106A_LOGD(TAG, "Invalid Timer Handler %s", __func__);
        return IOT_TIMER_INVALID_VALUE;
    }

    if (ulMicroSeconds == NULL) {
        RT106A_LOGE(TAG, "NULL buffer");
        return IOT_TIMER_INVALID_VALUE;
    }
    timer_ctx_t *tm_ctx = (timer_ctx_t *) pxTimerHandle;
    if (tm_ctx->status != TIMER_RUNNING) {
        return IOT_TIMER_NOT_RUNNING;
    }

    if (tm_ctx->timer_index != TIMER0_INSTANCE) {
        return IOT_TIMER_INVALID_VALUE;
    }

    *ulMicroSeconds = read_timer0();

    return IOT_TIMER_SUCCESS;
}

int32_t iot_timer_delay( IotTimerHandle_t const pxTimerHandle,
                         uint32_t ulDelayMicroSeconds )
{
    if (pxTimerHandle == NULL) {
        RT106A_LOGD(TAG, "Invalid Timer Handler %s", __func__);
        return IOT_TIMER_INVALID_VALUE;
    }

    timer_ctx_t *tm_ctx = (timer_ctx_t *) pxTimerHandle;

    if (tm_ctx->timer_index != TIMER0_INSTANCE) {
        return IOT_TIMER_INVALID_VALUE;
    }

    tm_ctx->start_time = read_timer0();

    tm_ctx->timeout_us = ulDelayMicroSeconds;
    return IOT_TIMER_SUCCESS;
}

int32_t iot_timer_close( IotTimerHandle_t const pxTimerHandle )
{
    if (pxTimerHandle == NULL) {
        RT106A_LOGD(TAG, "Invalid Timer Handler %s", __func__);
        return IOT_TIMER_INVALID_VALUE;
    }

    timer_ctx_t *tm_ctx = (timer_ctx_t *) pxTimerHandle;

    if (tm_ctx->timer_index != TIMER0_INSTANCE) {
        return IOT_TIMER_INVALID_VALUE;
    }

    stop_timer0();
    pgtimer[0] = NULL;

    NVIC_DisableIRQ(PIT_IRQn);
    PIT_Deinit(PIT);

    memset(tm_ctx, 0, sizeof(*tm_ctx));
    vPortFree(tm_ctx);
    tm_ctx = NULL;
    return IOT_TIMER_SUCCESS;
}

int32_t iot_timer_cancel( IotTimerHandle_t const pxTimerHandle)
{
    if (pxTimerHandle == NULL) {
        RT106A_LOGD(TAG, "Invalid Timer Handler %s", __func__);
        return IOT_TIMER_INVALID_VALUE;
    }

    timer_ctx_t *tm_ctx = (timer_ctx_t *) pxTimerHandle;

    if (tm_ctx->status != TIMER_RUNNING || tm_ctx->timeout_us == 0) {
        //TODO based on iot_timer.h statements, here should return IOT_TIMER_NOTHING_TO_CANCEL, but it's not defined.
        //need to check with Amazon what value we should return here.
        return IOT_TIMER_NOT_RUNNING;
    }

    tm_ctx->timeout_us = 0;
    return IOT_TIMER_SUCCESS;
}
