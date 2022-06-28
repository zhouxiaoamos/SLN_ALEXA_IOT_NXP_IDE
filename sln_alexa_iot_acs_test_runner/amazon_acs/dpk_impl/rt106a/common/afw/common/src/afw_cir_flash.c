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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <afw_def.h>
#include <afw_cir_flash.h>

#include <flash_manager.h>

#define MIN_FREE_ENTRIES       (1)

#define DEFAULT_TABLE_SIZE     (8192)
#define FLASH_BLOCK_SIZE       (4096)
#define BITS32_DEFAULT_VALUE   (0xFFFFFFFF)
#define BITS16_DEFAULT_VALUE   (0xFFFF)

/* Checking atleast 5 bytes to avoid int values(4 bytes) with 0xFFs */
#define MIN_FREE_BYTES_COUNT   (5)
#define LOCAL_BUF_SIZE         (256)

/* Status flags - 2 bytes */
#define STATUS_WRITE_START     (1 << 0)
#define STATUS_WRITE_COMPLETE  (1 << 1)
#define STATUS_READ_START      (1 << 2)
#define STATUS_READ_COMPLETE   (1 << 3)
#define STATUS_ABANDONED       (1 << 4)
#define STATUS_ERASE_START     (1 << 5)
#define STATUS_ERASE_COMPLETE  (1 << 6)

#define SEMAPHORE_TIMEOUT      (5000 / portTICK_PERIOD_MS)

typedef struct flash_table_entry_s {
    uint32_t offset;
    uint32_t length;
    uint16_t id;
    uint16_t status;
    uint32_t reserved;
} flash_table_entry_t;

static int32_t return_unblock(afw_cir_flash_t *flash, int32_t error)
{
    xSemaphoreGive(flash->mutex);

    return error;
}

static bool is_block_aligned(uint32_t offset)
{
    return (offset % FLASH_BLOCK_SIZE ? false : true);
}

static uint32_t get_next_entry_offset(uint32_t offset)
{
    offset += sizeof(flash_table_entry_t);

    if (offset == AFW_CIR_FLASH_TABLE_SIZE) {
        offset = 0;
    }

    return offset;
}

static uint32_t get_previous_entry_offset(uint32_t offset)
{
    if (offset) {
        offset -= sizeof(flash_table_entry_t);
    } else {
        offset = AFW_CIR_FLASH_TABLE_SIZE - sizeof(flash_table_entry_t);
    }

    return offset;
}

static uint32_t get_next_flash_table_block_offset(uint32_t offset)
{
    offset += FLASH_BLOCK_SIZE;
    if (offset == AFW_CIR_FLASH_TABLE_SIZE) {
        offset = 0;
    }

    return offset;
}

static uint32_t get_previous_flash_table_block_offset(uint32_t offset)
{
    if (offset) {
        offset -= FLASH_BLOCK_SIZE;
    } else {
        offset = AFW_CIR_FLASH_TABLE_SIZE - FLASH_BLOCK_SIZE;
    }

    return offset;
}

static uint32_t get_next_data_block_offset(afw_cir_flash_t *flash,
        uint32_t offset)
{
    offset += FLASH_BLOCK_SIZE;
    if (offset == flash->size) {
        offset = AFW_CIR_FLASH_TABLE_SIZE;
    }

    return offset;
}

static uint32_t get_block_aligned_offset(uint32_t offset)
{
    uint32_t aligned_offset;
    aligned_offset = (offset < FLASH_BLOCK_SIZE) ? 0
                     : offset - (offset % FLASH_BLOCK_SIZE);

    return aligned_offset;
}

static int32_t update_entry_status(afw_cir_flash_t *flash, uint32_t offset,
                                   uint16_t status, flash_table_entry_t *entry)
{
    int32_t ret;

    entry->status &= ~status;
    ret = fm_flash_write(flash->part, FM_BYPASS_CLIENT_ID, offset, sizeof(flash_table_entry_t),
                          (const uint8_t *)entry);
    if (ret < 0) {
        return -AFW_CIR_FLASH_EWRITE_FAIL;
    }

    return AFW_OK;
}

static int32_t erase_block(afw_cir_flash_t *flash, uint32_t offset,
                           uint32_t length)
{
    EventBits_t eraseEventsBits;
    int32_t ret;

    if (!is_block_aligned(offset)) {
        return -AFW_CIR_FLASH_ECORRUPT;
    }

    ret = fm_flash_erase_sectors(flash->part, FM_BYPASS_CLIENT_ID, offset, length);
    return ret;
}

static int32_t is_entry_erase_complete(afw_cir_flash_t *flash, uint32_t offset,
                                       bool *complete)
{
    int32_t ret;
    flash_table_entry_t entry;
    uint16_t status;
    *complete = false;

    ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, offset, sizeof(flash_table_entry_t),
                         (uint8_t *)&entry);
    if (ret < 0) {
        return -AFW_CIR_FLASH_EREAD_FAIL;
    }

    status = ~(entry.status);
    *complete = ((status & STATUS_ERASE_COMPLETE) == STATUS_ERASE_COMPLETE);

    return AFW_OK;
}

static int32_t erase_flash_table(afw_cir_flash_t *flash)
{
    int32_t ret;
    uint32_t erase_offset = 0;
    flash_table_entry_t entry;
    bool complete = false;

    /* Mark entries as erase complete */
    do {
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, flash->next_entry_erase_offset,
                             sizeof(flash_table_entry_t), (uint8_t *)&entry);
        if (ret < 0) {
            return -AFW_CIR_FLASH_EREAD_FAIL;
        }

        uint16_t status = ~(entry.status);
        /* Only the first erase entry has erase start bit set */
        if ((status & STATUS_ERASE_START) == STATUS_ERASE_START) {
            erase_offset = get_block_aligned_offset(entry.offset);
        }

        ret = update_entry_status(flash, flash->next_entry_erase_offset,
                                  STATUS_ERASE_START | STATUS_ERASE_COMPLETE,
                                  &entry);
        if (ret != AFW_OK) {
            return return_unblock(flash, ret);
        }

        flash->next_entry_erase_offset =
            get_next_entry_offset(flash->next_entry_erase_offset);

    } while (entry.offset + entry.length < erase_offset + FLASH_BLOCK_SIZE);

    for (erase_offset = 0; erase_offset < AFW_CIR_FLASH_TABLE_SIZE;
         erase_offset += FLASH_BLOCK_SIZE) {

        ret = is_entry_erase_complete(flash, erase_offset, &complete);
        if (ret != AFW_OK) {
            return ret;
        }

        if (complete) {
            uint32_t last_offset = erase_offset + FLASH_BLOCK_SIZE
                                   - sizeof(flash_table_entry_t);
            ret = is_entry_erase_complete(flash, last_offset, &complete);
            if (ret != AFW_OK) {
                return ret;
            }

            if (complete) {
                ret = erase_block(flash, erase_offset, FM_FLASH_ERASE_SIZE_4K);
                if (ret != AFW_OK) {
                    return ret;
                }

                flash->num_free_entries +=
                    (FLASH_BLOCK_SIZE / sizeof(flash_table_entry_t));
            }
        }
    }

    return AFW_OK;
}

static int32_t is_flash_empty(afw_cir_flash_t *flash, bool *empty)
{
    uint32_t data;
    uint32_t offset = 0;
    int32_t ret;

    *empty = true;

    /* read first 4 bytes of each block to determine if the flash is empty */
    /* TODO: just looping through flash table should be good */
    while (offset < flash->size) {
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, offset, sizeof(uint32_t),
                             (uint8_t *)&data);
        if (ret < 0) {
            return -AFW_CIR_FLASH_EREAD_FAIL;
        }

        if (data != BITS32_DEFAULT_VALUE) {
            *empty = false;
            break;
        }

        offset += FLASH_BLOCK_SIZE;
    }

    return AFW_OK;
}

static uint32_t get_free_size(afw_cir_flash_t *flash)
{
    uint32_t free_size = 0;

    /* free_size: erase pointer - write pointer */
    if (flash->next_data_write_offset >= flash->next_data_erase_offset) {
        free_size = flash->size - flash->next_data_write_offset;
        free_size += (flash->next_data_erase_offset - AFW_CIR_FLASH_TABLE_SIZE);
    } else {
        free_size = flash->next_data_erase_offset
                    - flash->next_data_write_offset;
    }

    /* Atleast 4K should be free for calculating incomplete data entry length */
    if (free_size > FLASH_BLOCK_SIZE) {
        free_size -= FLASH_BLOCK_SIZE;
    } else {
        free_size = 0;
    }

    return free_size;
}

static int32_t find_incomplete_data_length(afw_cir_flash_t *flash,
        uint32_t offset, uint32_t *length)
{
    uint8_t buf[LOCAL_BUF_SIZE];
    uint32_t buf_len;
    int32_t i;
    int32_t ret;
    int32_t end_offset;

    /* Atleast 4K bytes are available */
    *length = 0;
    end_offset = get_block_aligned_offset(offset);
    if (offset - end_offset) {
        end_offset = get_next_data_block_offset(flash, end_offset);
        *length  += end_offset - offset;
    }
    end_offset = get_next_data_block_offset(flash, end_offset);
    *length  += FLASH_BLOCK_SIZE;

    end_offset -= LOCAL_BUF_SIZE;
    *length  -= LOCAL_BUF_SIZE;
    buf_len = LOCAL_BUF_SIZE;
    do {
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, end_offset, buf_len, buf);
        if (ret < 0) {
            return -AFW_CIR_FLASH_EREAD_FAIL;
        }

        for (i = buf_len - 1; i >= 0; i--) {
            if (buf[i] != 0xFF) {
                *length += i;
                return AFW_OK;
            }
        }

        if (*length == 0) {
            break;
        }

        if (*length >= LOCAL_BUF_SIZE) {
            end_offset -= LOCAL_BUF_SIZE;
            if (end_offset < AFW_CIR_FLASH_TABLE_SIZE) {
                end_offset = flash->size - LOCAL_BUF_SIZE;
            }
            *length  -= LOCAL_BUF_SIZE;
            buf_len = LOCAL_BUF_SIZE;
        } else {
            end_offset -= *length;
            if (end_offset < AFW_CIR_FLASH_TABLE_SIZE) {
                end_offset = flash->size - *length;
            }
            buf_len = *length;
            *length = 0;
        }

    } while (1);

    return -AFW_EBUSY;
}

static int32_t get_write_offset(afw_cir_flash_t *flash)
{
    int32_t ret;
    flash_table_entry_t entry;
    uint32_t write_block;
    uint32_t offset;
    uint32_t start;
    uint32_t end;
    uint32_t mid;

    flash->num_free_entries =
        (AFW_CIR_FLASH_TABLE_SIZE / sizeof(flash_table_entry_t));

    flash->next_data_write_offset = BITS32_DEFAULT_VALUE;
    flash->next_entry_write_offset = BITS32_DEFAULT_VALUE;

    write_block = BITS32_DEFAULT_VALUE;
    /*
     * Find the first block with atleast one entry.
     *
     * This function is called only if there is an entry, so not checking
     * for empty flash table case. Also, the table will have atleast one
     * entry free. So, not checking for full table case.
     */
    for (offset = 0; offset < AFW_CIR_FLASH_TABLE_SIZE;
         offset += FLASH_BLOCK_SIZE) {

        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, offset, sizeof(flash_table_entry_t),
                             (uint8_t *)&entry);
        if (ret < 0) {
            return -AFW_CIR_FLASH_EREAD_FAIL;
        }

        if (entry.status != BITS16_DEFAULT_VALUE) {
            write_block = offset;
            break;
        }
    }

    if (write_block == BITS32_DEFAULT_VALUE) {
        return -AFW_CIR_FLASH_ECORRUPT;
    }

    offset = write_block;
    do {
        start = offset;
        end = offset + FLASH_BLOCK_SIZE - sizeof(flash_table_entry_t);

        /* Check the last entry if the block is full */
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, end, sizeof(flash_table_entry_t),
                             (uint8_t *)&entry);
        if (ret < 0) {
            return -AFW_CIR_FLASH_EREAD_FAIL;
        }

        /* If block full skip */
        if (entry.status != BITS16_DEFAULT_VALUE) {
            flash->num_free_entries -=
                (FLASH_BLOCK_SIZE / sizeof(flash_table_entry_t));
        } else {
            /* The newest write entry offset should be in this block */

            while (start <= end) {
                mid = start + (end - start) / 2;
                mid -= mid % sizeof(flash_table_entry_t);

                ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, mid,
                                     sizeof(flash_table_entry_t),
                                     (uint8_t *)&entry);
                if (ret < 0) {
                    return -AFW_CIR_FLASH_EREAD_FAIL;
                }

                /* store first entry that is free */
                if (entry.status == BITS16_DEFAULT_VALUE) {
                    flash->next_entry_write_offset = mid;
                    if (end == 0) {
                        break;
                    }
                    end = mid - sizeof(flash_table_entry_t);
                } else {
                    start = mid + sizeof(flash_table_entry_t);
                }
            }

            if (flash->next_entry_write_offset == BITS32_DEFAULT_VALUE) {
                return -AFW_CIR_FLASH_ECORRUPT;
            }

            flash->num_free_entries -=
                ((flash->next_entry_write_offset - offset)
                 / sizeof(flash_table_entry_t));

            break;
        }

        offset = get_next_flash_table_block_offset(offset);
    } while (offset != write_block);

    /* Read previous offset to find data size */
    offset = get_previous_entry_offset(flash->next_entry_write_offset);

    ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, offset, sizeof(flash_table_entry_t),
                         (uint8_t *)&entry);
    if (ret < 0) {
        return -AFW_CIR_FLASH_EREAD_FAIL;
    }

    /* Flipping the bits to check the set bits */
    uint16_t status = ~(entry.status);

    /* Mark entry as abandoned if it is in incomplete state */
    if (((status & STATUS_ABANDONED) != STATUS_ABANDONED)
        && ((status & STATUS_WRITE_COMPLETE) != STATUS_WRITE_COMPLETE)) {
        ret = find_incomplete_data_length(flash, entry.offset, &(entry.length));
        if (ret != AFW_OK) {
            if (ret == -AFW_CIR_FLASH_EREAD_FAIL) {
                return ret;
            } else {
                return -AFW_CIR_FLASH_ECORRUPT;
            }
        }

        /* Write length */
        ret = fm_flash_write(flash->part, FM_BYPASS_CLIENT_ID, offset, sizeof(flash_table_entry_t),
                              (const uint8_t *)&entry);
        if (ret < 0) {
            return -AFW_CIR_FLASH_EWRITE_FAIL;
        }

        ret = update_entry_status(flash, offset, STATUS_ABANDONED, &entry);
        if (ret != AFW_OK) {
            return ret;
        }
    }

    flash->next_data_write_offset = entry.offset + entry.length;
    if (flash->next_data_write_offset >= flash->size) {
        flash->next_data_write_offset -= flash->size - AFW_CIR_FLASH_TABLE_SIZE;
    }
    flash->next_entry_id = entry.id + 1;

    return AFW_OK;
}

static int32_t get_read_offset(afw_cir_flash_t *flash)
{
    int32_t ret;
    flash_table_entry_t entry;
    uint32_t start;
    uint32_t end;
    uint32_t mid;
    uint32_t offset;
    uint16_t status;
    uint32_t read_block;

    /* Read block is before Write offset */
    read_block = get_block_aligned_offset(flash->next_entry_write_offset);
    if (read_block == flash->next_entry_write_offset) {
        /* no data has been written to this block, so read block is previous
           block */
        if (read_block) {
            read_block -= FLASH_BLOCK_SIZE;
        } else {
            read_block = AFW_CIR_FLASH_TABLE_SIZE - FLASH_BLOCK_SIZE;
        }
    }

    // read_block is now the block in which the last write happened

    offset = read_block;
    flash->next_data_read_offset = BITS32_DEFAULT_VALUE;
    flash->next_entry_read_offset = BITS32_DEFAULT_VALUE;
    /* loop through blocks in reverse to find the newest read complete */
    do {
        start = offset;
        end = offset + FLASH_BLOCK_SIZE - sizeof(flash_table_entry_t);

        /*
         * if the first entry in block is empty, then next block contains the read offset
         */
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, start, sizeof(flash_table_entry_t),
                             (uint8_t *)&entry);
        if (ret < 0) {
            return -AFW_CIR_FLASH_EREAD_FAIL;
        }

        // if the first entry in the block is uninitialized, then the whole
        // block is uninitialized, so read pointer should be at the start
        // of the next block
        if (entry.status == BITS16_DEFAULT_VALUE) {
            flash->next_entry_read_offset =
                get_next_flash_table_block_offset(offset);
            break;
        }

        /* do a binary search to find the oldest entry */
        while (start <= end) {
            mid = start + (end - start) / 2;
            mid -= mid % sizeof(flash_table_entry_t);

            ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, mid, sizeof(flash_table_entry_t),
                                 (uint8_t *)&entry);
            if (ret < 0) {
                return -AFW_CIR_FLASH_EREAD_FAIL;
            }

            /* Flipping the bits to check the set bits */
            status = ~(entry.status);

            if ((status & STATUS_READ_COMPLETE) == STATUS_READ_COMPLETE) {
                flash->next_entry_read_offset = mid;
                start = mid + sizeof(flash_table_entry_t);
            } else {
                if (end == 0) {
                    break;
                }
                end = mid - sizeof(flash_table_entry_t);
            }
        }

        // next_entry_read_offset now either points to the last READ_COMPLETE
        // entry in the block or is the DEFAULT value (didn't find any
        // READ_COMPLETE entries)

        // If we found a READ_COMPLETE entry then break because
        // we found the last READ_COMPLETE entry in the table
        if (flash->next_entry_read_offset != BITS32_DEFAULT_VALUE) {
            break;
        }

        offset = get_previous_flash_table_block_offset(offset);
    } while (offset != read_block);

    // If we didn't find a READ_COMPLETE entry in any block and we didn't find an
    // uninitialized block, then we're corrupt.
    if (flash->next_entry_read_offset == BITS32_DEFAULT_VALUE) {
        return -AFW_CIR_FLASH_ECORRUPT;
    }

    ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, flash->next_entry_read_offset,
                         sizeof(flash_table_entry_t), (uint8_t *)&entry);
    if (ret < 0) {
        return -AFW_CIR_FLASH_EREAD_FAIL;
    }

    status = ~(entry.status);

    // If we found a READ_COMPLETE entry (and didn't just infer
    // it from an uninitialized block)
    if ((status & STATUS_READ_COMPLETE) == STATUS_READ_COMPLETE) {
        flash->next_entry_read_offset =
            get_next_entry_offset(flash->next_entry_read_offset);
        flash->next_data_read_offset = entry.offset + entry.length;
        if (flash->next_data_read_offset >= flash->size) {
            flash->next_data_read_offset -= flash->size - AFW_CIR_FLASH_TABLE_SIZE;
        }
    } else { // We inferred it from an uninitialized block
        /* This current entry is the read entry */
        flash->next_data_read_offset = entry.offset;
    }

    return AFW_OK;
}

static int32_t get_erase_offset(afw_cir_flash_t *flash)
{
    int32_t ret;
    flash_table_entry_t entry;
    uint32_t start;
    uint32_t end;
    uint32_t mid;
    uint32_t offset;
    uint16_t status;
    uint32_t erase_block;

    /* Erase block is before Write offset */
    erase_block = get_block_aligned_offset(flash->next_entry_write_offset);
    if (erase_block == flash->next_entry_write_offset) {
        /* no data has been written to this block, so erase block is previous
           block */
        if (erase_block) {
            erase_block -= FLASH_BLOCK_SIZE;
        } else {
            erase_block = AFW_CIR_FLASH_TABLE_SIZE - FLASH_BLOCK_SIZE;
        }
    }

    offset = erase_block;
    flash->next_data_erase_offset = BITS32_DEFAULT_VALUE;
    flash->next_entry_erase_offset = BITS32_DEFAULT_VALUE;
    /* loop through reverse to find the newest erase complete */
    do {
        start = offset;
        end = offset + FLASH_BLOCK_SIZE - sizeof(flash_table_entry_t);

        /*
         * if the first entry is empty, then next block contains the erase offset
         */
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, start, sizeof(flash_table_entry_t),
                             (uint8_t *)&entry);
        if (ret < 0) {
            return -AFW_CIR_FLASH_EREAD_FAIL;
        }

        if (entry.status == BITS16_DEFAULT_VALUE) {
            flash->next_entry_erase_offset =
                get_next_flash_table_block_offset(offset);

            break;
        }

        /* do a binary search to find the oldest entry */
        while (start <= end) {
            mid = start + (end - start) / 2;
            mid -= mid % sizeof(flash_table_entry_t);

            ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, mid, sizeof(flash_table_entry_t),
                                 (uint8_t *)&entry);
            if (ret < 0) {
                return -AFW_CIR_FLASH_EREAD_FAIL;
            }

            /* Flipping the bits to check the set bits */
            status = ~(entry.status);

            if ((status & STATUS_ERASE_COMPLETE) == STATUS_ERASE_COMPLETE) {
                flash->next_entry_erase_offset = mid + sizeof(flash_table_entry_t);
                start = mid + sizeof(flash_table_entry_t);
            } else {
                if (end == 0) {
                    break;
                }
                end = mid - sizeof(flash_table_entry_t);
            }
        }

        if (flash->next_entry_erase_offset != BITS32_DEFAULT_VALUE
            && flash->next_entry_erase_offset
            != get_block_aligned_offset(flash->next_entry_erase_offset)) {
            break;
        }

        offset = get_previous_flash_table_block_offset(offset);
    } while (offset != erase_block);

    ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, flash->next_entry_erase_offset,
                         sizeof(flash_table_entry_t), (uint8_t *)&entry);
    if (ret < 0) {
        return -AFW_CIR_FLASH_EREAD_FAIL;
    }

    status = ~(entry.status);

    /* The next block is erase entry */
    if ((status & STATUS_ERASE_COMPLETE) == STATUS_ERASE_COMPLETE) {
        flash->next_entry_erase_offset =
            get_next_entry_offset(flash->next_entry_erase_offset);
        flash->next_data_erase_offset =
            get_block_aligned_offset(entry.offset + entry.length);
    } else {
        /* Start erasing from this entry */
        flash->next_data_erase_offset = get_block_aligned_offset(entry.offset);
    }

    return AFW_OK;
}

int32_t get_next_read_entry(afw_cir_flash_t *flash, flash_table_entry_t *entry)
{
    int32_t ret;
    uint16_t status;

    /* Find the read offset */
    do {
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, flash->next_entry_read_offset,
                             sizeof(flash_table_entry_t),
                             (uint8_t *)entry);
        if (ret < 0) {
            return -AFW_CIR_FLASH_EREAD_FAIL;
        }

        status = ~(entry->status);
        if ((status & STATUS_ABANDONED) == STATUS_ABANDONED) {
            /* Mark read complete and skip */
            ret = update_entry_status(flash, flash->next_entry_read_offset,
                                      STATUS_READ_COMPLETE, entry);
            if (ret != AFW_OK) {
                return ret;
            }

            flash->next_entry_read_offset =
                get_next_entry_offset(flash->next_entry_read_offset);

        } else if (((status & STATUS_READ_START) == STATUS_READ_START)
                   || ((status & STATUS_WRITE_COMPLETE)
                       == STATUS_WRITE_COMPLETE)) {
            break;
        } else {
            return -AFW_CIR_FLASH_ENO_BYTES;
        }

    } while (1);

    return AFW_OK;
}

int32_t cir_flash_read(afw_cir_flash_t *flash, uint32_t length,
                       uint8_t *buf, bool peek)
{
    flash_table_entry_t entry;
    int32_t ret;
    uint16_t status;

    ret = get_next_read_entry(flash, &entry);
    if (ret != AFW_OK) {
        return ret;
    }

    status = ~(entry.status);

    if ((status & STATUS_READ_START) != STATUS_READ_START) {
        flash->data_read_length = 0;
        ret = update_entry_status(flash, flash->next_entry_read_offset,
                                  STATUS_READ_START, &entry);
        if (ret != AFW_OK) {
            return -AFW_CIR_FLASH_EREAD_FAIL;
        }
    }

    if (length > (entry.length - flash->data_read_length)) {
        return -AFW_EINVAL;
    }

    uint32_t rem;
    uint32_t offset = 0;
    uint32_t data_read_offset = flash->next_data_read_offset;
    do {
        rem = length;
        if (rem > (flash->size - data_read_offset)) {
            rem = flash->size - data_read_offset;
        }
        length -= rem;

        /* read data from flash */
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, data_read_offset,
                             rem, buf + offset);
        if (ret < 0) {
            return -AFW_CIR_FLASH_EREAD_FAIL;
        }

        offset += rem;
        data_read_offset += rem;

        if (data_read_offset == flash->size) {
            data_read_offset = AFW_CIR_FLASH_TABLE_SIZE;
        }

    } while (length);

    if (!peek) {
        flash->data_read_length += offset;
        flash->next_data_read_offset = data_read_offset;
    }

    return AFW_OK;
}

int32_t afw_cir_flash_init(char *name, afw_cir_flash_t *flash)
{
    int32_t ret;
    bool empty;

    if (NULL == flash || NULL == name) {
        return -AFW_EINVAL;
    }

    if (AFW_CIR_FLASH_TABLE_SIZE < DEFAULT_TABLE_SIZE) {
        return -AFW_CIR_FLASH_EINVALID_TABLE_SIZE;
    }

    memset(flash, 0, sizeof(afw_cir_flash_t));

    flash->part = fm_flash_get_partition(name);
    if (NULL == flash->part) {
        return -AFW_CIR_FLASH_ECANT_FIND;
    }

    flash->size = flash->part->size;
    if (0 == flash->size) {
        return -AFW_CIR_FLASH_EACCESS_FAIL;
    }

    ret = is_flash_empty(flash, &empty);
    if (ret != AFW_OK) {
        return ret;
    }

    if (empty) {
        flash->next_data_write_offset = AFW_CIR_FLASH_TABLE_SIZE;
        flash->next_data_read_offset = AFW_CIR_FLASH_TABLE_SIZE;
        flash->next_data_erase_offset = AFW_CIR_FLASH_TABLE_SIZE;
        flash->num_free_entries =
            (AFW_CIR_FLASH_TABLE_SIZE / sizeof(flash_table_entry_t));
    } else {
        ret = get_write_offset(flash);
        if (ret != AFW_OK) {
            return ret;
        }

        ret = get_read_offset(flash);
        if (ret != AFW_OK) {
            return ret;
        }

        ret = get_erase_offset(flash);
        if (ret != AFW_OK) {
            return ret;
        }
    }

    flash->mutex = xSemaphoreCreateMutex();
    if (flash->mutex == NULL) {
        return -AFW_ENOMEM;
    }

    flash->erase_async_event = xEventGroupCreate();
    if (flash->erase_async_event == NULL) {
        vSemaphoreDelete(flash->mutex);
        return -AFW_ENOMEM;
    }

    return AFW_OK;
}

int32_t afw_cir_flash_deinit(afw_cir_flash_t *flash)
{
    if (NULL == flash) {
        return -AFW_EINVAL;
    }

    vSemaphoreDelete(flash->mutex);
    vEventGroupDelete(flash->erase_async_event);
    memset(flash, 0, sizeof(afw_cir_flash_t));

    return AFW_OK;
}

int32_t afw_cir_flash_write(afw_cir_flash_t *flash, uint32_t length,
                            const uint8_t *buf)
{
    int32_t ret = -AFW_CIR_FLASH_EACCESS_TIMEOUT;
    flash_table_entry_t entry;
    uint16_t status;

    if (NULL == flash || NULL == buf || 0 == length) {
        return -AFW_EINVAL;
    }

    if (pdTRUE == xSemaphoreTake(flash->mutex, SEMAPHORE_TIMEOUT)) {
        /*
         * Cant fit data in the flash. Atleast 4K memory should be free.
         *
         * Or atleast 1 entry should be always be free.
         */
        if ((length > get_free_size(flash))
            || (flash->num_free_entries == MIN_FREE_ENTRIES)) {
            return return_unblock(flash, -AFW_CIR_FLASH_ENO_FREE_SPACE);
        }

        /* Get the next entry from table */
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, flash->next_entry_write_offset,
                             sizeof(flash_table_entry_t), (uint8_t *)&entry);
        if (ret < 0) {
            return return_unblock(flash, -AFW_CIR_FLASH_EREAD_FAIL);
        }

        /* Flipping the bits to check the set bits */
        status = ~(entry.status);

        /* Is it unused? */
        if (entry.status == BITS16_DEFAULT_VALUE) {
            /* Create an entry in the table */
            entry.id = flash->next_entry_id;
            entry.offset = flash->next_data_write_offset;
            entry.length = BITS32_DEFAULT_VALUE;
            entry.status = BITS16_DEFAULT_VALUE;
            entry.reserved = 0;

            ret = fm_flash_write(flash->part, FM_BYPASS_CLIENT_ID, flash->next_entry_write_offset,
                                  sizeof(flash_table_entry_t),
                                  (const uint8_t *)&entry);
            if (ret < 0) {
                return return_unblock(flash, -AFW_CIR_FLASH_EWRITE_FAIL);
            }

            ret = update_entry_status(flash, flash->next_entry_write_offset,
                                      STATUS_WRITE_START, &entry);
            if (ret != AFW_OK) {
                return return_unblock(flash, ret);
            }

            (flash->next_entry_id)++;
            (flash->num_free_entries)--;
            flash->data_write_length = 0;
        } else if ((status & STATUS_WRITE_COMPLETE) == STATUS_WRITE_COMPLETE) {
            return return_unblock(flash, -AFW_CIR_FLASH_ENO_FREE_SPACE);
        }

        uint32_t rem;
        uint32_t offset = 0;
        do {
            rem = length;
            /* Split data write if the flash address space needs wrapping */
            if (rem > (flash->size - flash->next_data_write_offset)) {
                rem = flash->size - flash->next_data_write_offset;
            }
            length -= rem;

            /* write data to flash */
            ret = fm_flash_write(flash->part, FM_BYPASS_CLIENT_ID, flash->next_data_write_offset,
                                  rem, buf + offset);
            if (ret < 0) {
                return return_unblock(flash, -AFW_CIR_FLASH_EWRITE_FAIL);
            }

            offset += rem;
            flash->next_data_write_offset += rem;
            flash->data_write_length += rem;

            if (flash->next_data_write_offset == flash->size) {
                flash->next_data_write_offset = AFW_CIR_FLASH_TABLE_SIZE;
            }

        } while (length);

        ret = 0;
        xSemaphoreGive(flash->mutex);
    }

    return ret;
}

int32_t afw_cir_flash_write_complete(afw_cir_flash_t *flash)
{
    flash_table_entry_t entry;
    int32_t ret = -AFW_CIR_FLASH_EACCESS_TIMEOUT;

    if (NULL == flash) {
        return -AFW_EINVAL;
    }

    if (pdTRUE == xSemaphoreTake(flash->mutex, SEMAPHORE_TIMEOUT)) {
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, flash->next_entry_write_offset,
                             sizeof(flash_table_entry_t), (uint8_t *)&entry);
        if (ret < 0) {
            return return_unblock(flash, -AFW_CIR_FLASH_EREAD_FAIL);
        }

        uint16_t status = ~(entry.status);
        if (status != STATUS_WRITE_START) {
            return return_unblock(flash, -AFW_CIR_FLASH_EWRONG_STATE);
        }

        /* Update status to complete and length of the data */
        entry.length = flash->data_write_length;

        ret = fm_flash_write(flash->part, FM_BYPASS_CLIENT_ID, flash->next_entry_write_offset,
                              sizeof(flash_table_entry_t),
                              (const uint8_t *)&entry);
        if (ret < 0) {
            return return_unblock(flash, -AFW_CIR_FLASH_EWRITE_FAIL);
        }

        ret = update_entry_status(flash, flash->next_entry_write_offset,
                                  STATUS_WRITE_COMPLETE, &entry);
        if (ret != AFW_OK) {
            return return_unblock(flash, ret);
        }

        flash->data_write_length = 0;
        flash->next_entry_write_offset =
            get_next_entry_offset(flash->next_entry_write_offset);

        ret = 0;
        xSemaphoreGive(flash->mutex);
    }

    return ret;
}

int32_t afw_cir_flash_read(afw_cir_flash_t *flash, uint32_t length,
                           uint8_t *buf)
{
    int32_t ret = -AFW_CIR_FLASH_EACCESS_TIMEOUT;

    if (NULL == flash || NULL == buf || 0 == length) {
        return -AFW_EINVAL;
    }

    if (pdTRUE == xSemaphoreTake(flash->mutex, SEMAPHORE_TIMEOUT)) {
        ret = cir_flash_read(flash, length, buf, false);
        xSemaphoreGive(flash->mutex);
    }

    return ret;
}

int32_t afw_cir_flash_peek(afw_cir_flash_t *flash, uint32_t length,
                           uint8_t *buf)
{
    int32_t ret = -AFW_CIR_FLASH_EACCESS_TIMEOUT;

    if (NULL == flash || NULL == buf || 0 == length) {
        return -AFW_EINVAL;
    }

    if (pdTRUE == xSemaphoreTake(flash->mutex, SEMAPHORE_TIMEOUT)) {
        ret = cir_flash_read(flash, length, buf, true);
        xSemaphoreGive(flash->mutex);
    }

    return ret;
}

int32_t afw_cir_flash_read_complete(afw_cir_flash_t *flash)
{
    flash_table_entry_t entry;
    int32_t ret = -AFW_CIR_FLASH_EACCESS_TIMEOUT;

    if (NULL == flash) {
        return -AFW_EINVAL;
    }

    if (pdTRUE == xSemaphoreTake(flash->mutex, SEMAPHORE_TIMEOUT)) {
        ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, flash->next_entry_read_offset,
                             sizeof(flash_table_entry_t), (uint8_t *)&entry);
        if (ret < 0) {
            return return_unblock(flash, -AFW_CIR_FLASH_EREAD_FAIL);
        }

        uint16_t status = ~(entry.status);
        if ((status & STATUS_READ_START) != STATUS_READ_START) {
            return return_unblock(flash, -AFW_CIR_FLASH_EWRONG_STATE);
        }

        ret = update_entry_status(flash, flash->next_entry_read_offset,
                                  STATUS_READ_COMPLETE, &entry);
        if (ret != AFW_OK) {
            return return_unblock(flash, ret);
        }

        flash->data_read_length = 0;
        flash->next_entry_read_offset =
            get_next_entry_offset(flash->next_entry_read_offset);

        ret = 0;
        xSemaphoreGive(flash->mutex);
    }

    return ret;
}

int32_t afw_cir_flash_erase(afw_cir_flash_t *flash)
{
    int32_t ret = -AFW_CIR_FLASH_EACCESS_TIMEOUT;
    int32_t size;
    flash_table_entry_t entry;

    if (NULL == flash) {
        return -AFW_EINVAL;
    }

    if (pdTRUE == xSemaphoreTake(flash->mutex, SEMAPHORE_TIMEOUT)) {
        if (flash->next_data_read_offset >= flash->next_data_erase_offset) {
            size = flash->next_data_read_offset - flash->next_data_erase_offset;
        } else {
            size = flash->size - flash->next_data_erase_offset;
            size += flash->next_data_read_offset - AFW_CIR_FLASH_TABLE_SIZE;
        }

        if (size >= FLASH_BLOCK_SIZE) {
            ret = fm_flash_read(flash->part, FM_BYPASS_CLIENT_ID, flash->next_entry_erase_offset,
                                 sizeof(flash_table_entry_t),
                                 (uint8_t *)&entry);
            if (ret < 0) {
                return return_unblock(flash, -AFW_CIR_FLASH_EREAD_FAIL);
            }

            ret = update_entry_status(flash, flash->next_entry_erase_offset,
                                      STATUS_ERASE_START, &entry);
            if (ret != AFW_OK) {
                return return_unblock(flash, ret);
            }

            ret = erase_block(flash, flash->next_data_erase_offset,
                              FM_FLASH_ERASE_SIZE_4K);
            if (ret < 0) {
                return return_unblock(flash, ret);
            }

            flash->next_data_erase_offset =
                get_next_data_block_offset(flash,
                                           flash->next_data_erase_offset);
            ret = erase_flash_table(flash);
        }

        ret = AFW_OK;
        xSemaphoreGive(flash->mutex);
    }

    return ret;
}

int32_t afw_cir_flash_erase_partition(afw_cir_flash_t *flash)
{
    int32_t ret = -AFW_CIR_FLASH_EACCESS_TIMEOUT;
    uint32_t size;
    uint32_t rem;
    uint32_t offset;

    if (NULL == flash) {
        return -AFW_EINVAL;
    }

    size = flash->size;
    if ((size % FM_FLASH_ERASE_SIZE_4K) != 0) {
        return -AFW_CIR_FLASH_EERASE_FAIL;
    }

    if (pdTRUE == xSemaphoreTake(flash->mutex, SEMAPHORE_TIMEOUT)) {
        offset = 0;
        do {
            rem = size;
            if (rem >= FM_FLASH_ERASE_SIZE_64K) {
                rem = FM_FLASH_ERASE_SIZE_64K;
            } else if (rem >= FM_FLASH_ERASE_SIZE_32K) {
                rem = FM_FLASH_ERASE_SIZE_32K;
            } else {
                rem = FM_FLASH_ERASE_SIZE_4K;
            }

            ret = erase_block(flash, offset, rem);
            if (ret < 0) {
                return return_unblock(flash, ret);
            }

            size -= rem;
            offset += rem;

        } while (size);

        ret = 0;
        xSemaphoreGive(flash->mutex);
    }

    return ret;
}

int32_t afw_cir_flash_get_read_data_length(afw_cir_flash_t *flash,
        uint32_t *length)
{
    int32_t ret = -AFW_CIR_FLASH_EACCESS_TIMEOUT;
    flash_table_entry_t entry;

    if (NULL == flash || NULL == length) {
        return -AFW_EINVAL;
    }

    if (pdTRUE == xSemaphoreTake(flash->mutex, SEMAPHORE_TIMEOUT)) {
        ret = get_next_read_entry(flash, &entry);
        if (ret == AFW_OK) {
            *length = entry.length;
        }
        xSemaphoreGive(flash->mutex);
    }

    return ret;

}

static uint32_t get_avail_erase_size(afw_cir_flash_t *flash)
{
    uint32_t size = 0;

    /* avail erase size: read pointer - erase pointer */
    if (flash->next_data_read_offset >= flash->next_data_erase_offset) {
        size = flash->next_data_read_offset - flash->next_data_erase_offset;
    } else {
        size = flash->size - flash->next_data_erase_offset;
        size += (flash->next_data_read_offset - AFW_CIR_FLASH_TABLE_SIZE);
    }

    return size;
}

static uint32_t get_avail_erase_entries(afw_cir_flash_t *flash)
{
    uint32_t num = 0;

    if (flash->next_entry_read_offset >= flash->next_entry_erase_offset) {
        num = (flash->next_entry_read_offset - flash->next_entry_erase_offset) / sizeof(flash_table_entry_t);
    } else {
        num = (AFW_CIR_FLASH_TABLE_SIZE - flash->next_entry_erase_offset + flash->next_entry_read_offset) / sizeof(flash_table_entry_t);
    }

    return num;
}

static uint32_t get_avail_unread_size(afw_cir_flash_t *flash)
{
    uint32_t size = 0;

    /* avail unread size: write pointer - reade pointer */
    if (flash->next_data_write_offset >= flash->next_data_read_offset) {
        size = flash->next_data_write_offset - flash->next_data_read_offset;
    } else {
        size = flash->size - flash->next_data_read_offset;
        size += (flash->next_data_write_offset - AFW_CIR_FLASH_TABLE_SIZE);
    }

    return size;
}

static uint32_t get_avail_unread_entries(afw_cir_flash_t *flash)
{
    uint32_t num = 0;

    if (flash->next_entry_write_offset >= flash->next_entry_read_offset) {
        num = (flash->next_entry_write_offset - flash->next_entry_read_offset) / sizeof(flash_table_entry_t);
    } else {
        num = (AFW_CIR_FLASH_TABLE_SIZE - flash->next_entry_read_offset + flash->next_entry_write_offset) / sizeof(flash_table_entry_t);
    }

    return num;
}

int32_t afw_cir_flash_get_info(afw_cir_flash_t *flash,
                               afw_cir_flash_info_t *info)
{
    int32_t ret = -AFW_CIR_FLASH_EACCESS_TIMEOUT;

    if (NULL == flash || NULL == info) {
        return -AFW_EINVAL;
    }

    if (pdTRUE == xSemaphoreTake(flash->mutex, SEMAPHORE_TIMEOUT)) {

        info->size = flash->size - AFW_CIR_FLASH_TABLE_SIZE - FLASH_BLOCK_SIZE;
        info->entries = AFW_CIR_FLASH_TABLE_SIZE / sizeof(flash_table_entry_t) - MIN_FREE_ENTRIES;

        info->avail_free_size = get_free_size(flash);
        info->avail_free_entries = flash->num_free_entries - MIN_FREE_ENTRIES;

        info->avail_erase_size = get_avail_erase_size(flash);
        info->avail_erase_entries = get_avail_erase_entries(flash);

        info->avail_unread_size = get_avail_unread_size(flash);
        info->avail_unread_entries = get_avail_unread_entries(flash);

        ret = 0;
        xSemaphoreGive(flash->mutex);
    }

    return ret;
}
