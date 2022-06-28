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
#include <crypto_utils.h>

#include <stdio.h>
#include <string.h>

#include <ace/ace.h>
#include <afw_error.h>
#include "asd_log_platform_api.h"
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/entropy.h>

int32_t crypto_utils_gen_ecc_key_pair(const char* id, mbedtls_pk_context* key) {

    int32_t retval = ACE_STATUS_OK;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    if(!key) {
        retval = -AFW_EINVAL;
    }

    if(!retval) {
        retval = mbedtls_ctr_drbg_seed(&ctr_drbg,
                                           mbedtls_entropy_func,
                                           &entropy,
                                           (const unsigned char *) id,
                                           strlen(id));
        if(retval) {
            ASD_LOG_E(ace_hal, "Counter seeding failed %d", retval);
        }
    }

    if (!retval) {
        retval = mbedtls_pk_setup(key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
        if(retval) {
            ASD_LOG_E(ace_hal, "PK set up failed %d", retval);
        }
    }

    if (!retval) {
        retval = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1,
                                         mbedtls_pk_ec(*key),
                                         mbedtls_ctr_drbg_random,
                                         &ctr_drbg);
        if(retval) {
            ASD_LOG_E(ace_hal, "EC key gen failed %d", retval);
        }
    }

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return retval;
}

int32_t crypto_utils_dump_pri_key_der(mbedtls_pk_context* key,
                                  uint8_t **pkey_data, size_t *pkey_data_len) {
    int32_t retval = ACE_STATUS_OK;

    if((!pkey_data) || (!pkey_data_len)) {
        retval = -AFW_EINVAL;
    }

    uint8_t* key_buf = (uint8_t*)malloc(ECC_PRIVATE_KEY_MAX_SIZE);
    if(!key_buf) {
        retval = -AFW_ENOMEM;
    }

    if (!retval) {
        retval = mbedtls_pk_write_key_der(key, (unsigned char*)key_buf,
                                          ECC_PRIVATE_KEY_MAX_SIZE);
        if(retval < 0) {
            ASD_LOG_E(ace_hal, "PK write key failed %d", retval);
        } else {
            *pkey_data_len = retval;
            retval = ACE_STATUS_OK;
        }
    }

    if(!retval) {
        /**
         * MbedTLS API used above put the key at the end of the buffer. Relocate
         * to begining so that pointer to start of buffer is returned to caller
         * which also needs to use it with 'free'
         */
        if(*pkey_data_len < ECC_PRIVATE_KEY_MAX_SIZE) {
            uint8_t* src = key_buf + ECC_PRIVATE_KEY_MAX_SIZE - *pkey_data_len;
            uint8_t* dest = key_buf;
            for(size_t idx = 0; idx<(*pkey_data_len); idx++) {
                *dest = *src;
                dest++;
                src++;
            }
        }
        *pkey_data = key_buf;
    } else if (key_buf) {
        free(key_buf);
    }

    return retval;
}

int32_t crypto_utils_dump_pri_key_pem(mbedtls_pk_context* key,
                                  uint8_t **pkey_data) {
    int32_t retval = ACE_STATUS_OK;

    if(!pkey_data) {
        retval = -AFW_EINVAL;
    }

    uint8_t* key_buf = (uint8_t*)malloc(ECC_PRIVATE_KEY_MAX_SIZE);
    if(!key_buf) {
        retval = -AFW_ENOMEM;
    }

    if (!retval) {
        retval = mbedtls_pk_write_key_pem(key, (unsigned char*)key_buf,
                                          ECC_PRIVATE_KEY_MAX_SIZE);
        if(retval) {
            ASD_LOG_E(ace_hal, "PK write key failed %d", retval);
        }
    }

    if(!retval) {
        *pkey_data = key_buf;
    } else if(key_buf) {
        free(key_buf);
    }

    return retval;
}

int32_t crypto_utils_dump_pub_der(mbedtls_pk_context* key,
                                  uint8_t **pubkey_data,
                                  size_t *pubkey_data_len) {
    int32_t retval = ACE_STATUS_OK;

    uint8_t* pub_key_buf = (uint8_t*)malloc(ECC_PUBLIC_KEY_MAX_SIZE);
    if(!pub_key_buf) {
        retval = -AFW_ENOMEM;
    }

    if(!retval) {
        retval = mbedtls_pk_write_pubkey_der(key, (unsigned char*)pub_key_buf,
                                             ECC_PUBLIC_KEY_MAX_SIZE);
        if(retval <= 0) {
            ASD_LOG_E(ace_hal, "Converting key failed %d", retval);
        } else {
            *pubkey_data_len = retval;
            retval = ACE_STATUS_OK;
        }
    }

    if(!retval) {
         /**
         * MbedTLS API used above put the key at the end of the buffer. Relocate
         * to begining so that pointer to start of buffer is returned to caller
         * which also needs to use it with 'free'
         */
       if(*pubkey_data_len < ECC_PUBLIC_KEY_MAX_SIZE) {
            uint8_t* src = pub_key_buf + ECC_PUBLIC_KEY_MAX_SIZE - *pubkey_data_len;
            uint8_t* dest = pub_key_buf;
            for(size_t idx = 0; idx<(*pubkey_data_len); idx++) {
                *dest = *src;
                dest++;
                src++;
            }
        }
        *pubkey_data = pub_key_buf;
    } else if (pub_key_buf) {
        free(pub_key_buf);
    }
    return retval;
}

int32_t crypto_utils_ecdsa_sign(mbedtls_pk_context* key,
                                const uint8_t *data, const size_t data_len,
                                uint8_t **signed_data, size_t *signed_data_len) {

    int32_t retval = ACE_STATUS_OK;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const  char *personality = "ecdsa sign";
    unsigned char* sig_buf = NULL;

    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    sig_buf = (unsigned char*)malloc(MBEDTLS_ECDSA_MAX_LEN);
    if(!sig_buf) {
        retval = -AFW_ENOMEM;
    }

    if(!retval) {
        retval = mbedtls_ctr_drbg_seed(&ctr_drbg,
                                           mbedtls_entropy_func,
                                           &entropy,
                                           (const unsigned char *) personality,
                                           strlen(personality));
        if(retval) {
            ASD_LOG_E(ace_hal, "Counter seeding failed %d", retval);
        }
    }

    if(!retval) {
        retval = mbedtls_ecdsa_write_signature(mbedtls_pk_ec(*key),
                                               MBEDTLS_MD_SHA256,
                                               data, data_len,
                                               sig_buf, signed_data_len,
                                               mbedtls_ctr_drbg_random,
                                               &ctr_drbg);

        if(retval) {
            ASD_LOG_E(ace_hal, "Signing failed %d", retval);
        }
    }

    if(!retval) {
        retval = mbedtls_ecdsa_read_signature(mbedtls_pk_ec(*key),
                                              data, data_len,
                                              sig_buf, *signed_data_len);
        if(retval) {
            ASD_LOG_E(ace_hal, "Signature verification failed %d", retval);
        }
    }

    if(!retval) {
        *signed_data = (uint8_t*)sig_buf;
    } else if(sig_buf) {
        free(sig_buf);
    }

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return retval;
}

int32_t crypto_utils_dump_csr_pem(mbedtls_x509write_csr* req,
                                  uint8_t** csr_buf) {
    int32_t retval = ACE_STATUS_OK;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const  char *personality = "ecdsa sign";

    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    *csr_buf = (unsigned char*)malloc(CSR_MAX_SIZE);
    if(!*csr_buf) {
        retval = -AFW_ENOMEM;
    }

    if(!retval) {
        retval = mbedtls_ctr_drbg_seed(&ctr_drbg,
                                           mbedtls_entropy_func,
                                           &entropy,
                                           (const unsigned char *) personality,
                                           strlen(personality));
        if(retval) {
            ASD_LOG_E(ace_hal, "Counter seeding failed %d", retval);
        }
    }

    if(!retval) {
        retval = mbedtls_x509write_csr_pem(req, *csr_buf, CSR_MAX_SIZE,
                                           mbedtls_ctr_drbg_random, &ctr_drbg);
        if(retval) {
            ASD_LOG_E(ace_hal, "Pem CSR gen failed %d", retval);
        }
    }

    if((retval) && (*csr_buf)) {
        free(*csr_buf);
        *csr_buf = NULL;
    }

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return retval;
}
