/*
 * Copyright 2019-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdio.h>
#include <string.h>

#include "stddef.h"
#include "app_aws_shadow.h"
#include "fsl_common.h"
#include "ux_attention_system.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Set to 1 to enable the default powerOn Shadow demo */
#define SHADOW_DEMO_POWERON 1

/* Set to 1 to enable the default brightness Shadow demo */
#define SHADOW_DEMO_BRIGHTNESS 1

/*******************************************************************************
 * Declarations
 ******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

status_t ShadowDemoPowerOn(const char *value, size_t length)
{
    status_t status = kStatus_Success;

    configPRINTF(("[SHADOW] Add Shadow powerOn logic here!\r\n"));

#if SHADOW_DEMO_POWERON
    if (value != NULL)
    {
        if ((strlen(SHADOW_POWERON_OFF) == length) &&
            (strncmp(value, SHADOW_POWERON_OFF, strlen(SHADOW_POWERON_OFF)) == 0))
        {
            configPRINTF(("[SHADOW] New powerOn state is OFF(%.*s)\r\n", length, value));
        }
        else if ((strlen(SHADOW_POWERON_ON) == length) &&
                 (strncmp(value, SHADOW_POWERON_ON, strlen(SHADOW_POWERON_ON)) == 0))
        {
            configPRINTF(("[SHADOW] New powerOn state is ON(%.*s)\r\n", length, value));
        }
        else
        {
            configPRINTF(("[SHADOW] New powerOn state is UNKNOWN(%.*s)\r\n", length, value));
        }
    }
    else
    {
        configPRINTF(("[SHADOW] ERROR: Failed to process powerOn change\r\n"));
        status = kStatus_Fail;
    }
#endif /* SHADOW_DEMO_POWERON */

    return status;
}

status_t ShadowDemoBrightness(const char *value, size_t length)
{
    status_t status = kStatus_Success;

    configPRINTF(("[SHADOW] Add Shadow brightness logic here!\r\n"));

#if SHADOW_DEMO_BRIGHTNESS
    if (value != NULL)
    {
        if ((strlen(SHADOW_BRIGHTNESS_LOW) == length) &&
            (strncmp(value, SHADOW_BRIGHTNESS_LOW, strlen(SHADOW_BRIGHTNESS_LOW)) == 0))
        {
            configPRINTF(("[SHADOW] New brightness state is LOW(%.*s)\r\n", length, value));
            ux_attention_set_state(uxLowBrightness);
        }
        else if ((strlen(SHADOW_BRIGHTNESS_MEDIUM) == length) &&
                 (strncmp(value, SHADOW_BRIGHTNESS_MEDIUM, strlen(SHADOW_BRIGHTNESS_MEDIUM)) == 0))
        {
            configPRINTF(("[SHADOW] New brightness state is MEDIUM(%.*s)\r\n", length, value));
            ux_attention_set_state(uxMediumBrightness);
        }
        else if ((strlen(SHADOW_BRIGHTNESS_HIGH) == length) &&
                 (strncmp(value, SHADOW_BRIGHTNESS_HIGH, strlen(SHADOW_BRIGHTNESS_HIGH)) == 0))
        {
            configPRINTF(("[SHADOW] New brightness state is HIGH(%.*s)\r\n", length, value));
            ux_attention_set_state(uxHighBrightness);
        }
        else
        {
            configPRINTF(("[SHADOW] New brightness state is UNKNOWN(%.*s)\r\n", length, value));
        }
    }
    else
    {
        configPRINTF(("[SHADOW] ERROR: Failed to process brightness change\r\n"));
        status = kStatus_Fail;
    }
#endif /* SHADOW_DEMO_BRIGHTNESS */

    return status;
}
