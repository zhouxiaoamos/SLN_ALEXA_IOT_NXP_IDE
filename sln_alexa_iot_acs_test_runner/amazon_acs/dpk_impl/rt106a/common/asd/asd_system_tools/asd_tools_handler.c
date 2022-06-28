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
 * @File: asd_tools_handler.c
 */
#include "asd_debug.h"
//#include <ace/ace_tools_handler.h>
#include <string.h>
#include "asd_tools_handler.h"
#include "asd_reboot_manager.h"

#ifdef LOGUPLOAD_ENABLED
#include <ace/ace_logmgr.h>
#endif
#include "FreeRTOS.h"
#include "semphr.h"

SemaphoreHandle_t g_crashUploadWaitSemaphore;

// static int32_t asd_tool_handler_callback(void* ctx, ace_debug_tool_t error_type) {

//     switch (error_type) {
//         case ACE_TOOLS_STACK_CORRUPTION:
//             printf("\n ---------Stack Corruption Detected--------- " );
//             asd_clear_cached_minidump();
//             asd_get_cached_minidump()->reason = ASD_CD_REASON_STACK_CORRUPTION;
//             asd_crashdump_handler();
//             break;
//         case ACE_TOOLS_HEAP_CORRUPTION:
//             printf("\n ---------Heap Over-run Detected--------- " );
//             asd_clear_cached_minidump();
//             asd_get_cached_minidump()->reason = ASD_CD_REASON_HEAP_CORRUPTION;
//             asd_crashdump_handler();
//             break;
//         default:
//             printf("\n -------- Unknown Error Detected--------- " );
//             break;
//     }
//     return 0;

// }

void asd_tools_update_cb(void){
    //aceToolHandler_update_cb(&asd_tool_handler_callback, NULL);
}

#ifdef LOGUPLOAD_ENABLED
static void crashUpload_cb(uint32_t id, log_upload_event_t event)
{
    printf("LOGS UPLOADED\n");
    if (g_crashUploadWaitSemaphore != NULL) {
        xSemaphoreGive(g_crashUploadWaitSemaphore);
    }
}

static bool asd_tools_trigger_crashdump_cb(log_upload_callback cb)
{
    //trigger a crash log upload event.
    aceLogmgr_Msg_t msg;
    // the payload is json format.
    char *payload = "{\"UPLOAD_TAG\":\"crash_log\"}";
    msg.event = ACE_LOGMGR_EVENT_UPLOAD_CRASH;
    msg.source = ACE_LOGMGR_SOURCE_ACP;
    msg.payload = payload;

    ace_status_t ret = aceLogmgr_send_msg_with_callback(&msg, cb, 0);
    if (ret != ACE_STATUS_OK) {
        printf("Failed to trigger crash log upload.\n");
        return false;
    }

    return true;
}
#endif

bool asd_tools_crashdump_sync(uint32_t upload_timeout_ms)
{
#ifdef LOGUPLOAD_ENABLED
    bool res = asd_tools_trigger_crashdump_cb(&crashUpload_cb);
    if (res) {
        // wait for cb to be called
        if (g_crashUploadWaitSemaphore == NULL) {
            return false;
        }
        // while crashdump will timeout on failure, it's timeout time is
        // very long, so time out earlier if we don't get response.
        return (xSemaphoreTake(g_crashUploadWaitSemaphore, pdMS_TO_TICKS(upload_timeout_ms)) == pdTRUE);
    }

    return res;
#else
    printf("LOG upload is disabled. Enable it by compile flag LOGUPLOAD_ENABLED first.\n");
    return false;
#endif
}

void asd_tools_trigger_upload_crashdump(void)
{
#ifdef LOGUPLOAD_ENABLED
    asd_tools_trigger_crashdump_cb(NULL);
#else
    printf("LOG upload is disabled. Enable it by compile flag LOGUPLOAD_ENABLED first.\n");
#endif
}

int32_t asd_tools_trigger_upload_log(asd_log_upload_callback_t cb)
{

#ifdef LOGUPLOAD_ENABLED
    //Perform upload of logs to the cloud
    aceLogmgr_Msg_t msg;
    // the payload is json format.
    char *payload = "{\"UPLOAD_TAG\":\"main_log\"}";

    /* trigger a reboot history flush */
    asd_reboot_manager_dump();

    ace_status_t ret = aceLogmgr_enable_upload();
    if (ret != ACE_STATUS_OK)
        goto exit;

    msg.event = ACE_LOGMGR_EVENT_UPLOAD_LOG;
    msg.source = ACE_LOGMGR_SOURCE_PUFFIN;
    msg.payload = payload;

    ret = aceLogmgr_send_msg_with_callback(&msg, (log_upload_callback) cb, 0);
exit:
    return (ret == ACE_STATUS_OK)? 0 : -1;

#else
    printf("LOG upload is disabled. Enable it by compile flag LOGUPLOAD_ENABLED first.\n");
    return -1;
#endif
}


void asd_tools_init(void)
{
    asd_tools_update_cb();
	
	/* NXP: disable asd */
//    asd_debug_init();
//    asd_debug_register_upload_crashdump_callback(asd_tools_trigger_upload_crashdump);

    g_crashUploadWaitSemaphore = xSemaphoreCreateBinary();
}

