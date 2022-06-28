/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */
#ifndef __WIFI_MGR_UTILS_H__
#define __WIFI_MGR_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/ace_status.h>

// MD5 hashes are 128 bits in length and represented by 32 hex digits.
#define ACE_WIFI_MD5_DIGEST_LENGTH 16
#define ACE_WIFI_MD5_HEX_OUTPUT_LENGTH (ACE_WIFI_MD5_DIGEST_LENGTH * 2 + 1)

/**
 * @defgroup ACE_WIFI_DEF structs, defines, enums
 * @{
 * @ingroup ACE_WIFI
 */

/** The structure to contain the output MD5 obfuscated SSID string, NULL
    terminated */
typedef char aceWifiMgr_obfuscatedSsid[ACE_WIFI_MD5_HEX_OUTPUT_LENGTH];

/**
 * @defgroup ACE_WIFI_API Public API
 * @{
 * @ingroup ACE_WIFI
 */

/**
 * Obfuscate Wi-Fi SSID with MD5.
 *
 * @param in:      input SSID, does not need to be NULL terminated.
 * @param in_len:  input SSID length, must be more than 0.
 * @param out:     buffer passed in for the output string.
 *                 return buffer contains NULL terminated MD5 obfuscated string.
 * @return ace_status_t: ACE_STATUS_OK on success, error code if failed.
 */
ace_status_t aceWifiMgr_obfuscateSsid(const char* in, size_t in_len,
                                      aceWifiMgr_obfuscatedSsid out);

/** @} */  // ACE_WIFI_API

#ifdef __cplusplus
}
#endif

#endif  // __WIFI_MGR_UTILS_H__
