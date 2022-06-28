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
/*
 * @File: time_utils_cli.c
 */

/*
 * Purpose: Command line options for logging
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ace/aceCli.h>
#include <sys/time.h>
#include "time_utils.h"
#include "FreeRTOS.h"
#include "task.h"

#define TIME_UTILS_TIME_FORMAT  "%Y-%m-%dT%H:%M:%S"

#define TIME_TEST_ERROR_ALLOWANCE   5 /* percentage error in time allowed */
#define TIME_TEST_CHECK_RESULT(x, y)    ( \
    (y) <= ((x) + (x) * TIME_TEST_ERROR_ALLOWANCE/100) &&   \
    (y) >= ((x) - (x) * TIME_TEST_ERROR_ALLOWANCE/100)      \
    )
#define TIME_TEST_DELAY_MS      (5000)

extern time_t getRtcTimeSec(void);

static ace_status_t cli_time_get(int32_t len, const char *param[])
{
    time_t curtime = time(NULL);
    struct timespec tp;
    char date_time[32];
    tp.tv_sec = 0;
    clock_gettime(CLOCK_REALTIME, &tp);
    if (curtime <= 0) {
        printf("Unable to get current time\n");
        return ACE_STATUS_OK;
    }

    struct tm utctime;
    gmtime_r(&curtime, &utctime);

    if (!clock_isset()) {
        printf("Warning time will not match sntp, needs resync\n");
    }

    if (curtime != tp.tv_sec) {
        printf("Warning time and clock_gettime disagree on seconds field, should not happen\n");
    }

    strftime(date_time, sizeof(date_time), TIME_UTILS_TIME_FORMAT, &utctime);
    printf("Current utc time is %s. (epoch: %d vs time: %d.%ld)\n",
              date_time, (int)curtime, (int)tp.tv_sec, tp.tv_nsec);

    struct tm loctime;
    localtime_r(&curtime, &loctime);

    printf("Current local time is %d:%d:%d\n", loctime.tm_hour, loctime.tm_min, loctime.tm_sec);

    return ACE_STATUS_OK;
}

static ace_status_t cli_time_set(int32_t len, const char *param[])
{
    struct tm tv = {0};
    time_t t;

    if (len != 1) {
        return ACE_STATUS_BAD_PARAM;
    }

    strptime(param[0], TIME_UTILS_TIME_FORMAT, &tv);
    if (time_utils_settime(&tv) != 0) {
        printf("failed to settime\n");
        return ACE_STATUS_GENERAL_ERROR;
    }

    return ACE_STATUS_OK;
}

static ace_status_t cli_time_test(int32_t len, const char *param[])
{
    struct timeval start_tv;
    struct timeval end_tv;
    time_t start_time = 0;
    time_t end_time = 0;
    int start, end;
    bool passed = 0;
    unsigned long start_tm = 0;
    unsigned long end_tm = 0;
    int delay = TIME_TEST_DELAY_MS;
    int start_rtcSeconds = 0;
    int end_rtcSeconds = 0;
    int start_clock = 0;
    int end_clock = 0;

    if (len == 1) {
        delay = SEC_TO_MSEC(strtoul(param[0], NULL, 10));
    }
    gettimeofday(&start_tv, NULL);
    time(&start_time);
    start_rtcSeconds = getRtcTimeSec();
    start_clock = clock();

    start = xTaskGetTickCount();
    usleep(MSEC_TO_USEC(delay));
    end = xTaskGetTickCount();

    gettimeofday(&end_tv, NULL);
    time(&end_time);
    end_rtcSeconds = getRtcTimeSec();
    end_clock = clock();

    start_tm = (unsigned long)(((unsigned long long)SEC_TO_MSEC(start_tv.tv_sec)) +
            USEC_TO_MSEC(start_tv.tv_usec));
    end_tm = (unsigned long) (((unsigned long long)SEC_TO_MSEC(end_tv.tv_sec)) +
            USEC_TO_MSEC(end_tv.tv_usec));

    printf("tick rate: %lu\n", configTICK_RATE_HZ);

    if (TIME_TEST_CHECK_RESULT(delay, (int)(end - start)) &&
        TIME_TEST_CHECK_RESULT(delay, (int)(end_tm - start_tm)) &&
        TIME_TEST_CHECK_RESULT(delay, (int)(end_clock - start_clock)) &&
        TIME_TEST_CHECK_RESULT(delay, (int)((end_rtcSeconds - start_rtcSeconds) * configTICK_RATE_HZ)) &&
        TIME_TEST_CHECK_RESULT(delay, (int)((end_time - start_time) * configTICK_RATE_HZ))) {
        passed = 1;
    }

    printf("Time Test Result: %s\n", passed? "PASSED" : "FAILED");
    printf("\t xTaskGetTickCount() start: %d \t end: %d \t diff: %d \n", start, end, end - start);
    printf("\t gettimeofday()    start: %lu \t end: %lu \t diff: %lu \n", start_tm, end_tm, (end_tm - start_tm));
    printf("\t clock()    start: %d \t end: %d \t diff: %d \n", start_clock, end_clock, (end_clock - start_clock));
    printf("\t getRtcTimeSec()    start: %d \t end: %d \t diff: %d \n",
            start_rtcSeconds, end_rtcSeconds, (end_rtcSeconds - start_rtcSeconds));
    printf("\t time()            start: %lu \t\t end: %lu \t diff: %lu\n", (unsigned long)start_time,
            (unsigned long)end_time,
            (unsigned long)(end_time - start_time));

    return ACE_STATUS_OK;
}

//cli for log
const aceCli_moduleCmd_t time_cli[] = {
    { "get", "get time", ACE_CLI_SET_LEAF, .command.func=&cli_time_get},
    { "set", "set time (e.g 2019-05-22T19:12)",  ACE_CLI_SET_LEAF, .command.func=&cli_time_set},
#ifdef AMAZON_TESTS_ENABLE
    { "test","test time APIs [seconds]",  ACE_CLI_SET_LEAF, .command.func=&cli_time_test},
#endif
    ACE_CLI_NULL_MODULE
};
