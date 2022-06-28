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
 * Various memory-related utility functions.
 *******************************************************************************
 */

#pragma once

#include <stddef.h>

/**
 * @brief A constant-time memory compare function; useful for security-related
 *        code (e.g. verifying an HMAC).
 *
 * @param[in] a  Pointer to the first memory area
 * @param[in] b  Pointer to the second memory area
 * @param     n  Number of bytes to compare
 * @return int   Will return 0 if equal; non-zero otherwise.
 */
int memcmp_const(const void* s1, const void* s2, size_t n);
