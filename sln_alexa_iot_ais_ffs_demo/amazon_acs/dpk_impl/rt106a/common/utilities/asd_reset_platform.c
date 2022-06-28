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
 * @file asd_reset_platform.c
 *
 * @brief provides functions for reseting platform
 *
 *******************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include <ace/aceCli.h>
#include "iot_reset.h"
#include "afw_reset_platform.h"
#include "asd_reboot_manager.h"

void asd_reset_reboot(char *reboot_reason){
    IotResetBootFlag_t coldBootFlag = eResetColdBootFlag;

    printf("Reset device\r\n");
    /*give time for message to print*/
    vTaskDelay( 500 );
    asd_reboot_manager_reboot(coldBootFlag, reboot_reason);
}

static void asd_reset_flash(void){
    afw_flash_device();

    /* Also need to think about if interupt happen here */
    asd_reset_reboot(REBOOT_REASON_FLASH_RESET);
    while(1);
}

static ace_status_t asd_reset_reboot_cli(int32_t argc, const char** argv) {
    if (argc != 0)
    {
        printf("Expected 0 arg\n");
        return ACE_STATUS_CLI_FUNC_ERROR;
    }

    asd_reset_reboot(REBOOT_REASON_CLI);
    return ACE_STATUS_OK;
}

static ace_status_t asd_reset_flash_cli(int32_t argc, const char** argv) {
    if (argc != 0)
    {
        printf("Expected 0 arg\n");
        return ACE_STATUS_CLI_FUNC_ERROR;
    }

    asd_reset_flash();
    return ACE_STATUS_OK;
}

aceCli_moduleCmd_t asd_reset_cli_sub[] = {
    {"reboot", "Reset device", ACE_CLI_SET_LEAF | ACE_CLI_SET_NOT_RESTRICTED, .command.func=&asd_reset_reboot_cli},
    {"flash", "Put device into flash mode", ACE_CLI_SET_LEAF, .command.func=&asd_reset_flash_cli},
    ACE_CLI_NULL_MODULE
};
