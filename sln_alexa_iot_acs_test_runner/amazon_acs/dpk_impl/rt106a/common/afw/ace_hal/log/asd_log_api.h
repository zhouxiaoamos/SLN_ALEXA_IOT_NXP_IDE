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
* API header for asd_log. This API is exposed to ACE shim layer.
*@File: asd_log_api.h
********************************************************************************
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "asd_log_config.h"
#ifdef __cplusplus
extern "C"
{
#endif

//Maximum message per line, including tag and message body.
#define ASD_LOG_LINE_MAXSIZE 256
// Minimum buffer that used to read log. The buffer should accomodate at least
// one decoded log line, including decoded log line header, tag, and message body.
#define ASD_LOG_READ_BUF_MINSIZE 320
#define ASD_LOG_OPTION_VERSION 1

#ifndef ASD_LOG_LEVEL_DEFAULT
#define ASD_LOG_LEVEL_DEFAULT ASD_LOG_LEVEL_INFO
#endif


typedef enum {
    ASD_LOG_ID_DSP       = 1,
    ASD_LOG_ID_MAIN      = 2,
    ASD_LOG_ID_METRICS   = 3,
    ASD_LOG_ID_CRASH_LOG = 4,
    ASD_LOG_ID_3P        = 5,
    // ASD_LOG_ID_NUM should be the second of last, and never manually changed.
    // It should automatically count the number of LOG_ID.
    // LOD ID 0 is reserved for internal use.
    ASD_LOG_ID_NUM,
    ASD_LOG_ID_INVALID   = 0xFF,
} asd_log_id_t;

typedef enum {
    ASD_LOG_STREAM_CONSOLE,
    ASD_LOG_STREAM_FLASH,
    ASD_LOG_STREAM_RAM,
    ASD_LOG_STREAM_VIRTUAL,
    ASD_LOG_STREAM_NUM,
} asd_log_stream_type_t;

typedef enum {
    ASD_LOG_DATA_TYPE_LOG_TXT,
    ASD_LOG_DATA_TYPE_BIN,
    ASD_LOG_DATA_TYPE_NUM,
} asd_log_data_type_t;

//bump the logger error base to 51, to avoid the confliction with afw error (afw_error.h).
typedef enum {
    ASD_LOGGER_OK                       = 0,   ///< Sucessful process.
    ASD_LOGGER_ERROR                    =-51,  ///< General error
    ASD_LOGGER_ERROR_INVALID_PARAMETERS =-52,  ///< Invalid parameters passed in.
    ASD_LOGGER_ERROR_UNINTIALIZED       =-53,  ///< Logger or interface is unintialized.
    ASD_LOGGER_ERROR_MALLOC             =-54,  ///< Memory allocation failure.
    ASD_LOGGER_ERROR_FLASH              =-55,  ///< Flash error.
    ASD_LOGGER_ERROR_OVERFLOW           =-56,  ///< Buffer is too small, and causes buffer overflow.
    ASD_LOGGER_ERROR_STREAM_LIMIT       =-57,  ///< Input or output reaches stream limit.
    ASD_LOGGER_ERROR_TASK               =-58,  ///< Task create error.
    ASD_LOGGER_ERROR_LINE_LENGTH_LIMIT  =-59,  ///< Line length limit reached.
    ASD_LOGGER_ERROR_CRC                =-60,  ///< CRC error
    ASD_LOGGER_ERROR_SEMAPHORE          =-61,  ///< Error on semaphore.
    ASD_LOGGER_ERROR_QUEUE              =-62,  ///< Error on log reqeust queue.
    ASD_LOGGER_ERROR_FLASH_MAP          =-63,  ///< No flash map for log region.
    ASD_LOGGER_ERROR_NO_RESOURCE        =-64,  ///< No resource available.
    ASD_LOGGER_ERROR_TIMEOUT            =-65,  ///< Timeout
} asd_logger_rc_t;

typedef uint8_t asd_log_stream_bitmap_t;

// stream bit to set in asd_log_options_t.more_options.
#define ASD_LOG_OPTION_BIT_STREAM_CONSOLE  (1 << ASD_LOG_STREAM_CONSOLE)
#define ASD_LOG_OPTION_BIT_STREAM_FLASH  (1 << ASD_LOG_STREAM_FLASH)
#define ASD_LOG_OPTION_BIT_STREAM_RAM  (1 << ASD_LOG_STREAM_RAM)
#define ASD_LOG_OPTION_STREAM_CONSOLE_AND_FLASH (ASD_LOG_OPTION_BIT_STREAM_CONSOLE | ASD_LOG_OPTION_BIT_STREAM_FLASH)

// Asd log level macros.
#define ASD_LOG_LEVEL_DEBUG    0
#define ASD_LOG_LEVEL_INFO     1
#define ASD_LOG_LEVEL_WARN     2
#define ASD_LOG_LEVEL_ERROR    3
#define ASD_LOG_LEVEL_FATAL    4
#define ASD_LOG_LEVEL_NUM      5

typedef uint8_t asd_log_level_t;

typedef struct asd_log_options {
    uint8_t version;            ///< option version. asd_log will check if the version is supported.
    uint8_t id;                 ///< id indicates the id of log.
    uint8_t level;              ///< level of the log line.
    uint8_t reserved;           ///< reserve 8 bits to align 32-bit.
    const char* tag;            ///< tag pointing to the tag name string.
    uint32_t pc;                ///< program pointer value of print line.
    uint32_t more_options;      ///< additional options of the log line.
    const char* func_name;      ///< Function name. if NULL, no function name and line number;
    int   line_no;              ///< line number;
} asd_log_options_t;

// define type of log vprint callback.
typedef int32_t (*log_vprint_func_t)(const asd_log_options_t* opts, const char* message, va_list varg);

int32_t register_log_vprint_callback(log_vprint_func_t vprint_cb);

//Declare for platform specific logger; This API doesn't have filter option.
int32_t asd_log_logger(const asd_log_options_t* opts, const char* message, ...);

int32_t asd_log_logger_v(const asd_log_options_t* opts, const char* message, va_list varg);

uint32_t asd_get_pc(void);
//===========================Logging Macros=====================================

// ACE Shim layer can use ASD_LOG_BASE().
#define ASD_LOG_BASE(log_id, _level, _tag, _pc, _more_options, fmt, ...)    \
do{                                                     \
    asd_log_options_t _log_options = {0}; /*local options in stack */ \
    _log_options.id = (uint8_t) log_id;                 \
    _log_options.level = (uint8_t) _level;              \
    _log_options.tag = (const char*) _tag;              \
    _log_options.pc = (uint32_t) _pc;                   \
    _log_options.version = ASD_LOG_OPTION_VERSION;      \
    _log_options.more_options = (uint32_t)_more_options;\
    asd_log_logger(&_log_options, fmt, ##__VA_ARGS__);  \
}while(0)

// ------------------ log reader APIs ----------------------------------------

//forward declare log_reader.
struct asd_log_reader;
typedef struct asd_log_reader asd_log_reader_t;

/**
 * @brief log position definition. It defines where is the origin for the log position.
 */
typedef enum {
    LOG_POSITION_START,     ///< start of the log.
    LOG_POSITION_UNREAD,    ///< start of the unread log.
    LOG_POSITION_END        ///< end of the log.
} log_reader_origin_t;


/**
 * @brief Log data type. It is defined in bitwise. So value should be 1, 2, 4, 8, ...
 */
typedef enum {
    LOG_DATA_TYPE_BIT_BIN   = 1,      ///< The log data type is binary data.
    LOG_DATA_TYPE_BIT_TXT   = 2,      ///< The log data type is text.
} log_data_type_bit_t;

/**
 *@brief Create a log reader for log id.
 *       User can create multiple log readers. Each reader has its own context, and doesn't interfere each other.
 *
 * @param [in] log_id: log id to read.
 * @param [in] options: options to create the reader. Log data type must match the data type of log_id in flash.
 *             Check log_data_type_bit_t for more info.

 * @return the handle of the log reader.
 */
asd_log_reader_t* log_reader_create(uint8_t log_id, uint32_t options);

/**
 *@brief Move reader position to an origin.
 *
 * @param [in/out] reader: handle of the log reader.
 * @param [in] origin: position to move.  Check log_reader_origin_t for more origin information.
 *
 * @return 0 for success, or negative for error.
 */
int32_t log_reader_setpos (asd_log_reader_t* reader, log_reader_origin_t origin);

/**
 * @brief Query remaining log length from current reader position.
 *
 * @param [in] reader: log reader handle
 *
 * @return length of remaining log after the current reader position, or negtive for error.
 */
int32_t log_reader_get_remaining_log_length(asd_log_reader_t* reader);

/**
 * @brief Read the log from flash from current reader position. It always returns multiple log lines.
 *        If "size" is smaller than one line, no log data returns, but remaining_log_length is non-zero.
 *        The output is plain text including line ending. reader->position is moved to end of this reading automatically.
 *        This read only support sequential read, no random read.
 *
 * @param [in/out] reader: handle of the log reader.
 * @param[in/out] buf: data buffer pointer for output.
 * @param[in] size: buffer size. Read max size. A line is up to 256 bytes, including header. So buffer size should be no less than 256.
 *
 * @return  >= 0 for actual read size, or negative for error. If return 0, read position reaches end of the log.
*/
int32_t log_reader_read(asd_log_reader_t* reader, void* buf, uint32_t size);

/**
 * @brief check if log reader reaches end of the log. It is equivalent to Log_reader_get_remaining_log_length(reader) == 0.
 *
 * @param [in] reader: handle of the log reader.
 *
 * @return true if log reader reaches end of the log, otherwise false.
 */
bool log_reader_eof(asd_log_reader_t* reader);

/**
 * @brief mark the log is read to current position.
 *
 * @param [in/out] reader: the log reader handle. All log lines before reader->position will be marked read in flash.
 *
 * @return 0 for success, or negative for error.
 */
int32_t log_reader_mark_read_flag(asd_log_reader_t* reader);

/**
 * @brief Destroy the reader. Release the internal resources. The buf is not allocated by reader, and won’t be freed by this API.
 *
 * @param [in/out] reader: log reader handle.
 *
 * @return none
 */
void log_reader_destroy(asd_log_reader_t*  reader);

#ifdef __cplusplus
}
#endif

