/*
 * Copyright 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "board.h"
#include <iot_reset.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sln_reset.h"

void iot_reset_reboot(IotResetBootFlag_t coldBootFlag)
{
    (void) coldBootFlag;

    sln_reset(NULL);
}

int32_t iot_reset_shutdown()
{
    return IOT_RESET_FUNCTION_NOT_SUPPORTED;
}

int32_t iot_get_reset_reason(IotResetReason_t *xResetReason)
{
    return IOT_RESET_FUNCTION_NOT_SUPPORTED;
}
