/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates.  All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and conditions set
 * forth in the accompanying LICENSE.TXT file.  This file is a Modifiable File, as
 * defined in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY DISCLAIMS,
 * WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
 * INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, AND NON-INFRINGEMENT.
 */
/*******************************************************************************
 * Flash map template
 *
 *******************************************************************************
 */

#ifndef __FLASH_MAP_H__
#define __FLASH_MAP_H__

#include <stdint.h>
#include "sln_flash_mgmt.h"

/** The file table index from which we want the KVS data to be stored
 *  Using indexes 28-31 from sln_file_table.h
 */
#define FLASH_INDEX 28
#define FIRST_UNUSABLE_INDEX 32

/* Must equal KVS_ROLLING_LENGTH + KVS_PERSIST_LENGTH */
#define KVS_LENGTH                      0xC0000
#define FLASH_SCRATCH_BUF_LENGTH        0x40000

#define KVS_BASE                        SLN_FLASH_MGMT_FILE_ADDR(FLASH_INDEX)

/*
 * Base addresses
 */
extern uintptr_t _g_global_flash_offset;
#define FLASH_BASE _g_global_flash_offset

/* Rolling and persist must sum to KVS_LENGTH */
#define KVS_ROLLING_LENGTH      0x80000    /* 512KB */
#define KVS_PERSIST_LENGTH      0x40000    /* 256KB */
#if KVS_ROLLING_LENGTH + KVS_PERSIST_LENGTH != KVS_LENGTH
#error "KVS_ROLLING_LENGTH + KVS_PERSIST_LENGTH != KVS_LENGTH"
#endif

#define KVS_ROLLING_BASE        (KVS_BASE)
#define KVS_PERSIST_BASE        (KVS_BASE + KVS_ROLLING_LENGTH)
#define KVS_MAX_PARTITION_COUNT 0x20

/*
 * Rolling Credentials Section Partitions
 * FLASH_KVS_BACKUP_SIZE must be equivalent to to
 * the largest partition in KVS.
 *
 * FLASH_KVS_BACKUP_SIZE must be placed in it's own sector
 * for data persistence reasons (when a partition fills up,
 * it is backed up before being erased, to prevent data loss when
 * power goes down unexpectedly; if backup partition is in the same
 * sector as other partitions, it would be erased when those partitions
 * fill up).
 *
 * NOTE: Sizes must be multiples of a page (512 bytes)
 */

/* All entries below occupy one sector - 256KB */
#define FLASH_ROLL_CRED_WIFI_SIZE           0x4000       /*   16KB */
#define FLASH_ROLL_CRED_CUSTID_SIZE         0x4000       /*   16KB */
#define FLASH_ROLL_CRED_IOTKEY_SIZE         0x4000       /*   16KB */
#define FLASH_ROLL_CRED_IOTCRT_SIZE         0x4000       /*   16KB */
#define FLASH_ROLL_FIRMWARE_CONFIG_SIZE     0x4000       /*   16KB */
#define FLASH_ROLL_CRED_CUST_KEY_SIZE       0x4000       /*   16KB */
#define FLASH_ROLL_CRED_USER_ACCT_SIZE      0x4000       /*   16KB */
#define FLASH_ROLL_CRED_DEVICE_INFO_SIZE    0x4000       /*   16KB */
#define FLASH_ROLL_CRED_CUST_DATA_SIZE      0x4000       /*   16KB */
#define FLASH_MAPLITE_SIZE                  0x4000       /*   16KB */
#define FLASH_KVS_SHARED_SIZE               0x10000      /*   64KB */
#define FLASH_ROLL_RSRV1_SIZE               0x8000       /*   32KB */

/* All entries below occupy one sector - 256KB */
#define FLASH_KVS_BACKUP_SIZE               0x10000      /*   64KB */
#define FLASH_ROLL_RSRV2_SIZE               0x30000      /*  192KB */

#define FLASH_ROLL_CRED_WIFI_ADDR       (KVS_ROLLING_BASE)
#define FLASH_ROLL_CRED_CUSTID_ADDR     (FLASH_ROLL_CRED_WIFI_ADDR          + FLASH_ROLL_CRED_WIFI_SIZE)
#define FLASH_ROLL_CRED_IOTKEY_ADDR     (FLASH_ROLL_CRED_CUSTID_ADDR        + FLASH_ROLL_CRED_CUSTID_SIZE)
#define FLASH_ROLL_CRED_IOTCRT_ADDR     (FLASH_ROLL_CRED_IOTKEY_ADDR        + FLASH_ROLL_CRED_IOTKEY_SIZE)
#define FLASH_ROLL_FIRMWARE_CONFIG_ADDR (FLASH_ROLL_CRED_IOTCRT_ADDR        + FLASH_ROLL_CRED_IOTCRT_SIZE)
#define FLASH_ROLL_CRED_CUST_KEY_ADDR   (FLASH_ROLL_FIRMWARE_CONFIG_ADDR    + FLASH_ROLL_FIRMWARE_CONFIG_SIZE)
#define FLASH_ROLL_CRED_USER_ACCT_ADDR  (FLASH_ROLL_CRED_CUST_KEY_ADDR      + FLASH_ROLL_CRED_CUST_KEY_SIZE)
#define FLASH_ROLL_CRED_DEVICE_INFO_ADDR (FLASH_ROLL_CRED_USER_ACCT_ADDR    + FLASH_ROLL_CRED_USER_ACCT_SIZE)
#define FLASH_ROLL_CRED_CUST_DATA_ADDR  (FLASH_ROLL_CRED_DEVICE_INFO_ADDR   + FLASH_ROLL_CRED_DEVICE_INFO_SIZE)
#define FLASH_MAPLITE_ADDR              (FLASH_ROLL_CRED_CUST_DATA_ADDR     + FLASH_ROLL_CRED_CUST_DATA_SIZE)
#define FLASH_KVS_SHARED_ADDR           (FLASH_MAPLITE_ADDR                 + FLASH_MAPLITE_SIZE)
#define FLASH_ROLL_RSRV1_ADDR           (FLASH_KVS_SHARED_ADDR              + FLASH_KVS_SHARED_SIZE)
#define FLASH_KVS_BACKUP_ADDR           (FLASH_ROLL_RSRV1_ADDR              + FLASH_ROLL_RSRV1_SIZE)

/*
 *  Persistent Credentials Section Partitions
 *  Added all of them in a dedicated sector in order to not have
 *  the sector erased when a rolling partition fills up.
 *
 * NOTE: Sizes must be multiples of a page (512 bytes)
 */

/* All entries below occupy one sector - 256KB */
#define FLASH_PERSIST_CRED_DD_SIZE          0xC000      /*    48KB */
#define FLASH_PERSIST_CRED_DHAKEY_SIZE      0xC000      /*    48KB */
#define FLASH_PERSIST_CRED_DHACRT_SIZE      0xC000      /*    48KB */
#define FLASH_PERSIST_CRED_DHACRT1_SIZE     0xC000      /*    48KB */
#define FLASH_PERSIST_CRED_DHACRT2_SIZE     0xC000      /*    48KB */
#define FLASH_PERSIST_RSRV_SIZE             0x4000      /*    16KB */

#define FLASH_PERSIST_CRED_DD_ADDR      (KVS_PERSIST_BASE)
#define FLASH_PERSIST_CRED_DHAKEY_ADDR  (FLASH_PERSIST_CRED_DD_ADDR      + FLASH_PERSIST_CRED_DD_SIZE)
#define FLASH_PERSIST_CRED_DHACRT_ADDR  (FLASH_PERSIST_CRED_DHAKEY_ADDR  + FLASH_PERSIST_CRED_DHAKEY_SIZE)
#define FLASH_PERSIST_CRED_DHACRT1_ADDR (FLASH_PERSIST_CRED_DHACRT_ADDR  + FLASH_PERSIST_CRED_DHACRT_SIZE)
#define FLASH_PERSIST_CRED_DHACRT2_ADDR (FLASH_PERSIST_CRED_DHACRT1_ADDR + FLASH_PERSIST_CRED_DHACRT1_SIZE)
#define FLASH_PERSIST_RSRV_ADDR         (FLASH_PERSIST_CRED_DHACRT2_ADDR + FLASH_PERSIST_CRED_DHACRT2_SIZE)
#define FLASH_SCRATCH_ADDR_NOT_ALIGN    (FLASH_PERSIST_RSRV_ADDR + FLASH_PERSIST_RSRV_SIZE)

// Align to next hyper flash sector
#if FLASH_SCRATCH_ADDR_NOT_ALIGN % SECTOR_SIZE == 0
// Address is already align
#define FLASH_SCRATCH_BUF_ADDR FLASH_SCRATCH_ADDR_NOT_ALIGN
#else
// Add to the address the bytes remaining until the next sector
#define BYTES_UNTIL_NEXT_SECTOR         (SECTOR_SIZE                        - FLASH_SCRATCH_ADDR_NOT_ALIGN % SECTOR_SIZE)
#define FLASH_SCRATCH_BUF_ADDR          (FLASH_SCRATCH_ADDR_NOT_ALIGN       + BYTES_UNTIL_NEXT_SECTOR)
#endif

#define FLASH_FIRST_FREE_ADDR           (FLASH_SCRATCH_BUF_ADDR             + FLASH_SCRATCH_BUF_LENGTH)

#if SLN_FLASH_MGMT_FILE_ADDR(FIRST_UNUSABLE_INDEX) < FLASH_FIRST_FREE_ADDR - 1
#error "FLASH MAP OVERFLOWS LAST USABLE INDEX"
#endif

/*
 * Flash Partition names for Platform Flash Map
 */
#define FLASH_PARTITION_PERSIST_CRED_DD         "deviceData"
#define FLASH_PARTITION_PERSIST_CRED_DHAKEY     "DhaKey"
#define FLASH_PARTITION_PERSIST_CRED_DHACRT     "DhaCert"
#define FLASH_PARTITION_PERSIST_CRED_DHACRT2    "DhaCert2"
#define FLASH_PARTITION_ROLL_FIRMWARE_CONFIG    "FirmwareConfig"
#define FLASH_PARTITION_ROLL_CRED_WIFI          "WifiCred"
#define FLASH_PARTITION_MAPLITE                 "map"
#define FLASH_PARTITION_KVS_BACKUP              "kvsBackup"
#define FLASH_PARTITION_KVS_SHARED              "kvsShared"

/* TODO clean the log definitions below later
 * for now we keep them because asd_logger doesn't compile without these */
#define FLASH_PARTITION_LOG_MAIN                "flashlogMain"
#define FLASH_PARTITION_LOG_CRASH               "flashlogcrash"
#define FLASH_PARTITION_LOG_META                "flashlogMeta"
#define LOG_META_VERSION        1
#define LOG_META_SIZE          0x1000
#define LOG_DSP_SIZE           0x0000
#define LOG_MAIN_SIZE          0x1E000
#define LOG_VITAL_SIZE         0x0000
#define LOG_CRASH_LOG_SIZE     0x1000

#ifdef AMAZON_FLASH_MAP_EXT_ENABLE
#include "flash_map_ext.h"
#endif

#ifndef FACTORY_RESET_EXT_PARTITIONS_TO_ERASE
#define FACTORY_RESET_EXT_PARTITIONS_TO_ERASE
#endif

/*
 * Flash partitions to be erased for factory reset. All should be user data
 */
#define FACTORY_RESET_PARTITIONS_TO_ERASE      \
    FLASH_PARTITION_ROLL_CRED_WIFI,            \
    FLASH_PARTITION_KVS_BACKUP,                \
    FLASH_PARTITION_MAPLITE,                   \
    FLASH_PARTITION_KVS_SHARED,                \
    FACTORY_RESET_EXT_PARTITIONS_TO_ERASE

#define FLASH_PARTITION    \
    {    \
        .name = FLASH_PARTITION_KVS_BACKUP,    \
        .size = FLASH_KVS_BACKUP_SIZE,    \
        .offset = FLASH_KVS_BACKUP_ADDR,    \
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,    \
    },    \
    {    \
        .name = FLASH_PARTITION_KVS_SHARED,    \
        .size = FLASH_KVS_SHARED_SIZE,    \
        .offset = FLASH_KVS_SHARED_ADDR,    \
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,    \
    },    \
    {    \
        .name = FLASH_PARTITION_PERSIST_CRED_DD,    \
        .size = FLASH_PERSIST_CRED_DD_SIZE,    \
        .offset = FLASH_PERSIST_CRED_DD_ADDR,    \
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,    \
    },   \
    {    \
        .name = FLASH_PARTITION_PERSIST_CRED_DHAKEY,    \
        .size = FLASH_PERSIST_CRED_DHAKEY_SIZE,    \
        .offset = FLASH_PERSIST_CRED_DHAKEY_ADDR,    \
        .flags = FM_FLASH_UNSECURED_WRITEABLE,    \
    },    \
    {    \
        .name = FLASH_PARTITION_PERSIST_CRED_DHACRT,    \
        .size = FLASH_PERSIST_CRED_DHACRT_SIZE,    \
        .offset = FLASH_PERSIST_CRED_DHACRT_ADDR,    \
        .flags = FM_FLASH_UNSECURED_WRITEABLE,    \
    },    \
    {    \
        .name = FLASH_PARTITION_PERSIST_CRED_DHACRT2,    \
        .size = FLASH_PERSIST_CRED_DHACRT2_SIZE,    \
        .offset = FLASH_PERSIST_CRED_DHACRT2_ADDR,    \
        .flags = FM_FLASH_UNSECURED_WRITEABLE,    \
    },    \
    {    \
        .name = FLASH_PARTITION_MAPLITE,    \
        .size = FLASH_MAPLITE_SIZE,    \
        .offset = FLASH_MAPLITE_ADDR,    \
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,    \
    },    \
    {    \
        .name = FLASH_PARTITION_ROLL_CRED_WIFI,    \
        .size = FLASH_ROLL_CRED_WIFI_SIZE,    \
        .offset = FLASH_ROLL_CRED_WIFI_ADDR,    \
        .flags = FM_FLASH_UNSECURED_WRITEABLE | FM_FLASH_SECURED_WRITEABLE,    \
    },   \

#endif // __FLASH_MAP_H__
