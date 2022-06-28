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

 /**
 * @file
 *
 * @brief Selected Crypto APIs/ helper functions which automatically use the
 * default entropy source and provides appropriate DRBG needed for underlying
 * mbedTLS APIs
 */
#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <stdlib.h>
#include <stdint.h>

#include <mbedtls/pk.h>
#include <mbedtls/x509_csr.h>

/**
 * @brief MAX size of Elliptic Curve Private key
 */
#define ECC_PRIVATE_KEY_MAX_SIZE    512

/**
 * @brief MAX size of Elliptic Curve Certificate
 */
#define ECC_CERT_MAX_SIZE           1280

/**
 * @brief MAX size of Elliptic Curve Public key
 */
#define ECC_PUBLIC_KEY_MAX_SIZE     256

/**
 * @brief MAX size of Certificate Signing Request for ECC key pair
 */
#define CSR_MAX_SIZE                512

/**
 * @brief Generate ECC key pair using 256 bits NIST curve
 *
 * @param id Client specific identifier used to seed DRBG
 * @param key Context to populate with ECC point, keys
 * @returns 0 on success, an afw_error/aceHal_error/MbedTLS value on failure
 *
 */
int32_t crypto_utils_gen_ecc_key_pair(const char* id, mbedtls_pk_context* key);

/**
 * @brief Dump private key in DER format from MbedTLS Private Key context
 *
 * @param key Context with private key
 * @param pkey_data Pointer to a uint8_t* which will be populated with address
 * of allocated buffer holding PKCS#1 or SEC1 DER structure of private key.
 * Buffer allocated is of size ECC_PRIVATE_KEY_MAX_SIZE. Caller responsible to
 * free after use.
 * @param pkey_data_len Pointer to size_t which will be populated with size of
 * PKCS#1 or SEC1 DER structure
 * @returns 0 on success, an afw_error/aceHal_error/MbedTLS value on failure
 *
 */
int32_t crypto_utils_dump_pri_key_der(mbedtls_pk_context* key,
                                  uint8_t **pkey_data, size_t *pkey_data_len);

/**
 * @brief Dump private key in PEM format from MbedTLS Private Key context
 *
 * @param key Context with private key
 * @param pkey_data Pointer to a uint8_t* which will be populated with address
 * of allocated buffer holding PKCS#1 or SEC1 PEM string of private key. Buffer
 * allocated is of size ECC_PRIVATE_KEY_MAX_SIZE. Caller responsible to free
 * after use.
 * @returns 0 on success, an afw_error/aceHal_error/MbedTLS value on failure
 *
 */
int32_t crypto_utils_dump_pri_key_pem(mbedtls_pk_context* key,
                                  uint8_t **pkey_data);

/**
 * @brief Dump public key in DER format from MbedTLS Private Key context
 *
 * @param key Context with private key
 * @param pubkey_data Pointer to a uint8_t* which will be populated with address
 * of allocated buffer holding SubjectPublicKeyInfo DER structure. Buffer
 * allocated is of size ECC_PUBLIC_KEY_MAX_SIZE. Caller responsible to free
 * after use.
 * @param pubkey_data_len Pointer to size_t which will be populated with size of
 * SubjectPublicKeyInfo DER structure
 * @returns 0 on success, an afw_error/aceHal_error/MbedTLS value on failure
 *
 */
int32_t crypto_utils_dump_pub_der(mbedtls_pk_context* key,
                                  uint8_t **pubkey_data,
                                  size_t *pubkey_data_len);

/**
 * @brief Sign using ECDSA a SHA256 hash/message digest
 *
 * @param key Context with private key
 * @param data Buffer holding data to sign. Caller is expected to hash the
 * message and pass the digest holding the Message Digest. Use SHA256.
 * @param data_len Size of Buffer holding data i.e. message digest
 * @param signed_data Pointer to a uint8_t* which will be populated with address
 * of allocated buffer holding signature. Caller responsible to free after use.
 * @param signed_data_len Pointer to size_t which will be populated with size of
 * signature
 * @returns 0 on success, an afw_error/aceHal_error/MbedTLS value on failure
 *
 */
int32_t crypto_utils_ecdsa_sign(mbedtls_pk_context* key,
                                const uint8_t *data, const size_t data_len,
                                uint8_t **signed_data, size_t *signed_data_len);

/**
 * @brief Dump Certificate Signing Request in PEM format
 *
 * @param req Context holding a certificate signing request
 * @param pubkey_data Pointer to a uint8_t* which will be populated with address
 * of allocated buffer Certificate Signing Request PEM string. Buffer allocated
 * is of size CSR_MAX_SIZE. Caller responsible to free after use.
 * @returns 0 on success, an afw_error/aceHal_error/MbedTLS value on failure
 *
 */
int32_t crypto_utils_dump_csr_pem(mbedtls_x509write_csr* req, uint8_t** csr_buf);

#endif