/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef SLN_CONVERT_H_
#define SLN_CONVERT_H_

#include <stdint.h>

/* Biggest uint64_t number has 20 digits */
#define LONG_STR_BUFSIZE 21

/**
 * @brief Convert an uint64_t to a string in order to be printed.
 *
 * @param number[in]              The number to be converted.
 * @param number[inout]           Pointer where the converted string to be stored.
 *                                Should be a buffer of at least LONG_STR_BUFSIZE size.
 *
 * @return                        Void.
 */
void sln_long_to_str(uint64_t number, char *str);

#endif /* SLN_CONVERT_H_ */
