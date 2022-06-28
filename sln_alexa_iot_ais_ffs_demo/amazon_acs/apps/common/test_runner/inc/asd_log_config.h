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

#pragma once

// Override asd logger configuration.

// Log request queue length. This is the max request number that log request
// queue can hold.
#define ASD_LOGGER_REQUEST_QUEUE_LENGTH (8)
// log task priority
#define ASD_LOG_TASK_PRIORITY_DEFAULT (1)
// client log enqueue timeout (ms)
#define ASD_LOGGER_ENQUEUE_TIMEOUT_MS (20)
// RAM buffer size for log print.
#define ASD_LOG_MAIN_BUFFER_SIZE (4 * 1024)  // 4K buffer

#define ASD_LOG_TASK_STACK_SIZE_DEFAULT (3 * 1024)

// in test_runner config, keep log level as Debug.
#define ASD_LOG_LEVEL_DEFAULT ASD_LOG_LEVEL_DEBUG

#ifdef FLASH_PARTITION_LOG_3P
// Redefine the log region mapping table if 3P log is supported.
//                  Partition_name,         LOG_ID,           Data_type
// clang-format off
#define ASD_FLASH_LOG_REGIONS_ENTRY_TABLE   \
            {FLASH_PARTITION_LOG_MAIN,  ASD_LOG_ID_MAIN,     ASD_LOG_DATA_TYPE_LOG_TXT},   \
            {FLASH_PARTITION_LOG_CRASH, ASD_LOG_ID_CRASH_LOG,ASD_LOG_DATA_TYPE_BIN},       \
            {FLASH_PARTITION_LOG_3P,    ASD_LOG_ID_3P,       ASD_LOG_DATA_TYPE_LOG_TXT},
// clang-format on
#endif
