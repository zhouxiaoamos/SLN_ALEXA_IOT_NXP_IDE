/*
 * Copyright 2018-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "reconnection_task.h"

/* Board includes */
#include "board.h"
#include "pin_mux.h"

/* FreeRTOS kernel includes */
#include "FreeRTOS.h"
#include "task.h"

/* AIS includes */
#include "aisv2.h"
#include "aisv2_app.h"
#include "ais_alerts.h"

/* Audio includes */
#include "audio_processing_task.h"

/* Connection includes */
#include "iot_mqtt_agent.h"
#include "iot_wifi.h"
#include "mqtt_connection.h"

/* UX includes */
#include "ux_attention_system.h"

/* OTA includes */
#include "aws_ota_check.h"

/* AWS includes */
#include "aws_iot_demo_shadow.h"

/* WiFi Manager control */
#include "wifi_mgr.h"
#include "wifi_management.h"

#include "sln_flash_mgmt.h"

#include "sln_reset.h"

/*! @brief RECONNECTION Task settings */
#define RECONNECTION_TASK_NAME       "reconnection_task"
#define RECONNECTION_TASK_STACK_SIZE (1280U) /* 5KB */
#define RECONNECTION_TASK_PRIORITY   (configTIMER_TASK_PRIORITY - 1)

#if (defined(AIS_SPEC_REV_325) && (AIS_SPEC_REV_325 == 1))
#define RECONNECTION_EVENT_MASK                                                                                    \
    (kReconnectInvalidSequence | kReconnectMessageTampered | kReconnectAPIDeprecated | kReconnectEncryptionError | \
     kReconnectAISDisconnect | kReconnectNetworkLoss | kReconnectMQTTDisconnect)
#else
#define RECONNECTION_EVENT_MASK                                                                                    \
    (kReconnectInvalidSequence | kReconnectMessageTampered | kReconnectAPIDeprecated | kReconnectEncryptionError | \
     kReconnectAISDisconnect | kReconnectNetworkLoss | kReconnectMQTTDisconnect)
#endif

/* TODO: This is a bit of a hack to push the reconnection out of a waiting state
 * (see aws_mqtt_agent.c for what these numbers are) */
#define RECONNECTION_MQTT_TIMEOUT_MASK \
    (0xFFFF0000UL | 32 |               \
     0x00000001UL) // (mqttMESSAGE_IDENTIFIER_MASK | eMQTTOperationTimedOut | mqttNOTIFICATION_STATUS_MASK)

/* TODO: Clean up this external pile, it does not "spark joy" */
extern ais_handle_t aisHandle;
extern ota_handle_t otaHandle;
extern EventGroupHandle_t s_offlineAudioEventGroup;

static TaskHandle_t *s_thisTask = NULL;

static uint32_t s_reconnectState = kInitState;

static ais_disconnect_code_t s_aisDisconnectCode = AIS_DISCONNECT_GOING_OFFLINE;

static EventGroupHandle_t s_reconnectionEventGroup;

/* Disconnect timeout timer handle */
static TimerHandle_t s_disconnectTimeoutHandle;

/* Reconnect timeout timer handle */
static TimerHandle_t s_reconnectTimeoutHandle;

/* AIS Connection timeout timer handle */
static TimerHandle_t s_aisConnectionTimeoutHandle;

/* AIS Disconnection timeout timer handle */
static TimerHandle_t s_aisDisconnectionTimeoutHandle;

/* OTA Connection timeout timer handle */
static TimerHandle_t s_otaConnectionTimeoutHandle;

/* OTA Disconnection timeout timer handle */
static TimerHandle_t s_otaDisconnectionTimeoutHandle;

/* NetRecv task still running check timer handle */
static TimerHandle_t s_netRecvRunningCheckTimerHandle;

__attribute__((section(".ocram_non_cacheable_bss")))
StackType_t reconnection_task_stack_buffer[RECONNECTION_TASK_STACK_SIZE];
__attribute__((section(".ocram_non_cacheable_bss"))) StaticTask_t reconnection_task_buffer;

BaseType_t prvMQTTCallback(void *, const MQTTAgentCallbackParams_t *const);

void ais_app_disconnect_timeout_cb(TimerHandle_t xTimer)
{
    /* Mitigation for MQTT Disconnect deadlock */
    if (s_reconnectState == kLinkLoss)
    {
        sln_reset("[ERROR] MQTT Disconnect timeout... resetting...");
    }
}

void ais_app_reconnect_timeout_cb(TimerHandle_t xTimer)
{
    /* Mitigation for MQTT Disconnect deadlock */
    if (s_reconnectState == kMqttReconnect)
    {
        sln_reset("[ERROR] MQTT Connect timeout... resetting...");
    }
}

void ais_app_ais_connect_timeout_cb(TimerHandle_t xTimer)
{
    /* Mitigation for AIS Connection deadlock */
    if (s_reconnectState == kAisReconnect)
    {
        sln_reset("[ERROR] AIS Connect timeout... resetting...");
    }
}

void ais_app_ais_disconnect_timeout_cb(TimerHandle_t xTimer)
{
    /* Mitigation for AIS Disconnect deadlock */
    if (s_reconnectState == kAisDisconnect)
    {
        sln_reset("[ERROR] AIS Disconnect timeout... resetting...");
    }
}

void ais_app_ota_connect_timeout_cb(TimerHandle_t xTimer)
{
    /* Mitigation for OTA Connect deadlock */
    if (s_reconnectState == kOtaReconnect)
    {
        sln_reset("[ERROR] OTA Connect timeout... resetting...");
    }
}

void ais_app_ota_disconnect_timeout_cb(TimerHandle_t xTimer)
{
    /* Mitigation for OTA Disconnect deadlock */
    if (s_reconnectState == kOtaDisconnect)
    {
        sln_reset("[ERROR] OTA Disconnect timeout... resetting...");
    }
}

void ais_app_net_recv_running_check_timeout_cb(TimerHandle_t xTimer)
{
    static uint8_t consecutive_times = 0;
    /* Check NetRecv task is still running.
     * In case NetRecv task is not running for AIS_APP_NET_RECV_CONSECUTIVE_TIMES checks,
     * stop the timer and notify the reconnection task.
     * Consecutive counter is used in order to give time other mechanisms
     * to trigger reconnection (ex: WiFi link lost from WWD needs time to propagate).
     * Timer auto-reload is enabled so there is no need to restart manually this timer.
     */
    if (xTaskGetHandle("NetRecv") == NULL)
    {
        if (s_reconnectState == kStartState)
        {
            consecutive_times++;
            if (consecutive_times == AIS_APP_NET_RECV_CONSECUTIVE_TIMES)
            {
                if (xTimerStop(s_netRecvRunningCheckTimerHandle, 0) != pdPASS)
                {
                    configPRINTF(("xTimerStop s_netRecvRunningCheckTimerHandle failed\r\n"));
                }

                if (reconnection_task_set_event(kReconnectMQTTDisconnect))
                {
                    configPRINTF(("NetRecv task is not running. Reconnection triggered.\r\n"));
                }
            }
        }
        else
        {
            consecutive_times = 0;

            /* Something else already triggered the reconnection process. */
            if (xTimerStop(s_netRecvRunningCheckTimerHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStop s_netRecvRunningCheckTimerHandle failed\r\n"));
            }
        }
    }
    else
    {
        consecutive_times = 0;
    }
}

static void reconnect_interaction_ux(void)
{
    ux_attention_states_t currUxState;
    ais_app_data_t *appData = AIS_APP_GetAppData();

    /* Set back to idle state to follow the behavior of Echo Dot */
    appData->state = AIS_STATE_IDLE;

    currUxState = ux_attention_get_state();

    if (UX_INTERACTION_MASK(currUxState))
    {
        ux_attention_set_state(uxDisconnected);
    }
}

static void reconnect_state_machine(uint32_t *reconnectState)
{
    status_t connectStatus        = kStatus_Fail;
    status_t disconnectStatus     = kStatus_Fail;
    wifi_status_t wifiStatus      = kWiFiStatus_Fail;
    ais_app_data_t *appData       = AIS_APP_GetAppData();
    MQTTAgentReturnCode_t mqttRet = eMQTTAgentSuccess;

    uint32_t attempts = 3;

    switch (*reconnectState)
    {
        case kLinkLoss:

            reconnect_interaction_ux();

            if (xTimerStart(s_disconnectTimeoutHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStart failed\r\n"));
            }

            /* Clean-up the speaker state */
            AIS_TerminateSpeaker(&aisHandle);
            /* AIS connection state clean-up */
            AIS_Disconnected(&aisHandle);
            /* MQTT connection state clean-up */
            APP_MQTT_Disconnect(false);

            *reconnectState = kLinkFix;
            configPRINTF(("Waiting for link to re-establish...\r\n"));

            break;

        case kLinkFix:
            reconnect_interaction_ux();

            /* Block here to wait for link to re-establish. */
            wifiStatus = wifi_reconnect();
            if (wifiStatus == kWiFiStatus_Success)
            {
                *reconnectState = kMqttReconnect;
                configPRINTF(("...link re-established!\r\n"));
            }
            else
            {
                /* Could not reconnect to WiFi, better restart. */
                *reconnectState = kSysReboot;
                configPRINTF(("[ERROR] WiFi reconnection failed, better restart\r\n"));
            }

            break;

        case kMqttReconnect:
            reconnect_interaction_ux();

            /* Clean-up the speaker state */
            AIS_TerminateSpeaker(&aisHandle);
            /* AIS connection state clean-up */
            AIS_Disconnected(&aisHandle);
            /* MQTT connection state clean-up */
            APP_MQTT_Disconnect(false);

            if (xTimerStart(s_reconnectTimeoutHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStart failed\r\n"));
            }

            mqttRet = APP_MQTT_Connect(prvMQTTCallback);
            if (mqttRet == eMQTTAgentSuccess)
            {
                if (RunShadowDemo(false) == EXIT_SUCCESS)
                {
                    configPRINTF(("SHADOW reconnected\r\n"));
                }
                else
                {
                    configPRINTF(("[WARNING] The shadow could not be reconnected\r\n"));
                }

                *reconnectState = kOtaDisconnect;
            }
            else
            {
                configPRINTF(("Fail to connect to MQTT, error %d, restarting the system\r\n", mqttRet));
                *reconnectState = kSysReboot;
            }

            break;

        case kOtaDisconnect:
            configPRINTF(("Disconnect from OTA service\r\n"));

            if (xTimerStart(s_otaDisconnectionTimeoutHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStart failed\r\n"));
            }
            disconnectStatus = OTA_Disconnect(&otaHandle);

            if (disconnectStatus == kStatus_Success)
            {
                *reconnectState = kOtaReconnect;
            }
            else
            {
                *reconnectState = kLinkLoss;
                configPRINTF(("Fail to disconnect from OTA service\r\n"));
            }
            break;

        case kOtaReconnect:
            do
            {
                configPRINTF(("Reconnect to OTA service, countdown = %d\r\n", attempts));

                if (xTimerStart(s_otaConnectionTimeoutHandle, 0) != pdPASS)
                {
                    configPRINTF(("xTimerStart failed\r\n"));
                }
                connectStatus = OTA_Connect(&otaHandle);

                if (connectStatus == kStatus_Success)
                {
                    if (aisHandle.aisConnected)
                    {
                        *reconnectState = kAisDisconnect;
                    }
                    else
                    {
                        *reconnectState = kAisReconnect;
                    }
                }
                else
                {
                    attempts--;
                    if (attempts == 0)
                    {
                        *reconnectState = kLinkLoss;
                        configPRINTF(("Fail to reconnect to OTA service, countdown = %d, move on...\r\n", attempts));
                    }
                }
            } while ((attempts > 0) && (connectStatus != kStatus_Success));
            break;

        case kAisDisconnect:

            /* Clean-up the speaker state */
            AIS_TerminateSpeaker(&aisHandle);

            if (xTimerStart(s_aisDisconnectionTimeoutHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStart failed\r\n"));
            }

            if (AIS_DISCONNECT_NONE != s_aisDisconnectCode)
            {
                disconnectStatus = AIS_Disconnect(&aisHandle, s_aisDisconnectCode);
            }
            else
            {
                /* If we don't have a particular error code, disconnect with GOING_OFFLINE code */
                disconnectStatus = AIS_Disconnect(&aisHandle, AIS_DISCONNECT_GOING_OFFLINE);
            }

            /* Timeout will have changed our state, best not to ignore */
            if (kAisDisconnect == *reconnectState)
            {
                if (kStatus_Success == disconnectStatus)
                {
                    *reconnectState = kAisReconnect;
                }
                else
                {
                    configPRINTF(("Fail to disconnect from AIS service, disconnecting from MQTT\r\n"));
                    *reconnectState = kLinkLoss;

                    /* If initially AIS disconnect was intended, now avoid it as reconnect will suffice */
                    s_aisDisconnectCode = AIS_DISCONNECT_NONE;
                }
            }
            break;

        case kAisReconnect:

            /* Connect to the service again */
            do
            {
                uint32_t registrationConfigLen = sizeof(aisHandle.config->registrationConfig);

                vTaskDelay(portTICK_PERIOD_MS * 500);

                if (xTimerStart(s_aisConnectionTimeoutHandle, 0) != pdPASS)
                {
                    configPRINTF(("xTimerStart failed\r\n"));
                }

                /* Reload the secret and topic before connecting just in case a disconnect happened during a secret
                 * rotate */
                if (kStatus_Success != SLN_FLASH_MGMT_Read(AIS_REGISTRATION_INFO_FILE_NAME,
                                                           (uint8_t *)&aisHandle.config->registrationConfig,
                                                           &registrationConfigLen))
                {
                    configPRINTF(("Error, Missing Secret, unexpected issues will occur!\r\n"));
                }

                connectStatus = AIS_Connect(&aisHandle);
                attempts--;
            } while ((attempts > 0) && (kStatus_Success != connectStatus));

            if (kStatus_Success == connectStatus)
            {
                /* Stop any offline audio */
                if (NULL != s_offlineAudioEventGroup)
                {
                    xEventGroupSetBits(s_offlineAudioEventGroup, OFFLINE_AUDIO_ABORT);
                }

                /* Gather any alerts requires for deletion */
                alertTokenList_t alertsList;
                uint32_t alertsCount = 0;

#if (defined(AIS_SPEC_REV_325) && (AIS_SPEC_REV_325 == 1))
                AIS_Alerts_GetAlertsList(&alertsList, &alertsCount);
#else
                AIS_Alerts_GetDeletedList(&alertsList, &alertsCount);
#endif
                configPRINTF(("Found %d alerts ready to send.\r\n", alertsCount));

                /* Send SynchronizeState to update on our status. */
                char *alertTokens[AIS_APP_MAX_ALERT_COUNT] = {0};
                for (uint32_t idx = 0; idx < alertsCount; idx++)
                {
                    alertTokens[idx] = (char *)(&alertsList[idx]);
                    configPRINTF(("Alert ready to send: %s\r\n", alertTokens[idx]));
                }

                /* Send synchronize event */
                connectStatus =
                    AIS_EventSynchronizeState(&aisHandle, appData->volume, (const char **)alertTokens, alertsCount);

                if (kStatus_Success != connectStatus)
                {
                    configPRINTF(("[Reconnect] Error sending SynchronizeState: %d\r\n", connectStatus));
                }

                *reconnectState = kLinkUp;
            }
            else
            {
                *reconnectState = kLinkLoss;
            }
            break;

        case kLinkUp:

            *reconnectState = kStartState;

            /* Stop all the timeouts (we don't need them anymore) */
            if (xTimerStop(s_disconnectTimeoutHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStop failed\r\n"));
            }

            if (xTimerStop(s_reconnectTimeoutHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStop failed\r\n"));
            }

            if (xTimerStop(s_aisConnectionTimeoutHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStop failed\r\n"));
            }

            if (xTimerStop(s_aisDisconnectionTimeoutHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStop failed\r\n"));
            }

            if (xTimerStop(s_otaConnectionTimeoutHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStop failed\r\n"));
            }

            if (xTimerStop(s_otaDisconnectionTimeoutHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStop failed\r\n"));
            }

            if (xTimerStart(s_netRecvRunningCheckTimerHandle, 0) != pdPASS)
            {
                configPRINTF(("xTimerStart failed\r\n"));
            }
            /* Reconnect cycle end */
            break;

        case kSysReboot:

            ux_attention_sys_fault();
            sln_reset("kSysReboot"); /* Something has gone wrong */
            break;

        default:
            break;
    }
}

void reconnectionTask(void *arg)
{
    EventBits_t reconnectionEventBits;

    s_reconnectionEventGroup = xEventGroupCreate();

    if (s_reconnectionEventGroup != NULL)
    {
        /* Stop all the timeouts (we don't need them yet) */
        if (xTimerStop(s_disconnectTimeoutHandle, 0) != pdPASS)
        {
            configPRINTF(("xTimerStop failed\r\n"));
        }

        if (xTimerStop(s_reconnectTimeoutHandle, 0) != pdPASS)
        {
            configPRINTF(("xTimerStop failed\r\n"));
        }

        if (xTimerStop(s_aisConnectionTimeoutHandle, 0) != pdPASS)
        {
            configPRINTF(("xTimerStop failed\r\n"));
        }

        if (xTimerStop(s_aisDisconnectionTimeoutHandle, 0) != pdPASS)
        {
            configPRINTF(("xTimerStop failed\r\n"));
        }

        if (xTimerStop(s_otaConnectionTimeoutHandle, 0) != pdPASS)
        {
            configPRINTF(("xTimerStop failed\r\n"));
        }

        if (xTimerStop(s_otaDisconnectionTimeoutHandle, 0) != pdPASS)
        {
            configPRINTF(("xTimerStop failed\r\n"));
        }

        while (1)
        {
            reconnectionEventBits =
                xEventGroupWaitBits(s_reconnectionEventGroup, RECONNECTION_EVENT_MASK, pdTRUE, pdFALSE, portMAX_DELAY);

            if (kReconnectInvalidSequence & reconnectionEventBits)
            {
                /* Set reconnect state machine entry state */
                s_reconnectState = kAisDisconnect;

                /* Set code for disconnect publish message */
                s_aisDisconnectCode = AIS_DISCONNECT_INVALID_SEQUENCE;
            }
            else if (kReconnectMessageTampered & reconnectionEventBits)
            {
                /* Set reconnect state machine entry state */
                s_reconnectState = kAisDisconnect;

                /* Set code for disconnect publish message */
#if (defined(AIS_SPEC_REV_325) && (AIS_SPEC_REV_325 == 1))
                s_aisDisconnectCode = AIS_DISCONNECT_MESSAGE_TAMPERED;
#endif
            }
            else if (kReconnectAPIDeprecated & reconnectionEventBits)
            {
                /* Set reconnect state machine entry state */
                s_reconnectState = kAisDisconnect;

                /* Set code for disconnect publish message */
                s_aisDisconnectCode = AIS_DISCONNECT_API_DEPRECATED;
            }
            else if (kReconnectEncryptionError & reconnectionEventBits)
            {
                /* Set reconnect state machine entry state */
                s_reconnectState = kAisDisconnect;

                /* Set code for disconnect publish message */
                s_aisDisconnectCode = AIS_DISCONNECT_ENCRYPTION_ERROR;
            }
            else if (kReconnectAISDisconnect & reconnectionEventBits)
            {
                /* Set reconnect state machine entry state */
                s_reconnectState = kAisDisconnect;

                /* Not sending a disconnect */
                s_aisDisconnectCode = AIS_DISCONNECT_NONE;
            }
            else if (kReconnectNetworkLoss & reconnectionEventBits)
            {
                /* Set reconnect state machine entry state */
                s_reconnectState = kLinkLoss;

                /* Not sending a disconnect */
                s_aisDisconnectCode = AIS_DISCONNECT_NONE;
            }
            else if (kReconnectMQTTDisconnect & reconnectionEventBits)
            {
                /* Set reconnect state machine entry state */
                s_reconnectState = kMqttReconnect;

                /* Not sending a disconnect */
                s_aisDisconnectCode = AIS_DISCONNECT_NONE;
            }

            /* Run reconnect state machine to regain connection */
            while ((kStartState != s_reconnectState))
            {
                reconnect_state_machine(&s_reconnectState);
            }
        }
    }
}

bool reconnection_task_set_event(reconnectEvent_t event)
{
    if (s_reconnectState == kStartState)
    {
        audio_processing_set_state(kReconnect);
        /* Return to default CPU speed when Idle */
        BOARD_RevertClock();

        if (NULL != s_reconnectionEventGroup)
        {
            xEventGroupSetBits(s_reconnectionEventGroup, event);
        }
        return true;
    }
    return false;
}

reconnectState_t reconnection_task_get_state(void)
{
    return s_reconnectState;
}

int32_t reconnection_task_init(TaskHandle_t *handle)
{
    s_disconnectTimeoutHandle = xTimerCreate("AIS_MQTT_Timeout", AIS_APP_MQTT_DISCONNECT_TIMEOUT_MSEC, pdFALSE,
                                             (void *)0, ais_app_disconnect_timeout_cb);

    if (NULL == s_disconnectTimeoutHandle)
    {
        return 1;
    }

    s_reconnectTimeoutHandle = xTimerCreate("AIS_MQTT_Timeout2", AIS_APP_MQTT_RECONNECT_TIMEOUT_MSEC, pdFALSE,
                                            (void *)0, ais_app_reconnect_timeout_cb);

    if (NULL == s_reconnectTimeoutHandle)
    {
        return 2;
    }

    s_aisConnectionTimeoutHandle = xTimerCreate("AIS_MQTT_Timeout3", AIS_APP_MQTT_AIS_CONNECT_TIMEOUT_MSEC, pdFALSE,
                                                (void *)0, ais_app_ais_connect_timeout_cb);

    if (NULL == s_aisConnectionTimeoutHandle)
    {
        return 3;
    }

    s_aisDisconnectionTimeoutHandle = xTimerCreate("AIS_MQTT_Timeout4", AIS_APP_MQTT_DISCONNECT_TIMEOUT_MSEC, pdFALSE,
                                                   (void *)0, ais_app_ais_disconnect_timeout_cb);

    if (NULL == s_aisDisconnectionTimeoutHandle)
    {
        return 4;
    }

    s_otaConnectionTimeoutHandle = xTimerCreate("AIS_MQTT_Timeout5", AIS_APP_MQTT_OTA_CONNECT_TIMEOUT_MSEC, pdFALSE,
                                                (void *)0, ais_app_ota_connect_timeout_cb);

    if (NULL == s_otaConnectionTimeoutHandle)
    {
        return 5;
    }

    s_otaDisconnectionTimeoutHandle = xTimerCreate("AIS_MQTT_Timeout6", AIS_APP_MQTT_OTA_DISCONNECT_TIMEOUT_MSEC,
                                                   pdFALSE, (void *)0, ais_app_ota_disconnect_timeout_cb);

    if (NULL == s_otaDisconnectionTimeoutHandle)
    {
        return 6;
    }

    s_netRecvRunningCheckTimerHandle = xTimerCreate("NetRecvRunningCheck_Timer", AIS_APP_NET_RECV_CHECK_MSEC, pdTRUE,
                                                    (void *)0, ais_app_net_recv_running_check_timeout_cb);

    if (NULL == s_netRecvRunningCheckTimerHandle)
    {
        return 7;
    }

    if (xTimerStart(s_netRecvRunningCheckTimerHandle, 0) != pdPASS)
    {
        configPRINTF(("xTimerStart s_netRecvRunningCheckTimerHandle failed\r\n"));
        return 8;
    }

    *handle = xTaskCreateStatic(reconnectionTask, RECONNECTION_TASK_NAME, RECONNECTION_TASK_STACK_SIZE, NULL,
                                RECONNECTION_TASK_PRIORITY, reconnection_task_stack_buffer, &reconnection_task_buffer);
    if (*handle == NULL)
    {
        configPRINTF(("[RECONN TASK] Task Creation Failed\r\n"));
        return 9;
    }
    else
    {
        s_thisTask       = handle;
        s_reconnectState = kStartState;
    }

    return 0;
}
