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
*@File: asd_flash_buffer.h
********************************************************************************
*/

/**
 * @file
 *
 * This is an implementation of a flash buffer.
 *
 * This implementation contains a flash buffer with sector meta data allocated at
 * last 8 bytes of every sector (4K). And the user data is packed in a data frame.
 * The data frame contains a header following payload(user data).
 * The sector meta data (8 bytes) maintains page bitmap(16 bits), first frame index
 * and frame header offset in current sector. Those information is useful for data
 * integrity, and helps to recover safely after device reboots.
 * The frame header includes payload length, and crc, which provide data verification.
 * User data read/write should be based on frame.
 *
 * The total size of this flash segment should be atleast 8K.
 *
 * @remark This implementation assumes contiguous 0xFF bytes no more than
 * 4096 bytes. This allows the library to recover safely after device reboots.
 *
 * 0 --------------------------4088-------------
 * ...|frame header|payload|...|sector meta data|
 * +4K ------------------------4088--------------
 * ...|frame header|payload|...|sector meta data|
 * +4K ------------------------4088--------------
 * ...|frame header|payload|...|sector meta data|
 * +4K ------------------------4088--------------
 * ...|frame header|payload|...|sector meta data|
 * +4K ------------------------4088--------------
 * ...|frame header|payload|...|sector meta data|
 * +4K ------------------------4088--------------
 * ...|frame header|payload|...|sector meta data|
 *   .
 *   .
 *   .
 * +4K ------------------------4088--------------
 * ...|frame header|payload|...|sector meta data|
 * ---------------------roll back 0 ------------
 *
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "flash_manager.h"

#ifdef __cplusplus
extern "C"
{
#endif


#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE        (4*1024)
#endif
#define DATA_SIZE_PER_SECTOR     (FLASH_SECTOR_SIZE - sizeof(flash_sector_meta_t))
#define SECTOR_META_SIZE         (sizeof(flash_sector_meta_t))
#define FRAME_HEADER_SIZE        (sizeof(frame_header_t))

//payload size smaller than one sector excluding sector meta data (8 bytes)
// and frame header (4 bytes)
#define FRAME_PAYLOAD_SIZE_MAX      (FLASH_SECTOR_SIZE - SECTOR_META_SIZE - FRAME_HEADER_SIZE)

//The buffer size must be at least 8K. We will erase the next sector when
//current sector is full. One sector can't support such operation.
#define SUPPORTED_FLASH_BUFFER_SIZE_MIN   (2*FLASH_SECTOR_SIZE)

#define ASD_FLASH_BUFFER_OK                    0

/**
 * @brief Flash may be corrupted
 */
#define ASD_FLASH_BUFFER_ECORRUPT              (-101)

/**
 * @brief Flash erase failed
 */
#define ASD_FLASH_BUFFER_EERASE_FAIL           (-102)

/**
 * @brief Flash read failed
 */
#define ASD_FLASH_BUFFER_EREAD_FAIL            (-103)

/**
 * @brief Flash write failed
 */
#define ASD_FLASH_BUFFER_EWRITE_FAIL           (-104)

/**
 * @brief Cannot find flash partition
 */
#define ASD_FLASH_BUFFER_ECANT_FIND            (-105)

/**
 * @brief Cannot access flash partition
 */
#define ASD_FLASH_BUFFER_EACCESS_FAIL          (-106)

/**
 * @brief Flash partition is full
 */
#define ASD_FLASH_BUFFER_ENO_FREE_SPACE        (-107)

/**
 * @brief The data is not in the state for the operation to be performed
 */
#define ASD_FLASH_BUFFER_EWRONG_STATE          (-108)

/**
 * @brief No more bytes to read
 */
#define ASD_FLASH_BUFFER_ENO_BYTES             (-109)

/**
 * @brief Reach the frame size limit
 */
#define ASD_FLASH_BUFFER_EFRAME_LIMIT          (-110)

/**
 * @brief NULL pointer error.
 */
#define ASD_FLASH_BUFFER_EBAD_PARAMS           (-111)

/**
 * @brief bad position, out of range.
 */
#define ASD_FLASH_BUFFER_EBAD_POSITION         (-112)

/**
 * @brief wrong frame info. Resync is required.
 */
#define ASD_FLASH_BUFFER_EBAD_FRAME_INFO       (-113)

/**
 * @brief Ouf of memory.
 */
#define ASD_FLASH_BUFFER_OUT_OF_MEMORY         (-114)

// frame information struct. It describes the position of frame
// in flash buffer, frame index and frame length.
// Frame start_pos is useful for random read access.
// Frame index is useful to calculate number of frames.
typedef struct {
    uint32_t start_pos;      ///< Start position of the frame
    uint32_t index;          ///< The frame index.
    uint16_t length;         ///< length of the frame payload.
} asd_frame_info_t;

// Sector meta data struct
typedef struct {
    uint32_t frame_index;           ///< The frame index of first frame header in the current sector.
    uint16_t frame_offset   :12;    ///< The offset of first frame header. Help quick locate the first frame for random access.
    uint16_t reserved       :4;
    uint16_t page_bm;               ///< There is total 4096/256 = 16 pages in a sector.
                                    ///< 16 bits are required to mark which page is currently writing.
} __attribute__((packed)) flash_sector_meta_t;

//Frame header struct.
typedef struct {
    uint8_t magic_num;              ///< The magic number must be a unique value. Combining with length
                                    ///< and CRC, it can recover from a corrupted data. 0x7A. This is the high 7bits of the first byte.
    uint8_t crc;                    ///< 8-bit CRC for data after field "CRC".
    uint16_t length             :12;///< Data length of the frame payload, excluding frame header.
    uint16_t unread_flag        :1; ///< Flag shows the log is read. Default is 1 (unread).
                                    ///< Can not protected in CRC, since we need to change it on the fly.
    uint16_t reserved           :3;
    uint8_t payload[];              ///< Payload of the data frame. The length is defined in "length" field
} __attribute__((packed)) frame_header_t;

//frame state for write.
typedef struct {
    uint32_t start_pos;
    uint32_t index;          ///< Frame index of current write.
    uint16_t w_length;       ///< bytes already written. It includes length of frame header.
    uint8_t  crc;            ///< crc for frame.
} frame_state_t;

// struct of flash ring buffer. Single consumer/producer.
struct asd_flash_buffer {
    frame_state_t    head;              ///< head of buffer. This is the frame state of write.
    asd_frame_info_t tail;              ///< tail of buffer. This is the frame info of start of
                                        ///< all available log.
    asd_frame_info_t unread_tail;       ///< first frame unread in buffer
    asd_frame_info_t last_frame;        ///< last frame written in buffer
    uint32_t start_offset;              ///< flash start offset based on flash partition.
    uint32_t size;                      ///< must be 4K aligned. size of the raw buffer.

    flash_sector_meta_t cur_write_sector_meta;///< meta data copy of current sector.
    uint16_t next_erase_index;          ///< next sector index for erase
                                        ///< directly frame header, and user manages the data structure.
    struct fm_flash_partition* flash_partition;
};

typedef struct asd_flash_buffer asd_flash_buffer_t;

/**
 * @brief Initialize flash buffer with flash manager partition,
 *        flash start offset, and size. If the data in
 *        correspoding flash region is not available, it will create a new buffer
 *        on that flash region. If the data exists in flash region, it will load
 *        data from flash, and initialize the state of flash buffer.
 *
 * @param [in/out] pflashbuf: pointer of flash buffer to initialize.
 * @param [in] flash_partition: flash partition. Use this partition pointer
 *             to call fm_flash_manager APIs.
 * @param [in] offset_in_partition: start offset of the buffer in the partition. The offset is
 *             from the base of the partition passed in.
 * @param [in] size: size of the buffer in flash.
 * @param [in] load_from_flash: true: load state from flash
 *                              false: erase the flash, and start from no data.
 *
 * @return 0 on success, or negative on failure.(Check ASD_FLASH_BUFFER_Exxx for error code)
 */
int32_t asd_flash_buffer_init(asd_flash_buffer_t *pflashbuf,
                   struct fm_flash_partition* flash_partition,
                   uint32_t offset_in_partition,
                   uint32_t size,
                   bool load_from_flash);

/**
 * @brief Deinitialize the flash buffer. It won't delete data in flash.
 *
 * @param[in/out] pflashbuf: Pointer of a flash buffer.
 * @return 0 on success, or negative on failure
 */
int32_t asd_flash_buffer_deinit(asd_flash_buffer_t *ppflashbuf);

/**
 * @brief Write frame payload in flash. A payload can be split in multi-call of
 *        this function, but the last part of data write must be with complete = true.
 *        It only support maximum one frame payload in one call. Not thread_safe.
 *
 * @param [in/out] flashbuf: pointer of flash buffer.
 * @param [in] buf: data buffer to write.
 * @param [in] size: size of data to write.
 * @param [in] complete: true: complete the current frame, a frame head will be
 *             filled in flash; false: there are more
 *             payload data coming, the current frame won't finish. Data is still
 *             written in flash, but the frame header is incomplete in flash.
 *             When size is 0 and complete is true, it only complete the current frame,
 *             e.g. write the frame header to flash.
 *
 * @return number of bytes written on success, or negative for an error on failure.
 *         (Check ASD_FLASH_BUFFER_Exxx for error code)
 */
int32_t asd_flash_buffer_write_payload(asd_flash_buffer_t *flashbuf, const uint8_t *buf, uint32_t size, bool complete);

/**
 * @brief Get the frame info of first frame in buffer, e.g on buffer tail.
 *        This is the postion that first frame starts in the buffer, regardless
 *        of read or unread frame. The frame may be read or unread.
 *
 * @param [in] flashbuf: pointer of flash buffer.
 * @param [out] first_frame_info: the frame on tail of buffer. Start position of first frame
 *                       regardless of read or unread data.
 *
 * @return 0 on success, or negative for an error on failure. (Check ASD_FLASH_BUFFER_Exxx for error code)
 *         ASD_FLASH_BUFFER_ENO_BYTES: no frame at all, e.g. buffer is empty.
 */
int32_t asd_flash_buffer_get_first_frame_info(const asd_flash_buffer_t *flashbuf,
                                              asd_frame_info_t *first_frame_info);

/**
 * @brief Get the frame info of last frame in buffer, e.g. the frame before head.
 *        The frame may be read or unread.
 *
 * @param [in] flashbuf: pointer of flash buffer.
 * @param [out] last_frame_info: the last frame before buffer head.
 *
 * @return 0 on success, or negative for an error on failure. (Check ASD_FLASH_BUFFER_Exxx for error code)
 *         ASD_FLASH_BUFFER_ENO_BYTES: no frame at all, e.g. buffer is empty.
 */
int32_t asd_flash_buffer_get_last_frame_info(const asd_flash_buffer_t *flashbuf,
                                              asd_frame_info_t *last_frame_info);

/**
 * @brief Get the frame info of first unread frame.
 *        The frame must be unread data.
 *
 * @param [in] flashbuf: pointer of flash buffer.
 * @param [out] first_unread_frame_info: the first unread frame info.
 *                              Start position of first unread frame.
 *
 * @return 0 on success, or negative for an error on failure. (Check ASD_FLASH_BUFFER_Exxx for error code)
 *         ASD_FLASH_BUFFER_ENO_BYTES: no unread frame, all data is read.
 */
int32_t asd_flash_buffer_get_first_unread_frame_info(const asd_flash_buffer_t *flashbuf,
                                             asd_frame_info_t *first_unread_frame_info);

/**
 * @brief Get the information of next frame after frame_info.
 *        The frame may be read or unread.
 *
 * @param [in] flashbuf: pointer of flash buffer.
 * @param [in] cur_frame_info: input the current frame information.
 * @param [out] next_frame_info: output the next frame information.
 *
 * @return 0 on success, or negative for an error on failure. (Check ASD_FLASH_BUFFER_Exxx for error code)
 *         ASD_FLASH_BUFFER_ENO_BYTES: when cur_frame_info->start_pos points the
 *         head of buffer, no next frame, it returns  read can stop. Before next
 *         read,
 */
int32_t asd_flash_buffer_get_next_frame_info(const asd_flash_buffer_t *flashbuf,
                const asd_frame_info_t *cur_frame_info, asd_frame_info_t *next_frame_info);

/**
 * @brief Read frame payload in flash. Support partial payload reading.
 *        Not thread_safe.
 *
 * @param [in] flashbuf: pointer of flash buffer.
 * @param [in] current_frame_info: The information of the frame to be read. Must be returned from APIs,
 *                         caller should not modify the internal members.
 * @param [in] offset: The data offset in the frame payload to start read. To support multiple
 *                     read for one payload: offset is adjusted by caller to read segment by
 *                     segment. offset should not larger than frame_info->length;
 * @param [out] buf: data buffer accomodating read data.
 * @param [in] size: maximum size of data to read.
 *
 * @return number of bytes read on success, or negative for an error on failure.
 *         (Check ASD_FLASH_BUFFER_Exxx for error code)
 *         The read bytes are actual payload size returned in buf, and may less
 *         than parameter "size". To read the next frame payload, call
 *         asd_flash_buffer_get_next_frame_info().
 */
int32_t asd_flash_buffer_read_payload(const asd_flash_buffer_t *flashbuf,
                    const asd_frame_info_t *current_frame_info, uint32_t offset, uint8_t *buf, uint32_t size);

/**
 * @brief Get free bytes left in the buffer.
 *
 * @param [in] flashbuf: pointer of flash buffer.
 *
 * @return ASD_FLASH_BUFFER_EBAD_PARAMS: flashbuf is unintialized, or flashbuf NULL;
 *          >=0: number of free bytes in buffer. User should take the frame header length
 *         into account.
 */
int32_t asd_flash_buffer_get_free_size(const asd_flash_buffer_t *flashbuf);

/**
 * @brief Get total size of the flash buffer, excluding sector meta data.
 *
 * @param [in] flashbuf: pointer of flash buffer.
 *
 * @return ASD_FLASH_BUFFER_EBAD_PARAMS: flashbuf is unintialized, or flashbuf NULL;
 *         >0: size of the flash buffer.
 */
int32_t asd_flash_buffer_get_total_size(const asd_flash_buffer_t *flashbuf);

/**
 * @brief Get data bytes, and frame number available in buffer regardless of
 *        read or unread data. The length excludes sector meta data.
 *
 * @param [in] flashbuf: pointer of flash buffer.
 * @param [out] data_length: pointer to output data length available in buffer.
 * @param [out] frame_num: pointer to output number of total frames available in buffer.
 *
 * @return 0 on success, or negative on failure. (Check ASD_FLASH_BUFFER_Exxx for error code)
 */
int32_t asd_flash_buffer_get_data_length_and_frame_num(const asd_flash_buffer_t *flashbuf,
                         uint32_t *data_length, uint32_t *frame_num);

/**
 * @brief Erase the entire flash region. All data will be gone, and buffer state
 *        is reset. Not thread_safe.
 *
 * This is an expensive operation and therefore should be used with caution.
 *
 * @param[in/out]  flashbuf  Pointer to a flash buffer object
 * @return 0 on success, or negative on failure. (Check ASD_FLASH_BUFFER_Exxx for error code)
 */
int32_t asd_flash_buffer_erase_all(asd_flash_buffer_t *flashbuf);

/**
 * @brief Erase the sectors before a position. Not thread_safe.
 *
 * This is an expensive operation and therefore should be used with caution.
 *
 * @param[in/out]  flashbuf:  Pointer to a flash buffer object.
 * @param[in]  pos:  erase stop at the sector "pos" is pointing. It won't erase
 *                  the sector "pos" pointing.
 *
 * @return 0 on success, or negative on failure. (Check ASD_FLASH_BUFFER_Exxx for error code)
 */
int32_t asd_flash_buffer_erase_before(asd_flash_buffer_t *flashbuf, uint32_t pos);

/**
 * @brief Get the length of user data, and number of frames after the frame in frame_info.
 *        Not thread_safe.
 *
 * @param[in]  flashbuf:  Pointer to a flash buffer object.
 * @param[in]  frame_info: The frame information to calculate how many user data
 *                         and frames starting from this frame.
 * @param[out] data_length: Pointer to return user data length starting from the frame.
 * @param[out] frame_num: Pointer to return how many frames starting from the frame.
 *
 * @return 0 on success, or negative on failure. (Check ASD_FLASH_BUFFER_Exxx for error code)
 */
int32_t asd_flash_buffer_get_data_length_and_frame_num_from_frame(
                        const asd_flash_buffer_t *flashbuf, asd_frame_info_t* frame_info,
                        uint32_t *data_length, uint32_t *frame_num);

/**
 * @brief Mark frames as read before the postion. It flips a read flag in flash
 *        for each frame.
 *
 * @param[in/out]  flashbuf:  Pointer to a flash buffer object.
 * @param[in]  frame_start_pos: before this position, all frames are marked as read-already.
 *
 * @return 0 on success, or negative on failure. (Check ASD_FLASH_BUFFER_Exxx for error code)
 */
int32_t asd_flash_buffer_mark_read_before(asd_flash_buffer_t *flashbuf, uint32_t frame_start_pos);

#ifdef __cplusplus
}
#endif
