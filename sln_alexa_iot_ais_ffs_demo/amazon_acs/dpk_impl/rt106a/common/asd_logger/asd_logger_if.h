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
* The header for asd_logger interface in platform.
* It defines the interface of asd logger. External libraries can include the header,
* and call the interface API.
*@File: asd_logger_if.h
********************************************************************************
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "asd_log_api.h"
#include "asd_log_platform_api.h"
#ifdef __cplusplus
extern "C"
{
#endif

struct afw_stream_s;
typedef struct afw_stream_s afw_stream_t;

struct asd_log_stream;
typedef struct asd_log_stream asd_log_stream_t;
struct asd_logger_config;
typedef struct asd_logger_config asd_logger_config_t;
struct asd_log_request;
typedef struct asd_log_request asd_log_request_t;

typedef enum {
    ASD_LOGGER_STREAM_INPUT,
    ASD_LOGGER_STREAM_OUTPUT,
    ASD_LOGGER_STREAM_DIRECTION_NUM,
} asd_logger_stream_direction_t;

struct asd_logger_if;

typedef struct asd_logger_if asd_logger_if_t;

typedef struct asd_logger_if {
    uint8_t initialized;
    asd_logger_rc_t (*write)(
        const asd_logger_if_t* logger, const asd_log_options_t* options, const uint8_t* data, int32_t length);
    asd_logger_rc_t (*vprint)(
        const asd_logger_if_t* logger, const asd_log_options_t* options, const char* fmt, va_list vargs);
    bool (*option_filter)(
        const asd_logger_if_t* logger, const asd_log_control_block_t* module_filter, asd_log_options_t* options);
    asd_logger_rc_t (*register_stream)(
        const asd_logger_if_t* logger, asd_logger_stream_direction_t direction,
        afw_stream_t* stream, uint8_t index);
    asd_logger_rc_t (*set_log_filter)(const asd_logger_if_t* logger, const char* module_name,
                                    const asd_log_control_block_t* filter);
    asd_logger_rc_t (*get_log_filter)(const asd_logger_if_t* logger, const char* module_name,
                                      asd_log_control_block_t* filter);
    int32_t (*send_request)(const asd_logger_if_t* logger, asd_log_request_t* request, uint32_t timeout_tick);
    int32_t (*wait_completion)(const asd_logger_if_t* logger, asd_log_request_t* request, uint32_t timeout_tick);

} asd_logger_if_t;

/**
 * @brief get the global asd logger handle.
 * @param none
 *
 * @return asd logger handle. Only singleton.
 */
asd_logger_if_t* asd_logger_get_handle(void);

/**
 * @brief Write raw data directly to the logger service. The raw data is written in one frame.
 * @param [in] logger: pointer to logger.
 * @param [in] options: options of the data.
 * @param [in] data: pointer to the data to write.
 * @param [in] length: data length.
 *
 * @return Check asd_logger_rc_t for return values.
 */
static inline asd_logger_rc_t asd_logger_write(
    const asd_logger_if_t* logger,
    const asd_log_options_t* options,
    const uint8_t* data,
    int32_t length)
{
    if (!logger || !logger->initialized || !logger->write) return ASD_LOGGER_ERROR_UNINTIALIZED;
    return logger->write(logger, options, data, length);
}

/**
 * @brief Logs a message with format args
 * @param [in] logger: pointer to logger.
 * @param [in] options: options of the data.
 * @param [in] fmt: print format string.
 * @param [in] vargs: format arguments to pass
 *
 * @return Check asd_logger_rc_t for return values.
 */
static inline asd_logger_rc_t asd_logger_vprint(
    const asd_logger_if_t* logger,
    const asd_log_options_t* options,
    const char* fmt, va_list vargs)
{
    if (!logger || !logger->initialized || !logger->vprint) return ASD_LOGGER_ERROR_UNINTIALIZED;
    return logger->vprint(logger, options, fmt, vargs);
}

/**
 * @brief filter the option, and set the filtered option from input.
 * @param [in] logger: logger pointer.
 * @param [in] module_filter: pointer to the module filter struct.
 * @param [in/out] options: pointer of log message option. The function will set the filtered
 *                      in the variable.
 * @return true: the log message needs to process. false: the log message is filtered, and no need to
 *         go further process, e.g. log print can skip.
 */
static inline bool asd_logger_option_filter(
    const asd_logger_if_t* logger,
    const asd_log_control_block_t* module_filter,
    asd_log_options_t* options)
{
    if (!logger || !logger->initialized || !logger->option_filter) return true;
    return logger->option_filter(logger, module_filter, options);
}


/**
 * @brief Register a stream for input or output.
 * @param [in] logger: pointer to logger.
 * @param [in] direction: input or output stream direction.
 * @param [in] stream: afw stream pointer.
 *
 * @return Check asd_logger_rc_t for return values.
 */
static inline asd_logger_rc_t asd_logger_register_stream(
    const asd_logger_if_t* logger,
    asd_logger_stream_direction_t direction,
    afw_stream_t *stream,
    uint8_t index)
{
    if (!logger || !logger->initialized || !logger->option_filter) return ASD_LOGGER_ERROR_UNINTIALIZED;
    return logger->register_stream(logger, direction, stream, index);
}

/**
 * @brief Set the log filter for global log system, or individual log module.
 * @param [in] logger: logger pointer.
 * @param [in] module_name: module name, if NULL, means logger global filter.
 * @param [in] filter: pointer of filter to set.
 *
 * @return Check asd_logger_rc_t for return values.
 */
static inline asd_logger_rc_t asd_logger_set_log_filter(
    const asd_logger_if_t* logger, const char* module_name,
    const asd_log_control_block_t* filter)
{
    if (!logger || !logger->initialized || !logger->set_log_filter)
        return ASD_LOGGER_ERROR_UNINTIALIZED;
    return logger->set_log_filter(logger, module_name, filter);
}



/**
 * @brief Get the log filter from global log system, or individual log module.
 * @param [in] logger: logger pointer.
 * @param [in] module_name: module name, if NULL, means logger global filter.
 * @param [out] filter: pointer of filter to get.
 *
 * @return  Check asd_logger_rc_t for return values.
 */
static inline asd_logger_rc_t asd_logger_get_log_filter(
    const asd_logger_if_t* logger, const char* module_name,
    asd_log_control_block_t* filter)
{
    if (!logger || !logger->initialized || !logger->get_log_filter)
        return ASD_LOGGER_ERROR_UNINTIALIZED;
    return logger->get_log_filter(logger, module_name, filter);
}


/**
 * @brief Send a request to logger.
 * @param [in] logger: logger pointer.
 * @param [in/out] request: pointer of request.
 * @param [in] timeout_tick: ticks to timeout.
 *
 * @return Check asd_logger_rc_t for return values.
 */
static inline int32_t asd_logger_send_request(
              const asd_logger_if_t* logger, asd_log_request_t* request, uint32_t timeout_tick)
{
    if (!logger || !logger->initialized || !logger->send_request) return ASD_LOGGER_ERROR_UNINTIALIZED;
    return logger->send_request(logger, request, timeout_tick);
}

/**
 * @brief Wait completion of a request.
 * @param [in] logger: logger pointer.
 * @param [in/out] request: pointer of request.
 * @param [in] timeout_tick: ticks to timeout.
 *
 * @return Check asd_logger_rc_t for return values.
 */
static inline int32_t asd_logger_wait_request_completion(
              const asd_logger_if_t* logger, asd_log_request_t* request, uint32_t timeout_tick)
{
    if (!logger || !logger->initialized || !logger->wait_completion) return ASD_LOGGER_ERROR_UNINTIALIZED;
    return logger->wait_completion(logger, request, timeout_tick);
}


/**
 * @brief This initializes and starts the logger service
 *        This should only be called once
 * @param [in/out] logger: logger pointer.
 *
 * @return Check asd_logger_rc_t for return values.
 */
asd_logger_rc_t asd_logger_init(asd_logger_if_t* logger,
                const asd_logger_config_t* config);

/**
 * @brief This shuts down the log manager service handling memory
 *        freeing for any messages in queue
 * @param [in/out] logger: logger pointer.
 *
 * @return None.
 */
void asd_logger_deinit(asd_logger_if_t* logger);

/**
 * @brief dump all module filter setting to console.
 * @param [in] logger: logger pointer
 *
 * @return none.
 */
void asd_logger_dump_module_filters(asd_logger_if_t* logger);

/**
 * @brief Erase all flash log for log_id. Called by user task.
 * @param [in] log_id: log id.
 * @param [in] timeout_tick: timeout in tick.
 *
 * @return 0 for success, negative for failure. Check asd_logger_rc_t and
 *         error code ASD_FLASH_BUFFER_E...
 */
int32_t asd_logger_erase_flash_log(asd_log_id_t log_id, uint32_t timeout_tick);

/**
 * @brief Erase all flash log for all log_id. Called by user task.
 * @param [in] timeout_tick: timeout in tick.
 *
 * @return 0 for success, negative for failure. Check asd_logger_rc_t and
 *         error code ASD_FLASH_BUFFER_E...
 */
int32_t asd_logger_erase_all_flash_log(uint32_t timeout_tick);

/**
 * @brief Flush log from RAM buffer to flash and console.
 * @param [in] timeout_tick: timeout in tick.
 *
 * @return 0 for success, negative for failure.
 */
int32_t asd_logger_flush_log(uint32_t timeout_tick);

#ifdef __cplusplus
}
#endif
