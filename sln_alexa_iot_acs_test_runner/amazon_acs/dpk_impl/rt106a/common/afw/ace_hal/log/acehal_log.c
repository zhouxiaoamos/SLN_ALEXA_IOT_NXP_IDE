/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file.This file
 * is a Modifiable File, as defined in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include <ace/hal_log.h>
#include "asd_log_api.h"
#include "asd_log_platform_api.h"

#define MAX_LOG_LINE_LENGTH 256
static int32_t ace_log_txt(aceLogHal_id_t log_id, aceLogHal_level_t priority,
                           const char* tag, const char* str)
{
    asd_log_id_t asd_log_id = ASD_LOG_ID_MAIN;
    uint8_t asd_log_level = -1;

    if (log_id == ACE_LOGHAL_ID_DM_MAIN) {
        asd_log_id = ASD_LOG_ID_3P;
    }

    switch (priority) {
        case ACE_LOGHAL_VERBOSE:
            return 0;
        case ACE_LOGHAL_DEBUG:
            asd_log_level = ASD_LOG_LEVEL_DEBUG;
            break;
        case ACE_LOGHAL_INFO:
            asd_log_level = ASD_LOG_LEVEL_INFO;
            break;
        case ACE_LOGHAL_WARN:
            asd_log_level = ASD_LOG_LEVEL_WARN;
            break;
        case ACE_LOGHAL_ERROR:
            asd_log_level = ASD_LOG_LEVEL_ERROR;
            break;
        case ACE_LOGHAL_FATAL:
            asd_log_level = ASD_LOG_LEVEL_FATAL;
            break;
        default:
            // Nothing for now
            return 0;
    }

    asd_log_options_t log_options = {
        .version = ASD_LOG_OPTION_VERSION,
        .id = (uint8_t)asd_log_id,
        .level = asd_log_level,
        .tag = tag,
        .pc = 0x0, // There's a bug where addr will point to wrong address https://issues.labcollab.net/browse/ACE-23799
        .more_options = (uint32_t)ASD_LOG_PLATFORM_STREAM_BM_DEFAULT
    };

    if (asd_log_option_filtering(NULL, &log_options)) {
        return asd_log_logger(&log_options, str);
    } else {
        return 0;
    }
}

int32_t aceLogHal_getPlatformProperty(aceLogHal_platformProperty_t property_id)
{
    switch (property_id) {
        case ACE_LOGHAL_PROPERTY_API_VER:
            return ACE_LOGHAL_API_VERSION;
        case ACE_LOGHAL_PROPERTY_LOG_TXT_MSG_MAX_LEN:
            return MAX_LOG_LINE_LENGTH;
        case ACE_LOGHAL_PROPERTY_LOG_BIN_MSG_MAX_LEN:
        case ACE_LOGHAL_PROPERTY_LOG_TXT_ENTRY_MAX_LEN:
        case ACE_LOGHAL_PROPERTY_SUPPORTED_READ_MODES:
        default:
            return ACE_STATUS_NOT_SUPPORTED;
    }
}

int32_t aceLogHal_write(aceLogHal_id_t log_buffer_id, aceLogHal_level_t prio, const char* tag,
                        const void* payload, size_t len, int flags)
{
    if (payload == NULL) {
        return ACE_STATUS_NULL_POINTER;
    }
    if ((flags != ACE_LOGHAL_TYPE_BIN) && (log_buffer_id != ACE_LOGHAL_ID_BIN)) {
        int32_t stat = ace_log_txt(log_buffer_id, prio, tag, payload);
        if(ASD_LOGGER_ERROR_LINE_LENGTH_LIMIT != stat) {
            return len;
        } else {
            return ASD_LOG_LINE_MAXSIZE;
        }
    }

    return ACE_STATUS_NOT_SUPPORTED;
}

aceLogHal_readHandle_t aceLogHal_readOpen(aceLogHal_id_t log_id, aceLogHal_readMode_t mode)
{
    return NULL;
}

int32_t aceLogHal_read(aceLogHal_readHandle_t handle, char* log_mem, size_t len)
{
    return ACE_STATUS_NOT_SUPPORTED;
}

ace_status_t aceLogHal_readClose(aceLogHal_readHandle_t handle)
{
    return ACE_STATUS_NOT_SUPPORTED;
}
