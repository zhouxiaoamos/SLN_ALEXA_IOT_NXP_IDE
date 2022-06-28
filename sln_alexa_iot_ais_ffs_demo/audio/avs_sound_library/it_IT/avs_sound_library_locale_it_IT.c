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
#include "avs_sound_library.h"

#include "avs_sound_library_locale_it_IT.h"
#include "avs_sound_library_locale_it_IT_sounds.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

static const avs_sound_sizes_t sound_sizes = {IT_IT_AEW_FFS_SYSTEM_OOBE_SETUP_SIZE, IT_IT_AEW_FFS_SYSTEM_PROMPT_ERROR_OFFLINE_CAPTIVE_PORTAL_SIZE, IT_IT_AEW_FFS_SYSTEM_PROMPT_ERROR_OFFLINE_NO_PROFILE_SIZE, IT_IT_AEW_FFS_SYSTEM_PROMPT_ERROR_OFFLINE_NOT_CONNECTED_TO_INTERNET_SIZE, IT_IT_AEW_FFS_SYSTEM_PROMPT_ERROR_OFFLINE_NOT_REGISTERED_SIZE, IT_IT_AEW_FFS_SYSTEM_PROMPT_OOBE_CONNECTED_TO_DEVICE_APP_SIZE, IT_IT_AEW_FFS_SYSTEM_PROMPT_OOBE_SETUP_MODE_ON_SIZE, IT_IT_AEW_FFS_SYSTEM_PROMPT_OOBE_SETUP_SHORT_PRESS_SIZE, IT_IT_AEW_SYSTEM_OOBE_SETUP_SIZE, IT_IT_AEW_SYSTEM_PROMPT_CONNECTION_UNSUCCESSFUL_SIZE, IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_CAPTIVE_PORTAL_SIZE, IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_LOST_CONNECTION_SIZE, IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_NO_PROFILE_SIZE, IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_NOT_CONNECTED_TO_INTERNET_SIZE, IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_NOT_CONNECTED_TO_SERVICE_ELSE_SIZE, IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_NOT_REGISTERED_SIZE, IT_IT_AEW_SYSTEM_PROMPT_ERROR_WIFI_PASSWORD_SIZE, IT_IT_AEW_SYSTEM_PROMPT_FACTORY_DATA_RESET_SIZE, IT_IT_AEW_SYSTEM_PROMPT_OOBE_CONNECTED_TO_DEVICE_AP_SIZE, IT_IT_AEW_SYSTEM_PROMPT_OOBE_SETUP_HELLO_SIZE, IT_IT_AEW_SYSTEM_PROMPT_OOBE_SETUP_MODE_OFF_SIZE, IT_IT_AEW_SYSTEM_PROMPT_OOBE_SETUP_MODE_OFF_SHORT_SIZE, IT_IT_AEW_SYSTEM_PROMPT_OOBE_SETUP_MODE_ON_SIZE, IT_IT_AEW_SYSTEM_PROMPT_OOBE_SETUP_SHORT_PRESS_SIZE, IT_IT_AEW_SYSTEM_PROMPT_OTA_CRITICAL_ERROR_SIZE, IT_IT_AEW_SYSTEM_PROMPT_OTA_DAY0_SIZE, IT_IT_AEW_SYSTEM_PROMPT_OTA_IN_PROGRESS_SIZE, IT_IT_AEW_SYSTEM_PROMPT_STANDBY_CONNECT_SIZE, IT_IT_AEW_SYSTEM_PROMPT_WIFI_CONNECTED_SIZE, IT_IT_AEW_SYSTEM_PROMPT_YOUR_ALEXA_DEVICE_IS_READY_SIZE};
extern const char it_it_aew_ffs_system_oobe_setup_opus[IT_IT_AEW_FFS_SYSTEM_OOBE_SETUP_SIZE];
extern const char it_it_aew_ffs_system_prompt_error_offline_captive_portal_opus[IT_IT_AEW_FFS_SYSTEM_PROMPT_ERROR_OFFLINE_CAPTIVE_PORTAL_SIZE];
extern const char it_it_aew_ffs_system_prompt_error_offline_no_profile_opus[IT_IT_AEW_FFS_SYSTEM_PROMPT_ERROR_OFFLINE_NO_PROFILE_SIZE];
extern const char it_it_aew_ffs_system_prompt_error_offline_not_connected_to_internet_opus[IT_IT_AEW_FFS_SYSTEM_PROMPT_ERROR_OFFLINE_NOT_CONNECTED_TO_INTERNET_SIZE];
extern const char it_it_aew_ffs_system_prompt_error_offline_not_registered_opus[IT_IT_AEW_FFS_SYSTEM_PROMPT_ERROR_OFFLINE_NOT_REGISTERED_SIZE];
extern const char it_it_aew_ffs_system_prompt_oobe_connected_to_device_app_opus[IT_IT_AEW_FFS_SYSTEM_PROMPT_OOBE_CONNECTED_TO_DEVICE_APP_SIZE];
extern const char it_it_aew_ffs_system_prompt_oobe_setup_mode_on_opus[IT_IT_AEW_FFS_SYSTEM_PROMPT_OOBE_SETUP_MODE_ON_SIZE];
extern const char it_it_aew_ffs_system_prompt_oobe_setup_short_press_opus[IT_IT_AEW_FFS_SYSTEM_PROMPT_OOBE_SETUP_SHORT_PRESS_SIZE];
extern const char it_it_aew_system_oobe_setup_opus[IT_IT_AEW_SYSTEM_OOBE_SETUP_SIZE];
extern const char it_it_aew_system_prompt_connection_unsuccessful_opus[IT_IT_AEW_SYSTEM_PROMPT_CONNECTION_UNSUCCESSFUL_SIZE];
extern const char it_it_aew_system_prompt_error_offline_captive_portal_opus[IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_CAPTIVE_PORTAL_SIZE];
extern const char it_it_aew_system_prompt_error_offline_lost_connection_opus[IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_LOST_CONNECTION_SIZE];
extern const char it_it_aew_system_prompt_error_offline_no_profile_opus[IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_NO_PROFILE_SIZE];
extern const char it_it_aew_system_prompt_error_offline_not_connected_to_internet_opus[IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_NOT_CONNECTED_TO_INTERNET_SIZE];
extern const char it_it_aew_system_prompt_error_offline_not_connected_to_service_else_opus[IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_NOT_CONNECTED_TO_SERVICE_ELSE_SIZE];
extern const char it_it_aew_system_prompt_error_offline_not_registered_opus[IT_IT_AEW_SYSTEM_PROMPT_ERROR_OFFLINE_NOT_REGISTERED_SIZE];
extern const char it_it_aew_system_prompt_error_wifi_password_opus[IT_IT_AEW_SYSTEM_PROMPT_ERROR_WIFI_PASSWORD_SIZE];
extern const char it_it_aew_system_prompt_factory_data_reset_opus[IT_IT_AEW_SYSTEM_PROMPT_FACTORY_DATA_RESET_SIZE];
extern const char it_it_aew_system_prompt_oobe_connected_to_device_ap_opus[IT_IT_AEW_SYSTEM_PROMPT_OOBE_CONNECTED_TO_DEVICE_AP_SIZE];
extern const char it_it_aew_system_prompt_oobe_setup_hello_opus[IT_IT_AEW_SYSTEM_PROMPT_OOBE_SETUP_HELLO_SIZE];
extern const char it_it_aew_system_prompt_oobe_setup_mode_off_opus[IT_IT_AEW_SYSTEM_PROMPT_OOBE_SETUP_MODE_OFF_SIZE];
extern const char it_it_aew_system_prompt_oobe_setup_mode_off_short_opus[IT_IT_AEW_SYSTEM_PROMPT_OOBE_SETUP_MODE_OFF_SHORT_SIZE];
extern const char it_it_aew_system_prompt_oobe_setup_mode_on_opus[IT_IT_AEW_SYSTEM_PROMPT_OOBE_SETUP_MODE_ON_SIZE];
extern const char it_it_aew_system_prompt_oobe_setup_short_press_opus[IT_IT_AEW_SYSTEM_PROMPT_OOBE_SETUP_SHORT_PRESS_SIZE];
extern const char it_it_aew_system_prompt_ota_critical_error_opus[IT_IT_AEW_SYSTEM_PROMPT_OTA_CRITICAL_ERROR_SIZE];
extern const char it_it_aew_system_prompt_ota_day0_opus[IT_IT_AEW_SYSTEM_PROMPT_OTA_DAY0_SIZE];
extern const char it_it_aew_system_prompt_ota_in_progress_opus[IT_IT_AEW_SYSTEM_PROMPT_OTA_IN_PROGRESS_SIZE];
extern const char it_it_aew_system_prompt_standby_connect_opus[IT_IT_AEW_SYSTEM_PROMPT_STANDBY_CONNECT_SIZE];
extern const char it_it_aew_system_prompt_wifi_connected_opus[IT_IT_AEW_SYSTEM_PROMPT_WIFI_CONNECTED_SIZE];
extern const char it_it_aew_system_prompt_your_alexa_device_is_ready_opus[IT_IT_AEW_SYSTEM_PROMPT_YOUR_ALEXA_DEVICE_IS_READY_SIZE];

/*******************************************************************************
 * Global Vars
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

void AVS_SOUNDS_Load_it_IT_Sounds(char *locale, avs_sound_library_t *sounds)
{
    sounds->sound_sizes = (avs_sound_sizes_t *) &sound_sizes;

    sounds->sound_files->ffs_oobe_setup = (uint8_t *) &it_it_aew_ffs_system_oobe_setup_opus;
#if 0
    sounds->sound_files->ffs_error_offline_captive_portal = (uint8_t *) it_it_aew_ffs_system_prompt_error_offline_captive_portal_opus;
#endif
#if 0
    sounds->sound_files->ffs_error_offline_no_profile = (uint8_t *) it_it_aew_ffs_system_prompt_error_offline_no_profile_opus;
#endif
#if 0
    sounds->sound_files->ffs_error_offline_not_connected_to_internet = (uint8_t *) it_it_aew_ffs_system_prompt_error_offline_not_connected_to_internet_opus;
#endif
#if 0
    sounds->sound_files->ffs_error_offline_not_registered = (uint8_t *) it_it_aew_ffs_system_prompt_error_offline_not_registered_opus;
#endif
#if 0
    sounds->sound_files->ffs_connected_to_device_app = (uint8_t *) it_it_aew_ffs_system_prompt_oobe_connected_to_device_app_opus;
#endif
#if 0
    sounds->sound_files->ffs_setup_mode_on = (uint8_t *) it_it_aew_ffs_system_prompt_oobe_setup_mode_on_opus;
#endif
#if 0
    sounds->sound_files->ffs_setup_short_press = (uint8_t *) it_it_aew_ffs_system_prompt_oobe_setup_short_press_opus;
#endif
#if 0
    sounds->sound_files->setup = (uint8_t *) it_it_aew_system_oobe_setup_opus;
#endif
#if 0
    sounds->sound_files->system_connection_unsuccessful = (uint8_t *) it_it_aew_system_prompt_connection_unsuccessful_opus;
#endif
#if 0
    sounds->sound_files->system_error_offline_captive_portal = (uint8_t *) it_it_aew_system_prompt_error_offline_captive_portal_opus;
#endif
    sounds->sound_files->system_error_offline_lost_connection = (uint8_t *) it_it_aew_system_prompt_error_offline_lost_connection_opus;
#if 0
    sounds->sound_files->system_error_offline_no_profile = (uint8_t *) it_it_aew_system_prompt_error_offline_no_profile_opus;
#endif
#if 0
    sounds->sound_files->system_error_offline_not_connected_to_the_internet = (uint8_t *) it_it_aew_system_prompt_error_offline_not_connected_to_internet_opus;
#endif
#if 0
    sounds->sound_files->system_error_offline_not_connected_to_service_else = (uint8_t *) it_it_aew_system_prompt_error_offline_not_connected_to_service_else_opus;
#endif
#if 0
    sounds->sound_files->system_error_offline_not_registered = (uint8_t *) it_it_aew_system_prompt_error_offline_not_registered_opus;
#endif
#if 0
    sounds->sound_files->system_error_wifi_password = (uint8_t *) it_it_aew_system_prompt_error_wifi_password_opus;
#endif
#if 0
    sounds->sound_files->system_factory_data_reset = (uint8_t *) it_it_aew_system_prompt_factory_data_reset_opus;
#endif
#if 0
    sounds->sound_files->system_connected_to_device = (uint8_t *) it_it_aew_system_prompt_oobe_connected_to_device_ap_opus;
#endif
#if 0
    sounds->sound_files->system_setup_hello = (uint8_t *) it_it_aew_system_prompt_oobe_setup_hello_opus;
#endif
#if 0
    sounds->sound_files->system_setup_mode_off_short = (uint8_t *) it_it_aew_system_prompt_oobe_setup_mode_off_opus;
#endif
#if 0
    sounds->sound_files->system_setup_mode_off = (uint8_t *) it_it_aew_system_prompt_oobe_setup_mode_off_short_opus;
#endif
#if 0
    sounds->sound_files->system_setup_mode_on = (uint8_t *) it_it_aew_system_prompt_oobe_setup_mode_on_opus;
#endif
#if 0
    sounds->sound_files->system_setup_short_press = (uint8_t *) it_it_aew_system_prompt_oobe_setup_short_press_opus;
#endif
#if 0
    sounds->sound_files->system_ota_error = (uint8_t *) it_it_aew_system_prompt_ota_critical_error_opus;
#endif
#if 0
    sounds->sound_files->system_ota_day0 = (uint8_t *) it_it_aew_system_prompt_ota_day0_opus;
#endif
#if 0
    sounds->sound_files->system_ota_in_progress = (uint8_t *) it_it_aew_system_prompt_ota_in_progress_opus;
#endif
#if 0
    sounds->sound_files->system_standby_connect = (uint8_t *) it_it_aew_system_prompt_standby_connect_opus;
#endif
#if 0
    sounds->sound_files->system_wifi_connected = (uint8_t *) it_it_aew_system_prompt_wifi_connected_opus;
#endif
    sounds->sound_files->system_your_alexa_device_is_ready = (uint8_t *) it_it_aew_system_prompt_your_alexa_device_is_ready_opus;
}
