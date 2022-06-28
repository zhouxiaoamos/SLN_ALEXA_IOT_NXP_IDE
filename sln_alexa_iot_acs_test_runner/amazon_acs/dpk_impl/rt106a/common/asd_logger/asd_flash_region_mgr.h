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
* The header for flash region manager.
*
*@File: asd_flash_region_mgr.h
********************************************************************************
*/

#pragma once


#include <stdint.h>
#include <stddef.h>
#include "asd_log_api.h"
#include "afw_stream.h"
#include "flash_map.h"
#include "asd_flash_buffer.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ASD_FLASH_META_MAGIC_NUM     (0xDADADADA)

// struct of a flash region.
typedef struct {
    const char *partition_name;        ///< partition name string for this region.
    uint8_t log_id;                    ///< log id of the region.
    uint8_t data_type;                 ///< data type of the region. BIN or LOG_TXT.
} flash_region_t;

typedef struct {
    uint32_t crc;                       ///< CRC 32-bits. compute based on the bytes after crc.
    uint32_t magic_num;                 ///< magic number should be 0xDADADADA
    uint16_t length     : 12;           ///< length of meta data. 4K at most.
    uint16_t reserved   : 4;
    uint8_t  version;                   ///< version of flash region meta data.
    uint8_t  region_num;                ///< region number.
} __attribute__((packed)) flash_region_meta_t;

//forward declaration.
struct asd_flash_mgr;
typedef struct asd_flash_mgr asd_flash_mgr_t;

/**
 * @brief initialize flash manager.
 * @param none
 *
 * @return flash manager handle.
 */
asd_flash_mgr_t* asd_flash_mgr_init(void);

/**
 * @brief deinite flash manager.
 * @param [in] handle: flash manager handle to deinit.
 *
 * @return None.
 */
void asd_flash_mgr_deinit(asd_flash_mgr_t* handle);

/**
 * @brief Write one data frame in flash. Only support one frame per write.
 * @param [in] stream: flash stream pointer.
 * @param [in] frame: data frame pointer.
 * @param [in] size: size of data frame.
 *
 * @return Check asd_logger_rc_t for return values.
 */
int32_t asd_flash_stream_write_frame(afw_stream_t* stream, const uint8_t* frame, uint32_t size);

/**
 * @brief Get the stream pointer of flash manager.
 * @param [in] mgr: flash manager pointer.
 *
 * @return afw_stream pointer.
 */
afw_stream_t* asd_flash_mgr_get_stream(asd_flash_mgr_t* mgr);

/**
 * @brief Get the flash buffer according to log id.
 * @param [in] mgr: flash manager pointer.
 * @param [in] log_id: log id.
 *
 * @return flash buffer pointer.
 */
asd_flash_buffer_t * asd_flash_mgr_get_buffer_by_log_id(const asd_flash_mgr_t* mgr, uint8_t log_id);

/**
 * @brief Get data type according to log id
 * @param [in] mgr: flash manager pointer.
 * @param [in] log_id: log id.
 *
 * @return data type in the log buffer.
 */
asd_log_data_type_t asd_flash_mgr_get_buffer_data_type(const asd_flash_mgr_t* mgr, uint8_t log_id);


const char* asd_logger_get_flash_partition_name(uint8_t log_id);

#ifdef __cplusplus
}
#endif