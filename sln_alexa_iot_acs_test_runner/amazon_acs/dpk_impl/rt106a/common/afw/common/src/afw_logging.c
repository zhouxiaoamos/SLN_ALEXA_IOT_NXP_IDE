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
 * AFW logging
 *******************************************************************************
 */

#include "asd_log_platform_api.h"

/* Create all the logging modules used by AFW code */
asd_log_create_module(afw, ASD_LOG_LEVEL_DEFAULT, ASD_LOG_PLATFORM_STREAM_BM_DEFAULT);
asd_log_create_module(hal, ASD_LOG_LEVEL_DEFAULT, ASD_LOG_PLATFORM_STREAM_BM_DEFAULT);

