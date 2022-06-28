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
* The header for asd_logger configuration. It defines the asd logger configuration.
*
*@File: asd_logger_config.h
********************************************************************************
*/

#pragma once

#include <stdint.h>
#include "asd_log_api.h"

typedef struct asd_logger_config{
    //Frontend config:
    asd_log_level_t level;
    asd_log_stream_bitmap_t stream_bm;
    uint32_t main_input_buffer_size;    ///< buffer size for main input.

    //backend config:
    //priority for log task
    int task_priority;
    int task_stack_size;

    uint32_t flash_log_expire_check_timer_interval_seconds;
    uint32_t flash_log_expire_first_check_seconds;
    uint32_t flash_log_expire_retry_seconds;
    // The number of seconds a flash log line is allowed to remain before getting cleared.
    uint32_t flash_log_lifetime_seconds;

} asd_logger_config_t;

const asd_logger_config_t* asd_logger_get_default_config(void);
