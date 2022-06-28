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
* log reader implementation.
*@File: asd_log_reader.c
********************************************************************************
*/


#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include "log_request.h"
#include "asd_log_api.h"
#include "asd_logger_if.h"
#include "asd_logger_internal_config.h"
#include "asd_log_reader.h"
#include "asd_log_msg.h"
#include "asd_crashdump.h"
#include "asd_log_platform_api.h"
#include "log_request_queue.h"
#include "asd_logger_impl.h"


// default timeout for log reader.
#define LOG_READER_TIMEOUT_TICK_DEFAULT      pdMS_TO_TICKS(10000)
#define RESYNC_LOG_POS_MSG     "\n###Resync pos\n"
static int32_t log_reader_backend_read_log_lines_from_flash(const asd_flash_mgr_t* flashmgr, asd_log_reader_t* reader);
static int32_t log_reader_backend_read_crash_log(asd_log_reader_t* reader);

asd_log_create_module(logReader, ASD_LOG_LEVEL_DEFAULT, ASD_LOG_PLATFORM_STREAM_BM_DEFAULT);

asd_log_reader_t* log_reader_create(uint8_t log_id, uint32_t options)
{
    asd_log_reader_t* reader = (asd_log_reader_t*) malloc(sizeof(asd_log_reader_t));
    if (!reader) return NULL;
    memset(reader, 0, sizeof(asd_log_reader_t));
    reader->log_id = log_id;
    reader->data_type = (options & LOG_DATA_TYPE_BIT_TXT)?
                       ASD_LOG_DATA_TYPE_LOG_TXT : ASD_LOG_DATA_TYPE_BIN;
    return reader;
}

int32_t log_reader_setpos (asd_log_reader_t* reader, log_reader_origin_t origin)
{
    asd_log_request_op_t op;

    if (!reader) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    //build request:
    switch (origin) {
    case LOG_POSITION_START:
        op = LOG_REQUEST_SET_POSITION_START;
        break;
    case LOG_POSITION_UNREAD:
        op = LOG_REQUEST_SET_POSITION_UNREAD;
        break;
    case LOG_POSITION_END:
        op = LOG_REQUEST_SET_POSITION_END;
        break;
    default:
        printf("Wrong origin.%d" ASD_LINE_ENDING, origin);
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    }
    asd_log_request_t *req = asd_log_request_alloc();
    if (!req) return ASD_LOGGER_ERROR_NO_RESOURCE;

    ASD_LOG_REQUEST_INIT(req, op, reader->log_id, 0, reader);
    reader->offset_in_frame = 0;
    reader->read_len = 0;
    int32_t rc = log_request_send_and_wait_completion(req, LOG_READER_TIMEOUT_TICK_DEFAULT);
    if (rc < 0) {
        reader->frame_info.length = 0;
    } else {
       LOGGER_DPRINTF("Get frame_info %lu, %lu, %lu", reader->frame_info.start_pos,
             (uint32_t) reader->frame_info.length, reader->frame_info.index);
    }
    //always free request, it will reduce ref counter.
    asd_log_request_free(req);

    return rc;

}

int32_t log_reader_get_remaining_log_length(asd_log_reader_t* reader)
{
    if (!reader) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    asd_log_request_t *req = asd_log_request_alloc();
    if (!req) return ASD_LOGGER_ERROR_NO_RESOURCE;

    ASD_LOG_REQUEST_INIT(req, LOG_REQUEST_GET_DATA_LENGTH, reader->log_id, 0, reader);

    int32_t rc = log_request_send_and_wait_completion(req, LOG_READER_TIMEOUT_TICK_DEFAULT);
    //always free request, it will reduce ref counter.
    asd_log_request_free(req);
    return rc;
}

int32_t log_reader_read(asd_log_reader_t* reader, void* buf, uint32_t size)
{
    if (!reader || !buf || !size || !reader->frame_info.length)
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    reader->buf = buf;
    reader->buf_len = size;
    asd_log_request_t *req = asd_log_request_alloc();
    if (!req) return ASD_LOGGER_ERROR_NO_RESOURCE;

    ASD_LOG_REQUEST_INIT(req, LOG_REQUEST_READ_LOG, reader->log_id, 0, reader);
    int32_t rc = log_request_send_and_wait_completion(req, LOG_READER_TIMEOUT_TICK_DEFAULT);
    //always free request, it will reduce ref counter.
    asd_log_request_free(req);
    return rc;

}

bool log_reader_eof(asd_log_reader_t* reader)
{
    if (!reader) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    return log_reader_get_remaining_log_length(reader) == 0;
}
int32_t log_reader_mark_read_flag(asd_log_reader_t* reader)
{

    asd_log_request_t *req = asd_log_request_alloc();
    if (!req) return ASD_LOGGER_ERROR_NO_RESOURCE;

    ASD_LOG_REQUEST_INIT(req, LOG_REQUEST_MARK_READ_FLAG, reader->log_id, 0, reader);
    int32_t rc = log_request_send_and_wait_completion(req, LOG_READER_TIMEOUT_TICK_DEFAULT);
    //always free request, it will reduce ref counter.
    asd_log_request_free(req);
    return rc;

}

void log_reader_destroy(asd_log_reader_t*  reader)
{
    if (!reader) return;
    free(reader);
}

int32_t log_reader_backend_read_log_from_flash(const asd_flash_mgr_t* flashmgr, asd_log_reader_t* reader)
{
    if (!reader) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    if (reader->log_id >= ASD_LOG_ID_NUM) return ASD_LOGGER_ERROR;

    switch (reader->log_id) {
    case ASD_LOG_ID_CRASH_LOG:
        return log_reader_backend_read_crash_log(reader);
    default:
        return log_reader_backend_read_log_lines_from_flash(flashmgr, reader);
    }
}

static int32_t log_reader_backend_read_crash_log(asd_log_reader_t* reader)
{
    int32_t rc = 0;

    if (!reader->buf
        || !reader->buf_len
        || ASD_LOG_DATA_TYPE_LOG_TXT != reader->data_type
        || !reader->frame_info.length) {
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    }
    uint32_t bufsize = reader->buf_len;
    char* buf = (char*) reader->buf;
    asd_frame_info_t frame_info = reader->frame_info;
    uint32_t  offset_in_frame = reader->offset_in_frame;
    uint32_t log_len = 0;

    while (bufsize > 0) {
        if (offset_in_frame == frame_info.length) {
            asd_frame_info_t next_frame_info = {0};
            //frame already read.
            //move to next frame.
            rc = asd_crashdump_get_next_frame(
                       &frame_info,
                       &next_frame_info);
            if (rc < 0)  break;
            frame_info = next_frame_info;
            offset_in_frame = 0;
            if (!next_frame_info.length) continue;
        }

        rc = asd_crashdump_read_frame(
                  &frame_info,
                  offset_in_frame,
                  buf,
                  bufsize);
        if (rc < 0) break;

        //update output buffer:
        log_len += rc;
        buf += rc;
        bufsize -= rc;
        //update offset for frame.
        offset_in_frame += rc;
        if ((rc == 0) || (offset_in_frame > frame_info.length)) {
            // the reported size doesn't match the actual data reading.
            //
            ASD_LOG_W(logReader, "crash log section %d length reports %d, but read %d",
                            (int) frame_info.index, (int) frame_info.length,
                            (int) offset_in_frame);
            //overwrite offset_in_frame.
            offset_in_frame = frame_info.length;
        }

    }

    if (rc < 0 && rc != -AFW_ENOSPC) {
        //error but not EOF.
        return rc;
    }
    // read success, update frame info.
    reader->frame_info = frame_info;
    reader->offset_in_frame = offset_in_frame;
    reader->read_len += log_len;

    //return the decoded log length
    return log_len;

}

static int32_t log_reader_backend_read_log_lines_from_flash(const asd_flash_mgr_t* flashmgr, asd_log_reader_t* reader)
{
    int32_t rc = 0;
    asd_flash_buffer_t* flashbuf =
                    asd_flash_mgr_get_buffer_by_log_id(flashmgr, reader->log_id);

    bool txt_log = (ASD_LOG_DATA_TYPE_LOG_TXT == asd_flash_mgr_get_buffer_data_type(
                         flashmgr, reader->log_id));
    if (!reader->buf
        || !txt_log
        || ASD_LOG_DATA_TYPE_LOG_TXT != reader->data_type
        || !reader->frame_info.length
        || !flashbuf) {
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    }
    uint32_t bufsize = reader->buf_len;
    char* buf = (char*) reader->buf;
    asd_frame_info_t frame_info = reader->frame_info;
    uint32_t  offset_in_frame = reader->offset_in_frame;
    uint32_t log_len = 0;

    while (bufsize >= ASD_LOG_LINE_HEADER_TXT_SIZE) {
        //read one line.
        if (offset_in_frame >= frame_info.length) {
            asd_frame_info_t next_frame_info;
            //frame already read.
            //move to next frame.
            rc = asd_flash_buffer_get_next_frame_info(
                       flashbuf,
                       &frame_info,
                       &next_frame_info);
            if (ASD_FLASH_BUFFER_EBAD_POSITION == rc) {
                //the frame start_pos is out of range. update it to the latest first frame in log buffer.
                rc = asd_flash_buffer_get_first_frame_info(flashbuf, &frame_info);
                if (rc < 0) {
                    //if still fail, return error;
                    break;
                }
                offset_in_frame = 0;
                //report a log position resync in read log. Some log are missed, due to position resync.
                strncpy(buf,  RESYNC_LOG_POS_MSG, bufsize);
                uint32_t aLen = strlen(RESYNC_LOG_POS_MSG);
                aLen = MIN(bufsize, aLen);
                //update output buffer:
                log_len += aLen;
                buf += aLen;
                bufsize -= aLen;
                //go to the next frame read.
                continue;
            }
            if (rc < 0) {
                if (ASD_FLASH_BUFFER_ENO_BYTES != rc) {
                    ASD_LOG_E(logReader, "Fail to get frame info rc = %d", rc);
                    ASD_LOG_E(logReader, "frame info pos: %lu, log_len %lu",
                        frame_info.start_pos, log_len);
                }

                break;
            }
            frame_info = next_frame_info;
            offset_in_frame = 0;
            assert(next_frame_info.length);
        }
        if (bufsize < frame_info.length + ASD_LOG_LINE_HEADER_SIZE_DELTA + 2) {
            //the rest buffer can not accomdate the whole log line. skip.
            if (log_len == 0) {
                //the buffer is too small
                rc = ASD_LOGGER_ERROR_OVERFLOW;
                ASD_LOG_E(logReader, "too small buffer");
            }
            break;
        }
        // read one payload/line data. Reserve the line header txt size, so that
        // we can decode line header without move the line body data.
        rc = asd_flash_buffer_read_payload(
                  flashbuf,
                  &frame_info,
                  0,
                  (uint8_t*) buf + ASD_LOG_LINE_HEADER_SIZE_DELTA,
                  bufsize - ASD_LOG_LINE_HEADER_SIZE_DELTA);
        if (ASD_FLASH_BUFFER_EBAD_POSITION == rc) {
            //the frame start_pos is out of range. update it to the latest first frame in log buffer.
            rc = asd_flash_buffer_get_first_frame_info(flashbuf, &frame_info);
            if (rc < 0) {
                //if still fail, return error;
                break;
            }
            offset_in_frame = 0;
            //report a log position resync in read log. Some log are missed, due to position resync.
            strncpy(buf,  RESYNC_LOG_POS_MSG, bufsize);
            uint32_t aLen = strlen(RESYNC_LOG_POS_MSG);
            aLen = MIN(bufsize, aLen);
            //update output buffer:
            log_len += aLen;
            buf += aLen;
            bufsize -= aLen;
            //go to the next frame read.
            continue;
        }
        if (rc < 0) {
            ASD_LOG_E(logReader, "read log line failed. rc = %d", rc);
            ASD_LOG_E(logReader, "frame info pos: %lu, log_len %lu",
                    frame_info.start_pos, log_len);
            break;
        }
        //only support one full line per read. The frame length must be the read length.
        assert(rc == frame_info.length);

        uint32_t length = rc + ASD_LOG_LINE_HEADER_SIZE_DELTA;

        //Add "\r\n" for line ending.
        if (bufsize > length) {
            strncpy(buf + length, ASD_LINE_ENDING,
                      bufsize - length);
            length += strlen(ASD_LINE_ENDING);
            length = MIN(length, bufsize);
        }

        char header[ASD_LOG_LINE_HEADER_TXT_SIZE+1];
        rc = asd_logger_decode_line_header(
                 (asd_log_line_header_t*) (buf + ASD_LOG_LINE_HEADER_SIZE_DELTA),
                 header, ASD_LOG_LINE_HEADER_TXT_SIZE+1);
        if (rc < 0) break;
        //copy header back to output buffer.
        memcpy(buf, header, ASD_LOG_LINE_HEADER_TXT_SIZE);
        //update output buffer:
        log_len += length;
        buf += length;
        bufsize -= length;
        //update offset for frame.
        offset_in_frame = frame_info.length;

    }

    if (rc < 0 && rc != ASD_FLASH_BUFFER_ENO_BYTES) {
        //error but not EOF.
        return rc;
    }
    // read success, update frame_info.
    reader->frame_info = frame_info;
    reader->offset_in_frame = offset_in_frame;
    reader->read_len += log_len;

    //return the decoded log line length
    return log_len;

}
