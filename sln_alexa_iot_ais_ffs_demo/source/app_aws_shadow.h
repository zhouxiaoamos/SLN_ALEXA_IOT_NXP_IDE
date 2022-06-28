/*
 * Copyright 2019-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _APP_AWS_SHADOW_H_
#define _APP_AWS_SHADOW_H_

#include "stddef.h"
#include "fsl_common.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define SHADOW_POWERON_FIELD "powerOn"
#define SHADOW_POWERON_OFF   "0"
#define SHADOW_POWERON_ON    "1"

#define SHADOW_BRIGHTNESS_FIELD  "brightness"
#define SHADOW_BRIGHTNESS_LOW    "\"LOW\""
#define SHADOW_BRIGHTNESS_MEDIUM "\"MEDIUM\""
#define SHADOW_BRIGHTNESS_HIGH   "\"HIGH\""

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/*!
 * @brief Process the updated powerOn value; called by _shadowDeltaCallback
 *
 * @params value String containing the new value of the powerOn field
 * @params length The length of the string pointed by the value parameter
 *
 * @returns Returns the result of applying the new powerOn value
 */
status_t ShadowDemoPowerOn(const char *value, size_t length);

/*!
 * @brief Process the updated brightness value; called by _shadowDeltaCallback
 *
 * @params value String containing the new value of the brightness field
 * @params length The length of the string pointed by the value parameter
 *
 * @returns Returns the result of applying the new brightness value
 */
status_t ShadowDemoBrightness(const char *value, size_t length);

#if defined(__cplusplus)
}
#endif

#endif /* _APP_AWS_SHADOW_H_ */
