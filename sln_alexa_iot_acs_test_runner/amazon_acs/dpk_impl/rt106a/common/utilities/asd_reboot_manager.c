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
 * @file asd_reboot_manager.c
 *
 * @brief manages platform reboot information
 *******************************************************************************
 */

#include "FreeRTOS.h"
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include "task.h"
#include <sys/types.h>
#include <string.h>
#include "flash_map.h"
#include "flash_manager.h"
#include <ace/hal_kv_storage.h>
#include <asd_crc32.h>
#include "asd_log_platform_api.h"
#include "iot_watchdog.h"
#include "asd_reboot_manager.h"

asd_log_create_module(RebootMgr, ASD_LOG_LEVEL_DEFAULT, ASD_LOG_PLATFORM_STREAM_BM_DEFAULT);

#if defined FLASH_PARTITION_KVS_PERSIST_DATA

typedef struct {
    uint32_t timestamp_s;
    uint32_t uptime_s;
    char reboot_reason[REBOOT_REASON_STRING_LENGTH_MAX + 1];
} asd_reboot_record_t;

typedef struct {
    asd_reboot_record_t reboot_record;
    uint32_t checksum;
} asd_persist_reboot_record_t;

#define PLATFORM_METRIC_DATAPOINTS_NUM_MAX   (3)

/* This timeout value should be less than the watchdog refresh
   period of health manager */
#define ASD_REBOOT_SLEEP_MS (5 * 1000)

#define REBOOT_REASON "reboot_reason"
#define TIMESTAMP_S "timestamp_s"
#define UPTIME_S "uptime_s"
// max length of date string.
#define DATE_STRING_MAX   (32)


#define TAG_ASD_REBOOT_RECORD FLASH_PARTITION_KVS_PERSIST_DATA ".asd_reboot_record"
#define TAG_ASD_REBOOT_RECORD_ELEMENT(index) TAG_ASD_REBOOT_RECORD "_" #index
#define TAG_ASD_REBOOT_RECORD_INDEX TAG_ASD_REBOOT_RECORD "_current"

#define REBOOT_RECORD_INDEX_INVALID (-1)

const char *asd_reboot_record_tag[] = {
    TAG_ASD_REBOOT_RECORD_ELEMENT(0),
    TAG_ASD_REBOOT_RECORD_ELEMENT(1),
    TAG_ASD_REBOOT_RECORD_ELEMENT(2),
    TAG_ASD_REBOOT_RECORD_ELEMENT(3),
    TAG_ASD_REBOOT_RECORD_ELEMENT(4),
    TAG_ASD_REBOOT_RECORD_ELEMENT(5),
    TAG_ASD_REBOOT_RECORD_ELEMENT(6),
    TAG_ASD_REBOOT_RECORD_ELEMENT(7),
    TAG_ASD_REBOOT_RECORD_ELEMENT(8),
    TAG_ASD_REBOOT_RECORD_ELEMENT(9),
};
#define MAX_REBOOT_RECORD (sizeof(asd_reboot_record_tag)/sizeof(asd_reboot_record_tag[0]))

#define GET_NEXT_RECORD_INDEX(cur) (((cur) + 1) % MAX_REBOOT_RECORD)
#define GET_PREV_RECORD_INDEX(cur) ((cur) ? (((cur) - 1) % MAX_REBOOT_RECORD) : (MAX_REBOOT_RECORD - 1))

static bool g_reboot_reason_support = false;
static int g_reboot_record_index = REBOOT_RECORD_INDEX_INVALID;
static bool g_reboot_reason_flag = false;
static warm_boot_storage_iface_t g_wbs_if;

typedef struct {
    const char* reason_string;
    const char* source;
} asd_lcr_entry_t;
static const asd_lcr_entry_t g_lcr_lookup_table[] = {
#define TABLE_ENTRY(a, b) { a, b },
    REBOOT_REASON_VITAL_TABLE
#undef TABLE_ENTRY
};

static const asd_lcr_entry_t* _search_lcr_entry(const char* reboot_reason)
{
    if (reboot_reason == NULL) reboot_reason = REBOOT_REASON_NA;

    for (uint32_t i = 0; i < ARRAY_SIZE(g_lcr_lookup_table); i++) {
        if (!strncmp(reboot_reason,
                     g_lcr_lookup_table[i].reason_string,
                     REBOOT_REASON_STRING_LENGTH_MAX)) {
            // find the entry, let's return it:
            return &g_lcr_lookup_table[i];
        }
    }
    return NULL;
}

// record LCR KDM/Vital
static int asd_LCR_record(const asd_reboot_record_t* reboot_record)
{
    if (!reboot_record) return ASD_REBOOT_MANAGER_STATUS_ERROR_GENERAL;
    int ret;

    const asd_lcr_entry_t* entry = _search_lcr_entry(reboot_record->reboot_reason);
    // unit is "sec", and value is the uptime for the reboot.
    if (entry) {
        ret = PLATFORM_VLOG1(LCR_TAG_PROGRAM, entry->source, LCR_TAG_UNIT,
                      reboot_record->uptime_s, LCR_TAG_KEY, reboot_record->reboot_reason);
    } else {
        ret = PLATFORM_VLOG1(LCR_TAG_PROGRAM, LCR_TAG_CUSTOMIZED, LCR_TAG_UNIT,
                      reboot_record->uptime_s, LCR_TAG_KEY, reboot_record->reboot_reason);
    }
    return ret;
}

static int get_reboot_record_index(void) {
    ace_status_t status;

    if (g_reboot_record_index == REBOOT_RECORD_INDEX_INVALID) {
        status = aceKeyValueDsHal_get(TAG_ASD_REBOOT_RECORD_INDEX, &g_reboot_record_index, sizeof(g_reboot_record_index));
        if (status != sizeof(g_reboot_record_index)) {
            g_reboot_record_index = REBOOT_RECORD_INDEX_INVALID;
        }
    }
    return g_reboot_record_index;
}

static int make_time_string(time_t time, char *buf, size_t buflen)
{
    struct tm time_tm;
    buf[0] = '\0';
    if (time == 0) return -1;

    localtime_r(&time, &time_tm);
    return strftime(buf, buflen, "%Y-%m-%d %H:%M:%S", &time_tm);
}

static void write_reboot_record(asd_reboot_record_t *reboot_record)
{
    ace_status_t status;
    bool reboot_flag = true;
    int record_index = get_reboot_record_index();

    if (record_index == REBOOT_RECORD_INDEX_INVALID) {
        record_index = 0;
    } else {
        record_index = GET_NEXT_RECORD_INDEX(record_index);
    }

    status = aceKeyValueDsHal_set(asd_reboot_record_tag[record_index], reboot_record, sizeof(*reboot_record));
    if (status != ACE_STATUS_OK) {
        ASD_LOG_E(RebootMgr, "failed to write reboot record: %d", status);
        return;
    }

    g_reboot_record_index = record_index;
    status = aceKeyValueDsHal_set(TAG_ASD_REBOOT_RECORD_INDEX, &record_index, sizeof(record_index));
    if (status != ACE_STATUS_OK) {
        ASD_LOG_E(RebootMgr, "failed to write next record index: %d", status);
    }
}

void asd_reboot_manager_add_reason(const char* reboot_reason)
{
    struct timeval tv;
    asd_persist_reboot_record_t record;
    TickType_t tick_count;

    if(!g_reboot_reason_support) {
        return;
    }

    /* Check and ignore if the reboot reason flag is already set */
    if (__atomic_test_and_set(&g_reboot_reason_flag, __ATOMIC_SEQ_CST)) {
        return;
    }

    bzero(&record, sizeof(record));

    gettimeofday(&tv, NULL);
    record.reboot_record.timestamp_s = tv.tv_sec;
    if (pdTRUE == xPortIsInsideInterrupt()) {
        tick_count = xTaskGetTickCountFromISR();
    } else {
        tick_count = xTaskGetTickCount();
    }
    record.reboot_record.uptime_s = tick_count / configTICK_RATE_HZ;
    strlcpy(record.reboot_record.reboot_reason, reboot_reason, REBOOT_REASON_STRING_LENGTH_MAX);

    record.checksum = asd_crc32_ietf((uint8_t*)&record.reboot_record,
                                    sizeof(asd_reboot_record_t));

    if(g_wbs_if.write_cb) {
        if(!g_wbs_if.write_cb(g_wbs_if.block_id, 0, (char *)&record, sizeof(record))) {
            printf("RebootMgr: failed to write warm boot storage\n");
        }
    }
}

void asd_reboot_manager_reboot(IotResetBootFlag_t ucColdBootFlag, const char* reboot_reason)
{
    IotWatchdogHandle_t hm_wdt;
    static bool reboot_in_progress = false;

    // Check and sleep if called in task context while reboot call in progress
    if (true == __atomic_test_and_set(&reboot_in_progress, __ATOMIC_SEQ_CST)) {
         printf("RebootMgr: Reboot in progress. Sleeping\n");
         vTaskDelay(pdMS_TO_TICKS(ASD_REBOOT_SLEEP_MS));
    }
    vTaskSuspendAll();
    asd_reboot_manager_add_reason(reboot_reason);
    iot_reset_reboot(ucColdBootFlag);
}

asd_reboot_manager_status_t asd_reboot_manager_init(const warm_boot_storage_iface_t *wbs_if)
{
   /* Initialize reboot manager. This function reads reset reason from watchdog
    * timer and initilizes flash based on reset reason and flash boot flag.
    * Note: This function is not thread safe.
    */
    IotResetReason_t wdt_reset_reason;
    bool reboot_flag = false;
    uint32_t checksum;
    asd_persist_reboot_record_t record;
    char date[DATE_STRING_MAX];

    if (wbs_if) {
        g_wbs_if = *wbs_if;
    } else {
        bzero(&g_wbs_if, sizeof(g_wbs_if));
    }

    if(!fm_flash_get_partition(FLASH_PARTITION_KVS_PERSIST_DATA)) {
        ASD_LOG_E(RebootMgr, "FLASH_PARTITION_KVS_PERSIST_DATA does not exist");
        return ASD_REBOOT_MANAGER_STATUS_ERROR_NOT_SUPPORTED;
    }
    g_reboot_reason_support = true;

    if (g_wbs_if.read_cb && g_wbs_if.read_cb(g_wbs_if.block_id, 0, (char *)&record, sizeof(record))) {
        checksum = asd_crc32_ietf((uint8_t*)&record.reboot_record,
                                sizeof(asd_reboot_record_t));
        reboot_flag = (checksum == record.checksum);
    }

    if (g_wbs_if.clear_cb) {
        g_wbs_if.clear_cb(g_wbs_if.block_id, 0, sizeof(record));
    }

    /* The reboot reason check logic is as follows:
       (1) If reboot_reason flag is set then the reboot reason is already recorded
       (2) If the reboot_reason flag is not set then read the reset reason from watchdog
       (3) Write reboot reason to flash
     */
    if (!reboot_flag) {
        bzero(&record, sizeof(record));
        if (iot_get_reset_reason(&wdt_reset_reason) != IOT_RESET_SUCCESS) {
            ASD_LOG_E(RebootMgr, "Failed to read reset reason");
            return ASD_REBOOT_MANAGER_STATUS_ERROR_READ_FAILED;
        }

        switch(wdt_reset_reason) {
        case eResetWatchdog:
            strlcpy(record.reboot_record.reboot_reason, REBOOT_REASON_WATCHDOG_TIMEOUT,
                                                 REBOOT_REASON_STRING_LENGTH_MAX);
            break;
        case eResetWarmBoot:
            strlcpy(record.reboot_record.reboot_reason, REBOOT_REASON_UNKNOWN_SW_REBOOT,
                                                 REBOOT_REASON_STRING_LENGTH_MAX);
            break;
        default:
            strlcpy(record.reboot_record.reboot_reason, REBOOT_REASON_POWER_LOSS,
                                                 REBOOT_REASON_STRING_LENGTH_MAX);
            break;
        }
    }

    write_reboot_record(&record.reboot_record);

    make_time_string((time_t)record.reboot_record.timestamp_s, date, sizeof(date));
    ASD_LOG_I(RebootMgr, "Reason: %s, uptime: %lu sec, time: %.*s (%lu)",
                            record.reboot_record.reboot_reason,
                            record.reboot_record.uptime_s,
                            sizeof(date), date,
                            record.reboot_record.timestamp_s);

    return ASD_REBOOT_MANAGER_STATUS_OK;
}

/* Publish last boot reason if available */
void asd_reboot_manager_publish(void)
{
    asd_reboot_record_t reboot_record = {0};
    int status;
    int num_dp = 0;
    aceMetricHal_pmetDatapoint_t datapoints[PLATFORM_METRIC_DATAPOINTS_NUM_MAX];
    int record_index = get_reboot_record_index();

    if(!g_reboot_reason_support) {
        return;
    }

    if (record_index == REBOOT_RECORD_INDEX_INVALID) {
        ASD_LOG_E(RebootMgr, "Invalid reboot record index");
        return;
    }

    status = aceKeyValueDsHal_get(asd_reboot_record_tag[record_index], &reboot_record, sizeof(reboot_record));
    if (status != sizeof(reboot_record)) {
        ASD_LOG_E(RebootMgr, "Can't read the reboot record: %d", status);
        return;
    }

    if (0 != asd_LCR_record(&reboot_record)) {
        ASD_LOG_E(RebootMgr, "Fail to record LCR kdm: %.*s",
                        sizeof(reboot_record.reboot_reason),
                        reboot_record.reboot_reason);
    }

    datapoints[num_dp++] = (aceMetricHal_pmetDatapoint_t) PPMET_DATAPOINT_INIT_STRING(REBOOT_REASON, reboot_record.reboot_reason, 1);
    datapoints[num_dp++] = (aceMetricHal_pmetDatapoint_t) PPMET_DATAPOINT_INIT_TIMER(TIMESTAMP_S, reboot_record.timestamp_s, 1);
    datapoints[num_dp++] = (aceMetricHal_pmetDatapoint_t) PPMET_DATAPOINT_INIT_TIMER(UPTIME_S, reboot_record.uptime_s, 1);

    status = PLATFORM_MLOG("asd", "reboot_manager", num_dp, datapoints, ACE_METRIC_HAL_PRIORITY_NORMAL);
    if (AFW_OK != status) {
        ASD_LOG_E(RebootMgr, "failed to record pmet: %d", status);
    }
}

void asd_reboot_manager_dump(void)
{
    unsigned int i;
    int status;
    asd_reboot_record_t reboot_record = {0};
    int current_index = get_reboot_record_index();

    if(!g_reboot_reason_support) {
        return;
    }

    if (current_index == REBOOT_RECORD_INDEX_INVALID) {
        ASD_LOG_E(RebootMgr, "Invalid reboot record index for dump");
        return;
    }

    char date[DATE_STRING_MAX];

    ASD_LOG_I(RebootMgr, "Reboot history");
    ASD_LOG_I(RebootMgr, "   %-32s %-12s %-16s %s", "reason", "uptime", "timestamp", "time");
    for (i = 0; i < MAX_REBOOT_RECORD; i++) {

        status = aceKeyValueDsHal_get(asd_reboot_record_tag[current_index], &reboot_record, sizeof(reboot_record));
        if (status != sizeof(reboot_record)) {
            // end of list
            break;
        }

        if (strlen(reboot_record.reboot_reason) == 0) {
            /* record already cleared */
            ASD_LOG_E(RebootMgr, "%u empty reboot reason", i);
            break;
        }
        make_time_string((time_t)reboot_record.timestamp_s, date, sizeof(date));

        ASD_LOG_I(RebootMgr, "%-2u %-32s %-12lu %-16lu %.*s",
                             i,
                             reboot_record.reboot_reason,
                             reboot_record.uptime_s,
                             reboot_record.timestamp_s,
                             sizeof(date), date);

        current_index = GET_PREV_RECORD_INDEX(current_index);
    }
}

#else // defined FLASH_PARTITION_KVS_PERSIST_DATA

asd_reboot_manager_status_t asd_reboot_manager_init(const warm_boot_storage_iface_t *wbs_if)
{
    ASD_LOG_E(RebootMgr, "reboot reason not supported");
    return ASD_REBOOT_MANAGER_STATUS_ERROR_NOT_SUPPORTED;
}

void asd_reboot_manager_reboot(IotResetBootFlag_t ucColdBootFlag, const char* reboot_reason)
{
    iot_reset_reboot(ucColdBootFlag);
}

void asd_reboot_manager_publish(void)
{
    return;
}

void asd_reboot_manager_dump(void)
{
    ASD_LOG_E(RebootMgr, "reboot reason not supported");
    return;
}

#endif // defined FLASH_PARTITION_KVS_PERSIST_DATA
