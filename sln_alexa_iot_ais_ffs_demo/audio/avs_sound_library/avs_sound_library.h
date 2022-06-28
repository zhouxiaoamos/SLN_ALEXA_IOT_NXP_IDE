/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _AVS_SOUND_LIBRARY_H_
#define _AVS_SOUND_LIBRARY_H_

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* AVS Locale Sound Declaration */

typedef struct _avs_sound_sizes
{
    uint32_t ffs_oobe_setup_len;
    uint32_t ffs_error_offline_captive_portal_len;
    uint32_t ffs_error_offline_no_profile_len;
    uint32_t ffs_error_offline_not_connected_to_internet_len;
    uint32_t ffs_error_offline_not_registered_len;
    uint32_t ffs_connected_to_device_app_len;
    uint32_t ffs_setup_mode_on_len;
    uint32_t ffs_setup_short_press_len;
    uint32_t setup_len;
    uint32_t system_connection_unsuccessful_len;
    uint32_t system_error_offline_captive_portal_len;
    uint32_t system_error_offline_lost_connection_len;
    uint32_t system_error_offline_no_profile_len;
    uint32_t system_error_offline_not_connected_to_the_internet_len;
    uint32_t system_error_offline_not_connected_to_service_else_len;
    uint32_t system_error_offline_not_registered_len;
    uint32_t system_error_wifi_password_len;
    uint32_t system_factory_data_reset_len;
    uint32_t system_connected_to_device_len;
    uint32_t system_setup_hello_len;
    uint32_t system_setup_mode_off_short_len;
    uint32_t system_setup_mode_off_len;
    uint32_t system_setup_mode_on_len;
    uint32_t system_setup_short_press_len;
    uint32_t system_ota_error_len;
    uint32_t system_ota_day0_len;
    uint32_t system_ota_in_progress_len;
    uint32_t system_standby_connect_len;
    uint32_t system_wifi_connected_len;
    uint32_t system_your_alexa_device_is_ready_len;
} avs_sound_sizes_t;

typedef struct _avs_sound_files
{
    uint8_t *ffs_oobe_setup;
    uint8_t *ffs_error_offline_captive_portal;
    uint8_t *ffs_error_offline_no_profile;
    uint8_t *ffs_error_offline_not_connected_to_internet;
    uint8_t *ffs_error_offline_not_registered;
    uint8_t *ffs_connected_to_device_app;
    uint8_t *ffs_setup_mode_on;
    uint8_t *ffs_setup_short_press;
    uint8_t *setup;
    uint8_t *system_connection_unsuccessful;
    uint8_t *system_error_offline_captive_portal;
    uint8_t *system_error_offline_lost_connection;
    uint8_t *system_error_offline_no_profile;
    uint8_t *system_error_offline_not_connected_to_the_internet;
    uint8_t *system_error_offline_not_connected_to_service_else;
    uint8_t *system_error_offline_not_registered;
    uint8_t *system_error_wifi_password;
    uint8_t *system_factory_data_reset;
    uint8_t *system_connected_to_device;
    uint8_t *system_setup_hello;
    uint8_t *system_setup_mode_off;
    uint8_t *system_setup_mode_off_short;
    uint8_t *system_setup_mode_on;
    uint8_t *system_setup_short_press;
    uint8_t *system_ota_error;
    uint8_t *system_ota_day0;
    uint8_t *system_ota_in_progress;
    uint8_t *system_standby_connect;
    uint8_t *system_wifi_connected;
    uint8_t *system_your_alexa_device_is_ready;
} avs_sound_files_t;

typedef struct _avs_sound_core_sizes
{
    uint8_t *alerts_notifications_01;
    uint8_t *alerts_notifications_03;
    uint8_t *state_bluetooth_connected;
    uint8_t *state_bluetooth_disconnected;
    uint8_t *state_privacy_mode_off;
    uint8_t *state_privacy_mode_on;
    uint8_t *system_alerts_melodic_01;
    uint8_t *system_alerts_melodic_01_short;
    uint8_t *system_alerts_melodic_02;
    uint8_t *system_alerts_melodic_02_short;
    uint8_t *ui_endpointing;
    uint8_t *ui_endpointing_touch;
    uint8_t *ui_wakesound;
    uint8_t *ui_wakesound_touch;
    uint8_t *utility_500ms_bank;
} avs_sound_core_files_t;

typedef struct _avs_sound_core_files
{
    uint32_t alerts_notifications_01_len;
    uint32_t alerts_notifications_03_len;
    uint32_t state_bluetooth_connected_len;
    uint32_t state_bluetooth_disconnected_len;
    uint32_t state_privacy_mode_off_len;
    uint32_t state_privacy_mode_on_len;
    uint32_t system_alerts_melodic_01_len;
    uint32_t system_alerts_melodic_01_short_len;
    uint32_t system_alerts_melodic_02_len;
    uint32_t system_alerts_melodic_02_short_len;
    uint32_t ui_endpointing_len;
    uint32_t ui_endpointing_touch_len;
    uint32_t ui_wakesound_len;
    uint32_t ui_wakesound_touch_len;
    uint32_t utility_500ms_bank_len;
} avs_sound_core_sizes_t;

typedef struct _avs_sound_library
{
    avs_sound_files_t *sound_files;
    avs_sound_sizes_t *sound_sizes;
    avs_sound_core_files_t *core_sound_files;
    avs_sound_core_sizes_t *core_sound_sizes;
} avs_sound_library_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Initializes the sound memory pointer
 *
 * @param *sounds Pointer to the sounds structure
 *
 */
void AVS_SOUNDS_Init(avs_sound_library_t *sounds);

/*!
 * @brief Frees the sound memory pointer
 *
 * @param *sounds Pointer to the sounds structure
 *
 */
void AVS_SOUNDS_Deinit(avs_sound_library_t *sounds);

/*!
 * @brief Deciphers the locale and loads the correct sound library into the pointer
 *
 * @param *locale The locale to be used
 * @param *sounds The pointer to the sounds to be loaded based on the locale
 *
 */
void AVS_SOUNDS_LoadSounds(char *locale, avs_sound_library_t *sounds);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _AVS_SOUND_LIBRARY_H_ */

/*******************************************************************************
 * API
 ******************************************************************************/
