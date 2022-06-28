/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _APP_EVENTS_H
#define _APP_EVENTS_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

/*!
 * @addtogroup
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define APP_EVENTS_SET_EVENT(CATEGORY, EVENT) (((CATEGORY << 16) | EVENT))

/*!
 * @brief The different event cateogries for events to be handled
 */
typedef enum _app_events_category
{
    kAudioEvents = 0, /*!< Audio Events Category. */
    kAlertEvents,     /*!< Alerts/Timers Events Category. */
    kAvsEvents,       /*!< AVS Message Category. */
    kShadowEvents,    /*!< Shadow Message Category. */
} app_events_category_t;

/*!
 * @brief The event category for audio
 */
typedef enum _app_events_category_audio
{
    kIdle = 0,             /*!< Audio processing in idle state. */
    kWakeWordDetected,     /*!< Audio processing has detected a wake word. */
    kMicRecording,         /*!< Audio processing is recording microphone capture. */
    kMicStopRecording,     /*!< Audio processing has been instructed to stop recording. */
    kMicKeepOpen,          /*!< Audio processing should start recording without wake word detection. */
    kMicStop,              /*!< Audio processing has stopped recording microphone stream. */
    kMicCloudWakeVerifier, /*!< Audio processing is streaming cloud wake word verification data. */
    kReconnect             /*!< Audio processing ends current state and returns to idle for reconnection. */
} app_events_category_audio_t;

/*!
 * @brief The event category for audio
 */
typedef enum _app_events_category_app_notifications
{
    kExceptionEncountered = 0, /**< This notification is sent in case of a streamer error */
    kVolChanged,               /**< This notification is sent in case of a volume change */
    kAlertVolChanged,          /**< This notification is sent in case of an alert volume change */
    kSetLocale,                /**< This notification is sent in case of a set locale event */
    kLocaleChanged,            /**< This notification is sent in case of a change locale event */
    kSyncClock                 /**< This notification is sent as a response to the synchronize clock */
} app_events_category_app_notifications_t;

typedef enum _app_events_category_alert
{
    kNewAlertSet = 0,   /**< This notification is sent when a new alert is received */
    kAlertDelete,       /**< This notification is sent when an alert is deleted */
    kNewAlertFail,      /**< This notification is sent when the new alert wasn't saved */
    kFailDelete,        /**< This notification is sent when a delete of an alert failed */
    kAlertOnlineTrigger /**< This notification is sent when an alert is triggering */
} app_events_category_alert_t;

/*!
 * @brief The event payload for passing on the queue
 */
typedef struct _app_events
{
    uint32_t category; /*!< Event Category. */
    uint32_t event;    /*!< Event based on category. */
    void *data;        /*!< Data to be sent by the event */
} app_events_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _APP_EVENTS_H */
