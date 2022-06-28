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
 * ACE HAL Factory Reset platform implementation header
 */


#pragma once


#define FR_LOG_TAG "AceFR"
// Factory reset completion key name. This key should always exists in KVS in
// normal state. It is only removed in progress of factory reset. Once factory
// reset is complete (during reboot), we set the key in KVS.
#define ACE_KVS_FACTORY_RESET_COMPLETION_NAME   "factory_reset_completion"

// factory reset event, user can register callback on the event.
typedef enum {
    ACE_FR_EVENT_OPEN = 0,
    ACE_FR_EVENT_PRE_OP,
    ACE_FR_EVENT_POST_OP,
    ACE_FR_EVENT_CLOSE,
    ACE_FR_EVENT_NUM,
} ace_fr_event_t;

// all user callback must return, and should not block forever.
// All blocking call in usercallback must have timeout.
typedef int (*aceHalFr_user_callback_t)(void* arg);

/**
 * @brief Initialize factory reset manager. It will check the breakcrumb for FR,
 *        and decide to resume factory reset. In this stage, we can assume there
 *        is no interference with other serivces. IMPORTANT: it must called before any
 *        services that uses KVS and flash user partitions.
 */
void aceFrHalMgr_init(void);

/**
 * @brief Register user callback for different events. Not thread safe.
 *        We assume register happens during system setup. And only one callback per event.
 * @param event: the event indicates different timing of factory reset.
 *
 * @return 0 for success, negative for failure.
 */
int aceFrHalMgr_register_user_callback(ace_fr_event_t event, aceHalFr_user_callback_t cb, void* arg);

/**
 * @brief execute user callbacks for different events. Not thread safe
 *        with aceFrHalMgr_register_user_callback.
 *        We assume callback register happens during system setup.
 * @param event: the event indicates different timing of factory reset.
 *
 * @return 0 for success, negative for failure.
 */
int aceFrHalMgr_execute_callback(ace_fr_event_t event);
/**
 * @brief Erase all user partitions defined in macro FACTORY_RESET_PARTITIONS_TO_ERASE.
 *        FACTORY_RESET_PARTITIONS_TO_ERASE should be defined in flash_map.h,
 *        so that project can override. This function should be called during bootup
 *        early, so that there is no interference between other services.
 *
 * @return 0 for success, negative for failure.
 */
int aceFrHalMgr_erase_user_partitions(void);
/**
 * @brief Set/Clear the breadcrumb for factory reset process. The first step of factory
 *        reset is to set breadcrumb. The last step is to clear breadcrumb.
 * @param enable: true to set breadcrumb, false to clear breadcrumb.
 *
 * @return 0 for success, negative for failure.
 */
int aceFrHalMgr_set_breadcrumb(bool enable);

/**
 * @brief Check if factory reset is in progress.
 *
 * @return true: if factory reset is in progress, otherwise false.
 */
bool aceFrHalMgr_is_resetting(void);

