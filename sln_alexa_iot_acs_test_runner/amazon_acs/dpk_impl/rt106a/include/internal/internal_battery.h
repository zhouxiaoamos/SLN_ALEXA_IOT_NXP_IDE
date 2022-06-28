/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file. This file is a
 * Modifiable File, as defined in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef INTERNAL_BATTERY_H_
#define INTERNAL_BATTERY_H_

#include "iot_battery.h"

#ifdef __cplusplus
extern "C" {
#endif

/* @brief battery Interface */
typedef struct IotBattery_Interface_s
{
    uint32_t lAdcInstance;  /* ADC instance */
    uint8_t ucAdcChannel;   /* ADC channel */
} IotBattery_Interface_t;

/* @brief battery context */
typedef struct IotBattery_Ctxt_s
{
    IotBatteryCallback_t xCallback;             /* callback */
    void* pvUserContext;                        /* user context to provide in callback */
    IotBatteryInfo_t xBatteryInfo;              /* Battery Info */
    IotBattery_Interface_t xBatteryInterface;    /* Battery Interface info like ADC info */
    uint16_t usBatteryMinVoltageThreshold;      /* Battery minimum operating voltage threshold */
    uint16_t usBatteryMaxVoltageThreshold;      /* Battery maximum operating voltage threshold */
    int16_t  sBatteryMinBatteryTempThreshold;   /* Battery minimim operating temperature threshold */
    int32_t  lBatteryMaxBatteryTempThreshold;   /* Battery maximim operating temperature threshold */
    int16_t  sBatteryMinChargeTempThreshold;    /* Battery minimum temperature threshold for charging */
    int32_t  lBatteryMaxChargeTempThreshold;    /* Battery maximum temperature threshold for charging */
    uint32_t ulBatteryMaxOutputCurrent;         /* Battery maximum ouput current limit */
    uint32_t ulBatteryMaxInputChargeCurrent;    /* Battery maximum input current limit from charging */
    uint32_t ulBatteryChargeTimer;              /* Battery maximum charging time limit */
    IotBatteryStatus_t xStatus;                 /* Battery/Charging status */
} IotBattery_Ctxt_t;

int32_t battery_convert_voltage_to_charging_level(uint16_t voltage, uint8_t * chargingLevel);

#ifdef __cplusplus
}
#endif

#endif /* INTERNAL_BATTERY_H_ */

