/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All rights reserved.
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
 * Reference App Flash map extension
 *
 *******************************************************************************
 */

#pragma once

/**
 * @file flash_map_ext.h
 * @brief This is where a product extends the default flashmap to add
 * application specific stores
 *
 */
#if !defined(APP_FLASH_EXT_BASE) || !defined(APP_FLASH_EXT_LENGTH)
#error "Platform flash_map.h doesn't support configurable APP region extensions"
#endif

/*
 * Configure part of the APP flash extension to be used with KVS
 */
#define KVS_EXT_BASE APP_FLASH_EXT_BASE
#define KVS_EXT_LENGTH 0x4000 /* 16KB */

/*
 * KVS partition extensions
 */
#define APP_KVS_FLASH0_TEST_LENGTH 0x1000
#define APP_KVS_FLASH1_TEST_LENGTH 0x1000
#define APP_KVS_ROLL_DATA_LENGTH 0x1000

#define APP_KVS_FLASH0_TEST_ADDR (KVS_EXT_BASE)
#define APP_KVS_FLASH1_TEST_ADDR \
    (APP_KVS_FLASH0_TEST_ADDR + APP_KVS_FLASH0_TEST_LENGTH)
#define APP_KVS_ROLL_DATA_ADDR \
    (APP_KVS_FLASH1_TEST_ADDR + APP_KVS_FLASH1_TEST_LENGTH)

/*
 * Flash Partition names for Flash map extensions
 */
#define APP_KVS_FLASH0_TEST "kvsTest0"
#define APP_KVS_FLASH1_TEST "kvsTest1"
#define APP_ROLL_KVS_DATA "appData"

// clang-format off
/*
 * Configure the flash partitions
 */
#define FLASH_PARTITION_EXTENTION           \
    {                                       \
        .name = APP_KVS_FLASH0_TEST,        \
        .size = APP_KVS_FLASH0_TEST_LENGTH, \
        .offset = APP_KVS_FLASH0_TEST_ADDR, \
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE, \
    },                                      \
    {                                       \
        .name = APP_KVS_FLASH1_TEST,        \
        .size = APP_KVS_FLASH1_TEST_LENGTH, \
        .offset = APP_KVS_FLASH1_TEST_ADDR, \
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE, \
    },                                      \
    {                                       \
        .name = APP_ROLL_KVS_DATA,          \
        .size = APP_KVS_ROLL_DATA_LENGTH,   \
        .offset = APP_KVS_ROLL_DATA_ADDR,   \
        .flags = FM_FLASH_SECURED_WRITEABLE, \
    },                                      \
    /* Add more partitions to this list */
// clang-format on

/*
 * App stores that must be cleared during factory reset
 */
#define FACTORY_RESET_EXT_PARTITIONS_TO_ERASE \
    APP_ROLL_KVS_DATA, /* Add more stores to this list */
