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
* The header for asd_log_reader. Read flash log.
*
*@File: asd_log_reader.h
********************************************************************************
*/

#pragma once

#include "asd_flash_buffer.h"
#include "asd_flash_region_mgr.h"
#include "asd_log_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct asd_log_reader {
    uint16_t  offset_in_frame;  ///< read offset in current frame;
    asd_log_id_t    log_id;     ///< log id
    uint32_t  read_len;         ///< data length already read.
    asd_frame_info_t frame_info; ///< current frame information for both log and crash dump.
    uint32_t buf_len;           ///< buffer length.
    uint8_t *buf;               ///< Buffer for log read.
    asd_log_data_type_t data_type;
};

typedef struct asd_log_reader asd_log_reader_t;

/**
 * @brief read log data from flash buffer, and decode lines to text in reader's buffer.
 *
 * @param [in] flashmgr: flash manager pointer
 * @param [in/out] reader: pointer of reader object.
 *
 * @return negative for failure. positive: bytes of log data in reader's buffer.
 */
int32_t log_reader_backend_read_log_from_flash(const asd_flash_mgr_t *flashmgr, asd_log_reader_t * reader);

#ifdef __cplusplus
}
#endif
