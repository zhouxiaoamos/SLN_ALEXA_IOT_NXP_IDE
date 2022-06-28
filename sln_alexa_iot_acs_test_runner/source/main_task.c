/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All rights reserved.
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
 * Main Task
 *
 *******************************************************************************
 */
/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* ACE includes */
#include <ace/ace.h>
#include <ace/ace_log.h>
#include <ace/wifi_mgr.h>
#include <ace/eventmgr_types.h>
#include <ace/eventmgr_api.h>
#include <assert.h>
#ifdef SNTP_SERVER_DNS
#include <asd_sntp_client_lwip.h>
#endif
#include "app_init.h"

#ifdef AMAZON_TESTS_ENABLE
void testReboot() {}
#endif

static void network_event_cb(aceEventMgr_eventParams_t* params, void* ctx) {
    ACE_LOGI(ACE_LOG_ID_MAIN, "main", "wifi event received group=%d event= %d",
             params->group_id, params->event_id);
    switch (params->group_id) {
        case ACE_EVENTMGR_GROUP_WIFI_STATE: {
            switch (params->event_id) {
                case ACE_EVENTMGR_EVENT_WIFI_STATE_CONNECTED:
                    ACE_LOGI(ACE_LOG_ID_MAIN, "main",
                             "received WIFI_STATE_CONNECTED");
#ifdef SNTP_SERVER_DNS
                    ACE_LOGI(ACE_LOG_ID_MAIN, "main",
                             "Startting SNTP time server");
                    asd_sntp_client_start_default();
#endif
                    break;
                default:  // other wifi events.
                    break;
            }
        }
        default:  // Other groups
            break;
    }
}

static ace_status_t main_register_wifi_cb(void) {
    aceEventMgr_subscribeHandle_t subs_id;

    aceEventMgr_subscribeParams_t subs_params;
    subs_params.type = ACE_EVENTMGR_SUBSCRIBE_TYPE_CALLBACK;
    subs_params.cb.func = network_event_cb;
    ace_status_t status =
        aceEventMgr_registerSubscriber(ACE_MODULE_WIFI, &subs_params, &subs_id);
    if (status != ACE_STATUS_OK) {
        ACE_LOGI(ACE_LOG_ID_MAIN, "main",
                 "Failed in aceEventMgr_registerSubscriber err = %d", status);
        return status;
    }

    status = aceEventMgr_subscribe(&subs_id, ACE_EVENTMGR_GROUP_WIFI_STATE, 0);
    if (status != ACE_STATUS_OK) {
        ACE_LOGE(ACE_LOG_ID_MAIN, "main",
                 "Subscribe ACE_EVENTMGR_GROUP_WIFI_STATE failed %d", status);
        return status;
    }
}

void ace_app_main(void* parameters) {
    ACE_LOGI(ACE_LOG_ID_SYSTEM, "main", "Hello World!!");

    vTaskDelay(1000);
#ifdef ACE_WIFI_MIDDLEWARE
    aceWifiMgr_error_t err = aceWifiMgr_init();
    if (err != aceWifiMgr_ERROR_SUCCESS) {
        ACE_LOGE(ACE_LOG_ID_MAIN, "main", "wifi init failed, errorcode: %d",
                 err);
    }
    /* register for callbacks */
    else if (aceWifiMgr_isWifiReady()) {
        main_register_wifi_cb();
    }
#endif
    while (1) {
        vTaskSuspend(NULL);
    }
}
