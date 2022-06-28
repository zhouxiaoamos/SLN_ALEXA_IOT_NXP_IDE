/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*******************************************************************************
 * Inlcudes
 ******************************************************************************/
#include "aisv2.h"

#include "mbedtls/base64.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 *******************************************************************************/

void AIS_AppCallback_PowerController(ais_handle_t *handle, aia_smart_home_queue_payload_t *smart_home_msg)
{
    /* Write code here to handle Power Controller */

    if (handle->smart_home.s_smartHomeLock != NULL)
    {
        /* The below variables are the state of the endpoint */
        xSemaphoreTake(handle->smart_home.s_smartHomeLock, portMAX_DELAY);

        configPRINTF(("Power = %d\r\n", smart_home_msg->endpoint->powerController->on));

        switch (smart_home_msg->controller_directive_type)
        {
            case AIS_SMART_HOME_POWER_CONTROLLER_TURN_ON:
                /* Handle the turn on command here */
                configPRINTF(("[App Smart Home] - Power On!\r\n"));
                break;

            case AIS_SMART_HOME_POWER_CONTROLLER_TURN_OFF:
                /* Handle the turn off command here */
                configPRINTF(("[App Smart Home] - Power Off!\r\n"));
                break;

            default:
                configPRINTF(("[App Smart Home] - Unknown Power Controller Command\r\n"));
                break;
        }

        xSemaphoreGive(handle->smart_home.s_smartHomeLock);
    }
    else
    {
        configPRINTF(("[App Smart Home] - Smart Home Semiphore Not Initialized\r\n"));
    }
}

void AIS_AppCallback_ToggleController(ais_handle_t *handle, aia_smart_home_queue_payload_t *smart_home_msg)
{
    /* Write code here to handle Toggle Controller */
    /* The below variables are the state of the endpoint */

    if (handle->smart_home.s_smartHomeLock != NULL)
    {
        xSemaphoreTake(handle->smart_home.s_smartHomeLock, portMAX_DELAY);

        configPRINTF(("Toggle = %d", smart_home_msg->endpoint->toggleController->on));

        switch (smart_home_msg->controller_directive_type)
        {
            case AIS_SMART_HOME_TOGGLE_CONTROLLER_TURN_ON:
                /* Handle the turn on command here */
                configPRINTF(("[App Smart Home] - Toggle On!\r\n"));
                break;

            case AIS_SMART_HOME_TOGGLE_CONTROLLER_TURN_OFF:
                configPRINTF(("[App Smart Home] - Toggle Off!\r\n"));
                /* Handle the turn off command here */
                break;

            default:
                configPRINTF(("[App Smart Home] - Unknown Power Controller Command\r\n"));
                break;
        }

        xSemaphoreGive(handle->smart_home.s_smartHomeLock);
    }
    else
    {
        configPRINTF(("[App Smart Home] - Smart Home Semiphore Not Initialized\r\n"));
    }
}

void AIS_AppCallback_RangeController(ais_handle_t *handle, aia_smart_home_queue_payload_t *smart_home_msg)
{
    /* Write code here to handle Range Controller */

    if (handle->smart_home.s_smartHomeLock != NULL)
    {
        /* The below variables are the state of the endpoint */
        xSemaphoreTake(handle->smart_home.s_smartHomeLock, portMAX_DELAY);

        configPRINTF(("Range = %d\r\n", smart_home_msg->endpoint->rangeController->range));
        configPRINTF(("Range Delta = %d\r\n", smart_home_msg->endpoint->rangeController->rangeDelta));
        configPRINTF(("Range Delta Default = %d\r\n", smart_home_msg->endpoint->rangeController->rangeDeltaDefault));

        switch (smart_home_msg->controller_directive_type)
        {
            case AIS_SMART_HOME_RANGE_CONTROLLER_SET_RANGE:
                /* Handle the turn on command here */
                configPRINTF(("[App Smart Home] - Set Range!\r\n"));
                break;

            case AIS_SMART_HOME_RANGE_CONTROLLER_ADJUST_RANGE:
                /* Handle the turn off command here */
                configPRINTF(("[App Smart Home] - Adjust Range!\r\n"));
                break;

            default:
                configPRINTF(("[App Smart Home] - Unknown Power Controller Command"));
                break;
        }

        xSemaphoreGive(handle->smart_home.s_smartHomeLock);
    }
    else
    {
        configPRINTF(("[App Smart Home] - Smart Home Semiphore Not Initialized\r\n"));
    }
}
void AIS_AppCallback_ModeController(ais_handle_t *handle, aia_smart_home_queue_payload_t *smart_home_msg)
{
    /* Write code here to handle Mode Controller */

    if (handle->smart_home.s_smartHomeLock != NULL)
    {
        xSemaphoreTake(handle->smart_home.s_smartHomeLock, portMAX_DELAY);

        configPRINTF(("Mode = %d\r\n", smart_home_msg->endpoint->modeController->mode));
        configPRINTF(("Mode Delta = %d\r\n", smart_home_msg->endpoint->modeController->modeDelta));
        switch (smart_home_msg->controller_directive_type)
        {
            case AIS_SMART_HOME_MODE_CONTROLLER_SET:
                /* Handle the turn on command here */
                configPRINTF(("[App Smart Home] - Mode Set!\r\n"));
                break;

            case AIS_SMART_HOME_MODE_CONTROLLER_ADJUST:
                configPRINTF(("[App Smart Home] - Mode Adjust!\r\n"));

                /* Handle the turn off command here */
                break;

            default:
                configPRINTF(("[App Smart Home] - Unknown Power Controller Command\r\n"));
                break;
        }

        xSemaphoreGive(handle->smart_home.s_smartHomeLock);
    }
    else
    {
        configPRINTF(("[App Smart Home] - Smart Home Semiphore Not Initialized\r\n"));
    }
}

void AIS_AppCallback_BrightnessController(ais_handle_t *handle, aia_smart_home_queue_payload_t *smart_home_msg)
{
    /* Write code here to handle Brightness Controller */

    if (handle->smart_home.s_smartHomeLock != NULL)
    {
        /* The below variables are the state of the endpoint */
        xSemaphoreTake(handle->smart_home.s_smartHomeLock, portMAX_DELAY);

        configPRINTF(("Brightness = %d\r\n", smart_home_msg->endpoint->brightnessController->brightness));
        configPRINTF(("Brightness Delta = %d\r\n", smart_home_msg->endpoint->brightnessController->brightnessDelta));

        switch (smart_home_msg->controller_directive_type)
        {
            case AIS_SMART_HOME_BRIGHTNESS_CONTROLLER_SET:
                /* Handle the turn on command here */
                configPRINTF(("[App Smart Home] - Set Brightness!\r\n"));
                break;

            case AIS_SMART_HOME_BRIGHTNESS_CONTROLLER_ADJUST:
                /* Handle the turn off command here */
                configPRINTF(("[App Smart Home] - Adjust Brightness!\r\n"));
                break;

            default:
                configPRINTF(("[App Smart Home] - Unknown Power Controller Command\r\n"));
                break;
        }

        xSemaphoreGive(handle->smart_home.s_smartHomeLock);
    }
    else
    {
        configPRINTF(("[App Smart Home] - Smart Home Semiphore Not Initialized\r\n"));
    }
}
