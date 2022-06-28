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

#pragma once

#include <ace/hal_wifi.h>

/**
 * @brief V1 wifi profile version number
 */
#define WIFI_PROFILE_VERSION_V1    0

/**
 * @brief Find V1 profile in kvs and convert it to V2 profile.
 *
 * The function looks for wifi V1 profile in kvs. If it exist, update the version
 * number, and convert it to V2 profile.
 *
 * @param[out] profile reference to V2 wifi config struct
 * @param[out] version reference of version number.
 */
void aceWifiHal_profile_v1_to_v2(aceWifiHal_config_t *profile, int32_t *version);

/**
 * @brief Remove V1 wifi profile from kvs
 */
void aceWifiHal_profile_remove_v1(void);
