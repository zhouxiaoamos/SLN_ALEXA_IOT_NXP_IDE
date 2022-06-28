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
 * @File: asd_debug.c
 *
 *
 */

#include <FreeRTOS.h>
#include <assert.h>
#include <string.h>
#include <common_macros.h>
#include <task.h>
#include <inttypes.h>
#include "asd_crashdump.h"
#include "iot_reset.h"
#include <time.h>
#include "mbedtls/sha1.h"
#include "crc16.h"
#include "flash_map.h"
#include "flash_manager.h"
#include "asd_log_platform_api.h"
#include "afw_error.h"

#define ASD_CD_MAGIC_NUM      0x504d554445524f43    // reverse of text "COREDUMP"

#define SHA1_HASHOUT_SIZE                    (20)
#define TO_BASE16(c) ((c) < 10 ? (c) + '0' : (c) - 10 + 'A')

#define DUMP_STACK_DWORD_PER_LINE            (8)
#define IS_DUMP_LINE_START(idx)              ((idx)%DUMP_STACK_DWORD_PER_LINE == 0)
#define IS_DUMP_LINE_END(idx, n)             (((idx+1)%DUMP_STACK_DWORD_PER_LINE == 0) || \
                                                (idx == n -1))
#define NOT_IN_EXCLUSIVE_RANGE(x, lower, upper)  ((x) <= (lower) || (x) >= (upper))

#define CD_SCRATCH_PAD_LENGTH (256)
#define VALIDATE_PRINT_IN_SCRATCH_PAD(print_bytes)                       \
    if (NOT_IN_EXCLUSIVE_RANGE(print_bytes, 0, CD_SCRATCH_PAD_LENGTH)) { \
        return -AFW_ENOMEM;                                              \
    }

enum {
    CD_FLASH_UNINITIALIZED = 0,
    CD_FLASH_INITIALIZED = 1,
    CD_FLASH_EXIST   = 0x2,
    CD_FLASH_CACHED  = 0x4,
};

#define CRASHDUMP_HEADER_INIT(_pheader, _crc, _len)  { \
        (_pheader)->magic_num = ASD_CD_MAGIC_NUM;      \
        (_pheader)->crc = _crc;                        \
        (_pheader)->version = ASD_CD_VERSION;          \
        (_pheader)->type = ASD_CD_TYPE_MINIDUMP;       \
        (_pheader)->length = _len;                     \
    }


#define CRASHLOG_HEADER_INIT(_pheader, _len)   {       \
        (_pheader)->magic_num = ASD_CD_MAGIC_NUM;      \
        (_pheader)->crc = 0xFF;                        \
        (_pheader)->version = ASD_CD_VERSION;          \
        (_pheader)->type = ASD_CD_TYPE_LOG;            \
        (_pheader)->length = _len;                     \
    }


typedef struct {
    struct fm_flash_partition* partition; ///< log partition.
    uint8_t status;                       ///< status bits
    uint32_t total_frame_length;              ///< cached length of all frames in text format.
    asd_crashdump_mini_t mini_dump;       ///< cached mini_dump in binary.
    uint32_t log_len;                     ///< crash log length; log is not crc protected.
} asd_crashdump_config_t;

const char* asd_crash_dump_reason_txt_list[] = {
    #define TABLE_ENTRY(a, b, c)  c
    ASD_CRASH_DUMP_TABLE
    #undef  TABLE_ENTRY
};
const char* asd_crash_dump_metrics_key_list[] = {
    #define TABLE_ENTRY(a, b, c)  b
    ASD_CRASH_DUMP_TABLE
    #undef  TABLE_ENTRY
};

const char* cpu_regs_name[] = {
    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "sp",              /* after pop r0-r3, lr, pc, xpsr                   */
    "lr",              /* lr before exception                             */
    "pc",              /* pc before exception                             */
    "psr",             /* xpsr before exeption                            */
    "control",         /* nPRIV bit & FPCA bit meaningful, SPSEL bit = 0  */
    "exc_ret",         /* current lr                                      */
    "msp",             /* msp                                             */
    "psp",             /* psp                                             */
    "fpscr",
    "hfsr",
    "cfsr",
    "mmfar",
    "bfar",
};

asd_log_create_module(crashdump, ASD_LOG_LEVEL_DEFAULT, ASD_LOG_PLATFORM_STREAM_BM_DEFAULT);

static asd_crashdump_config_t cd_config = {0};
char g_scratchpad[CD_SCRATCH_PAD_LENGTH];
static app_version_info_callback* g_version_info_cb = NULL;

static int32_t cd_flash_read(asd_crashdump_config_t* cd, uint32_t offset, uint32_t size, void* buf)
{
    if (!cd->partition) {
        cd->partition = fm_flash_get_partition(FLASH_PARTITION_LOG_CRASH);
    }
    if (!cd->partition) return -AFW_ENODEV;

    return fm_flash_read(cd->partition,
                          FM_BYPASS_CLIENT_ID,
                          offset,
                          size,
                          (uint8_t*) buf);
}

static int32_t cd_flash_write(asd_crashdump_config_t* cd, uint32_t offset, uint32_t size, const void* buf)
{
    if (!cd->partition) {
        cd->partition = fm_flash_get_partition(FLASH_PARTITION_LOG_CRASH);
    }
    if (!cd->partition) return -AFW_ENODEV;

    return fm_flash_write(cd->partition,
                          FM_BYPASS_CLIENT_ID,
                          offset,
                          size,
                          (const uint8_t*) buf);
}

static int32_t cd_flash_erase(asd_crashdump_config_t* cd, uint32_t offset, uint32_t size)
{
    if (!cd->partition) {
        cd->partition = fm_flash_get_partition(FLASH_PARTITION_LOG_CRASH);
    }
    if (!cd->partition) return -AFW_ENODEV;

    return fm_flash_erase_sectors(cd->partition,
                          FM_BYPASS_CLIENT_ID,
                          offset,
                          size);
}

static int make_msg_time(time_t *time, char *buf, size_t buflen)
{
    struct tm time_tm;
    localtime_r(time, &time_tm);
    //UTC time yyMMdd:HHmmss
    return strftime(buf, buflen, "%y%m%d:%H%M%S", &time_tm);
}

//TODO link up to country of residence
static const char *crash_report_country_of_residence(void)
{
    return "US";
}


//TODO Implement crash duplicate count support
static int crash_report_duplicate_count(void)
{
    return 1;
}

static const char *crash_report_marketplace_id(void)
{
    return "1";
}

static bool flash_data_is_all_FF(asd_crashdump_config_t *cd, uint32_t offset, uint32_t size)
{
    uint32_t dw = 0xFFFFFFFF;
    for(uint32_t i = 0; i < size; i += sizeof(dw)) {
        int32_t len;
        len = (size-i > sizeof(dw))? sizeof(dw) : (size-i);
        if (cd_flash_read(cd, offset + i, len, &dw) != len
            || dw != 0xFFFFFFFF) {
            return false;
        }
    }
    return true;
}

static int32_t asd_crashdump_check_and_erase(void)
{
    if(!flash_data_is_all_FF(&cd_config, 0, CRASHDUMP_MINI_TOTAL_SIZE)) {
        //erase the crash dump region.
        return asd_crashdump_erase();
    }
    return 0;
}

static int32_t asd_crashdump_get_frame_length(asd_frame_info_t* frame_info)
{
    return asd_crashdump_read_frame(frame_info, 0, NULL, 0);
}

static uint32_t append_data(char* dst, uint32_t size, const char* src, uint32_t src_len,
                           int32_t offset)
{
    if (!dst || !src || (offset >= (int)src_len) || (offset < 0))
        return 0;

    size = MIN(size, src_len - offset);

    memcpy(dst, src + offset, size);
    return size;
}

static int get_crashdump_hash_string(
              const asd_crashdump_mini_t *dump,
              char *hash_string,
              size_t size_hash_string)
{
    assert(size_hash_string >= SHA1_HASHOUT_SIZE * 2);
    unsigned char hashbin[SHA1_HASHOUT_SIZE];
    if (dump->reason == ASD_CD_REASON_ASSERT) {
        // calculate hash code based on assert meta data.
        mbedtls_sha1((unsigned char*) &dump->assertion, sizeof(dump->assertion), hashbin);
    } else {
        mbedtls_sha1((unsigned char*) dump->call_stacks, dump->call_stack_depth * sizeof(uint32_t), hashbin);
    }
    for(int index = 0; index < SHA1_HASHOUT_SIZE; index++) {
        hash_string[index * 2] = TO_BASE16(hashbin[index] >> 4);
        hash_string[index * 2 + 1] = TO_BASE16(hashbin[index] & 0xF);
    }
    if (size_hash_string > SHA1_HASHOUT_SIZE*2)
        hash_string[SHA1_HASHOUT_SIZE*2] = 0;
    return SHA1_HASHOUT_SIZE * 2;
}

static int32_t asd_crashdump_mini_decode(
    const asd_crashdump_mini_t* minidump,
    const asd_frame_info_t* frame_info,
    uint32_t offset,
    char *buf,
    size_t size)
{
    // length of decoded text in total. including frame_info before offset.
    int decoded_len = 0;
    // length of one print
    int bytes_per_print = 0;
    // length of output text;
    int output_len = 0;
    // output of each snprintf must be less CD_SCRATCH_PAD_LENGTH.
    time_t time_sec = (time_t) minidump->time;
    char *scratchpad = g_scratchpad;

    switch(frame_info->index) {
    case ASD_CD_FRAME_CRASH_META:
    {
        for (int i = 0; i < 8; i++) {
            switch (i) {
            case 0:
                bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH,
                        "Process:\n"
                        "CrashTimeUTC:%u000\n"
                        "CrashType:%s\n"
                        "CrashDescriptor:mini crash dump\n",
                        (unsigned int)minidump->time,
                        asd_crashdump_reason_name(minidump->reason));
                break;
            case 1:
                bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH,
                    "ContentType:cpu reg, call stack and stack data\n"
                    "CrashDuplicateCount:%d\n"
                    "UpTime:%u\n"
                    "StartTime:",
                    crash_report_duplicate_count(),
                    (unsigned int)minidump->uptime_sec);
                break;
            case 2:
                bytes_per_print = make_msg_time(&time_sec, scratchpad, CD_SCRATCH_PAD_LENGTH);
                break;
            case 3:
                bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH, "\nEndTime:");
                break;
            case 4:
                bytes_per_print = make_msg_time(&time_sec, scratchpad , CD_SCRATCH_PAD_LENGTH);
                break;
            case 5:
                bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH,
                    "\ncountryOfResidence: %s\n"
                    "MarketplaceID: %s\n"
                    "TraceHashCode: ",
                    crash_report_country_of_residence(),
                    crash_report_marketplace_id());
                break;
            case 6:
                bytes_per_print = get_crashdump_hash_string(minidump, scratchpad, CD_SCRATCH_PAD_LENGTH);
                bytes_per_print += snprintf(scratchpad + bytes_per_print,
                    CD_SCRATCH_PAD_LENGTH - bytes_per_print,
                    "\n"
                    "Subject:\n\n"
                    "[Events]\n");
               break;
            case 7:
                if (!g_version_info_cb) {
                    continue;
                }
                // Add device and version info
                bytes_per_print = g_version_info_cb(scratchpad, CD_SCRATCH_PAD_LENGTH);
                if (NOT_IN_EXCLUSIVE_RANGE(bytes_per_print, 0, CD_SCRATCH_PAD_LENGTH)) {
                    // We allow app version info ignored, if it returns error.
                    continue;
                }
                break;
            default:
                bytes_per_print = 0;
                break;
            }
            VALIDATE_PRINT_IN_SCRATCH_PAD(bytes_per_print);
            // append output data, if applicable.
            output_len += append_data(buf + output_len, size - output_len,
                                       scratchpad, bytes_per_print,
                                       (int)offset + output_len - decoded_len);
            // count decoded text length.
            decoded_len += bytes_per_print;
            // check if buffer is full.
            if (buf && (output_len == (int) size)) break;
        }
        break;
    }
    case ASD_CD_FRAME_CALL_STACK:
    {
        int depth = 0;
        int istack = 0;
        for (int i = 0; i < 4;) {
            switch (i) {
            case 0:
                ++i;
                bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH,
                    "crash reason: %s\ntask:%.*s\n",
                    asd_crashdump_reason_name(minidump->reason),
                    sizeof(minidump->task_name),
                    minidump->task_name);
                break;
            case 1:
                ++i;
                if(minidump->reason == ASD_CD_REASON_MALLOC_FAILURE) {
                    bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH,
                        "size requested 0x%lx (%lu) bytes, heap free %lu, min heap free %lu\n",
                        minidump->malloc_size_request,
                        minidump->malloc_size_request,
                        minidump->heap_free_size,
                        minidump->min_heap_free_size);
                } else if (minidump->reason == ASD_CD_REASON_ASSERT) {
                    bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH,
                        "File %.*s:%ld %.*s(): assert(%.*s)\n",
                        sizeof(minidump->assertion.file),
                        minidump->assertion.file,
                        minidump->assertion.line,
                        sizeof(minidump->assertion.func),
                        minidump->assertion.func,
                        sizeof(minidump->assertion.expr),
                        minidump->assertion.expr);
                } else {
                    continue;
                }
                break;

            case 2:
                ++i;
                if (!minidump->call_stack_depth)   continue;

                bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH,
                    "Call stack=%u\nTo decode address:\n>>> addr2line -f -e xxx_app.elf ",
                    minidump->call_stack_depth);
                depth = MIN(MAX_CALL_STACK_DEPTH, (int)minidump->call_stack_depth);
                istack = 0;
                break;

            case 3:
                if (minidump->call_stack_depth == 0
                    || istack == depth){
                    ++i;
                    continue;
                }
                //now print call stacks, 8 per line.
                bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH,
                      "%08lx%s", minidump->call_stacks[istack],
                      IS_DUMP_LINE_END(istack, depth)? "\n" : " ");
                ++istack;
                break;

            default:
                break;
            }

            VALIDATE_PRINT_IN_SCRATCH_PAD(bytes_per_print);
            // append output data, if applicable.
            output_len += append_data(buf + output_len, size - output_len,
                                       scratchpad, bytes_per_print,
                                       (int)offset + output_len - decoded_len);
            // count decoded text length.
            decoded_len += bytes_per_print;
            // check if buffer is full.
            if (buf && (output_len == (int) size)) break;
        }
        break;
    }

    case ASD_CD_FRAME_CPU_REG_DUMP: {
        if (minidump->cpu.pc == 0 &&
            minidump->cpu.lr == 0 &&
            minidump->cpu.sp == 0 &&
            minidump->cpu.cfsr == 0)
            //no available CPU register info.
            break;
        const unsigned int* reg = &minidump->cpu.r0;
        for (int i = 0; i < (int)(sizeof(minidump->cpu)/sizeof(*reg)) ; i++) {
            if (((&reg[i]) == &minidump->cpu.fpscr)
                && ((minidump->cpu.exc_return & 0x10) != 0)) {
                /* no FPU context? */
                continue;
            } else {
                bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH,
                    "%-8s= 0x%08x\n", asd_get_cpu_reg_name(i), reg[i]);
            }
            VALIDATE_PRINT_IN_SCRATCH_PAD(bytes_per_print);
            // append output data, if applicable.
            output_len += append_data(buf + output_len, size - output_len,
                                       scratchpad, bytes_per_print,
                                       (int)offset + output_len - decoded_len);
            // count decoded text length.
            decoded_len += bytes_per_print;
            // check if buffer is full.
            if (buf && (output_len == (int) size)) break;

        }
        break;
    }

    case ASD_CD_FRAME_STACK_DUMP:
    {
        if (!minidump->cpu.sp) break;

        for (int i = 0; i < MAX_STACK_DUMP_SIZE_DWORD; i++) {
            bytes_per_print = 0;
            if (i == 0) {
                bytes_per_print = snprintf(scratchpad, CD_SCRATCH_PAD_LENGTH,
                    "\nStack dump:\nsp=0x%x\n", minidump->cpu.sp);
            }
            if (IS_DUMP_LINE_START(i)) {
                bytes_per_print += snprintf(scratchpad + bytes_per_print,
                    CD_SCRATCH_PAD_LENGTH - bytes_per_print,
                    "\n[%08x]: ", minidump->cpu.sp + i*4);
            }

            bytes_per_print += snprintf(scratchpad + bytes_per_print,
                    CD_SCRATCH_PAD_LENGTH - bytes_per_print,
                    "%08lx ",
                minidump->dumped_stack[i]);
                //IS_DUMP_LINE_END(i, MAX_STACK_DUMP_SIZE_DWORD)? "\n" : " ");

            VALIDATE_PRINT_IN_SCRATCH_PAD(bytes_per_print);
            // append output data, if applicable.
            output_len += append_data(buf + output_len, size - output_len,
                                       scratchpad, bytes_per_print,
                                       (int)offset + output_len - decoded_len);
            // count decoded text length.
            decoded_len += bytes_per_print;
            // check if buffer is full.
            if (buf && (output_len == (int) size)) break;
        }
        break;
    }

    case ASD_CD_FRAME_LOG:
    {
        // log is not cache in RAM, and always read from flash.
        // read the log section header first.

        asd_crashdump_header_t logheader;

        if(sizeof(logheader) != cd_flash_read(&cd_config,
                              offsetof(asd_crashdump_map_t, log_header),
                              sizeof(logheader),
                              &logheader)) {
            return 0;
        }

        if (!asd_crashdump_header_is_valid(&logheader))
            // there is no text log append. just return 0
            return 0;
        if (!buf) {
            // no output buffer, just return the length.
            decoded_len = logheader.length;
            break;
        }
        if (offset >= logheader.length) return 0;
        output_len = MIN(logheader.length - offset, (uint32_t) size);
        if (output_len != cd_flash_read(&cd_config,
                              offsetof(asd_crashdump_map_t, log) + offset,
                              output_len, buf)){
            return -AFW_EIO;
        }
        break;
    }

    default:
        return -AFW_EINVAL;
    }


    // no output, and only parse the dump, and report size.
    if (!buf) return decoded_len;

    //real output in buf, return length of data.
    return output_len;
}

static void asd_crashdump_flash_print_finalize(void)
{
    asd_crashdump_header_t header = {0};
    // no log written. skip.
    if (!cd_config.log_len) return;

    CRASHLOG_HEADER_INIT(&header, cd_config.log_len);
    //write the header back to flash.
    if (cd_flash_write(&cd_config, offsetof(asd_crashdump_map_t, log_header),
        sizeof(header), &header) != sizeof(header)) {
        printf("Failed to write crash log header in flash.\n");
    }
    // reset log length.
    cd_config.log_len = 0;
}

/**
 * @brief print to crash log section in flash. crash log section appends minidump.
 *
 * @param [in] fmt ... print format and arguments.
 * @return negative for error (check afw_error.h),
 *         otherwise number of bytes written to flash.
 */
static int asd_crashdump_flash_print(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    // now let's write one to flash.
    int32_t rc = vsnprintf(g_scratchpad, CD_SCRATCH_PAD_LENGTH, fmt, ap);
    va_end(ap);
    VALIDATE_PRINT_IN_SCRATCH_PAD(rc);
    int32_t offset = offsetof(asd_crashdump_map_t, log) + cd_config.log_len;

    // check available length in flash for crash log.
    if (rc + offset > LOG_CRASH_LOG_SIZE) {
        return -AFW_ENOSPC;
    }
    if (cd_flash_write(&cd_config, offset, rc, g_scratchpad) != rc) {
        printf("crash log write error.");
        return -AFW_EIO;
    }
    //update log_len
    cd_config.log_len += rc;
    return rc;

}


static void crashlog_save_heap_info(void)
{
    // DON'T suspend all tasks, because flash manager APIs need context switch enabled
    // for semaphore access.
    asd_crashdump_flash_print("Warning: info may be inconsistent for heap chunks\n");
    asd_dump_heap_stats(asd_crashdump_flash_print);

    asd_crashdump_flash_print_finalize();
}

static void asd_crashdump_post_save(uint32_t reason)
{

    if (ASD_CD_REASON_MALLOC_FAILURE == reason) {
        crashlog_save_heap_info();
    }
}


//============================ APIs ==========================================

int32_t asd_crashdump_init(void)
{
    if (cd_config.status != CD_FLASH_UNINITIALIZED) return 0;
    cd_config.partition = fm_flash_get_partition(FLASH_PARTITION_LOG_CRASH);
    if (!cd_config.partition) return -AFW_ENODEV;
    cd_config.status |= (asd_crashdump_is_available())? CD_FLASH_EXIST : 0;
    cd_config.status |= CD_FLASH_INITIALIZED;
    return 0;
}

const char* asd_crashdump_reason_name(uint32_t rs)
{
    return asd_crash_dump_reason_txt_list[MIN(rs, (uint32_t)ASD_CD_REASON_NUM)];
}

const char* asd_crashdump_metrics_key(uint32_t rs)
{
    return asd_crash_dump_metrics_key_list[MIN(rs, (uint32_t)ASD_CD_REASON_NUM)];
}

const char* asd_get_cpu_reg_name(uint32_t reg_index)
{
    if (reg_index >= sizeof(cpu_regs_name)/sizeof(cpu_regs_name[0]))
        return "unknown";
    return cpu_regs_name[reg_index];
}

bool asd_crashdump_header_is_valid(const asd_crashdump_header_t* header)
{
    if (header->magic_num != ASD_CD_MAGIC_NUM ) return false;
    //Currently, support mini dump and log
    switch (header->type) {
        case ASD_CD_TYPE_MINIDUMP:
            return (header->length == sizeof(asd_crashdump_mini_t));
        case ASD_CD_TYPE_LOG:
            return (header->length > 0) &&
                   (header->length <= CRASHDUMP_LOG_SIZE_MAX);
        default:
            return false;
    }
}

bool asd_crashdump_is_available(void)
{
    uint32_t tmp;
    uint16_t checksum = 0;
    asd_crashdump_header_t cheader;

    if (cd_config.status & CD_FLASH_INITIALIZED) {
        //if already initialized, return the cached value.
        return !!(cd_config.status & CD_FLASH_EXIST);
    }

    if(sizeof(cheader) != asd_crashdump_read_raw(ASD_CD_RAW_HEADER,
                          sizeof(cheader),
                          &cheader)) {
        return false;
    }

    if (!asd_crashdump_header_is_valid(&cheader)) return false;

    for(int i = 0; i < (int)cheader.length; i += sizeof(tmp)) {
        int32_t len = MIN((int)cheader.length-i, (int)sizeof(tmp));

        if (cd_flash_read(&cd_config, offsetof(asd_crashdump_map_t, mini_dump) + i,
                          len, &tmp) != len) {
            return false;
        }
        checksum = crc16_update(checksum, &tmp, sizeof(tmp));
    }

    if (cheader.crc == checksum) {
        cd_config.status |= CD_FLASH_EXIST;
    }


    return (cheader.crc == checksum);
}



//There is no protection against read or write operations
int32_t asd_crashdump_erase(void)
{
    //clear crashdump exist flag.
    cd_config.status &= ~(CD_FLASH_EXIST | CD_FLASH_CACHED);
    cd_config.total_frame_length = 0;
    cd_config.log_len = 0;
    return cd_flash_erase(&cd_config, 0, LOG_CRASH_LOG_SIZE);
}

//There is no protection between reading and writing to flash the sd_info
//The data is not considered critical so the low probability of corruption is
//accepted. The checksum provides an ability to recognize corruption to an extent
void asd_crashdump_save(const asd_crashdump_mini_t *sd_info)
{
    if (!sd_info) return;
    if (asd_crashdump_check_and_erase() != 0) {
        printf("failed to erase crash log.\n");
    }
    printf("Writing to crash log sector\n");
    if( sizeof(*sd_info) != cd_flash_write(&cd_config,
        offsetof(asd_crashdump_map_t, mini_dump),
        sizeof(*sd_info), sd_info)) {
        printf("Failed to write mini dump in crash log sector.\n");
        return;
    }
    asd_crashdump_header_t header;
    CRASHDUMP_HEADER_INIT(&header, 0, sizeof(asd_crashdump_mini_t));
    header.crc = crc16_update(0, sd_info, sizeof(*sd_info));
    if(sizeof(asd_crashdump_header_t)
       != cd_flash_write(&cd_config, offsetof(asd_crashdump_map_t, header),
                         sizeof(asd_crashdump_header_t), &header)){
        printf("Failed to write crash dump header\n");
        return;
    }

    //save additional log:
    asd_crashdump_post_save(sd_info->reason);

    //set flash exist bit.
    cd_config.status |= CD_FLASH_EXIST;
    printf("Succeeds to write crash mini dump. %u bytes = %u + %u\n", CRASHDUMP_MINI_TOTAL_SIZE,
        sizeof(asd_crashdump_header_t), sizeof(asd_crashdump_mini_t));
}

//There is no protection between reading and writing to flash the sd_info
//The data is not considered critical so the low probability of corruption is
//accepted. The checksum provides an ability to recognize corruption to an extent
int32_t asd_crashdump_read_raw(asd_crashdump_section_t section, uint32_t size, void* buf)
{
    uint32_t offset, readsize;
    switch (section) {
    case ASD_CD_RAW_HEADER:
        offset = offsetof(asd_crashdump_map_t, header);
        readsize = sizeof(asd_crashdump_header_t);
        break;
    case ASD_CD_RAW_MINI:
        offset = offsetof(asd_crashdump_map_t, mini_dump);
        readsize = sizeof(asd_crashdump_mini_t);
        break;
    default:
        return -AFW_EINVAL;
    }

    if (size < readsize) return -AFW_EINTRL;

    if (cd_flash_read(&cd_config, offset,
                           readsize, buf) != (int) readsize) {
        return -AFW_EIO;
    }

    return readsize;
}

int32_t asd_crashdump_get_total_frame_length(void)
{
    int total_len = 0;

    if ((cd_config.status & CD_FLASH_INITIALIZED) == 0) {
            return -AFW_ENOINIT;
    }

    if (!asd_crashdump_is_available()) return -AFW_ENOENT;

    if ((cd_config.status & CD_FLASH_CACHED) &&
        (cd_config.total_frame_length != 0))
        return cd_config.total_frame_length;

    for (int i = ASD_CD_FRAME_CRASH_META; i < ASD_CD_FRAME_NUM; i++) {
        asd_frame_info_t frame_info = {0};
        frame_info.index = i;
        int32_t len = asd_crashdump_get_frame_length(&frame_info);
        if (len < 0) return len;
        total_len += len;
    }
    cd_config.total_frame_length = total_len;
    return total_len;
}

int32_t asd_crashdump_get_first_frame(asd_frame_info_t* first)
{
    if (!first) return -AFW_EINVAL;
    if ((cd_config.status & CD_FLASH_INITIALIZED) == 0) {
        return -AFW_ENOINIT;
    }

    if (!asd_crashdump_is_available()) return -AFW_ENOENT;

    first->index = ASD_CD_FRAME_CRASH_META;
    int32_t rc = asd_crashdump_get_frame_length(first);
    if (rc < 0) return rc;
    first->length = rc;
    return 0;
}

int32_t asd_crashdump_get_next_frame(const asd_frame_info_t* cur, asd_frame_info_t* next)
{

    if ((cd_config.status & CD_FLASH_INITIALIZED) == 0) {
        return -AFW_ENOINIT;
    }

    if (!asd_crashdump_is_available()) return -AFW_ENOENT;
    if (!cur || !next || cur->index >= ASD_CD_FRAME_NUM) return -AFW_EINVAL;

    if (cur->index + 1 >= ASD_CD_FRAME_NUM) return -AFW_ENOSPC;

    next->index = cur->index + 1;
    int32_t rc = asd_crashdump_get_frame_length(next);
    if (rc < 0) return rc;
    next->length = rc;
    return 0;
}

int32_t asd_crashdump_read_frame(
    const asd_frame_info_t* frame_info,
    uint32_t offset,
    char *buf,
    size_t size)
{

    if ((cd_config.status & CD_FLASH_INITIALIZED) == 0) {
        return -AFW_ENOINIT;
    }

    if (!asd_crashdump_is_available()) return -AFW_ENOENT;

    if ((cd_config.status & CD_FLASH_CACHED) == 0) {
        //read the minidump.
        int32_t rc= asd_crashdump_read_raw(ASD_CD_RAW_MINI, sizeof(cd_config.mini_dump),
                                      &cd_config.mini_dump);
        if (sizeof(cd_config.mini_dump) != rc) {
            ASD_LOG_E(crashdump, "asd_crashdump_read_raw failed. rc =%ld\n", rc);
            return -AFW_ENOENT;
        }
        cd_config.status |= CD_FLASH_CACHED;
    }
    // Now use the cached mini dump raw data to decode.

    return asd_crashdump_mini_decode(&cd_config.mini_dump, frame_info, offset, buf, size);
}

asd_crashdump_mini_t* asd_get_cached_minidump(void)
{
    return &cd_config.mini_dump;
}

void asd_clear_cached_minidump(void)
{
    cd_config.status &= ~CD_FLASH_CACHED;
    memset(&cd_config.mini_dump, 0, sizeof(cd_config.mini_dump));
}

void crashdump_register_version_info_callback(app_version_info_callback *cb)
{
    g_version_info_cb = cb;
}
