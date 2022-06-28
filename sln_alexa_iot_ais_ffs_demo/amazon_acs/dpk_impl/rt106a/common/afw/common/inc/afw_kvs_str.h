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
 * A implementation of Key Value Store interface with support for strings Module
 *******************************************************************************
 */

/**
 * @file
 *
 */
#ifdef AFW_KVS_MIGRATE_PARTITIONS
#pragma once

#include "afw_kvs.h"
#include "cJSON.h"

typedef struct afw_kvs_str_s afw_kvs_str_t;

/**
 * @brief Migrate all KVS storage containers to new format
 *
 * @param void
 *
 * @return AFW_OK on success, AFW_ERR on failure
 */
int32_t afw_kvs_str_migrate_all(void);

/*************************************************************************************************/
/********************************* INTERNAL ******************************************************/
/*************************************************************************************************/
/********** Do not rely on the following implementation details in application code***************/
/*************************************************************************************************/
struct afw_kvs_str_s {
    afw_kvs_t afw_kvs;
    uint32_t flags; /**< Flags corresponding to file READ/WRITE/APPEND/OPEN */
    struct fm_flash_partition* partition; /**< Flash partition info of KVS*/
    cJSON* json_obj; /**< JSON object containing KVS members*/
    uint8_t* data_buf; /**< Buffer to hold serialized data for parsing*/
};

#endif /* AFW_KVS_MIGRATE_PARTITIONS */
