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

#ifndef ACE_CLI_H
#define ACE_CLI_H

#include <ace/ace_config.h>
#include <ace/aceCli.h>

 /*
  * FIXME: This is a temporary work around until all MWs are
  * rightly made configurable. Unconditionally include DHA
  * and KVS until top level ACE configurations are added.
  * EPIC that tracks CLI migration: ACE-5375
  */
#define ACE_KVS
#define ACE_LED

#ifdef ACE_ATT_TOOL
#include "ace/att_cli.h"
#define ACE_ATT_CLI_ENTRY \
    {"att", "ATT", ACE_CLI_SET_LEAF, .command.func=ace_att_cli},
#else
#define ACE_ATT_CLI_ENTRY
#endif

#ifdef ACE_MAPLITE_MIDDLEWARE
#include "ace/maplite_cli.h"
#define ACE_MAPLITE_CLI_ENTRY \
    {"ace_maplite", "ACE Maplite commands", ACE_CLI_SET_LEAF | ACE_CLI_SET_EXEC_SPAWN, .command.func=ace_maplite_register_cli, .stack_size_kb = MAPLITE_CLI_STACK_SIZE_KB},
#else
#define ACE_MAPLITE_CLI_ENTRY
#endif

#ifdef ACE_WIFI_MIDDLEWARE
extern const aceCli_moduleCmd_t ace_wifi_cli[];
#define ACE_WIFI_CLI_ENTRY {"ace_wifi", "ACE Wifi commands", ACE_CLI_SET_FUNC, .command.subCommands=ace_wifi_cli},
#else
#define ACE_WIFI_CLI_ENTRY
#endif

#ifdef ACE_HEAP_TRACK
#include <ace/heap_track_cli.h>
#define ACE_HEAP_TRACK_CLI_ENTRY \
    {"heap_track", "Track Heap Allocs", ACE_CLI_SET_FUNC, .command.subCommands=ace_heap_track_cli},
#else
#define ACE_HEAP_TRACK_CLI_ENTRY
#endif

#ifdef ACE_STACK_CANARY
#include <ace/stack_canary_cli.h>
#define ACE_STACK_CANARY_CLI_ENTRY \
    {"stack_canary", "Stack Canary", ACE_CLI_SET_FUNC, .command.subCommands=ace_stack_canary_cli},
#else
#define ACE_STACK_CANARY_CLI_ENTRY
#endif

#ifdef ACE_HEAP_GUARD
#include <ace/heap_guard_cli.h>
#define ACE_HEAP_GUARD_CLI_ENTRY \
    {"heap_guard", "Heap Guard", ACE_CLI_SET_FUNC, .command.subCommands=ace_heap_guard_cli},
#else
#define ACE_HEAP_GUARD_CLI_ENTRY
#endif

#ifdef AFR_BDTOOL_CLI
#include <ace/afr_bdtool_cli.h>
#define AFR_BDTOOL_CLI_ENTRY \
    {"afr_bdtool", "AFR BDTool CLI", ACE_CLI_SET_FUNC, .command.subCommands=afr_bdtool_cli},
#else
#define AFR_BDTOOL_CLI_ENTRY
#endif

#define ACE_CLI_CMDS            ACE_KVS_CLI_ENTRY   \
                                ACE_MAPLITE_CLI_ENTRY \
                                ACE_WIFI_CLI_ENTRY  \
                                ACE_HEAP_TRACK_CLI_ENTRY \
                                ACE_STACK_CANARY_CLI_ENTRY \
                                ACE_HEAP_GUARD_CLI_ENTRY \
                                ACE_ATT_CLI_ENTRY \
                                AFR_BDTOOL_CLI_ENTRY \
//  Keep this comment here. Makes for easier merges when this list has merge conflicts
#endif  // ACE_CLI_H
