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
#include <stdbool.h>
#include <string.h>
#include <flash_map.h>

#include <afw_kvs.h>
#include <afw_error.h>

#define DHA_KVS_PRI_KEY "prkey"
#define DHA_KVS_CERT_KEY "cert"
#define END_CERT_STRING "-----END CERTIFICATE-----"

int32_t port_dha_write_key_pair(const uint8_t* buf, size_t buf_len) {
    int32_t retval;

    // Only write key pair if it doesn't previously exist
    retval = afw_kvs_get_size(FLASH_PARTITION_PERSIST_CRED_DHAKEY, DHA_KVS_PRI_KEY);
    if (retval > 0) {
        return -AFW_EROFS;
    }

    if (retval == -AFW_ENOENT) {
        retval = afw_kvs_set(FLASH_PARTITION_PERSIST_CRED_DHAKEY, DHA_KVS_PRI_KEY, buf, buf_len, true);
    }

    return retval;
}

int32_t port_dha_read_key_pair(uint8_t* buf, size_t max_buf_len) {
    int32_t retval;

    retval = afw_kvs_get(FLASH_PARTITION_PERSIST_CRED_DHAKEY,
                         DHA_KVS_PRI_KEY,
                         buf,
                         max_buf_len);

    return retval;
}

int32_t port_dha_write_cert_chain(const uint8_t* buf, size_t buf_len) {
    int32_t retval;
    size_t top_half_len = buf_len / 2;
    size_t bottom_half_len = buf_len - top_half_len;

    retval = afw_kvs_set(FLASH_PARTITION_PERSIST_CRED_DHACRT, DHA_KVS_CERT_KEY, buf, top_half_len, false);

    if(retval) {
        return retval;
    }

    retval = afw_kvs_set(FLASH_PARTITION_PERSIST_CRED_DHACRT2, DHA_KVS_CERT_KEY, buf + top_half_len, bottom_half_len, false);

    if (retval) {
        afw_kvs_delete(FLASH_PARTITION_PERSIST_CRED_DHACRT, DHA_KVS_CERT_KEY);
        return retval;
    }

    return retval;
}

int32_t port_dha_read_cert_chain(uint8_t* buf, size_t max_buf_len) {
    int32_t retval;
    size_t cur_len = 0;

    retval = afw_kvs_get(FLASH_PARTITION_PERSIST_CRED_DHACRT,
                         DHA_KVS_CERT_KEY,
                         buf,
                         max_buf_len);
    if(retval < 0) {
        return retval;
    }
    cur_len = retval;
    retval = afw_kvs_get(FLASH_PARTITION_PERSIST_CRED_DHACRT2,
                         DHA_KVS_CERT_KEY,
                         buf + cur_len,
                         max_buf_len - cur_len);
    if(retval < 0) {
        return retval;
    }
    cur_len += retval;

    if(cur_len > max_buf_len) {
        return -AFW_ENOSPC;
    }
    retval = cur_len;
    return retval;
}

int32_t port_dha_read_leaf_cert(uint8_t* buf, size_t max_buf_len) {
    int32_t retval;

    retval = port_dha_read_cert_chain(buf, max_buf_len);

    if(retval < 0) {
        return retval;
    }

    char* p = strstr((const char*)buf, END_CERT_STRING);
    if(p == NULL) {
        return -AFW_ENOENT;
    }
    *(p+strlen(END_CERT_STRING)) = '\0';
    retval = strlen((const char*)buf)+1;
    return retval;
}

int32_t port_dha_get_cert_chain_size(void) {
    int32_t retval;
    size_t cert_len = 0;

    retval = afw_kvs_get_size(FLASH_PARTITION_PERSIST_CRED_DHACRT, DHA_KVS_CERT_KEY);
    if(retval < 0) {
        return retval;
    }
    cert_len += retval;
    retval = afw_kvs_get_size(FLASH_PARTITION_PERSIST_CRED_DHACRT2, DHA_KVS_CERT_KEY);
    if(retval < 0) {
        return retval;
    }
    cert_len += retval;

    if(cert_len == 0) {
        retval = -AFW_ENOENT;
    } else {
        retval = cert_len;
    }

    return retval;
}
