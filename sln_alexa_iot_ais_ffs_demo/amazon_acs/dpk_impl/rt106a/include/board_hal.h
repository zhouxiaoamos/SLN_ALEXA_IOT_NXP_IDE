/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates.  All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and conditions set
 * forth in the accompanying LICENSE.TXT file.  This file is a Modifiable File, as
 * defined in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY DISCLAIMS,
 * WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
 * INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef _BOARD_HAL_H_
#define _BOARD_HAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************************
 * ADC BOARD CONFIG
 *******************************************************************************/

#define ADC_MAX_INSTANCE    0 //adc max instance
#define ADC_NO_OF_PORTS     1 //number of adc ports
#define ADC_NO_OF_CHANNELS  13 //number of adc channels

//ADC convert to celsius table
#define ADC_CONVERT_BETA (3380)
#define ADC_CONVERT_ROOM_TEMP_K (298.15)
#define ADC_CONVERT_ZERO_TEMP_K (273.15)
#define ADC_CONVERT_ADC_MAX (4095)

/********************************************************************************
 * ADC MANAGER BOARD CONFIG
 *******************************************************************************/

#define ADC_MGR_SEMAPHORE_WAIT_MS   pdMS_TO_TICKS( 1500 ) //semaphore wait time

/********************************************************************************
 * I2C BOARD CONFIG
 *******************************************************************************/

#define I2C_MAX_PORTS    1

/********************************************************************************
 * TSENSOR BOARD CONFIG
 *******************************************************************************/

#include "internal_tsensor.h"

/**
 * Example sensors listed below. This needs to be properly initialized
 * to match what is supported by the hardware.
 */
uint8_t uTsensorNumber = 2; //1 ADC temp sensor

tsensorHalContext_t xTsensorCtxt0 =
{
    .lInstance = 0,
    .xpHandle = NULL,
    .xCallback = NULL,
    .pvUserContext = NULL,
    .xHandleType = eTSensorADC,
    .xHandleState = eTSensorClosed,
    .xUnit.ucChannel = 3,
    .ucDeviceReg = 0,
    .ulMinThreshold = INT32_MIN,
    .ulMaxThreshold = INT32_MAX,
};

tsensorHalContext_t xTsensorCtxt1 =
{
    .lInstance = 0,
    .xpHandle = NULL,
    .xCallback = NULL,
    .pvUserContext = NULL,
    .xHandleType = eTSensorI2C,
    .xHandleState = eTSensorClosed,
    .xUnit.ucAddress = 0x48,
    .ucDeviceReg = 0x00,
    .ulMinThreshold = INT32_MIN,
    .ulMaxThreshold = INT32_MAX,
};

tsensorHalContext_t * pxTsensorCtxts[] =
{
    &xTsensorCtxt0
};

/********************************************************************************
 * BATTERY BOARD CONFIG
 *******************************************************************************/

#include "internal_battery.h"

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_HAL_H_ */
