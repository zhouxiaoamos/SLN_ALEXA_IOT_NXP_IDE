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
 * @brief APIs used by DHA HAL to abstract out Key, Cert chain storage
 */

#ifndef PORT_DHA_STORAGE_H
#define PORT_DHA_STORAGE_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Store private key value. Contains both Private and Public key pair.
 *
 * @param buf Key to be set
 * @param buf_len Size of key. Includes terminating NULL in case of strings
 * (e.g. PEM).
 * @returns 0 on success, an afw_error/aceHal_error value on failure
 *
 */
int32_t port_dha_write_key_pair(const uint8_t* buf, size_t buf_len);

/**
 * @brief Retrieve private key value. Contains both Private and Public key pair.
 *
 * @param buf Buffer to read key into
 * @param max_buf_len maximum size of buffer to safely read into. Validity of
 * first max_buf_len bytes not guaranteed when buffer not big enough hold entire
 * key pair.
 * @returns length of key on success, an afw_error/aceHal_error value on failure
 *
 */
int32_t port_dha_read_key_pair(uint8_t* buf, size_t max_buf_len);

/**
 * @brief Store DHA certificate chain.
 *
 * @param buf Buffer containing certificate chain
 * @param buf_len Size of certificate chain. Includes terminating NULL in case
 * of strings (e.g. PEM).
 * @returns 0 on success, an afw_error/aceHal_error value on failure
 *
 */
int32_t port_dha_write_cert_chain(const uint8_t* buf, size_t buf_len);

/**
 * @brief Retrieve DHA certificate chain.
 *
 * @param buf Buffer to read certificate chain into
 * @param max_buf_len maximum size of buffer to safely read into. Validity of
 * first max_buf_len bytes not guaranteed when buffer not big enough hold entire
 * certificate chain.
 * @returns length of cert chain on success, an afw_error/aceHal_error value on
 * failure. Length should include terminating NULL byte if in PEM format.
 *
 */
int32_t port_dha_read_cert_chain(uint8_t* buf, size_t max_buf_len);

/**
 * @brief Retrieve DHA leaf certificate (device certificate)
 *
 * @param buf Buffer to read leaf certificate into
 * @param max_buf_len maximum size of buffer to safely read into. Validity of
 * first max_buf_len bytes not guaranteed when buffer not big enough hold entire
 * certificate chain.
 * @returns length of cert chain on success, an afw_error/aceHal_error value on
 * failure. Length should include terminating NULL byte if in PEM format.
 *
 */
int32_t port_dha_read_leaf_cert(uint8_t* buf, size_t max_buf_len);

/**
 * @brief Get DHA certificate chain size
 *
 * @returns length of cert chain on success, an afw_error/aceHal_error value on
 * failure. Length should include terminating NULL byte if in PEM format.
 * Length will never be 0.
 */
int32_t port_dha_get_cert_chain_size(void);

#endif  //  PORT_DHA_STORAGE_H