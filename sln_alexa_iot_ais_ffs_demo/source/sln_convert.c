/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdio.h>
#include <string.h>
#include "sln_convert.h"

void sln_long_to_str(uint64_t number, char *str)
{
#ifdef __NEWLIB__
    char buf[LONG_STR_BUFSIZE] = {0};
    char *end_buf              = NULL;
    uint8_t characters         = 0;
    uint8_t rest;

    if (str != NULL)
    {
        end_buf = &buf[LONG_STR_BUFSIZE - 1];

        *end_buf-- = 0;
        characters++;

        do
        {
            rest   = number % 10;
            number = number / 10;

            *end_buf-- = '0' + rest;
            characters++;
        } while ((number != 0) && (characters != LONG_STR_BUFSIZE));

        memcpy(str, ++end_buf, characters);
    }
#else
    if (str != NULL)
    {
        snprintf(str, LONG_STR_BUFSIZE, "%llu", number);
    }
#endif
}
