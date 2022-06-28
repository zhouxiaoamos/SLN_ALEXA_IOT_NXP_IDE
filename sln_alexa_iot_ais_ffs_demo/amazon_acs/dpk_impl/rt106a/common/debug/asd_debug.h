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
 * @File: asd_debug.h
 *******************************************************************************
 */

#pragma once

#include <FreeRTOS.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ASD_CRASHDUMP_CONSOLE_ENABLED
#define ASD_CRASHDUMP_CONSOLE_ENABLED      1
#endif

#ifndef ASD_MALLOC_FAILURE_APP_HOOK
//app can do specific post process for malloc failure.
#define ASD_MALLOC_FAILURE_APP_HOOK(malloc_size, heap_free_size)
#else
extern void ASD_MALLOC_FAILURE_APP_HOOK(size_t, uint32_t);
#endif

#define ASD_CD_REASON_MALLOC_FAILURE       0
#define ASD_CD_REASON_STACK_OVERFLOW       1
#define ASD_CD_REASON_EXCEPTION_HARDFAULT  2
#define ASD_CD_REASON_EXCEPTION_MEMMANAGE  3
#define ASD_CD_REASON_EXCEPTION_BUSFAULT   4
#define ASD_CD_REASON_EXCEPTION_USAGEFAULT 5
#define ASD_CD_REASON_STACK_CORRUPTION     6
#define ASD_CD_REASON_HEAP_CORRUPTION      7
#define ASD_CD_REASON_ASSERT               8
#define ASD_CD_REASON_NULL_POINTER         9
#define ASD_CD_REASON_NUM                  10

#define MAX_CALL_STACK_DEPTH (32)
#define MAX_STACK_DUMP_SIZE_DWORD (64)

// Table: crash_dump_index   metrics_key   text_description
#define ASD_CRASH_DUMP_TABLE                                                                      \
    TABLE_ENTRY(ASD_CD_REASON_MALLOC_FAILURE, "bg_crash_malloc", "Malloc Failure"),               \
    TABLE_ENTRY(ASD_CD_REASON_STACK_OVERFLOW, "bg_crash_stack_overflow", "Stack Overflow"),       \
    TABLE_ENTRY(ASD_CD_REASON_EXCEPTION_HARDFAULT, "bg_crash_hardfault", "Hard Fault Exception"), \
    TABLE_ENTRY(ASD_CD_REASON_EXCEPTION_MEMMANAGE, "bg_crash_memmanage", "Mem Manage Exception"), \
    TABLE_ENTRY(ASD_CD_REASON_EXCEPTION_BUSFAULT,  "bg_crash_busfault",  "Bus Fault Exception"),  \
    TABLE_ENTRY(ASD_CD_REASON_EXCEPTION_USAGEFAULT,"bg_crash_usagefault","Usage Fault Exception"),\
    TABLE_ENTRY(ASD_CD_REASON_STACK_CORRUPTION,  "bg_crash_stackcorrupt","Stack Corruption"),     \
    TABLE_ENTRY(ASD_CD_REASON_HEAP_CORRUPTION,   "bg_crash_heapcorrupt", "Heap Corruption"),      \
    TABLE_ENTRY(ASD_CD_REASON_ASSERT,            "bg_crash_assert",      "SW Assertion"),         \
    TABLE_ENTRY(ASD_CD_REASON_NULL_POINTER,      "bg_crash_null",        "NULL pointer"),         \
    TABLE_ENTRY(ASD_CD_REASON_NUM,               "bg_crash_unknown",     "Unknown Crash"),

typedef enum {
    ASD_DEBUG_INFO_TASK_STATUS = 1,
    ASD_DEBUG_INFO_STACK_RAW = 2,
    ASD_DEBUG_INFO_ALL = 0xFFFF,
} asd_debug_info_t;

enum {
    STACKED_R0,
    STACKED_R1,
    STACKED_R2,
    STACKED_R3,
    STACKED_R12,
    STACKED_LR,
    STACKED_PC,
    STACKED_PSR,
    STACKED_S0,
    STACKED_S1,
    STACKED_S2,
    STACKED_S3,
    STACKED_S4,
    STACKED_S5,
    STACKED_S6,
    STACKED_S7,
    STACKED_S8,
    STACKED_S9,
    STACKED_S10,
    STACKED_S11,
    STACKED_S12,
    STACKED_S13,
    STACKED_S14,
    STACKED_S15,
    STACKED_FPSCR,
};

typedef struct {
    unsigned int hfsr;            /* SCB.HFSR                                        */
    unsigned int cfsr;            /* SCB.CFSR                                        */
    unsigned int mmfar;           /* SCB.MMFAR                                       */
    unsigned int bfar;            /* SCB.BFAR                                        */
} asd_exception_reg_t;

typedef struct crashdump_mini {
    uint8_t reason;
    uint8_t call_stack_depth;
    uint8_t reserved[2];
    uint32_t call_stacks[MAX_CALL_STACK_DEPTH];
    uint32_t dumped_stack[MAX_STACK_DUMP_SIZE_DWORD];
    uint64_t time;
    uint32_t uptime_sec;                       ///< system uptime in seconds before a crash
    struct {
        unsigned int r0;
        unsigned int r1;
        unsigned int r2;
        unsigned int r3;
        unsigned int r4;
        unsigned int r5;
        unsigned int r6;
        unsigned int r7;
        unsigned int r8;
        unsigned int r9;
        unsigned int r10;
        unsigned int r11;
        unsigned int r12;
        unsigned int sp;              /* after pop r0-r3, lr, pc, xpsr                   */
        unsigned int lr;              /* lr before exception                             */
        unsigned int pc;              /* pc before exception                             */
        unsigned int psr;             /* xpsr before exeption                            */
        unsigned int control;         /* nPRIV bit & FPCA bit meaningful, SPSEL bit = 0  */
        unsigned int exc_return;      /* current lr                                      */
        unsigned int msp;             /* msp                                             */
        unsigned int psp;             /* psp                                             */
        unsigned int fpscr;
        unsigned int hfsr;            /* SCB.HFSR                                        */
        unsigned int cfsr;            /* SCB.CFSR                                        */
        unsigned int mmfar;           /* SCB.MMFAR                                       */
        unsigned int bfar;            /* SCB.BFAR                                        */
    } cpu;
    union {
        //malloc failure meta data.
        struct {
            uint32_t malloc_size_request;
            uint32_t heap_free_size;
            uint32_t min_heap_free_size;
        };
        //assert meta data.
        struct {
            uint32_t line;   ///< line number
            char file[32];   ///< file name
            char func[16];   ///< function name
            char expr[64];   ///< assert expression.
        } assertion;
    };
    //task information.
    char task_name[configMAX_TASK_NAME_LEN];
}__attribute__((packed)) asd_crashdump_mini_t;

typedef void (*upload_crashdump_cb_t)(void);

typedef int (*asd_print_func_t)(const char* fmt, ...);

/**
 *@brief callback that executed by asd_crashdump_handler().
 *@param [in] reason: reason trigger the crashdump handling, check ASD_CD_REASON_XXX
 *@parma [in] persistent_saved: is the dump saved in flash.
 */
typedef void (*crashdump_post_process_cb_t)(uint32_t reason, bool persistent_saved);

/**
 * @brief Initialize asd debug module.
 */
int32_t asd_debug_init(void);


/**
 * @brief Register the callback for crashdump upload.
 */
void asd_debug_register_upload_crashdump_callback
     (upload_crashdump_cb_t trigger_upload_cb);

/**
 * @brief handler function for dump stack.
 */
void asd_crashdump_handler(void);

/**
 * @brief Dump task info to stdout
 *
 * @param task_info_flag: flag for task info dump. check asd_debug_info_t for details.
 * @return none
 */
void asd_debug_print_all_tasks(uint32_t task_info_flag);

/**
 * @brief Preprocess for an assert. This will copy assert meta data for coredump.
 *
 * @param [in] expr: const string of assert expression.
 * @param [in] file: const string file name where assert.
 * @param [in] line: line number where assert.
 * @param [in] func: const string function name where assert.
 *
 * @return none
 */
void asd_assert_preprocess(const char *expr, const char *file, int line, const char* func);

/**
 * @brief Dump task context in static ram variable, according to stack and exc_return.
 *
 * @param [in] stack: pointer to the task's stack top.
 * @param [in] exc_return: exc return value.
 *
 * @return none
 */
void asd_dump_task_context_from_stack(asd_crashdump_mini_t* dump_info, uint32_t stack[], uint32_t exc_return);

/**
 * @brief Dump memory in hex and ASCII. Print on console.
 *
 * @param [in] desc: pointer description of the dump.
 * @param [in] addr: memory addr to start dump.
 * @param [in] len:  length of memory.
 *
 * @return none
 */
void memory_dump_hex_ascii(const char * desc, const void * addr, int len);

/**
 * @brief Set flag to allow reboot on exception.
 *
 * @param [in] allowed: true to allow reboot; false to disable reboot on exception, e.g.
 *             hang after exception.
 *
 * @return none
 */
void exception_reboot_allow(bool allowed);

/**
 * @brief Reboot used by exception handler.
 *
 * @return none
 */
void exception_reboot(void);

/**
 * @brief Query the location of minidump in RAM.
 *
 * @return writable pointer of cached minidump
 */
asd_crashdump_mini_t* asd_get_cached_minidump(void);

/**
 * @brief Clear cached minidump in RAM.
 *
 * @return none
 */
void asd_clear_cached_minidump(void);

/**
 * @brief An assert is happening or not.
 *
 * @return true: assert in progress, false: no assert.
 */
bool asd_is_assert_happening(void);


/**
 * @brief Dump system heap stats
 * Ideally, asd_dump_heap_stats() should run
 * without malloc/free from other tasks, so that the walkHeap can be consistent
 * with xPortGetHeapStats(). When print callback involves flash access, flash manager
 * APIs requires context switch enabled to access semaphore. In that case, we
 * should NOT suspend all tasks.
 *
 * @param [in] print: print function pointer.
 *
 * @return none.
 */
void asd_dump_heap_stats(asd_print_func_t print);

/**
 * @brief Set callback for post process of crash handling. App can customize crash handling.
 *        The callback will be called in asd_crashdump_handler in thread mode.
 * @param [in] cb: callback to run after crash is handled and before reboot/crash log upload.
 *
 * @return none.
 */
void asd_debug_set_crashdump_post_process_cb(crashdump_post_process_cb_t cb);

#ifdef __cplusplus
}
#endif


