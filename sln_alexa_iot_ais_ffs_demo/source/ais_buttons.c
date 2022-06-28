/*
 * Copyright 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "switch.h"

/* AIS includes */
#include "ais_alerts.h"
#include "aisv2_app.h"
#include "ais_streamer.h"

#include "audio_processing_task.h"
#include "network_connection.h"
#include "reconnection_task.h"
#include "sln_cfg_file.h"
#include "wifi_credentials.h"

/* UX includes */
#include "sln_RT10xx_RGB_LED_driver.h"
#include "ux_attention_system.h"
#include "app_events.h"
#include "avs_sound_library.h"

#include "sln_reset.h"

/*******************************************************************************
 * Code
 ******************************************************************************/
extern ais_handle_t aisHandle;
extern streamer_handle_t streamerHandle;
extern avs_sound_library_t avs_sounds;
extern EventGroupHandle_t g_buttonPressed;
extern EventGroupHandle_t s_offlineAudioEventGroup;

#ifdef FFS_ENABLED
extern TaskHandle_t mainTaskHandle;
/* FFS registration related */
extern bool g_ffsRunning;
extern bool g_ugsButtonPressed;
#endif

static int32_t SaveVolumeToFlash(ais_app_data_t *appData)
{
    uint32_t len      = 0;
    sln_dev_cfg_t cfg = DEFAULT_CFG_VALUES;

    int32_t status = SLN_FLASH_MGMT_OK;
    /* When the button one is held and button two is released, this is a volume down change */
    status = SLN_FLASH_MGMT_Read(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, &len);

    if (SLN_FLASH_MGMT_OK == status)
    {
        /* Check if the volume is different. If it's not, skip and return success */
        if (appData->volume != cfg.streamVolume)
        {
            cfg.streamVolume = appData->volume;
            status           = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));

            if ((SLN_FLASH_MGMT_EOVERFLOW == status) || (SLN_FLASH_MGMT_EOVERFLOW2 == status))
            {
                SLN_FLASH_MGMT_Erase(DEVICE_CONFIG_FILE_NAME);
                /* Volume Down */
                status = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));
            }
        }
    }
    else if ((SLN_FLASH_MGMT_ENOENTRY2 == status) || (SLN_FLASH_MGMT_ENOENTRY == status))
    {
        // We should have an empty file so we can save a new one
        cfg.streamVolume = appData->volume;
        status           = SLN_FLASH_MGMT_Save(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, sizeof(sln_dev_cfg_t));
    }

    if (SLN_FLASH_MGMT_OK != status)
    {
        configPRINTF(("[AIS App] ERROR: Unable to store stream volume %d to NVM\r\n", appData->volume));
        configPRINTF(("[AIS App] ERROR: NVM operation returned with %d\r\n", status));
    }

    return status;
}

static void vProcessVolume(ais_app_data_t *appData, ais_app_volume_direction_t direction)
{
    int32_t status             = SLN_FLASH_MGMT_OK;
    reconnectState_t currState = reconnection_task_get_state();
    uint64_t offset            = 0;

    if (kInitState != currState)
    {
        /* Guard against a current invalid volume which could be after startup */
        if (appData->volume < 0)
        {
            appData->volume = 0;
        }

        appData->volume += direction * 10;
        appData->volume = (appData->volume > 100 ? 100 : appData->volume);
        appData->volume = (appData->volume < 0 ? 0 : appData->volume);

        status = SaveVolumeToFlash(appData);

        if (SLN_FLASH_MGMT_OK == status)
        {
            STREAMER_SetVolume(appData->volume);
            ux_attention_set_state(uxDeviceChange);

            configPRINTF(("Change volume to %d\r\n", appData->volume));

            if ((kStartState == currState) || (kLinkUp == currState))
            {
                if (appData->speakerOpen)
                {
                    offset = appData->speakerOffsetWritten - STREAMER_GetQueuedRaw(aisHandle.audioPlayer);
                }
                AIS_EventVolumeChanged(&aisHandle, appData->volume, offset);
            }
        }
        else
        {
            configPRINTF(("Failed to save to flash restoring to previous volume\r\n"));
            appData->volume -= direction * 10;
        }
    }
}

static void vProcessAlexaActionButton(ais_app_data_t *appData)
{
    /* Make sure we are offline */
    reconnectState_t currState = reconnection_task_get_state();
    bool alertsToDelete        = false;

    /* If the device is offline, process offline alerts */
    if ((kStartState != currState) && (kLinkUp != currState))
    {
        int32_t ret = 0;

        /* Mark all that are not idle as deleted */
        for (uint32_t idx = 0; idx < AIS_APP_MAX_ALERT_COUNT; idx++)
        {
            if (false == AIS_Alerts_GetIdleState(idx))
            {
                /* Make a note that we should notify the alert task. */
                alertsToDelete = true;

                ret = AIS_Alerts_MarkForDeleteOffline(idx);
                if (0 != ret)
                {
                    configPRINTF(("Unable to mark alert for delete: %d\r\n", ret));
                }
            }
        }

        if (alertsToDelete)
        {
            /* Send request to alert task to delete marked alerts */
            TaskHandle_t alertTask = AIS_Alerts_GetThisHandle();

            if (NULL != alertTask)
            {
                xTaskNotify(alertTask, APP_EVENTS_SET_EVENT(kAlertEvents, kAlertDelete), eSetBits);
            }
            else
            {
                configPRINTF(("Alert task handle is NULL.\r\n"));
            }
        }

        ux_attention_states_t currUxState = ux_attention_get_state();

        switch (currUxState)
        {
            case uxTimer:
                ux_attention_set_state(uxTimerEnd);
                break;
            case uxAlarm:
                ux_attention_set_state(uxAlarmEnd);
                break;
            case uxReminder:
                ux_attention_set_state(uxReminderEnd);
                break;
            default:
                break;
        };

        if (NULL != s_offlineAudioEventGroup)
        {
            xEventGroupSetBits(s_offlineAudioEventGroup, OFFLINE_AUDIO_ABORT);
        }
    }
    /* Device is online */
    else
    {
        if (AIS_CheckState(&aisHandle, AIS_TASK_STATE_MICROPHONE))
        {
            AIS_AppCallback_CloseMicrophone(&aisHandle);
            AIS_EventMicrophoneClosed(&aisHandle, aisHandle.micStream.audio.audioData.offset);
        }
        else
        {
            configPRINTF(("Alexa Action Button\r\n"));
            /* If the device is in idle, act as a wakeup/barge-in
             * Need to ensure that the device is not in mute mode.
             * Echo dot doesn't wakeup when in mute mode
             * If its IDLE, DND or NOTIFICATION and NOT MUTE, open Mic
             */
            if (((appData->state == AIS_STATE_IDLE) || (appData->state == AIS_STATE_DO_NOT_DISTURB) ||
                 (appData->state == AIS_STATE_NOTIFICATION)) &&
                (audio_processing_get_mic_mute() == kMicMuteModeOff))
            {
                configPRINTF(("Push to talk\r\n"));
                /* Boost CPU now for best performance */
                BOARD_BoostClock();
                /* Send a wakeup but without cloud based verifier */
                if (AIS_CheckState(&aisHandle, AIS_TASK_STATE_SPEAKER))
                {
                    appData->bargein = true;

                    /* Force speaker state update here to close and send event */
                    AIS_StateSpeaker(&aisHandle);
                }
                audio_processing_set_state(kMicKeepOpen);
            }
            /* Else treat it like a stop button which the service will handle */
            else
            {
                /* Anything else we should send button stop */
                AIS_EventButtonCommandIssued(&aisHandle, AIS_BUTTON_CMD_STOP);
            }
        }
    }
}

void button_task(void *arg)
{
    EventBits_t bits;
    ais_app_data_t *appData = AIS_APP_GetAppData();

    volatile uint32_t sec_1    = 0;
    OsaTimeval pressed_time_1  = {0};
    OsaTimeval released_time_1 = {0};

    volatile uint32_t sec_2    = 0;
    OsaTimeval pressed_time_2  = {0};
    OsaTimeval released_time_2 = {0};

    bool buttonHeld1      = false;
    bool buttonHeld2      = false;
    bool buttonProcessed1 = false;
    bool buttonProcessed2 = false;

    while (1)
    {
        bits = xEventGroupWaitBits(g_buttonPressed, BIT_PRESS_1 | BIT_RELEASE_1 | BIT_PRESS_2 | BIT_RELEASE_2, pdTRUE,
                                   pdFALSE, portMAX_DELAY);

        if (bits & BIT_PRESS_1)
        {
            osa_time_get(&pressed_time_1);
            buttonHeld1 = true;
        }

        if (buttonHeld1 && (bits & BIT_RELEASE_1))
        {
            /* If an event was already processed on another button then reset and don't do anything
             * We don't want to process an overloaded function if volume was changed*/
            if (buttonProcessed2)
            {
                buttonHeld2      = false;
                buttonHeld1      = false;
                buttonProcessed1 = false;
                buttonProcessed2 = false;
                continue;
            }
            /* When the button two is held and button one is released, this is a volume up change */
            if (buttonHeld2 == true)
            {
                vProcessVolume(appData, kVolumeUp);
                buttonProcessed1 = true;
                continue;
            }

            osa_time_get(&released_time_1);
            sec_1 = (released_time_1.tv_usec - pressed_time_1.tv_usec) / 1000000;

            /* factory reset */
            if (sec_1 >= 10)
            {
                ux_attention_set_state(uxFactoryReset);
                /* a bit of delay to let everyone admire the beautiful LED blinking */
                osa_time_delay(3000);

                AIS_Deregister();
            }
            if (sec_1 <= 3)
            {
                if (kMicMuteModeOff == audio_processing_get_mic_mute())
                {
                    configPRINTF(("Switch to microphone mute mode ...\r\n"));
                    /* ignore error code on purpose, logs printed in the function */
                    audio_processing_set_mic_mute(kMicMuteModeOn, true);
                    ux_attention_set_state(uxMicOntoOff);

                    /* Play local sound for privacy mode on */
                    if (streamerHandle.streamer != NULL)
                    {
                        STREAMER_SetLocalSound(&streamerHandle, avs_sounds.core_sound_files->state_privacy_mode_on,
                                               avs_sounds.core_sound_sizes->state_privacy_mode_on_len);
                        if (!STREAMER_IsPlaying(&streamerHandle))
                        {
                            STREAMER_Start(&streamerHandle);
                            STREAMER_SetVolume(appData->volume);
                        }
                    }
                }
                else
                {
                    configPRINTF(("Finish microphone mute mode ...\r\n"));
                    /* ignore error code on purpose, logs printed in the function */
                    audio_processing_set_mic_mute(kMicMuteModeOff, true);
                    /* Set the UX state to the current one */
                    ux_attention_resume_state(appData->state);

                    /* Play local sound for privacy mode off */
                    if (streamerHandle.streamer != NULL)
                    {
                        STREAMER_SetLocalSound(&streamerHandle, avs_sounds.core_sound_files->state_privacy_mode_off,
                                               avs_sounds.core_sound_sizes->state_privacy_mode_off_len);
                        if (!STREAMER_IsPlaying(&streamerHandle))
                        {
                            STREAMER_Start(&streamerHandle);
                            STREAMER_SetVolume(appData->volume);
                        }
                    }
                }
            }
#if USE_WIFI_CONNECTION
            else if ((sec_1 >= 5) && (sec_1 < 10))
            {
#ifdef FFS_ENABLED
                if (g_ffsRunning && (mainTaskHandle != NULL))
                {
                    ux_attention_set_state(uxUGSSessionRestart);
                    /* a bit of delay to let everyone admire the beautiful LED blinking */
                    osa_time_delay(3000);

                    g_ugsButtonPressed = true;
                    vTaskResume(mainTaskHandle);
                }
                else
#endif /* FFS_ENABLED */
                {
                    ux_attention_set_state(uxWiFiCredentialsErase);
                    /* a bit of delay to let everyone admire the beautiful LED blinking */
                    osa_time_delay(3000);

                    wifi_credentials_flash_reset();
                    sln_reset("WiFi credentials reset button pressed");
                }
            }
            buttonHeld1 = false;
#endif /* USE_WIFI_CONNECTION */
        }

        if (bits & BIT_PRESS_2)
        {
            osa_time_get(&pressed_time_2);
            buttonHeld2 = true;
        }

        if (buttonHeld2 && (bits & BIT_RELEASE_2))
        {
            /* If an event was already processed on another button then reset and don't do anything
             * We don't want to process an overloaded function if volume was changed*/
            if (buttonProcessed1)
            {
                buttonHeld2      = false;
                buttonHeld1      = false;
                buttonProcessed1 = false;
                buttonProcessed2 = false;
                continue;
            }
            if (buttonHeld1 == true)
            {
                /* When the button one is held and button two is released, this is a volume Down change */
                vProcessVolume(appData, kVolumeDown);
                buttonProcessed2 = true;
                continue;
            }

            osa_time_get(&released_time_2);
            sec_2 = (released_time_2.tv_usec - pressed_time_2.tv_usec) / 1000000;

            if (sec_2 <= 3)
            {
                /* This is Alexa Action Button */
                vProcessAlexaActionButton(appData);
            }
            buttonHeld2 = false;
        }
    }

    vTaskDelete(NULL);
}
