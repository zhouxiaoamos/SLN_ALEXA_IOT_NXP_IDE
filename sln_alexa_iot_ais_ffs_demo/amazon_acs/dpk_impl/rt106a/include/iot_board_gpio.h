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

#ifndef _BOARD_GPIO_H_
#define _BOARD_GPIO_H_

#include <stdint.h>
#include <stddef.h>
#include "iot_gpio.h"

/* GPIO board specific config */
typedef struct GPIO_Port_PinNumber_Pair
{
  const uint8_t ucUserPinNumber; /* Logical User Pin number */
  const uint8_t ucPort;          /* Gpio Port */
  const uint8_t ucHalPinNumber;  /* Physical GPio Pin number */
} GPIO_Port_PinNumber_Pair_t;

#define MAX_GPIO_PORTS ( 36 )
#define GPIO_PORT0 ( 0 )
#define GPIO_PORT1 ( 1 )
#define GPIO_PORT2 ( 2 )

/* defines below shall match xPinMap defined in board_gpio.c */
#define GPIO_PORT_VOL_UP ( 1 )
#define GPIO_PORT_VOL_DOWN ( 2 )
#define GPIO_PORT_ACTION_BTN ( 7 )
#define GPIO_PORT_LED_DRIVE_EN ( 9 )
#define GPIO_PORT_EN_3V3_WIFI ( 11 )
#define GPIO_PORT_EN_1V8_WIFI ( 12 )
#define GPIO_PORT_RESET_DAC ( 20 )
#define GPIO_PORT_MUTE_STATUS ( 25 )
#define GPIO_PORT_WIFI_PMU_EN ( 29 )

/* define GPIO ports used for AFQP test */
#define IOT_GPIO_TEST_PORT_A ( 17 )
#define IOT_GPIO_TEST_PORT_B ( 18 )

extern const GPIO_Port_PinNumber_Pair_t xPinMap[ MAX_GPIO_PORTS ];

#endif /* _BOARD_GPIO_H_ */
