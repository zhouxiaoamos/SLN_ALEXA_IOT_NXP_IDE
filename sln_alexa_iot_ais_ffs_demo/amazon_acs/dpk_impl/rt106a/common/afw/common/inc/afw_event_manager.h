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
 * AFW Event Manager
 *******************************************************************************
 */

/**
 * @file
 *
 * @brief This module implements a publish/subscribe event system to enable simple co-ordination
 * between multiple tasks.
 *
 * In order to subscribe to events, you need to initialize an event queue. Then you can subscribe
 * to a list of event IDs. Whenever an event with a matching event ID is published, it will arrive
 * on the queue.
 *
 * The event handler should regularly
 */

#ifndef AFW_EVENT_MANAGER_H
#define AFW_EVENT_MANAGER_H

#include "ace/aceCli.h"
#include "FreeRTOS.h"


#ifdef AFW_EVENT_MANAGER
#include "afw_events.h"

// define additional events here
typedef enum {
    ACORN_EVENT_STATE_CHANGE_REQUIRED = AFW_EVENT_MAX,
    ACORN_EVENT_MAX
} acorn_event_def_t;

#define AFW_NUM_EVENTS ACORN_EVENT_MAX

#define AFW_NUM_EVENT_QUEUES 20

#define AFW_EVENT_QUEUE_LENGTH 6
#endif

typedef struct {
	/** @brief The event ID */
    int id;

    /** @brief The data attached to the event */
    void *pdata;
} afw_event_t;

#define EVENT_QUEUE_AUTO        (-1)
#define AFW_MAX_EVENT_QUEUES    32
#define AFW_NO_EVENT            (-1)

#if AFW_NUM_EVENT_QUEUES > AFW_MAX_EVENT_QUEUES
#error Too many event queues
#endif

/**
 * Do not call this function from an ISR.
 *
 * @param queue_id Used to specify the queue that needs to be
 * initialized, where queue_id is in [0, AFW_NUM_EVENT_QUEUES - 1]. If it is
 * EVENT_QUEUE_AUTO, the first available event queue is used. It is recommended
 * that the two approaches not be mixed in an application.
 *
 * @param dbgname Used to identify the queue when printing debug messages
 *
 * @return The queue id of the initialized event queue. Returns -1 on failure.
 */
int8_t afw_event_queue_init(int8_t queue_id, const char *dbgname);


/**
 * @brief Post event to a specific event queue.
 *
 * @param queue_id needs to be in [0, AFW_NUM_EVENT_QUEUES - 1].
 *
 * @param event_id Since this function posts directly to a queue and does not use the
 * subscription mechanism, event_id is not restricted to [0, AFW_NUM_EVENTS -
 * 1]. This enables tasks to use this function to post private events to their
 * own queue or to other known queues.
 *
 * @param pxHigherPriorityTaskWoken If calling from an ISR,
 * 'pxHigherPriorityTaskWoken' should be non-NULL.
 *
 * @return 0 on success, -1 on failure.
 */
int afw_post_event(int8_t queue_id, int event_id, void *pdata,
        portBASE_TYPE *pxHigherPriorityTaskWoken);

/**
 * @brief Publish event to all event queues that have subscribed to it.
 *
 * Published events
 * appear in the same order to all subscribers.
 *
 * @param event_id needs to be in [0, AFW_NUM_EVENTS - 1].
 *
 * @param pxHigherPriorityTaskWoken If calling from an ISR,
 * 'pxHigherPriorityTaskWoken' should be non-NULL.
 *
 * @return 0 on success, -1 on failure.
 */
int afw_publish_event(int event_id, void *pdata, portBASE_TYPE *pxHigherPriorityTaskWoken);

/**
 * @brief Get an event from an event queue, blocking for at most timeout_ticks.
 *
 * Do not call from an ISR.
 *
 * @return the event id
 */
int afw_get_event(int8_t queue_id, void **pdata, TickType_t timeout_ticks);

/**
 * @brief Wait for specified event from event queue, blocking for at most timeout_ticks
 *
 * Do not call from an ISR.
 * Supports both infinite and normal timeout
 *
 * @param queue_id should be in [0, AFW_NUM_EVENT_QUEUES - 1]
 *
 * @param event_id should be in [0, AFW_NUM_EVENTS - 1]
 *
 * @return 0 on success, an afw_error value on failure
 */
int afw_wait_for_event(int8_t queue_id, int event_id, void **pdata, TickType_t timeout_ticks);

/**
 * @brief Subscribe the specified queue to the events in 'events'.
 *
 * @param queue_id Argument queue_id should be in [0, AFW_NUM_EVENT_QUEUES - 1].
 *
 * @param event_list Each event_id in the event list should be in [0, AFW_NUM_EVENTS - 1].
 */
void afw_subscribe_events(int8_t queue_id, const int event_list[], int num_events, bool is_isr);

/**
 * @brief Unsubscribe the specified queue from the events in event_list.
 *
 * @param event_list Each event_id in the event list should be in [0, AFW_NUM_EVENTS - 1].
 *
 * @param queue_id Argument queue_id should be in [0, AFW_NUM_EVENT_QUEUES - 1].
 */
void afw_unsubscribe_events(int8_t queue_id, const int event_list[], int num_events, bool is_isr);

/**
 * @brief Unsubscribe queue from all events.
 */
void afw_unsubscribe_all_events(int8_t queue_id, bool is_isr);

/**
 * @brief Check whether a queue has been subscribed to an event.
 */
bool afw_event_is_subscribed(int8_t queue_id, int event_id);

/**
 * @brief Flush all events in the event queue and reset it to empty.
 */
void afw_flush_event_queue(int8_t queue_id);

/**
 * @brief Delete a queue and all events inside. Do not call from an ISR.
 */
int afw_event_queue_deinit(int8_t queue_id);

/**
 * @brief Subscribe the specified queue to the event.
 *
 * @param queue_id Argument queue_id should be in [0, AFW_NUM_EVENT_QUEUES - 1].
 *
 * @param event_id Event ID should be in [0, AFW_NUM_EVENTS - 1].
 */
static inline void afw_subscribe_event(int8_t queue_id, int event_id, bool is_isr)
{
    afw_subscribe_events(queue_id, &event_id, 1, is_isr);
}

/**
 * @brief Unsubscribe the specified queue fromt the event.
 *
 * @param queue_id Argument queue_id should be in [0, AFW_NUM_EVENT_QUEUES - 1].
 *
 * @param event_id Event ID should be in [0, AFW_NUM_EVENTS - 1].
 */
static inline void afw_unsubscribe_event(int8_t queue_id, int event_id, bool is_isr)
{
    afw_unsubscribe_events(queue_id, &event_id, 1, is_isr);
}

extern const aceCli_moduleCmd_t eventmgr_cli[];

#endif
