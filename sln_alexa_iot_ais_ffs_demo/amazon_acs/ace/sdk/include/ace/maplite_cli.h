/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef ACE_MAPLITE_CLI_H
#define ACE_MAPLITE_CLI_H

#include <ace/aceCli.h>

#define MAPLITE_CLI_STACK_SIZE_KB 6

ace_status_t ace_maplite_register_cli(int32_t len, const char* param[]);

extern int device_register_cli_main(int len, const char** param);

#endif /* ACE_MAPLITE_CLI_H */
