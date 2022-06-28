/*
 * Copyright 2019-2021 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdio.h>

#include "fsl_gpt.h"
#include "perf.h"

#ifdef SLN_TRACE_CPU_USAGE
/* In this configuration, PERF_TIMER_GPT will overflow after ~10 hours and
 * has a resolution of 1 tick == 8.33us;
 * In case the MQS is used, this configuration should not be changed because
 * it is also used for the barge-in purposes. */
#define PERF_TIMER_GPT          GPT2
#define PERF_TIMER_GPT_FREQ_MHZ 24
#define PERF_TIMER_GPT_PS       200

/* Output of vTaskGetRunTimeStats requires ~40 bytes per task */
static char perfBuffer[30 * 40];

void PERF_InitTimer(void)
{
    gpt_config_t gpt;

    /* The timer is set with a PERF_TIMER_GPT_FREQ_MHZ clock freq. */
    CLOCK_SetMux(kCLOCK_PerclkMux, 1);
    CLOCK_SetDiv(kCLOCK_PerclkDiv, 0);

    GPT_GetDefaultConfig(&gpt);
    gpt.divider = PERF_TIMER_GPT_PS;
    GPT_Init(PERF_TIMER_GPT, &gpt);

    GPT_StartTimer(PERF_TIMER_GPT);
}

uint32_t PERF_GetTimer(void)
{
    return GPT_GetCurrentTimerCount(PERF_TIMER_GPT);
}

char *PERF_GetCPULoad(void)
{
    vTaskGetRunTimeStats(perfBuffer);
    return perfBuffer;
}
#endif /* SLN_TRACE_CPU_USAGE */

void PERF_PrintHeap(void)
{
    configPRINTF(("  min heap remaining: %d\r\n", xPortGetMinimumEverFreeHeapSize()));
}

void PERF_PrintStack(TaskHandle_t task_id)
{
    configPRINTF(("  stack high water mark: %d\r\n", uxTaskGetStackHighWaterMark(task_id)));
}

void PERF_PrintStacks(void)
{
    TaskStatus_t *pxTaskStatusArray;
    TaskStatus_t *pxTaskStatus;
    UBaseType_t uxArraySize, x;
    int8_t *pcWriteBuffer = NULL;
    uint32_t pos          = 0;

    uxArraySize = uxTaskGetNumberOfTasks();

    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if (pxTaskStatusArray != NULL)
    {
        /* Generate the (binary) data. */
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);

        pcWriteBuffer = pvPortMalloc(128);
        if (pcWriteBuffer != NULL)
        {
            configPRINTF(("\r\n"));
            configPRINTF(("Task Name\t\t\tStackHighWaterMark\r\n"));
            for (x = 0; x < uxArraySize; x++)
            {
                pxTaskStatus = &pxTaskStatusArray[x];

                memcpy(pcWriteBuffer, pxTaskStatus->pcTaskName, strlen(pxTaskStatus->pcTaskName));
                pos = strlen(pxTaskStatus->pcTaskName);
                pos += sprintf((char *)(pcWriteBuffer + pos), "\t\t\t%u\t\t",
                               (uint16_t)pxTaskStatus->usStackHighWaterMark);

                configPRINTF(("%s\r\n", pcWriteBuffer));
                pos = 0;
                vTaskDelay(10); // Give serial port some time to dump logs
            }

            vPortFree(pcWriteBuffer);
            pcWriteBuffer = NULL;
        }
        else
        {
            configPRINTF(("%s: No memory for print buffer\r\n", __func__));
        }
        vPortFree(pxTaskStatusArray);
        pxTaskStatusArray = NULL;
    }
    else
    {
        configPRINTF(("%s: No memory for storing task status\r\n", __func__));
    }
}
