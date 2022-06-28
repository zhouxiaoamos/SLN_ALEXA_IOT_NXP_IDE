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
 * AFW Flash APIs
 *******************************************************************************
 */

/**
 * @file
 * @brief This module builds on top of the flash hardware abstraction layer and adds partitioning
 * and asynchronous erase.
 *
 * Partitioning: with this module, you can divide the flash into multiple partitions,
 * each with a textual name, a starting address and an offset, and a flash specifying whether the
 * partition is writable or not. You then treat each partition as a separate area of storage.
 *
 * Asynchronous erase: when you submit an erase operation, it will be handled by an erase service.
 * This service may suspend the erase operation if any read/write operations are made in the
 * meantime. A callback will be called when the erase operation completes.
 */

#ifndef AFW_FLASH_API_H
#define AFW_FLASH_API_H

#include <stdint.h>
#include <ace/aceCli.h>
#include "FreeRTOS.h"
#include "flash_map.h"

extern const aceCli_moduleCmd_t afw_flash_cli[];

/**
 * @name Max number of partitions supported
 */
#define AFW_FLASH_MAX_PARTITIONS 32

/**
 * @name Max number of chars in partition name
 */
#define AFW_FLASH_PARTITION_NAME_MAX   16

/**
 * @anchor erase_request_states
 * @name Erase request states
 */
/**@{*/
#define AFW_FLASH_ERASE_PENDING    0x01
#define AFW_FLASH_ERASING          0x02
#define AFW_FLASH_ERASE_SUSPEND    0x04
#define AFW_FLASH_ERASE_DONE       0x08
#define AFW_FLASH_ERASE_FAILED     0x10
/**@}*/

/**
 * @anchor supported_erase_sizes
 * @name Supported erase sizes
 *
 */
/**@{*/
#define AFW_FLASH_ERASE_SIZE_4K   0x01000
#define AFW_FLASH_ERASE_SIZE_32K  0x08000
#define AFW_FLASH_ERASE_SIZE_64K  0x10000
/**@}*/

#define AFW_FLASH_MAX_SECTOR_ERASE_WAIT_TICKS  20000

/**
 * @anchor partition_flags
 * @name Partition flags
 */
/**@{*/
/**
 * @brief Bitmask to hold R/W flags for flash partitions
 */
enum afw_flash_rw_flags {
    AFW_FLASH_READ_ONLY                 = 0,
    AFW_FLASH_UNSECURED_WRITEABLE       = 1U << 0,
    AFW_FLASH_SECURED_WRITEABLE         = 1U << 1,
    // Product controllable secured-writable flag; this is AND'd with the
    // SECURE_WRITEABLE flag, based on the product-specific security state.
    AFW_FLASH_MODULE_SECURED_WRITEABLE = 1U << 6,    // Max value of flags
};
/**@}*/


/**@}*/

struct flash_info;

/**
 * @brief Struct containing a partition's name, size, position and flags
 */
struct flash_partition {
    /** @brief Partition name. Must be unique. */
    const char *name;

    /** @brief Partition size in bytes */
    uint32_t size;

    /** @brief Offset within the total flash address space */
    uint32_t offset;

    /** @brief Flag to indicate whether particular store should
     *         be encrypted or not (for kvs) */
    uint8_t encrypted   :1;

    /** @brief See @ref partition_flags "partition flags" */
    uint8_t flags       :7;
};

/**
 * @brief Struct containing a description of an erase operation
 */
struct erase_info {
    /** @brief Flash info for the partition or device to perform operation on*/
    struct flash_info  *flash;

    /** @brief Erase operation start address */
    uint32_t           addr;

    /** @brief Erase length. Must be a @ref supported_erase_sizes "supported erase size". */
    uint32_t           len;

    /** @brief Number of erase retries. NOT YET IMPLEMENTED. */
    uint8_t            retries;

    /** @brief Erase state to be used by callbacks. Will be an
     *         @ref erase_request_states "erase request state" */
    uint8_t            state;

    /** @brief callback function to be called after erase is done */
    void (*callback) (struct erase_info *self);

    /** @brief pointer to caller's data
     *
     *  You could use this pointer to save some relevant data needed by the erase done callback.
     */
    void               *priv;
};

/**
 * @brief Initialize a flash partition table using a flash device.
 *
 * Uses default partitions if parts is NULL
 * Uses default platform_spi_flash if device is NULL
 *
 * @return An error code from afw_error.h
 */
int afw_flash_init(const struct flash_info *device, const struct flash_partition *parts, int nr_parts);

/**
 * @brief Get the flash_info struct for a partition or a device
 *        from path name.
 */
struct flash_info *afw_flash_get_info(const char *name);

/**
 * @brief Get the flash_info struct for a partition or a device
 *        from base address.
 *
 * @param addr - Negative values will automatically return
 *             external flash info. Otherwise, function will
 *             look up flash partitions from internal flash map
 *
 */
struct flash_info *afw_flash_get_info_from_address(const int64_t addr);

/**
 * @brief Get the name of a partition or a device.
 */
const char * afw_flash_get_name(struct flash_info* flash);

/**
 * @brief Get the size of a partition or a device.
 */
uint32_t afw_flash_get_size(struct flash_info *flash);

/**
 * @brief Submit an erase request.
 *
 * Erase is an asynchronous operation.
 * Callers should pass a callback to be called after erase is done.
 *
 * @param flash The handle to the flash area you want to perform the erase on
 *
 * @param info The erase operation which you want to be performed. This will be
 * modified during this call.
 *
 * @return An error code from afw_error.h
 */
int afw_flash_erase(struct flash_info *flash, struct erase_info *info);

/**
 * @brief Submit a smart erase request.
 *
 * Erase is an asynchronous operation.
 * Callers should pass a callback to be called after erase is done.
 *
 * @param flash The handle to the flash area you want to perform the erase on
 *
 * @param info The erase operation which you want to be performed. This will be
 * modified during this call.
 *
 * @return An error code from afw_error.h
 */
int afw_flash_smart_erase(struct flash_info *flash, struct erase_info *info,
                          TickType_t max_sector_erase_wait_ticks);

/**
 * @brief Updates product-specific security state to control write-access
 * to relevant secure flash partitions.
 *
 * @param writeable Controls whether writes are allowed / disabled
 * @return int 0 on success, non-zero otherwise
 */
int afw_flash_update_module_security_state(bool writeable);

/**
 * @brief Read from flash device or a partition.
 *
 * @return An error code from afw_error.h
 */
int afw_flash_read(struct flash_info *flash, uint32_t from, uint32_t len,
                   uint8_t *buf);

/**
 * @brief Write to flash device or a partition.
 *
 * @return An error code from afw_error.h
 */
int afw_flash_write(struct flash_info *flash, uint32_t to, uint32_t len,
                    const uint8_t *buf);

/**
 * @brief Calculate the SHA256 hash of a flash area
 *
 * @return An error code from afw_error.h
 */
int32_t afw_flash_sha256(struct flash_info* flash, uint32_t from, uint32_t len, uint8_t * out_32);

/**
 * @brief Get master device struct from a partition struct.
 */
struct flash_info *afw_flash_get_master(struct flash_info *flash);

/**
 * @brief Print partition info (offset, size, name)
 */
void afw_flash_print_partitions(void);

/*
 * Get maximum erase block supported by this flash
 */
int afw_flash_get_max_erase_block(struct flash_info *flash);

/*
 * Get minimum erase block supported by this flash
 */
int afw_flash_get_min_erase_block(struct flash_info *flash);

/*
 * Is size eraseable?
 * If there is a supported erase block less than or equal to
 * size, then sets blocksize to that size and returns 1.
 * Otherwise,returns 0.
 */
int afw_flash_can_erase_block(struct flash_info *flash, uint32_t offset,
                              uint32_t size, int *blocksize);

/**
* @brief api to check if partions is clean/empty
*
* Checks all bytes in partition are 0xFF
*
* @return AFW_TRUE if clean or AFW_FALSE if dirty or error
*/
int afw_is_partition_empty(struct flash_info *flash);

int afw_flash_get_writeable_mode(void);

#endif /* AFW_FLASH_API_H */
