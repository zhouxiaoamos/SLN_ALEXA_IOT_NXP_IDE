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
 * AFW CRYPTO
 *******************************************************************************
 */

#ifndef AFW_CRYPTO_H_
#define AFW_CRYPTO_H_

#include <stdint.h>

#define AFW_CRYPTO_AES_STATUS_OK       (0)
#define AFW_CRYPTO_AES_CBC_IV_LENGTH   (16)
#define AFW_CRYPTO_AES_BLOCK_SIZE      (16)
#define AFW_CRYPTO_RSA_2048_SIG_LENGTH (256)
#define AFW_CRYPTO_SIG_LEN             (AFW_CRYPTO_RSA_2048_SIG_LENGTH)

// for use with mbedtls_sha256
#define AFW_CRYPTO_SHA256_SIZE (32)
#define AFW_CRYPTO_RSASSA_PSS_2048_SIG_LEN  (256)

#endif
