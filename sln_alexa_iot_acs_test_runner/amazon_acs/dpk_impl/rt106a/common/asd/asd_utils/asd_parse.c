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
#include "asd_parse.h"
#include <ctype.h>
#include <limits.h>

bool acorn_parse_uint32_t(const char* str, size_t str_len, uint32_t* result)
{
    *result = 0;
    // Temporary uint64 variable is used to detect overflows during parsing
    uint64_t temp = 0;
    if (str == NULL) {
        return false;
    }

    for (size_t i = 0; i < str_len; i++) {
        char c = str[i];
        if (!isdigit((int)c)) {
            return false;
        }
        uint8_t nr = (uint8_t)(c - '0');
        temp = (uint64_t)10*temp + (uint64_t)nr;

        if (temp > (uint64_t)UINT32_MAX) {
            return false;
        }
    }

    *result = (uint32_t)temp;
    return true;
}