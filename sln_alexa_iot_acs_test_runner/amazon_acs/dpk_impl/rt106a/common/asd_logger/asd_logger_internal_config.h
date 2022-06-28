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
*@File: asd_logger_internal_config.h
********************************************************************************
*/

#pragma once

#include <stdint.h>
#include "asd_log_api.h"
#include "time_utils.h"
// This is the config header from app include which will override some configuration here.
// App must create the header.
#include "asd_log_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef ASD_LOGGER_REQUEST_QUEUE_LENGTH
#define ASD_LOGGER_REQUEST_QUEUE_LENGTH    8
#endif
#define ASD_LINE_ENDING "\r\n"
#define ASD_LOGGER_SCRATCH_PAD_SIZE   (512)
#define ASD_LOGGER_FUNC_NAME_LENGTH_MAX (16)

//timeout for logger enqueue.
#ifndef ASD_LOGGER_ENQUEUE_TIMEOUT_MS
#define ASD_LOGGER_ENQUEUE_TIMEOUT_MS   (20)
#endif

// Uncomment the following line to enable logger debugging print.
//#define LOGGER_DPRINTF(fmt, ...)  printf("%s:%u: " fmt ASD_LINE_ENDING, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGGER_DPRINTF(fmt, ...)


#define NOT_IN_EXCLUSIVE_RANGE(x, lower, upper)  ((x) <= (lower) || (x) >= (upper))

#ifndef ASD_LOG_TASK_PRIORITY_DEFAULT
#define ASD_LOG_TASK_PRIORITY_DEFAULT   (1)
#endif

#ifndef ASD_LOG_MAIN_BUFFER_SIZE
#define ASD_LOG_MAIN_BUFFER_SIZE   (16*1024)
#endif

#ifndef ASD_LOG_TASK_STACK_SIZE_DEFAULT
#define ASD_LOG_TASK_STACK_SIZE_DEFAULT  (2*1024)
#endif

#ifndef ASD_LOG_FLASH_LIFETIME_SECONDS_DEFAULT
#define ASD_LOG_FLASH_LIFETIME_SECONDS_DEFAULT              DAYS_TO_SECONDS(14)
#endif

#ifndef ASD_LOG_FLASH_CHECK_TIMER_INTERVAL_SECONDS_DEFAULT
#define ASD_LOG_FLASH_CHECK_TIMER_INTERVAL_SECONDS_DEFAULT  HOURS_TO_SECONDS(8)
#endif

#ifndef ASD_LOG_FLASH_FIRST_CHECK_DELAY_SECONDS_DEFAULT
#define ASD_LOG_FLASH_FIRST_CHECK_DELAY_SECONDS_DEFAULT     MINUTES_TO_SECONDS(1)
#endif

#ifndef ASD_LOG_FLASH_RETRY_CHECK_DELAY_SECONDS_DEFAULT
#define ASD_LOG_FLASH_RETRY_CHECK_DELAY_SECONDS_DEFAULT     MINUTES_TO_SECONDS(5)
#endif

// ASD_LOG_ID_META must be 0.
#define ASD_LOG_ID_META              (0)

#ifndef ASD_FLASH_LOG_REGIONS_ENTRY_TABLE
//Use default log region table.
#define ASD_FLASH_LOG_REGIONS_ENTRY_TABLE   \
            {FLASH_PARTITION_LOG_MAIN,  ASD_LOG_ID_MAIN,     ASD_LOG_DATA_TYPE_LOG_TXT},   \
            {FLASH_PARTITION_LOG_CRASH, ASD_LOG_ID_CRASH_LOG,ASD_LOG_DATA_TYPE_BIN},
#endif

#ifdef __cplusplus
}
#endif
