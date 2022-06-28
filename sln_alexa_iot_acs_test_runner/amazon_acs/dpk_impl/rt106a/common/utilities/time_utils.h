/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file. This file is a
 * Modifiable File, as defined in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */
/*******************************************************************************
 * time_utils.h
 *******************************************************************************
 */

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define NSEC_PER_USEC (1000)
#define USEC_PER_MSEC (1000)
#define MSEC_TO_USEC(x) ((x) * 1000)
#define USEC_TO_MSEC(x) ((x) / 1000)
#define SEC_TO_MIN(sec) ((sec) / SECONDS_IN_A_MINUTE)
#define MIN_TO_HOUR(min) ((min) / MINUTES_IN_A_HOUR)
#define MAX_EPOCH_PADDING 604800

#define SECONDS_IN_A_DAY (86400U)
#define SECONDS_IN_A_HOUR (3600U)
#define SECONDS_IN_A_MINUTE (60U)
#define SECONDS_IN_A_YEAR (31536000U)
#define MINUTES_IN_A_HOUR (60)
#define MINUTES_IN_A_DAY 1440
#define DAYS_IN_A_YEAR (365U)

#define SEC_TO_MSEC(sec) ((sec)*1000)
#define MSEC_TO_SEC(msec) ((msec) / 1000)
#define NSEC_TO_MSEC(_nsec) ((_nsec) / 1000000)

#define DAYS_TO_SECONDS(days) ((days)*SECONDS_IN_A_DAY)
#define HOURS_TO_SECONDS(hours) ((hours)*SECONDS_IN_A_HOUR)
#define MINUTES_TO_SECONDS(min) ((min)*SECONDS_IN_A_MINUTE)

/*!
 * @brief Initializes the time_util functions. time_utils_init
 * must be called to initialize, and use other functions provided in this file.
 *
 * @return  returns '0' for success or "-1" for an error.
 */
int time_utils_init(void);

/*!
 * @brief Get the time in microseconds since the begining of init.
 *
 * @return  returns microseconds from the start of time_utils
 */
uint64_t time_utils_get_microseconds(void);

/*!
 * @brief set the time of day in RTC to use the time functions
 *
 * @return  returns '0' for success or "-1" for an error.
 */
int time_utils_settime(const struct tm * timeVal);

/*!
 * @brief Get the current time of a clock.
 * @param [in] clk_id: The ID of the clock whose time you want to get; Now support:
 *      CLOCK_REALTIME — the standard POSIX-defined clock.System-wide realtime clock.
 * @param [out] tp: timespec structs, as specified in <time.h>:
 *    struct timespec {
 *        time_t   tv_sec;
 *        long     tv_nsec;
 *    };
 *
 * @return  returns '0' for success or "-1" for an error.
 */
int clock_gettime(clockid_t clk_id, struct timespec *tp);

/*!
 * @brief Get the current time in milliseconds.
 *
 * @return clock time in milliseconds or 0 if there were issues.
 */
uint64_t clock_gettime_ms(void);

/*!
 * @brief Ensures the clock is valid and set by an external entity (e.g. SNTP or
 * manually by the user).
 *
 * @return  returns true if valid.
 */
bool clock_isset(void);
