/*
 * Copyright 2019-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <FreeRTOS.h>
#include <string.h>

#include "aws_clientcredential.h"
#include AWS_MQTT_AGENT_INCLUDE

#include "device_utils.h"
#include "mqtt_connection.h"
#include "iot_mqtt_types.h"

static MQTTAgentHandle_t mqttHandle = NULL;
static char *alexaCLIENT_ID         = NULL;

/*!
 * @brief Generates an incremental backoff delay between MQTT connections
 *
 * @param retry_attempts number of retries that have occurred
 * @param base_delay is the minimum delay between connections
 *
 * @returns Number of ticks to delay between connections
 */
static TickType_t get_mqtt_connection_backoff(uint32_t retry_attempts, uint32_t base_delay)
{
    /** Example
     *  base_delay = 500ms
     *  retry_attempts = 3
     *  exponential = 150ms = ((base_delay / 10) * retry attempts)
     *  delay_adder_from_base = 950ms = (base_delay + (exponential * retry_attempts)
     *  full_delay = 1450ms = base_delay + delay_adder_from_base
     */
    return (pdMS_TO_TICKS((base_delay) + ((base_delay + ((base_delay / 10) * retry_attempts) * retry_attempts))));
}

/* Retrieves the MQTT v2 connection from the MQTT v1 connection handle. */
extern IotMqttConnection_t MQTT_AGENT_Getv2Connection(MQTTAgentHandle_t xMQTTHandle);

MQTTAgentHandle_t APP_MQTT_Getv2Handle()
{
    MQTTAgentHandle_t retHandle = NULL;

    if (mqttHandle != NULL)
    {
        retHandle = (MQTTAgentHandle_t)MQTT_AGENT_Getv2Connection(mqttHandle);
    }

    return retHandle;
}

MQTTAgentHandle_t APP_MQTT_GetHandle()
{
    return mqttHandle;
}

MQTTAgentHandle_t APP_MQTT_GetOtaHandle()
{
#ifdef SDK_2_8
    return APP_MQTT_GetHandle();
#else
    return APP_MQTT_Getv2Handle();
#endif
}

MQTTAgentReturnCode_t APP_MQTT_Connect(MQTTAgentCallback_t callback)
{
    MQTTAgentReturnCode_t xReturned;
    MQTTAgentConnectParams_t xConnectParameters;
    int attempts = 0;

    if (alexaCLIENT_ID == NULL)
    {
#if USE_BASE64_UNIQUE_ID
        /* Amazon doesn't use special characters, need to remove "=" character */
        APP_GetUniqueID(&alexaCLIENT_ID, true);
#else
        APP_GetHexUniqueID(&alexaCLIENT_ID);
#endif /* USE_BASE64_UNIQUE_ID */
    }

    if (alexaCLIENT_ID != NULL)
    {
        memset(&xConnectParameters, 0, sizeof(xConnectParameters));

        xConnectParameters.pcURL  = clientcredentialMQTT_BROKER_ENDPOINT;
        xConnectParameters.xFlags = mqttagentREQUIRE_TLS;
#if clientcredentialMQTT_BROKER_PORT == 443
        xConnectParameters.xFlags |= mqttagentUSE_AWS_IOT_ALPN_443;
#endif

        /* Deprecated - unused. */
        xConnectParameters.xURLIsIPAddress  = pdFALSE;
        xConnectParameters.usPort           = clientcredentialMQTT_BROKER_PORT;
        xConnectParameters.pucClientId      = (const uint8_t *)alexaCLIENT_ID;
        xConnectParameters.usClientIdLength = (uint16_t)strlen(alexaCLIENT_ID);
        /* Deprecated - unused. */
        xConnectParameters.xSecuredConnection = pdFALSE;
        xConnectParameters.pvUserData         = NULL;
        xConnectParameters.pxCallback         = callback;
        xConnectParameters.pcCertificate      = NULL;
        xConnectParameters.ulCertificateSize  = 0;

        xReturned = MQTT_AGENT_Create(&mqttHandle);
    }
    else
    {
        xReturned = eMQTTAgentFailure;
    }

    if (xReturned == eMQTTAgentSuccess)
    {
        for (attempts = 0; attempts < MQTT_CONNECTION_ATTEMPTS; attempts++)
        {
            /* Connect to the broker. */
            configPRINTF(
                ("MQTT Alexa attempt %d to connect to %s.\r\n", (attempts + 1), clientcredentialMQTT_BROKER_ENDPOINT));
            xReturned = MQTT_AGENT_Connect(mqttHandle, &xConnectParameters, pdMS_TO_TICKS(3000));
            if (xReturned == eMQTTAgentSuccess)
            {
                configPRINTF(("MQTT Alexa connected.\r\n"));
                break;
            }
            else
            {
                configPRINTF(("ERROR %d: MQTT Alexa failed to connect.\r\n", xReturned));

                /* The last attempt does not need a delay */
                if (attempts < (MQTT_CONNECTION_ATTEMPTS - 1))
                {
                    vTaskDelay(get_mqtt_connection_backoff(attempts, MQTT_CONNECTION_DELAY_MS));
                }
            }
        }

        if (xReturned != eMQTTAgentSuccess)
        {
            /* Could not connect, so delete the MQTT client. */
            MQTT_AGENT_Delete(mqttHandle);
            mqttHandle = NULL;
        }
    }

    return xReturned;
}

void APP_MQTT_Disconnect(bool sendDisconnect)
{
    if (NULL != mqttHandle)
    {
        if (sendDisconnect)
        {
            MQTT_AGENT_Disconnect(mqttHandle, 0);
        }

        MQTT_AGENT_Delete(mqttHandle);
        mqttHandle = NULL;
    }
}
