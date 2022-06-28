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
#include "port_dha_storage.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ace/ace_status.h>
#include <afw_error.h>
#include <crypto_utils.h>

uint8_t key_pair[ECC_PRIVATE_KEY_MAX_SIZE];
size_t key_pair_len;
uint8_t cert_chain[5*ECC_CERT_MAX_SIZE];

int32_t port_dha_write_key_pair(const uint8_t* buf, size_t buf_len) {
    if(buf_len > ECC_PRIVATE_KEY_MAX_SIZE) {
        return -AFW_ENOSPC;
    } else {
        memcpy((void*)key_pair, (const void*)buf, buf_len);
        key_pair_len = buf_len;
        return ACE_STATUS_OK;
    }
}

int32_t port_dha_read_key_pair(uint8_t* buf, size_t max_buf_len) {
    if(key_pair_len < max_buf_len) {
        memcpy((char*)buf, (char*)key_pair, key_pair_len);
        return max_buf_len;
    } else {
        return -AFW_ENOSPC;
    }
}

int32_t port_dha_write_cert_chain(const uint8_t* buf, size_t buf_len) {
    if(buf_len > (5*ECC_CERT_MAX_SIZE)) {
        return -AFW_ENOSPC;
    } else {
        memcpy((void*)cert_chain, (const void*)buf, buf_len);
        return ACE_STATUS_OK;
    }
}

int32_t port_dha_read_cert_chain(uint8_t* buf, size_t max_buf_len) {
    if(strnlen((const char*)cert_chain, max_buf_len) < max_buf_len) {
        strcpy((char*)buf, (char*)cert_chain);
        return ACE_STATUS_OK;
    } else {
        return -AFW_ENOSPC;
    }
    return ACE_STATUS_NOT_SUPPORTED;
}
