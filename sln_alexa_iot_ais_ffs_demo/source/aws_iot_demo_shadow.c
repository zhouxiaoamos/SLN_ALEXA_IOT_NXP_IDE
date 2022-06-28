/*
 * Amazon FreeRTOS V201908.00
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/*
 * Copyright 2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/* The config header is always included first. */
#include "iot_config.h"

/* Standard includes. */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Set up logging for this demo. */
#include "iot_demo_logging.h"

/* Platform layer includes. */
#include "platform/iot_clock.h"
#include "platform/iot_threads.h"

/* MQTT include. */
#include "iot_mqtt.h"

/* Shadow include. */
#include "aws_iot_shadow.h"
#include "app_aws_shadow.h"

/* JSON utilities include. */
#include "iot_json_utils.h"

/* Mqtt connection wrapper */
#include "iot_mqtt_agent.h"
#include "mqtt_connection.h"

#include "fsl_common.h"
#include "aws_iot_demo_shadow.h"

#ifdef LIBRARY_LOG_NAME
#undef LIBRARY_LOG_NAME
#endif /* LIBRARY_LOG_NAME */
#define LIBRARY_LOG_NAME ("SLN SHADOW")

/* Set to 1 to enable the Shadow Demo logs */
#define SHADOW_DEMO_LOGS 0

#if SHADOW_DEMO_LOGS
#define ShadowLogError(...) IotLogError(__VA_ARGS__)
#define ShadowLogWarn(...)  IotLogWarn(__VA_ARGS__)
#define ShadowLogInfo(...)  IotLogInfo(__VA_ARGS__)
#define ShadowLogDebug(...) IotLogDebug(__VA_ARGS__)
#else
#define ShadowLogError(...) IotLogError(__VA_ARGS__)
#define ShadowLogWarn(...)
#define ShadowLogInfo(...)
#define ShadowLogDebug(...)
#endif /* SHADOW_DEMO_LOGS */

/**
 * @brief The timeout for Shadow and MQTT operations in this demo.
 */
#define TIMEOUT_MS (5000)

/**
 * @brief Set to 1 to clear the Shadow document at each boot so that application starts with no existing Shadow
 */
#define SHADOW_BOOT_RESET 1

/**
 * @brief Format string representing a Shadow document with a "reported" state.
 *
 * Note the client token, which is required for all Shadow updates. The client
 * token must be unique at any given time, but may be reused once the update is
 * completed. For this demo, a timestamp is used for a client token.
 *
 * This is the structure of a reported JSON document
 * "{"
 * "\"state\":{"
 * "\"reported\":{"
 * "\"powerOn\":%.*s,"
 * "\"brightness\":%.*s"
 * "}"
 * "},"
 * "\"clientToken\":\"%08lu\""
 * "}"
 *
 * This is an example of a reported JSON document
 * {
 *   "state":
 *   {
 *     "reported":
 *     {
 *       "powerOn": 1,
 *       "brightness": "MEDIUM"
 *     }
 *   },
 *   "clientToken":"12345678"
 * }
 */

#define SHADOW_REPORTED_JSON_PREF \
    "{"                           \
    "\"state\":{"                 \
    "\"reported\":{"

#define SHADOW_REPORTED_JSON_SUFF \
    "}"                           \
    "},"                          \
    "\"clientToken\":\"%08lu\""   \
    "}"

#define SHADOW_REPORTED_JSON_POWERON "\"" SHADOW_POWERON_FIELD "\":%.*s"

#define SHADOW_REPORTED_JSON_BRIGHTNESS "\"" SHADOW_BRIGHTNESS_FIELD "\":%.*s"

#define SHADOW_REPORTED_JSON_LEN                                                                                    \
    (sizeof(SHADOW_REPORTED_JSON_PREF) + sizeof(SHADOW_REPORTED_JSON_SUFF) + sizeof(SHADOW_REPORTED_JSON_POWERON) + \
     sizeof(SHADOW_REPORTED_JSON_BRIGHTNESS) + 20)

/*-----------------------------------------------------------*/

/* Retrieves the MQTT v2 connection from the MQTT v1 connection handle. */
extern IotMqttConnection_t MQTT_AGENT_Getv2Connection(MQTTAgentHandle_t xMQTTHandle);

/*-----------------------------------------------------------*/

/* The packet structure to be passed to the task which will call SendUpdateShadowDemo.
 *
 * mqttConnection information about the connection
 * updateDocument the document to be sent as the update
 */
typedef struct _IotShadowUpadte
{
    IotMqttConnection_t mqttConnection;
    AwsIotShadowDocumentInfo_t *updateDocument;
} IotShadowUpadte_t;

/* Shadow Configurations */
static char *s_thingName            = NULL;
static uint8_t s_thingNameLen       = 0;
static shadowUpdateCb s_appCallback = NULL;

/*-----------------------------------------------------------*/

/**
 * @brief Parses a key in the "state" section of a Shadow delta document.
 *
 * @param[in] pDeltaDocument The Shadow delta document to parse.
 * @param[in] deltaDocumentLength The length of `pDeltaDocument`.
 * @param[in] pDeltaKey The key in the delta document to find. Must be NULL-terminated.
 * @param[out] pDelta Set to the first character in the delta key.
 * @param[out] pDeltaLength The length of the delta key.
 *
 * @return `true` if the given delta key is found; `false` otherwise.
 */
static bool _getDelta(const char *pDeltaDocument,
                      size_t deltaDocumentLength,
                      const char *pDeltaKey,
                      const char **pDelta,
                      size_t *pDeltaLength)
{
    bool stateFound = false, deltaFound = false;
    const size_t deltaKeyLength = strlen(pDeltaKey);
    const char *pState          = NULL;
    size_t stateLength          = 0;

    /* Find the "state" key in the delta document. */
    stateFound = IotJsonUtils_FindJsonValue(pDeltaDocument, deltaDocumentLength, "state", 5, &pState, &stateLength);

    if (stateFound == true)
    {
        /* Find the delta key within the "state" section. */
        deltaFound = IotJsonUtils_FindJsonValue(pState, stateLength, pDeltaKey, deltaKeyLength, pDelta, pDeltaLength);
    }
    else
    {
        ShadowLogWarn("Failed to find \"state\" in Shadow delta document.");
    }

    return deltaFound;
}

/*-----------------------------------------------------------*/

/**
 * @brief Parses the "state" key from the "previous" or "current" sections of a
 * Shadow updated document.
 *
 * @param[in] pUpdatedDocument The Shadow updated document to parse.
 * @param[in] updatedDocumentLength The length of `pUpdatedDocument`.
 * @param[in] pSectionKey Either "previous" or "current". Must be NULL-terminated.
 * @param[out] pState Set to the first character in "state".
 * @param[out] pStateLength Length of the "state" section.
 *
 * @return `true` if the "state" was found; `false` otherwise.
 */
static bool _getUpdatedState(const char *pUpdatedDocument,
                             size_t updatedDocumentLength,
                             const char *pSectionKey,
                             const char **pState,
                             size_t *pStateLength)
{
    bool sectionFound = false, stateFound = false;
    const size_t sectionKeyLength = strlen(pSectionKey);
    const char *pSection          = NULL;
    size_t sectionLength          = 0;

    /* Find the given section in the updated document. */
    sectionFound = IotJsonUtils_FindJsonValue(pUpdatedDocument, updatedDocumentLength, pSectionKey, sectionKeyLength,
                                              &pSection, &sectionLength);

    if (sectionFound == true)
    {
        /* Find the "state" key within the "previous" or "current" section. */
        stateFound = IotJsonUtils_FindJsonValue(pSection, sectionLength, "state", 5, pState, pStateLength);
    }
    else
    {
        ShadowLogWarn("Failed to find section %s in Shadow updated document.", pSectionKey);
    }

    return stateFound;
}

/*-----------------------------------------------------------*/

/**
 * @brief Free the IotShadowUpadte_t structure fields.
 *
 * @param[in] shadowUpdate - document to be freed
 */
static void _removeResponseUpdateDocument(IotShadowUpadte_t *shadowUpdate)
{
    if (shadowUpdate != NULL)
    {
        if (shadowUpdate->updateDocument != NULL)
        {
            vPortFree((void *)shadowUpdate->updateDocument->pThingName);
            vPortFree((void *)shadowUpdate->updateDocument->u.update.pUpdateDocument);
        }
        vPortFree((void *)shadowUpdate->updateDocument);

        shadowUpdate->updateDocument = NULL;
        shadowUpdate->mqttConnection = NULL;
    }

    vPortFree(shadowUpdate);
}

/**
 * @brief Generate an update JSON document to be sent as an update to the Shadow.
 *
 * @param[in] pCallbackParam Contains the information about the shadow and the connection.
 * @param[in] powerOnVal Current value of the powerOn.
 * @param[in] powerOnLen The length of the current value of the powerOn.
 * @param[in] brightnessVal Current value of the brightness.
 * @param[in] brightnessLen The length of the current value of the brightness.
 *
 * @return The pointer to the generated packet to be sent. NULL in case of an error.
 */
static IotShadowUpadte_t *_createResponseUpdateDocument(AwsIotShadowCallbackParam_t *pCallbackParam,
                                                        const char *powerOnVal,
                                                        size_t powerOnLen,
                                                        const char *brightnessVal,
                                                        size_t brightnessLen)
{
    status_t status                            = kStatus_Success;
    IotShadowUpadte_t *shadowUpdate            = NULL;
    AwsIotShadowDocumentInfo_t *updateDocument = NULL;
    uint32_t updateDocumentPotentialLen        = 0;
    uint32_t updateDocumentActualLen           = 0;
    uint32_t updateDocumentMaxLen              = 0;
    long unsigned int clientToken              = 0;

    shadowUpdate = (IotShadowUpadte_t *)pvPortMalloc(sizeof(IotShadowUpadte_t));
    if (shadowUpdate != NULL)
    {
        memset(shadowUpdate, 0, sizeof(IotShadowUpadte_t));
    }
    else
    {
        ShadowLogError("Could not allocate memory for shadowUpdate.");
        status = kStatus_Fail;
    }

    /* Allocate Memory for the Update Document Structure */
    if (status == kStatus_Success)
    {
        if (updateDocument == NULL)
        {
            updateDocument = (AwsIotShadowDocumentInfo_t *)pvPortMalloc(sizeof(AwsIotShadowDocumentInfo_t));
            if (updateDocument != NULL)
            {
                memset((uint8_t *)updateDocument, 0, sizeof(AwsIotShadowDocumentInfo_t));
            }
            else
            {
                ShadowLogError("Could not allocate memory for updateDocument.");
                status = kStatus_Fail;
            }
        }
        else
        {
            ShadowLogError("Can't send an Update Document, another update in progress.");
            status = kStatus_Fail;
        }
    }

    /* Allocate Memory for the Thing Name field */
    if (status == kStatus_Success)
    {
        updateDocument->pThingName = pvPortMalloc(pCallbackParam->thingNameLength);
        if (updateDocument->pThingName != NULL)
        {
            memcpy((char *)updateDocument->pThingName, (char *)pCallbackParam->pThingName,
                   pCallbackParam->thingNameLength);
            updateDocument->thingNameLength = pCallbackParam->thingNameLength;
        }
        else
        {
            ShadowLogError("Could not allocate memory for updateDocument->pThingName.");
            status = kStatus_Fail;
        }
    }

    /* Allocate Memory for the Update Document itself */
    if (status == kStatus_Success)
    {
        updateDocumentMaxLen                     = SHADOW_REPORTED_JSON_LEN + powerOnLen + brightnessLen + 1;
        updateDocument->u.update.pUpdateDocument = pvPortMalloc(updateDocumentMaxLen);
        if (updateDocument->u.update.pUpdateDocument != NULL)
        {
            memset((char *)updateDocument->u.update.pUpdateDocument, 0, updateDocumentMaxLen);

            clientToken = (long unsigned int)IotClock_GetTimeMs();

            updateDocumentPotentialLen +=
                snprintf((char *)updateDocument->u.update.pUpdateDocument + updateDocumentActualLen,
                         (updateDocumentMaxLen - updateDocumentActualLen), SHADOW_REPORTED_JSON_PREF);
            updateDocumentActualLen = strlen(updateDocument->u.update.pUpdateDocument);

            if ((powerOnLen > 0) && (powerOnVal != NULL))
            {
                updateDocumentPotentialLen +=
                    snprintf((char *)updateDocument->u.update.pUpdateDocument + updateDocumentActualLen,
                             (updateDocumentMaxLen - updateDocumentActualLen), SHADOW_REPORTED_JSON_POWERON, powerOnLen,
                             powerOnVal);
                updateDocumentActualLen = strlen(updateDocument->u.update.pUpdateDocument);
            }

            if ((powerOnLen > 0) && (powerOnVal != NULL) && (brightnessLen > 0) && (brightnessVal != NULL))
            {
                updateDocumentPotentialLen +=
                    snprintf((char *)updateDocument->u.update.pUpdateDocument + updateDocumentActualLen,
                             (updateDocumentMaxLen - updateDocumentActualLen), ",");
                updateDocumentActualLen = strlen(updateDocument->u.update.pUpdateDocument);
            }

            if ((brightnessLen > 0) && (brightnessVal != NULL))
            {
                updateDocumentPotentialLen +=
                    snprintf((char *)updateDocument->u.update.pUpdateDocument + updateDocumentActualLen,
                             (updateDocumentMaxLen - updateDocumentActualLen), SHADOW_REPORTED_JSON_BRIGHTNESS,
                             brightnessLen, brightnessVal);
                updateDocumentActualLen = strlen(updateDocument->u.update.pUpdateDocument);
            }

            updateDocumentPotentialLen +=
                snprintf((char *)updateDocument->u.update.pUpdateDocument + updateDocumentActualLen,
                         (updateDocumentMaxLen - updateDocumentActualLen), SHADOW_REPORTED_JSON_SUFF, clientToken);
            updateDocumentActualLen = strlen(updateDocument->u.update.pUpdateDocument);

            if ((updateDocumentActualLen == updateDocumentPotentialLen) &&
                (updateDocumentActualLen < updateDocumentMaxLen))
            {
                updateDocument->u.update.updateDocumentLength = updateDocumentActualLen;
            }
            else
            {
                ShadowLogError("Update Document is too long: allocated: %d, written: %d, required: %d.",
                               updateDocumentMaxLen, updateDocumentActualLen, updateDocumentPotentialLen);
                status = kStatus_Fail;
            }
        }
        else
        {
            ShadowLogError("Could not allocate memory for updateDocument->u.update.pUpdateDocument.");
            status = kStatus_Fail;
        }
    }

    if (status == kStatus_Success)
    {
        shadowUpdate->updateDocument = updateDocument;
        shadowUpdate->mqttConnection = pCallbackParam->mqttConnection;
    }
    else
    {
        if (shadowUpdate != NULL)
        {
            shadowUpdate->updateDocument = updateDocument;
            shadowUpdate->mqttConnection = pCallbackParam->mqttConnection;
        }
        _removeResponseUpdateDocument(shadowUpdate);
        shadowUpdate = NULL;
    }

    return shadowUpdate;
}

/**
 * @brief Shadow delta callback, invoked when the desired and updates Shadow
 * states differ.
 *
 * This function simulates a device updating its state in response to a Shadow.
 *
 * @param[in] pCallbackContext Pointer to the function to be called in order to update the reported state.
 * @param[in] pCallbackParam The received Shadow delta document.
 */
static void _shadowDeltaCallback(void *pCallbackContext, AwsIotShadowCallbackParam_t *pCallbackParam)
{
    status_t status                 = kStatus_Success;
    IotShadowUpadte_t *shadowUpdate = NULL;
    bool powerOnDiff                = false;
    const char *powerOnVal          = NULL;
    size_t powerOnLen               = 0;
    bool brightnessDiff             = false;
    const char *brightnessVal       = NULL;
    size_t brightnessLen            = 0;

    ShadowLogInfo("----- SHADOW DELTA CALLBACK -----");

    if (pCallbackParam != NULL)
    {
        ShadowLogInfo("Callback type: %d", pCallbackParam->callbackType);
        ShadowLogInfo("Thing Name   : %.*s", pCallbackParam->thingNameLength, pCallbackParam->pThingName);
        ShadowLogInfo("Document     : %.*s", pCallbackParam->u.callback.documentLength,
                      pCallbackParam->u.callback.pDocument);
    }
    else
    {
        ShadowLogError("pCallbackParam is NULL");
        status = kStatus_Fail;
    }

    if (status == kStatus_Success)
    {
        /* Check if there is a different "powerOn" state in the Shadow. */
        powerOnDiff = _getDelta(pCallbackParam->u.callback.pDocument, pCallbackParam->u.callback.documentLength,
                                SHADOW_POWERON_FIELD, &powerOnVal, &powerOnLen);
        if (powerOnDiff == true)
        {
            ShadowDemoPowerOn(powerOnVal, powerOnLen);
        }
        else
        {
            powerOnVal = NULL;
            powerOnLen = 0;
            ShadowLogWarn("The Delta Document does not contain any powerOn changes");
        }
    }

    if (status == kStatus_Success)
    {
        /* Check if there is a different "brightness" state in the Shadow. */
        brightnessDiff = _getDelta(pCallbackParam->u.callback.pDocument, pCallbackParam->u.callback.documentLength,
                                   SHADOW_BRIGHTNESS_FIELD, &brightnessVal, &brightnessLen);
        if (brightnessDiff == true)
        {
            ShadowDemoBrightness(brightnessVal, brightnessLen);
        }
        else
        {
            brightnessVal = NULL;
            brightnessLen = 0;
            ShadowLogWarn("The Delta Document does not contain any brightness changes");
        }
    }

    if (status == kStatus_Success)
    {
        /* Do not update the reported state in case there are no powerOn or brightness changes. */
        if ((powerOnDiff == false) && (brightnessDiff == false))
        {
            ShadowLogWarn("No interesting changes, nothing to update");
            status = kStatus_InvalidArgument;
        }
    }

    if (status == kStatus_Success)
    {
        /* Create a response message in order to update the reported state.
         * The update message contains the new values for powerOn and/or brightness */
        shadowUpdate =
            _createResponseUpdateDocument(pCallbackParam, powerOnVal, powerOnLen, brightnessVal, brightnessLen);
        if (shadowUpdate != NULL)
        {
            if (pCallbackContext != NULL)
            {
                /* Notify the APP_Task to perform the update by calling the SendUpdateShadowDemo.
                 * Since we have only 1 iot_thread, we cannot call AwsIotShadow_TimedUpdate directly from here.
                 * AwsIotShadow_TimedUpdate expects that another iot_thread will register the update, but
                 * the only iot_thread is the current thread.
                 */
                ((shadowUpdateCb)(pCallbackContext))((void *)shadowUpdate);
            }
            else
            {
                ShadowLogError("Could not send the Update Document, the Callback function is NULL");
                status = kStatus_Fail;
            }
        }
        else
        {
            ShadowLogError("Could not generate the response update document");
            status = kStatus_Fail;
        }
    }

    if (status == kStatus_Success)
    {
        ShadowLogInfo("----- SHADOW DELTA CALLBACK SUCCESS -----\r\n\r\n");
    }
    else if (kStatus_InvalidArgument)
    {
        ShadowLogWarn("----- SHADOW DELTA CALLBACK SKIP -----\r\n\r\n");
    }
    else
    {
        ShadowLogError("----- SHADOW DELTA CALLBACK FAIL -----\r\n\r\n");
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Shadow updated callback, invoked when the Shadow document changes.
 *
 * This function reports when a Shadow has been updated.
 *
 * @param[in] pCallbackContext Not used.
 * @param[in] pCallbackParam The received Shadow updated document.
 */
static void _shadowUpdatedCallback(void *pCallbackContext, AwsIotShadowCallbackParam_t *pCallbackParam)
{
    status_t status       = kStatus_Success;
    bool previousFound    = false;
    bool currentFound     = false;
    const char *pPrevious = NULL;
    const char *pCurrent  = NULL;
    size_t previousLength = 0;
    size_t currentLength  = 0;

    ShadowLogInfo("----- SHADOW UPDATE CALLBACK -----");

    if (pCallbackParam != NULL)
    {
        ShadowLogInfo("Callback type: %d", pCallbackParam->callbackType);
        ShadowLogInfo("Thing Name   : %.*s", pCallbackParam->thingNameLength, pCallbackParam->pThingName);
    }
    else
    {
        ShadowLogError("pCallbackParam is NULL");
        status = kStatus_Fail;
    }

    if (status == kStatus_Success)
    {
        /* Find the previous Shadow document. */
        previousFound =
            _getUpdatedState(pCallbackParam->u.callback.pDocument, pCallbackParam->u.callback.documentLength,
                             "previous", &pPrevious, &previousLength);
        if (previousFound == true)
        {
            ShadowLogInfo("Previous     : %.*s", previousLength, pPrevious);
        }
        else
        {
            ShadowLogError("The Update Document does not contain the previous state");
            status = kStatus_Fail;
        }
    }

    if (status == kStatus_Success)
    {
        /* Find the current Shadow document. */
        currentFound = _getUpdatedState(pCallbackParam->u.callback.pDocument, pCallbackParam->u.callback.documentLength,
                                        "current", &pCurrent, &currentLength);
        if (currentFound == true)
        {
            ShadowLogInfo("Current      : %.*s", currentLength, pCurrent);
        }
        else
        {
            ShadowLogError("The Update Document does not contain the current state");
            status = kStatus_Fail;
        }
    }

    if (status == kStatus_Success)
    {
        ShadowLogInfo("----- SHADOW UPDATE CALLBACK SUCCESS -----\r\n\r\n");
    }
    else
    {
        ShadowLogError("----- SHADOW UPDATE CALLBACK FAIL -----\r\n\r\n");
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Set the Shadow callback functions used in this demo.
 *
 * @param[in] pDeltaSemaphore Used to synchronize Shadow updates with the delta
 * callback.
 * @param[in] mqttConnection The MQTT connection used for Shadows.
 * @param[in] pThingName The Thing Name for Shadows in this demo.
 * @param[in] thingNameLength The length of `pThingName`.
 *
 * @return `EXIT_SUCCESS` if all Shadow callbacks were set; `EXIT_FAILURE`
 * otherwise.
 */
static int _setShadowCallbacks(void *callbackParamCb,
                               IotMqttConnection_t mqttConnection,
                               const char *pThingName,
                               size_t thingNameLength)
{
    int status                                 = EXIT_SUCCESS;
    AwsIotShadowError_t callbackStatus         = AWS_IOT_SHADOW_STATUS_PENDING;
    AwsIotShadowCallbackInfo_t deltaCallback   = AWS_IOT_SHADOW_CALLBACK_INFO_INITIALIZER;
    AwsIotShadowCallbackInfo_t updatedCallback = AWS_IOT_SHADOW_CALLBACK_INFO_INITIALIZER;

    /* Set the functions for callbacks. */
    deltaCallback.pCallbackContext = callbackParamCb;
    deltaCallback.function         = _shadowDeltaCallback;
    updatedCallback.function       = _shadowUpdatedCallback;

    /* Set the delta callback, which notifies of different desired and reported Shadow states. */
    callbackStatus = AwsIotShadow_SetDeltaCallback(mqttConnection, pThingName, thingNameLength, 0, &deltaCallback);
    if (callbackStatus != AWS_IOT_SHADOW_SUCCESS)
    {
        status = EXIT_FAILURE;
    }

    if (status == EXIT_SUCCESS)
    {
        /* Set the updated callback, which notifies when a Shadow document is changed. */
        callbackStatus =
            AwsIotShadow_SetUpdatedCallback(mqttConnection, pThingName, thingNameLength, 0, &updatedCallback);
        if (callbackStatus != AWS_IOT_SHADOW_SUCCESS)
        {
            status = EXIT_FAILURE;
        }
    }

    if (status != EXIT_SUCCESS)
    {
        ShadowLogError("Failed to set demo shadow callback, error %s.", AwsIotShadow_strerror(callbackStatus));
    }

    return status;
}

/*-----------------------------------------------------------*/

#if SHADOW_BOOT_RESET
/**
 * @brief Try to delete any Shadow document in the cloud.
 *
 * @param[in] mqttConnection The MQTT connection used for Shadows.
 * @param[in] pThingName The Shadow Thing Name to delete.
 * @param[in] thingNameLength The length of `pThingName`.
 */
static void _clearShadowDocument(IotMqttConnection_t mqttConnection,
                                 const char *const pThingName,
                                 size_t thingNameLength)
{
    AwsIotShadowError_t deleteStatus = AWS_IOT_SHADOW_STATUS_PENDING;

    /* Delete any existing Shadow document so that this demo starts with an empty
     * Shadow. */
    deleteStatus = AwsIotShadow_TimedDelete(mqttConnection, pThingName, thingNameLength, 0, TIMEOUT_MS);

    /* Check for return values of "SUCCESS" and "NOT FOUND". Both of these values
     * mean that the Shadow document is now empty. */
    if ((deleteStatus == AWS_IOT_SHADOW_SUCCESS) || (deleteStatus == AWS_IOT_SHADOW_NOT_FOUND))
    {
        ShadowLogInfo("Successfully cleared Shadow of %.*s.", thingNameLength, pThingName);
    }
    else
    {
        ShadowLogWarn("Shadow of %.*s not cleared.", thingNameLength, pThingName);
    }
}
#endif /* SHADOW_BOOT_RESET */

/*-----------------------------------------------------------*/

int SendUpdateShadowDemo(void *document)
{
    int status                       = EXIT_SUCCESS;
    AwsIotShadowError_t shadowStatus = AWS_IOT_SHADOW_SUCCESS;
    IotShadowUpadte_t *updateData    = (IotShadowUpadte_t *)document;

    ShadowLogInfo("----- SENDING THE UPDATE DOCUMENT -----");
    ShadowLogInfo("Thing Name: %.*s", updateData->updateDocument->thingNameLength,
                  updateData->updateDocument->pThingName);
    ShadowLogInfo("Document  : %.*s", updateData->updateDocument->u.update.updateDocumentLength,
                  updateData->updateDocument->u.update.pUpdateDocument);

    /* Give time to iot_thread to finish processing the delta callback */
    vTaskDelay(100);

    shadowStatus = AwsIotShadow_TimedUpdate(updateData->mqttConnection, updateData->updateDocument,
                                            AWS_IOT_SHADOW_FLAG_KEEP_SUBSCRIPTIONS, TIMEOUT_MS);
    if (shadowStatus == AWS_IOT_SHADOW_SUCCESS)
    {
        ShadowLogInfo("----- SENDING THE UPDATE DOCUMENT SUCCESS -----\r\n\r\n");
        status = EXIT_SUCCESS;
    }
    else
    {
        ShadowLogError("----- SENDING THE UPDATE DOCUMENT FAIL -----\r\n\r\n");
        status = EXIT_FAILURE;
    }

    _removeResponseUpdateDocument(updateData);

    return status;
}

int ConfigShadowDemo(const char *pIdentifier, shadowUpdateCb callbackParamCb)
{
    int status = EXIT_SUCCESS;

    if ((pIdentifier == NULL) || (callbackParamCb == NULL))
    {
        ShadowLogError("The parameters should not be NULL");
        status = EXIT_FAILURE;
    }

    if (status == EXIT_SUCCESS)
    {
        if (strlen(pIdentifier) > 0)
        {
            vPortFree(s_thingName);

            s_thingName = pvPortMalloc(strlen(pIdentifier) + 1);
            if (s_thingName == NULL)
            {
                ShadowLogError("Failed to allocate memory for thingName");
                status = EXIT_FAILURE;
            }
        }
        else
        {
            ShadowLogError("A Thing Name (identifier) must be configured for the Shadow demo.");
            status = EXIT_FAILURE;
        }
    }

    if (status == EXIT_SUCCESS)
    {
        s_thingNameLen = strlen(pIdentifier);
        strncpy(s_thingName, pIdentifier, s_thingNameLen);
        s_thingName[s_thingNameLen] = '\0';

        s_appCallback = callbackParamCb;
    }
    else
    {
        vPortFree(s_thingName);
        s_thingName    = NULL;
        s_thingNameLen = 0;
        s_appCallback  = NULL;
    }

    return status;
}

int RunShadowDemo(bool deleteShadow)
{
    int status                         = EXIT_SUCCESS;
    AwsIotShadowError_t shadowStatus   = AWS_IOT_SHADOW_SUCCESS;
    IotMqttConnection_t mqttConnection = IOT_MQTT_CONNECTION_INITIALIZER;
    static bool shadowLibInitialized   = false;

    if ((s_thingName == NULL) || (s_thingNameLen == 0) || (s_appCallback == NULL))
    {
        status = EXIT_FAILURE;
    }

    /* Initialize the Shadow Stack. */
    if (status == EXIT_SUCCESS)
    {
        if (shadowLibInitialized == true)
        {
            /* Deinit the Shadow Stack for a fresh start */
            AwsIotShadow_Cleanup();
            shadowLibInitialized = false;
        }

        shadowStatus = AwsIotShadow_Init(0);
        if (shadowStatus == AWS_IOT_SHADOW_SUCCESS)
        {
            shadowLibInitialized = true;
        }
        else
        {
            ShadowLogError("AwsIotShadow_Init failed.");
            status = EXIT_FAILURE;
        }
    }

    if (status == EXIT_SUCCESS)
    {
        mqttConnection = (IotMqttConnection_t)APP_MQTT_Getv2Handle();
        if (mqttConnection == NULL)
        {
            ShadowLogError("mqttConnection is NULL.");
            status = EXIT_FAILURE;
        }
    }

    /* Set the Shadow callbacks (Delta and Update). */
    if (status == EXIT_SUCCESS)
    {
        status = _setShadowCallbacks(s_appCallback, mqttConnection, s_thingName, s_thingNameLen);
        if (status != EXIT_SUCCESS)
        {
            ShadowLogError("_setShadowCallbacks failed.");
        }
    }

#if SHADOW_BOOT_RESET
    if (status == EXIT_SUCCESS)
    {
        if (deleteShadow == true)
        {
            /* Clear the Shadow document so that application starts with no existing Shadow. */
            _clearShadowDocument(mqttConnection, s_thingName, s_thingNameLen);
        }
    }
#endif /* SHADOW_BOOT_RESET */

    return status;
}

/*-----------------------------------------------------------*/
