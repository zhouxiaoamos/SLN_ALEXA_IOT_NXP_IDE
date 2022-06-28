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

#pragma once

#include <stdlib.h>
#include <stdint.h>

#define UINT64_MAX_CHARS_FOR_BASE(base) \
    ((base == 2)  ? 64 : \
     (base == 3)  ? 41 : \
     (base == 4)  ? 32 : \
     (base == 5)  ? 28 : \
     (base == 6)  ? 25 : \
     (base == 7)  ? 23 : \
     (base == 8)  ? 22 : \
     (base == 9)  ? 21 : \
     (base == 10) ? 20 : \
     (base == 11) ? 19 : \
     (base == 12) ? 18 : \
     (base == 13) ? 18 : \
     (base == 14) ? 17 : \
     (base == 15) ? 17 : \
      16)

/**
 * @brief      Converts an unsigned 64-bit integer to printable C string using
 *             the specified radix.
 *
 * @param[in]   val       Number to be converted/
 * @param[out]  str       The string result.
 * @param[out]  str_size  The size (in chars) of the buffer pointed to by str.
 * @param[in]   radix     Base of val; must be in the range 2-16.
 */
void acorn_u64toa(uint64_t val, char *str, size_t str_size, int radix);
