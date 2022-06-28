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
* The header fo asd log message, it is pused in RAM log buffer.
*
*@File: asd_log_msg.h
********************************************************************************
*/

#pragma once

#include <stdint.h>
#include "asd_log_api.h"
#include "common_macros.h"
#ifdef __cplusplus
extern "C"
{
#endif

//Set log message header
#define ASD_LOG_MSG_SET(_pmsg, _stream_bm, _logid, _type)   \
    (_pmsg)->stream_bitmap = _stream_bm;                    \
    (_pmsg)->log_id = _logid;                               \
    (_pmsg)->type = _type

//Set log line header
#define ASD_LOG_LINE_HEADER_SET(_pline_header, _ts, _level, _tid, _pc)    \
    (_pline_header)->timestamp_ms = _ts;                           \
    (_pline_header)->level = _level;                               \
    (_pline_header)->task_id = _tid;                               \
    (_pline_header)->pc = _pc

//Get the log line body length, excluding message header.
#define GET_LOG_LINE_BODY_LENGTH(_plog_msg)      \
    (_plog_msg->length - sizeof(asd_log_msg_t) - sizeof(asd_log_line_header_t))

//Get the binary log line length, including binary line header.
#define GET_LOG_LINE_BIN_LENGTH(_plog_msg)      \
    (_plog_msg->length - sizeof(asd_log_msg_t))

// log message header definition.
typedef struct {
    uint16_t length;                        ///< length of the message. Including "length".
    uint16_t stream_bitmap          : 8;    ///< Bitmap for output streams. Up to 8 output streams.
    uint16_t log_id                 : 4;    ///< Support upto 16 log_id, including DSP log, MCU log, crash log, vital.
    uint16_t type                   : 2;    ///< Type of log data. RAW: raw binary data. LOG_TXT: text based log line, and its header.
    uint16_t reserved               : 2;
    uint8_t  data[];                        ///< log data; line data or binary data.
} __attribute__((packed)) asd_log_msg_t;
//log message header is by design, any size change needs review.
COMPILE_TIME_ASSERT(sizeof(asd_log_msg_t) == 4);

//log line header structure.
typedef struct {
    uint64_t timestamp_ms           : 42;   ///< Time stamp of the log line. In Milliseconds. 42 bits integer can hold about 139 years in milliseconds from epoch time.
    uint64_t level                  : 3;    ///< Log verbosity level, up to 8 levels.
    uint64_t reserved               : 11;
    uint64_t task_id                : 8;    ///< Task ID the log belongs to. Up to 255.
    uint32_t pc;                            ///< Program counter for the print line. This is equivalent to filename + line.
    uint8_t  data[];                        ///< Log_body contains one line of log print, and should not have line ending.
} __attribute__((packed)) asd_log_line_header_t;
//log line header is by design, any size change needs review.
COMPILE_TIME_ASSERT(sizeof(asd_log_line_header_t) == 12);

#define ASD_LOG_LINE_HEADER_BIN_SIZE (sizeof(asd_log_line_header_t))
// length of text length header. This value must be fixed.
#define ASD_LOG_LINE_HEADER_TXT_SIZE (40)
// size delta of binary line header and text line header.
#define ASD_LOG_LINE_HEADER_SIZE_DELTA (ASD_LOG_LINE_HEADER_TXT_SIZE - ASD_LOG_LINE_HEADER_BIN_SIZE)



#ifdef __cplusplus
}
#endif
