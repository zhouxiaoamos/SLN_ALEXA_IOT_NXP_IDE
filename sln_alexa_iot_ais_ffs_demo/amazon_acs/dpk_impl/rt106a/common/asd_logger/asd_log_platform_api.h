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
* API header for asd_log_platform. This API is exposed only to platform, not ACE.
*@File: asd_log_platform api.h
********************************************************************************
*/

#pragma once

#include <stdint.h>
#include "asd_log_api.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define ASD_LOG_PLATFORM_ID      ASD_LOG_ID_MAIN

#ifndef ASD_LOG_PLATFORM_STREAM_BM_DEFAULT
#define ASD_LOG_PLATFORM_STREAM_BM_DEFAULT (ASD_LOG_OPTION_BIT_STREAM_CONSOLE | ASD_LOG_OPTION_BIT_STREAM_FLASH)
#endif

enum {
    ASD_LOG_INPUT_STREAM_MAIN,          ///< Main input stream. This is the ring buffer allocated for all MCU log.
    ASD_LOG_INPUT_STREAM_DSP,           ///< DSP log input stream. This is the ring buffer allocated in shared memory for DSP log.
    ASD_LOG_INTPUT_LOG_REQUEST,         ///< Log request queue stream.
    ASD_LOG_INPUT_STREAM_NUM,
};

// Use ASD_LOG_STREAM_NUM as asd logger output stream number.
#define ASD_LOG_OUT_STREAM_NUM  ASD_LOG_STREAM_NUM

typedef enum {
    ASD_LOG_EVENT_BIT_STREAM_MAIN   = (1<<ASD_LOG_INPUT_STREAM_MAIN),   ///< Event for sending data in main stream.
    ASD_LOG_EVENT_BIT_STREAM_DSP    = (1<<ASD_LOG_INPUT_STREAM_DSP),    ///< Event for sending data in DSP stream.
    ASD_LOG_EVENT_BIT_LOG_REQUEST   = (1<<ASD_LOG_INTPUT_LOG_REQUEST),    ///< Event for sending log request.
    ASD_LOG_EVENT_BIT_ALL = (1<<ASD_LOG_INPUT_STREAM_NUM) - 1,
} asd_log_event_bit_t;
#define ASD_LOG_EVENT_NUM    3          ///< number of LOG events defined in asd_log_event_bit_t.

/**
 * @brief log module context for filtering.
 */
typedef struct asd_log_control_block{
    const char *module_name;             ///< module name, string.
    asd_log_level_t level;             ///< verbosity of log control.
    asd_log_stream_bitmap_t ostream_bm;  ///< bitmap of output stream.
} asd_log_control_block_t;

/**
 * @brief create log module filter
 */
#define asd_log_create_module(_module, _filter_level, _filter_stream_bm)  \
    asd_log_control_block_t asd_log_control_block_##_module                 \
    __attribute__((section("asd_log_module_filters"))) =        \
    {                                                                       \
        .module_name = #_module,                                            \
        .level = (_filter_level),                                             \
        .ostream_bm = (_filter_stream_bm),                                  \
    }



#define asd_log_module_set_verbose(_module, _filter_verbose)                \
        asd_log_control_block_##_module.verbose = _filter_verbose

#define asd_log_module_set_stream_bitmap(_module, _filter_stream_bm)        \
        asd_log_control_block_##_module.ostream_bm = _filter_stream_bm

//Declare for platform specific logger; This API doesn't have filter option.
void asd_log_base_logger(asd_log_control_block_t* module, uint8_t logid, uint8_t level, uint32_t pc, uint32_t more_options, const char* fmt, ...);
#define ASD_LOG_PLATFORM_BASE(module, logid, level,  pc, more_options, fmt, ...) \
        asd_log_base_logger(&module, logid, level,  pc, more_options, fmt, ##__VA_ARGS__)


// LOG macro for platform usage.
#if ASD_LOG_COMPILE_LEVEL <= ASD_LOG_LEVEL_DEBUG
#define ASD_LOG_D(_module, fmt, ...)                \
do { \
        extern asd_log_control_block_t asd_log_control_block_##_module; \
        ASD_LOG_PLATFORM_BASE(asd_log_control_block_##_module, ASD_LOG_PLATFORM_ID, ASD_LOG_LEVEL_DEBUG, asd_get_pc(), ASD_LOG_PLATFORM_STREAM_BM_DEFAULT, fmt, ##__VA_ARGS__); \
} while (0)
#else
#define ASD_LOG_D(_module, fmt, ...) {}
#endif

#if ASD_LOG_COMPILE_LEVEL <= ASD_LOG_LEVEL_INFO
#define ASD_LOG_I(_module, fmt, ...)                \
do { \
        extern asd_log_control_block_t asd_log_control_block_##_module; \
        ASD_LOG_PLATFORM_BASE(asd_log_control_block_##_module, ASD_LOG_PLATFORM_ID, ASD_LOG_LEVEL_INFO, asd_get_pc(), ASD_LOG_PLATFORM_STREAM_BM_DEFAULT, fmt, ##__VA_ARGS__); \
} while (0)
#else
#define ASD_LOG_I(_module, fmt, ...) {}

#endif

#if ASD_LOG_COMPILE_LEVEL <= ASD_LOG_LEVEL_WARN
#define ASD_LOG_W(_module, fmt, ...)                \
do { \
        extern asd_log_control_block_t asd_log_control_block_##_module; \
        ASD_LOG_PLATFORM_BASE(asd_log_control_block_##_module, ASD_LOG_PLATFORM_ID, ASD_LOG_LEVEL_WARN, asd_get_pc(), ASD_LOG_PLATFORM_STREAM_BM_DEFAULT, fmt, ##__VA_ARGS__); \
} while (0)
#else
#define ASD_LOG_W(_module, fmt, ...) {}
#endif

#if ASD_LOG_COMPILE_LEVEL <= ASD_LOG_LEVEL_ERROR
#define ASD_LOG_E(_module, fmt, ...)                \
do { \
        extern asd_log_control_block_t asd_log_control_block_##_module; \
        ASD_LOG_PLATFORM_BASE(asd_log_control_block_##_module, ASD_LOG_PLATFORM_ID, ASD_LOG_LEVEL_ERROR, asd_get_pc(), ASD_LOG_PLATFORM_STREAM_BM_DEFAULT, fmt, ##__VA_ARGS__); \
} while (0)
#else
#define ASD_LOG_E(_module, fmt, ...)  {}
#endif

#if ASD_LOG_COMPILE_LEVEL <= ASD_LOG_LEVEL_FATAL
#define ASD_LOG_F(_module, fmt, ...)                \
do { \
        extern asd_log_control_block_t asd_log_control_block_##_module; \
        ASD_LOG_PLATFORM_BASE(asd_log_control_block_##_module, ASD_LOG_PLATFORM_ID, ASD_LOG_LEVEL_FATAL, asd_get_pc(), ASD_LOG_PLATFORM_STREAM_BM_DEFAULT, fmt, ##__VA_ARGS__); \
} while (0)
#else
#define ASD_LOG_F(_module, fmt, ...) {}
#endif

extern bool asd_log_option_filtering(const asd_log_control_block_t* module_filter, asd_log_options_t* options);

#ifdef __cplusplus
}
#endif

