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
 * AFW Internal Key Value Store (KVS) Interface
 *
 *******************************************************************************
 */

/**
 * @file afw_kvs_internal.h
 *
 * Provides an interface store/retrieve Key Value Pairs
 */
#pragma once

#include <stdint.h>

#pragma pack(push, 1)
typedef union afw_kvs_partition_header_u {
    // Used for partitions
    struct {
        unsigned char nonce[12];

        union {
            uint32_t restore_address;

            struct {
                uint32_t            :24;
                uint32_t magic      :8;
            };
        };

        uint16_t crc;
    };

    uint8_t header_buf[18];
} afw_kvs_partition_header_t;

typedef union afw_kvs_entry_meta_u {
    struct {
        uint8_t  magic;
        uint32_t namespace_len  :5;
        uint32_t key_len        :6;
        uint32_t value_len      :13;    // Max ~8KBytes
        uint32_t encrypted      :1;
        uint32_t valid          :1;
        uint32_t                :6;     // empty
        uint16_t value_crc;
        uint16_t meta_crc;
    };

    uint8_t meta_buf[9];
} afw_kvs_entry_meta_t;

#pragma pack(pop)