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
 * CRC32 IETF calculator
 *******************************************************************************
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Calculates the 32bit CRC with the IETF polynom and returns it.
 * The result is stored in big-endian format.
 */
uint32_t
asd_crc32_ietf(
    const uint8_t* buf, size_t len);
/**
 * Calculates the 32bit CRC with the MPEG2 polynom and returns it
 */
uint32_t
asd_crc32_mpeg(
    const uint8_t* buf, size_t len);
