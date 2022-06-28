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

#include <ace/aceCli.h>
#include <asd_cli.h>

#include "test_iot_cli.h"

#include "ace_cli_config.h"
#include <ace/ace_config.h>
#include <ace/ace_cli_cmds.h>
#include "ace_cli.h"
#include <acehal_device_info_cli.h>

#include "asd_debug_cli.h"
#include "asd_reset_platform.h"
#include "asd_logger_cli.h"
#include "time_utils_cli.h"

#ifdef IOT_DEVICE_TESTER_ENABLE
#include "iot_device_tester.h"
#endif

// clang-format off
const aceCli_moduleCmd_t ACE_CLI_MODULE_CMDS[] = {
                        RTOS_CLI
                        ACE_RTOS_CLI_CMDS
                        ACE_DEVICE_INFO_HAL_CLI_ENTRY
                        //ACE_HEAP_TRACK_CLI_ENTRY
                        //LOG_CLI_ENTRY
                        ACE_WIFI_CLI_ENTRY
                        ASD_RESET_CLI
                        AFW_TIME_CLI_ENTRY
#ifdef AMAZON_TESTS_ENABLE
#ifdef IOT_DEVICE_TESTER_ENABLE
//                        IOT_DEVICE_TESTER_CLI
#endif
                        IOT_TESTS_CLI
#endif
                        ACE_CLI_NULL_MODULE
};
// clang-format on