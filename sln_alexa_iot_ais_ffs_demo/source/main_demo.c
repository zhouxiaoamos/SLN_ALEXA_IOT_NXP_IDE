/*
 * Copyright 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/* Board includes */
#include "board.h"
#include "pin_mux.h"
#include "fsl_pit.h"

/* FreeRTOS kernel includes */
#include "FreeRTOS.h"
#include "task.h"

/* ACE SDK and DPK includes */
#include <ace/ace.h>
#include <ace/ffs_provisionable_api.h>
#include "hal_dha.h"
#include "port_dha_storage.h"
#include "hal_device_info.h"
#include "hal_kv_storage.h"

/* AWS includes */
#include "iot_init.h"
#include "iot_system_init.h"
#include "aws_clientcredential.h"
#include "aws_ota_check.h"
#include "aws_demo.h"
#include "aws_iot_demo_shadow.h"

/* AIS includes */
#include "aisv2_app.h"
#include "ais_alerts.h"
#include "ais_streamer.h"

/* Audio processing includes */
#include "audio_processing_task.h"
#include "pdm_to_pcm_task.h"
#include "sln_amplifier.h"
#include "avs_sound_library.h"

/* Network connection includes */
#include "reconnection_task.h"
#include "mqtt_connection.h"
#include "wifi_credentials.h"
#include "wifi_management.h"

/* Wake word includes*/
#include "amazon_wake_word.h"

/* UX includes */
#include "ux_attention_system.h"

/* Device and App specific includes */
#include "device_utils.h"
#include "fault_handlers.h"
#include "sln_cfg_file.h"
#include "sln_file_table.h"
#include "sln_reset.h"
#include "app_events.h"
#include "ffs_provision_cli.h"
#include "sln_shell.h"

/* Standard includes */
#include "time.h"
#include "stdbool.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define APP_VERSION_NUMBER \
    (uint32_t)(((APP_MAJ_VER & 0xFFU) << 24U) | ((APP_MIN_VER & 0xFFU) << 16U) | (APP_BLD_VER & 0xFFFFU))

#define MAX_VOLUME 100

#ifndef alexaACCOUNT_ID
#define alexaACCOUNT_ID "297605334355"
#endif

/*! @brief APP INIT Task settings */
#define APP_INIT_TASK_NAME       "APP_Init_Task"
#define APP_INIT_TASK_STACK_SIZE 2048
#define APP_INIT_TASK_PRIORITY   configTIMER_TASK_PRIORITY - 1

/*! @brief APP Task settings */
#define APP_TASK_NAME       "APP_Task"
#define APP_TASK_STACK_SIZE 1024
#define APP_TASK_PRIORITY   configTIMER_TASK_PRIORITY - 1

/*! @brief OFFLINE AUDIO Task settings */
#define OFFLINE_AUDIO_TASK_NAME       "Offline_Audio_Task"
#define OFFLINE_AUDIO_TASK_STACK_SIZE 512
#define OFFLINE_AUDIO_TASK_PRIORITY   configMAX_PRIORITIES - 2

/*! @brief PDM TO PCM Task settings */
#define PDM_TO_PCM_TASK_NAME       "pdm_to_pcm_task"
#define PDM_TO_PCM_TASK_STACK_SIZE 512
#define PDM_TO_PCM_TASK_PRIORITY   configMAX_PRIORITIES - 2

/*! @brief AUDIO PROC Task settings */
#define AUDIO_PROC_TASK_NAME       "Audio_proc_task"
#define AUDIO_PROC_TASK_STACK_SIZE 1536
#define AUDIO_PROC_TASK_PRIORITY   configMAX_PRIORITIES - 1

#define FFS_REG_CHECK_INTERVAL_MSEC (30000U) /* Check FFS registration state every 30 seconds */
#define FFS_REG_UGS_TIMEOUT_SEC     (900U)   /* UGS registration timeout interval 900 seconds */
#define APP_EVENT_QUEUE_SIZE        (5)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void vStreamerErrorCallback();

void offline_audio_task(void *arg);

static void vProcessAvsEvent(app_events_t *eventMessage);
static void vProcessAlertEvent(app_events_t *eventMessage);
static void vProcessAudioEvent(app_events_t *eventMessage, ais_mic_open_t *micOpen);

static void _initialize_audio_chain();
static void _configure_sound_and_volume();
static status_t _get_volume_from_flash();
static status_t _init_streamer();

/*******************************************************************************
 * Variables
 ******************************************************************************/

const uint64_t kEpochDayZero = 3752179200ULL; // 11/26/2018 0:00:00

__attribute__((section(".ocram_non_cacheable_bss"))) ais_handle_t aisHandle;
__attribute__((section(".ocram_non_cacheable_bss"))) ais_config_t aisConfig;

avs_sound_library_t avs_sounds;

streamer_handle_t streamerHandle;

ota_handle_t otaHandle;

TaskHandle_t appTaskHandle              = NULL;
TaskHandle_t appReconnectHandle         = NULL;
TaskHandle_t appInitTaskHandle          = NULL;
TaskHandle_t appInitDummyNullHandle     = NULL;
TaskHandle_t xAudioProcessingTaskHandle = NULL;
TaskHandle_t xPdmToPcmTaskHandle        = NULL;
TaskHandle_t xUXAttentionTaskHandle     = NULL;
TaskHandle_t xReconnectionTaskHandle    = NULL;
TaskHandle_t xOfflineAudioTaskHandle    = NULL;
TaskHandle_t mainTaskHandle             = NULL;

__attribute__((section(".ocram_non_cacheable_bss")))
StackType_t offline_audio_task_stack_buffer[OFFLINE_AUDIO_TASK_STACK_SIZE];
__attribute__((section(".ocram_non_cacheable_bss"))) StaticTask_t offline_audio_task_buffer;

EventGroupHandle_t s_offlineAudioEventGroup = NULL;

/* Clock sync timer handle */
TimerHandle_t aisClockSyncHandle;

/* Clock sync directive received check timer handle */
TimerHandle_t aisClockSyncCheckHandle;

/* FFS registration check timer handle */
TimerHandle_t ffsRegisterCheckHandle;

QueueHandle_t g_alertQueue;
QueueHandle_t g_eventQueue = NULL;

/* FFS registration state variables used for UGS button action */
bool g_ffsRunning       = false;
bool g_ugsButtonPressed = false;

/* Global PIT IRQ handle */
extern void timer_IRQHandler(void);
extern void UX_IRQHandler(void);

/* Note: The PIT_ClearStatusFlags is called in UX_IRQHandler/timer_IRQHandler */
void PIT_IRQHandler(void)
{
    /* kPIT_Chnl_0 IRQ is used by the UX attention system */
    if (PIT_GetStatusFlags(PIT, kPIT_Chnl_0) == kPIT_TimerFlag)
    {
        UX_IRQHandler();
    }

    /* kPIT_Chnl_1 IRQ is used by the iot timer */
    if (PIT_GetStatusFlags(PIT, kPIT_Chnl_1) == kPIT_TimerFlag)
    {
        timer_IRQHandler();
    }
}

/**
 * @brief Application task startup hook for applications using Wi-Fi. If you are not
 * using Wi-Fi, then start network dependent applications in the vApplicationIPNetorkEventHook
 * function. If you are not using Wi-Fi, this hook can be disabled by setting
 * configUSE_DAEMON_TASK_STARTUP_HOOK to 0.
 */
void vApplicationDaemonTaskStartupHook(void);

/*******************************************************************************
 * Code
 ******************************************************************************/
void appInit(void *arg);
void appTask(void *arg);

/**
 * @brief Tries to read the volume from flash, if it exists
 */
static status_t _get_volume_from_flash()
{
    uint32_t ret;
    sln_dev_cfg_t cfg       = DEFAULT_CFG_VALUES;
    uint32_t len            = 0;
    ais_app_data_t *appData = AIS_APP_GetAppData();

    ret = SLN_FLASH_MGMT_Read(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, &len);

    if (kStatus_Success != ret)
    {
        configPRINTF(("Warning, no saved Alert Volume!\r\n"));

        /* Set default stream volume to 6; this volume is used also
         * for opus sounds played through the streamer, so we can't afford
         * having an invalid value for it until receiving the right one from AIA */
        cfg.streamVolume = 60;

        /* The warning above is enough, just continue */
        ret = kStatus_Success;
    }

    /* Set the volume to the values from flash */
    appData->volume      = cfg.streamVolume;
    appData->alertVolume = cfg.alertVolume;

    return ret;
}

/**
 * @brief Initializes the streamer and creates the ring buffer
 */
static void _configure_sound_and_volume()
{
    char locale[6];

    _get_volume_from_flash();

    /* Initialize the Sound structure */
    AVS_SOUNDS_Init(&avs_sounds);

    memset(locale, 0, 6);
    SLN_AMAZON_WAKE_GetModelLocale((uint8_t *)locale);
    memcpy(&locale[2], "_", 1);

    /* Load the sounds based on the region in flash */
    AVS_SOUNDS_LoadSounds(locale, &avs_sounds);
}

/**
 * @brief Initializes the streamer and creates the ring buffer
 */
static status_t _init_streamer()
{
    uint32_t ret;
    /* Initialize the Streamer */
    STREAMER_Init();

    /* Setup the exception handler callback */
    streamerHandle.pvExceptionCallback = &vStreamerErrorCallback;

    ret = STREAMER_Create(&streamerHandle, AIS_DECODER_OPUS);

    if (ret != kStatus_Success)
    {
        configPRINTF(("STREAMER_Create failed\r\n"));
    }

    return ret;
}

/**
 * @brief Does AIA login for AIS on-boarding process
 * This will be started after FFS registration completes
 * and is only necessary to be performed once per device.
 */
static status_t appRegistration()
{
    int size;
    status_t status = kStatus_Success;
    char *alexaClientId;

#if USE_BASE64_UNIQUE_ID
    APP_GetUniqueID(&alexaClientId, true);
#else
    APP_GetHexUniqueID(&alexaClientId);
#endif /* USE_BASE64_UNIQUE_ID */

    aisConfig.awsAuthClientId = NULL;
    aisConfig.awsAuthToken    = NULL;

    strcpy(aisConfig.awsClientId, alexaClientId);
    strcpy(aisConfig.awsAccountId, alexaACCOUNT_ID);
    strcpy(aisConfig.awsEndpoint, clientcredentialMQTT_BROKER_ENDPOINT);

    vPortFree(alexaClientId);

    size = aceDeviceInfoDsHal_getEntrySize(ACE_DEVICE_INFO_CLIENT_ID);
    if (size > 0)
    {
        aisConfig.awsAuthClientId = pvPortMalloc((size + 1) * sizeof(char));
        if (aisConfig.awsAuthClientId != NULL)
        {
            aceDeviceInfoDsHal_getEntry(ACE_DEVICE_INFO_CLIENT_ID, aisConfig.awsAuthClientId, size + 1);
            aisConfig.awsAuthClientId[size] = 0;
        }
        else
        {
            status = kStatus_Fail;
        }
    }
    else
    {
        status = kStatus_Fail;
    }

    size = aceKeyValueDsHal_getValueSize("map.refresh_token");
    if (size > 0)
    {
        aisConfig.awsAuthToken = pvPortMalloc((size + 1) * sizeof(char));
        if (aisConfig.awsAuthToken != NULL)
        {
            aceKeyValueDsHal_get("map.refresh_token", aisConfig.awsAuthToken, size);
            aisConfig.awsAuthToken[size] = 0;
        }
        else
        {
            status = kStatus_Fail;
        }
    }
    else
    {
        status = kStatus_Fail;
    }

    if (status != kStatus_Success)
    {
        configPRINTF(("Failed getting AIS authorization credentials\r\n"));
    }
    else
    {
        AIS_SetConfig(&aisHandle, &aisConfig);

        /* Start AIS registration */
        status = AIS_Register(&aisHandle);
        if (kStatus_Success != status)
        {
            configPRINTF(("AIS registration failed\r\n"));
        }
        else
        {
            /* Delete the refresh token after a successful registration as
             * it will expire after next deregister from the Alexa app */
            aceKeyValueDsHal_remove("map.refresh_token");
        }
    }

    if (aisConfig.awsAuthClientId != NULL)
    {
        vPortFree(aisConfig.awsAuthClientId);
        aisConfig.awsAuthClientId = NULL;
    }

    if (aisConfig.awsAuthToken != NULL)
    {
        vPortFree(aisConfig.awsAuthToken);
        aisConfig.awsAuthToken = NULL;
    }

    return status;
}

/**
 * @brief Checks if LWA clientId and refresh token exist in KVS
 */
static bool checkAuthToken()
{
    int size;

    size = aceDeviceInfoDsHal_getEntrySize(ACE_DEVICE_INFO_CLIENT_ID);
    if (size > 0)
    {
        size = aceKeyValueDsHal_getValueSize("map.refresh_token");
    }

    return (size > 0) ? true : false;
}

/**
 * @brief Reads flash entry file and checks for secret (AIS) key
 * This shows if the device is registered with the AIA service
 */
static bool deviceRegistered()
{
    bool registered = false;
    ais_reg_config aisReg;

    /* Try read sharedKey */
    uint32_t secLen = sizeof(ais_reg_config);
    if (kStatus_Success == SLN_FLASH_MGMT_Read(AIS_REGISTRATION_INFO_FILE_NAME, (uint8_t *)&aisReg, &secLen))
    {
        for (uint32_t idx = 0; idx < AIS_SECRET_LENGTH; idx++)
        {
            if ((aisReg.sharedSecret[idx] != 0x00) && (aisReg.sharedSecret[idx] != 0xFF))
            {
                registered = true;
                break;
            }
        }
    }

    return registered;
}

static void ffs_registration_check_cb(TimerHandle_t xTimer)
{
    /* Wake up the main task */
    if (mainTaskHandle != NULL)
    {
        vTaskResume(mainTaskHandle);
    }
    else
    {
        configPRINTF(("Invalid appInit task handle, cannot resume demo app\r\n"));
    }
}

static ace_status_t ffs_registration_complete_cb(void *ctx, int success)
{
    ffs_registration_check_cb(0);
    return 0;
}

static ace_status_t ffs_provisioning_complete_cb(void *ctx)
{
    configPRINTF(("FFS  provisioning info exchanged"));
    return 0;
}

static ace_status_t ffs_provisioning_terminated_cb(void *ctx)
{
    /* In case the phone disconnects or the process fails form whatever reason
     * the ffs process will start again so we will set the LED in discovery mode */
    ux_attention_set_state(uxDiscovery);

    return ffs_registration_complete_cb(ctx, ACE_STATUS_GENERAL_ERROR);
}

static ace_status_t ffs_registration_stopped_cb(void *ctx, int reason)
{
    configPRINTF(("FFS registration stopped, reason: %d\r\n", reason));

    return 0;
}

static ace_status_t ffs_provisioner_connected_cb(void *ctx)
{
    ux_attention_set_state(uxFSSProvision);

    return 0;
}

static ace_status_t wifi_connect_state_cb(void *ctx, aceFfs_wifiState_t state, aceFfs_wifiReason_t reason)
{
    configPRINTF(("WiFi state: %d, reason: %d", state, reason));

    switch (state)
    {
        /* Update state controller state by calling stop registration */
        case ACE_FFS_WIFI_DISCONNECTED:
            break;

        default:
            break;
    }
    return 0;
}

/**
 * @brief Initializes services only needed before device has been registered
 *
 * Registration modules would be expected here, i.e. FFS, SoftAP
 */
static void init_pre_registration_services(void)
{
    ace_status_t status;

    /* Init FFS */
    aceFfs_config_t init_params = {.registration_complete     = ffs_registration_complete_cb,
                                   .provisioning_complete     = ffs_provisioning_complete_cb,
                                   .provisioning_terminated   = ffs_provisioning_terminated_cb,
                                   .stopped                   = ffs_registration_stopped_cb,
                                   .wifi_connect_state        = wifi_connect_state_cb,
                                   .ble_provisioner_connected = ffs_provisioner_connected_cb};

    if ((status = aceFfs_init(&init_params)) != ACE_STATUS_OK)
    {
        configPRINTF(("FFS init failed(%d)", status));
    }
}

static void init_services()
{
    status_t status = kStatus_Success;
    wifi_status_t status_wifi;

    /* initialize common libraries for the demo. */
    IotSdk_Init();

    /* Initialize WiFi and connect to the first profile */
    status_wifi = wifi_init();
    if (status_wifi != kWiFiStatus_Success)
    {
        configPRINTF(("[ERROR] wifi_init failed\r\n"));
        status = kStatus_Fail;
    }

    if (status == kStatus_Success)
    {
        status_wifi = wifi_connect(true);
        if (status_wifi != kWiFiStatus_Success)
        {
            configPRINTF(("[ERROR] wifi_connect failed\r\n"));
            status = kStatus_Fail;
        }
    }

    if (status == kStatus_Success)
    {
        if (deviceRegistered() == false)
        {
            /* FFS registration done, continue with AIS registration */
            status = appRegistration();
        }

        if (status == kStatus_Success)
        {
            if (xTaskCreate(appInit, APP_INIT_TASK_NAME, APP_INIT_TASK_STACK_SIZE, NULL, APP_INIT_TASK_PRIORITY,
                            &appInitTaskHandle) != pdPASS)
            {
                configPRINTF(("[ERROR] APP_Audio_Task task creation failed\r\n"));
                status = kStatus_Fail;
            }
        }
    }

    if (status != kStatus_Success)
    {
        sln_reset("[ERROR] Something went wrong during initialization.");
    }
}

static void wait_dha_provision(void)
{
    ace_status_t dha_status;
    int32_t dha_len;
    bool ffs_ready_flag_set = false;
    bool provision_done     = false;

    dha_status = aceDhaHal_open();
    assert(dha_status == ACE_STATUS_OK);

    dha_len = port_dha_get_cert_chain_size();
    if (dha_len <= 0)
    {
        /* Wait to be provisioned */
        while (1)
        {
            if (ffs_ready_flag_set == false)
            {
                FFSPROVISION_set_ready();

                configPRINTF(("No crt_chain for DHA found. Waiting for DHA provisioning...\r\n"));
                ffs_ready_flag_set = true;
            }

            provision_done = FFSPROVISION_get_done();
            if (provision_done)
            {
                configPRINTF(("DHA successfully provisioned\r\n"));
                break;
            }

            vTaskDelay(portTICK_PERIOD_MS * 500);
        }
    }

    configPRINTF(("crt_chain for DHA found. FFS process can start\r\n"));

    dha_status = aceDhaHal_close();
    assert(dha_status == ACE_STATUS_OK);
}

static void doFFS(aceFfs_mode_t ffs_mode)
{
    ace_status_t status;
    aceFfs_provisioningState_t state;
    aceFfs_provisioningState_t last_state = ACE_FFS_PROV_IDLE;
    status_t wifi_cred_status;
    uint32_t ffs_active_time_sec = 0;
    bool ffs_stopped             = false;
    ais_app_data_t *appData      = AIS_APP_GetAppData();

    wait_dha_provision();

    ux_attention_set_state(uxDiscovery);

    init_pre_registration_services();

    _init_streamer();

    /* Play the "Now in setup mode" sound */
    STREAMER_SetLocalSound(&streamerHandle, avs_sounds.sound_files->ffs_oobe_setup,
                           avs_sounds.sound_sizes->ffs_oobe_setup_len);
    STREAMER_Start(&streamerHandle);
    STREAMER_SetVolume(appData->volume);

    /* FFS is Fragile, need to wait until the audio has played as it
     * seems it doesn't like interrupts */
    while (STREAMER_IsPlaying(&streamerHandle))
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    status = aceFfs_start(ffs_mode);
    if (status != ACE_STATUS_OK)
    {
        configPRINTF(("Starting FFS failed, status: %d\r\n", status));
    }
    else
    {
        ffsRegisterCheckHandle = xTimerCreate("FFS registration check", FFS_REG_CHECK_INTERVAL_MSEC, pdTRUE, (void *)0,
                                              ffs_registration_check_cb);
        if (ffsRegisterCheckHandle == NULL)
        {
            configPRINTF(("Error creating FFS registration check timer\n"));
        }

        g_ffsRunning = true;

        /* Loop until FFS registers successfully */
        while (1)
        {
            if (g_ugsButtonPressed == false)
            {
                /* (re)start timer to wake up below */
                if (ffsRegisterCheckHandle != NULL)
                {
                    xTimerStart(ffsRegisterCheckHandle, 0);
                }
            }
            else
            {
                /* reset state variable after UGS button press with no action */
                g_ugsButtonPressed = false;
            }

            /* Wait for 30s interval or UGS button pressed */
            vTaskSuspend(NULL);

            if (g_ugsButtonPressed == false)
            {
                /* Increase FFS active time (+30sec) */
                ffs_active_time_sec += FFS_REG_CHECK_INTERVAL_MSEC / 1000;
            }

            /* Get FFS provisioning state */
            aceFfs_getState(&state);
            if (state == ACE_FFS_PROV_WAITING_FOR_PROVISIONER)
            {
                ux_attention_set_state(uxDiscovery);

                if (ffs_mode & ACE_FFS_MODE_UGS)
                {
                    /* Check if UGS beaconing timeout expired */
                    if (ffs_active_time_sec >= FFS_REG_UGS_TIMEOUT_SEC)
                    {
                        ffs_active_time_sec = 0;
                        ffs_mode &= ~ACE_FFS_MODE_UGS;

                        if (ffs_mode & ACE_FFS_MODE_ZTS)
                        {
                            /* Restart FFS in ZTS only mode */
                            status = aceFfs_start(ffs_mode);
                            if (status != ACE_STATUS_OK)
                            {
                                configPRINTF(("Restarting FFS failed, status: %d\r\n", status));
                                break;
                            }
                        }
                        else
                        {
                            /* Stop UGS here to respect timeout */
                            status = aceFfs_stop();
                            if (status != ACE_STATUS_OK)
                            {
                                configPRINTF(("Stopping FFS failed, status: %d\r\n", status));
                            }
                            ffs_stopped = true;
                        }
                    }
                }
                else if (ffs_mode & ACE_FFS_MODE_ZTS)
                {
                    /* Check if ZTS beaconing timeout expired or UGS button pressed */
                    if ((ffs_active_time_sec >= ACE_FFS_DEFAULT_ADVERTISING_TIMEOUT) || (g_ugsButtonPressed == true))
                    {
                        /* Stop ZTS here to respect timeout and re-enable UGS mode */
                        status = aceFfs_stop();
                        if (status != ACE_STATUS_OK)
                        {
                            configPRINTF(("Stopping FFS failed, status: %d\r\n", status));
                        }
                        ffs_stopped         = true;
                        ffs_active_time_sec = 0;

                        /* Leave ZTS pending if UGS button pressed */
                        if (g_ugsButtonPressed == false)
                        {
                            ffs_mode &= ~ACE_FFS_MODE_ZTS;
                        }
                    }
                }

                if (ffs_stopped)
                {
                    /* Wait indefinitely for UGS button press */
                    while (g_ugsButtonPressed == false)
                    {
                        vTaskDelay(portTICK_PERIOD_MS * 1000);
                    }
                    g_ugsButtonPressed = false;

                    ffs_mode |= ACE_FFS_MODE_UGS;
                    /* Restart in UGS only mode */
                    status = aceFfs_start(ACE_FFS_MODE_UGS);
                    if (status != ACE_STATUS_OK)
                    {
                        configPRINTF(("Starting FFS failed, status: %d\r\n", status));
                    }
                    else
                    {
                        ffs_stopped = false;
                    }
                }
            }

            if (state == ACE_FFS_PROV_DEVICE_REGISTERED)
            {
                wifi_cred_status = wifi_credentials_sync();
                if (wifi_cred_status != kStatus_Success)
                {
                    configPRINTF(("Could not sync WiFi credentials in KVS and Flash_management\r\n"));
                }
                /* Tear down FFS task - temporarily comment this due to unclean FFS stop */
                status = aceFfs_stop();
                if (status != ACE_STATUS_OK)
                {
                    configPRINTF(("Stopping FFS failed, status: %d\r\n", status));
                }
                configPRINTF(("FFS finished, registration successful\r\n"));

                /* Play the "device is ready" sound */
                STREAMER_SetLocalSound(&streamerHandle, avs_sounds.sound_files->system_your_alexa_device_is_ready,
                                       avs_sounds.sound_sizes->system_your_alexa_device_is_ready_len);
                STREAMER_Start(&streamerHandle);
                STREAMER_SetVolume(appData->volume);

                /* Wait prompt playback completed */
                while (STREAMER_IsPlaying(&streamerHandle))
                {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                break;
            }
            else if ((state == ACE_FFS_PROV_PROVISIONING_TERMINATED) || (state == ACE_FFS_PROV_STOPPED))
            {
                configPRINTF(("FFS terminated, registration result unknown\r\n"));
                break;
            }

            /* Show status message only on state change or button press */
            if ((state != last_state) || g_ugsButtonPressed)
            {
                configPRINTF(("FFS registration service active, current status is %d\r\n", state));
                last_state = state;
            }
        }
        if (ffsRegisterCheckHandle != NULL)
        {
            xTimerDelete(ffsRegisterCheckHandle, 0);
        }
    }

    sln_reset(NULL);
}

void ace_app_main(void *parameters)
{
    cJSON_Hooks hooks;
    status_t wifi_cred_present;

    /* Initialize cJSON library to use FreeRTOS heap memory management. */
    hooks.malloc_fn = pvPortMalloc;
    hooks.free_fn   = vPortFree;
    cJSON_InitHooks(&hooks);

    _configure_sound_and_volume();

    /* Initialize Amazon system libraries */
    if (SYSTEM_Init() != pdPASS)
    {
        configPRINTF(("SYSTEM_Init failed\r\n"));
        assert(0);
    }

    /* Initialize task handle to be used by FFS timeout callback */
    mainTaskHandle = xTaskGetCurrentTaskHandle();

    wifi_cred_present = wifi_credentials_present();
    /* Start full FFS if device not registered and credentials are missing */
    if (deviceRegistered() == false)
    {
        if ((wifi_cred_present != kStatus_Success) || (checkAuthToken() == false))
        {
            doFFS(ACE_FFS_MODE_UGS | ACE_FFS_MODE_ZTS);
        }
        else
        {
            /* We can continue to AIA(AIS) registration */
        }
    }
    else if (wifi_cred_present != kStatus_Success)
    {
        /* Start UGS only if WiFi credentials need to be updated */
        doFFS(ACE_FFS_MODE_UGS);
    }

    /* Continue with the Alexa demo application */
    init_services();

    vTaskDelete(NULL);
}

/* @brief Callback for any streamer error to send app notification
 * This is here to ensure there isn't tight coupling in the streamer module */
void vStreamerErrorCallback()
{
    app_events_t event;

    event.category = kAvsEvents;
    event.event    = kExceptionEncountered;

    if (g_eventQueue != NULL)
    {
        /* Notify app task */
        while (errQUEUE_FULL == xQueueSend(g_eventQueue, &event, (TickType_t)0))
        {
            vTaskDelay(portTICK_PERIOD_MS * 50);
            /* Send error response */
        }
    }
}

void vShadowSendUpdate(void *data)
{
    app_events_t event;

    event.category = kShadowEvents;
    event.event    = 0;
    event.data     = data;

    if (g_eventQueue != NULL)
    {
        /* Notify app task */
        while (errQUEUE_FULL == xQueueSend(g_eventQueue, &event, (TickType_t)0))
        {
            vTaskDelay(portTICK_PERIOD_MS * 50);
        }
    }
}

void ais_app_clock_cb(TimerHandle_t xTimer)
{
    app_events_t event;
    ais_app_data_t *appData = AIS_APP_GetAppData();
    appData->currTime += AIS_APP_TIMER_INTERVAL_SEC;

    uint64_t timeSinceSync = appData->currTime - appData->prevTime;

    if ((AIS_APP_TIME_SYNC_INTERVAL_SEC <= timeSinceSync) && (reconnection_task_get_state() == kStartState))
    {
        appData->prevTime = appData->currTime;
        event.category    = kAvsEvents;
        event.event       = kSyncClock;

        /* Notify app task */
        while (errQUEUE_FULL == xQueueSend(g_eventQueue, &event, (TickType_t)0))
        {
            vTaskDelay(portTICK_PERIOD_MS * 50);
            /* Send error response */
        }
    }

    if (kStartState != reconnection_task_get_state())
    {
        /* Begin check for timers that should have triggered (Offline use case) */
        for (uint32_t idx = 0; idx < AIS_APP_MAX_ALERT_COUNT; idx++)
        {
            uint64_t alertTime = AIS_Alerts_GetScheduledTime(idx);

            /* Any timer before this date is guaranteed to be invalid */
            if ((kEpochDayZero < alertTime) && ((alertTime != UINT64_MAX)))
            {
                if (alertTime < appData->currTime)
                {
                    /* Trigger alerts that aren't already triggered */
                    if (true == AIS_Alerts_GetIdleState(idx))
                    {
                        /* Trigger alert in UX system */
                        AIS_Alerts_Trigger(idx, true);

                        /* Mark this as triggered */
                        AIS_Alerts_MarkAsTriggeredOffline(idx);
                    }
                }
            }
        }
    }
}

void ais_app_sync_clock_check_cb(TimerHandle_t xTimer)
{
    ais_app_data_t *appData = AIS_APP_GetAppData();
    /* reconnect if sync clock directive not received in specified timeout */
    if (!appData->syncClockReceived)
    {
        if (reconnection_task_set_event(kReconnectAISDisconnect))
        {
            configPRINTF(("Sync clock directive not received in specified timeout, triggering a reconnection\r\n"));
        }
    }
}

BaseType_t prvMQTTCallback(void *pvUserData, const MQTTAgentCallbackParams_t *const pxCallbackParams)
{
    BaseType_t xReturn = pdFALSE;

    if (pxCallbackParams->xMQTTEvent == eMQTTAgentPublish)
    {
        configPRINTF(("MQTT Topic Received %s.\r\n", pxCallbackParams->u.xPublishData.pucTopic));
    }
    else if (pxCallbackParams->xMQTTEvent == eMQTTAgentDisconnect)
    {
        configPRINTF(("MQTT Disconnected %s.\r\n", clientcredentialMQTT_BROKER_ENDPOINT));

        /* Trigger the reconnection process */
        reconnection_task_set_event(kReconnectMQTTDisconnect);
    }

    return xReturn;
}

void appInit(void *arg)
{
    status_t ret            = 0;
    fault_ret_t fault_ret   = 0;
    ais_app_data_t *appData = AIS_APP_GetAppData();
    uint32_t len            = 0;
    sln_dev_cfg_t cfg       = DEFAULT_CFG_VALUES;
    bool restoreMicMute     = false;
    char *alexaCLIENT_ID    = NULL;

    configPRINTF(("*** Starting Alexa application ***\r\n"));

    /* Create the Event Queue consists of a size of the pointer to save memory */
    g_eventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(app_events_t));

    /* Setup the audio chain */
    _initialize_audio_chain();

    /* Check if the device was recovered from fault state */
    fault_ret = fault_context_print();
    if (kFaultRet_Error == fault_ret)
    {
        configPRINTF(("[ERR] Failed to get fault log from flash\r\n"));
    }

    /* Indicate to user we are now connecting */
    ux_attention_set_state(uxReconnecting);

#if USE_BASE64_UNIQUE_ID
    /* Amazon doesn't use special characters, need to remove "=" character */
    APP_GetUniqueID(&alexaCLIENT_ID, true);
#else
    APP_GetHexUniqueID(&alexaCLIENT_ID);
#endif /* USE_BASE64_UNIQUE_ID */

    if (eMQTTAgentSuccess != APP_MQTT_Connect(prvMQTTCallback))
    {
        goto error;
    }

    /* Initialize the shadow */
    if (ConfigShadowDemo(alexaCLIENT_ID, &vShadowSendUpdate) == EXIT_SUCCESS)
    {
        if (RunShadowDemo(true) == EXIT_SUCCESS)
        {
            configPRINTF(("SHADOW started\r\n"));
        }
        else
        {
            configPRINTF(("[WARNING] The shadow could not be started\r\n"));
        }
    }
    else
    {
        configPRINTF(("[WARNING] The shadow could not be configured\r\n"));
    }

    /* Setup task for playing offline audio */
    xTaskCreateStatic(offline_audio_task, OFFLINE_AUDIO_TASK_NAME, OFFLINE_AUDIO_TASK_STACK_SIZE, NULL,
                      OFFLINE_AUDIO_TASK_PRIORITY, offline_audio_task_stack_buffer, &offline_audio_task_buffer);

    xTaskCreate(appTask, APP_TASK_NAME, APP_TASK_STACK_SIZE, NULL, APP_TASK_PRIORITY, &appTaskHandle);

    ret = AIS_Init(&aisHandle, (void *)&streamerHandle);
    if (ret != kStatus_Success)
    {
        configPRINTF(("AIS_Init failed\r\n"));
        goto error;
    }

    /* Try read sharedKey */
    uint32_t secLen = sizeof(ais_reg_config);
    ret = SLN_FLASH_MGMT_Read(AIS_REGISTRATION_INFO_FILE_NAME, (uint8_t *)&aisConfig.registrationConfig, &secLen);
    if ((ret == SLN_FLASH_MGMT_ENOENTRY) || (ret == SLN_FLASH_MGMT_ENOENTRY2))
    {
        configPRINTF(("Invalid shared secret key!\r\n"));
        goto error;
    }
    else if (ret != SLN_FLASH_MGMT_OK)
    {
        configPRINTF(("Failed to read the shared secret key from storage!\r\n"));
        goto error;
    }

    strcpy(aisConfig.awsClientId, alexaCLIENT_ID);
    strcpy(aisConfig.awsAccountId, alexaACCOUNT_ID);
    strcpy(aisConfig.awsEndpoint, clientcredentialMQTT_BROKER_ENDPOINT);

    sprintf(aisConfig.firmwareVersion, "%ld", APP_VERSION_NUMBER);

    SLN_AMAZON_WAKE_GetModelLocale((uint8_t *)aisConfig.locale);

    aisConfig.maxAlertCount      = AIS_APP_MAX_ALERT_COUNT;
    aisConfig.speakerDecoder     = AIS_SPEAKER_DECODER_OPUS;
    aisConfig.speakerChannels    = 1;
    aisConfig.speakerBitrate     = 64000;
    aisConfig.speakerBitrateType = AIS_SPEAKER_BITRATE_CONSTANT;
    aisConfig.numberOfLocales    = AMZ_WW_NUMBER_OF_WW_MODELS;

    /* Making it local to avoid a pointer being kept around for not reason if declared up top */
    {
        amzn_ww_model_map *wwModelMap = SLN_AMAZON_GetSupportedLocales();

        aisConfig.localesSupported = (char **)pvPortMalloc(sizeof(char *) * aisConfig.numberOfLocales);

        for (uint32_t i = 0; i < aisConfig.numberOfLocales; i++)
        {
            aisConfig.localesSupported[i] = wwModelMap[i].model;
        }
    }

    AIS_SetConfig(&aisHandle, &aisConfig);

    /* Provide the ais app_task module with APP queue handle */
    AIS_APP_set_queue_handle(&g_eventQueue);

    /* Provide the shell task with APP queue handle */
    sln_shell_set_app_queue_handle(&g_eventQueue);

    /* Initialize AIS Alerts module */
    ret = AIS_Alerts_Init();

    if (kStatus_Success != ret)
    {
        configPRINTF(("Alerts Management failed to initialize!\r\n"));
        goto error;
    }

    /* Set alert volume from NVM */
    ret = SLN_FLASH_MGMT_Read(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, &len);

    if (kStatus_Success != ret)
    {
        configPRINTF(("Warning, no saved Alert Volume!\r\n"));

        /* We are setting volume to an invalid value. Customers can change this
         * if they want to set there own default volume */
        appData->volume = -1;
        ret             = kStatus_Success; // Just a warning, continue as needed
    }
    else
    {
        /* Set the volume to the value from flash */
        appData->volume      = cfg.streamVolume;
        appData->alertVolume = cfg.alertVolume;

        /* set restore mic mute accordingly if cfg file reading succeeded */
        if (kMicMuteModeOn == cfg.mic_mute_mode)
        {
            restoreMicMute = true;
        }
    }

    /* Set UX */
    ux_attention_set_state(uxConnected);

    configPRINTF(("Wait 1 second\r\n"));
    vTaskDelay(1000);

    ret = AIS_Connect(&aisHandle);
    if (ret != kStatus_Success)
    {
        configPRINTF(("AIS_Connect failed\r\n"));
        goto error;
    }

    /* Gather any alerts requires for deletion */
    alertTokenList_t alertsList;
    uint32_t alertsCount = 0;
#if (defined(AIS_SPEC_REV_325) && (AIS_SPEC_REV_325 == 1))
    AIS_Alerts_GetAlertsList(&alertsList, &alertsCount);
#else
    AIS_Alerts_GetDeletedList(&alertsList, &alertsCount);
#endif
    configPRINTF(("Found %d alerts ready to delete.\r\n", alertsCount));

    /* Send SynchronizeState to update on our status. */
    char *alertTokens[AIS_APP_MAX_ALERT_COUNT] = {0};
    for (uint32_t idx = 0; idx < alertsCount; idx++)
    {
        alertTokens[idx] = (char *)(&alertsList[idx]);
        configPRINTF(("Alert ready for delete: %s\r\n", alertTokens[idx]));
    }

    AIS_EventSynchronizeState(&aisHandle, appData->volume, (const char **)alertTokens, alertsCount);

    aisClockSyncHandle =
        xTimerCreate("AIS_Clock_Sync", AIS_APP_TIMER_INTERVAL_MSEC, pdTRUE, (void *)0, ais_app_clock_cb);

    xTimerStart(aisClockSyncHandle, 0);

    aisClockSyncCheckHandle = xTimerCreate("AIS_Clock_Sync_Check", AIS_APP_TIMER_SYNC_CLOCK_TIMEOUT_MSEC, pdFALSE,
                                           (void *)0, ais_app_sync_clock_check_cb);

    OTA_Init(&otaHandle, (const char *)alexaCLIENT_ID);
    ret = OTA_Connect(&otaHandle);
    if (ret != kStatus_Success)
    {
        configPRINTF(("OTA_Connect failed\r\n"));
        goto error;
    }

    reconnection_task_init(&xReconnectionTaskHandle);

    /* Restore mic mute mode from flash */
    if (restoreMicMute)
    {
        configPRINTF(("Restoring microphone mute mode ... \r\n"));
        /* set mute mode in a non persistent way, we already have it written in flash;
         * ignore error code, the function will simply perform variable copy */
        audio_processing_set_mic_mute(kMicMuteModeOn, false);
        ux_attention_set_state(uxMicOntoOff);
    }

    vPortFree(alexaCLIENT_ID);

    vTaskResume(appTaskHandle);

    configPRINTF(("Exiting App Init\r\n"));
    vTaskDelete(NULL);

    return;

error:
    ux_attention_sys_fault();
    sln_reset("App initialization failed");
}

static void _initialize_audio_chain()
{
    uint32_t ret, len;
    sln_dev_cfg_t cfg       = DEFAULT_CFG_VALUES;
    ais_app_data_t *appData = AIS_APP_GetAppData();

    _init_streamer();

    /* Provide the audio_processing_task module with queue handle */
    audio_processing_set_queue_handle(&g_eventQueue);

    int16_t *micBuf = pdm_to_pcm_get_pcm_output();
    audio_processing_set_mic_input_buffer(&micBuf);

    int16_t *ampBuf = pdm_to_pcm_get_amp_output();
    audio_processing_set_amp_input_buffer(&ampBuf);

    audio_processing_set_task_handle(&xAudioProcessingTaskHandle);

    /* Create audio processing task */
    if (xTaskCreate(audio_processing_task, AUDIO_PROC_TASK_NAME, AUDIO_PROC_TASK_STACK_SIZE, NULL,
                    AUDIO_PROC_TASK_PRIORITY, &xAudioProcessingTaskHandle) != pdPASS)
    {
        configPRINTF(("Task creation failed!.\r\n"));
        while (1)
            ;
    }

#if USE_TFA
    // Set loopback event bit for AMP
    SLN_AMP_SetLoopBackEventBits(pdm_to_pcm_get_amp_loopback_event());
    SLN_AMP_SetLoopBackErrorEventBits(pdm_to_pcm_get_amp_loopback_error_event());
#endif /* USE_TFA */

    // Set PDM to PCM config
    pcm_pcm_task_config_t config = {0};
    config.thisTask              = &xPdmToPcmTaskHandle;
    config.processingTask        = &xAudioProcessingTaskHandle;
    config.feedbackEnable        = SLN_AMP_LoopbackEnable;
    config.feedbackDisable       = SLN_AMP_LoopbackDisable;
#if USE_TFA
    config.feedbackInit   = SLN_AMP_Read;
    config.feedbackBuffer = (int16_t *)SLN_AMP_GetLoopBackBuffer();

#elif USE_MQS
    config.loopbackRingBuffer = SLN_AMP_GetRingBuffer();
    config.loopbackMutex      = SLN_AMP_GetLoopBackMutex();
    config.updateTimestamp    = SLN_AMP_UpdateTimestamp;
    config.getTimestamp       = SLN_AMP_GetTimestamp;
#endif /* USE_TFA */

    pcm_to_pcm_set_config(&config);

    /* Create pdm to pcm task */
    if (xTaskCreate(pdm_to_pcm_task, PDM_TO_PCM_TASK_NAME, PDM_TO_PCM_TASK_STACK_SIZE, NULL, PDM_TO_PCM_TASK_PRIORITY,
                    &xPdmToPcmTaskHandle) != pdPASS)
    {
        configPRINTF(("Task creation failed!.\r\n"));
        while (1)
            ;
    }

    /* Turn the mics off during boot */
    pdm_to_pcm_mics_off();

#if USE_TFA
    // Pass loopback event group to AMP
    EventGroupHandle_t ampLoopBackEventGroup = NULL;
    while (1)
    {
        ampLoopBackEventGroup = pdm_to_pcm_get_event_group();
        if (ampLoopBackEventGroup != NULL)
        {
            break;
        }
        vTaskDelay(1);
    }
    SLN_AMP_SetLoopBackEventGroup(&ampLoopBackEventGroup);
#endif /* USE_TFA */

    /* Get the volume from flash */
    ret = SLN_FLASH_MGMT_Read(DEVICE_CONFIG_FILE_NAME, (uint8_t *)&cfg, &len);

    if (kStatus_Success != ret)
    {
        configPRINTF(("Warning, no saved Alert Volume!\r\n"));

        /* We are setting volume to an invalid value. Customers can change this
         * if they want to set there own default volume */
        cfg.streamVolume = 60;
        ret              = kStatus_Success; // Just a warning, continue as needed
    }
    else
    {
        /* Set the volume to the value from flash */
        appData->volume      = cfg.streamVolume;
        appData->alertVolume = cfg.alertVolume;
    }
}

void appTask(void *arg)
{
    app_events_t eventMessage;
    ais_app_data_t *appData = AIS_APP_GetAppData();
    ais_mic_open_t micOpen;

    configPRINTF(("One second delay...\r\n"));
    vTaskDelay(1000);

    /* Queue needs to be initialized before AIS connect as this will cause a fault if a set or delete alert occurs */
    g_alertQueue = xQueueCreate(AIS_APP_MAX_ALERT_COUNT, AIS_MAX_ALERT_TOKEN_LEN_BYTES);

    vTaskSuspend(NULL);

    vTaskDelay(5);
    configPRINTF(("[App Task] Starting Available Heap: %d\r\n", xPortGetFreeHeapSize()));

    STREAMER_SetLocalSound(&streamerHandle, avs_sounds.core_sound_files->ui_wakesound,
                           avs_sounds.core_sound_sizes->ui_wakesound_len);
    STREAMER_Start(&streamerHandle);
    STREAMER_SetVolume(MAX_VOLUME);

    /* Give alerts the handle for UX system*/
    AIS_Alerts_SetUxHandle(&xUXAttentionTaskHandle);
    AIS_Alerts_SetUxTimerBit(uxTimer);
    AIS_Alerts_SetUxAlarmBit(uxAlarm);
    AIS_Alerts_SetUxReminderBit(uxReminder);

    /* Wait for a valid time */
    while (0 == appData->currTime)
    {
        vTaskDelay(10);
    }

    /* Re-sync alerts with service */
    int32_t updateStatus = -1;

    updateStatus = AIS_Alerts_UpdateState(appData->currTime);

    if (1 == updateStatus)
    {
        /* Gather any alerts requires for deletion */
        alertTokenList_t alertsList;
        uint32_t alertsCount = 0;
#if (defined(AIS_SPEC_REV_325) && (AIS_SPEC_REV_325 == 1))
        AIS_Alerts_GetAlertsList(&alertsList, &alertsCount);
#else
        AIS_Alerts_GetDeletedList(&alertsList, &alertsCount);
#endif

        /* Send SynchronizeState to update on our status. */
        char *alertTokens[AIS_APP_MAX_ALERT_COUNT] = {0};

        for (uint32_t idx = 0; idx < AIS_APP_MAX_ALERT_COUNT; idx++)
        {
            alertTokens[idx] = (char *)(&alertsList[idx]);
        }

        AIS_EventSynchronizeState(&aisHandle, appData->volume, (const char **)alertTokens, alertsCount);
    }

    micOpen.asr_profile = AIS_ASR_FAR_FIELD;
    micOpen.initiator   = AIS_INITIATOR_WAKEWORD;
    /* Initiator opaque type/token and wakeword indices not used for TAP. */
    micOpen.init_type  = NULL;
    micOpen.init_token = NULL;
    micOpen.wwStart    = 0;
    micOpen.wwEnd      = 0;

    /* Mics were turned off for startup, make sure they are turned on (unrelated to the mute state) */
    pdm_to_pcm_mics_on();

    while (1)
    {
        if (xQueueReceive(g_eventQueue, &eventMessage, portMAX_DELAY))
        {
            switch (eventMessage.category)
            {
                case kAudioEvents:
                    vProcessAudioEvent(&eventMessage, &micOpen);
                    break;
                case kAlertEvents:
                    vProcessAlertEvent(&eventMessage);
                    break;
                case kAvsEvents:
                    vProcessAvsEvent(&eventMessage);
                    break;
                case kShadowEvents:
                    SendUpdateShadowDemo(eventMessage.data);
                    break;
                default:
                    configPRINTF(("[APP Task] Unknown Event Category %d", eventMessage.category));
            }
        }
    }

    configPRINTF(("Error in APP_Task, shutting down\r\n"));
    vTaskDelete(NULL);
}

static void vProcessAudioEvent(app_events_t *eventMessage, ais_mic_open_t *micOpen)
{
    ais_app_data_t *appData = AIS_APP_GetAppData();

    if (kIdle == eventMessage->event)
    {
        /* Set UX attention state */
        ux_attention_set_state(uxIdle);
    }

    if (kWakeWordDetected == eventMessage->event)
    {
        if (reconnection_task_get_state() == kStartState)
        {
            if (!AIS_CheckState(&aisHandle, AIS_TASK_STATE_MICROPHONE))
            {
                /* Set UX attention state */
                ux_attention_set_state(uxListeningStart);

                if (AIS_CheckState(&aisHandle, AIS_TASK_STATE_SPEAKER))
                {
                    appData->bargein = true;

                    /* Force speaker state update here to close and send event */
                    AIS_StateSpeaker(&aisHandle);

                    /* Need to make sure there is a 50ms gap between close speaker and mic open for spec compliance */
                    vTaskDelay(portTICK_PERIOD_MS * 50);
                }

                /* Begin sending speech */
                micOpen->wwStart   = aisHandle.micStream.audio.audioData.offset + 16000;
                micOpen->wwEnd     = aisHandle.micStream.audio.audioData.offset + audio_processing_get_wake_word_end();
                micOpen->initiator = AIS_INITIATOR_WAKEWORD;
                AIS_EventMicrophoneOpened(&aisHandle, micOpen);

                /* We are now recoding */
                audio_processing_set_state(kWakeWordDetected);
            }
        }
        else if (reconnection_task_get_state() != kInitState)
        {
            /* Play the device lost connection audio cue */
            STREAMER_SetLocalSound(&streamerHandle, avs_sounds.sound_files->system_error_offline_lost_connection,
                                   avs_sounds.sound_sizes->system_error_offline_lost_connection_len);
            if (!STREAMER_IsPlaying(&streamerHandle))
            {
                STREAMER_Start(&streamerHandle);
                STREAMER_SetVolume(appData->volume);
            }
            ux_attention_set_state(uxDisconnected);
            audio_processing_set_state(kReconnect);
        }
    }

    if (kMicKeepOpen == eventMessage->event)
    {
        if (reconnection_task_get_state() == kStartState)
        {
            if (!AIS_CheckState(&aisHandle, AIS_TASK_STATE_MICROPHONE))
            {
                /* Set UX attention state */
                ux_attention_set_state(uxListeningStart);

                /* Being Sending speech */
                micOpen->initiator = AIS_INITIATOR_TAP;
                AIS_EventMicrophoneOpened(&aisHandle, micOpen);
            }
        }
        else
        {
            ux_attention_set_state(uxDisconnected);
            audio_processing_set_state(kReconnect);
        }
    }

    if (kMicRecording == eventMessage->event)
    {
        /* Set UX attention state and check if mute is active
         * so we don't confuse the user */
        if (audio_processing_get_mic_mute())
        {
            ux_attention_set_state(uxMicOntoOff);
        }
        else
        {
            ux_attention_set_state(uxListeningActive);
        }
    }

    if (kMicStop == eventMessage->event)
    {
        /* Set UX attention state */
        ux_attention_set_state(uxListeningEnd);
    }
}

static void vProcessAlertEvent(app_events_t *eventMessage)
{
    if (kNewAlertSet == eventMessage->event)
    {
        char token[AIS_MAX_ALERT_TOKEN_LEN_BYTES] = {0};
        if (xQueueReceive(g_alertQueue, token, (TickType_t)5))
        {
            AIS_EventSetAlertSucceeded(&aisHandle, token);
        }
    }

    if (kNewAlertFail == eventMessage->event)
    {
        char token[AIS_MAX_ALERT_TOKEN_LEN_BYTES] = {0};
        if (xQueueReceive(g_alertQueue, token, (TickType_t)5))
        {
            AIS_EventSetAlertFailed(&aisHandle, token);
        }
    }

    if (kAlertDelete == eventMessage->event)
    {
        char token[AIS_MAX_ALERT_TOKEN_LEN_BYTES] = {0};
        if (xQueueReceive(g_alertQueue, token, (TickType_t)5))
        {
            AIS_EventDeleteAlertSucceeded(&aisHandle, token);
        }
    }

    if (kFailDelete == eventMessage->event)
    {
        char token[AIS_MAX_ALERT_TOKEN_LEN_BYTES] = {0};
        if (xQueueReceive(g_alertQueue, token, (TickType_t)5))
        {
            AIS_EventDeleteAlertFailed(&aisHandle, token);
        }
    }

    if (kAlertOnlineTrigger == eventMessage->event)
    {
        char token[AIS_MAX_ALERT_TOKEN_LEN_BYTES] = {0};
        if (xQueueReceive(g_alertQueue, token, (TickType_t)5))
        {
            AIS_EventSetAlertSucceeded(&aisHandle, token);
        }
    }
}

static void vProcessAvsEvent(app_events_t *eventMessage)
{
    uint64_t offset         = 0;
    ais_app_data_t *appData = AIS_APP_GetAppData();

    if (kVolChanged == eventMessage->event)
    {
        if (appData->speakerOpen)
        {
            offset = appData->speakerOffsetWritten - STREAMER_GetQueuedRaw(aisHandle.audioPlayer);
        }

        AIS_EventVolumeChanged(&aisHandle, appData->volume, offset);
    }

    if (kAlertVolChanged == eventMessage->event)
    {
        /* Sent VolumeChanged back to service */
        AIS_EventAlertVolumeChanged(&aisHandle, appData->alertVolume);
    }

    if (kExceptionEncountered == eventMessage->event)
    {
        /* TODO: set appData->bargein to true in case we want to abort streaming */

        if (reconnection_task_get_state() == kStartState)
        {
            /* TODO: Create a queue for any exceptions that occur */
            const char *error = AIS_EXCEPTION_DESCRIPTION_STREAMER_MSG;
            AIS_EventExceptionEncountered(&aisHandle, error);
        }
    }

    if (kSyncClock == eventMessage->event)
    {
        /* reset directive received check timer */
        appData->syncClockReceived = false;
        if (xTimerReset(aisClockSyncCheckHandle, 0) != pdPASS)
        {
            configPRINTF(("xTimerReset failed\r\n"));
        }

        if (kStatus_Success != AIS_EventSynchronizeClock(&aisHandle))
        {
            configPRINTF(("Stopping SynchronizeClock timeout timer...\r\n"));

            if (xTimerStop(aisClockSyncCheckHandle, 0))
            {
                configPRINTF(("xTimerStop failed\r\n"));
            }
        }
    }

    if (kSetLocale == eventMessage->event)
    {
        char locale[6];

        /* Modify the locale only if the received locale is not in place at the moment */
        if (strncmp(aisConfig.locale, eventMessage->data, sizeof(aisConfig.locale)))
        {
            memcpy(aisConfig.locale, eventMessage->data, sizeof(aisConfig.locale));
            /* Need to shut off the mics to make sure audio isn't still being pushed */
            pdm_to_pcm_mics_off();

            /* Destroy the wake word engine, set the new locale and reinitialize */
            SLN_AMAZON_WAKE_Destroy();
            SLN_AMAZON_WAKE_SetModelLocale(aisConfig.locale);
            SLN_AMAZON_WAKE_Initialize();

            memcpy(locale, aisConfig.locale, 6);
            memcpy(&locale[2], "_", 1);

            /* Load the sounds based on the region in flash */
            AVS_SOUNDS_LoadSounds(locale, &avs_sounds);
            configPRINTF(("The language has been set to %s\r\n", locale));

            /* Turn the mics back on */
            pdm_to_pcm_mics_on();
        }

        /* Send the Locale Report response as per spec */
        AIS_EventLocalesReport(&aisHandle, aisConfig.locale);

        vPortFree(eventMessage->data);
    }

    if (kLocaleChanged == eventMessage->event)
    {
        char *locale = eventMessage->data;

        /* Send the locale change event to the cloud */
        AIS_EventLocalesChanged(&aisHandle, locale);

        /* Note: the event message data dose not need to be freed here,
         * it is statically allocated in the shell task */
    }
}

void offline_audio_task(void *arg)
{
    ais_app_data_t *appData = AIS_APP_GetAppData();
    /* Play offline alert audio if needed */

    EventBits_t offlineAudioEventBits;
    uint32_t alerting_state = 0;

    s_offlineAudioEventGroup = xEventGroupCreate();

    if (s_offlineAudioEventGroup != NULL)
    {
        while (1)
        {
            offlineAudioEventBits = xEventGroupWaitBits(s_offlineAudioEventGroup, OFFLINE_AUDIO_EVENT_MASK, pdTRUE,
                                                        pdFALSE, pdMS_TO_TICKS(1000));

            /* Choose what audio to play */
            if (OFFLINE_AUDIO_TIMER & offlineAudioEventBits)
            {
                configPRINTF(("Starting offline timer audio...\r\n"));
                /* Need to set the alerting state, put individual assignments as this may change depending on the type
                 * of alert This needs to be set to ensure that the resume UX shows alerting when offline */
                appData->state = AIS_STATE_ALERTING;
                alerting_state = OFFLINE_AUDIO_TIMER;

                STREAMER_SetLocalSound(&streamerHandle, avs_sounds.core_sound_files->system_alerts_melodic_02,
                                       avs_sounds.core_sound_sizes->system_alerts_melodic_02_len);
                if (!STREAMER_IsPlaying(&streamerHandle))
                {
                    STREAMER_Start(&streamerHandle);
                    STREAMER_SetVolume(appData->alertVolume);
                }
            }
            else if (OFFLINE_AUDIO_ALARM & offlineAudioEventBits)
            {
                configPRINTF(("Starting offline alarm audio...\r\n"));
                /* Need to set the alerting state, put individual assignments as this may change depending on the type
                 * of alert This needs to be set to ensure that the resume UX shows alerting when offline */
                appData->state = AIS_STATE_ALERTING;
                alerting_state = OFFLINE_AUDIO_ALARM;

                STREAMER_SetLocalSound(&streamerHandle, avs_sounds.core_sound_files->system_alerts_melodic_01,
                                       avs_sounds.core_sound_sizes->system_alerts_melodic_01_len);
                if (!STREAMER_IsPlaying(&streamerHandle))
                {
                    STREAMER_Start(&streamerHandle);
                    STREAMER_SetVolume(appData->alertVolume);
                }
            }
            else if (OFFLINE_AUDIO_RMNDR & offlineAudioEventBits)
            {
                configPRINTF(("Starting offline reminder audio...\r\n"));
                /* Need to set the alerting state, put individual assignments as this may change depending on the type
                 * of alert This needs to be set to ensure that the resume UX shows alerting when offline */
                appData->state = AIS_STATE_ALERTING;
                alerting_state = OFFLINE_AUDIO_RMNDR;

                STREAMER_SetLocalSound(&streamerHandle, avs_sounds.core_sound_files->alerts_notifications_01,
                                       avs_sounds.core_sound_sizes->alerts_notifications_01_len);
                if (!STREAMER_IsPlaying(&streamerHandle))
                {
                    STREAMER_Start(&streamerHandle);
                    STREAMER_SetVolume(appData->alertVolume);
                }
            }
            else if (OFFLINE_AUDIO_ABORT & offlineAudioEventBits)
            {
                configPRINTF(("...offline audio stopped!\r\n"));
                /* Set the state back to idle */
                appData->state = AIS_STATE_IDLE;
                alerting_state = 0;
                STREAMER_Stop(&streamerHandle);
            }
            else
            {
                /* Check to see if we are in an alerting state and if so, attempt to play the sound if a sound is not
                 * already playing */
                switch (alerting_state)
                {
                    case OFFLINE_AUDIO_TIMER:
                        STREAMER_SetLocalSound(&streamerHandle, avs_sounds.core_sound_files->system_alerts_melodic_02,
                                               avs_sounds.core_sound_sizes->system_alerts_melodic_02_len);

                        break;
                    case OFFLINE_AUDIO_ALARM:
                        STREAMER_SetLocalSound(&streamerHandle, avs_sounds.core_sound_files->system_alerts_melodic_01,
                                               avs_sounds.core_sound_sizes->system_alerts_melodic_01_len);

                        break;

                    case OFFLINE_AUDIO_RMNDR:
                        STREAMER_SetLocalSound(&streamerHandle, avs_sounds.core_sound_files->alerts_notifications_01,
                                               avs_sounds.core_sound_sizes->alerts_notifications_01_len);
                        break;
                }
                if (alerting_state != 0)
                {
                    /* Only start the streamer if it's not already playing */
                    if (!STREAMER_IsPlaying(&streamerHandle))
                    {
                        STREAMER_Start(&streamerHandle);
                        STREAMER_SetVolume(appData->alertVolume);
                    }
                }
            }
        }
    }
}

void vApplicationDaemonTaskStartupHook(void)
{
    /* start attention task */
    ux_attention_task_Init(&xUXAttentionTaskHandle);

    /* start blue and cyan boot up */
    ux_attention_set_state(uxBootUp);
}
