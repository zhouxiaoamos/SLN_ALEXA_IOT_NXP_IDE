/*
 * Amazon FreeRTOS POSIX V1.1.1
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "time.h"
#include "time_utils.h"
#include "ace/osal_time.h"
#include "ace/ace_log.h"

// Converting system ticks to milliseconds
#define pdTICKS_TO_MS(_ticks) (((uint64_t)(_ticks)*1000) / configTICK_RATE_HZ)

const char *slnTimeUtils_logTag = "slnTimeUtils";

int clock_gettime( clockid_t clock_id,
                   struct timespec * tp )
{
    TimeOut_t xCurrentTime = { 0 };

    /* Intermediate variable used to convert TimeOut_t to struct timespec.
     * Also used to detect overflow issues. It must be unsigned because the
     * behavior of signed integer overflow is undefined. */
    uint64_t ullTickCount = 0ULL;

    /* Silence warnings about unused parameters. */
    ( void ) clock_id;

    /* Get the current tick count and overflow count. vTaskSetTimeOutState()
     * is used to get these values because they are both static in tasks.c. */
    vTaskSetTimeOutState( &xCurrentTime );

    /* Adjust the tick count for the number of times a TickType_t has overflowed.
     * portMAX_DELAY should be the maximum value of a TickType_t. */
    ullTickCount = ( uint64_t ) ( xCurrentTime.xOverflowCount ) << ( sizeof( TickType_t ) * 8 );

    /* Add the current tick count. */
    ullTickCount += xCurrentTime.xTimeOnEntering;

    /* Convert ullTickCount to timespec. */
    tp->tv_nsec = (pdTICKS_TO_MS(ullTickCount) % 1000) * 1000000;
    tp->tv_sec  = (pdTICKS_TO_MS(ullTickCount) / 1000);

    return 0;
}

int gettimeofday( struct timeval *tv, void *tzvp )
{
    // TODO NXP uncomment the three lines below after iot_timer gets fixed
//    uint64_t microSeconds = time_utils_get_microseconds();
//    tv->tv_sec = microSeconds / (1000 * 1000);
//    tv->tv_usec = microSeconds % (1000 * 1000);

    // TODO NXP delete the two lines below after iot_timer gets fixed
    tv->tv_sec = time(NULL);
    tv->tv_usec = 0;

    return 0;
}

static int set_time(const struct tm* gt)
{
    if (time_utils_settime(gt) != 0 ) {
        ACE_LOGE(ACE_LOG_ID_MAIN, slnTimeUtils_logTag, "time_utils_settime failed\r\n");
        return -1;
    }

    struct timespec tp;
    int res = clock_gettime(CLOCK_REALTIME, &tp);
    if (res != 0) {
        ACE_LOGE(ACE_LOG_ID_MAIN, slnTimeUtils_logTag, "clock_gettime failed\r\n");
        return -1;
    }

    aceTime_timespec_t aceTime;
    aceTime.tv_sec = tp.tv_sec;
    aceTime.tv_nsec = tp.tv_nsec;

    ace_status_t aceRes = aceTime_setClockTime(&aceTime);
    if (aceRes != ACE_STATUS_OK) {
        ACE_LOGE(ACE_LOG_ID_MAIN, slnTimeUtils_logTag, "Failed to setClockTime: %d", aceRes);
        return -1;
    }

    return 0;
}

void sntp_set_system_time(time_t t)
{
    struct tm gt;
    ACE_LOGI(ACE_LOG_ID_MAIN, slnTimeUtils_logTag, "sntp_set_system_time input: %d\r\n", t);
    if (gmtime_r(&t, &gt) == NULL) {
        ACE_LOGE(ACE_LOG_ID_MAIN, slnTimeUtils_logTag, "Could not convert to gmt\r\n");
    }
    else
    {
        set_time(&gt);
    }
}

int usleep( useconds_t usec )
{
    /* To avoid delaying for less than usec, always round up. */
    vTaskDelay( pdMS_TO_TICKS( usec / 1000 + ( usec % 1000 != 0 ) ) );

    return 0;
}
