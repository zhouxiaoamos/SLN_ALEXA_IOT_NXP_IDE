/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <time.h>

#include "aisv2.h"
#include "aisv2_app.h"

#include "mbedtls/base64.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static cJSON *AIA_AlexaSmartHomeBrightnessControllerSetBrightness(ais_handle_t *handle,
                                                                  cJSON *payload,
                                                                  aia_smart_home_response_payload_t *response,
                                                                  ais_avs_smart_home_endpoint_t *endpoint);
static cJSON *AIA_AlexaSmartHomeBrightnessControllerAdjustBrightness(ais_handle_t *handle,
                                                                     cJSON *payload,
                                                                     aia_smart_home_response_payload_t *response,
                                                                     ais_avs_smart_home_endpoint_t *endpoint);

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

status_t AIA_AlexaSmartHomeBrightnessControllerHandler(ais_handle_t *handle, cJSON *json)
{
    cJSON *directive, *name_space, *name, *correlationToken, *endpointId, *endpoint, *payload, *header;
    aia_smart_home_response_payload_t *response;
    ais_avs_smart_home_endpoint_t *sh_endpoint;
    status_t status = kStatus_Fail;

    response = (aia_smart_home_response_payload_t *)pvPortMalloc(sizeof(aia_smart_home_response_payload_t));

    if (response != NULL)
    {
        /* Brightness Controller Endpoint Handler */
        directive        = cJSON_GetObjectItemCaseSensitive(json, "directive");
        header           = cJSON_GetObjectItemCaseSensitive(directive, "header");
        name_space       = cJSON_GetObjectItemCaseSensitive(header, "namespace");
        name             = cJSON_GetObjectItemCaseSensitive(header, "name");
        correlationToken = cJSON_GetObjectItemCaseSensitive(header, "correlationToken");
        endpoint         = cJSON_GetObjectItemCaseSensitive(directive, "endpoint");
        endpointId       = cJSON_GetObjectItemCaseSensitive(endpoint, "endpointId");
        payload          = cJSON_GetObjectItemCaseSensitive(directive, "payload");

        sh_endpoint = AIA_SmartHomeGetEndpoint(handle, endpointId->valuestring);

        if (sh_endpoint != NULL)
        {
            response->correlationToken = correlationToken->valuestring;
            response->endpointId       = endpointId->valuestring;

            if (strcmp(name->valuestring, "SetBrightness") == 0)
            {
                response->context.json =
                    AIA_AlexaSmartHomeBrightnessControllerSetBrightness(handle, payload, response, sh_endpoint);
            }

            if (strcmp(name->valuestring, "AdjustBrightness") == 0)
            {
                response->context.json =
                    AIA_AlexaSmartHomeBrightnessControllerAdjustBrightness(handle, payload, response, sh_endpoint);
            }

            status = AIA_AlexaSmartHomeResponse(handle, response);
        }
        else
        {
            configPRINTF(("[SmartHome] Failed to find matching endpoint %s", endpointId->valuestring));
        }

        vPortFree(response);
    }

    return status;
}

cJSON *AIA_AlexaSmartHomeBrightnessControllerCreateResponseContext(ais_handle_t *handle,
                                                                   ais_avs_smart_home_endpoint_t *endpoint)
{
    cJSON *response;
    ais_app_data_t *appData                         = AIS_APP_GetAppData();
    char time_buff[AIA_SMART_HOME_TIME_BUFFER_SIZE] = {0};

    /* This function is responsible for performing the turn on function.
    ** Please note, this is still in MQTT Callback context.
    */

    response = cJSON_CreateObject();

    cJSON_AddItemToObject(response, "namespace", cJSON_CreateString("Alexa.BrightnessController"));
    cJSON_AddItemToObject(response, "name", cJSON_CreateString("brightness"));

    xSemaphoreTake(handle->smart_home.s_smartHomeLock, portMAX_DELAY);
    cJSON_AddItemToObject(response, "value", cJSON_CreateNumber(endpoint->brightnessController->brightness));
    xSemaphoreGive(handle->smart_home.s_smartHomeLock);

    time_t epoch = appData->currTime - AIA_TIME_EPOCH_ADJUST;

    strftime(time_buff, sizeof(time_buff), "%Y-%m-%dT%H:%M:%SZ", gmtime(&epoch));
    time_buff[AIA_SMART_HOME_TIME_BUFFER_SIZE - 2] = 'Z';
    cJSON_AddItemToObject(response, "timeOfSample", cJSON_CreateString(time_buff));
    cJSON_AddItemToObject(response, "uncertaintyInMilliseconds", cJSON_CreateNumber(AIA_SMART_HOME_UNCERTAINTY_IN_MS));

    return response;
}

static cJSON *AIA_AlexaSmartHomeBrightnessControllerSetBrightness(ais_handle_t *handle,
                                                                  cJSON *payload,
                                                                  aia_smart_home_response_payload_t *response,
                                                                  ais_avs_smart_home_endpoint_t *endpoint)
{
    cJSON *context, *brightnessValue;

    /* This function is responsible for performing the turn on function.
    ** Please note, this is still in MQTT Callback context.
    */

    context = cJSON_CreateArray();

    if (payload != NULL)
    {
        brightnessValue = cJSON_GetObjectItemCaseSensitive(payload, "brightness");

        xSemaphoreTake(handle->smart_home.s_smartHomeLock, portMAX_DELAY);
        endpoint->brightnessController->brightness = brightnessValue->valueint;
        xSemaphoreGive(handle->smart_home.s_smartHomeLock);
    }

    if (handle->smart_home.p_smartHomeQueue != NULL)
    {
        aia_smart_home_queue_payload_t *smartHomePayload =
            (aia_smart_home_queue_payload_t *)pvPortMalloc(sizeof(aia_smart_home_queue_payload_t));

        smartHomePayload->response                  = response;
        smartHomePayload->controller_type           = AIS_SMART_HOME_BRIGHTNESS_CONTROLLER;
        smartHomePayload->controller_directive_type = AIS_SMART_HOME_BRIGHTNESS_CONTROLLER_SET;
        smartHomePayload->endpoint                  = endpoint;

        /* Sending the address of the pointer to reduce memory on the queue */
        if (errQUEUE_FULL == xQueueSend(handle->smart_home.p_smartHomeQueue, &smartHomePayload, (TickType_t)0))
        {
            vPortFree(smartHomePayload);
            /* Send error response */
        }
    }

    cJSON_AddItemToArray(context, AIA_AlexaSmartHomeBrightnessControllerCreateResponseContext(handle, endpoint));

    return context;
}

static cJSON *AIA_AlexaSmartHomeBrightnessControllerAdjustBrightness(ais_handle_t *handle,
                                                                     cJSON *payload,
                                                                     aia_smart_home_response_payload_t *response,
                                                                     ais_avs_smart_home_endpoint_t *endpoint)
{
    cJSON *context, *brightnessValueDelta;

    /* This function is responsible for performing the turn on function.
    ** Please note, this is still in MQTT Callback context.
    */

    context = cJSON_CreateArray();

    brightnessValueDelta = cJSON_GetObjectItemCaseSensitive(payload, "brightnessDelta");
    xSemaphoreTake(handle->smart_home.s_smartHomeLock, portMAX_DELAY);
    endpoint->brightnessController->brightnessDelta = brightnessValueDelta->valueint;
    xSemaphoreGive(handle->smart_home.s_smartHomeLock);

    if (handle->smart_home.p_smartHomeQueue != NULL)
    {
        aia_smart_home_queue_payload_t *smartHomePayload =
            (aia_smart_home_queue_payload_t *)pvPortMalloc(sizeof(aia_smart_home_queue_payload_t));

        smartHomePayload->response                  = response;
        smartHomePayload->controller_type           = AIS_SMART_HOME_BRIGHTNESS_CONTROLLER;
        smartHomePayload->controller_directive_type = AIS_SMART_HOME_BRIGHTNESS_CONTROLLER_ADJUST;
        smartHomePayload->endpoint                  = endpoint;

        /* Sending the address of the pointer to reduce memory on the queue */
        if (errQUEUE_FULL == xQueueSend(handle->smart_home.p_smartHomeQueue, &smartHomePayload, (TickType_t)0))
        {
            vPortFree(smartHomePayload);
            /* Send error response */
        }
    }

    cJSON_AddItemToArray(context, AIA_AlexaSmartHomeBrightnessControllerCreateResponseContext(handle, endpoint));

    return context;
}
