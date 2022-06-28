/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef SLN_RESET_H_
#define SLN_RESET_H_

/**
 * @brief Print a reset message (default prefix + passed message), delay 1 second, reset the board.
 *
 * @param msg[in]                 Pointer to a message to be displayed.
 * @return                        Void.
 */
void sln_reset(const char *msg);

#endif /* SLN_RESET_H_ */
