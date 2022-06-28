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
#include <ace/hal_dha.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <ace/ace.h>
#include <ace/hal_device_info.h>
#include <ace/hal_kv_storage.h>
#include "asd_log_platform_api.h"
#include <afw_crypto.h>
#include <afw_error.h>
#include <afw_utils.h>
#include <crypto_utils.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include "port/port_dha_storage.h"
#include <iot_hw.h>

#define CUSTOM_OID_LENGTH (9)
#define DEVICE_IDENTIFIER_BUFF_LENGTH (33)

// Custom OID 1.3.6.1.4.1.4843.1.1 (DSN)
static const char dsn_oid[CUSTOM_OID_LENGTH] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0xA5, 0x6B, 0x01, 0x01};
// Custom OID 1.3.6.1.4.1.4843.1.2 (Device ID 64 Bit immutable Identifer [OPTIONAL])
//static const char did_oid[CUSTOM_OID_LENGTH] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0xA5, 0x6B, 0x01, 0x02};
// Custom OID 1.3.6.1.4.1.4843.1.3 (Device Type)
static const char dtid_oid[CUSTOM_OID_LENGTH] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0xA5, 0x6B, 0x01, 0x03};

/*
* Embed Amazon DHAv2 Root Certificate in code
* To define in your project set TRUSTED_CA_ROOT_FILE to the file.
* Otherwise uses the default amazon root certificate
* You'll want to override this on your project to it's trusted in
* order to set certs that don't have the chain to the amazon root
* since they are already trusted higher up.
*/
#ifndef amzn_dhav2_root_ca_data
#define amzn_dhav2_root_ca_data \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDNjCCAtugAwIBAgIJAKpBxYNyH8biMAoGCCqGSM49BAMCMIGUMRQwEgYDVQQK\n"\
"DAtBbWF6b24gSW5jLjEjMCEGA1UECwwaRGV2aWNlIE1hbmFnZW1lbnQgU2Vydmlj\n"\
"ZXMxEDAOBgNVBAcMB1NlYXR0bGUxEzARBgNVBAgMCldhc2hpbmd0b24xCzAJBgNV\n"\
"BAYTAlVTMSMwIQYDVQQDDBpBbWF6b25ESEEgUm9vdCBDZXJ0aWZpY2F0ZTAgFw0x\n"\
"NzExMTYwNjI3MDlaGA8yMTE3MTEwODA2MjcwOVowgZQxFDASBgNVBAoMC0FtYXpv\n"\
"biBJbmMuMSMwIQYDVQQLDBpEZXZpY2UgTWFuYWdlbWVudCBTZXJ2aWNlczEQMA4G\n"\
"A1UEBwwHU2VhdHRsZTETMBEGA1UECAwKV2FzaGluZ3RvbjELMAkGA1UEBhMCVVMx\n"\
"IzAhBgNVBAMMGkFtYXpvbkRIQSBSb290IENlcnRpZmljYXRlMFkwEwYHKoZIzj0C\n"\
"AQYIKoZIzj0DAQcDQgAEv7+Zfvkc+qvUaKgaGxQMZoDFHQ18Z5OSXB4BNkYRszgR\n"\
"PQ82o54KVb0RKkkq0e3niRn1gUZ8jozePEPrpPV5yaOCARAwggEMMA8GA1UdEwEB\n"\
"/wQFMAMBAf8wHQYDVR0OBBYEFGGL3E8kSqVu7gplvLx0mlGV6qtgMIHJBgNVHSME\n"\
"gcEwgb6AFGGL3E8kSqVu7gplvLx0mlGV6qtgoYGapIGXMIGUMRQwEgYDVQQKDAtB\n"\
"bWF6b24gSW5jLjEjMCEGA1UECwwaRGV2aWNlIE1hbmFnZW1lbnQgU2VydmljZXMx\n"\
"EDAOBgNVBAcMB1NlYXR0bGUxEzARBgNVBAgMCldhc2hpbmd0b24xCzAJBgNVBAYT\n"\
"AlVTMSMwIQYDVQQDDBpBbWF6b25ESEEgUm9vdCBDZXJ0aWZpY2F0ZYIJAKpBxYNy\n"\
"H8biMA4GA1UdDwEB/wQEAwIBhjAKBggqhkjOPQQDAgNJADBGAiEAoCM4t1cMuTeu\n"\
"8yIlw/1BUIUb1Q4MYXp+LyfjcbmVz8ECIQDuKW8gQZjUS8Z7GcgNYnFup3UjTznj\n"\
"5ja4/PmvDx0Glw==\n"\
"-----END CERTIFICATE-----\n"
#endif

static int32_t read_parse_key_pair(mbedtls_pk_context* key) {
    int32_t retval = ACE_STATUS_OK;

    uint8_t* key_buf = (uint8_t*)malloc(ECC_PRIVATE_KEY_MAX_SIZE);
    if(!key_buf) {
        retval = ACE_STATUS_OUT_OF_MEMORY;
    }

    if(!retval) {
        retval = port_dha_read_key_pair(key_buf, ECC_PRIVATE_KEY_MAX_SIZE);
    }

    if(retval >= 0) {
        retval = mbedtls_pk_parse_key(key, key_buf, retval,
                                      NULL, 0);
        if(retval) {
            ASD_LOG_E(ace_hal, "Error parsing key pair %d", retval);
        }
    }

    if(key_buf) {
        free(key_buf);
    }
    return retval;
}

static int32_t add_custom_oid(mbedtls_x509write_csr* req) {
    int32_t retval = ACE_STATUS_OK;
    unsigned char* buf = NULL;
    mbedtls_asn1_named_data* asn1_data = NULL;
    int32_t bufsize;

    buf = (unsigned char*)malloc(DEVICE_IDENTIFIER_BUFF_LENGTH);
    if(!buf) {
        retval = ACE_STATUS_OUT_OF_MEMORY;
    }

    if(!retval) {
        retval = aceDeviceInfoDsHal_getEntry(DEVICE_SERIAL, (char*)buf, DEVICE_IDENTIFIER_BUFF_LENGTH);
        if(retval < 0) {
            ASD_LOG_E(ace_hal, "DSN read failed %d", retval);
        } else {
            bufsize = retval;
            retval = ACE_STATUS_OK;
        }
    }

    if(!retval) {
        mbedtls_asn1_free_named_data_list(&req->subject);
        asn1_data = mbedtls_asn1_store_named_data(&req->subject,
                                               dsn_oid, CUSTOM_OID_LENGTH,
                                               buf, bufsize);
        if(!asn1_data) {
            ASD_LOG_E(ace_hal, "OID store failed");
            retval = ACE_STATUS_OUT_OF_MEMORY;
        } else {
            asn1_data->val.tag = MBEDTLS_ASN1_PRINTABLE_STRING;
        }
    }

    if(!retval) {
        retval = aceDeviceInfoDsHal_getEntry(DEVICE_TYPE_ID, (char*)buf, DEVICE_IDENTIFIER_BUFF_LENGTH);
        if(retval < 0) {
            ASD_LOG_E(ace_hal, "Device type read failed %d", retval);
        } else {
            bufsize = retval;
            retval = ACE_STATUS_OK;
        }
    }

    if(!retval) {
        asn1_data = mbedtls_asn1_store_named_data(&req->subject,
                                               dtid_oid, CUSTOM_OID_LENGTH,
                                               buf, bufsize);
        if(!asn1_data) {
            ASD_LOG_E(ace_hal, "OID store failed");
            retval = ACE_STATUS_OUT_OF_MEMORY;
        } else {
            asn1_data->val.tag = MBEDTLS_ASN1_PRINTABLE_STRING;
        }
    }

    if(buf) {
        free(buf);
    }

    return retval;
}

static int32_t verify_custom_oid(mbedtls_x509_crt* cert) {
    int32_t retval = ACE_STATUS_OK;
    unsigned char* buf = NULL;
    mbedtls_asn1_named_data* asn1_data = NULL;
    int32_t bufsize;

    buf = (unsigned char*)malloc(DEVICE_IDENTIFIER_BUFF_LENGTH);
    if(!buf) {
        retval = ACE_STATUS_OUT_OF_MEMORY;
    }

    if(!retval) {
        retval = aceDeviceInfoDsHal_getEntry(DEVICE_SERIAL, (char*)buf, DEVICE_IDENTIFIER_BUFF_LENGTH);
        if(retval < 0) {
            ASD_LOG_E(ace_hal, "DSN read failed %d", retval);
        } else {
            bufsize = retval;
            retval = ACE_STATUS_OK;
        }
    }

    if(!retval) {
        asn1_data = mbedtls_asn1_find_named_data(&cert->subject, dsn_oid,
                                                 CUSTOM_OID_LENGTH);
        if(!asn1_data) {
            ASD_LOG_E(ace_hal, "Custom OID read failed");
            retval = ACE_STATUS_NOT_FOUND;
        }
    }

    if(!retval) {
        if(((size_t)bufsize != asn1_data->val.len) ||
            (memcmp(buf, asn1_data->val.p, bufsize))) {
                ASD_LOG_E(ace_hal, "Cert Custom OID Mismatch");
                retval = ACE_STATUS_GENERAL_ERROR;
        }
    }

    if(!retval) {
        retval = aceDeviceInfoDsHal_getEntry(DEVICE_TYPE_ID, (char*)buf, DEVICE_IDENTIFIER_BUFF_LENGTH);

        // For 2 stage registration, fallback to module device_type if product device_type is not set
#ifdef AMAZON_MODULE_DEVICE_TYPE
        if(retval < 0) {
            retval = aceKeyValueDsHal_get(AMAZON_MODULE_DEVICE_TYPE, (void*)buf, DEVICE_IDENTIFIER_BUFF_LENGTH);
        }
#endif

        if(retval < 0) {
            ASD_LOG_E(ace_hal, "Device type read failed %d", retval);
        } else {
            bufsize = retval;
            retval = ACE_STATUS_OK;
        }
    }

    if(!retval) {
        asn1_data = mbedtls_asn1_find_named_data(&cert->subject, dtid_oid,
                                                 CUSTOM_OID_LENGTH);
        if(!asn1_data) {
            ASD_LOG_E(ace_hal, "Custom OID read failed");
            retval = ACE_STATUS_NOT_FOUND;
        }
    }

    if(!retval) {
        if(((size_t)bufsize != asn1_data->val.len) ||
            (memcmp(buf, asn1_data->val.p, bufsize))) {
                ASD_LOG_E(ace_hal, "Cert Custom OID Mismatch");
                retval = ACE_STATUS_GENERAL_ERROR;
        }
    }

    if(buf) {
        free(buf);
    }

    return retval;
}

static int32_t get_cert_sign_request( void **input, size_t *size) {
    int32_t retval = ACE_STATUS_OK;
    mbedtls_pk_context key;
    mbedtls_x509write_csr req;

    mbedtls_pk_init(&key);

    retval = read_parse_key_pair(&key);
    if(retval) {
        ASD_LOG_E(ace_hal, "Parsing key failed %d", retval);
    }

    mbedtls_x509write_csr_init(&req);
    mbedtls_x509write_csr_set_md_alg(&req, MBEDTLS_MD_SHA256);

    if(!retval) {
        retval = add_custom_oid(&req);
        if(retval) {
            ASD_LOG_E(ace_hal, "Add Custom OID failed %d", retval);
        }
    }

    if(!retval) {
        mbedtls_x509write_csr_set_key(&req, &key);
        retval = crypto_utils_dump_csr_pem(&req, (uint8_t**)input);
        if(retval) {
            ASD_LOG_E(ace_hal, "PEM gen failed %d", retval);
        } else {
            *size = strlen((const char*) *input);
        }
    }

    mbedtls_pk_free(&key);
    mbedtls_x509write_csr_free(&req);

    return retval;
}

static int32_t verify_key_pair(mbedtls_pk_context* pub_key) {
    int32_t retval = ACE_STATUS_OK;
    uint8_t test_hash[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                            0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
    uint8_t* sig_buf = NULL;
    size_t sig_buf_len = 0;

    retval = aceDhaHal_sign_data(test_hash, sizeof(test_hash),
                                 &sig_buf, &sig_buf_len);
    if(retval) {
        ASD_LOG_E(ace_hal, "Error signing data %d", retval);
    }

    if(!retval) {
        retval = mbedtls_ecdsa_read_signature(mbedtls_pk_ec(*pub_key),
                                              test_hash, sizeof(test_hash),
                                              sig_buf, sig_buf_len);
        if(retval) {
            ASD_LOG_E(ace_hal, "Error verifying data %d", retval);
        }
    }

    if(sig_buf) {
        free(sig_buf);
    }
    return retval;
}

int aceDhaHal_generate_dha_key(void) {
    mbedtls_pk_context key;
    int32_t retval = ACE_STATUS_OK;

    uint8_t* key_buf = NULL;
    size_t key_buf_len = 0;
    // Check if the key already exists
    retval = aceDhaHal_get_dha_key_public(&key_buf, &key_buf_len);
    if(key_buf) {
        // Zero-ize the memory to prevent security exposures
        memset(key_buf, 0, key_buf_len);
        key_buf_len = 0;
        free(key_buf);
        key_buf = NULL;
    }

    if(retval == ACE_STATUS_OK) {
        return ACE_STATUS_ALREADY_EXISTS;
    }

    mbedtls_pk_init(&key);

    retval = crypto_utils_gen_ecc_key_pair("dha_v2", &key);
    if(retval) {
        ASD_LOG_E(ace_hal, "DHA v2 key generation failed %d", retval);
    }

    if(!retval) {
        retval = crypto_utils_dump_pri_key_der(&key, &key_buf, &key_buf_len);
        if(retval) {
            ASD_LOG_E(ace_hal, "DER key dump failed %d", retval);
        }
    }

    if(!retval) {
        retval = port_dha_write_key_pair((uint8_t*)key_buf, key_buf_len);
        if(retval) {
            ASD_LOG_E(ace_hal, "Key pair store failed %s(%d)", afw_strerror(retval), retval);
        }
    }

    if(key_buf) {
        memset(key_buf, 0, key_buf_len);
        free(key_buf);
    }

    mbedtls_pk_free(&key);

    return retval;
}

int aceDhaHal_get_dha_key_public(uint8_t **x509_data, size_t *x509_data_len) {
    mbedtls_pk_context key;
    int32_t retval;

    if((!x509_data) || (!x509_data_len)) {
        return ACE_STATUS_NULL_POINTER;
    }

    *x509_data = NULL;
    *x509_data_len = 0;
    mbedtls_pk_init(&key);

    retval = read_parse_key_pair(&key);
    if(retval) {
        ASD_LOG_E(ace_hal, "Parsing key failed %d", retval);
    }

    if(!retval) {
        retval = crypto_utils_dump_pub_der(&key, x509_data, x509_data_len);
        if(retval) {
            ASD_LOG_E(ace_hal, "DER key dump failed %d", retval);
        }
    }
    mbedtls_pk_free(&key);

    return retval;
}

int aceDhaHal_sign_data(const uint8_t *data, const size_t data_len,
                        uint8_t **signed_data, size_t *signed_data_len) {
    mbedtls_pk_context key;
    int32_t retval;

    if((!data) || (!signed_data) || (!signed_data_len)) {
        return ACE_STATUS_NULL_POINTER;
    }

    if (data_len != AFW_CRYPTO_SHA256_SIZE) {
        return ACE_STATUS_BAD_PARAM;
    }

    *signed_data = NULL;
    *signed_data_len = 0;
    mbedtls_pk_init(&key);

    retval = read_parse_key_pair(&key);
    if(retval) {
        ASD_LOG_E(ace_hal, "Parsing key failed %d", retval);
    }

    if(!retval) {
        retval = crypto_utils_ecdsa_sign(&key, data, data_len,
                                         signed_data, signed_data_len);
        if(retval) {
            ASD_LOG_E(ace_hal, "Signing failed %d", retval);
        }
    }

    mbedtls_pk_free(&key);

    return retval;
}

int aceDhaHal_verify_data(const uint8_t *data, const size_t data_len,
                          const uint8_t *signature, const size_t signature_len) {
    mbedtls_pk_context key;
    int32_t retval;

    if((!data) || (!signature)) {
        return ACE_STATUS_NULL_POINTER;
    }

    if (data_len != AFW_CRYPTO_SHA256_SIZE) {
        return ACE_STATUS_BAD_PARAM;
    }

    mbedtls_pk_init(&key);

    retval = read_parse_key_pair(&key);
    if(retval) {
        ASD_LOG_E(ace_hal, "Parsing key failed %d", retval);
    }

    if(!retval) {
        retval = mbedtls_ecdsa_read_signature(mbedtls_pk_ec(key),
                                              data, data_len,
                                              signature, signature_len);
        if(retval) {
            ASD_LOG_E(ace_hal, "Signature verification failed %d", retval);
        }
    }

    mbedtls_pk_free(&key);

    return retval;
}

int aceDhaHal_get_field(int type, void **input, size_t *size) {
    int32_t retval = ACE_STATUS_OK;
    size_t cert_buf_size = 0;
    unsigned char* buf = NULL;

    if((!input) || (!size)) {
        return ACE_STATUS_NULL_POINTER;
    }

    *input = NULL;
    *size = 0;

    switch(type) {
        case ACE_DHA_SOC_ID: {
            uint16_t hwID = iot_hw_get_id();
            *size = sizeof(hwID)*2;// each byte is two hex strings
            *input = calloc(1, *size + 1); // need null space
            afw_hexlify((uint8_t*)&hwID, sizeof(hwID), *input);
            break;
        }

        case ACE_DHA_DSN:
            buf = (unsigned char*)malloc(DEVICE_IDENTIFIER_BUFF_LENGTH);
            if(!buf) {
                retval = ACE_STATUS_OUT_OF_MEMORY;
            }

            if(!retval) {
                retval = aceDeviceInfoDsHal_getEntry(DEVICE_SERIAL, (char*)buf, DEVICE_IDENTIFIER_BUFF_LENGTH);
                if(retval < 0) {
                    ASD_LOG_E(ace_hal, "DSN read failed");
                }
            }

            if(retval > 0) {
                *size = retval;
                *input = buf;
                retval = ACE_STATUS_OK;
            } else {
                if(buf) {
                    free(buf);
                }
            }
            break;

        case ACE_DHA_CSR:
            retval = get_cert_sign_request(input, size);
            if(retval) {
                ASD_LOG_E(ace_hal, "CSR get failed");
            }
            break;
        case ACE_DHA_CERTIFICATE_CHAIN:
            retval = port_dha_get_cert_chain_size();
            if(retval < 0) {
                break;
            }

            cert_buf_size = retval;
            buf = (unsigned char*)malloc(cert_buf_size);
            if(!buf) {
                retval = ACE_STATUS_OUT_OF_MEMORY;
                break;
            }

            retval = port_dha_read_cert_chain(buf, cert_buf_size);
            if(retval < 0) {
                ASD_LOG_E(ace_hal, "Error reading certificate chain %d", retval);
                free(buf);
            } else {
                *input = buf;
                *size = retval;
                retval = ACE_STATUS_OK;
            }
            break;

        case ACE_DHA_LEAF_CERTIFICATE:
            retval = port_dha_get_cert_chain_size();
            if(retval < 0) {
                break;
            }

            cert_buf_size = retval;
            buf = (unsigned char*)malloc(cert_buf_size);
            if(!buf) {
                retval = ACE_STATUS_OUT_OF_MEMORY;
                break;
            }

            retval = port_dha_read_leaf_cert(buf, cert_buf_size);
            if(retval < 0) {
                ASD_LOG_E(ace_hal, "Error reading certificate chain %d", retval);
                free(buf);
            } else {
                *size = retval;
                if(cert_buf_size != (size_t)*size) {
                    buf = (unsigned char*)realloc(buf, (size_t)*size);
                    if(!buf){
                        retval = ACE_STATUS_OUT_OF_MEMORY;
                        break;
                    }
                }
                *input = buf;
                retval = ACE_STATUS_OK;
            }
            break;
        default:
            retval = ACE_STATUS_NOT_FOUND;
    }
    return retval;
}

int aceDhaHal_set_certificate(const char *certificate) {
    int32_t retval;
    mbedtls_x509_crt trusted_root;
    mbedtls_x509_crt cert_chain;
    uint32_t flags;

    if(!certificate) {
        return ACE_STATUS_NULL_POINTER;
    }

    mbedtls_x509_crt_init(&trusted_root);
    mbedtls_x509_crt_init(&cert_chain);

    retval = mbedtls_x509_crt_parse(&cert_chain, (unsigned char*)certificate,
                                    strlen(certificate)+1);
    if(retval) {
        ASD_LOG_E(ace_hal, "Error parsing certs %d", retval);
    }

    if(!retval) {
        retval = mbedtls_x509_crt_parse(&trusted_root,
                                        (unsigned char*)amzn_dhav2_root_ca_data,
                                        strlen(amzn_dhav2_root_ca_data)+1);
        if(retval) {
            ASD_LOG_E(ace_hal, "Error parsing root cert %d", retval);
        }
    }

    if(!retval) {
        retval = mbedtls_x509_crt_verify(&cert_chain, &trusted_root,
                                         NULL, NULL, &flags, NULL, NULL);

        if(retval) {
            ASD_LOG_E(ace_hal, "Cert verification failed %d flags(x509.h) %x", retval, flags);
        }
    }

    if(!retval) {
        retval = verify_custom_oid(&cert_chain);
        if(retval) {
            ASD_LOG_E(ace_hal, "Cert custom OID verification failed %d", retval);
        }
    }

    if(!retval) {
        retval = verify_key_pair(&cert_chain.pk);
        if(retval) {
            ASD_LOG_E(ace_hal, "Verify key pair failed %d", retval);
        }
    }

    if(!retval) {
        retval = port_dha_write_cert_chain((uint8_t*)certificate,
                                           strlen(certificate)+1);
        if(retval) {
            ASD_LOG_E(ace_hal, "Error storing cert chain %d", retval);
        }
    }

    mbedtls_x509_crt_free(&cert_chain);
    mbedtls_x509_crt_free(&trusted_root);

    return retval;
}

ace_status_t aceDhaHal_open(void) {
    return ACE_STATUS_OK;
}

ace_status_t aceDhaHal_close(void) {
    return ACE_STATUS_OK;
}

