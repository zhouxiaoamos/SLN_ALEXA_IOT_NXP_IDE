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
 * asd debug CLI module
 *******************************************************************************
 */


#include <stdlib.h>
#include <string.h>
#include "asd_debug_cli.h"
#include "asd_debug.h"
#include "inttypes.h"

static ace_status_t asd_debug_cli_dump_tasks(int32_t len, const char *params[])
{
    if (len != 1) {
        return ACE_STATUS_CLI_FUNC_ERROR;
    }
    uint32_t flag = ASD_DEBUG_INFO_TASK_STATUS;
    if (strncmp(params[0], "all", sizeof("all")) == 0) {
        flag = ASD_DEBUG_INFO_ALL;
    } else if (strncmp(params[0], "stack", sizeof("stack")) == 0) {
        flag = ASD_DEBUG_INFO_STACK_RAW;
    }
    asd_debug_print_all_tasks(flag);
    return ACE_STATUS_OK;
}

static ace_status_t asd_debug_cli_allow_exception_reboot(int32_t len, const char *params[])
{
    if (len == 0) {
        exception_reboot_allow(true);
    } else if (len == 1) {
        exception_reboot_allow(atoi(params[0]) != 0);
    } else {
        return ACE_STATUS_BAD_PARAM;
    }
    return ACE_STATUS_OK;
}
//==============================================================================
const aceCli_moduleCmd_t asd_debug_cli[] = {
        {"dt", "dt [status/all/stack] dump information of tasks", ACE_CLI_SET_LEAF, .command.func=&asd_debug_cli_dump_tasks},
        {"allow_ereboot", "allow_ereboot [0/1] 1: allow reboot for exception, 0: not allow reboot for exception, just hang.", ACE_CLI_SET_LEAF, .command.func=&asd_debug_cli_allow_exception_reboot},
        ACE_CLI_NULL_MODULE
};
