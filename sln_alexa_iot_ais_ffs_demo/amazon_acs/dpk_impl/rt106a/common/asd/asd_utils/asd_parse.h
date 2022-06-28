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
 * Number parsing functions
 *******************************************************************************
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Parses a 32bit unsigned integer number.
 * 
 * This function will only parse 100% valid numbers. If any invalid character is
 * encountered the number will be rejected.
 * 
 * @param str The string to parse
 * @param str_len The amount of characters in the string to parse.
 * @param result Points to the location where the result will be written.
 * @return True if a number could be successfully parsed, false otherwise.
 */
bool acorn_parse_uint32_t(const char* str, size_t str_len, uint32_t* result);

#ifdef __cplusplus
}
#endif
