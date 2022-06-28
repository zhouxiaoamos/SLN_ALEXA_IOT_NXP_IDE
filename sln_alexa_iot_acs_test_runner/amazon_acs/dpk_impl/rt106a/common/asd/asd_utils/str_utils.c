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
 * Various string-related utility functions.
 *******************************************************************************
 */

#include "str_utils.h"

void acorn_u64toa(uint64_t val, char *str, size_t str_size, int radix)
{
    uint64_t val_ = val;
    // Worst case + null char
    char tmp_str[UINT64_MAX_CHARS_FOR_BASE(2) + 1] = {0};
    // One before the final null terminator
    char* tmp_str_ = &tmp_str[UINT64_MAX_CHARS_FOR_BASE(2) - 1];

    if (str == NULL || str_size == 0) {
        return;
    }

    do {
        *tmp_str_-- = "0123456789abcdef"[val_ % radix];
        val_ /= radix;
    } while (val_);

    tmp_str_++; // Point to the beginning of the string

    for (size_t i = 0; i < (str_size - 1); i++) {
        if (tmp_str_[i] != '\0') {
            str[i] = tmp_str_[i];
        } else {
            str[i] = '\0';
            break;
        }
    }
    str[str_size - 1] = '\0';
}
