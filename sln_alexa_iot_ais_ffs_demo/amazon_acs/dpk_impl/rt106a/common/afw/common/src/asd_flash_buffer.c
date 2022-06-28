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
* The header for flash buffer.
*
*@File: asd_flash_buffer.c
********************************************************************************
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "asd_flash_buffer.h"
#include "crc8.h"
#include "assert.h"

// Uncomment the following line to enable flashbuffer debugging print.
//#define FBUFFER_DPRINTF(fmt, ...)  printf("%s:%u: " fmt "\r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FBUFFER_DPRINTF(fmt, ...)

#ifndef MIN
#define MIN(x, y)  (((x) < (y))? (x) : (y))
#endif

#define PAGE_SIZE                (256)

#define FRAME_HEADER_MAGIC_NUM   (0xF5)
#define ACTUAL_USER_DATA_SIZE(flash_buf_size) (flash_buf_size/FLASH_SECTOR_SIZE*DATA_SIZE_PER_SECTOR)


//TODO: head/tail wrap the 32bits. FWPLATFORM-533


static int32_t asd_flash_buffer_init_from_flash(asd_flash_buffer_t *flashbuf);
static int32_t asd_flash_buffer_reset(asd_flash_buffer_t *flashbuf);
static int32_t set_sector_meta_frame_index(asd_flash_buffer_t *flashbuf, uint32_t frame_index, uint16_t offset);
static int32_t write_cur_sector_meta_page_bitmap(asd_flash_buffer_t *flashbuf, uint16_t page_bitmap);
static int32_t update_cur_sector_meta_page_bitmap(asd_flash_buffer_t *flashbuf);
static bool asd_flash_buffer_pos_in_range(const asd_flash_buffer_t *flashbuf, uint32_t pos);
static int32_t asd_flash_buffer_read_data(const asd_flash_buffer_t *flashbuf, uint32_t pos, void* buf, uint32_t size);
static int32_t asd_flash_buffer_erase_next_sector(asd_flash_buffer_t *flashbuf);
static int _flash_read(const asd_flash_buffer_t* flashbuf, uint32_t offset, void* buffer, uint32_t size);
static int _flash_write(const asd_flash_buffer_t* flashbuf, uint32_t offset, const void* buffer, uint32_t size);
static int _flash_erase(const asd_flash_buffer_t* flashbuf, uint32_t offset, uint32_t size);
static int32_t asd_flash_buffer_reset(asd_flash_buffer_t *flashbuf);
static int32_t mark_read_flag(asd_flash_buffer_t *flashbuf, asd_frame_info_t *frame_info);
static uint32_t get_erased_data_bytes(const asd_flash_buffer_t *flashbuf);
static int32_t forward_tail(asd_flash_buffer_t *flashbuf, uint32_t bytes);
static int32_t forward_head(asd_flash_buffer_t *flashbuf, uint32_t bytes);
static int32_t asd_flash_buffer_write_data(asd_flash_buffer_t *flashbuf,
                                          const uint8_t *buf, uint32_t size);
static int32_t asd_flash_buffer_random_write_data(
                     asd_flash_buffer_t *flashbuf, uint32_t pos,
                     const uint8_t *buf, uint32_t size);
static uint32_t move_pos(uint32_t pos, int32_t bytes);
static bool is_frame_start_pos(const asd_flash_buffer_t *flashbuf, uint32_t pos, bool check_crc,
                      const uint8_t* data, uint32_t size);
static bool is_valid_frame_head(const frame_header_t* header);
static int32_t asd_flash_buffer_read_frame_length(const asd_flash_buffer_t *flashbuf,
                                    uint32_t frame_start_pos, uint16_t *length);
static void update_with_head_frame_complete(asd_flash_buffer_t* flashbuf);
static int32_t update_write_frame_header_in_flash(asd_flash_buffer_t* flashbuf);


// ======================== local inline functions =============================
static inline uint32_t convert_pos_to_offset(uint32_t bufsize, uint32_t pos)
{
    return pos%bufsize;
}

static inline uint32_t get_sector_start_addr(uint32_t pos)
{
    return pos&(~((uint32_t)FLASH_SECTOR_SIZE-1));
}

static inline uint32_t get_sector_start_offset(uint32_t bufsize, uint32_t pos)
{
    return get_sector_start_addr(convert_pos_to_offset(bufsize, pos));

}

static inline uint32_t get_meta_offset_from_offset(uint32_t offset)
{
    return get_sector_start_addr(offset) + DATA_SIZE_PER_SECTOR;
}

static inline uint32_t get_meta_offset(uint32_t bufsize, uint32_t pos)
{
    uint32_t offset = convert_pos_to_offset(bufsize, pos);
    return get_meta_offset_from_offset(offset);
}

static inline uint32_t get_sector_index(uint32_t bufsize, uint32_t pos)
{
    return convert_pos_to_offset(bufsize, pos)/FLASH_SECTOR_SIZE;
}

static inline uint32_t get_offset_in_sector(uint32_t pos)
{
    return pos%FLASH_SECTOR_SIZE;
}

static inline uint32_t get_next_sector_start_offset(uint32_t bufsize, uint32_t pos)
{
    return convert_pos_to_offset(bufsize, get_sector_start_addr(pos) + FLASH_SECTOR_SIZE);
}

static inline uint32_t get_previous_sector_start_offset(uint32_t bufsize, uint32_t offset)
{
    return (get_sector_start_addr(offset) + bufsize - FLASH_SECTOR_SIZE)%bufsize;
}

static inline uint32_t get_next_sector_index(uint32_t bufsize, uint32_t cur_sector_index)
{
    return (cur_sector_index + 1)%(bufsize/FLASH_SECTOR_SIZE);
}

// get data length from position, excluding sector meta data.
static inline uint32_t get_data_length_from(const asd_flash_buffer_t *flashbuf, uint32_t pos)
{
    assert(flashbuf);
    uint32_t num_sector_meta = flashbuf->head.start_pos/FLASH_SECTOR_SIZE - pos/FLASH_SECTOR_SIZE;
    return flashbuf->head.start_pos - pos - SECTOR_META_SIZE * num_sector_meta;
}

static inline uint32_t get_frame_num_from(const asd_flash_buffer_t *flashbuf, uint32_t frame_index)
{
    assert(flashbuf);

    return flashbuf->head.index - frame_index;
}

static inline bool buffer_is_empty(const asd_flash_buffer_t* flashbuf)
{
    assert(flashbuf);
    return flashbuf->head.start_pos == flashbuf->tail.start_pos;
}

static inline uint32_t get_write_pos(const frame_state_t* frame_state)
{
    return move_pos(frame_state->start_pos, frame_state->w_length);
}

static inline bool is_valid_sector_meta(const flash_sector_meta_t* meta)
{
    return meta->frame_offset < FRAME_PAYLOAD_SIZE_MAX;
}

// ================================ API ========================================

int32_t asd_flash_buffer_init(asd_flash_buffer_t *pflashbuf ,
                struct fm_flash_partition* flash_partition,
                uint32_t offset_in_partition,
                uint32_t size,
                bool load_from_flash)
{
    if (!pflashbuf
        || offset_in_partition%FLASH_SECTOR_SIZE != 0 /*flash start offset must be sector aligned.*/
        || size%FLASH_SECTOR_SIZE != 0   /* size must be multiple of sectors */
        || size < SUPPORTED_FLASH_BUFFER_SIZE_MIN /*buffer size must at least 2  sectors */
        || !flash_partition)
         return ASD_FLASH_BUFFER_EBAD_PARAMS;

    memset(pflashbuf, 0, sizeof(asd_flash_buffer_t));

    pflashbuf->size = size;
    pflashbuf->start_offset = offset_in_partition;
    pflashbuf->flash_partition = flash_partition;

    int32_t rc = 0;
    // load the region from flash.

    if (load_from_flash) {
        rc = asd_flash_buffer_init_from_flash(pflashbuf);
    } else {
        rc = asd_flash_buffer_reset(pflashbuf);
    }
    if (rc < 0) {
        //cleanup
        asd_flash_buffer_deinit(pflashbuf);
    }
    return rc;
}

int32_t asd_flash_buffer_deinit(asd_flash_buffer_t *pflashbuf)
{
    return ASD_FLASH_BUFFER_OK;
}

int32_t asd_flash_buffer_get_free_size(const asd_flash_buffer_t *flashbuf)
{
    if (!flashbuf || !flashbuf->size) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }
    uint32_t freesize = flashbuf->size/FLASH_SECTOR_SIZE*DATA_SIZE_PER_SECTOR
                   - get_data_length_from(flashbuf, flashbuf->tail.start_pos)
                   - get_offset_in_sector(flashbuf->tail.start_pos);
    return (int32_t) ((freesize > FRAME_HEADER_SIZE)? (freesize - FRAME_HEADER_SIZE) : 0);

}

int32_t asd_flash_buffer_get_data_length_and_frame_num(const asd_flash_buffer_t *flashbuf,
                         uint32_t *data_length, uint32_t *frame_num)
{
    if (!flashbuf || !flashbuf->size) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }
    uint32_t frames = get_frame_num_from(flashbuf, flashbuf->tail.index);
    if (data_length) {
        //data length doesn't include frame header size;
        *data_length = get_data_length_from(flashbuf, flashbuf->tail.start_pos);
        *data_length = (*data_length > FRAME_HEADER_SIZE * frames)?
                          (*data_length - FRAME_HEADER_SIZE * frames) : 0;
    }
    if (frame_num) {
        *frame_num = frames;
    }
    return ASD_FLASH_BUFFER_OK;
}

int32_t asd_flash_buffer_erase_all(asd_flash_buffer_t *flashbuf)
{
    if (!flashbuf || !flashbuf->size) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }
    _flash_erase(flashbuf, 0, flashbuf->size);
    return asd_flash_buffer_reset(flashbuf);
}

int32_t asd_flash_buffer_erase_before(asd_flash_buffer_t *flashbuf, uint32_t pos)
{
    if (!flashbuf || !flashbuf->size) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }

    if (!asd_flash_buffer_pos_in_range(flashbuf, pos)) {
        return ASD_FLASH_BUFFER_EBAD_POSITION;
    }
    int rc = ASD_FLASH_BUFFER_OK;

    while (get_sector_index(flashbuf->size, flashbuf->tail.start_pos) !=
           get_sector_index(flashbuf->size, pos) ) {
        rc = asd_flash_buffer_erase_next_sector(flashbuf);
        if (rc < 0) break;
    }
    return rc;
}

int32_t asd_flash_buffer_get_data_length_and_frame_num_from_frame(
                        const asd_flash_buffer_t *flashbuf, asd_frame_info_t* frame_info,
                        uint32_t *data_length, uint32_t *frame_num)
{
    if (!flashbuf || !flashbuf->size) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }

    if (!asd_flash_buffer_pos_in_range(flashbuf, frame_info->start_pos)) {
        return ASD_FLASH_BUFFER_EBAD_POSITION;
    }
    uint32_t frames = get_frame_num_from(flashbuf, frame_info->index);
    if (data_length) {
        //data length doesn't include frame header size;
        *data_length = get_data_length_from(flashbuf, frame_info->start_pos);
        *data_length = (*data_length > FRAME_HEADER_SIZE * frames)?
                          (*data_length - FRAME_HEADER_SIZE * frames) : 0;
    }
    if (frame_num) {
        *frame_num = frames;
    }
    return ASD_FLASH_BUFFER_OK;

}

int32_t asd_flash_buffer_write_payload(asd_flash_buffer_t *flashbuf, const uint8_t *buf, uint32_t size, bool complete)
{
    if (!flashbuf) return ASD_FLASH_BUFFER_EBAD_PARAMS;
    int rc = 0;

    uint32_t write_size = size;
    bool frame_header_completed = false;
    FBUFFER_DPRINTF("write payload %lu bytes", size);

    // allow size == 0 only there is data in current frame payload.
    if (flashbuf->head.w_length == 0 &&
        (!buf || size == 0)) {
        //no data at the beginning of a frame. return failure.
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }
    if (flashbuf->head.w_length + size > FRAME_PAYLOAD_SIZE_MAX) {
        //check payload size limit
        return ASD_FLASH_BUFFER_EFRAME_LIMIT;
    }

    if (flashbuf->head.w_length == 0) {
        //start of a new frame, and write frame header first.
        write_size += FRAME_HEADER_SIZE;
    }

    if (write_size >= get_erased_data_bytes(flashbuf)) {
        //TODO: soft_write: erase only data is read, otherwise return failure.
        //erase next
        if ((rc = asd_flash_buffer_erase_next_sector(flashbuf)) != ASD_FLASH_BUFFER_OK) {
            return rc;
        }
    }

    // User data is packed in frame, e.g. frame header + payload.
    // head.w_length == 0, that's a new frame. So frame header is add in front.
    // After frame header, write the payload, e.g. user data.
    // if header.w_length > 0, no frame header is required, and user data is append
    // directly in payload.

    uint32_t old_sector_start_pos = get_sector_start_addr(get_write_pos(&flashbuf->head));
    //write frame data to flash.
    if (flashbuf->head.w_length == 0) {
        flashbuf->head.crc = 0;

        //process frame header
        frame_header_t header;
        memset(&header, 0xFF, FRAME_HEADER_SIZE);
        header.magic_num = FRAME_HEADER_MAGIC_NUM;
        if (complete) {
            //only if frame complete, fill crc and length. Ohterwise, keep crc/length all 1.
            header.length = size;
            uint16_t length = header.length;
            flashbuf->head.crc = crc8_update(flashbuf->head.crc, buf, size);
            flashbuf->head.crc = crc8_update(flashbuf->head.crc,
                                         (uint8_t*)&length, sizeof(length));
            header.crc = flashbuf->head.crc;
            frame_header_completed = true;
        }

        //write frame header.
        rc = asd_flash_buffer_write_data(flashbuf, (uint8_t*)&header, sizeof(header));
        if (rc < 0) {
            return rc;
        }
    }

    if (size && buf) {
        //append user data in payload, if there is user data to write.
        rc = asd_flash_buffer_write_data(flashbuf, buf, size);
        if (rc < 0) {
            return rc;
        }
    }

    if (!frame_header_completed) {
        if (size && buf) {
            //accumulate crc.
            flashbuf->head.crc = crc8_update(flashbuf->head.crc, buf, size);
        }
        if (complete) {
            rc = update_write_frame_header_in_flash(flashbuf);

        }
    }

    if (complete) {
        update_with_head_frame_complete(flashbuf);
    }

    //update frame index across sector.
    if (get_sector_start_addr(get_write_pos(&flashbuf->head)) != old_sector_start_pos) {
        set_sector_meta_frame_index(flashbuf,
                flashbuf->head.index,
                get_offset_in_sector(get_write_pos(&flashbuf->head)));
    }

    return (rc < 0)? rc : (int)size;
}

int32_t asd_flash_buffer_get_first_frame_info(const asd_flash_buffer_t *flashbuf,
                                              asd_frame_info_t *first_frame_info)
{
    if (!flashbuf || !flashbuf->size || !first_frame_info) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }

    if (buffer_is_empty(flashbuf)) {
        //empty buffer.
        return ASD_FLASH_BUFFER_ENO_BYTES;
    }
    *first_frame_info = flashbuf->tail;
    return ASD_FLASH_BUFFER_OK;
}

int32_t asd_flash_buffer_get_total_size(const asd_flash_buffer_t *flashbuf)
{
    if (!flashbuf || !flashbuf->size) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }

    return (int32_t) ACTUAL_USER_DATA_SIZE(flashbuf->size);
}

int32_t asd_flash_buffer_get_first_unread_frame_info(const asd_flash_buffer_t *flashbuf,
                                             asd_frame_info_t *first_unread_frame_info)
{
    if (!flashbuf || !flashbuf->size || !first_unread_frame_info) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }

    if (buffer_is_empty(flashbuf) || flashbuf->unread_tail.length == 0) {
        //empty buffer.
        return ASD_FLASH_BUFFER_ENO_BYTES;
    }
    *first_unread_frame_info = flashbuf->unread_tail;
    return ASD_FLASH_BUFFER_OK;
}

int32_t asd_flash_buffer_get_last_frame_info(const asd_flash_buffer_t *flashbuf,
                                              asd_frame_info_t *last_frame_info)
{
    if (!flashbuf || !flashbuf->size || !last_frame_info) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }
    if (buffer_is_empty(flashbuf) || flashbuf->last_frame.length == 0) {
        //empty buffer.
        return ASD_FLASH_BUFFER_ENO_BYTES;
    }
    *last_frame_info = flashbuf->last_frame;
    return ASD_FLASH_BUFFER_OK;
}

int32_t asd_flash_buffer_get_next_frame_info(const asd_flash_buffer_t *flashbuf,
                const asd_frame_info_t *cur_frame_info, asd_frame_info_t *next_frame_info)
{
    if (!flashbuf || !flashbuf->size
        || !next_frame_info || !cur_frame_info
        || !cur_frame_info->length) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }

    assert(cur_frame_info->length <= FRAME_PAYLOAD_SIZE_MAX);

    if (!asd_flash_buffer_pos_in_range(flashbuf, cur_frame_info->start_pos)) {
        FBUFFER_DPRINTF("out of range: %lu not in (%lu, %lu, %lu)\n", cur_frame_info->start_pos,
            flashbuf->tail.start_pos, flashbuf->unread_tail.start_pos,
            flashbuf->head.start_pos);
        return ASD_FLASH_BUFFER_EBAD_POSITION;
    }

    if (cur_frame_info->start_pos == flashbuf->head.start_pos
        || cur_frame_info->start_pos == flashbuf->last_frame.start_pos ) {
        //eof
        return ASD_FLASH_BUFFER_ENO_BYTES;
    }

    uint32_t pos = move_pos(cur_frame_info->start_pos,
                            cur_frame_info->length + FRAME_HEADER_SIZE);
    if (!asd_flash_buffer_pos_in_range(flashbuf, pos)) {
        FBUFFER_DPRINTF("out of range: move pos %lu not in (%lu, %lu)\n", pos,
            flashbuf->tail.start_pos, flashbuf->head.start_pos);
        return ASD_FLASH_BUFFER_EBAD_POSITION;
    }

    //update cur_frame_info if cur_frame_info 0-length, e.g. previously it is the end of buffer.


    uint32_t cur_index = cur_frame_info->index;
    int32_t rc = 0;

    uint32_t meta_off = get_meta_offset(flashbuf->size, pos);
    if (get_meta_offset(flashbuf->size, cur_frame_info->start_pos) != meta_off) {
        // next frame is the first frame of new sector.
        // check meta data.
        flash_sector_meta_t meta;
        rc = _flash_read(flashbuf, meta_off, &meta, sizeof(meta));
        if (rc < 0) {
            return rc;
        }
        if (!is_valid_sector_meta(&meta)) {
            return ASD_FLASH_BUFFER_ECORRUPT;
        }
        if ((meta.frame_index != cur_frame_info->index + 1) ||
            (meta.frame_offset != get_offset_in_sector(pos))) {
            //Correct next frame info by meta data.
            cur_index = meta.frame_index - 1;
            pos = pos - get_offset_in_sector(pos) + meta.frame_offset;
        }
    }

    asd_frame_info_t frame_info;
    frame_info.start_pos = pos;
    frame_info.length = 0;
    frame_info.index = cur_index + 1;
    rc = asd_flash_buffer_read_frame_length(flashbuf, pos, &frame_info.length);
    if (rc < 0) {
        FBUFFER_DPRINTF("out of range: read frame length: pos %lu not in (%lu, %lu)\n",
            frame_info.start_pos,
            flashbuf->tail.start_pos, flashbuf->head.start_pos);

        return rc;
    }
    *next_frame_info = frame_info;
    return ASD_FLASH_BUFFER_OK;
}

int32_t asd_flash_buffer_read_payload(const asd_flash_buffer_t *flashbuf,
                          const asd_frame_info_t* current_frame_info, uint32_t offset,
                          uint8_t *buf, uint32_t size)
{
    FBUFFER_DPRINTF("read payload pos %lu %lu %lu ...", current_frame_info->start_pos, offset, size);
    if (!flashbuf || !flashbuf->size || !buf || !current_frame_info) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }
    uint32_t pos = move_pos(current_frame_info->start_pos, offset + FRAME_HEADER_SIZE);
    if (!asd_flash_buffer_pos_in_range(flashbuf, pos)) {
        return ASD_FLASH_BUFFER_EBAD_POSITION;
    }
    if (current_frame_info->length < offset)
        return ASD_FLASH_BUFFER_EBAD_PARAMS;

    if (current_frame_info->length == offset)
        return 0;

    uint32_t available_len = get_data_length_from(flashbuf, current_frame_info->start_pos);

    if (!available_len) return ASD_FLASH_BUFFER_ENO_BYTES;

    size = MIN(available_len, size);
    size = MIN(size, current_frame_info->length - offset);
    int rc = 0;

    // start of a frame. Only check if length == 0
    if (current_frame_info->length == 0) {
        frame_header_t header;
        //read frame header
        rc = asd_flash_buffer_read_data(flashbuf, current_frame_info->start_pos, &header, sizeof(header));
        if (rc < 0) return rc;
        //verify frame header.
        if (!is_valid_frame_head(&header)) {
            return ASD_FLASH_BUFFER_EBAD_FRAME_INFO;
        }
        if (current_frame_info->length != header.length) {
            return ASD_FLASH_BUFFER_EBAD_FRAME_INFO;
        }
    }

    //read payload data in buffer:
    rc = asd_flash_buffer_read_data(flashbuf, pos, buf, size);
    if (rc < 0) return rc;
    FBUFFER_DPRINTF("read payload %lu bytes", size);

    return size;
}

int32_t asd_flash_buffer_mark_read_before(asd_flash_buffer_t *flashbuf, uint32_t pos)
{
    if (!flashbuf || !flashbuf->size) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }

    if (!asd_flash_buffer_pos_in_range(flashbuf, pos)) {
        return ASD_FLASH_BUFFER_EBAD_POSITION;
    }

    if (buffer_is_empty(flashbuf)) return ASD_FLASH_BUFFER_OK;

    asd_frame_info_t frame_info;
    int32_t rc = 0;
    rc = asd_flash_buffer_get_first_unread_frame_info(flashbuf, &frame_info);
    if (rc < 0) return rc;
    while(frame_info.start_pos < pos &&
          frame_info.start_pos < flashbuf->head.start_pos) {
        rc = mark_read_flag(flashbuf, &frame_info);
        if (rc < 0) break;
        rc = asd_flash_buffer_get_next_frame_info(flashbuf, &frame_info, &frame_info);
        if (rc == ASD_FLASH_BUFFER_ENO_BYTES) {
            flashbuf->unread_tail.start_pos = flashbuf->head.start_pos;
            flashbuf->unread_tail.index = flashbuf->head.index;
            flashbuf->unread_tail.length = 0;
            rc = ASD_FLASH_BUFFER_OK;
            break;
        }
        if (rc < 0) break;
        //update unread_tail;
        flashbuf->unread_tail = frame_info;

    }

    return rc;

}

// =======================  local static functions ===========================

static int _flash_read(const asd_flash_buffer_t* flashbuf, uint32_t offset, void* buffer, uint32_t size)
{
    assert(flashbuf->size >= offset + size);
    int rc = fm_flash_read(flashbuf->flash_partition,
            FM_BYPASS_CLIENT_ID,
            flashbuf->start_offset + offset,
            size,
            (uint8_t*) buffer);
    if (rc > 0 && (uint32_t)rc == size) {
        FBUFFER_DPRINTF("Read %lu, %lu", flashbuf->start_offset + offset, size);
        return size;
    } else {
        FBUFFER_DPRINTF("Failed(%d) to read %lu, %lu", rc, flashbuf->start_offset + offset, size);
        return ASD_FLASH_BUFFER_EREAD_FAIL;
    }
}

static int _flash_write(const asd_flash_buffer_t* flashbuf, uint32_t offset, const void* buffer, uint32_t size)
{
    assert(flashbuf->size >= offset + size);
    int rc = fm_flash_write(flashbuf->flash_partition,
            FM_BYPASS_CLIENT_ID,
            flashbuf->start_offset + offset,
            size,
            (const uint8_t*) buffer);
    if (rc > 0 && (uint32_t)rc == size) {
        FBUFFER_DPRINTF("Write %lu, %lu", flashbuf->start_offset + offset, size);
        return size;
    } else {
        FBUFFER_DPRINTF("Failed(%d) to write %lu, %lu", rc, flashbuf->start_offset + offset, size);
        return ASD_FLASH_BUFFER_EWRITE_FAIL;
    }
}

static int _flash_erase(const asd_flash_buffer_t* flashbuf, uint32_t offset, uint32_t size)
{
    assert(flashbuf->size >= offset + size);
    int rc = fm_flash_erase_sectors(flashbuf->flash_partition,
            FM_BYPASS_CLIENT_ID,
            flashbuf->start_offset + offset,
            size);
    FBUFFER_DPRINTF("Erase %lu, 0x%lx, rc = %d", flashbuf->start_offset + offset, size, rc);
    return rc;
}

//Move the postion forward or backward. The moved bytes skip sector meta data(last 8 bytes of each sector)
// The input postion should be data segment, and should not fall in sector meta data.
static uint32_t move_pos(uint32_t pos, int32_t bytes)
{
    uint32_t offset_in_sector = get_offset_in_sector(pos);
    //check position is always in data segment.
    assert(offset_in_sector<DATA_SIZE_PER_SECTOR);
    if (bytes >= 0) {
        // move forward.
        uint32_t num_sector_meta = (offset_in_sector+bytes)/DATA_SIZE_PER_SECTOR;
        return (pos + bytes + SECTOR_META_SIZE*num_sector_meta);
    } else {
        // Move backward.
        offset_in_sector = DATA_SIZE_PER_SECTOR - offset_in_sector - 1;
        uint32_t num_sector_meta = (offset_in_sector-bytes)/DATA_SIZE_PER_SECTOR;
        return (pos + bytes - SECTOR_META_SIZE*num_sector_meta);
    }
}


static uint32_t get_data_length_between_offset(uint32_t bufsize, uint32_t start_off, uint32_t end_off)
{
    if (start_off > end_off) {
        //wrap case
        return ((end_off+bufsize)/FLASH_SECTOR_SIZE - start_off/FLASH_SECTOR_SIZE)*DATA_SIZE_PER_SECTOR +
                end_off%FLASH_SECTOR_SIZE - start_off%FLASH_SECTOR_SIZE;

    } else {
        return (end_off/FLASH_SECTOR_SIZE - start_off/FLASH_SECTOR_SIZE)*DATA_SIZE_PER_SECTOR +
                end_off%FLASH_SECTOR_SIZE - start_off%FLASH_SECTOR_SIZE;
    }
}

static uint32_t get_erased_data_bytes(const asd_flash_buffer_t *flashbuf)
{
    return get_data_length_between_offset(flashbuf->size,
                     convert_pos_to_offset(flashbuf->size, get_write_pos(&flashbuf->head)),
                     flashbuf->next_erase_index*FLASH_SECTOR_SIZE);

}

static bool data_is_all_FF(const uint8_t *data, uint32_t size)
{
    //address need to be dword aligned.
    assert((((uint32_t)data)&3) == 0);
    const uint32_t *pInt = (const uint32_t*) data;
    uint32_t i = 0;

    for (i = 0; i + sizeof(uint32_t) <= size; i += sizeof(uint32_t), ++pInt ) {
        if (*pInt != 0xFFFFFFFF) return false;

    }
    for (; i < size; ++i) {
        if (data[i] != 0xFF) return false;
    }
    return true;
}

static int32_t asd_flash_buffer_reset(asd_flash_buffer_t *flashbuf)
{
    assert(flashbuf);
    int rc = 0;
    //start the buffer from scratch.
    rc = _flash_erase(flashbuf, 0, FLASH_SECTOR_SIZE*2);
    if (rc < 0) return rc;
    if (flashbuf->size > FLASH_SECTOR_SIZE*2) {
        rc = _flash_erase(flashbuf, flashbuf->size-FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
        if (rc < 0) return rc;
    }

    memset(&flashbuf->head, 0, sizeof(flashbuf->head));
    memset(&flashbuf->tail, 0, sizeof(flashbuf->tail));
    memset(&flashbuf->unread_tail, 0, sizeof(flashbuf->unread_tail));
    memset(&flashbuf->last_frame, 0, sizeof(flashbuf->last_frame));
    memset(&flashbuf->cur_write_sector_meta, 0xFF, sizeof(flashbuf->cur_write_sector_meta));
    flashbuf->next_erase_index = 1;
    return set_sector_meta_frame_index(flashbuf, 0, 0);
}

static void update_with_head_frame_complete(asd_flash_buffer_t* flashbuf)
{
    //update tail frame if tail == head
    if (flashbuf->tail.start_pos == flashbuf->head.start_pos) {
        flashbuf->tail.length = flashbuf->head.w_length - FRAME_HEADER_SIZE;
        flashbuf->tail.index = flashbuf->head.index;
    }
    //update tail frame if unread_tail == head
    if (flashbuf->unread_tail.start_pos == flashbuf->head.start_pos) {
        flashbuf->unread_tail.length = flashbuf->head.w_length - FRAME_HEADER_SIZE;
        flashbuf->unread_tail.index = flashbuf->head.index;
    }
    //update the cached last frame.
    flashbuf->last_frame.index = flashbuf->head.index;
    flashbuf->last_frame.start_pos= flashbuf->head.start_pos;
    flashbuf->last_frame.length = flashbuf->head.w_length - FRAME_HEADER_SIZE;

    forward_head(flashbuf, flashbuf->head.w_length);
    ++flashbuf->head.index;

    //clear head w_length for next frame.
    flashbuf->head.w_length = 0;
    flashbuf->head.crc = 0;
}

static int32_t update_write_frame_header_in_flash(asd_flash_buffer_t* flashbuf)
{

    frame_header_t header;
    memset(&header, 0xFF, FRAME_HEADER_SIZE);
    header.magic_num = FRAME_HEADER_MAGIC_NUM;
    // compute the payload size.
    header.length = flashbuf->head.w_length - FRAME_HEADER_SIZE;
    uint16_t length = header.length;
    flashbuf->head.crc = crc8_update(flashbuf->head.crc,
                              (uint8_t*)&length, sizeof(length));
    header.crc = flashbuf->head.crc;
    // overwrite the frame header content, since the full frame are completed.
    return asd_flash_buffer_random_write_data(flashbuf,
                      flashbuf->head.start_pos,
                      (uint8_t*)&header,
                      sizeof(header));
}

static int32_t asd_flash_buffer_read_frame_length(const asd_flash_buffer_t *flashbuf,
                                    uint32_t frame_start_pos, uint16_t *length)
{
    if (!flashbuf || !flashbuf->size || !length) {
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    }
    if (!asd_flash_buffer_pos_in_range(flashbuf, frame_start_pos)) {
        return ASD_FLASH_BUFFER_EBAD_POSITION;
    }

    if (frame_start_pos == flashbuf->head.start_pos) {
        *length = 0;
        return ASD_FLASH_BUFFER_ENO_BYTES;
    }

    frame_header_t header;
    //read frame header
    int32_t rc = asd_flash_buffer_read_data(flashbuf, frame_start_pos,
                                            &header, sizeof(header));
    if (rc < 0) {
        return rc;
    }

    if (!is_frame_start_pos(flashbuf, frame_start_pos, false,
                            (uint8_t*)&header, sizeof(header))) {
        return ASD_FLASH_BUFFER_EBAD_FRAME_INFO;
    }

    *length = header.length;
    return ASD_FLASH_BUFFER_OK;
}

static bool is_valid_frame_head(const frame_header_t* header)
{
    if (header->magic_num != FRAME_HEADER_MAGIC_NUM) return false;
    if (header->length > FRAME_PAYLOAD_SIZE_MAX) return false;
    return true;
}

static int32_t search_end_in_last_sector(asd_flash_buffer_t *flashbuf, frame_state_t *frame)
{
    frame_header_t header = {0};
    int32_t rc = 0;
    uint32_t off = convert_pos_to_offset(flashbuf->size, frame->start_pos);
    uint32_t end_off = get_meta_offset_from_offset(off);

    uint32_t delta = 0;
    uint32_t frame_index = frame->index;

    while (off < end_off) {
        rc = asd_flash_buffer_read_data(flashbuf, off, &header, sizeof(header));
        if (rc < 0) return rc;
        if (data_is_all_FF((uint8_t*)&header, sizeof(header))) {
            // frame header no data. e.g. Eof.
            header.crc = 0;
            header.length = 0;
            break;
        }
        if (!is_valid_frame_head(&header)) return ASD_FLASH_BUFFER_ECORRUPT;
        //update the last frame info.
        flashbuf->last_frame.index = frame_index;
        flashbuf->last_frame.length = header.length;
        flashbuf->last_frame.start_pos =
            get_sector_start_addr(frame->start_pos) + get_offset_in_sector(off);
        if (off + header.length >= end_off) {
            break;  // last frame in the sector.
        }


        off += header.length + FRAME_HEADER_SIZE;
        delta += header.length + FRAME_HEADER_SIZE;
        frame_index ++;
    }
    //TODO: check CRC. FWPLATFORM-533

    frame->start_pos = move_pos(frame->start_pos, delta);
    frame->index = frame_index;
    frame->crc = header.crc;
    frame->w_length = (header.length)? (header.length + FRAME_HEADER_SIZE) : 0;


    return ASD_FLASH_BUFFER_OK;
}


static int32_t search_first_unread_frame(asd_flash_buffer_t *flashbuf,
                                                 const asd_frame_info_t *from,
                                                 asd_frame_info_t *unread_tail)
{
    int32_t rc = 0;
    frame_header_t header;
    if (!from || !flashbuf || !unread_tail || !flashbuf->size)
        return ASD_FLASH_BUFFER_EBAD_PARAMS;
    *unread_tail = *from;

    while (unread_tail->start_pos < flashbuf->head.start_pos) {
        //check the frame header one by one.
        rc = asd_flash_buffer_read_data(flashbuf, unread_tail->start_pos,
                     &header, sizeof(header));
        if (rc < 0) return rc;
        if (!is_valid_frame_head(&header))
            return ASD_FLASH_BUFFER_ECORRUPT;

        unread_tail->length = header.length;
        if (header.unread_flag) {
            //find the first unread frame, return it.
            return ASD_FLASH_BUFFER_OK;
        }
        // move to the next frame.
        unread_tail->index ++;
        unread_tail->start_pos = move_pos(unread_tail->start_pos,
                                 FRAME_HEADER_SIZE + header.length);

    }

    if (unread_tail->index != flashbuf->head.index ||
        unread_tail->start_pos != flashbuf->head.start_pos)
        return ASD_FLASH_BUFFER_ECORRUPT;
    // no frame data in unread frame (e.g. a empty frame).
    unread_tail->length = 0;
    return ASD_FLASH_BUFFER_OK;

}

static int32_t search_unread_tail(asd_flash_buffer_t *flashbuf, asd_frame_info_t *unread_tail)
{
    assert(unread_tail);
    assert(flashbuf);
    frame_header_t header;
    struct {
        uint32_t frame_pos;
        uint32_t sector_pos;
        uint32_t frame_index;
        bool unread;
    } left, right, mid;

    int32_t rc = ASD_FLASH_BUFFER_OK;
    if (buffer_is_empty(flashbuf)) {
        *unread_tail = flashbuf->tail;
        return ASD_FLASH_BUFFER_OK;
    }

    // buffer: tail .............>>> head (write head)

    //check the frame on the tail:
    rc = asd_flash_buffer_read_data(flashbuf, flashbuf->tail.start_pos,
                     &header, sizeof(header));
    if (rc < 0) return rc;

    if (!is_valid_frame_head(&header))
        return ASD_FLASH_BUFFER_ECORRUPT;

    if (header.unread_flag) {
        //the tail is unread.
        *unread_tail = flashbuf->tail;
        return ASD_FLASH_BUFFER_OK;
    }
    //the tail is read. Search the sector includes the first unread frame.
    left.frame_pos = flashbuf->tail.start_pos;
    left.sector_pos = get_sector_start_addr(flashbuf->tail.start_pos);
    left.frame_index = flashbuf->tail.index;
    left.unread = false;
    right.frame_pos = flashbuf->head.start_pos;
    right.sector_pos = get_sector_start_addr(flashbuf->head.start_pos);
    right.frame_index = flashbuf->head.index;
    right.unread = true;

    //binary search the first unread sector.
    while (left.sector_pos + FLASH_SECTOR_SIZE < right.sector_pos) {
        // We guarantee the bracket at least includes two sectors:
        // left sector always read, and rigth sector always unread.

        //check the mid sector:
        uint32_t mid_pos = (left.sector_pos + right.sector_pos)/2;
        //start from the beginning of the sector.
        mid_pos = get_sector_start_addr(mid_pos);
        mid.sector_pos = mid_pos;
        // Get the sector meta
        flash_sector_meta_t sector_meta;
        rc = _flash_read(flashbuf, get_meta_offset(flashbuf->size, mid_pos),
                         &sector_meta, sizeof(sector_meta));
        if (rc < 0) return ASD_FLASH_BUFFER_EREAD_FAIL;
        assert(sector_meta.page_bm == 0);
        // find the first frame in this sector.
        mid.frame_pos = mid_pos + sector_meta.frame_offset;
        mid.frame_index = sector_meta.frame_index;

        //check the unread flag of the first frame.
        rc = asd_flash_buffer_read_data(flashbuf, mid.frame_pos,
                     &header, sizeof(header));
        if (rc < 0) return rc;
        if (!is_valid_frame_head(&header)) {
            FBUFFER_DPRINTF("meta: %lx %lx\n", *((uint32_t*) &sector_meta), ((uint32_t*) &sector_meta)[1]);
            FBUFFER_DPRINTF("mid %lx %lx %lx\n", mid.frame_pos, mid.sector_pos, mid.frame_index);
            FBUFFER_DPRINTF("header : %lx\n", *((uint32_t*)&header));
            return ASD_FLASH_BUFFER_ECORRUPT;
        }
        mid.unread = !!header.unread_flag;
        if (mid.unread) {
            //mid sector is all unread. next search [left, mid]
            right = mid;
        } else {
            // mid sector includes read frames. next search [mid, right]
            left = mid;
        }
    }

    //Now, we find the sector (left) that before the unread frame.
    asd_frame_info_t from;
    from.start_pos = left.frame_pos;
    from.length = 0;
    from.index = left.frame_index;

    return search_first_unread_frame(flashbuf, &from, unread_tail);
}

// Intialize head frame info from flash. Head is for write position.
static int32_t init_head_from_flash(asd_flash_buffer_t *flashbuf)
{
    uint32_t off = 0;
    flash_sector_meta_t sector_meta;
    int32_t rc = 0;
    // 1. Find the first non-FF sector. The sector contains user data, and should
    //     in the middle of flash buffer.
    for (off = 0; off < flashbuf->size; off += FLASH_SECTOR_SIZE) {
        rc = _flash_read(flashbuf, get_meta_offset_from_offset(off), &sector_meta, sizeof(sector_meta));
        if (rc < 0) return ASD_FLASH_BUFFER_EREAD_FAIL;
        if (!data_is_all_FF((uint8_t*)&sector_meta, sizeof(sector_meta))) {
            //a sector has data.
            break;
        }
    }

    if (off >= flashbuf->size) {
        //didn't find the first non-FF sector. e.g. all sectors are FF.
        return ASD_FLASH_BUFFER_ENO_BYTES;
    }

    // 2. From the non-FF sector(non-empty), continue to find the first non-0
    //    page_bm (non-full sector), which is the last sector of buffer,
    //    e.g. the sector for write.
    for (; off < flashbuf->size; off += FLASH_SECTOR_SIZE) {
        rc = _flash_read(flashbuf, get_meta_offset_from_offset(off), &sector_meta, sizeof(sector_meta));
        if (rc < 0) return ASD_FLASH_BUFFER_EREAD_FAIL;
        if (sector_meta.page_bm != 0) {
            //this sector is the current write sector.
            break;
        }
    }

    // wrap offset.
    if (off >= flashbuf->size) {
        off = 0;
        rc = _flash_read(flashbuf, get_meta_offset_from_offset(off), &sector_meta, sizeof(sector_meta));
        if (rc < 0) return ASD_FLASH_BUFFER_EREAD_FAIL;
        //won't happen. There is always one sector is not full.
        if (sector_meta.page_bm == 0) return ASD_FLASH_BUFFER_ECORRUPT;
    }

    // verify sector meta data.
    if(sector_meta.frame_offset >= DATA_SIZE_PER_SECTOR) {
        return ASD_FLASH_BUFFER_ECORRUPT;
    }

    // 3. update head info as the first frame of the sector.
    memset(&flashbuf->head, 0, sizeof(flashbuf->head));
    flashbuf->head.start_pos = flashbuf->size + off + sector_meta.frame_offset;
    flashbuf->head.index = sector_meta.frame_index;
    // 4. Continue to search the last frame in the sector.
    rc = search_end_in_last_sector(flashbuf, &flashbuf->head);
    if (rc < 0) return rc;

    // We should find the end of the last frame.
    if(flashbuf->head.w_length != 0) {
        //data corruption.
        return ASD_FLASH_BUFFER_ECORRUPT;
    }
    // Update cached sector meta data.
    memcpy(&flashbuf->cur_write_sector_meta, &sector_meta, sizeof(sector_meta));
    // Update erase index.
    flashbuf->next_erase_index = get_sector_index(flashbuf->size, flashbuf->head.start_pos);
    flashbuf->next_erase_index = get_next_sector_index(flashbuf->size, flashbuf->next_erase_index);
    return ASD_FLASH_BUFFER_OK;

}

// Initialize tail from flash.
static int32_t init_tail_from_flash(asd_flash_buffer_t *flashbuf)
{
    uint32_t head_off = get_sector_start_offset(flashbuf->size, flashbuf->head.start_pos);
    uint32_t last_off = get_previous_sector_start_offset(flashbuf->size, flashbuf->head.start_pos);
    flash_sector_meta_t sector_meta;
    int32_t rc =0 ;
    frame_header_t header;

    // 1. From head -1 sector, search backward to find the first non-0 page_bm sector.
    for (uint32_t i  = 0; i < flashbuf->size/FLASH_SECTOR_SIZE; i++) {
        rc = _flash_read(flashbuf, get_meta_offset_from_offset(last_off), &sector_meta, sizeof(sector_meta));
        if (rc < 0) return ASD_FLASH_BUFFER_EREAD_FAIL;
        if (sector_meta.page_bm != 0) {
            // not a full buffer written. The last sector should be the tail.
            break;
        }

        last_off = get_previous_sector_start_offset(flashbuf->size, last_off);
    }

    // 2. Walk back from last_off until find a valid tail. Should return after first loop if the tail is
    //    not corrupted.
    while (1) {
        last_off = get_next_sector_start_offset(flashbuf->size, last_off);
        rc = _flash_read(flashbuf, get_meta_offset_from_offset(last_off), &sector_meta, sizeof(sector_meta));
        if (rc < 0) return ASD_FLASH_BUFFER_EREAD_FAIL;
        if (sector_meta.frame_offset >= DATA_SIZE_PER_SECTOR) return ASD_FLASH_BUFFER_ECORRUPT;

        if (flashbuf->size + last_off + sector_meta.frame_offset == flashbuf->head.start_pos) {
            //tail is same as head. Empty buffer.
            flashbuf->tail.start_pos = flashbuf->head.start_pos;
            flashbuf->tail.index = flashbuf->head.index;
            flashbuf->tail.length = flashbuf->head.w_length; //0
            flashbuf->unread_tail = flashbuf->tail;
            return ASD_FLASH_BUFFER_OK;
        }

        rc = asd_flash_buffer_read_data(flashbuf, last_off + sector_meta.frame_offset, &header, sizeof(header));
        if (rc < 0) return ASD_FLASH_BUFFER_EREAD_FAIL;

        if (is_valid_frame_head(&header)) {
            break;
        }
        else if (last_off == head_off) {
            return ASD_FLASH_BUFFER_ECORRUPT;
        }
    }

    flashbuf->tail.start_pos = flashbuf->size + last_off + sector_meta.frame_offset;
    if (flashbuf->tail.start_pos > flashbuf->head.start_pos) {
        flashbuf->tail.start_pos -= flashbuf->size;
    }
    flashbuf->tail.index = sector_meta.frame_index;
    flashbuf->tail.length = header.length;

    flashbuf->unread_tail = flashbuf->tail;
    // Search the unread tail.
    rc = search_unread_tail(flashbuf, &flashbuf->unread_tail);

    return rc;
}

static int32_t asd_flash_buffer_init_from_flash(asd_flash_buffer_t *flashbuf)
{
    int rc;

    //init head;
    rc = init_head_from_flash(flashbuf);

    if (rc == ASD_FLASH_BUFFER_ENO_BYTES) {
        // no data to restore
        FBUFFER_DPRINTF("No data to restore.\r\n");
        //start from scratch.
        return asd_flash_buffer_reset(flashbuf);

    }
    if (rc == ASD_FLASH_BUFFER_ECORRUPT) {
        // can't find flash head due to corruption.
        FBUFFER_DPRINTF("Can't find head due to corruption.\r\n");
        //start from scratch.
        return asd_flash_buffer_reset(flashbuf);
    }
    if (rc < 0) return rc;

    //init tail;
    rc = init_tail_from_flash(flashbuf);
    if (rc == ASD_FLASH_BUFFER_ECORRUPT) {
        // can't find flash tail due to corruption.
        FBUFFER_DPRINTF("Can't find tail due to corruption.\r\n");
        //start from scratch.
        return asd_flash_buffer_reset(flashbuf);
    }
    return rc;
}

//move head position forward.
static int32_t forward_head(asd_flash_buffer_t *flashbuf, uint32_t bytes)
{

    uint32_t new_head = move_pos(flashbuf->head.start_pos, bytes);

    //keep flashbuf->head no move before write_cur_sector_meta_page_bitmap().
    if (get_sector_index(flashbuf->size, flashbuf->head.start_pos)
        != get_sector_index(flashbuf->size, new_head)) {
        //across the sector boundry. complete bitmap for current sector, and get new sector ready.
        write_cur_sector_meta_page_bitmap(flashbuf, 0);
        //clear cached meta data for new sector.
        memset(&flashbuf->cur_write_sector_meta, 0xFF, sizeof (flashbuf->cur_write_sector_meta));
    }

    //move the head forward. Then update page bitmap in sector.
    flashbuf->head.start_pos = new_head;

    //update the bitmap of current sector.
    int32_t rc = ASD_FLASH_BUFFER_OK;
    if ((rc = update_cur_sector_meta_page_bitmap(flashbuf)) < 0) {
        return rc;
    }

    return bytes;
}


//move tail position forward.
static int32_t forward_tail(asd_flash_buffer_t *flashbuf, uint32_t bytes)
{
    flashbuf->tail.start_pos = move_pos(flashbuf->tail.start_pos, bytes);
    return bytes;
}

static int32_t asd_flash_buffer_erase_next_sector(asd_flash_buffer_t *flashbuf)
{
    int32_t rc = 0;
    //move tail if erase impact it.
    if (flashbuf->next_erase_index == get_sector_index(flashbuf->size, flashbuf->tail.start_pos)) {
        //available window moves due to erase.
        //read out the next sector after current read sector.
        uint32_t offset = get_next_sector_start_offset(flashbuf->size, flashbuf->tail.start_pos);
        flash_sector_meta_t meta;
        rc = _flash_read(flashbuf, get_meta_offset_from_offset(offset), &meta, sizeof(meta));
        if (rc < 0) return ASD_FLASH_BUFFER_EREAD_FAIL;

        //TODO: verify (offset + meta.frame_offset) is a start of valid frame. FWPLATFORM-433
        uint32_t forward_bytes = get_data_length_between_offset(flashbuf->size,
                           convert_pos_to_offset(flashbuf->size, flashbuf->tail.start_pos),
                           offset + meta.frame_offset);
        flashbuf->tail.index = meta.frame_index;
        forward_tail(flashbuf, forward_bytes);
        //update tail frame length.
        rc = asd_flash_buffer_read_frame_length(flashbuf, flashbuf->tail.start_pos, &flashbuf->tail.length);
        if (rc < 0) return rc;
        if (!asd_flash_buffer_pos_in_range(flashbuf, flashbuf->unread_tail.start_pos)) {
            //update unread tail if it is out of the window.
            flashbuf->unread_tail = flashbuf->tail;
        }

    }

    if ((rc = _flash_erase(flashbuf, flashbuf->next_erase_index * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE)) < 0) {
        return ASD_FLASH_BUFFER_EERASE_FAIL;
    }
    flashbuf->next_erase_index = get_next_sector_index(flashbuf->size, flashbuf->next_erase_index);

    return ASD_FLASH_BUFFER_OK;
}

static int32_t write_cur_sector_meta_page_bitmap(asd_flash_buffer_t *flashbuf, uint16_t page_bitmap)
{

    if ((~page_bitmap) & flashbuf->cur_write_sector_meta.page_bm) {
        page_bitmap &= flashbuf->cur_write_sector_meta.page_bm;
        int32_t rc = _flash_write(flashbuf,
                                  get_meta_offset(flashbuf->size, flashbuf->head.start_pos)
                                  + offsetof(flash_sector_meta_t, page_bm),
                                  &page_bitmap,
                                  sizeof(page_bitmap));
        if (rc < 0){
            return ASD_FLASH_BUFFER_EWRITE_FAIL;
        }
        flashbuf->cur_write_sector_meta.page_bm = page_bitmap;

    }
    return ASD_FLASH_BUFFER_OK;
}

static int32_t update_cur_sector_meta_page_bitmap(asd_flash_buffer_t *flashbuf)
{
    uint32_t pages = get_offset_in_sector(flashbuf->head.start_pos)/PAGE_SIZE;
    // flip the bitmap whose page completes writing. The page still in progress won't flip the bit.
    return write_cur_sector_meta_page_bitmap(flashbuf, (uint16_t)(0xFFFF<<pages));
}

//only support upto DATA_SIZE_PER_SECTOR (4088) bytes.
// random write. And skip the sector meta data.
static int32_t asd_flash_buffer_random_write_data(
                     asd_flash_buffer_t *flashbuf, uint32_t pos,
                     const uint8_t *buf, uint32_t size)
{
    if (size > DATA_SIZE_PER_SECTOR) {
        return ASD_FLASH_BUFFER_EFRAME_LIMIT;
    }
    int32_t rc = 0;

    uint32_t free_bytes_in_sector = DATA_SIZE_PER_SECTOR - get_offset_in_sector(pos);
    uint32_t offset = convert_pos_to_offset(flashbuf->size, pos);
    if (free_bytes_in_sector >= size) {
        rc = _flash_write(flashbuf,
                                  offset,
                                  buf,
                                  size);
    } else {

        rc = _flash_write(flashbuf,
                                  offset,
                                  buf,
                                  free_bytes_in_sector);
        if (rc < 0) {
            return ASD_FLASH_BUFFER_EWRITE_FAIL;
        }
        rc = _flash_write(flashbuf,
                                  get_next_sector_start_offset(flashbuf->size, pos),
                                  buf+free_bytes_in_sector,
                                  size - free_bytes_in_sector);
    }

    if (rc < 0) {
        return ASD_FLASH_BUFFER_EWRITE_FAIL;
    }
    return size;
}


//only support upto DATA_SIZE_PER_SECTOR (4088) bytes.
// This supports sequetial write from flashbuf->head.start_pos + head.w_length.
// And skip the sector meta data.
// For random write, use  asd_flash_buffer_random_write_data()
static int32_t asd_flash_buffer_write_data(asd_flash_buffer_t *flashbuf,
                                          const uint8_t *buf, uint32_t size)
{

    if (size > DATA_SIZE_PER_SECTOR) {
        return ASD_FLASH_BUFFER_EFRAME_LIMIT;
    }
    int32_t rc = 0;

    if (size >= get_erased_data_bytes(flashbuf)) {
        //erase next
        if ((rc = asd_flash_buffer_erase_next_sector(flashbuf)) < 0) {
            return rc;
        }
    }

    rc = asd_flash_buffer_random_write_data(flashbuf,
                     move_pos(flashbuf->head.start_pos, flashbuf->head.w_length),
                     buf, size);
    if (rc == (int32_t) size) {
        //add the length of written data. This includes frame header length.
        flashbuf->head.w_length += size;
    }

    return rc;
}


static int32_t set_sector_meta_frame_index(asd_flash_buffer_t *flashbuf, uint32_t frame_index, uint16_t offset)
{

    if (flashbuf->cur_write_sector_meta.frame_index != 0xFFFFFFFF) {
        //frame index already set. Can't set twice.
        return ASD_FLASH_BUFFER_EWRONG_STATE;
    }

    flash_sector_meta_t meta;
    //read sector meta before overwrite.
    int32_t rc = _flash_read(flashbuf, get_meta_offset(flashbuf->size, flashbuf->head.start_pos),
                     &meta, sizeof(meta));
    if (rc < 0) return ASD_FLASH_BUFFER_EREAD_FAIL;

    meta.frame_index = frame_index;
    meta.frame_offset = offset;
    //overwrite sector meta:
    rc = _flash_write(flashbuf,
                              get_meta_offset(flashbuf->size, flashbuf->head.start_pos),
                              &meta,
                              sizeof(meta));
    if (rc < 0) {
       return ASD_FLASH_BUFFER_EWRITE_FAIL;
    }

    flashbuf->cur_write_sector_meta.frame_index = frame_index;
    flashbuf->cur_write_sector_meta.frame_offset = offset;
    return ASD_FLASH_BUFFER_OK;
}

static bool is_frame_start_pos(const asd_flash_buffer_t *flashbuf, uint32_t pos, bool check_crc,
                      const uint8_t* data, uint32_t size)
{
    frame_header_t header0, header1;
    int32_t rc = 0;
    assert(flashbuf);

    if (!data || !size) {
        assert(size >= FRAME_HEADER_SIZE);
        rc = asd_flash_buffer_read_data(flashbuf,
                                  pos,
                                  &header0,
                                  sizeof(header0));
        if (rc < 0) {
            return false;
        }
    } else {
        memcpy(&header0, data, FRAME_HEADER_SIZE);
    }

    if (!is_valid_frame_head(&header0)) return false;

    if (!asd_flash_buffer_pos_in_range(flashbuf, move_pos(pos, header0.length + FRAME_HEADER_SIZE))) {
        //no full frame left. Wrong postion.
        return false;
    }

    if (asd_flash_buffer_pos_in_range(flashbuf, move_pos(pos, header0.length + 2*FRAME_HEADER_SIZE +1))) {
        // not the last frame
        rc = asd_flash_buffer_read_data(flashbuf, move_pos(pos, header0.length + FRAME_HEADER_SIZE),
                                  &header1,
                                  sizeof(header1));
        if (rc < 0) {
            return false;
        }
        if (header0.magic_num != FRAME_HEADER_MAGIC_NUM) return false;
    } else if (move_pos(pos, header0.length + FRAME_HEADER_SIZE) != flashbuf->head.start_pos) {
        // should be last frame. but length doesn't match.
        return false;
    }

    if (check_crc) {
        //TODO read payload data, and check crc. FWPLATFORM-533
        return true;
    }

    return true;
}

static bool asd_flash_buffer_pos_in_range(const asd_flash_buffer_t *flashbuf, uint32_t pos)
{
    return (flashbuf->head.start_pos >= flashbuf->tail.start_pos) ?
           (pos <= flashbuf->head.start_pos && pos >= flashbuf->tail.start_pos) :
           (pos <= flashbuf->head.start_pos || pos >= flashbuf->tail.start_pos);
}

static int32_t asd_flash_buffer_read_data(const asd_flash_buffer_t *flashbuf,
                                              uint32_t pos, void* buf, uint32_t size)
{
    uint32_t total_size = size;
    while (size > 0) {
        uint32_t start_off = convert_pos_to_offset(flashbuf->size, pos);
        uint32_t in_sector_off = get_offset_in_sector(pos);
        uint32_t read_len = MIN(size, DATA_SIZE_PER_SECTOR-in_sector_off);
        int32_t rc = _flash_read(flashbuf,
                              start_off,
                              buf,
                              read_len);
        if (rc < 0) return ASD_FLASH_BUFFER_EREAD_FAIL;
        buf = (uint8_t*)buf + read_len;
        size -= read_len;
        pos = move_pos(pos, read_len);
    }

    return total_size;
}


static int32_t mark_read_flag(asd_flash_buffer_t *flashbuf, asd_frame_info_t *frame_info)
{
    frame_header_t header;

    //read back the header content first.
    int32_t rc = asd_flash_buffer_read_data(flashbuf, frame_info->start_pos,
                                            &header, sizeof(header));
    if (rc < 0) {
        return rc;
    }

    if (!is_frame_start_pos(flashbuf, frame_info->start_pos, false,
                            (uint8_t*)&header, sizeof(header))) {
        return ASD_FLASH_BUFFER_EBAD_FRAME_INFO;
    }

    //Overwrite header for read flags.
    header.unread_flag = 0;
    return asd_flash_buffer_random_write_data(flashbuf,
                        frame_info->start_pos,
                        (uint8_t*) &header,
                        sizeof(header));
}
