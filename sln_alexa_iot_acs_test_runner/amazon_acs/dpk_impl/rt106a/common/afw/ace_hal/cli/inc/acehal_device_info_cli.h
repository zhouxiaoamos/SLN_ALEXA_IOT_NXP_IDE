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

#ifndef ACEHAL_DEVICE_INFO_CLI_H
#define ACEHAL_DEVICE_INFO_CLI_H

#include <ace/aceCli.h>

extern const aceCli_moduleCmd_t acehal_device_info_cli[];

#define ACE_DEVICE_INFO_HAL_CLI_ENTRY {"device_info", "Controls device info", ACE_CLI_SET_FUNC, .command.subCommands=acehal_device_info_cli},

#endif /* ACEHAL_DEVICE_INFO_CLI_H */
