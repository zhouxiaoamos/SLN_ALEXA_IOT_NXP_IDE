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

/*******************************************************************************
 * IOT On-Target Unit Test Board Config
 * @File: test_iot_config.h
 * @Brief: File for define board configuation fro IOT HAL test
 *
 ******************************************************************************/

#pragma once

/* Typical test configuration */

/* UART */
#define UART_TEST_SET 1
extern const uint8_t uartTestPort[UART_TEST_SET];

/* FLASH */
#define FLASH_TEST_SET 1
extern const uint32_t flashTestStartOffset[FLASH_TEST_SET];

/* WATCHDOG */
#define WATCHDOG_TEST_SET 1

/* RTC */
#define RTC_TEST_SET 1

/* GPIO */
/*
 * Test general case, testCaseIndex used to pass gpio test config info
 * bits 0-7 : port
 * bit 8    : direction, 0 - input, 1 - output
 * bit 9    : IRQ, 0 - non IRQ, 1 - IRQ
 * bit 10   : value, 0 - low, 1 - high, only valid for output case where bit 8 = 1
 * example  : testCaseIndex = 513 ( 0x201 ), input for port 1 with IRQ
 *            testCaseIndex = 1282 ( 0x502 ), output for port 2 with high
 */
#define GPIO_TEST_SET 1
extern const uint16_t gpioTestPortA[GPIO_TEST_SET];
extern const uint16_t gpioTestPortB[GPIO_TEST_SET];
extern const uint16_t gpioTestConfig[GPIO_TEST_SET];
extern const uint32_t gpioTestFunction[GPIO_TEST_SET];

/* TIMER */
#define TIMER_TEST_SET 1

/* ADC */
#define ADC_TEST_SET 1
#define ADC_TEST_COMPONENT 3
extern const uint8_t adcTestChListLen[ADC_TEST_SET];
extern const uint8_t adcTestChListArray[];
extern const uint8_t *adcTestChList[ADC_TEST_SET];

/* RESET */
#define RESET_TEST_SET 1

/* PERFCOUNTER */
#define PERFCOUNTER_TEST_SET 1

/* PWM */
#define PWM_TEST_SET 1
extern const uint32_t pwmTestFrequency[PWM_TEST_SET]; /* 1KHz frequency */
extern const uint32_t pwmTestDutyCycle[PWM_TEST_SET];
#define TEST_PWM_IOT_GPIO_PIN 18
#define TEST_PWM_IOT_INSTANCE 1

/* I2C */
#define I2C_TEST_SET 1
extern const uint8_t i2cTestSlaveAddr[I2C_TEST_SET];
extern const size_t i2cTestDeviceRegister[I2C_TEST_SET];
extern const uint8_t i2cTestWriteVal[I2C_TEST_SET];

/* SPI */
#define SPI_TEST_SET 1

/* POWER */
#define POWER_TEST_SET 1

/* SDIO */
#define SDIO_TEST_SET 1

/* TEMP SENSOR */
#define TEMP_SENSOR_TEST_SET 1

/* BATTERY */
#define BATTERY_TEST_SET 1

/* EFUSE */
#define EFUSE_TEST_SET 1
const uint32_t efuseTest16BitWordValidIdx[EFUSE_TEST_SET];
const uint32_t efuseTest16BitWordInvalidIdx[EFUSE_TEST_SET];
const uint16_t efuseTest16BitWordWriteVa[EFUSE_TEST_SET];
const uint32_t efuseTest32BitWordValidIdx[EFUSE_TEST_SET];
const uint32_t efuseTest32BitWordInvalidIdx[EFUSE_TEST_SET];
const uint32_t efuseTest32BitWordWriteVal[EFUSE_TEST_SET];
