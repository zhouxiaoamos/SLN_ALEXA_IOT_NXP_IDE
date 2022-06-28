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
 * @file time_utils.c
 *
 * A layer on top of the timer HAL to provide the ticks in microseconds.
 * This may also provide overwritten implementation of the time.h and sys/time.h
 * functions
 *******************************************************************************
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include "time_utils.h"
#include "iot_timer.h"
#include "iot_rtc.h"
#include "FreeRTOS.h"
#include <semphr.h>
#include "task.h"

#define USEC_PER_SEC (USEC_PER_MSEC * 1000)
#define USEC_TO_SEC(x)  ((x) / USEC_PER_SEC)
#define SEC_TO_USEC(x)  ((x) * USEC_PER_SEC)

#define USLEEP_BUSY_WAIT_THRESHOLD (1000U)

/* Use timer instance 1 by default, can be overridden with CFLAGS.
 * The instance must be a free running timer and other modules shall not start and stop this timer.
 */
#ifndef TIMER_INSTANCE
#define TIMER_INSTANCE 1
#endif

/*******************************************************************************
 * Globals
 ******************************************************************************/
IotTimerHandle_t gIotTimerHandle = NULL;
IotRtcHandle_t gIotRtcHandle = NULL;
static SemaphoreHandle_t g_set_mutex = NULL;
static bool g_isClockSetExternally = false;

static struct {
    time_t   rtcSeconds;
    uint64_t clockUs;
} clockBase;

/*******************************************************************************
 * Local functions
 ******************************************************************************/

extern time_t timegm(const struct tm *tv);

// update the clock time and rtc seconds for timestamp base.
static int time_utils_update_clockbase(const struct tm * tv)
{
    time_t t = timegm(tv);

    if (((time_t)-1) == t){
        return -1;
    }

    clockBase.rtcSeconds  = t;
    clockBase.clockUs = time_utils_get_microseconds();
    return 0;
}

static int time_utils_settime_internal(const struct tm * timeVal, bool external_set)
{
    // wait on semaphore, in case other setter is just rebasing to sync up gpt, always wait
    if (pdTRUE != xSemaphoreTake(g_set_mutex, portMAX_DELAY)) {
        return -1;
    }

    // intentionally not letting fail if no rtc is available
    int ret = 0;

    //update the clock base first so if we don't have rtc, software clock will still work
    time_utils_update_clockbase(timeVal);

    if (NULL == gIotRtcHandle)
    {
        printf("DOES NOT HAVE RTC\n");
        goto exit;
    }

    IotRtcDatetime_t iotTime = {.ucSecond = timeVal->tm_sec,
                                .ucMinute = timeVal->tm_min,
                                .ucHour = timeVal->tm_hour,
                                .ucDay = timeVal->tm_mday,
                                .ucMonth = timeVal->tm_mon,
                                .usYear = timeVal->tm_year,
                                .ucWday = timeVal->tm_wday,
    };

    // if we do have rtc and this fails, that is an issue
    ret = (IOT_RTC_SUCCESS !=  iot_rtc_set_datetime(gIotRtcHandle, &iotTime)) ? -1 : 0;

exit:
    if (external_set)
    {
        g_isClockSetExternally = true;
    }

    xSemaphoreGive(g_set_mutex);
    return ret;
}

/*******************************************************************************
 * Implementation of new APIs
 ******************************************************************************/

int time_utils_init(void)
{
    if (gIotTimerHandle && gIotRtcHandle)
    {
        //if already initialized, skip.
        return 0;
    }

    g_set_mutex = xSemaphoreCreateMutex();
    if (!gIotTimerHandle)
    {
        //Open timer to initialize hardware.
        gIotTimerHandle = iot_timer_open(TIMER_INSTANCE);
        if (gIotTimerHandle)
        {
            //Start the timer
            if (IOT_TIMER_SUCCESS != iot_timer_start(gIotTimerHandle))
            {
                return -1;
            }
        }
    }

    if (!gIotRtcHandle)
    {
        //Open rtc to initialize hardware. If it fails it's not considered a failure
        // not all hardware has rtc, and time will function correctly on gpt.
        gIotRtcHandle = iot_rtc_open(0);
        //initialize the first time
        struct tm timeVal = {.tm_year = 70,
                             .tm_mday = 1};
        // Not calling time_utils_settime since the default time is not correct
        time_utils_settime_internal(&timeVal, false);
    }

    return 0;
}

uint64_t time_utils_get_microseconds(void)
{
    uint64_t microSeconds = 0;

    if (gIotTimerHandle == NULL)
    {
        return 0;
    }

    //Get the time in micro seconds
    //Ignore the error as microSeconds will be 0 if errors.
    iot_timer_get_value(gIotTimerHandle, &microSeconds);

    return microSeconds;
}

// get the Rtc Time in seconds
// It is preferred to use getGptTime if you just need time.
// Use this to re-sync gpt time or compare gpt to rtc.
// not static so that test cli can use
time_t getRtcTimeSec(void)
{
    if (gIotRtcHandle == NULL)
    {
        return (time_t)-1;
    }

    IotRtcDatetime_t dateTime;

     // Get the date and time
    if (IOT_RTC_SUCCESS == iot_rtc_get_datetime(gIotRtcHandle, &dateTime))
    {
        /* Number of days from begin of the non Leap-year*/
        uint16_t monthDays[] = {0U, 0U, 31U, 59U, 90U, 120U, 151U, 181U, 212U, 243U, 273U, 304U, 334U};
        uint32_t seconds;

        /* Compute number of days from 1970 till given year*/
        seconds = (dateTime.usYear - 70U) * DAYS_IN_A_YEAR;
        /* Add leap year days */
        seconds += (dateTime.usYear - 70U) >> 2; // fast divide by 4
        /* Add number of days till given month*/
        seconds += monthDays[dateTime.ucMonth];
        /* Add days in given month. We subtract the current day as it is
        * represented in the hours, minutes and seconds field*/
        seconds += (dateTime.ucDay - 1);
        /* For leap year if month less than or equal to Febraury, decrement day counter*/
        if ((!(dateTime.usYear & 3U)) && (dateTime.ucMonth <= 2U))
        {
            seconds--;
        }

        seconds = (seconds * SECONDS_IN_A_DAY) + (dateTime.ucHour * SECONDS_IN_A_HOUR) +
                  (dateTime.ucMinute * SECONDS_IN_A_MINUTE) + dateTime.ucSecond;
        return seconds;
    }
    return (time_t)-1;
}

/**
 * gets the time based on GPT clock
 * works without rtc hardware, but is still controlled by settime()
 * If RTC hardware exists, will make sure clocks are in sync to seconds level
 * and resync gpt if necessary
 * @param tv[out] the non null timeval to get filled in
 */
static void getGptTime(struct timeval *tv)
{
    bool isTimeGood = true;
    memset(tv, 0, sizeof(*tv));

    uint64_t clockUs = time_utils_get_microseconds();

    time_t gptSeconds = (time_t)(clockBase.rtcSeconds + USEC_TO_SEC(clockUs - clockBase.clockUs));

    // check if we need to resync gpt clock (only works if have rtc)
    // note when this happens will lose milliseconds back to 0
    // this shoudldn't happen but ll-hal can be buggy (e.g. overflows at 32 bits)
    if (clockUs < clockBase.clockUs)
    {
        // don't wait here, if someone else is setting, let them keep the set.
        // (it's either a more accurate time or already rebasing)
        if (pdTRUE != xSemaphoreTake(g_set_mutex, portMAX_DELAY))
        {
            return;
        }

        if (gIotRtcHandle != NULL)
        {
            // cannot use LOG here because that could cause circular logic since log gets time
            printf("Falling back to RTC time, GPT is wrong\n");
            time_t rtcSeconds = getRtcTimeSec();

            clockBase.rtcSeconds  = rtcSeconds;
            clockBase.clockUs = clockUs;

            gptSeconds = rtcSeconds;

            g_isClockSetExternally = true;
        }
        else
        {
            // if we don't have rtc and the clock rolled over (cause bug in ll-hal)
            // then don't set the time and mark as invalid until next set
            g_isClockSetExternally = false;
            isTimeGood = false;
        }

        xSemaphoreGive(g_set_mutex);
    }

    if (isTimeGood)
    {
        tv->tv_sec = gptSeconds;
        tv->tv_usec = (suseconds_t)((clockUs - clockBase.clockUs)% (SEC_TO_USEC(1)));
    }
}

/*******************************************************************************
 * overwriting functions
 ******************************************************************************/
#if 0
int usleep(useconds_t usec)
{
    if (usec < USLEEP_BUSY_WAIT_THRESHOLD)
    {
        //TODO- To improve - use events rather than busy wait for > some threshold
        //current threshold is 1ms for busywait. So may be use busywait for < threshold1
        //and use events for < threshold2, and anything > threshold2, just use
        //FreeRTOS.
        uint64_t curTime;
        curTime = time_utils_get_microseconds();

        //busy wait
        while (time_utils_get_microseconds() < ( curTime + (uint64_t)usec ))
        {
        }
    }
    else
    {
        //Use FreeRTOS taskDelay function.
        vTaskDelay( USEC_TO_MSEC(usec) / portTICK_PERIOD_MS );
    }
    return 0;
}
#endif

int time_utils_settime(const struct tm * timeVal)
{
    return time_utils_settime_internal(timeVal, true);
}

/*
 * clock tick here is 1msec
 */
clock_t clock(void)
{
    uint64_t microSeconds = 0;

    if (gIotTimerHandle == NULL)
    {
        return 0;
    }
    iot_timer_get_value(gIotTimerHandle, &microSeconds);

    return USEC_TO_MSEC(microSeconds);
}

/*
 * time function provided by time.h.
 * TODO: This does not provide the day of week,
 * day of year and DST flag yet.
 */
time_t time(time_t *_timer)
{
    // TODO NXP uncomment the 3 lines below after iot_timer is fixed
//    struct timeval tv;
//    getGptTime(&tv);
//
//    time_t seconds = tv.tv_sec;

    // TODO NXP delete the line below after iot_timer gets fixed
    time_t seconds = getRtcTimeSec();

    if (_timer)
    {
        *_timer = seconds;
    }
    return seconds;
}

#if 0
/**
 * gettimeofday() provided by time.h
 * TODO: This ignores the timezone even if passed non null
 */
int gettimeofday(struct timeval *tv, void *tz)
{
    if (!tv) {
        errno = EFAULT;
        return -1;
    }

    getGptTime(tv);

    return 0;
}
/**
 * overwrite clock_gettime() to provide timestamps in nsec.
 */
int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    if (!tp) {
        errno = EFAULT;
        return -1;
    }
    struct timeval tv;

    switch (clk_id) {
    case CLOCK_REALTIME:
        getGptTime(&tv);
        tp->tv_sec = tv.tv_sec;
        tp->tv_nsec = tv.tv_usec * NSEC_PER_USEC;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    return 0;
}
#endif
/**
 * Some libc implemenations include this, ours does not so implement
 */
time_t timegm(const struct tm *tv)
{
    // copied from http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
    return tv->tm_sec +
           tv->tm_min  * SECONDS_IN_A_MINUTE +
           tv->tm_hour * SECONDS_IN_A_HOUR +
           tv->tm_yday * SECONDS_IN_A_DAY +
           // tm_year is based from 1900 but seconds epoch should be from 1970, so subtract 70
           (tv->tm_year - 70) * SECONDS_IN_A_YEAR +
           //leap year: add in a day for each year that follows a leap year
           //starting with the first leap year since the Epoch.
           // The first term adds a day every 4 years starting in 1973
           //the second subtracts a day back out every 100 years starting in 2001
           //the third adds a day back in every 400 years starting in 2001
           //The divisions in the formula are integer divisions;
           // that is, the remainder is discarded leaving only the integer quotient.
           ((tv->tm_year - 69) / 4) * SECONDS_IN_A_DAY -
           ((tv->tm_year - 1) / 100) * SECONDS_IN_A_DAY +
           ((tv->tm_year + 299) / 400) * SECONDS_IN_A_DAY;
}

uint64_t clock_gettime_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return 0;
    } else {
        return (uint64_t)(SEC_TO_MSEC((uint64_t)ts.tv_sec) + NSEC_TO_MSEC(ts.tv_nsec));
    }
}

bool clock_isset()
{
    return g_isClockSetExternally;
}
