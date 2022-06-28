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
 * @file asd_reboot_manager.h
 *
 * @brief manages platform reboot information
 *******************************************************************************
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "iot_reset.h"
#include "warm_boot_storage_iface.h"

typedef enum asd_reboot_manager_status {
    ASD_REBOOT_MANAGER_STATUS_OK = 0,
    ASD_REBOOT_MANAGER_STATUS_ERROR_GENERAL = -1,
    ASD_REBOOT_MANAGER_STATUS_ERROR_READ_FAILED = -2,
    ASD_REBOOT_MANAGER_STATUS_ERROR_IN_PROGRESS = -3,
    ASD_REBOOT_MANAGER_STATUS_ERROR_NOT_SUPPORTED = -4,
} asd_reboot_manager_status_t;

#define REBOOT_REASON_STRING_LENGTH_MAX   (32)

/* Following are some common reboot reason strings but not
   meant to be an exhustive list of all possible reboot reason.
   The reboot_reason can be any ASCII string provided by application. */

//shutdown by ACE software powerDown request.
#define REBOOT_REASON_POWER_DOWN           "Software_Shutdown"
//reboot by OTA process
#define REBOOT_REASON_OTA                  "OTA_Reboot"
//Factory reset reboot
#define REBOOT_REASON_FACTORY_RESET        "Factory_Reset_Reboot"
//Reboot directive from clould
#define REBOOT_REASON_CLOUD_DIRECTIVE      "Cloud_Directive_Reboot"
//Reset to flash mode
#define REBOOT_REASON_FLASH_RESET          "Warm_Boot_By_Flash"
//Reset by CLI
#define REBOOT_REASON_CLI                  "CLI"
//power loss, maybe unknown
#define REBOOT_REASON_POWER_LOSS           "Sudden_Power_Loss"
//reboot by watchdog timeout
#define REBOOT_REASON_WATCHDOG_TIMEOUT     "Warm_Boot_By_HW_Watchdog"
//Exception reboot.
#define REBOOT_REASON_SOFTWARE_EXCEPTION   "SW_Exception"
//Soft reset request from health monitor.
#define REBOOT_REASON_HEALTH_MONITOR_SOFT  "Warm_Boot_By_HM"
//Unknown software reboot
#define REBOOT_REASON_UNKNOWN_SW_REBOOT    "Warm_Boot_By_Software"

//Reason not available
#define REBOOT_REASON_NA                   "LCR_Not_Available"

#define LCR_TAG_PROGRAM        "LifeCycleReason"
#define LCR_TAG_ABNORMAL       "LCR_abnormal"
#define LCR_TAG_NORMAL         "LCR_normal"
#define LCR_TAG_CUSTOMIZED     "LCR_customized"
#define LCR_TAG_UNIT           "sec"
#define LCR_TAG_KEY            "key"


// Table to convert reboot reason to vital message
//                   reason                       source,
#define REBOOT_REASON_VITAL_TABLE                                     \
    TABLE_ENTRY(REBOOT_REASON_POWER_DOWN,           LCR_TAG_NORMAL)   \
    TABLE_ENTRY(REBOOT_REASON_OTA,                  LCR_TAG_NORMAL)   \
    TABLE_ENTRY(REBOOT_REASON_FACTORY_RESET,        LCR_TAG_NORMAL)   \
    TABLE_ENTRY(REBOOT_REASON_CLOUD_DIRECTIVE,      LCR_TAG_NORMAL)   \
    TABLE_ENTRY(REBOOT_REASON_FLASH_RESET,          LCR_TAG_NORMAL)   \
    TABLE_ENTRY(REBOOT_REASON_CLI,                  LCR_TAG_NORMAL)   \
    TABLE_ENTRY(REBOOT_REASON_POWER_LOSS,           LCR_TAG_ABNORMAL) \
    TABLE_ENTRY(REBOOT_REASON_WATCHDOG_TIMEOUT,     LCR_TAG_ABNORMAL) \
    TABLE_ENTRY(REBOOT_REASON_SOFTWARE_EXCEPTION,   LCR_TAG_ABNORMAL) \
    TABLE_ENTRY(REBOOT_REASON_HEALTH_MONITOR_SOFT,  LCR_TAG_ABNORMAL) \
    TABLE_ENTRY(REBOOT_REASON_UNKNOWN_SW_REBOOT,    LCR_TAG_ABNORMAL) \
    TABLE_ENTRY(REBOOT_REASON_NA,                   LCR_TAG_ABNORMAL)

/**
 * @brief    asd_reboot_manager_reboot is used to reboot the device. This function call adds a breadcrum
 *           in the flash to identify the reboot reason.
 *
 * @param[in]   ucColdBootFlag  flag to determine either to do cold-reset or warm-reset.
 *                              cold-reset means the device is restarted and does not keep
 *                              any blocks of the SOC powered on i.e. device is shutdown and rebooted),
 *                              and warm-reset means the device is restarted while keeping some of the SoC blocks
 *                              powered on through the reboot process.
 *                              For example warm-boot may keep the RAM contents valid after reset by keeping the power
 *                              on for RAM banks, while cold-boot will wipe off the contents.
 *                              One of the IotResetBootFlag_t value. Ref: iot_reset.h
 * @param[in]   reboot_reason   Reboot reason string. It must be null terminated string length
 *                              less than REBOOT_REASON_STRING_LENGTH_MAX.
 */
void asd_reboot_manager_reboot(IotResetBootFlag_t ucColdBootFlag, const char* reboot_reason);

/**
 * @brief   asd_reboot_manager_add_reason is called to add the reboot reason in the reboot history.
 * In case the device is triggering external reboot by sending message to external system, it can use this API
 * to add the reboot reason.
 *
 * @param[in]   reboot_reason   Reboot reason string. It must be null terminated string length
 *                              less than REBOOT_REASON_STRING_LENGTH_MAX.
 */
void asd_reboot_manager_add_reason(const char* reboot_reason);

/**
 * @brief    This function earliest in boot chain to record reboot reason.
 *           This function needs KVS to be initialized to read and write boot reasons.
 *
 * @param[in] wbs_if     warm boot storage interface
 * @return   Reboot manager initialization status.
 */
asd_reboot_manager_status_t asd_reboot_manager_init(const warm_boot_storage_iface_t *wbs_if);

/**
 * @brief    This function should be called publish last reboot reason,
 *           so that reboot manager can process reboot reason.
 */
void asd_reboot_manager_publish(void);

/**
 * @brief    This function is called to dump the reboot history
 */
void asd_reboot_manager_dump(void);
