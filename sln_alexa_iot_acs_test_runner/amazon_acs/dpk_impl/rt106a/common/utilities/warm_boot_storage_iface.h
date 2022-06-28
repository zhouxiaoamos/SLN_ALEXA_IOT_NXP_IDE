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
 * Interface functions to read and write to memory persistant across warm reboots.
 *
 * Note:
 * (1) This is an "interface" file to manage warm boot storage. The requirement
 * for the storage is, the data written to the memory using this interface should
 * be persistant across soft reboots like, watchdog resets, resets called by reboot
 * command. But, this interface does not gurantee persistant data across hard power
 * cycles, i.e., if board is powered off and on, etc.
 *
 * (2) The interfaces may be called in handler mode.
 *
 * (3) The implementation of this interface need to be in application since
 * application may choose various devices, memory regions to provice this support.
 * The app can decide if it want to store in RTC memory, flash, or SRAM.
 *******************************************************************************
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Callback for read data from the storage
 *
 * @param[in] wbs_block_id     warm boot store block id
 * @param[in] offset           offset of the read location
 * @param[in] buf              buffer to storage read data
 * @param[in] length           length of the data to read
 * @return true on success
 */
typedef bool (*warm_boot_storage_read_cb)(int wbs_block_id, uint32_t offset, char *buf, size_t length);

/**
 * @brief Callback for write data to the storage
 *
 * @param[in] wbs_block_id     warm boot store block id
 * @param[in] offset           offset of the write location
 * @param[in] buf              buffer containing data to write
 * @param[in] length           length of the data to read
 * @return true on success
 */
typedef bool (*warm_boot_storage_write_cb)(int wbs_block_id, uint32_t offset, const char *buf, size_t length);

/**
 * @brief Callback for clear storage data
 *
 * @param[in] wbs_block_id     warm boot store block id
 * @param[in] offset           offset of the clear location
 * @param[in] length           length of the data to clear
 * @return true on success
 */
typedef bool (*warm_boot_storage_clear_cb)(int wbs_block_id, uint32_t offset, size_t length);

/**
 * @brief Interface for warm boot store callbacks
 */
typedef struct {
    int block_id;                           /**< warm boot store block id */
    warm_boot_storage_read_cb read_cb;      /**< warm boot store read data callback */
    warm_boot_storage_write_cb write_cb;    /**< warm boot store write data callback */
    warm_boot_storage_clear_cb clear_cb;    /**< warm boot store clear data callback */
} warm_boot_storage_iface_t;