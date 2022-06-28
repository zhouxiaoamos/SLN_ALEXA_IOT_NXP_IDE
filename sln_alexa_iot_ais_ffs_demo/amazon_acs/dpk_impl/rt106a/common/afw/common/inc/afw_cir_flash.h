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
 * AFW circular flash
 *******************************************************************************
 */

/**
 * @file
 *
 * This is an implementation of a circular flash
 *
 * This implementation contains a flash table with entries of atleast 8K
 * followed by data section. The flash table entries maintain address,
 * length and read/write/erase statuses of the data block for data integrity.
 *
 * The total size of this flash segment should be atleast 16K (including 8K of
 * flash table).
 *
 * @remark This implementation assumes contiguous 0xFF bytes no more than
 * 4096 bytes. This allows the library to recover safely after device reboots.
 *
 * -------- 0 -------------
 * FLASH TABLE
 * -------- 8K ------------
 * DATA
 * -------- +4K -----------
 * DATA
 * -------- +4K -----------
 */

#ifndef AFW_CIR_FLASH_H
#define AFW_CIR_FLASH_H

#include <stdint.h>

#include <FreeRTOS.h>
#include <event_groups.h>
#include <semphr.h>

#include <afw_error.h>

/**
 * @brief Circular flash table size
 *
 * This can be adjusted as needed by application. A minimum of 8K bytes
 * is required. At 8K bytes, this can store 512 entries of 16 bytes each.
 *
 * It should be 4K aligned.
 */
#ifndef AFW_CIR_FLASH_TABLE_SIZE
#define AFW_CIR_FLASH_TABLE_SIZE            8192
#endif

/**
 * @brief Metrics Error codes
 * @{
 */
/**
 * @brief Flash may be corrupted
 */
#define AFW_CIR_FLASH_ECORRUPT              101

/**
 * @brief Flash erase failed
 */
#define AFW_CIR_FLASH_EERASE_FAIL           102

/**
 * @brief Flash read failed
 */
#define AFW_CIR_FLASH_EREAD_FAIL            103

/**
 * @brief Flash write failed
 */
#define AFW_CIR_FLASH_EWRITE_FAIL           104

/**
 * @brief Cannot find flash partition
 */
#define AFW_CIR_FLASH_ECANT_FIND            105

/**
 * @brief Cannot access flash partition
 */
#define AFW_CIR_FLASH_EACCESS_FAIL          106

/**
 * @brief Flash partition is full
 */
#define AFW_CIR_FLASH_ENO_FREE_SPACE        107

/**
 * @brief The data is not in the state for the operation to be performed
 */
#define AFW_CIR_FLASH_EWRONG_STATE          108

/**
 * @brief No more bytes to read
 */
#define AFW_CIR_FLASH_ENO_BYTES             109

/**
 * @brief AFW_CIR_FLASH_TABLE_SIZE should be atleast 8192 bytes
 */
#define AFW_CIR_FLASH_EINVALID_TABLE_SIZE   110

/**
 * @brief Timed out trying to get access to flash
 */
#define AFW_CIR_FLASH_EACCESS_TIMEOUT       111

/**
 * @}
 */

/**
 * @brief Circular flash object
 */
typedef struct afw_cir_flash_s {
    struct fm_flash_partition *part;
    uint32_t size;
    uint16_t next_entry_id;
    uint32_t num_free_entries;
    uint32_t next_entry_read_offset;
    uint32_t next_entry_write_offset;
    uint32_t next_entry_erase_offset;
    uint32_t next_data_write_offset;
    uint32_t data_write_length;
    uint32_t next_data_read_offset;
    uint32_t data_read_length;
    uint32_t next_data_erase_offset;
    SemaphoreHandle_t mutex;
    EventGroupHandle_t erase_async_event;
} afw_cir_flash_t;

/**
 * @brief Circular flash info object
 */
typedef struct afw_cir_flash_info_s {
    uint32_t size;                       /* Total size of Data partition (exclude reserved space) */
    uint32_t entries;                    /* Total number of entries (exclude reserved entries) */
    uint32_t avail_free_size;            /* Available free size to write */
    uint32_t avail_free_entries;         /* Available free entries to write */
    uint32_t avail_erase_size;
    uint32_t avail_erase_entries;        /* Available entries to erase */
    uint32_t avail_unread_size;
    uint32_t avail_unread_entries;       /* Available entries to read */
} afw_cir_flash_info_t;

/**
 * @brief Initialize the circular flash module
 *
 * @param[in]  name  Name of the flash partition
 * @param[out] flash Pointer to a circular flash object
 * @return AFW_OK on success, an error on failure
 */
int32_t afw_cir_flash_init(char *name, afw_cir_flash_t *flash);

/**
 * @brief Deinitialize the circular flash module
 *
 * @param[in] flash Pointer to a circular flash object
 * @return AFW_OK on success, an error on failure
 */
int32_t afw_cir_flash_deinit(afw_cir_flash_t *flash);

/**
 * @brief Write the input data to the circular flash
 *
 * This function should be followed by 'afw_cir_flash_write_complete()'
 * to complete the write to flash.
 *
 * If no free space available error is returned, the circular flash is full
 * of data that may be unread or not erased. Call the
 *
 * @remark The maximum contiguous 0xFF bytes is limited to 4096 bytes.
 *
 * @param[in]  flash  Pointer to a circular flash object
 * @param[in]  length Length of the data buffer
 * @param[in]  buf    Pointer to a data buffer to be written
 * @return AFW_OK on success, an error on failure
 */
int32_t afw_cir_flash_write(afw_cir_flash_t *flash, uint32_t length,
                            const uint8_t *buf);

/**
 * @brief Update the flash entry data structure with write complete info
 *
 * This function should be called after by 'afw_cir_flash_write()' to
 * to update the flash entry data structure which maintains meta data for
 * data integrity.
 *
 * @param[in]  flash  Pointer to a circular flash object
 * @return AFW_OK on success, an error on failure
 */
int32_t afw_cir_flash_write_complete(afw_cir_flash_t *flash);

/**
 * @brief Read the requested bytes of data from circular flash
 *
 * This function should be followed by 'afw_cir_flash_read_complete()'
 * to complete the read from flash.
 *
 * @param[in]  flash  Pointer to a circular flash object
 * @param[in]  length Length of the data buffer
 * @param[out] buf    Pointer to a data buffer to be filled
 * @return AFW_OK on success, an error on failure
 */
int32_t afw_cir_flash_read(afw_cir_flash_t *flash, uint32_t length,
                           uint8_t *buf);

/**
 * @brief Update the flash entry data structure with read complete info
 *
 * This function should be called after by 'afw_cir_flash_read()' to
 * to update the flash entry data structure which maintains meta data for
 * data integrity.
 *
 * @param[in]  flash  Pointer to a circular flash object
 * @return AFW_OK on success, an error on failure
 */
int32_t afw_cir_flash_read_complete(afw_cir_flash_t *flash);

/**
 * @brief Erase and free the circular flash blocks that are read
 *
 * Erases 4K blocks of read/abandoned circular flash. This should be
 * called periodically preferrably from a low priority task that erases
 * and frees memory for write operations.
 *
 * @param[in]  flash  Pointer to a circular flash object
 * @return AFW_OK on success, an error from afw_error.h
 */
int32_t afw_cir_flash_erase(afw_cir_flash_t *flash);

/**
 * @brief Erase the entire flash partition
 *
 * This is an expensive operation and therefore should be used with caution.
 *
 * @param[in]  flash  Pointer to a circular flash object
 * @return AFW_OK on success, an error on failure
 */
int32_t afw_cir_flash_erase_partition(afw_cir_flash_t *flash);

/**
 * @brief Peek at the requested bytes of data from circular flash
 *
 * The data is treated as unread and the next afw_cir_flash_peek() or
 * afw_cir_flash_read() will return the same data.
 *
 * @param[in]  flash  Pointer to a circular flash object
 * @param[in]  length Length of the data buffer
 * @param[out] buf    Pointer to a data buffer to be filled
 * @return AFW_OK on success, an error on failure
 */
int32_t afw_cir_flash_peek(afw_cir_flash_t *flash, uint32_t length,
                           uint8_t *buf);

/**
 * @brief Get the length of the next data available
 *
 * @param[in]  flash  Pointer to a circular flash object
 * @param[out] length Pointer to return the length.
 * @return AFW_OK on success, an error on failure
 */
int32_t afw_cir_flash_get_read_data_length(afw_cir_flash_t *flash,
        uint32_t *length);

/**
 * @brief Get information about the circular flash partition
 *
 * @param[in]  flash  Pointer to a circular flash object
 * @param[out] info   Pointer to a circular flash info object to be filled
 * @return AFW_OK on success, an error on failure
 */
int32_t afw_cir_flash_get_info(afw_cir_flash_t *flash,
                               afw_cir_flash_info_t *info);

#endif /* AFW_CIR_FLASH_H */
