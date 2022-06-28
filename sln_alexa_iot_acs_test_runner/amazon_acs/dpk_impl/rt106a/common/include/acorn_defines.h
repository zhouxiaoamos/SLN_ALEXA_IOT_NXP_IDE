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
#pragma once
// 32 for UUID + 4 for -'s + 1 '\0'
#define DEVICE_ID_STRING_BUFFER_SIZE (32+4+1)
#define DEVICE_TYPE_SIZE (16+1)
#define TICKS_PER_SECOND (1000/portTICK_PERIOD_MS)

// Set to a High value so that OTA to the device doesn't happen by default.
#define DEFAULT_HMCU_PACKAGE_VERSION 99999999999
#define DEFAULT_HMCU_PACKAGE_NAME    "com.amazon.abcd.host.os"
