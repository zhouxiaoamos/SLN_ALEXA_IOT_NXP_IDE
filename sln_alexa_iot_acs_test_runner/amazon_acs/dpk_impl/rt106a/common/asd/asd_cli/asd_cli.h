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
 * Command-Line-Interface (CLI) commands Acorn
 *******************************************************************************
 */

#pragma once
#include <ace/aceCli.h>

/**
 * Defines a subcommand for the 'cli' cli command.
 */
extern const aceCli_moduleCmd_t rtos_cli_sub[];
#define RTOS_CLI {"cli", "commands to control cli behavior", ACE_CLI_SET_FUNC, .command.subCommands=rtos_cli_sub},

/*Echo the injected command. */
#define ECHO_IN_CLI

/**
 * Initializes and starts the cli (command line interface) task
 *
 * This task reads from stdin, and outputs to stdout.
 * Compile code with local echo turned on in order to see output you are typing appear on terminal
 *
 * @param moduleCmdList     Sets the command table for the CLI task to traverse through
 * @param cli_stack_size    Sets the stack size for the CLI task
 *
 * @return 0 on success, -1 on failure
 */
int asd_rtos_cli_init(const aceCli_moduleCmd_t* moduleCmdList, uint32_t cli_stack_size);

