/*
 * Copyright 2018, 2020-2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __SLN_SHELL_H_
#define __SLN_SHELL_H_

#include "network_connection.h"

#if USE_WIFI_CONNECTION
#include "wifi_credentials.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "board.h"
#include "fsl_str.h"
#include "fsl_shell.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_common.h"

/* Fault includes */
#include "fault_handlers.h"

/* FreeRTOS kernel includes */
#include "FreeRTOS.h"
#include "task.h"

#include "serial_manager.h"
#include "serial_port_usb.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_class.h"
#include "usb_device_descriptor.h"
#include "usb_phy.h"
#include "usb_device_config.h"

#include "sln_flash.h"
#include "sln_flash_ops.h"
#include "event_groups.h"
#include "fsl_debug_console.h"

#include "aws_ota_check.h"

/* Amazon wake-word model includes */
#include "amazon_wake_word.h"

#define CONTROLLER_ID kSerialManager_UsbControllerEhci0

#define MAX_LOG_HISTORY 20

typedef enum __shell_event
{
#if USE_WIFI_CONNECTION
    PRINT_EVT = (1 << 0U),
    ERASE_EVT = (1 << 1U),
    SETUP_EVT = (1 << 2U),
#endif /* USE_WIFI_CONNECTION */
    RESET_EVT          = (1 << 3U),
    FACTORY_RESET_EVT  = (1 << 4U),
    MEM_VIEW_EVT       = (1 << 5U),
    ENA_HEAP_TRACE_EVT = (1 << 6U),
    DIS_HEAP_TRACE_EVT = (1 << 7U),
    ENA_USB_LOG_EVT    = (1 << 8U),
    DIS_USB_LOG_EVT    = (1 << 9U),
    LOGS_EVT           = (1 << 10U),
    SERIAL_NUMBER_EVT  = (1 << 11U),
    /* 1 << 12 free here */
    FAULTLOG_STATUSGET_EVT   = (1 << 13U),
    FAULTLOG_STATUSERASE_EVT = (1 << 14U),
    VERSION_EVT              = (1 << 15U),
    WW_EVT                   = (1 << 16U),
    HEAP_VIEW_EVT            = (1 << 17U),
    STACKS_VIEW_EVT          = (1 << 18U),
#ifdef SLN_TRACE_CPU_USAGE
    TRACE_CPU_USAGE_EVT = (1 << 19U),
#endif /* SLN_TRACE_CPU_USAGE */
#ifdef FFS_ENABLED
    FFS_PROVISION_EVT = (1 << 20U),
#endif /* FFS_ENABLED */
} shell_event_t;

typedef struct __shell_heap_trace
{
    uint8_t enable : 1;
    uint16_t threshold;
} shell_heap_trace_t;

typedef struct log_history_node
{
    void *log;
    void *next;
} log_history_node_t;

/**
 * @brief Main function called from shellTask. Initializes USB, SerialManager and SHELL.
 *
 * @return                        0 on success execution, 1 otherwise
 */
int sln_shell_init(void);

/**
 * @brief The task implements a basic SHELL function, supporting a limited number of commands
 *        and also having a history feature.
 *
 * @param arg[in]                 Pointer to an argument, in accordance with TaskFunction_t
 * @return                        Void.
 */
void sln_shell_task(void *arg);

/**
 * @brief Setter function for app_init_task_handle, needed for waking that task.
 *
 * @param handle[in]              Pointer to task handle.
 * @return                        Void.
 */
void sln_shell_set_app_init_task_handle(TaskHandle_t *handle);

/**
 * @brief Print given format string to shell
 *
 * @param formatString[in]        Format string for the log
 * @return                        0
 */
int log_shell_printf(const char *formatString, ...);

/**
 * @brief Add log to log history
 *
 * @param formatString[in]        Format string for the log
 * @return                        Void
 */
void log_history_log_add(const char *formatString, ...);

/**
 * @brief Copy all logs from logs history in the given buffer
 *
 * @param logs_buffer[in]        Buffer where all the logs are copied
 * @param logs_buffer_size[in]   Size of the given buffer
 * @return                       Void
 */
void log_history_buffer_create(uint8_t *logs_buffer, uint32_t logs_buffer_size);

/**
 * @brief Calculate the sum of all logs length from log history
 * @return                       uint32_t
 */
uint32_t log_history_get_size();

/**
 * @brief Print logs from flash address, page by page
 *
 * @param addr[in]               Flash address where logs are stored
 * @param size[in]               Size of the logs buffer
 * @return                       Void
 */
void fault_context_print_logs(uint32_t addr, uint32_t size);

/**
 * @brief Shell callback for FreeRTOS heap trace on pvPortMalloc
 *
 * @param ptr Heap address that is allocated
 * @param size Size of allocation in bytes
 */
void sln_shell_trace_malloc(void *ptr, size_t size);

/**
 * @brief Shell callback for FreeRTOS heap trace on vPortFree
 *
 * @param ptr Heap address that is de-allocated
 * @param size Size of de-allocation in bytes
 */
void sln_shell_trace_free(void *ptr, size_t size);

/**
 * @brief Initialize the static queue handle
 *
 * @param handle Pointer to the event queue handle
 */
void sln_shell_set_app_queue_handle(QueueHandle_t *handle);

#endif /* __SLN_SHELL_H */
