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
* Implement the logger config
*@File: asd_logger_config.c
********************************************************************************
*/
#include "asd_logger_config.h"
#include "asd_logger_internal_config.h"
#include "asd_log_platform_api.h"

// default asd_logger configuration. Application can define its own configuration for asd logger.
const asd_logger_config_t g_logger_default_config = {
    .level = ASD_LOG_LEVEL_DEFAULT,
    .stream_bm = ASD_LOG_PLATFORM_STREAM_BM_DEFAULT,
    .main_input_buffer_size = ASD_LOG_MAIN_BUFFER_SIZE,
    .task_priority = ASD_LOG_TASK_PRIORITY_DEFAULT,
    .task_stack_size = ASD_LOG_TASK_STACK_SIZE_DEFAULT,
    .flash_log_expire_check_timer_interval_seconds = ASD_LOG_FLASH_CHECK_TIMER_INTERVAL_SECONDS_DEFAULT,
    .flash_log_expire_first_check_seconds = ASD_LOG_FLASH_FIRST_CHECK_DELAY_SECONDS_DEFAULT,
    .flash_log_lifetime_seconds = ASD_LOG_FLASH_LIFETIME_SECONDS_DEFAULT,
    .flash_log_expire_retry_seconds = ASD_LOG_FLASH_RETRY_CHECK_DELAY_SECONDS_DEFAULT,
};

const asd_logger_config_t* asd_logger_get_default_config(void)
{
    return &g_logger_default_config;
}

