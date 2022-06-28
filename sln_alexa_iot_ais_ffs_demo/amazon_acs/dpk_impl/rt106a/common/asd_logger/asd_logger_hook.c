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
* Implement the real logger hooks used by asd_log. These APIs will overwrite the
* weak version in asd_log library.
*@File: asd_logger_hook.c
********************************************************************************
*/

#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "asd_log_api.h"
#include "asd_logger_if.h"
#include "asd_log_platform_api.h"

extern asd_logger_rc_t asd_logger_vprint_console(const asd_log_options_t* opts, const char* fmt, va_list varg);

// __builtin_return_address(0) is the return address of the function,
// and asd_get_pc() must be a non-inline function to get the correct
// PC at the point where asd_get_pc() is called.
uint32_t __attribute__((noinline)) asd_get_pc(void)
{
    return (uint32_t) __builtin_return_address(0);
}

int32_t asd_log_logger(const asd_log_options_t* opts, const char* message, ...)
{
        va_list ap;
        va_start(ap, message);
        int32_t rc = asd_log_logger_v(opts, message, ap);
        va_end(ap);
        return rc;
}

void asd_log_base_logger(asd_log_control_block_t* module, uint8_t logid, uint8_t level, uint32_t pc, uint32_t more_options, const char* fmt, ...)
{
    asd_log_options_t log_options = {0}; /*local options in stack */
    log_options.id = logid;
    log_options.level = level;
    log_options.pc = pc;
    log_options.tag = module->module_name;
    log_options.version = ASD_LOG_OPTION_VERSION;
    log_options.more_options = more_options;
    if (asd_log_option_filtering(module, &log_options)) {
        va_list ap;
        va_start(ap, fmt);
        int32_t rc = asd_log_logger_v(&log_options, fmt, ap);
        va_end(ap);
    }
}


int32_t asd_log_logger_v(const asd_log_options_t* opts, const char* fmt, va_list varg)
{
    // NXP: use our own print hook
    char logbuf[configLOGGING_MAX_MESSAGE_LENGTH] = {0};
    vsnprintf(logbuf, configLOGGING_MAX_MESSAGE_LENGTH, fmt, varg);

    configPRINTF(("%s\r\n", logbuf));

    return 0;
}

bool asd_log_option_filtering(const asd_log_control_block_t* module_filter, asd_log_options_t* opts)
{
    return asd_logger_option_filter(asd_logger_get_handle(), module_filter, opts);
}
