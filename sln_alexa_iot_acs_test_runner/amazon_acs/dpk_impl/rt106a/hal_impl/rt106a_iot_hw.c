/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdint.h>
#include "iot_hw.h"

/********************************************************************************
 * Defines
 *******************************************************************************/

/********************************************************************************
 * Globals
 *******************************************************************************/

/********************************************************************************
 * Internal
 *******************************************************************************/

/********************************************************************************
 * API implementation
 *******************************************************************************/
uint16_t iot_hw_get_id(void)
{
    return IOT_HW_UNSUPPORTED;
}

uint16_t iot_hw_get_rev(void)
{
    return IOT_HW_UNSUPPORTED;
}
