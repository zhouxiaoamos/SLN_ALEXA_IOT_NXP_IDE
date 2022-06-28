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
 * ACE HAL Factory Reset Driver
 */

#include <stdio.h>
#include <ace/hal_fr.h>
#include <afw_error.h>

#include "flash_map.h"
#include "flash_manager.h"
#include "iot_reset.h"
#include "acehal_fr_mgr.h"
#include "asd_logger_if.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "asd_reboot_manager.h"

#define FACTORY_RESET_ONE_STEP_TIMEOUT_TICK  pdMS_TO_TICKS(10000)


/*********************** API **************************************/

/* platform callbacks */
ace_status_t aceFrHal_open(void)
{
    int ret;
    ret = aceFrHalMgr_set_breadcrumb(true);
    if (ret < 0) {
        ASD_LOG_E(ace_hal, "factory reset fail to set breadcrumb");
        //keep going, try best to do factory reset
    }
    ret = aceFrHalMgr_execute_callback(ACE_FR_EVENT_OPEN);
    if (ret < 0){
        ASD_LOG_I(ace_hal, "factory reset user open failed.");
        //keep going, try best to do factory reset
    }

    //put platform common code here.

    //flush all logs in flash.
    asd_logger_flush_log(FACTORY_RESET_ONE_STEP_TIMEOUT_TICK);
    //erase all logs.
    asd_logger_erase_all_flash_log(FACTORY_RESET_ONE_STEP_TIMEOUT_TICK);


    ASD_LOG_I(ace_hal, "factory reset is open.");
    return ret;
}

bool aceFrHal_check(aceFrHal_type_t type)
{
    //put platform common code here.
    return true;
}

void aceFrHal_preOperation(aceFrHal_type_t type)
{
    ASD_LOG_I(ace_hal, "pre process factory reset");
    /* trigger factory-reset LED patter if any */

    aceFrHalMgr_execute_callback(ACE_FR_EVENT_PRE_OP);
    //put platform common code here.

}

void aceFrHal_postOperation(aceFrHal_type_t type)
{
    ASD_LOG_I(ace_hal, "post process factory reset");
    aceFrHalMgr_execute_callback(ACE_FR_EVENT_POST_OP);
    //put platform common code here.

    /* format flash partition is moved to do after reboot*/
}

ace_status_t aceFrHal_close(void)
{
    ASD_LOG_I(ace_hal, "Closing factory reset.");
    //flush all logs in flash.
    asd_logger_flush_log(FACTORY_RESET_ONE_STEP_TIMEOUT_TICK);
    //user callback. It may trigger a reboot as well.
    aceFrHalMgr_execute_callback(ACE_FR_EVENT_CLOSE);

    //put platform common code here.

    /* reboot */
    printf ("Default rebooting...\n");
    fflush(stdout);
    // The previous callback may reboot early. This reboot is a safe nest to finalize
    // factory reset with a MCU reboot.
    asd_reboot_manager_reboot(eResetColdBootFlag, REBOOT_REASON_FACTORY_RESET);
    return ACE_STATUS_OK;
}

