/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/**
 * @file ais_buttons.h
 * @brief This file is used for the buttons tasks implementation
 */
#ifndef AIS_BUTTONS_H_
#define AIS_BUTTONS_H_

/**
 * @brief This task handles buttons actions on the Alexa board
 *
 * Mute/Unmute:                    SW1 Held and released for 0 - 3 seconds
 * WiFi Credentials reset:         SW1 Held and released for 5 - 10 seconds
 * Factory Reset:                  SW1 Held and released more than 10 seconds
 * Alexa Action:                   SW2 Held and released for 0 - 3 seconds
 * Volume Up:                      SW2 Held while SW1 is pressed and release
 *                                    (Multi press and release of SW supported)
 * Volume Down:                    SW1 Held while SW2 is pressed and release
 *                                    (Multi press and release of SW supported)
 */
void button_task(void *arg);

#endif /* AIS_BUTTONS_H_ */
