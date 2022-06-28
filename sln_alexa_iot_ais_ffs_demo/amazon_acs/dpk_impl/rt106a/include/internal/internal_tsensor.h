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

#ifndef INTERNAL_TSENSOR_H_
#define INTERNAL_TSENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "iot_tsensor.h"

/**
 * tsensor state
 */
typedef enum tsensorState_s {
    eTSensorOpened,
    eTSensorEnabled,
    eTSensorDisabled,
    eTSensorClosed,
} tsensorState_t;

/**
 * tsensor type
 */
typedef enum tsensorHandleType_s {
    eTSensorADC,
    eTSensorI2C,
} tsensorHandleType_t;

/**
 * tsensor device channel/address type
 */
typedef union tsensorUnit_s {
    uint8_t ucChannel;
    uint8_t ucAddress;
} tsensorUnit_t;

/**
 * tsensor context
 */
typedef struct tsensorHalContext_s{
    int32_t lInstance;                  //instance of context handle
    void * xpHandle;                    //handle for peripheral handle
    IotTsensorCallback_t xCallback;     //tsensor callback
    void * pvUserContext;               //user context for tsensor
    tsensorHandleType_t xHandleType;    //tsensor driver type
    tsensorState_t xHandleState;        //tsensor handle state
    tsensorUnit_t xUnit;                //tsensor device channel/address
    uint8_t ucDeviceReg;                //extra device register address
    int32_t ulMinThreshold;             //min threshold
    int32_t ulMaxThreshold;             //man threshold
} tsensorHalContext_t;

/**
 * Convert ADC reading to celsius
 *
 * @param: pxTsensorCtxt: context pass from tsensor handle.
 *
 * @return SUCCESS:IOT_TSENSOR_SUCCESS, FAIL:IOT_TSENSOR_INVALID_VALUE, NOT_SUPPORT:IOT_TSENSOR_NOT_SUPPORTED.
 */
int32_t tsensor_set_min_threshold(tsensorHalContext_t * const pxTsensorCtxt);

/**
 * Convert ADC reading to celsius
 *
 * @param: pxTsensorCtxt: context pass from tsensor handle.
 *
 * @return SUCCESS:IOT_TSENSOR_SUCCESS, FAIL:IOT_TSENSOR_INVALID_VALUE, NOT_SUPPORT:IOT_TSENSOR_NOT_SUPPORTED.
 */
int32_t tsensor_set_max_threshold(tsensorHalContext_t * const pxTsensorCtxt);

/**
 * Convert ADC reading to celsius
 *
 * @param: pxTsensorCtxt: context pass from tsensor handle.
 * @param: param: calibration data need from specific sensor.
 *
 * @return SUCCESS:IOT_TSENSOR_SUCCESS, FAIL:IOT_TSENSOR_INVALID_VALUE, NOT_SUPPORT:IOT_TSENSOR_NOT_SUPPORTED.
 */
int32_t tsensor_calibration(tsensorHalContext_t * const pxTsensorCtxt,
                            void * const param);

/**
 * Convert I2C temp sensor reading to celsius
 *
 * @param: adcData: data read from adc.
 * @param: cTemp: temp buffer pass by user.
 *
 * @return SUCCESS:IOT_TSENSOR_SUCCESS, FAIL:IOT_TSENSOR_INVALID_VALUE.
 */
int32_t tsensor_convert_i2c_data_to_celsius(size_t adcData,
                                            int32_t * cTemp);

/**
 * Enable tsensor device
 *
 * @param: pxTsensorCtxt: context pass from tsensor handle.
 *
 * @return SUCCESS:IOT_TSENSOR_SUCCESS, FAIL:IOT_TSENSOR_INVALID_VALUE, NOT_SUPPORT:IOT_TSENSOR_NOT_SUPPORTED.
 */
int32_t tsensor_enable_device(tsensorHalContext_t * const pxTsensorCtxt);

/**
 * disable tsensor device
 *
 * @param: pxTsensorCtxt: context pass from tsensor handle.
 *
 * @return SUCCESS:IOT_TSENSOR_SUCCESS, FAIL:IOT_TSENSOR_INVALID_VALUE
 */
int32_t tsensor_disable_device(tsensorHalContext_t * const pxTsensorCtxt);

#ifdef __cplusplus
}
#endif

#endif /* INTERNAL_TSENSOR_H_ */
