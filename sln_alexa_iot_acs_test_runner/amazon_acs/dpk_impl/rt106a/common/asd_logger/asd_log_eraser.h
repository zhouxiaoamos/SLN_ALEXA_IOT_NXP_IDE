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
* The header for log eraser.
*
*@File: asd_log_eraser.h
********************************************************************************
*/

#pragma once

#include "asd_log_api.h"
#ifdef __cplusplus
extern "C"
{
#endif

union asd_eraser_cb_params {
    uint64_t expiration_timestamp_ms;
};

/**
 * @brief call back to check data is required to erase.
 * @param [in] cb_args: argument for callback.
 * @param [in] msg: data to compare
 *
 * @return true: data need to be erased; false: don't erase.
 */
typedef bool (*should_erase_data_callback_t)(union asd_eraser_cb_params* cb_args, void* msg);

typedef struct asd_log_eraser {
    should_erase_data_callback_t should_erase_data_cb;
    union asd_eraser_cb_params callback_params;
} asd_log_eraser_t;

int32_t asd_logger_erase_expired_flash_log(
    asd_log_id_t log_id, uint32_t timeout_tick, uint64_t expiration_timestamp_ms);

#ifdef __cplusplus
}
#endif
