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

// TODO:
//     - Use portBASE_TYPE or BaseType_t?
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "afw_event_manager.h"
#include "afw_error.h"
#include "string.h"
#include "asd_log_platform_api.h"



#ifdef AFW_EVENT_MANAGER

#define DBG_NAME_LEN    16

/* Array of event queues in the system. The number of event queues is defined
 * by the application. */
static struct afw_event_queue_t {
    QueueHandle_t q;
    char dbgname[DBG_NAME_LEN];
} event_queue_list[AFW_NUM_EVENT_QUEUES] = {{0}};

/* Each event has an associated bitmask indicating the event queues that have
 * subscribed to that event. The number of events is defined by the
 * application. */
static volatile struct afw_event_info_t {
    uint32_t subscribers;
} event_list[AFW_NUM_EVENTS] = {{0}};

/* Semaphore for controlling modifications to event_queue_list */
static volatile SemaphoreHandle_t event_queues_mutex = 0;
static volatile SemaphoreHandle_t publish_mutex = 0;

static inline int8_t get_next_idx_clear(uint32_t *word);
static int init_publish_mutex(void);


int8_t afw_event_queue_init(int8_t queue_id, const char *dbgname)
{
    taskENTER_CRITICAL();
    if (!event_queues_mutex)
        event_queues_mutex = xSemaphoreCreateMutex();
    taskEXIT_CRITICAL();

    if (!event_queues_mutex)
        return -1;

    xSemaphoreTake(event_queues_mutex, portMAX_DELAY);

    if (queue_id == EVENT_QUEUE_AUTO) {
        /* Find first available event queue if a valid event queue id has not
         * been provided */
        for (int i = 0; i < AFW_NUM_EVENT_QUEUES; i++) {
            if (event_queue_list[i].q == 0) {
                queue_id = i;
                break;
            }
        }
        if (queue_id == EVENT_QUEUE_AUTO) {
            ASD_LOG_E(afw, "ERROR: no event queue available\n");
            return -1;
        }
    } else {
        /* If an event queue id has been provided, check to make sure that it
         * is valid and available */
        if (queue_id >= AFW_NUM_EVENT_QUEUES || event_queue_list[queue_id].q) {
            ASD_LOG_E(afw, "ERROR: specified event queue %d unavailable\n", queue_id);
            return -1;
        }
    }

    event_queue_list[queue_id].q = xQueueCreate(AFW_EVENT_QUEUE_LENGTH, sizeof(afw_event_t));

    if (!event_queue_list[queue_id].q) {
        ASD_LOG_E(afw, "ERROR: failed to init event queue\n");
        return -1;
    }

    strncpy(event_queue_list[queue_id].dbgname, dbgname, DBG_NAME_LEN);
    event_queue_list[queue_id].dbgname[DBG_NAME_LEN - 1] = '\0';

    xSemaphoreGive(event_queues_mutex);

    return queue_id;
}


int afw_post_event(int8_t queue_id, int event_id, void *pdata,
        portBASE_TYPE *pxHigherPriorityTaskWoken)
{
    int status = 0;

    afw_event_t event = {
        .id    = event_id,
        .pdata = pdata,
    };

    if (queue_id < 0 || queue_id >= AFW_NUM_EVENT_QUEUES)
        return -1;

    if (!event_queue_list[queue_id].q) {
        ASD_LOG_E(afw, "ERROR: attempt to post to uninitialized event queue %d\n", queue_id);
        return -1;
    }

    if (pxHigherPriorityTaskWoken)
        status = xQueueSendFromISR(event_queue_list[queue_id].q, &event, pxHigherPriorityTaskWoken);
    else
        status = xQueueSend(event_queue_list[queue_id].q, &event, 1);

    if (status != pdTRUE) {
        ASD_LOG_E(afw, "ERROR: could not post event %d to queue %d [%s]\n", event_id,
                queue_id, event_queue_list[queue_id].dbgname);
        return -1;
    }

    return ACE_STATUS_OK;
}


int afw_publish_event(int event_id, void *pdata, portBASE_TYPE *pxHigherPriorityTaskWoken)
{
    int status = 0;

    if (event_id < 0 || event_id >= AFW_NUM_EVENTS)
        return -1;

    uint32_t subscribers = event_list[event_id].subscribers;

    if (!pxHigherPriorityTaskWoken) {
        if (!publish_mutex) {
            if (init_publish_mutex() != 0) {
                ASD_LOG_E(afw, "ERROR: could not create event publish mutex\n");
                return -1;
            }
        }

        xSemaphoreTake(publish_mutex, portMAX_DELAY);
    }

    while (subscribers) {
        int8_t queue_id = get_next_idx_clear(&subscribers);
        if (afw_post_event(queue_id, event_id, pdata, pxHigherPriorityTaskWoken) != 0)
            status = -1;
    }

    if (!pxHigherPriorityTaskWoken)
        xSemaphoreGive(publish_mutex);

    return status;
}


int afw_get_event(int8_t queue_id, void **pdata, TickType_t timeout_ticks)
{
    int event_id = AFW_NO_EVENT;
    afw_event_t event;

    if (queue_id >= AFW_NUM_EVENT_QUEUES || !event_queue_list[queue_id].q)
        return -1;

    if (xQueueReceive(event_queue_list[queue_id].q, (void *)&event, timeout_ticks)) {
            event_id = event.id;
            if (pdata)
                *pdata = event.pdata;
    }

    return event_id;
}

int afw_wait_for_event(int8_t queue_id, int event_id, void **pdata, TickType_t timeout_ticks)
{
    if (queue_id >= AFW_NUM_EVENT_QUEUES || !event_queue_list[queue_id].q)
        return -AFW_EINVAL;

    if (event_id < 0 || event_id >= AFW_NUM_EVENTS)
        return -AFW_EINVAL;

    TickType_t curr_tick = 0;
    TickType_t last_tick = portMAX_DELAY;
    if (timeout_ticks != portMAX_DELAY) {
        curr_tick = xTaskGetTickCount();
        last_tick = curr_tick + timeout_ticks;
    }

    while (curr_tick < last_tick) {
        if (afw_get_event(queue_id, pdata, last_tick - curr_tick) == event_id)
            return ACE_STATUS_OK;

        if (timeout_ticks == portMAX_DELAY)
            continue;
        curr_tick = xTaskGetTickCount();
    }
    return -AFW_ETIMEOUT;
}

void afw_subscribe_events(int8_t queue_id, const int events[], int num_events, bool is_isr)
{
    if (!is_isr)
        taskENTER_CRITICAL();

    for (int i = 0; i < num_events; i++) {
        int event_id = events[i];
        if (event_id >= AFW_NUM_EVENTS) {
            ASD_LOG_E(afw, "ERROR: bad event id %d\n", event_id);
            continue;
        }
        event_list[event_id].subscribers |= (1 << queue_id);
    }
    if (!is_isr)
        taskEXIT_CRITICAL();
}


void afw_unsubscribe_events(int8_t queue_id, const int events[], int num_events, bool is_isr)
{
    if (!is_isr)
        taskENTER_CRITICAL();

    for (int i = 0; i < num_events; i++) {
        int event_id = events[i];
        if (event_id >= AFW_NUM_EVENTS) {
            ASD_LOG_E(afw, "ERROR: bad event id %d\n", event_id);
            continue;
        }
        event_list[event_id].subscribers &= ~(1 << queue_id);
    }
    if (!is_isr)
        taskEXIT_CRITICAL();
}


void afw_unsubscribe_all_events(int8_t queue_id, bool is_isr)
{
    for (int i = 0; i < AFW_NUM_EVENTS; i++) {
        if (!is_isr)
            taskENTER_CRITICAL();

        event_list[i].subscribers &= ~(1 << queue_id);

        if (!is_isr)
            taskEXIT_CRITICAL();
    }
}


bool afw_event_is_subscribed(int8_t queue_id, int event_id)
{
    return (bool)(event_list[event_id].subscribers & (1 << queue_id));
}


void afw_flush_event_queue(int8_t queue_id)
{
    if (queue_id >= 0 && queue_id < AFW_NUM_EVENT_QUEUES && event_queue_list[queue_id].q)
        xQueueReset(event_queue_list[queue_id].q);
}


int afw_event_queue_deinit(int8_t queue_id)
{
    xSemaphoreTake(event_queues_mutex, portMAX_DELAY);

    if (queue_id >= AFW_NUM_EVENT_QUEUES || !event_queue_list[queue_id].q)
        return -1;

    afw_unsubscribe_all_events(queue_id, false);
    vQueueDelete(event_queue_list[queue_id].q);
    event_queue_list[queue_id].q = 0;
    event_queue_list[queue_id].dbgname[0] = '\0';

    xSemaphoreGive(event_queues_mutex);

    return ACE_STATUS_OK;
}

/*******************************************************************************
 * get_next_idx_clear
 *
 * Get index of leading set bit and then clear that bit.
 *******************************************************************************
 */
static inline int8_t get_next_idx_clear(uint32_t *word)
{
    int8_t leading_set_bit = 31 - __CLZ(*word);

    if (leading_set_bit >= 0)
        *word &= ~(1 << leading_set_bit);

    return leading_set_bit;
}

/*******************************************************************************
 * init_publish_mutex
 *******************************************************************************
 */
static int init_publish_mutex(void)
{
    taskENTER_CRITICAL();
    if (!publish_mutex)
        publish_mutex = xSemaphoreCreateMutex();
    taskEXIT_CRITICAL();

    if (!publish_mutex)
        return -1;

    return ACE_STATUS_OK;
}

/*******************************************************************************
 * afw_eventmgr_post_cli
 *
 * CLI command for posting an event to a specific event queue
 *******************************************************************************
 */
static ace_status_t afw_eventmgr_post_cli(int32_t len, const char *param[])
{
    if (len != 2) {
        printf("Usage: eventmgr post QUEUE_ID EVENT_ID\n\n");
        printf("Arguments:\n");
        printf("\tQUEUE_ID\ttarget queue (0 to %d)\n", AFW_NUM_EVENT_QUEUES - 1);
        printf("\tEVENT_ID\tevent to post (0 to %d)\n", AFW_NUM_EVENTS - 1);
        return ACE_STATUS_OK;
    }

    errno = 0;
    int8_t queue_id = strtol(param[0], NULL, 0);
    int event_id    = strtol(param[1], NULL, 0);

    if (errno) {
        printf("Error parsing parameters\n");
        return ACE_STATUS_OK;
    }

    int status = afw_post_event(queue_id, event_id, NULL, NULL);

    if (status != 0)
        printf("Error posting event %d to queue %d\n", event_id, queue_id);

    return ACE_STATUS_OK;
}

/*******************************************************************************
 * afw_eventmgr_publish_cli
 *
 * CLI command for publishing an event
 *******************************************************************************
 */
static ace_status_t afw_eventmgr_publish_cli(int32_t len, const char *param[])
{
    if (len != 1 || (len == 1 && param[0][0] == '?')) {
        printf("Usage: eventmgr publish EVENT_ID\n\n");
        printf("Arguments:\n");
        printf("\tEVENT_ID\tevent to publish (0 to %d)\n", AFW_NUM_EVENTS - 1);
        return ACE_STATUS_OK;
    }

    errno = 0;
    int event_id = strtol(param[0], NULL, 0);

    if (errno) {
        printf("Error parsing parameters\n");
        return ACE_STATUS_OK;
    }

    int status = afw_publish_event(event_id, NULL, NULL);

    if (status != 0)
        printf("Error publishing event %d\n", event_id);

    return ACE_STATUS_OK;
}

/*******************************************************************************
 * afw_eventmgr_show_sub_cli
 *
 * CLI command for showing current event subscribers
 *******************************************************************************
 */
static ace_status_t afw_eventmgr_show_sub_cli(int32_t len, const char *param[])
{
    if (len != 0) {
        printf("Usage: eventmgr show subscribers\n");
        return ACE_STATUS_OK;
    }

    taskENTER_CRITICAL();

    for (int i = 0; i < AFW_NUM_EVENTS; i++) {
        uint32_t subscribers = event_list[i].subscribers;

        if (subscribers)
            printf("event_id %d:\n", i);

        while (subscribers) {
            int8_t queue_id = get_next_idx_clear(&subscribers);
            if (queue_id >=0) {
                printf("\tqueue_id %2d [%s]\n", queue_id, event_queue_list[queue_id].dbgname);
            }
        }
    }

    taskEXIT_CRITICAL();

    return ACE_STATUS_OK;
}

/*******************************************************************************
 * afw_eventmgr_show_queues_cli
 *
 * CLI command for showing all registered event queues
 *******************************************************************************
 */
static ace_status_t afw_eventmgr_show_queues_cli(int32_t len, const char *param[])
{
    if (len != 0) {
        printf("Usage: eventmgr show queues\n");
        return ACE_STATUS_OK;
    }

    printf("%8s\t%16s\t%10s\n", "queue_id", "dbgname", "handle");
    printf("--------------------------------------------------\n");

    xSemaphoreTake(event_queues_mutex, portMAX_DELAY);

    for (int i = 0; i < AFW_NUM_EVENT_QUEUES; i++) {
        if (event_queue_list[i].q == 0)
            continue;

        printf("%8d\t%16s\t0x%p\n", i, event_queue_list[i].dbgname, event_queue_list[i].q);
    }

    xSemaphoreGive(event_queues_mutex);

    return ACE_STATUS_OK;
}

static const aceCli_moduleCmd_t eventmgr_show_cli[] = {
     { "subscribers",   "show event subscribers",   ACE_CLI_SET_LEAF, .command.func=&afw_eventmgr_show_sub_cli                     },
     { "queues",        "show all event queues",    ACE_CLI_SET_LEAF, .command.func=&afw_eventmgr_show_queues_cli                  },
     ACE_CLI_NULL_MODULE
};

const aceCli_moduleCmd_t eventmgr_cli[] = {
     { "post",          "post event to queue",      ACE_CLI_SET_LEAF, .command.func=&afw_eventmgr_post_cli},
     { "publish",       "publish event",            ACE_CLI_SET_LEAF, .command.func=&afw_eventmgr_publish_cli},
     { "show",          "show",                     ACE_CLI_SET_FUNC, .command.subCommands=eventmgr_show_cli  },
     ACE_CLI_NULL_MODULE,
};

#endif
