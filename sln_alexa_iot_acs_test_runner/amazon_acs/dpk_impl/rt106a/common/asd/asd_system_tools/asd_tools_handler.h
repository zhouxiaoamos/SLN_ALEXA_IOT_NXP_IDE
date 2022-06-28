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
 * @File: asd_tools_handler.h
 */

#pragma once

#include <stdint.h>

/* Suggested 30 sec timeout for asd_tools_crashdump_sync */
#define LOG_UPLOAD_TIMEOUT_MS 30000

typedef void (*asd_log_upload_callback_t)(uint32_t id, uint8_t event);

/**
 * @brief set ace tool handler callback for stack corruption and heap
 *        corruption.
 */
void asd_tools_update_cb(void);

/**
 * @brief trigger crash dump upload event to logmgr.
 */
void asd_tools_trigger_upload_crashdump(void);

/**
 * @brief same as asd_tools_trigger_upload_crashdump
 * but doesn't return until crash is uploaded (or fails/times out)
 *
 * Note this is not threadsafe and may return early if another task
 * triggered.
 *
 * @param upload_timeout_ms max time to wait
 *          (if use max note, log upload has it's own timeout that will hit).
 */
bool asd_tools_crashdump_sync(uint32_t upload_timeout_ms);

/**
 * @brief trigger log upload event to logmgr.
 */
int32_t asd_tools_trigger_upload_log(asd_log_upload_callback_t cb);

/**
 * @brief initialize asd tools.
 */
void asd_tools_init(void);
