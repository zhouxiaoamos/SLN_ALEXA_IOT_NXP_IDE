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
/*
 * @File: asd_logger_cli.c
 */

/*
 * Purpose: Command line options for logging
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ace/aceCli.h>
#include "asd_log_api.h"
#include "asd_logger_if.h"
#include "asd_logger_cli.h"
#include "asd_logger_impl.h"
#include "asd_log_eraser.h"
#include "asd_log_platform_api.h"
#include "asd_flash_region_mgr.h"
#include "common_macros.h"
#include "asd_debug.h"
#include "flash_manager.h"
#include "time_utils.h"

asd_log_create_module(logger_test, ASD_LOG_LEVEL_DEBUG, ASD_LOG_PLATFORM_STREAM_BM_DEFAULT);

#define LOGID_HELP "log_id: 1-DSP, 2-MAIN (default), 3-VITAL, 4-CrashLog"

#define LOG_CLI_FILTER_GET_USAGE_TXT  \
    "Query global or individual module filter setting.\n" \
    "get [module_name]\n"  \
    "    module_name: module name. \"global\" is for global setting;\n"  \
    "                 if module_name is omitted, all module filter settings are printed.\n"

#define LOG_CLI_FILTER_SET_USAGE_TXT  \
    "set global or individual module filter setting.\n"  \
    "set module_name level [stream_bm]\n"  \
    "    module_name: module name. \"global\" is for global setting;\n"  \
    "                            * : for all log modules\n" \
    "                 call \"log filter get\" to list available log modules.\n"  \
    "    level:       log level. D:Debug, I:Info, W:Warnning, E:Error, F:Fatal.\n" \
    "    stream_bm:   output stream bitmap, 1: console, 2: flash, 3: console and flash.\n"

static const char level_char[] = {'D', 'I', 'W', 'E', 'F'};

static ace_status_t cli_log_upload(int32_t len, const char *param[])
{
    ASD_LOG_I(logger_test, "Under construction...");
    return ACE_STATUS_OK;
}

static ace_status_t cli_log_dump(int32_t len, const char *param[])
{
    #define LOG_BUF_SIZE    1024
    log_reader_origin_t origin = LOG_POSITION_START;
    asd_log_id_t id = ASD_LOG_ID_MAIN;
    if (len > 2) return ACE_STATUS_BAD_PARAM;

    if (len == 2) {
        //read or unread.
        origin = (atoi(param[1]) == 0)? LOG_POSITION_START : LOG_POSITION_UNREAD;
    }

    if (len >= 1) {
        id = atoi(param[0]);
        if (id>=ASD_LOG_ID_NUM || id < ASD_LOG_ID_DSP) {
            return ACE_STATUS_BAD_PARAM;
        }
    }

    asd_log_reader_t* reader = log_reader_create(id, LOG_DATA_TYPE_BIT_TXT);
    if (!reader) {
        printf("error to create a log reader\r\n");
        return ACE_STATUS_OK;
    }
    uint8_t* buf = (uint8_t*)malloc(LOG_BUF_SIZE);
    if (!buf) {
        log_reader_destroy(reader);
        printf("Fail to malloc buffer for log dump.\r\n");
        return ACE_STATUS_OK;
    }
    int32_t rc = 0;

    rc = log_reader_setpos(reader, origin);
    if (rc < 0) {
        log_reader_destroy(reader);
        free(buf);
        printf("No available log (id = %u), Fail to set log start position. rc = %ld\r\n", id, rc);
        return ACE_STATUS_OK;
    }
    int32_t log_size = log_reader_get_remaining_log_length(reader);

    printf("\n============== dump log (id = %u) length =%ld ==============\n", id, log_size);
    log_size = 0;

    while(1) {
        rc = log_reader_read(reader, buf, LOG_BUF_SIZE);
        if (rc <=0) break;
        printf("%.*s", (int)rc, buf);
        log_size += rc;
    }

    log_reader_destroy(reader);
    free(buf);
    printf("\r\n============== log (id = %u) total size %ld bytes ==============\n", id, log_size);
    printf("rc = %ld\r\n", rc);
    return ACE_STATUS_OK;
}

static int convert_char_to_level(char level_ch)
{
    for (int level = 0; level < (int)sizeof(level_char); level++) {
        if (level_char[level] == level_ch) {
            return level;
        }
    }
    return -1;
}

static char convert_level_to_char(uint8_t level)
{
    if (level >= sizeof(level_char)) return 'U';
    return level_char[level];
}

static ace_status_t cli_log_module_filter_set(int32_t len, const char *param[])
{
    asd_log_control_block_t filter = {0};
    filter.level = ASD_LOG_LEVEL_NUM;
    filter.ostream_bm = 0xFF;
    int32_t rc = 0;
    const char* module_name = NULL;

    // cmd example:
    //  param        0    1 2
    // filter set ace_hal D 3
    // filter set ace_hal W

    // check parameter num.
    if (len < 2 || len > 3) return ACE_STATUS_BAD_PARAM;
    if (strcasecmp("global", param[0])) {
        // not global filter.
        module_name = param[0];
    }

    if (len == 3) {
        // set output stream bitmap.
        filter.ostream_bm = strtoul(param[2], NULL, 0);
    }

    do {
        int lvl = convert_char_to_level(*param[1]);
        if (lvl < 0) return ACE_STATUS_BAD_PARAM;
        filter.module_name = module_name;
        filter.level = lvl;
        rc = asd_logger_set_log_filter(asd_logger_get_handle(), module_name, &filter);
        if (rc < 0) break;
        // set all modules *
        if (module_name && !strcmp(module_name, "*")) break;

        asd_log_control_block_t filter_read;
        rc = asd_logger_get_log_filter(asd_logger_get_handle(), module_name, &filter_read);

        if (rc < 0) break;
        if (filter_read.level != filter.level){
            printf("Error: Level set %d, but read back %d\r\n",
                filter.level, filter_read.level);
        }
    } while (0);

    if (rc < 0) {
        printf("Failed (rc = %ld) to set module %s filter, %c ",
                            rc,
                            param[0],
                            convert_level_to_char(filter.level));

    } else {
        printf("Succeed to set module filter:\n%-24s%c\t",
                            param[0],
                            convert_level_to_char(filter.level));

    }
    if (filter.ostream_bm != 0xFF) {
        printf("0x%x\n", filter.ostream_bm);
    } else {
        printf("\n");
    }

    return ACE_STATUS_OK;
}

static ace_status_t cli_log_module_filter_get(int32_t len, const char *param[])
{
    if (len == 0) {
        //dump all module filter
        asd_logger_dump_module_filters(asd_logger_get_handle());
        return ACE_STATUS_OK;
    }

    asd_log_control_block_t filter = {0};
    filter.level = ASD_LOG_LEVEL_NUM;
    filter.ostream_bm = 0xFF;
    int32_t rc = 0;
    const char* module_name = NULL;

    if (len != 1) return ACE_STATUS_BAD_PARAM;

    if (strcasecmp("global", param[0])) {
        // not global filter.
        module_name = param[0];
    }

    // cmd example:
    // param         0
    // filter get ace_hal

    //query the log filter setting.
    rc = asd_logger_get_log_filter(asd_logger_get_handle(), module_name, &filter);

    if (rc < 0) {
        printf("Failed to get module %s filter, (%c, %d), rc = %ld\n", param[0],
                            convert_level_to_char(filter.level),
                            filter.ostream_bm, rc);
    } else {
        printf("Succeed to get module filter:\n%-24s%c\t%d\n", param[0],
                            convert_level_to_char(filter.level),
                            filter.ostream_bm);
    }

    return ACE_STATUS_OK;
}



static ace_status_t cli_log_wipe(int32_t len, const char *param[])
{

    if (len != 1) {
        printf("log wipe [log_id]\n");
        return ACE_STATUS_BAD_PARAM;
    }

    int log_id = atoi(param[0]);
    if (log_id >= ASD_LOG_ID_NUM || log_id < ASD_LOG_ID_DSP) {
        printf("log ID is wrong %d\n", log_id);
        return ACE_STATUS_BAD_PARAM;
    }

    int rc = asd_logger_erase_flash_log(log_id, portMAX_DELAY);
    if (rc < 0) {
        printf("Failed to erase log (log id = %d), rc = %d.\n", log_id, rc);
    } else {
        printf("All log is wiped for log id = %d.\n", log_id);
    }

    return ACE_STATUS_OK;
}

static ace_status_t cli_log_wipe_expired(int32_t len, const char *param[]) {

    if (len != 1) {
        printf("log wipe_expired [seconds before now]\n");
        return ACE_STATUS_BAD_PARAM;
    }

    int seconds = atoi(param[0]);
    if (seconds < 0) {
        printf("Seconds must be greater than 0 (%d)\n", seconds);
        return ACE_STATUS_BAD_PARAM;
    }

    if (!clock_isset()) {
        printf("Clock time is not set correctly. Try rebooting or reconnecting to Wifi.\n");
        return ACE_STATUS_BAD_PARAM;
    }

    uint64_t expiration_timestamp_ms = clock_gettime_ms();
    expiration_timestamp_ms -= SEC_TO_MSEC((unsigned long)seconds);
    asd_logger_erase_expired_flash_log(ASD_LOG_ID_MAIN, portMAX_DELAY, expiration_timestamp_ms);

    printf("Expiration Timestamp: %lu\n", (uint32_t)MSEC_TO_SEC(expiration_timestamp_ms));
    printf("Async expiration clean up started. Use 'log dump' to verify log state.\n");
    return ACE_STATUS_OK;
}


static ace_status_t cli_dump_flash(int32_t len, const char *param[])
{

    if (len != 3) return ACE_STATUS_BAD_PARAM;

    #define DUMP_ONE_SIZE  256
    uint32_t log_id = strtoul(param[0], NULL, 0);
    uint32_t offset = strtoul(param[1], NULL, 0);
    int size = strtoul(param[2], NULL, 0);
    const char* pname = asd_logger_get_flash_partition_name(log_id);
    if (!pname) {
        printf("can't get partition name!!\n");
        return ACE_STATUS_OK;
    }

    struct fm_flash_partition *partition = fm_flash_get_partition(pname);
    uint8_t data[DUMP_ONE_SIZE];
    int rc =0;

    if (!partition) {
        printf("Fail to get the log partition.");
        return ACE_STATUS_OK;
    }

    while (size) {
        int read_len = MIN(size, DUMP_ONE_SIZE);
        rc = fm_flash_read(partition,
                      FM_BYPASS_CLIENT_ID,
                      offset,
                      read_len,
                      data);
        if (rc != read_len) {
            printf("\r\n Flash read error %d\r\n", rc);
        }

        //print to console.
        memory_dump_hex_ascii("Flash dump", data, read_len);

        size -= read_len;
        offset += read_len;
    }
    printf("\r\n");
    return ACE_STATUS_OK;
}

static ace_status_t cli_query_flash_log_size(int32_t len, const char *param[])
{
    log_reader_origin_t origin = LOG_POSITION_START;
    asd_log_id_t id = ASD_LOG_ID_MAIN;
    if (len > 2) return ACE_STATUS_BAD_PARAM;

    if (len == 2) {
        //read or unread.
        origin = (atoi(param[1]) == 0)? LOG_POSITION_START : LOG_POSITION_UNREAD;
    }

    if (len >= 1) {
        id = atoi(param[0]);
        if (id>=ASD_LOG_ID_NUM || id < ASD_LOG_ID_DSP) {
            return ACE_STATUS_BAD_PARAM;
        }
    }

    asd_log_reader_t* reader = log_reader_create(id, LOG_DATA_TYPE_BIT_TXT);

    int32_t rc = 0;

    if (!reader) {
        printf("Fail to create reader.\n");
        return ACE_STATUS_OUT_OF_MEMORY;
    }


    rc = log_reader_setpos(reader, origin);

    if (rc < 0) {
        log_reader_destroy(reader);
        printf("No log(id=%u) available in flash. rc = %ld\n",id, rc);
        return ACE_STATUS_OK;
    }

    rc = log_reader_get_remaining_log_length(reader);

    if (rc < 0) {
        printf("Fail to query log size from flash. rc =%ld\n", rc);
    } else {
        printf("Log size in flash: %ld bytes\n", rc);
    }
    log_reader_destroy(reader);
    return ACE_STATUS_OK;
}

static ace_status_t cli_mark_read_flag(int32_t len, const char *param[])
{
    asd_log_id_t id = ASD_LOG_ID_MAIN;
    if (len > 1) return ACE_STATUS_BAD_PARAM;

    if (len == 1) {
        id = atoi(param[0]);
        if (id>=ASD_LOG_ID_NUM || id < ASD_LOG_ID_DSP) {
            return ACE_STATUS_BAD_PARAM;
        }
    }

    asd_log_reader_t* reader = log_reader_create(id, LOG_DATA_TYPE_BIT_TXT);
    int32_t rc = 0;

    if (!reader) {
        printf("Failed to create reader.\n");
        return ACE_STATUS_OK;
    }

    rc = log_reader_setpos(reader, LOG_POSITION_END);
    if (rc < 0) {
        log_reader_destroy(reader);
        printf("Fail to set log end position. rc= %ld\r\n", rc);
        return ACE_STATUS_OK;
    }

    rc = log_reader_mark_read_flag(reader);

    if (rc < 0) {
        log_reader_destroy(reader);
        printf("Fail to mark read flag. rc = %ld\r\n", rc);
        return ACE_STATUS_OK;
    }
    log_reader_destroy(reader);

    printf("Succeed to mark read flag.\r\n");

    return ACE_STATUS_OK;

}

static ace_status_t cli_log_status(int32_t len, const char *param[])
{
    printf("\tdropped log lines: %ld\n", asd_logger_get_log_drops(asd_logger_get_handle()));
    asd_flash_buffer_t *fbuf = asd_logger_get_flash_buffer(
                      (asd_logger_t*) asd_logger_get_handle(), ASD_LOG_ID_MAIN);
    printf("\tflash buffer:\n");
    printf("\ttail (%lu, %lu, %d)\n", fbuf->tail.index, fbuf->tail.start_pos,
        (int)fbuf->tail.length);
    printf("\tunread_tail (%lu, %lu, %d)\n", fbuf->unread_tail.index, fbuf->unread_tail.start_pos,
        (int)fbuf->unread_tail.length);
    printf("\thead (%lu, %lu, %d)\n", fbuf->head.index, fbuf->head.start_pos,
        (int)fbuf->head.w_length);

    return ACE_STATUS_OK;
}

//sub-cli for log filter.
const aceCli_moduleCmd_t log_filter_cli[] = {
    { "get", LOG_CLI_FILTER_GET_USAGE_TXT, ACE_CLI_SET_LEAF, .command.func=&cli_log_module_filter_get},
    { "set", LOG_CLI_FILTER_SET_USAGE_TXT, ACE_CLI_SET_LEAF, .command.func=&cli_log_module_filter_set},
    ACE_CLI_NULL_MODULE
};
//cli for log
const aceCli_moduleCmd_t log_cli[] = {
    // upload log to cloud.
    { "upload", "Uploads logs", ACE_CLI_SET_LEAF, .command.func=&cli_log_upload},
    // dump log to console
    { "dump",   " Dumps logs,\n\t dump [log_id] [0/1]\n\t "
                LOGID_HELP
                "\n\t 1: dump unread log only, 0(default): regardless of read/unread",  ACE_CLI_SET_LEAF, .command.func=&cli_log_dump},
    { "wipe",   " Wipes all log for log_id.\n\t wipe [log_id]\n\t " LOGID_HELP,  ACE_CLI_SET_LEAF, .command.func=&cli_log_wipe},
    { "wipe_expired",   " Wipes main logs older than x seconds from current time_stamp.\n\t wipe_expired [seconds before now]\n\t ",
                ACE_CLI_SET_LEAF, .command.func=&cli_log_wipe_expired},
    { "df",     " dump flash data raw from log region.\n\t df log_id offset size\n\t " LOGID_HELP,  ACE_CLI_SET_LEAF, .command.func=&cli_dump_flash},
    { "size",   " Query size of log in flash\n\t size [log_id] [0/1]\n\t "
                LOGID_HELP
                "\n\t 1: count unread length, 0(default): regardless of read/unread",  ACE_CLI_SET_LEAF, .command.func=&cli_query_flash_log_size},
    { "mark",   "Mark all log as read.\n\t mark [log_id]\n\t "
                LOGID_HELP, ACE_CLI_SET_LEAF, .command.func=&cli_mark_read_flag},
    { "status", "print dropped log number",  ACE_CLI_SET_LEAF, .command.func=&cli_log_status},
    { "filter", "log filter settings",  ACE_CLI_SET_FUNC, .command.subCommands=log_filter_cli},


    ACE_CLI_NULL_MODULE
};
