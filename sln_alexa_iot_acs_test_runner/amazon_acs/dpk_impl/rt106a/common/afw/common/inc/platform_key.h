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
 * @file platform_key.h
 *
 * @brief setup and supply platform specific crypto keys.
 *
 *******************************************************************************
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


/*******************************************************************************
 * Definitions
 ******************************************************************************/
/**
 * The return codes for the functions.
 */
#define PLATFORM_KEY_SUCCESS        (0)  /*!< operation completed successfully. */
#define PLATFORM_KEY_FAIL           (1)  /*!< operation failed. */


/**
 * @brief set_puf_key_for_kvs is used to setup platform specific key for KVS data
 * encryption/decryption.
 *
 * @param[out] key Pointer to pointer of platform specific KVS encryption key.
 *
 * @return
 *   - PLATFORM_KEY_SUCCESS if key is set successuflly.
 *   - PLATFORM_KEY_FAIL if key setup failed or no key is available.
 */
int platform_key_setup_kvs_key(unsigned char **key);

