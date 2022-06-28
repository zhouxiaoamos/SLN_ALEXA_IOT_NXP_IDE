/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All rights reserved.
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
 * AFW Flash
 *
 *******************************************************************************
 */

#ifndef AFW_FLASH_H
#define AFW_FLASH_H

#include <stdint.h>
#include "afw_flash_api.h"

/*
 * Flash manager internal data structure. Do not expose it to applications.
 */
struct flash_info {
    /* flash device or partition name */
    char       name[AFW_FLASH_PARTITION_NAME_MAX];
    uint32_t   size;
    int        index;

    /* TODO: need to change this to allow different erase sizes */
    uint32_t   erasesize_bitset;
    uint8_t    encrypted    :1;
    uint8_t    flags        :7;

    /* if this is a partition */
    uint8_t    is_partition;
    uint32_t   part_offset;

    int (*erase) (struct flash_info *flash, struct erase_info *info);
    int (*read) (struct flash_info *flash, uint32_t from, uint32_t len,
                 uint8_t *buf);
    int (*write) (struct flash_info *flash, uint32_t to, uint32_t len,
                  const uint8_t *buf);
    void (*sync) (struct flash_info *flash);
    int (*suspend) (struct flash_info *flash);
    void (*resume) (struct flash_info *flash);
};

#endif /* AFW_FLASH_H */
