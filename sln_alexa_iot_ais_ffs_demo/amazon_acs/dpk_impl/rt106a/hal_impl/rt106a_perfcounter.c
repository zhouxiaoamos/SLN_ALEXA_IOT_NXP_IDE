/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */


#include "FreeRTOSConfig.h"

#define MHZ (1000000)

void iot_perfcounter_open(void)
{
    // Nothing to do, high frequency timer is already running
}

uint64_t iot_perfcounter_get_value(void)
{
    return portGET_RUN_TIME_COUNTER_VALUE();
}

uint32_t iot_perfcounter_get_frequency(void)
{
    return MHZ;
}

void iot_perfcounter_close(void)
{
    // Nothing to do
}
