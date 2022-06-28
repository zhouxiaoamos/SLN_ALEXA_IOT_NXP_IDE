/*
 * Copyright 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdio.h>

#include "aisv2.h"
#include "aisv2_app.h"
#include "discovery_endpoint_gen.h"
#include "device_utils.h"

#include "mbedtls/base64.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define AIA_SMARTHOME_TASK_NAME     "Smart_Home_Task"
#define AIA_SMARTHOME_TASK_STACK    1024U
#define AIA_SMARTHOME_TASK_PRIORITY (configTIMER_TASK_PRIORITY - 3)

/*******************************************************************************
 * Variables
 ******************************************************************************/
TaskHandle_t smartHomeTaskHandle = NULL;
__attribute__((section(".ocram_non_cacheable_bss")))
StackType_t aia_smarthome_task_stack_buffer[AIA_SMARTHOME_TASK_STACK];
__attribute__((section(".ocram_non_cacheable_bss"))) StaticTask_t aia_smarthome_task_buffer;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void AIA_SmartHomeTask(void *arg);
status_t _AIA_SmartHomeHandleReportState(ais_handle_t *handle,
                                         aia_smart_home_response_payload_t *stateResponse,
                                         cJSON *payload);

/*******************************************************************************
 * Code
 *******************************************************************************/

status_t AIA_AlexaSmartHomeInit(ais_handle_t *handle)
{
    cJSON *discoveryJson, *event, *payload, *endpoint_array, *endpoint, *interface, *header, *capabilities_array,
        *capability;
    uint32_t endpoint_count = 0;
    char *serialNumber      = NULL;

#if USE_BASE64_UNIQUE_ID
    APP_GetUniqueID(&serialNumber, true);
#else
    APP_GetHexUniqueID(&serialNumber);
#endif /* USE_BASE64_UNIQUE_ID */

    status_t status = kStatus_Success;

    discoveryJson = AIA_DiscoveryEndpointGetAddOrUpdateReportJson(handle);

    event          = cJSON_GetObjectItemCaseSensitive(discoveryJson, "event");
    header         = cJSON_GetObjectItemCaseSensitive(event, "header");
    payload        = cJSON_GetObjectItemCaseSensitive(event, "payload");
    endpoint_array = cJSON_GetObjectItemCaseSensitive(payload, "endpoints");

    handle->smart_home.number_of_endpoints = cJSON_GetArraySize(endpoint_array);

    handle->smart_home.smart_home_endpoint = (ais_avs_smart_home_endpoint_t *)pvPortMalloc(
        handle->smart_home.number_of_endpoints * sizeof(ais_avs_smart_home_endpoint_t));

    memset(handle->smart_home.smart_home_endpoint, 0x0,
           handle->smart_home.number_of_endpoints * sizeof(ais_avs_smart_home_endpoint_t));

    /* Create the Smart Home queue which consists of a size of the pointer to save memory */
    handle->smart_home.p_smartHomeQueue =
        xQueueCreate(AIS_APP_SMART_HOME_QUEUE_COUNT, sizeof(aia_smart_home_queue_payload_t *));

    handle->smart_home.s_smartHomeLock = xSemaphoreCreateMutex();

    cJSON_ArrayForEach(endpoint, endpoint_array)
    {
        /* Declared locally as it needs resetting, size of four includes all colons */
        uint32_t endpointIdSize = 4;

        endpoint_array     = cJSON_GetObjectItemCaseSensitive(payload, "endpoints");
        capabilities_array = cJSON_GetObjectItemCaseSensitive(endpoint, "capabilities");

        /* Generate the Endpoint Id */
        endpointIdSize += strnlen(AIA_CLIENT_ID, 256);
        endpointIdSize += strnlen(clientcredentialIOT_PRODUCT_NAME, 256);
        endpointIdSize += strnlen(serialNumber, 256);

        if (endpoint_count != 0)
        {
            endpointIdSize += 1;
        }

        handle->smart_home.smart_home_endpoint[endpoint_count].endpointId = (char *)pvPortMalloc(endpointIdSize + 1);

        if (handle->smart_home.smart_home_endpoint != NULL)
        {
            memset(handle->smart_home.smart_home_endpoint[endpoint_count].endpointId, 0, endpointIdSize + 1);
            snprintf(handle->smart_home.smart_home_endpoint[endpoint_count].endpointId, endpointIdSize + 1,
                     "%s::%s::%s", AIA_CLIENT_ID, clientcredentialIOT_PRODUCT_NAME, serialNumber);

            if (endpoint_count != 0)
            {
                snprintf(handle->smart_home.smart_home_endpoint[endpoint_count].endpointId + (endpointIdSize - 1), 2,
                         "%i", (int)endpoint_count);
            }
        }

        cJSON_ArrayForEach(capability, capabilities_array)
        {
            interface = cJSON_GetObjectItemCaseSensitive(capability, "interface");

            if (0 == strcmp(interface->valuestring, AIA_SMART_HOME_NAMESPACE_ALEXA_MODE_CONTROLLER))
            {
                handle->smart_home.smart_home_endpoint[endpoint_count].modeController =
                    (ais_avs_mode_controller_t *)pvPortMalloc(sizeof(ais_avs_mode_controller_t));

                memset(handle->smart_home.smart_home_endpoint[endpoint_count].modeController, 0x0,
                       sizeof(ais_avs_mode_controller_t));
            }
            else if (0 == strcmp(interface->valuestring, AIA_SMART_HOME_NAMESPACE_ALEXA_TOGGLE_CONTROLLER))
            {
                handle->smart_home.smart_home_endpoint[endpoint_count].toggleController =
                    (ais_avs_toggle_controller_t *)pvPortMalloc(sizeof(ais_avs_toggle_controller_t));

                memset(handle->smart_home.smart_home_endpoint[endpoint_count].toggleController, 0x0,
                       sizeof(ais_avs_toggle_controller_t));
            }
            else if (0 == strcmp(interface->valuestring, AIA_SMART_HOME_NAMESPACE_ALEXA_POWER_CONTROLLER))
            {
                handle->smart_home.smart_home_endpoint[endpoint_count].powerController =
                    (ais_avs_power_controller_t *)pvPortMalloc(sizeof(ais_avs_power_controller_t));

                memset(handle->smart_home.smart_home_endpoint[endpoint_count].powerController, 0x0,
                       sizeof(ais_avs_power_controller_t));
            }
            else if (0 == strcmp(interface->valuestring, AIA_SMART_HOME_NAMESPACE_ALEXA_RANGE_CONTROLLER))
            {
                handle->smart_home.smart_home_endpoint[endpoint_count].rangeController =
                    (ais_avs_range_controller_t *)pvPortMalloc(sizeof(ais_avs_range_controller_t));

                memset(handle->smart_home.smart_home_endpoint[endpoint_count].rangeController, 0x0,
                       sizeof(ais_avs_range_controller_t));
            }
            else if (0 == strcmp(interface->valuestring, AIA_SMART_HOME_NAMESPACE_ALEXA_BRIGHTNESS_CONTROLLER))
            {
                handle->smart_home.smart_home_endpoint[endpoint_count].brightnessController =
                    (ais_avs_brightness_controller_t *)pvPortMalloc(sizeof(ais_avs_brightness_controller_t));

                memset(handle->smart_home.smart_home_endpoint[endpoint_count].brightnessController, 0x0,
                       sizeof(ais_avs_brightness_controller_t));
            }
            else if (0 != strcmp(interface->valuestring, AIA_SMART_HOME_NAMESPACE_ALEXA))
            {
                configPRINTF(("[SmartHome] - Unknown capability, check JSON string\r\n"));
            }
        }
        endpoint_count++;
    }

    if (smartHomeTaskHandle == NULL)
    {
        smartHomeTaskHandle =
            xTaskCreateStatic(AIA_SmartHomeTask, AIA_SMARTHOME_TASK_NAME, AIA_SMARTHOME_TASK_STACK, (void *)handle,
                              AIA_SMARTHOME_TASK_PRIORITY, aia_smarthome_task_stack_buffer, &aia_smarthome_task_buffer);
    }

    vPortFree(serialNumber);
    cJSON_Delete(discoveryJson);

    return status;
}

status_t AIA_AlexaSmartHomeHandler(ais_handle_t *handle, cJSON *payload)
{
    cJSON *header, *name, *directive, *messageId, *correlationToken, *endpoint, *endpointId;
    aia_smart_home_response_payload_t *response;
    status_t status = kStatus_Fail;

    directive        = cJSON_GetObjectItemCaseSensitive(payload, "directive");
    header           = cJSON_GetObjectItemCaseSensitive(directive, "header");
    name             = cJSON_GetObjectItemCaseSensitive(header, "name");
    messageId        = cJSON_GetObjectItemCaseSensitive(header, "messageId");
    correlationToken = cJSON_GetObjectItemCaseSensitive(header, "correlationToken");
    endpoint         = cJSON_GetObjectItemCaseSensitive(payload, "endpoint");
    endpointId       = cJSON_GetObjectItemCaseSensitive(endpoint, "endpointId");

    /* Possibly add additional JSON attributes for AIA specific requirements */

    if (strcmp(name->valuestring, "EventProcessed") == 0)
    {
        /* Set EventProcess Group Bit to notify AddOrUpdate/Delete report has been processed */
        status = kStatus_Success;
    }

    if (strcmp(name->valuestring, "ReportState") == 0)
    {
        response = (aia_smart_home_response_payload_t *)pvPortMalloc(sizeof(aia_smart_home_response_payload_t));

        if (response != NULL)
        {
            response->correlationToken = correlationToken->valuestring;
            response->endpointId       = endpointId->valuestring;

            status = _AIA_SmartHomeHandleReportState(handle, response, payload);

            vPortFree(response);
        }
    }

    return status;
}

cJSON *AIA_DiscoveryEndpointGetAddOrUpdateReportJson(ais_handle_t *handle)
{
    uint8_t messageIdString[17];

    cJSON *discoveryJson, *event, *payload, *endpoint_array, *endpoint, *header, *messageId, *eventCorrelationToken;
    uint32_t endpoint_count = 0;
    char *serialNumber      = NULL;

    discoveryJson = cJSON_Parse(avs_smart_home_json_string);

#if USE_BASE64_UNIQUE_ID
    APP_GetUniqueID(&serialNumber, true);
#else
    APP_GetHexUniqueID(&serialNumber);
#endif /* USE_BASE64_UNIQUE_ID */

    event          = cJSON_GetObjectItemCaseSensitive(discoveryJson, "event");
    header         = cJSON_GetObjectItemCaseSensitive(event, "header");
    payload        = cJSON_GetObjectItemCaseSensitive(event, "payload");
    endpoint_array = cJSON_GetObjectItemCaseSensitive(payload, "endpoints");

    messageId = cJSON_DetachItemFromObjectCaseSensitive(header, "messageId");
    cJSON_Delete(messageId);

    AIA_build_uuid(handle, messageIdString);
    cJSON_AddItemToObject(header, "messageId", cJSON_CreateString((char *)messageIdString));

    eventCorrelationToken = cJSON_DetachItemFromObjectCaseSensitive(header, "eventCorrelationToken");
    cJSON_Delete(eventCorrelationToken);

    AIA_build_uuid(handle, messageIdString);
    cJSON_AddItemToObject(header, "eventCorrelationToken", cJSON_CreateString((char *)messageIdString));

    cJSON_ArrayForEach(endpoint, endpoint_array)
    {
        char *endpointIdString = NULL;

        cJSON *endpointId, *registration, *productId, *deviceSerialNumber;

        registration = cJSON_GetObjectItemCaseSensitive(endpoint, "registration");

        if (registration == NULL)
        {
            registration = cJSON_CreateObject();
        }

        uint32_t endpointIdSize = 4; /* Counting the colons */
        endpointIdSize += strnlen(AIA_CLIENT_ID, 256);
        endpointIdSize += strnlen(clientcredentialIOT_PRODUCT_NAME, 256);
        endpointIdSize += strnlen(serialNumber, 256);

        if (endpoint_count != 0)
        {
            endpointIdSize += 1;
        }

        endpointIdString = (char *)pvPortMalloc(endpointIdSize + 1);

        if (endpointIdString != NULL)
        {
            memset(endpointIdString, 0, endpointIdSize + 1);
            snprintf(endpointIdString, endpointIdSize + 1, "%s::%s::%s", AIA_CLIENT_ID,
                     clientcredentialIOT_PRODUCT_NAME, serialNumber);

            if (endpoint_count != 0)
            {
                snprintf(endpointIdString + (endpointIdSize - 1), 2, "%i", (int)endpoint_count);
            }

            endpointId = cJSON_DetachItemFromObjectCaseSensitive(endpoint, "endpointId");
            cJSON_Delete(endpointId);

            endpointId = cJSON_CreateString(endpointIdString);

            cJSON_AddItemToObject(endpoint, "endpointId", endpointId);
            vPortFree(endpointIdString);

            deviceSerialNumber = cJSON_DetachItemFromObjectCaseSensitive(registration, "deviceSerialNumber");
            cJSON_Delete(deviceSerialNumber);

            cJSON_AddItemToObject(registration, "deviceSerialNumber", cJSON_CreateString(serialNumber));

            productId = cJSON_DetachItemFromObjectCaseSensitive(registration, "productId");
            cJSON_Delete(productId);

            cJSON_AddItemToObject(registration, "productId", cJSON_CreateString(clientcredentialIOT_PRODUCT_NAME));

            endpoint_count++;
        }
    }

    vPortFree(serialNumber);

    /* Possibly add additional JSON attributes for AIA specific requirements */

    return discoveryJson;
}

status_t AIA_AlexaSmartHomeBuildTemplate(ais_handle_t *handle, cJSON *json, aia_smart_home_response_payload_t *response)
{
    status_t status = kStatus_Fail;
    char messageId[17];
    cJSON *event, *header, *endpoint, *payload, *jsonId, *context;

    if (json != NULL)
    {
        status = kStatus_Success;
    }

    if (status == kStatus_Success)
    {
        event    = cJSON_CreateObject();
        header   = cJSON_CreateObject();
        endpoint = cJSON_CreateObject();
        payload  = cJSON_CreateObject();
        context  = cJSON_CreateObject();

        AIA_build_uuid(handle, (uint8_t *)messageId);
        jsonId = cJSON_CreateString((char *)messageId);

        cJSON_AddItemToObject(json, "event", event);
        cJSON_AddItemToObject(event, "header", header);
        cJSON_AddItemToObject(event, "endpoint", endpoint);
        cJSON_AddItemToObject(event, "payload", payload);
        cJSON_AddItemToObject(header, "payloadVersion",
                              cJSON_CreateString((char *)AIA_SMART_HOME_ALEXA_PAYLOAD_VERSION));
        cJSON_AddItemToObject(header, "namespace", cJSON_CreateString((char *)AIA_SMART_HOME_NAMESPACE_ALEXA));
        cJSON_AddItemToObject(header, "messageId", jsonId);

        cJSON_AddItemToObject(json, "context", context);
        cJSON_AddItemToObject(context, "properties", response->context.json);
        cJSON_AddItemToObject(endpoint, "endpointId", cJSON_CreateString((char *)response->endpointId));
    }

    return status;
}
status_t AIA_AlexaSmartHomeAddOrUpdateReport(ais_handle_t *handle)
{
    status_t status = kStatus_Fail;
    ais_json_t smart_home_add_update_report;
    cJSON *event;

    /* Create template JSON message */
    smart_home_add_update_report.json = AIA_DiscoveryEndpointGetAddOrUpdateReportJson(handle);

    AIS_EventSmartHomeEndpointForwarding(handle, &smart_home_add_update_report);

    return status;
}

status_t AIA_AlexaSmartHomeStateReport(ais_handle_t *handle, aia_smart_home_response_payload_t *stateReport)
{
    status_t status = kStatus_Fail;
    ais_json_t smart_home_state_report;
    cJSON *event, *header;

    smart_home_state_report.json = cJSON_CreateObject();

    /* Create template JSON message */
    status = AIA_AlexaSmartHomeBuildTemplate(handle, smart_home_state_report.json, stateReport);

    /* Add the StateReport specific json attributes */
    event  = cJSON_GetObjectItemCaseSensitive(smart_home_state_report.json, "event");
    header = cJSON_GetObjectItemCaseSensitive(event, "header");

    cJSON_AddItemToObject(header, "name", cJSON_CreateString((char *)"StateReport"));

    /* correlationToken is not always used when sending events */
    cJSON_AddItemToObject(header, "correlationToken", cJSON_CreateString((char *)stateReport->correlationToken));

    AIS_EventSmartHomeEndpointForwarding(handle, &smart_home_state_report);

    return status;
}

status_t AIA_AlexaSmartHomeResponse(ais_handle_t *handle, aia_smart_home_response_payload_t *response)
{
    status_t status = kStatus_Fail;
    cJSON *event, *header;
    ais_json_t smart_home_response;

    smart_home_response.json = cJSON_CreateObject();

    /* Create template JSON message */
    status = AIA_AlexaSmartHomeBuildTemplate(handle, smart_home_response.json, response);

    /* Add the StateReport specific json attributes */
    event  = cJSON_GetObjectItemCaseSensitive(smart_home_response.json, "event");
    header = cJSON_GetObjectItemCaseSensitive(event, "header");

    cJSON_AddItemToObject(header, "name", cJSON_CreateString((char *)"Response"));

    /* correlationToken is not always used when sending events */
    cJSON_AddItemToObject(header, "correlationToken", cJSON_CreateString((char *)response->correlationToken));

    AIS_EventSmartHomeEndpointForwarding(handle, &smart_home_response);

    return status;
}

status_t AIA_AlexaSmartHomeChangeReport(ais_handle_t *handle, aia_smart_home_response_payload_t *change_report)
{
    status_t status = kStatus_Fail;
    cJSON *event, *header;
    ais_json_t smart_home_change_report;

    smart_home_change_report.json = cJSON_CreateObject();

    /* Create template JSON message */
    status = AIA_AlexaSmartHomeBuildTemplate(handle, smart_home_change_report.json, change_report);

    /* Add the StateReport specific json attributes */
    event  = cJSON_GetObjectItemCaseSensitive(smart_home_change_report.json, "event");
    header = cJSON_GetObjectItemCaseSensitive(event, "header");

    cJSON_AddItemToObject(header, "name", cJSON_CreateString((char *)"ChangeReport"));

    AIS_EventSmartHomeEndpointForwarding(handle, &smart_home_change_report);

    return status;
}

status_t AIA_AlexaSmartHomeDeferredResponse(ais_handle_t *handle, aia_smart_home_response_payload_t *deferredResponse)
{
    status_t status = kStatus_Fail;
    cJSON *event, *header;
    ais_json_t smart_home_deferred_response;

    smart_home_deferred_response.json = cJSON_CreateObject();

    /* Create template JSON message */
    status = AIA_AlexaSmartHomeBuildTemplate(handle, smart_home_deferred_response.json, deferredResponse);

    /* Add the StateReport specific json attributes */
    event  = cJSON_GetObjectItemCaseSensitive(smart_home_deferred_response.json, "event");
    header = cJSON_GetObjectItemCaseSensitive(event, "header");

    cJSON_AddItemToObject(header, "name", cJSON_CreateString((char *)"StateReport"));

    /* correlationToken is not always used when sending events */
    cJSON_AddItemToObject(header, "correlationToken", cJSON_CreateString((char *)deferredResponse->correlationToken));

    AIS_EventSmartHomeEndpointForwarding(handle, &smart_home_deferred_response);

    return status;
}

status_t AIA_AlexaSmartHomeErrorResponse(ais_handle_t *handle, aia_smart_home_response_payload_t *errorResponse)
{
    status_t status = kStatus_Fail;
    cJSON *event, *header;
    ais_json_t smart_home_error_reponse;

    smart_home_error_reponse.json = cJSON_CreateObject();

    /* Create template JSON message */
    status = AIA_AlexaSmartHomeBuildTemplate(handle, smart_home_error_reponse.json, errorResponse);

    /* Add the StateReport specific json attributes */
    event  = cJSON_GetObjectItemCaseSensitive(smart_home_error_reponse.json, "event");
    header = cJSON_GetObjectItemCaseSensitive(event, "header");

    cJSON_AddItemToObject(header, "name", cJSON_CreateString((char *)"ErrorResponse"));

    /* correlationToken is not always used when sending events */
    cJSON_AddItemToObject(header, "correlationToken", cJSON_CreateString((char *)errorResponse->correlationToken));

    AIS_EventSmartHomeEndpointForwarding(handle, &smart_home_error_reponse);

    return status;
}

ais_avs_smart_home_endpoint_t *AIA_SmartHomeGetEndpoint(ais_handle_t *handle, char *endpointId)
{
    ais_avs_smart_home_endpoint_t *endpoint = NULL;
    uint8_t endpoint_count                  = 0;

    for (endpoint_count = 0; endpoint_count < handle->smart_home.number_of_endpoints; endpoint_count++)
    {
        if (0 == strcmp(handle->smart_home.smart_home_endpoint[endpoint_count].endpointId, endpointId))
        {
            endpoint = &handle->smart_home.smart_home_endpoint[endpoint_count];
            break;
        }
    }
    return endpoint;
}

status_t _AIA_SmartHomeHandleReportState(ais_handle_t *handle,
                                         aia_smart_home_response_payload_t *stateResponse,
                                         cJSON *payload)
{
    cJSON *directive, *endpoint, *endpointId, *properties, *event, *header;
    status_t status                    = kStatus_Fail;
    uint32_t endpoint_count            = 0;
    ais_json_t smart_home_report_state = {0};
    ais_avs_smart_home_endpoint_t *sh_endpoint;

    properties = cJSON_CreateArray();

    smart_home_report_state.json = cJSON_CreateObject();

    directive  = cJSON_GetObjectItemCaseSensitive(payload, "directive");
    endpoint   = cJSON_GetObjectItemCaseSensitive(directive, "endpoint");
    endpointId = cJSON_GetObjectItemCaseSensitive(endpoint, "endpointId");

    stateResponse->endpointId = endpointId->valuestring;

    sh_endpoint = AIA_SmartHomeGetEndpoint(handle, stateResponse->endpointId);

    if (sh_endpoint != NULL)
    {
        if (handle->smart_home.smart_home_endpoint[endpoint_count].brightnessController != NULL)
        {
            cJSON_AddItemToArray(properties,
                                 AIA_AlexaSmartHomeBrightnessControllerCreateResponseContext(handle, sh_endpoint));
        }
        if (handle->smart_home.smart_home_endpoint[endpoint_count].powerController != NULL)
        {
            cJSON_AddItemToArray(properties,
                                 AIA_AlexaSmartHomePowerControllerCreateResponseContext(handle, sh_endpoint));
        }
        if (handle->smart_home.smart_home_endpoint[endpoint_count].modeController != NULL)
        {
            cJSON_AddItemToArray(properties,
                                 AIA_AlexaSmartHomeModeControllerCreateResponseContext(handle, sh_endpoint));
        }
        if (handle->smart_home.smart_home_endpoint[endpoint_count].rangeController != NULL)
        {
            cJSON_AddItemToArray(properties,
                                 AIA_AlexaSmartHomeRangeControllerCreateResponseContext(handle, sh_endpoint));
        }
        if (handle->smart_home.smart_home_endpoint[endpoint_count].toggleController != NULL)
        {
            cJSON_AddItemToArray(properties,
                                 AIA_AlexaSmartHomeToggleControllerCreateResponseContext(handle, sh_endpoint));
        }
    }
    else
    {
        configPRINTF(("[SmartHome] - Unable to find associated endpoint %s\t\n", endpointId->valuestring));
    }

    stateResponse->context.json = properties;

    /* Create template JSON message */
    status = AIA_AlexaSmartHomeBuildTemplate(handle, smart_home_report_state.json, stateResponse);

    event  = cJSON_GetObjectItemCaseSensitive(smart_home_report_state.json, "event");
    header = cJSON_GetObjectItemCaseSensitive(event, "header");

    cJSON_AddItemToObject(header, "name", cJSON_CreateString((char *)"StateReport"));

    /* correlationToken is not always used when sending events */
    cJSON_AddItemToObject(header, "correlationToken", cJSON_CreateString((char *)stateResponse->correlationToken));

    AIS_EventSmartHomeEndpointForwarding(handle, &smart_home_report_state);

    return status;
}

/*!
 * @brief AIA_SmartHomeTask to receive all smart home events out of IoT Thread context
 *
 * Tasks that obtains pointers to events and calls the application callback
 *
 * @param *arg Reference to current ais_handle_t in use
 *
 * @return Success or failure
 */
static void AIA_SmartHomeTask(void *arg)
{
    aia_smart_home_queue_payload_t *smart_home_msg = NULL;
    ais_handle_t *handle                           = (ais_handle_t *)arg;

    while (1)
    {
        /* Passing the address of the pointer so the memory address will be copied into the address of the stack pointer
         * to save memory */
        if (xQueueReceive(handle->smart_home.p_smartHomeQueue, &smart_home_msg, portMAX_DELAY))
        {
            if (smart_home_msg != NULL)
            {
                switch (smart_home_msg->controller_type)
                {
                    case AIS_SMART_HOME_POWER_CONTROLLER:
                        AIS_AppCallback_PowerController(handle, smart_home_msg);
                        break;
                    case AIS_SMART_HOME_TOGGLE_CONTROLLER:
                        AIS_AppCallback_ToggleController(handle, smart_home_msg);
                        break;
                    case AIS_SMART_HOME_RANGE_CONTROLLER:
                        AIS_AppCallback_RangeController(handle, smart_home_msg);
                        break;
                    case AIS_SMART_HOME_MODE_CONTROLLER:
                        AIS_AppCallback_ModeController(handle, smart_home_msg);
                        break;
                    case AIS_SMART_HOME_BRIGHTNESS_CONTROLLER:
                        AIS_AppCallback_BrightnessController(handle, smart_home_msg);
                        break;
                    default:
                        configPRINTF(("[Smart Home Task] Unknown Controller Received\r\n"));
                }

                vPortFree(smart_home_msg);
            }
            else
            {
                configPRINTF(("[Smart Home Task] NULL pointer received\r\n"));
            }
        }
    }
}
