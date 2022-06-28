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
* Implementation of flash manager
*@File: asd_flash_region_mgr.c
********************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include "asd_flash_region_mgr.h"
#include "log_request.h"
#include "asd_logger_if.h"
#include "asd_logger_impl.h"
#include "asd_log_msg.h"
#include "asd_crc32.h"
#include "flash_manager.h"
#include "afw_utils.h"
#include "asd_logger_internal_config.h"



//flash manager for log regions.
struct asd_flash_mgr {
    afw_stream_t stream;
    uint8_t initialized;
    uint32_t valid_log_id_bm;
    struct {
        const flash_region_t* meta;         ///< meta data of the sub-region.
        asd_flash_buffer_t * buffer;        ///< log buffer corresponding to log_id.
        struct fm_flash_partition* flash_partition;
    } log[ASD_LOG_ID_NUM];

};
#define FLASH_REGION_META_INDEX      0
const flash_region_t flash_region_table_default[] = {
    {FLASH_PARTITION_LOG_META,  ASD_LOG_ID_META,     ASD_LOG_DATA_TYPE_BIN},
    ASD_FLASH_LOG_REGIONS_ENTRY_TABLE
};

#define ASD_LOG_REGION_NUM    ARRAY_SIZE(flash_region_table_default)

static void asd_flash_mgr_init_stream(afw_stream_t *stream);
static asd_logger_rc_t asd_flash_region_load_all(asd_flash_mgr_t* mgr);
static void flash_mgr_cleanup(asd_flash_mgr_t* mgr);
static asd_logger_rc_t asd_flash_region_create(asd_flash_mgr_t* mgr);

asd_flash_mgr_t* asd_flash_mgr_init(void)
{
    static asd_flash_mgr_t flashmgr;

    if (asd_flash_region_load_all(&flashmgr) != ASD_LOGGER_OK) {
        LOGGER_DPRINTF("flash region load failed.");
        goto exitonerror;
    }
    asd_flash_mgr_init_stream(&flashmgr.stream);
    flashmgr.initialized = 1;
    return (asd_flash_mgr_t*) &flashmgr;

exitonerror:
    flash_mgr_cleanup(&flashmgr);
    return NULL;
}

afw_stream_t* asd_flash_mgr_get_stream(asd_flash_mgr_t* mgr)
{
    return &mgr->stream;
}

void asd_flash_mgr_deinit(asd_flash_mgr_t* mgr)
{
    if (!mgr || !mgr->initialized) return;
    flash_mgr_cleanup(mgr);
}


int32_t asd_flash_stream_write_frame(afw_stream_t* stream,
                                        const uint8_t* data, uint32_t size)
{
    asd_flash_mgr_t *mgr = (asd_flash_mgr_t*) stream;
    if (!mgr || !mgr->initialized || !data)
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    const asd_log_msg_t * msg = (const asd_log_msg_t*) data;
    if (msg->log_id >= ASD_LOG_ID_NUM || !mgr->log[msg->log_id].buffer)
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    return asd_flash_buffer_write_payload( mgr->log[msg->log_id].buffer,
                              msg->data,
                              GET_LOG_LINE_BIN_LENGTH(msg),
                              true);
}

int32_t asd_flash_read_raw_by_logid(asd_flash_mgr_t* mgr,
                 uint8_t log_id, uint32_t offset, uint8_t* buffer, uint32_t size)
{
    if (!mgr || !mgr->initialized || (mgr->valid_log_id_bm & (1<<log_id)) == 0) {
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    }
    return fm_flash_read(mgr->log[log_id].flash_partition,
            FM_BYPASS_CLIENT_ID,
            offset,
            size,
            buffer);
}

int32_t asd_flash_write_raw_by_logid(asd_flash_mgr_t* mgr,
               uint8_t log_id, uint32_t offset, const uint8_t* buffer, uint32_t size)
{
    if (!mgr || !mgr->initialized || (mgr->valid_log_id_bm & (1<<log_id)) == 0) {
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    }
    return fm_flash_write(mgr->log[log_id].flash_partition,
            FM_BYPASS_CLIENT_ID,
            offset,
            size,
            buffer);
}

asd_flash_buffer_t * asd_flash_mgr_get_buffer_by_log_id(const asd_flash_mgr_t* mgr, uint8_t log_id)
{
    if (!mgr || !mgr->initialized || (mgr->valid_log_id_bm & (1<<log_id)) == 0
        || log_id >= ASD_LOG_ID_NUM) {
        return NULL;
    }
    return mgr->log[log_id].buffer;

}

asd_log_data_type_t asd_flash_mgr_get_buffer_data_type(const asd_flash_mgr_t* mgr, uint8_t log_id)
{
    if (!mgr || !mgr->initialized || (mgr->valid_log_id_bm & (1<<log_id)) == 0
        || log_id >= ASD_LOG_ID_NUM) {
        return ASD_LOG_DATA_TYPE_NUM;
    }
    return mgr->log[log_id].meta->data_type;
}

const char* asd_logger_get_flash_partition_name(uint8_t log_id)
{
    for (int i = 0; i < (int)ASD_LOG_REGION_NUM; i++) {
        if (flash_region_table_default[i].log_id == log_id)
            return flash_region_table_default[i].partition_name;

    }
    return NULL;
}

// ---------------------------------- static functions ----------------------------------------


static int32_t flash_stream_deinit(afw_stream_t *stream)
{
    asd_flash_mgr_deinit((asd_flash_mgr_t*) stream);
    return 0;
}

static afw_stream_table_t flash_mgr_stream_table = {
    &asd_flash_stream_write_frame,
    NULL,
    NULL,
    NULL,
    &flash_stream_deinit,
};

static void asd_flash_mgr_init_stream(afw_stream_t *stream)
{
    stream->table = &flash_mgr_stream_table;
}

static void flash_mgr_cleanup(asd_flash_mgr_t* mgr)
{
    if (!mgr) return;
    mgr->initialized = 0;
    for (int i = 0; i < ASD_LOG_ID_NUM; i++){
        if (mgr->log[i].buffer) {
            asd_flash_buffer_deinit(mgr->log[i].buffer);
            free(mgr->log[i].buffer);
            mgr->log[i].buffer = NULL;
        }
    }

    memset(mgr, 0, sizeof(asd_flash_mgr_t));
    return;
}

static asd_logger_rc_t check_region_meta(asd_flash_mgr_t* mgr,
                                                   flash_region_meta_t* meta)
{
    if (meta->magic_num != ASD_FLASH_META_MAGIC_NUM) return ASD_LOGGER_ERROR;
    if (meta->length != sizeof(flash_region_meta_t)) return ASD_LOGGER_ERROR;
    if (meta->region_num != ASD_LOG_REGION_NUM) return ASD_LOGGER_ERROR;
    if (meta->version != LOG_META_VERSION) return ASD_LOGGER_ERROR;
    uint32_t crc = 0;
    crc = asd_crc32_ietf((const uint8_t*)&meta->magic_num,
                sizeof(flash_region_meta_t) - offsetof(flash_region_meta_t, magic_num));
    return (crc == meta->crc)? ASD_LOGGER_OK : ASD_LOGGER_ERROR_CRC;
}

static asd_logger_rc_t load_region_meta(asd_flash_mgr_t* mgr,
                                          fm_flash_partition_t *flash_partition)
{
    flash_region_meta_t meta;
    if (!flash_partition) return ASD_LOGGER_ERROR_INVALID_PARAMETERS;

    if (fm_flash_read(flash_partition,
                FM_BYPASS_CLIENT_ID,
                0,
                sizeof(meta), (uint8_t*) &meta) != sizeof(meta)) {
        return ASD_LOGGER_ERROR_FLASH;
    }
    //check meta data.
    return check_region_meta(mgr, &meta);
}

static asd_logger_rc_t asd_flash_region_load_or_create_one(
                                             asd_flash_mgr_t* mgr,
                                             const flash_region_t *region,
                                             bool load)
{
    // flash region must have a partition name.
    if (!mgr || !region || !region->partition_name)
        return ASD_LOGGER_ERROR_INVALID_PARAMETERS;
    asd_logger_rc_t rc = ASD_LOGGER_OK;

    fm_flash_partition_t* flash_partition = fm_flash_get_partition(region->partition_name);

    if (!flash_partition) return ASD_LOGGER_ERROR_FLASH_MAP;

    uint32_t log_id = region->log_id;
    switch (log_id) {
    case ASD_LOG_ID_META:
        if (load) {
            rc = load_region_meta(mgr, flash_partition);
        } else {
            //create a new meta region. erase the partition.
            if (fm_flash_erase(flash_partition, FM_BYPASS_CLIENT_ID) < 0) {
                   LOGGER_DPRINTF("Erase partition %s (log id %u) failed.",
                      region->partition_name,
                      region->log_id);

                rc = ASD_LOGGER_ERROR_FLASH;
                break;
            }
            //init meta region.
            flash_region_meta_t region_meta;
            memset(&region_meta, 0xFF, sizeof(flash_region_meta_t));
            region_meta.magic_num = ASD_FLASH_META_MAGIC_NUM;
            region_meta.length = sizeof(flash_region_meta_t);
            region_meta.region_num = ASD_LOG_REGION_NUM;
            region_meta.version = LOG_META_VERSION;
            region_meta.crc = asd_crc32_ietf((const uint8_t*)&region_meta.magic_num,
                        sizeof(flash_region_meta_t) - offsetof(flash_region_meta_t, magic_num));
            if (fm_flash_write(flash_partition,
                              FM_BYPASS_CLIENT_ID,
                              0,
                              region_meta.length,
                              (const uint8_t*) &region_meta) != region_meta.length) {
                LOGGER_DPRINTF("region meta write failed.");
                rc = ASD_LOGGER_ERROR_FLASH;
                break;
            }
        }
        break;

    case ASD_LOG_ID_MAIN:
    case ASD_LOG_ID_DSP:
    case ASD_LOG_ID_METRICS:
    case ASD_LOG_ID_3P:
        //Save in flash buffer as ring buffer. create asd_flash_buffer for it.
        mgr->log[log_id].buffer = (asd_flash_buffer_t*)malloc(sizeof(asd_flash_buffer_t));
        if (!mgr->log[log_id].buffer) return ASD_LOGGER_ERROR_MALLOC;

        rc = asd_flash_buffer_init(mgr->log[log_id].buffer, flash_partition,
                            0, flash_partition->size, true == load);
        break;
    case ASD_LOG_ID_CRASH_LOG:
        // no flash ring buffer, just simple flash partition access.
        // don't do anything here.
        rc = ASD_LOGGER_OK;
        break;

    default:
        rc = ASD_LOGGER_ERROR_INVALID_PARAMETERS;
        break;
    }
    if (rc == ASD_LOGGER_OK) {
        mgr->log[log_id].meta = region;
        mgr->log[log_id].flash_partition = flash_partition;
        mgr->valid_log_id_bm |= 1<<log_id;
    }
    LOGGER_DPRINTF("%s flash region, log ID = %lu, rc = %d",
        load? "load" : "create",
        log_id, rc);
    return rc;
}

static asd_logger_rc_t asd_flash_region_create(asd_flash_mgr_t* mgr)
{
    if ((flash_region_table_default[FLASH_REGION_META_INDEX].log_id != ASD_LOG_ID_META)
        || (!flash_region_table_default[FLASH_REGION_META_INDEX].partition_name)) {
        return ASD_LOGGER_ERROR_FLASH_MAP;
    }

    asd_logger_rc_t rc = ASD_LOGGER_OK;

    for (int id = ASD_LOG_REGION_NUM - 1; id >= FLASH_REGION_META_INDEX; id--) {
        //create one region:
        rc = asd_flash_region_load_or_create_one(mgr,
                                  &flash_region_table_default[id],
                                  false);
        if (rc != ASD_LOGGER_OK) return rc;
    }

    return ASD_LOGGER_OK;
}

static asd_logger_rc_t asd_flash_region_load_all(asd_flash_mgr_t* mgr)
{
    asd_logger_rc_t rc = ASD_LOGGER_OK;
    if (flash_region_table_default[FLASH_REGION_META_INDEX].log_id != ASD_LOG_ID_META) {
        LOGGER_DPRINTF("region meta not in first of Table.");
        return ASD_LOGGER_ERROR_FLASH_MAP;
    }

    if (ASD_LOGGER_OK != asd_flash_region_load_or_create_one(mgr,
                 &flash_region_table_default[FLASH_REGION_META_INDEX], true) ) {
        LOGGER_DPRINTF("Meta region load failed, try create brand-new log regions.");
        //Create new region.
        if ((rc = asd_flash_region_create(mgr)) != ASD_LOGGER_OK){
            LOGGER_DPRINTF("create whole log regions Failed, rc = %d", rc);
            return rc;
        }
    } else {
        // load other regions.
        for (int i = FLASH_REGION_META_INDEX + 1; i < (int) ASD_LOG_REGION_NUM; i++) {
            rc = asd_flash_region_load_or_create_one(mgr,
                                    &flash_region_table_default[i], true);
            if (rc < 0) {
                LOGGER_DPRINTF("flash region %d load failed.", i);
                return rc;
            }
        }
    }

    return rc;
}
