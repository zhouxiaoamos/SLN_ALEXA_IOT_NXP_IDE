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
 * FLASH MANAGER APIs
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
 * Asynchronous flash erase is performed when underline physical flash supports
 * it. With either synchronous or asynchronous erase, client can issue a flash write request right
 * after a flash erase request. Flash manager guarantees that write will only happen after erase
 * is completed.  With asynchronous erase, client can also set a callback function to be
 * called when the erase operation completes. An unique client ID is required from each erase
 * caller to help identify that caller's previously registered callback.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "iot_flash.h"

/**
 * @name Max number of partitions supported
 */
#define FM_FLASH_MAX_PARTITIONS 32

/**
 * @name Max number of chars in partition name
 */
#define FM_FLASH_PARTITION_NAME_MAX   16
/**
 * @name invalid flash cient ID
 */
#define FM_BYPASS_CLIENT_ID   -1

/**
 * @anchor supported_erase_sizes
 * @name Supported erase sizes
 *
 */
/**@{*/
#define FM_FLASH_ERASE_SIZE_4K   0x01000
#define FM_FLASH_ERASE_SIZE_32K  0x08000
#define FM_FLASH_ERASE_SIZE_64K  0x10000

/* NXP: add 256K sector support */
#define FM_FLASH_ERASE_SIZE_256K 0x40000

#define FM_DEFAULT_FLASH_ERASE_SIZES (FM_FLASH_ERASE_SIZE_4K | \
                                      FM_FLASH_ERASE_SIZE_32K | \
                                      FM_FLASH_ERASE_SIZE_64K | \
                                      FM_FLASH_ERASE_SIZE_256K)
/**@}*/


#define FM_FLASH_MAX_SECTOR_ERASE_WAIT_TICKS  20000

/**
 * @anchor partition_flags
 * @name Partition flags
 */
/**@{*/
/**
 * @brief Bitmask to hold R/W flags for flash partitions
 */
enum fm_flash_rw_flags {
    FM_FLASH_READ_ONLY                 = 0,
    FM_FLASH_UNSECURED_WRITEABLE       = 1U << 0,
    FM_FLASH_SECURED_WRITEABLE         = 1U << 1,
};
/**@}*/

/**
 * @brief Struct containing a partition's name, size, position and flags
 */
typedef struct fm_flash_partition {
    /** @brief Partition name. Must be unique. */
    char name[FM_FLASH_PARTITION_NAME_MAX];

    /** @brief Partition size in bytes */
    uint32_t size;

    /** @brief Offset within the total flash address space */
    unsigned long offset;

    /** @brief Flag to indicate whether particular store should
     *         be encrypted or not (for kvs) */
    uint8_t encrypted   :1;

    /** @brief See @ref partition_flags "partition flags" */
    uint8_t flags       :7;

    /** @brief Partition is in use or not */
    bool busy;
} fm_flash_partition_t;

typedef IotFlashInfo_t fm_flash_info;
typedef IotFlashIoctlRequest_t fm_flash_ioctl_request;
typedef IotFlashCallback_t fm_flash_callback;

/**
 * @brief Initialize a flash partition table. This apis should be called
 *        only once during system initialization. Caller must place the
 *        partition table in permanent storage as flash manager will
 *        be only referencing the table and not making a copy of it.
 *
 * @param parts Pointer to the flash partition table to be initialized.
 * @param nr_parts Number of partitions in the table
 *
 * @return An error code from afw_error.h
 */
int fm_flash_init(struct fm_flash_partition *parts, int nr_parts);

/**
 * @brief Get the fm_flash_partition struct for a partition
 *        from path name.
 */
struct fm_flash_partition *fm_flash_get_partition(const char *name);

/**
 * @brief Get the fm_flash_partition struct for a partition
 *        from base address.
 *
 * @param addr - Negative values will automatically return NULL.
 *             Otherwise, function will look up flash partitions
 *             from internal flash map
 *
 */
struct fm_flash_partition *fm_flash_get_partition_from_address(const int64_t addr);

/**
 * @brief Read from flash device or a partition.
 *
 * @param part The handle to the flash partition you want to perform the read on.
 *             When this is NULL, read from flash device instead of a partition.
 * @param[optional] client_id returned by get_client_id().
 *    See fm_flash_get_unique_client_id() for more info.
 * @param from The offset from partition base address to read data from
 *             The offset from flash device base if param part is NULL.
 * @param len Number of bytes to read
 * @param buf Buffer to hold the read data
 *
 * @return number of bytes read or an error code from afw_error.h
 */
int fm_flash_read(struct fm_flash_partition *part, int client_id,
                  unsigned long from, uint32_t len, uint8_t *buf);

/**
 * @brief Write to flash device or a partition.
 *
 * @param part The handle to the flash partition you want to perform the write on.
 *             When this is NULL, write to flash device instead of a partition.
 * @param[optional] client_id returned by get_client_id.
 *    See fm_flash_get_unique_client_id() for more info.
 * @param to The offset from partition base address to write data to
 *             The offset from flash device base if param part is NULL.
 * @param len Number of bytes to write
 * @param buf Holds the data to write
 *
 * @return number of bytes written or an error code from afw_error.h
 */
int fm_flash_write(struct fm_flash_partition *part, int client_id,
                   unsigned long to, uint32_t len, const uint8_t *buf);

/**
 * @brief Erase a partition.
 *
 * Erase is an asynchronous operation unless it is not supported by underline
 * hardware, in which case erase will be perform synchronously.
 *
 * In the case of asynchronous erase, flash manager will make sure that
 * subsequent flash read/write will not occur until erase is completed. User
 * who wish to do a flash write after erase can simply call fm_flash_erase()
 * and then fm_flash_write() and be assured the operations will happen in that
 * order.
 *
 * If asynchronous erase is supported, Callers can use fm_flash_set_callback()
 * to set a callback function to be called after erase complete.
 *
 * If asynchronous erase is NOT supported, Callers are not allowed to set
 * callback, and fm_flash_set_callback() will return -Unsupported Operation Error.
 *
 * @param flash The handle to the flash area you want to perform the erase on
 * @param client id used to find corresponding callback function.
 *
 * @return An error code from afw_error.h
 */
int fm_flash_erase(struct fm_flash_partition *part, int client_id);

/**
 * @brief Perform erase on number of sectors as opposed to by partition.
 *
 * Erase is an asynchronous operation unless it is not supported by underline
 * hardware, in which case erase will be perform synchronously.
 *
 * In the case of asynchronous erase, flash manager will make sure that
 * subsequent flash read/write will not occur until erase is completed. User
 * who wish to do a flash write after ease can simply call fm_flash_erase_sectors()
 * and then fm_flash_write() and be assured the operations will happen in that
 * order.
 *
 * If asynchronous erase is supported, Callers can use fm_flash_set_callback()
 * to set a callback function to be called after erase complete.
 *
 * If asynchronous erase is NOT supported, Callers are not allowed to set
 * callback, and fm_flash_set_callback() will return -Unsupported Operation Error.
 *
 * @param flash The handle to the flash area you want to perform the erase on
 * @param offset The offset within the flash partition you want to start
 *               erasing from. The offset must be aligned to sector size.
 * @param xBytes The size of the area you want to erase.
 *               xBytes must be aligned to sector size.
 * @param client id used to find corresponding callback function.
 *
 * @return An error code from afw_error.h
 */
int fm_flash_erase_sectors(struct fm_flash_partition *part,
                           int client_id,
                           size_t offset,
                           size_t xBytes);

/**
 * @brief set erase callback for each flash client.
 *
 * If asynchronous erase is supported, caller can pass a callback to be called
 * after erase is done.
 *
 * If asynchronous erase is NOT supported, this API will return an error (-Unsupported
 * Operation Error), and fm_flash_erase() and fm_flash_erase_sector() will function in
 * blocking mode.
 *
 * @param[in]   client_id       Client ID used to identify the callback's owner
 * @param[in]   xCallback       The callback function to be called.
 * @param[in]   pvUserContext   The user context to be passed when callback is called.
 *
 * @return An error code from afw_error.h
 */
int fm_flash_set_callback(int client_id,
                          fm_flash_callback call_back,
                          void* client_context);

/**
 * @brief Gets information about flash device.
 *
 * @return  pointer to flash information or NULL if request failed.
 */
fm_flash_info* fm_flash_getinfo(void);

/**
 * @brief Configure flash with user configuration and get the configuration
 * details.
 *
 * @param[in]   xRequest    configuration request.
 * @param[in]   xBytes      number of bytes to be read.
 * @param[out]  pvBuffer   Data buffer to hold the data read from flash
 *
 * @return An error code from afw_error.h
 */
int32_t fm_flash_ioctl(fm_flash_ioctl_request xRequest,
                        size_t xBytes,
                        void * const pvBuffer);

/**
 * @brief return a unique client id to each caller.
 *
 * Client must call this api to obtain a unique flash client ID if
 * the client wants to set erase callback or use other features flash
 * manager may support in the future that requires client ID.  To
 * ensure its erase callback function get invoked, a client needs to:
 * 1. obtain client ID by calling this API.
 * 2. call fm_flash_set_callback API and pass in client ID.
 * 3. call fm_flash_erase API and pass in client ID.
 *
 * A client may also have more than 1 callback functions to be used in
 * different erase situations. In such case, the client can use the same
 * client ID and call fm_flash_set_callback API  to re-set its
 * callback function each time before each erase situation. Alternatively,
 * the client can request multiple client IDs and setup multiple callbacks.
 * Just make sure to pass the correct client ID when calling erase API to
 * get its corresponding callback invoked.
 *
 * Client can skip calling this API as well as fm_flash_set_callback
 * if it doesn't need to used callback or other features that requires
 * client ID. In such case, the client can use FM_BYPASS_CLIENT_ID as
 * its client ID when calling flash manager APIs.
 *
 * Flash manager assigns client id from 0 and up. A negative value
 * is considered invalid.
 *
 * @return unique client id to caller.
 */
int fm_flash_get_unique_client_id(void);

/**
 * @brief Get a pointer to the  partition table.
 * @param[out]   size    size of current partition table.
 *
 * @return pointer to the partition table
 */
struct fm_flash_partition *fm_flash_get_partition_table(int *size);

/**
 * @brief Get max possible erase size for the given offset and size.
 *
 * @param[in] offset The offset from flash device base.
 * @param[in] size The target size.
 * @param[out] blocksize Max possible block size that can erase.
 *
 * @return AFW_OK if find a suitible block size. Otherwise, returns AFW_EINVAL.
 */
int fm_flash_get_max_erase_block_size(uint32_t offset, uint32_t size, int *blocksize);
