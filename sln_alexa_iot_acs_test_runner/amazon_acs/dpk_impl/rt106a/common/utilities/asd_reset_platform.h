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
 * @file asd_reset_platform.h
 *
 * @brief provides functions for reseting platform
 *******************************************************************************
 */

#include <ace/aceCli.h>

extern aceCli_moduleCmd_t asd_reset_cli_sub[];
#define ASD_RESET_CLI {"reset", "device reset option", ACE_CLI_SET_FUNC | ACE_CLI_SET_NOT_RESTRICTED, .command.subCommands=asd_reset_cli_sub},
