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
 * @File: asd_crashdump.h
 *
 *******************************************************************************
 */

#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "asd_debug.h"
#include "asd_flash_buffer.h"


#ifdef __cplusplus
extern "C" {
#endif


#define ASD_CD_VERSION        1

typedef enum {
    ASD_CD_TYPE_MINIDUMP,
    ASD_CD_TYPE_LOG,
} asd_crashdump_type_t;

typedef enum {
    ASD_CD_FRAME_CRASH_META,
    ASD_CD_FRAME_CALL_STACK,
    ASD_CD_FRAME_CPU_REG_DUMP,
    ASD_CD_FRAME_STACK_DUMP,
    ASD_CD_FRAME_LOG,
    ASD_CD_FRAME_NUM,
} asd_crashdump_frame_index_t;

typedef enum {
    ASD_CD_RAW_HEADER,
    ASD_CD_RAW_MINI,
} asd_crashdump_section_t;


typedef struct {
    uint64_t magic_num;          ///< magic number for stack dump.
    uint32_t length;
    uint8_t  version;
    uint8_t  type;
    uint16_t crc;
} __attribute__((packed)) asd_crashdump_header_t;

_Static_assert(sizeof(asd_crashdump_header_t)%4 == 0, "asd_crashdump_header_t must be dword aligned!!!");

typedef struct {
    asd_crashdump_header_t header;    ///< header for mini dump.
    asd_crashdump_mini_t mini_dump;   ///< minidump in binary.
    asd_crashdump_header_t log_header;    ///< header for additional crash log.
    char log[];        ///< log is variable size, that can fill between offset
                       ///< CRASHDUMP_MINI_TOTAL_SIZE and LOG_CRASH_LOG_SIZE.
} __attribute__((packed)) asd_crashdump_map_t;

#define CRASHDUMP_MINI_TOTAL_SIZE (sizeof(asd_crashdump_header_t) + sizeof(asd_crashdump_mini_t))
#ifdef LOG_CRASH_LOG_SIZE
_Static_assert(CRASHDUMP_MINI_TOTAL_SIZE <= LOG_CRASH_LOG_SIZE, "mini crashdump too large");
#endif

#define CRASHDUMP_LOG_SIZE_MAX (LOG_CRASH_LOG_SIZE - offsetof(asd_crashdump_map_t, log))

/**
 * @brief initialize crashdump manager.
 *
 * @return 0 for success, check afw_error for errors.
 */
int32_t asd_crashdump_init(void);

/**
 * @brief read raw data of crashdump from flash.
 *
 * @param [in] section: section of crashdump to read.
 * @param [in] size: buffer size.
 * @param [out] buf: pointer of buffer to fill raw data.
 *
 * @return bytes read from crash dump, or negative for error.(-afw_error)
 */
int32_t asd_crashdump_read_raw(asd_crashdump_section_t section, uint32_t size, void* buf);


/**
 * @brief erase crash dump region in flash.
 *
 * @return 0 for success, negative for error. (-afw_error)
 */
int32_t asd_crashdump_erase(void);

/**
 * @brief check if crash dump is available in flash. It will read the dumped raw data and
 *        check CRC.
 *
 * @return true if crash dump is available in flash, otherwise false.
 */
bool asd_crashdump_is_available(void);

/**
 * @brief read frame data (decoded in text) of crashdump from flash.
 *
 * @param [in] frame_info: pointer of frame information.
 * @param [in] offset: offset in the frame.
 * @param [out] buf: pointer of buffer to fill frame text.a
 * @param [in] size: size of buffer.
 *
 * @return bytes read decoded frame data, or negative for error.(-afw_error)
 */
int32_t asd_crashdump_read_frame(
    const asd_frame_info_t* frame_info,
    uint32_t offset,
    char *buf,
    size_t size);

/**
 * @brief get the total length of all crashdump frames.
 *
 * @return bytes of all crashdump frames, or negative for error. (-afw_error)
 */
int32_t asd_crashdump_get_total_frame_length(void);

/**
 * @brief Get the information of first frame in crash dump.
 * @param [out] first: pointer of first frame info.
 *
 * @return 0 for success, negative for error. (-afw_error)
 */
int32_t asd_crashdump_get_first_frame(asd_frame_info_t* first);

/**
 * @brief Get the information of the next frame in crash dump.
 * @param [in] cur: pointer of current frame info.
 * @param [out] next: pointer of the next frame info.
 *
 * @return 0 for success, negative for error. (-afw_error)
 */
int32_t asd_crashdump_get_next_frame(const asd_frame_info_t* cur, asd_frame_info_t* next);

/**
 * @brief Save the minidump in flash.
 * @param [in] minidump: pointer of minidump in RAM.
 *
 * @return none.
 */
void asd_crashdump_save(const asd_crashdump_mini_t *minidump);

/**
 * @brief text name of crash dump reason.
 *
 * @param [in] rs: reason code.
 * @return pointer to the text of crash dump reason.
 */
const char* asd_crashdump_reason_name(uint32_t rs);

/**
 * @brief text metrics key of crash dump reason.
 *
 * @param [in] reason code.
 * @return pointer to the metrics key of crash dump.
 */
const char* asd_crashdump_metrics_key(uint32_t rs);

/**
 * @brief Test if the header is a valid crashdump header.
 *
 * @param [in] header: pointer to header.
 * @return true for a valid header; otherwise, false.
 */
bool asd_crashdump_header_is_valid(const asd_crashdump_header_t* header);

/**
 * @brief get the register name in text.
 *
 * @param [in] reg_index: register index, 0:r0, 1:r1,....
 * @return const pointer to the name of register.
 */
const char* asd_get_cpu_reg_name(uint32_t reg_index);


/**
 * @brief Dump exception error message.
 * @param [in] reason: reason to raise exception.
 * @param [in] ex_reg: SCB registers for exception.
 * @param [in] print: print function pointer.
 *
 * @return none.
 */
void print_exception_info(uint32_t reason,
                               const asd_exception_reg_t* ex_reg,
                               asd_print_func_t print);

/**
 * @param[out] char*  Buffer to hold a string that will be populated in the function
 * @param[in]  size_t   should be the max length for the returned string
 * @return  size of data
 */
typedef size_t(app_version_info_callback)(char*, size_t);

/**
 * @brief Install version info callback.
 * @param [in] cb: callback to get version information
 *
 * @return none.
 */
void crashdump_register_version_info_callback(app_version_info_callback *cb);
#ifdef __cplusplus
}
#endif
