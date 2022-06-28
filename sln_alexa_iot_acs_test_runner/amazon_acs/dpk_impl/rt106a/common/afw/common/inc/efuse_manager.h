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
 * EFUSE MANAGER APIs
 *
 *******************************************************************************
 */

/**
 * @file
 * @brief This module builds on top of the EFUSE hardware abstraction layer and
 * provides specific apis for app to use.
 */

#ifndef EFUSE_MANAGER_H
#define EFUSE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "iot_efuse.h"

/**
 * @brief Query efuse/OTP to check if device is a production device
 *        (i.e only takes production key)
 *
 * @param [out] production_device: set to true if device is production device else false
 *
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_is_device_production_device(bool *production_device);

/**
 * @brief Query efuse/OTP to check if device has been locked
 *        (i.e production locked device)
 *
 * @param [out] locked: set to true if device is locked else false
 *
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_is_device_locked(bool *locked);

/**
 * @brief Query efuse/OTP to check if device is secure boot enabled.
 *
 * @param [out] sb_enabled: set to true if device is secureboot enabled else false
 *
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_is_device_secure_boot_enabled(bool *sb_enabled);

/**
 * @brief Query efuse/OTP to check if device is otfad enabled
 *
 * @param [out] otfad_enabled: set to true if device is otfad enabled else false
 *
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_is_device_otfad_enabled(bool *otfad_enabled);

/**
 * @brief Query efuse/OTP to check if device is jtag disabled
 *
 * @param [out] jtag_disabled: set to true if device is jtag disabled else false
 *
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_is_device_jtag_disabled(bool *jtag_disabled);

/**
 * @brief Query efuse/OTP to get min. boot version
 *
 * @param [out] version: get the min version of firmware allowed to boot
 *
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_get_boot_version(uint16_t *version);

/**
 * @brief Write efuse/OTP to set min. boot version
 *
 * @param [in] version: set the min version of firmware allowed to boot
 *                      version must not be less than the current efuse value.
 *                      version must not be greater than the maxium value
 *                      supported by the underline efuse.
 *
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_set_boot_version(uint16_t version);

/**
 * @brief API to implement unlock capability per platform
 * @param [in] reboots
 *             if <reboots> is 0: Permanent unlock
 *             if <reboots> is 1: One-time unlock
 *             if <reboots> is [2-15]: Temporarly unlock
 *
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_unlock_device(uint8_t reboots);

/**
 * @brief API to relock the device no matter unlock type
 *
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_relock_device(void);

/**
 * @brief API to retrive the maximum number of reboots
 *            supported per unlock by the platform.
 *
 * @param [out] number of reboots supported
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_get_max_unlock_reboots(uint8_t *reboots);

/**
 * @brief API to retrive the permanet unlock times left
 *            supported by the platform.
 *
 * @param [out] number of permanent unlock left
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_get_permanent_unlock_times_left(uint8_t *times);

/**
 * @brief API to retrive the temporary unlock times left
 *            supported by the platform.
 *
 * @param [out] number of temporary unlock left
 * @return An error code from iot_efuse.h
 */
int32_t efuse_mgr_get_temporary_unlock_times_left(uint8_t *times);

#endif /* EFUSE_MANAGER_H */
