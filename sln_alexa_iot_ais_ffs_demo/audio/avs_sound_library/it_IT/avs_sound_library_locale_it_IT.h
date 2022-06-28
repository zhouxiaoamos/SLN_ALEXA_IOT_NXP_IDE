/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _AVS_SOUND_LIBRARY_IT_IT_H_
#define _AVS_SOUND_LIBRARY_IT_IT_H_

/*******************************************************************************
 * Includes
 ******************************************************************************/
/* Includes for Locale Header Files */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

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
 * @brief Passes audio data to the wake word to be processed
 *
 * @param *locale The region to be used to load the sounds
 * @param *sounds The region of the sounds
 *
 */
void AVS_SOUNDS_Load_it_IT_Sounds(char *locale, avs_sound_library_t *sounds);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _AVS_SOUND_LIBRARY_IT_IT_H_ */
