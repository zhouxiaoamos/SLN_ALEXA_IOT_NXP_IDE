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
 * Command-Line-Interface (CLI) commands for Acorn debug module
 *******************************************************************************
 */

#pragma once

#include <ace/aceCli.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const aceCli_moduleCmd_t asd_debug_cli[];

#define ASD_DEBUG_CLI_ENTRY \
    {"dbg", "ASD debug Commands", ACE_CLI_SET_FUNC, .command.subCommands=asd_debug_cli},

#ifdef __cplusplus
}
#endif

