/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _AVS_SOUND_LIBRARY_PROMPTS_H_
#define _AVS_SOUND_LIBRARY_PROMPTS_H_

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "avs_sound_library.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define ALERTS_NOTIFICATION_01_SIZE (23288)
#define ALERTS_NOTIFICATION_03_SIZE (28864)
#define STATE_BLUETOOTH_CONNECTED_SIZE (22140)
#define STATE_BLUETOOTH_DISCONNECTED_SIZE (20664)
#define STATE_PRIVACY_MODE_OFF_SIZE (7052)
#define STATE_PRIVACY_MODE_ON_SIZE (3116)
#define SYSTEM_ALERTS_MELODIC_01_SIZE (99384)
#define SYSTEM_ALERTS_MELODIC_01_SHORT_SIZE (32472)
#define SYSTEM_ALERTS_MELODIC_02_SIZE (99384)
#define SYSTEM_ALERTS_MELODIC_02_SHORT_SIZE (32472)
#define UI_ENDPOINTING_SIZE (11152)
#define UI_ENDPOINTING_TOUCH_SIZE (11152)
#define UI_WAKESOUND_SIZE (10332)
#define UI_WAKESOUND_TOUCH_SIZE (7544)
#define UTILITY_500MS_BLANK_SIZE (7872)

/*******************************************************************************
 * Global Vars
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Sets up the System Sound pointer
 *
 * @param *sounds A pointer to the sounds structure
 *
 */
void AVS_SOUNDS_Load_System_Sounds(avs_sound_library_t *sounds);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _AVS_SOUND_LIBRARY_PROMPTS_H_ */