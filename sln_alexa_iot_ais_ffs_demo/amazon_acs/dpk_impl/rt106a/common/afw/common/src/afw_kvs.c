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

#include "stdio.h"
#include "string.h"

#include "flash_manager.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "stdbool.h"

#include "afw_def.h"
#include "afw_error.h"
#include <asd_log_platform_api.h>

#include "crc_mpeg.h"

#include "afw_kvs.h"
#include "afw_kvs_internal.h"
#include <crc16.h>
#include <flash_map.h>
#include <mbedtls/aes.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#ifdef UNIT_TESTS
#include <mock_flash_manager.h>
#endif


/*
KVS entries in flash will be made up of 2 different parts
    - Meta blob: Contains lengths for value, namespace and key,
                 plus CRC and info regarding value
    - Value blob: Contains namespace, key, and value strings


Meta Blob (4 bytes)
-----------------------------------------------------
|         VAL META         |  Val CRC  |  Meta CRC  |
-----------------------------------------------------
          5 bytes             2 bytes      2 bytes

Value Blob
-----------------------------------------------------------------------------
|  NAMESPACE  |           KEY          |                VALUE               |
-----------------------------------------------------------------------------
  <= 16 bytes         <= 48 bytes                   <= 7680 bytes
*/

// defines
#define GET_MACRO1(_1, _2, _3, _4, NAME, ...) NAME
#define GET_MACRO2(_1, _2, _3, NAME, ...) NAME

// Flash Macros
#define FLASH_OFFSET(_offset) (FLASH_BASE + _offset)
#define KVS_FLASH_WRITE_REL_ADDR(_part, _to, _len, _buf)            \
    fm_flash_write(_part, kvs_mgr.kvs_fm_client_id, (unsigned long)(_to), _len, (const uint8_t *)(_buf))
#define KVS_FLASH_WRITE_ABS_ADDR(_to, _len, _buf)                   \
    fm_flash_write(NULL, kvs_mgr.kvs_fm_client_id, (unsigned long)(_to), _len, (const uint8_t *)(_buf))
#define KVS_FLASH_READ_REL_ADDR(_part, _from, _len, _buf)           \
    fm_flash_read(_part, kvs_mgr.kvs_fm_client_id, (unsigned long)(_from), _len, (uint8_t *)(_buf))
#define KVS_FLASH_READ_ABS_ADDR(_from, _len, _buf)                  \
    fm_flash_read(NULL, kvs_mgr.kvs_fm_client_id, (unsigned long)(_from), _len, (uint8_t *)(_buf))

#define KVS_FLASH_WRITE(...)                                        \
    GET_MACRO1(__VA_ARGS__, KVS_FLASH_WRITE_REL_ADDR, KVS_FLASH_WRITE_ABS_ADDR)(__VA_ARGS__)
#define KVS_FLASH_READ(...)                                         \
    GET_MACRO1(__VA_ARGS__, KVS_FLASH_READ_REL_ADDR, KVS_FLASH_READ_ABS_ADDR)(__VA_ARGS__)

#define KVS_FLASH_ERASE(_part)                                      \
    fm_flash_erase(_part, kvs_mgr.kvs_fm_client_id)
#define KVS_FLASH_ERASE_PARTIAL(_part, _offset, _len)               \
    fm_flash_erase_sectors(_part, kvs_mgr.kvs_fm_client_id, _offset, _len)
#define KVS_FLASH_ERASE_BACKUP()                                    \
    KVS_FLASH_ERASE(backup_part)

// Locking Macros
#define LOCK_OR_RETURN_ERROR(_partition)                            \
    do {                                                            \
        if (afw_kvs_lock(_partition)) {                             \
            return -AFW_EBUSY;                                      \
        }                                                           \
    } while(0)
#define UNLOCK_AND_RETURN_1_PART(_partition, _ret)                  \
    afw_kvs_unlock(_partition); return _ret
#define UNLOCK_AND_RETURN_2_PART(_partition1, _partition2, _ret)    \
    afw_kvs_unlock(_partition1); afw_kvs_unlock(_partition2); return _ret
#define UNLOCK_AND_RETURN(...)                                      \
    GET_MACRO2(__VA_ARGS__, UNLOCK_AND_RETURN_2_PART, UNLOCK_AND_RETURN_1_PART)(__VA_ARGS__)

// Utility Macros
#define KVS_SECTOR_OFFSET(_addr)                                    \
    ((uint32_t)(_addr) % kvs_mgr.flash_info->ulSectorSize)
#define HEADER_LEN_NO_CRC (sizeof(afw_kvs_partition_header_t) - 2)
#define META_CRC_LENGTHS (4)
#define META_LEN_NO_CRC (sizeof(afw_kvs_entry_meta_t) - META_CRC_LENGTHS)
#define META_CRC_COMP_LEN (sizeof(afw_kvs_entry_meta_t) - 2)

#define PACKED_NAMESPACE_LEN(_shared, _namespace)                   \
    (_namespace ? (shared ? (strlen(_namespace) < MAX_NAMESPACE_LEN ? strlen(_namespace) : MAX_NAMESPACE_LEN) : 0) : 0)
#define PACKED_KEY_LEN(_key)                                        \
    (strlen(_key) < MAX_KEY_LEN ? strlen(_key) : MAX_KEY_LEN)
#define PACKED_NAMESPACE_KEY_LEN(_shared, _namespace, _key)         \
    (PACKED_KEY_LEN(_key) + PACKED_NAMESPACE_LEN(_shared, _namespace))
#define PACKED_BUF_LEN(_shared, _namespace, _key, _val_len)         \
    (PACKED_KEY_LEN(_key) + PACKED_NAMESPACE_LEN(_shared, _namespace) + _val_len)

// Value Defines
#define VERSION_ARR_SIZE (1)
#define KVS_MAGIC_BYTE (0xC4)
#define KVS_TERM_BYTE (0xDB)
#define NONCE_LEN (12)
#define MAX_NAMESPACE_LEN (AFW_KVS_MAX_NAMESPACE_LEN)
#define MAX_KEY_LEN (AFW_KVS_MAX_KEY_LEN)
#define MAX_VALUE_LEN (AFW_KVS_MAX_VALUE_LEN)
#define AES_CTR_BUF_SIZE (256)

// Structs
typedef union afw_kvs_nonce_counter_u {
    struct {
        unsigned char nonce[NONCE_LEN];
        uint32_t address;
    };

    unsigned char nc_buf[16];
} afw_kvs_nonce_counter_t;

typedef struct afw_kvs_partition_info_s {
    fm_flash_partition_t *partition;
    unsigned long offset;
    SemaphoreHandle_t write_lock;
} afw_kvs_partition_info_t;


typedef struct afw_kvs_mgr_s {
    bool afw_kvs_initialized;
    int kvs_fm_client_id;
    fm_flash_info *flash_info;
    int partition_count;
    afw_kvs_partition_info_t partition_info[KVS_MAX_PARTITION_COUNT];

    mbedtls_aes_context kvs_aes_ctx;
    mbedtls_ctr_drbg_context kvs_ctr_drbg;
    mbedtls_entropy_context kvs_entropy;
    unsigned char *kvs_aes_key;
} afw_kvs_mgr_t;

// Static Vars
static fm_flash_partition_t *backup_part = NULL;
static fm_flash_partition_t *shared_part = NULL;
afw_kvs_mgr_t kvs_mgr = {0};

static const uint8_t terminal_byte = KVS_TERM_BYTE;

#ifdef AMAZON_TESTS_ENABLE
extern void testReboot(void);
#endif

// Static Function Prototypes
static int32_t cleanup_impl(const char * namespace,
                            const char* key,
                            const void* val,
                            uint16_t val_len,
                            bool encrypted);
static fm_flash_partition_t* find_partition(const char * namespace);
static bool is_partition_initialized(fm_flash_partition_t *part);
static bool initialize_partition(fm_flash_partition_t *part);
static void generate_nonce(unsigned char nonce[NONCE_LEN]);
static int32_t get_partition_header(afw_kvs_partition_header_t *hdr,
                                    fm_flash_partition_t *target);
static void populate_entry_header(const char * namespace, const char* key, char *header, bool shared);
static bool find_next_entry(uint8_t **pos, unsigned long boundary);
static int32_t find_write_address(fm_flash_partition_t *part, unsigned long *write_addr);
static void update_write_address(fm_flash_partition_t *part, unsigned long new_addr);
static void reset_write_address(fm_flash_partition_t *part);
static uint8_t* find_entry(const char * namespace, const char* key);
static bool crc_comp_check(uint16_t comp, void *ptr, uint32_t len);
static int32_t delete_entry(const uint8_t *pos);
static bool aes_ctr_in_place(void *data,
                             uint16_t len,
                             fm_flash_partition_t *part,
                             uint32_t address);
static int32_t aes_ctr_in_flash(afw_kvs_nonce_counter_t *src_nc,
                                fm_flash_partition_t *src_part,
                                uint32_t src_addr,
                                afw_kvs_nonce_counter_t *dst_nc,
                                fm_flash_partition_t *dst_part,
                                uint32_t dst_addr,
                                const uint32_t buf_len);
static int32_t aes_ctr_ram_to_flash(const void *data,
                                    uint16_t len,
                                    fm_flash_partition_t *part,
                                    uint32_t dst,
                                    uint16_t *crc,
                                    afw_kvs_nonce_counter_t nc);
static int32_t recover_backup(void);
static int32_t afw_kvs_lock(fm_flash_partition_t *partition);
static void afw_kvs_unlock(fm_flash_partition_t *partition);

int32_t afw_kvs_init(unsigned char *mbedtls_aes_key) {
    if (kvs_mgr.afw_kvs_initialized) {
        return 0;
    }
    kvs_mgr.afw_kvs_initialized = true;

    mbedtls_aes_init(&kvs_mgr.kvs_aes_ctx);
    mbedtls_entropy_init(&kvs_mgr.kvs_entropy);

    char *personalization = "my_app_specific_string";

    kvs_mgr.kvs_aes_key = mbedtls_aes_key;

    mbedtls_ctr_drbg_init(&kvs_mgr.kvs_ctr_drbg);
    if (mbedtls_ctr_drbg_seed(&kvs_mgr.kvs_ctr_drbg,
                              mbedtls_entropy_func,
                              &kvs_mgr.kvs_entropy,
                              (const unsigned char *)personalization,
                              strlen(personalization))) {
        return -AFW_EINTRL;
    }

    backup_part = fm_flash_get_partition(FLASH_PARTITION_KVS_BACKUP);
    shared_part = fm_flash_get_partition(FLASH_PARTITION_KVS_SHARED);
    if (!backup_part || !shared_part) {
        ASD_LOG_E(afw, "Failed to initialize kvs");
        return -AFW_ENOENT;
    }

    kvs_mgr.kvs_fm_client_id = fm_flash_get_unique_client_id();
    kvs_mgr.flash_info = fm_flash_getinfo();

    // Update write offsets
    fm_flash_partition_t *table = fm_flash_get_partition_table(&kvs_mgr.partition_count);
    int offset = 0;
    for (int i = 0; i < kvs_mgr.partition_count; i++) {
        if (afw_kvs_is_kvs_partition(table[i].offset)) {
            kvs_mgr.partition_info[offset].partition = &table[i];
            kvs_mgr.partition_info[offset].write_lock = xSemaphoreCreateRecursiveMutex();
            if (!kvs_mgr.partition_info[offset].write_lock      ||
                table[i].size > backup_part->size) {
                ASD_LOG_E(afw,
                          "Failed to initialize partition %s",
                          table[i].name);
                afw_kvs_deinit();
                return -1;
            }
            offset++;
        }
    }

    kvs_mgr.partition_count = offset;
    if (kvs_mgr.partition_count > KVS_MAX_PARTITION_COUNT) {
        ASD_LOG_E(afw,
                  "Number of partitions (%d) exceeds Max Partition Count(%d)",
                  kvs_mgr.partition_count, KVS_MAX_PARTITION_COUNT);
        afw_kvs_deinit();
        return -2;
    }

    recover_backup();

    for (unsigned i = 0; i < sizeof(kvs_mgr.partition_count); i++) {
        find_write_address(kvs_mgr.partition_info[i].partition, &kvs_mgr.partition_info[i].offset);
    }

    return AFW_OK;
}

int32_t afw_kvs_deinit(void)
{
    if (!kvs_mgr.afw_kvs_initialized) {
        return 0;
    }
    backup_part = NULL;
    shared_part = NULL;
    for (int i = 0; i < kvs_mgr.partition_count; i++) {
        kvs_mgr.partition_info[i].partition = NULL;
        kvs_mgr.partition_info[i].offset = 0;
        if (kvs_mgr.partition_info[i].write_lock) {
            vSemaphoreDelete(kvs_mgr.partition_info[i].write_lock);
        }
    }

    mbedtls_ctr_drbg_free(&kvs_mgr.kvs_ctr_drbg);
    mbedtls_entropy_free(&kvs_mgr.kvs_entropy);
    mbedtls_aes_free(&kvs_mgr.kvs_aes_ctx);

    kvs_mgr.afw_kvs_initialized = false;

    return AFW_OK;
}

int32_t afw_kvs_set(const char * namespace,
                    const char* key,
                    const void* val,
                    uint16_t val_len,
                    bool encrypted)
{
    // Input Validation
    if (!key                        ||
        strlen(key) > MAX_KEY_LEN   ||
        strlen(key) == 0            ||
        !val                        ||
        val_len > MAX_VALUE_LEN     ||
        val_len == 0) {
        return -AFW_EINVAL;
    }

    if (namespace &&
        strlen(namespace) > MAX_NAMESPACE_LEN) {
        return -AFW_EINVAL;
    }

    // Get target partition
    fm_flash_partition_t *target = find_partition(namespace);
    bool shared = (target == shared_part);
    bool force_encryption = (bool)target->encrypted || encrypted;

    int32_t ret = 0;
    LOCK_OR_RETURN_ERROR(target);

    // Target address
    unsigned long write_offset = 0;
    ret = find_write_address(target, &write_offset);
    if (ret) {
        ASD_LOG_E(afw, "failed to find write address");
        UNLOCK_AND_RETURN(target, ret);
    }

    uint8_t header_len = PACKED_NAMESPACE_KEY_LEN(shared, namespace, key);
    uint32_t entry_size = sizeof(afw_kvs_entry_meta_t) +
                          header_len + val_len + 1; // +1 for terminal byte

    // Cleanup if necessary
    if (write_offset + entry_size > target->size) {
        // Check to see if an old instance exists
        int32_t old_size = afw_kvs_get_size(namespace, key);
        if (old_size > 0 && write_offset + val_len <= target->size + old_size) {
            ret = cleanup_impl(namespace, key, val, val_len, force_encryption);
            UNLOCK_AND_RETURN(target, ret);
        }

        afw_kvs_cleanup(target->name);

        ret = find_write_address(target, &write_offset);
        if (ret) {
            ASD_LOG_E(afw, "failed to find write address");
            UNLOCK_AND_RETURN(target, ret);
        }
        if (write_offset + entry_size > target->size) {
            UNLOCK_AND_RETURN(target, -AFW_ENOMEM);
        }
    }


    // Read out header
    afw_kvs_partition_header_t target_hdr;
    ret = get_partition_header(&target_hdr, target);
    if (ret != AFW_OK) {UNLOCK_AND_RETURN(target, ret);}

    // Generate Header
    char header[header_len];
    memset(header, 0, header_len);
    populate_entry_header(namespace, key, header, shared);

    // Populate Meta Struct
    afw_kvs_entry_meta_t meta = {{0}};
    meta.magic = KVS_MAGIC_BYTE;
    meta.namespace_len = PACKED_NAMESPACE_LEN(shared, namespace);
    meta.key_len = PACKED_KEY_LEN(key);
    meta.value_len = val_len;
    meta.encrypted = force_encryption ? 1 : 0;
    meta.valid = 1;

    // Combine meta & namespace/key header into one buffer to consolidate writes
    uint8_t buf_len = sizeof(meta) + header_len;
    uint8_t buf[buf_len];
    memcpy(buf, &meta, META_LEN_NO_CRC);
    memset(buf + META_LEN_NO_CRC, 0xFF, META_CRC_LENGTHS);  // CRCs will be written last so keep as 0xFF for now
    memcpy(buf + sizeof(meta), header, header_len);

    // Write meta and header buf to target partition
    uint32_t write_pos = write_offset;
    ret = KVS_FLASH_WRITE(target, write_pos, buf_len, buf);
    if (ret != buf_len) {
        ASD_LOG_E(afw, "Failed to write first part of entry to %s", target->name);
        UNLOCK_AND_RETURN(target, ret);
    }
    write_pos += ret;

#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    // Write value to target partition and calculate value_crc
    meta.value_crc = crc16_update(0, header, header_len);
    if (!force_encryption) {
        meta.value_crc = crc16_update(meta.value_crc, val, val_len);
        ret = KVS_FLASH_WRITE(target, write_pos, val_len, val);
        if (ret != val_len) {
            ASD_LOG_E(afw, "error writing value");
            UNLOCK_AND_RETURN(target, ret);
        }
    } else {
        // Generate nonce-counter for encryption
        afw_kvs_nonce_counter_t nc;
        memcpy(nc.nonce,
               ((afw_kvs_partition_header_t *)FLASH_OFFSET(target->offset))->nonce,
               NONCE_LEN);
        nc.address = target->offset + write_pos;
        ret = aes_ctr_ram_to_flash(val, val_len, target, write_pos, &meta.value_crc, nc);
        if (ret != AFW_OK) {
            UNLOCK_AND_RETURN(target, ret);
        }

        ret = val_len;
    }
    meta.value_crc = crc16_update(meta.value_crc, &terminal_byte, 1);
    write_pos += ret;

#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    // Write Terminal Byte
    ret = KVS_FLASH_WRITE(target, write_pos, 1, &terminal_byte);
    if (ret != 1) {
        ASD_LOG_E(afw, "error writing Terminal Byte");
        UNLOCK_AND_RETURN(target, ret);
    }
    write_pos += ret;

#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    // generate meta_crc
    meta.meta_crc = crc16_update(0, &meta, META_CRC_COMP_LEN);

    // Write CRCs
    ret = KVS_FLASH_WRITE(target, write_offset + META_LEN_NO_CRC,
                          META_CRC_LENGTHS, &meta.value_crc);
    if (ret != META_CRC_LENGTHS) {
        ASD_LOG_E(afw, "error writing CRCs");
        UNLOCK_AND_RETURN(target, ret);
    }

#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    // Compare CRCs
    uint8_t *target_val = (uint8_t *)FLASH_OFFSET(target->offset + write_offset) + sizeof(meta);
    if (!crc_comp_check(meta.meta_crc, (uint8_t *)FLASH_OFFSET(target->offset + write_offset), META_LEN_NO_CRC + sizeof(meta.value_crc)) ||
        meta.meta_crc != *(uint16_t *)FLASH_OFFSET(target->offset + write_offset + META_LEN_NO_CRC + sizeof(uint16_t))) {
        meta.valid = 0;
        ret = KVS_FLASH_WRITE(target, write_offset, sizeof(meta), &meta);
        if (ret != sizeof(meta)) {
            ASD_LOG_E (afw, "error invalidating value after failed meta CRC check at addr");
        } else {
            ASD_LOG_E(afw, "Meta CRC Failure. Meta not written successfully");
        }
        UNLOCK_AND_RETURN(target, -AFW_EBADF);
    }
    if (!crc_comp_check(meta.value_crc, target_val, header_len + val_len + 1) ||
        meta.value_crc != *(uint16_t *)FLASH_OFFSET(target->offset + write_offset + META_LEN_NO_CRC)) {
        meta.valid = 0;
        ret = KVS_FLASH_WRITE(target, write_offset, sizeof(meta), &meta);
        if (ret != sizeof(meta)) {
            ASD_LOG_E (afw, "error invalidating value after failed value CRC check");
        } else {
            ASD_LOG_E(afw, "Value CRC Failure. Value not written successfully");
        }
        UNLOCK_AND_RETURN(target, -AFW_EBADF);
    }

#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    // Delete entry if it already exists
    uint8_t *old_entry = find_entry(namespace, key);
    while (old_entry &&
           old_entry < (uint8_t *)target->offset + write_offset) {
        ret = delete_entry(old_entry);
        if (ret) {UNLOCK_AND_RETURN(target, ret);}
        old_entry = find_entry(namespace, key);
    }

#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    update_write_address(target, write_pos);
    UNLOCK_AND_RETURN(target, AFW_OK;);
}

int32_t afw_kvs_get(const char * namespace,
                    const char* key,
                    void* value,
                    const uint16_t val_len)
{
    fm_flash_partition_t *target = find_partition(namespace);

    LOCK_OR_RETURN_ERROR(target);
    uint8_t *pos = find_entry(namespace, key);
    if (!pos) {
        UNLOCK_AND_RETURN(target, -AFW_ENOENT);
    }

    afw_kvs_entry_meta_t meta = {{0}};
    if (sizeof(meta) !=
        KVS_FLASH_READ(pos, sizeof(meta), &meta)) {
        UNLOCK_AND_RETURN(target, -AFW_EUFAIL);
    }

    // Check that input buffer size is valid
    if (meta.value_len > val_len) {
        ASD_LOG_E(afw, "value len: %d val_len: %d",
                  meta.value_len, val_len);
        UNLOCK_AND_RETURN(target, -AFW_ENOMEM);
    };

    pos += sizeof(meta);
    uint8_t *val_pos = pos + meta.namespace_len + meta.key_len;
    int ret = KVS_FLASH_READ(val_pos, meta.value_len, value);

    uint16_t crc = crc16_update(0, FLASH_OFFSET(pos), meta.namespace_len + meta.key_len);
    crc = crc16_update(crc, value, meta.value_len);
    crc = crc16_update(crc, &terminal_byte, 1);
    if (ret != meta.value_len ||
        crc != meta.value_crc ||
        !crc_comp_check(meta.meta_crc, &meta, META_CRC_COMP_LEN)) {
        UNLOCK_AND_RETURN(target, -AFW_EBADF);
    }

    if (meta.encrypted) {
        aes_ctr_in_place(value, meta.value_len, target, (uint32_t)val_pos);
    }

    UNLOCK_AND_RETURN(target, meta.value_len);
}

int32_t afw_kvs_get_size(const char * namespace,
                         const char* key)
{
    fm_flash_partition_t *target = find_partition(namespace);

    LOCK_OR_RETURN_ERROR(target);
    uint8_t *pos = find_entry(namespace, key);
    if (!pos) {
        UNLOCK_AND_RETURN(target, -AFW_ENOENT);
    }

    afw_kvs_entry_meta_t meta = {{0}};
    if (sizeof(meta) !=
        KVS_FLASH_READ(pos, sizeof(meta), &meta)) {
        UNLOCK_AND_RETURN(target, -AFW_EUFAIL);
    }

    UNLOCK_AND_RETURN(target, meta.value_len);
}

int32_t afw_kvs_get_all_keys(const char *namespace, char *buffer, uint32_t size)
{
    return afw_kvs_get_all_keys_with_callback(namespace, buffer, size, NULL);
}

int32_t afw_kvs_get_all_keys_with_callback(const char *namespace,
                                           char *buffer,
                                           uint32_t size,
                                           afw_key_cb key_cb)
{
    // Make sure something will be done with the flash that is read
    if (buffer == NULL && key_cb == NULL) {
        return AFW_EINVAL;
    }
    fm_flash_partition_t *target = find_partition(namespace);
    bool shared = (target == shared_part);
    char *buffer_pos = buffer;
    afw_kvs_entry_meta_t meta = {{0}};

    LOCK_OR_RETURN_ERROR(target);
    for (uint8_t *scan = (uint8_t *)(target->offset + sizeof(afw_kvs_partition_header_t));
         scan < (uint8_t *)(target->offset + target->size) && *FLASH_OFFSET(scan) != 0xFF; ) {
        // Read in meta
        if (sizeof(meta) !=
            KVS_FLASH_READ(scan, sizeof(meta), &meta)) {
            UNLOCK_AND_RETURN(target, -AFW_EUFAIL);
        }

        // Check for valid before anything else
        if (!meta.valid) {
            find_next_entry(&scan, target->offset + target->size);
            continue;
        }

        // Validate Meta with CRC
        if (!crc_comp_check(meta.meta_crc, &meta, META_CRC_COMP_LEN)) {
            find_next_entry(&scan, target->offset + target->size);
            continue;
        }

        // Validate Value with CRC
        uint32_t value_crc_comp_len = meta.namespace_len + meta.key_len + meta.value_len + 1;
        if (!crc_comp_check(meta.value_crc, FLASH_OFFSET(scan + sizeof(meta)), value_crc_comp_len)) {
            find_next_entry(&scan, target->offset + target->size);
            continue;
        }

        // Validate namespace match
        uint32_t key_offset = 0;
        // key_offset for NULL namespace and non-shared namespaces are the same
        // so they can be handled together
        if (!shared || !namespace) {
            if (shared && meta.namespace_len != 0) {
                find_next_entry(&scan, target->offset + target->size);
                continue;
            }
            key_offset = (uint32_t)scan + sizeof(meta) - target->offset;
        } else {
            if (strlen(namespace) != meta.namespace_len ||
                strncmp(namespace, (char *)FLASH_OFFSET(scan) + sizeof(meta), strlen(namespace))) {
                find_next_entry(&scan, target->offset + target->size);
                continue;
            }
            key_offset = (uint32_t)scan + sizeof(meta) + meta.namespace_len - target->offset;
        }

        // Copy key from flash over to buffer and then run the call-back
        // If a buffer was passed in, we do not need to malloc copy space.
        char *key_buffer;
        bool key_is_malloc;
        if (buffer_pos == NULL) {
            key_buffer = calloc(meta.key_len + 1, sizeof(char));
            key_is_malloc = true;
        } else {
            key_buffer = buffer_pos;
            key_is_malloc = false;
        }
#define CLEANUP_KEY_BUFFER_IF_NEEDED    \
        do {                            \
            if (key_is_malloc) {        \
                free(key_buffer);       \
            }                           \
        } while(0);

        if (meta.key_len !=
            KVS_FLASH_READ(target, key_offset, meta.key_len, key_buffer)) {
            CLEANUP_KEY_BUFFER_IF_NEEDED;
            UNLOCK_AND_RETURN(target, -AFW_EUFAIL);
        }
        if (key_cb) {
            if (key_cb(key_buffer) != 0) {
                CLEANUP_KEY_BUFFER_IF_NEEDED;
                UNLOCK_AND_RETURN(target, -AFW_EINVAL);
            }
        }
        if (buffer_pos) {
            // Validate that buffer is large enough to bring in next key
            if (buffer_pos + meta.key_len >= buffer + size) {
                CLEANUP_KEY_BUFFER_IF_NEEDED;
                UNLOCK_AND_RETURN(target, -AFW_ENOMEM);
            }

            strcpy(buffer_pos, key_buffer);
            buffer_pos += meta.key_len + 1;
        }
        CLEANUP_KEY_BUFFER_IF_NEEDED;
#undef CLEANUP_KEY_BUFFER_IF_NEEDED

        // Move on to next entry
        find_next_entry(&scan, target->offset + target->size);
    }

    UNLOCK_AND_RETURN(target, AFW_OK);
}

int32_t afw_kvs_delete(const char * namespace,
                       const char* key)
{
    fm_flash_partition_t *target = find_partition(namespace);
    LOCK_OR_RETURN_ERROR(target);

    uint8_t *scan = find_entry(namespace, key);
    if (!scan) {
        UNLOCK_AND_RETURN(target, -AFW_ENOENT);
    }

    int32_t ret = delete_entry(scan);

    UNLOCK_AND_RETURN(target, ret);
}
int32_t afw_kvs_delete_namespace(const char *namespace)
{
    fm_flash_partition_t *target = find_partition(namespace);
    bool shared = (target == shared_part);
    LOCK_OR_RETURN_ERROR(target);

    if (!shared) {
        KVS_FLASH_ERASE(target);
        reset_write_address(target);
        UNLOCK_AND_RETURN(target, AFW_OK);
    }


    // scan through partition and attempt to retrieve entry
    afw_kvs_entry_meta_t meta = {{0}};
    for (uint8_t *scan = (uint8_t *)(target->offset + sizeof(afw_kvs_partition_header_t));
         scan < (uint8_t *)(target->offset + target->size) && *FLASH_OFFSET(scan) != 0xFF; ) {
        // Grab Meta from current address
        if (sizeof(meta)!=
            KVS_FLASH_READ(scan, sizeof(meta), &meta)) {UNLOCK_AND_RETURN(target, -AFW_EUFAIL);}


        if (namespace) {
            // Check #1: continue if invalid or namespace/key len mismatch
            if (!meta.valid || meta.namespace_len != strlen(namespace)) {
                if (!find_next_entry(&scan, target->offset + target->size)) {UNLOCK_AND_RETURN(target, -AFW_EUFAIL);}
                continue;
            }

            // Check #2: namespace/key cmp mismatch
            if (memcmp(namespace, FLASH_OFFSET(scan) + sizeof(meta), meta.namespace_len)) {
                if (!find_next_entry(&scan, target->offset + target->size)) {UNLOCK_AND_RETURN(target, -AFW_EUFAIL);}
                continue;
            }
        } else {
            if (meta.namespace_len > 0) {
                if (!find_next_entry(&scan, target->offset + target->size)) {UNLOCK_AND_RETURN(target, -AFW_EUFAIL);}
                continue;
            }
        }

        // We have a match!
        delete_entry(scan);
        if (!find_next_entry(&scan, target->offset + target->size)) {
            UNLOCK_AND_RETURN(target, -AFW_EUFAIL);
        }
    }

    UNLOCK_AND_RETURN(target, AFW_OK);
}

int32_t afw_kvs_cleanup(const char *namespace) {
    return cleanup_impl(namespace, NULL, NULL, 0, false);
}

bool afw_kvs_is_kvs_partition(unsigned long offset) {
    return ((offset >= KVS_BASE && offset < (KVS_BASE + KVS_LENGTH))
#if defined(KVS_EXT_BASE) && defined(KVS_EXT_LENGTH)
        || (offset >= KVS_EXT_BASE  && offset < (KVS_EXT_BASE + KVS_EXT_LENGTH))
#endif
    );
}

/* ============================== Static Functions ============================== */
static int32_t cleanup_impl(const char * namespace,
                            const char* key,
                            const void* val,
                            uint16_t val_len,
                            bool encrypted)
{
    // Establish partition info for cleanup partition
    fm_flash_partition_t *target = find_partition(namespace);
    bool shared = (target == shared_part);
    int32_t ret;

    LOCK_OR_RETURN_ERROR(backup_part);
    // Macro won't work for this case
    if (afw_kvs_lock(target)) {UNLOCK_AND_RETURN(backup_part, -AFW_EBUSY);}
    reset_write_address(target);

    if (!is_partition_initialized(target)) {
        ASD_LOG_I(afw, "Nothing to cleanup", target->name);
        UNLOCK_AND_RETURN(target, backup_part, AFW_OK);
    }

    // Read out header
    afw_kvs_partition_header_t target_hdr;
    ret = get_partition_header(&target_hdr, target);
    if (ret != AFW_OK) {
        UNLOCK_AND_RETURN(target, backup_part, ret);
    }

    // Handle locals
    uint8_t *read_ptr = (uint8_t *)target->offset + sizeof(target_hdr);
    uint32_t write_offset = sizeof(target_hdr);

    // Setup Nonce-Counter for src
    afw_kvs_nonce_counter_t src_nc;
    memcpy(src_nc.nonce, target_hdr.nonce, NONCE_LEN);

    // Setup Nonce-Counter for dst
    afw_kvs_nonce_counter_t dst_nc;
    generate_nonce(dst_nc.nonce);

    // Reuse target_hdr as the backup hdr
    memcpy(target_hdr.nonce, dst_nc.nonce, NONCE_LEN);
    target_hdr.restore_address = (uint32_t)target->offset;
    target_hdr.crc = crc16_update(0, &target_hdr, HEADER_LEN_NO_CRC);

    // Erase backup
    KVS_FLASH_ERASE_BACKUP();

#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    // Loop through target partition and aggregate valid entries
    afw_kvs_entry_meta_t meta = {{0}};
    while (read_ptr + sizeof(meta) < (uint8_t *)target->offset + target->size) {
        // Grab Meta
        ret = KVS_FLASH_READ(read_ptr, sizeof(meta), &meta);
        if (ret != sizeof(meta)) {
            UNLOCK_AND_RETURN(target, backup_part, ret);
        }

        // Validate current meta/entry and move onto next entry if not valid
        uint8_t header_len = meta.namespace_len + meta.key_len;
        uint16_t kv_len = header_len + meta.value_len + 1;
        uint16_t entry_size = sizeof(meta) + header_len + meta.value_len + 1; // +1 for terminal byte
        while (!crc_comp_check(meta.meta_crc, &meta, META_CRC_COMP_LEN) ||
               !crc_comp_check(meta.value_crc, (void *)FLASH_OFFSET(read_ptr + sizeof(meta)), kv_len)) {
            // Grab next valid entry
            if (!find_next_entry(&read_ptr, target->offset + target->size)) {
                UNLOCK_AND_RETURN(target, backup_part, -AFW_EBADF);
            }

            // Read & validate meta for next entry
            ret = KVS_FLASH_READ(read_ptr, sizeof(meta), &meta);
            if (ret != sizeof(meta)) {
                UNLOCK_AND_RETURN(target, backup_part, ret);
            }
            if (*FLASH_OFFSET(read_ptr) == 0xFF ||
                read_ptr >= (uint8_t *)target->offset + target->size) {
                goto write_backup_to_target;
            }

            header_len = meta.namespace_len + meta.key_len;
            kv_len = header_len + meta.value_len + 1;
            entry_size = sizeof(meta) + header_len + meta.value_len + 1;
        }


        // Handle Valid Entries
        if (key && strlen(key) == meta.key_len &&
            !memcmp(key, FLASH_OFFSET(read_ptr) + sizeof(meta) + meta.namespace_len, meta.key_len)) {
            meta.encrypted = encrypted;
            meta.value_len = val_len;
            meta.meta_crc = 0xFFFF;
            meta.value_crc = 0xFFFF;

            // Write Meta
            ret = KVS_FLASH_WRITE(backup_part, write_offset, sizeof(meta), &meta);
            if (ret != sizeof(meta)) {
                UNLOCK_AND_RETURN(target, backup_part, ret);
            }

            // Write Namespace and Key
            uint8_t header_len = meta.namespace_len + meta.key_len;
            ret = KVS_FLASH_WRITE(backup_part,
                                  write_offset + sizeof(meta),
                                  header_len,
                                  FLASH_OFFSET(read_ptr) + sizeof(meta));
            if (ret != header_len) {
                UNLOCK_AND_RETURN (target, backup_part, ret);
            }

            meta.value_crc = crc16_update(0, FLASH_OFFSET(read_ptr) + sizeof(meta), header_len);

            // Write Val
            if (!encrypted) {
                ret = KVS_FLASH_WRITE(backup_part, write_offset + sizeof(meta) + header_len, val_len, val);
                if (ret != val_len) {
                    UNLOCK_AND_RETURN(target, backup_part, ret);
                }
                meta.value_crc = crc16_update(meta.value_crc, val, val_len);
            } else {
                dst_nc.address = (uint32_t)target->offset + write_offset + sizeof(meta) + header_len;
                ret = aes_ctr_ram_to_flash(val,
                                           val_len,
                                           backup_part, write_offset + sizeof(meta) + header_len,
                                           &meta.value_crc,
                                           dst_nc);
                if (ret != AFW_OK) {
                    UNLOCK_AND_RETURN(target, backup_part, ret);
                }
            }

            // Write Terminal Byte
            entry_size = sizeof(meta) + header_len + val_len + 1;
            kv_len = header_len + meta.value_len + 1;
            ret = KVS_FLASH_WRITE(backup_part, write_offset + entry_size - 1, 1, &terminal_byte);
            if (ret != 1) {
                UNLOCK_AND_RETURN(target, backup_part, ret);
            }
            meta.value_crc = crc16_update(meta.value_crc, &terminal_byte, 1);

            // Calculate Meta CRC and Write Both CRCs
            meta.meta_crc = crc16_update(0, &meta, META_CRC_COMP_LEN);
            ret = KVS_FLASH_WRITE(backup_part, write_offset + META_LEN_NO_CRC,
                                  META_CRC_LENGTHS, &meta.meta_buf[META_LEN_NO_CRC]);
            if (ret != META_CRC_LENGTHS) {
                UNLOCK_AND_RETURN(target, backup_part, ret);
            }
        }
        else if (!meta.encrypted && !target->encrypted) {
            ret = KVS_FLASH_WRITE(backup_part, write_offset, entry_size, FLASH_OFFSET(read_ptr));
            if (ret != entry_size) {
                UNLOCK_AND_RETURN(target, backup_part, entry_size);
            }
        } else {
            // Handle everything but value & terminal byte
            uint8_t buf_len = sizeof(meta) + header_len;
            uint8_t buf[buf_len];
            bool old_unencrypted = false;
            if (!meta.encrypted) {
                meta.encrypted = 1;
                old_unencrypted = true;
            }
            memcpy(buf, &meta, META_LEN_NO_CRC);
            memset(buf + META_LEN_NO_CRC, 0xFF, META_CRC_LENGTHS);  // CRCs will be written last so keep as 0xFF for now
            ret = KVS_FLASH_READ(read_ptr + sizeof(meta), header_len, buf + sizeof(meta));
            if (ret != header_len) {return ret;}
            ret = KVS_FLASH_WRITE(backup_part, write_offset, buf_len, buf);
            if (ret != buf_len) {return ret;}

            // Handle migrating encrypted value portion
            src_nc.address = (uint32_t)read_ptr + buf_len;
            dst_nc.address = (uint32_t)target->offset + write_offset + buf_len;
            if (AFW_OK != aes_ctr_in_flash(old_unencrypted ? NULL : &src_nc,
                                           target,
                                           src_nc.address - (uint32_t)target->offset,
                                           &dst_nc,
                                           backup_part,
                                           write_offset + buf_len,
                                           meta.value_len)) {
                UNLOCK_AND_RETURN(target, backup_part, -AFW_EINTRL);
            }

            // write terminal byte
            ret = KVS_FLASH_WRITE(backup_part, write_offset + entry_size - 1, 1, &terminal_byte);
            if (ret != 1) {
                UNLOCK_AND_RETURN(target, backup_part, ret);
            }

            // Calculate & write CRCs
            meta.value_crc = crc16_update(0, (uint8_t *)FLASH_OFFSET(backup_part->offset + write_offset + sizeof(meta)),
                                          header_len + meta.value_len + 1);
            meta.meta_crc = crc16_update(0, &meta, META_CRC_COMP_LEN);
            ret = KVS_FLASH_WRITE(backup_part, write_offset + META_LEN_NO_CRC,
                                  META_CRC_LENGTHS, &meta.meta_buf[META_LEN_NO_CRC]);
            if (ret != META_CRC_LENGTHS) {
                UNLOCK_AND_RETURN(target, backup_part, ret);
            }
        }

        // Check CRCs
        if (!crc_comp_check(meta.value_crc,
                            (void *)FLASH_OFFSET(backup_part->offset + write_offset + sizeof(meta)),
                            kv_len)) {
            UNLOCK_AND_RETURN(target, backup_part, -AFW_EBADF);
        }

        if (!crc_comp_check(meta.meta_crc,
                            (void *)FLASH_OFFSET(backup_part->offset + write_offset),
                            META_CRC_COMP_LEN)) {
            UNLOCK_AND_RETURN(target, backup_part, -AFW_EBADF);
        }

        write_offset += entry_size;
        read_ptr += entry_size;
    }

write_backup_to_target:

#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif
    // Writing backup header indicates that backup is ready to restore
    // in the event of an unexpected reboot
    ret = KVS_FLASH_WRITE(backup_part, 0, sizeof(target_hdr), &target_hdr);
    if (ret != sizeof(target_hdr)) {
        UNLOCK_AND_RETURN(target, backup_part, ret);
    }
    if (!crc_comp_check(target_hdr.crc, (void *)FLASH_OFFSET(backup_part->offset), HEADER_LEN_NO_CRC)) {
        UNLOCK_AND_RETURN(target, backup_part, -AFW_EBADF);
    }
#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    // Erase target
    KVS_FLASH_ERASE(target);

#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    // Write body of backup to target
    ret = KVS_FLASH_WRITE(target, sizeof(target_hdr),
                          write_offset - sizeof(target_hdr),
                          FLASH_OFFSET(backup_part->offset) + sizeof(target_hdr));
    if (ret != (int)(write_offset - sizeof(target_hdr))) {
        UNLOCK_AND_RETURN(target, backup_part, ret);
    }

    // Fix target header and write to target partition
    target_hdr.magic = KVS_MAGIC_BYTE;
    target_hdr.crc = crc16_update(0, &target_hdr, HEADER_LEN_NO_CRC);
    ret = KVS_FLASH_WRITE(target, 0, sizeof(target_hdr), &target_hdr);
    if (ret != sizeof(target_hdr)) {UNLOCK_AND_RETURN(backup_part, ret);}
    if (!crc_comp_check(target_hdr.crc, (void *)FLASH_OFFSET(target->offset), HEADER_LEN_NO_CRC)) {
        UNLOCK_AND_RETURN(target, backup_part, -AFW_EBADF);
    }
#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    // invalidate header in backup
    target_hdr.restore_address = 0;
    ret = KVS_FLASH_WRITE(backup_part, NONCE_LEN,
                          sizeof(target_hdr.restore_address), &target_hdr.restore_address);
    if (ret != sizeof(target_hdr.restore_address)) {
        UNLOCK_AND_RETURN(target, backup_part, ret);
    }

#ifdef AMAZON_TESTS_ENABLE
    testReboot();
#endif

    UNLOCK_AND_RETURN(target, backup_part, AFW_OK);
}

static fm_flash_partition_t* find_partition(const char * namespace)
{
    fm_flash_partition_t *part = namespace ? fm_flash_get_partition(namespace) : NULL;
    if (part && afw_kvs_is_kvs_partition(part->offset) &&
        part != backup_part) {
        return part;
    }

    // Return shared by default
    return shared_part ? shared_part : NULL;
}

static bool is_partition_initialized(fm_flash_partition_t *part)
{
    if (!part) {return false;}

    // Read out header
    afw_kvs_partition_header_t target_hdr;
    int32_t ret = get_partition_header(&target_hdr, part);
    if (ret != AFW_OK) {return false;}

    return (crc_comp_check(target_hdr.crc, (void *)FLASH_OFFSET(part->offset), HEADER_LEN_NO_CRC));
}

/* Caller of this function MUST take lock. Function is not implicitly thread-safe */
static bool initialize_partition(fm_flash_partition_t *part)
{
    if (!part) {return false;}

    afw_kvs_partition_header_t header;
    generate_nonce(header.nonce);
    header.magic = KVS_MAGIC_BYTE;
    header.crc = crc16_update(0, &header, sizeof(header) - sizeof(header.crc));
    int ret = KVS_FLASH_WRITE(part, 0, sizeof(header), &header);
    if (ret != sizeof(header)) {return false;}
    if (!crc_comp_check(header.crc, (void *)FLASH_OFFSET(part->offset), HEADER_LEN_NO_CRC)) {
        return false;
    }
    reset_write_address(part);

    return true;
}

static void generate_nonce(unsigned char nonce[NONCE_LEN])
{
    mbedtls_ctr_drbg_random(&kvs_mgr.kvs_ctr_drbg, nonce, NONCE_LEN);
}

static int32_t get_partition_header(afw_kvs_partition_header_t *hdr,
                                    fm_flash_partition_t *target)
{
    if (!target || !hdr) {
        return -AFW_EINVAL;
    }

    int32_t ret = KVS_FLASH_READ(target, 0, sizeof(*hdr), hdr);
    if (ret != sizeof(*hdr)) {
        return ret;
    }

    // validate header
    if (!crc_comp_check(hdr->crc, hdr, HEADER_LEN_NO_CRC)) {
        return -AFW_EBADF;
    }

    return AFW_OK;
}

static void populate_entry_header(const char * namespace, const char* key, char *header, bool shared)
{
    uint8_t header_pos = 0;
    if (shared && namespace) {
        uint8_t namespace_len = PACKED_NAMESPACE_LEN(shared, namespace);
        memcpy(&header[header_pos], namespace, namespace_len);
        header_pos += namespace_len;
    }
    uint8_t key_len = PACKED_KEY_LEN(key);
    memcpy(&header[header_pos], key, key_len);
}

static bool find_next_entry(uint8_t **pos, unsigned long boundary)
{
    if (!pos || !(*pos)) {
        return false;
    }

    afw_kvs_entry_meta_t meta = {{0}};
    if (sizeof(meta) != KVS_FLASH_READ(*pos, sizeof(meta), &meta)) {
        return false;
    }

    // Check for matching CRC
    // We check the crc against the version in RAM so we can correct
    // for cleared valid bits
    meta.valid = 1;
    if (crc_comp_check(meta.meta_crc, &meta, META_CRC_COMP_LEN)) {
        uint8_t header_len = meta.namespace_len + meta.key_len;
        *pos += sizeof(meta) + header_len + meta.value_len + 1;
        if (*pos > (uint8_t *)boundary) {return false;}
        return true;
    }

    // Meta above didn't match - scan for MAGIC byte or 0xFF
    // Skip the first (sizeof(meta)) bytes in the scan
    *pos += sizeof(meta);
    while (*pos < (uint8_t *)boundary) {
        // Handle boundary check before scanning
        while (*FLASH_OFFSET(*pos) != KVS_MAGIC_BYTE &&
               *FLASH_OFFSET(*pos) != 0xFF) {
            if ((*pos)++ > (uint8_t *)boundary) {return false;}
        }

        // Handle Empty Partition (**pos == 0xFF)
        if (*FLASH_OFFSET(*pos) == 0xFF) {
            // Read byte-by-byte
            uint8_t *scan = *pos;
            while (scan < (uint8_t *)boundary && *FLASH_OFFSET(scan) == 0xFF) {
                scan++;
            }
            if (boundary - (unsigned long)scan < sizeof(meta)) {
                return true;
            } else {
                *pos = scan;
                continue;
            }
        }

        // Found Magic Byte (**pos == MAGIC_BYTE)
        memcpy(&meta, FLASH_OFFSET(*pos), sizeof(meta));
        meta.valid = 1;
        if (crc_comp_check(meta.meta_crc, &meta, sizeof(meta) - sizeof(meta.meta_crc))) {
            return true;
        }

        (*pos)++;
    }

    return false;

}

static int32_t find_write_address(fm_flash_partition_t *part, unsigned long *write_addr)
{
    if (!is_partition_initialized(part)) {
        KVS_FLASH_ERASE(part);
        if (!initialize_partition(part)) {
            return -AFW_EUFAIL;
        }
    }

    int index = 0;
    for (index = 0; index < kvs_mgr.partition_count; index++) {
        if (part == kvs_mgr.partition_info[index].partition) {
            if (kvs_mgr.partition_info[index].offset > 0) {
                *write_addr = kvs_mgr.partition_info[index].offset;
                return AFW_OK;
            }
            break;
        }
    }

    uint8_t *scan;
    for (scan = (uint8_t *)(part->offset) + sizeof(afw_kvs_partition_header_t);
         scan < (uint8_t *)(part->offset) + part->size && *FLASH_OFFSET(scan) != 0xFF;) {
        if (!find_next_entry(&scan, part->offset + part->size)) {
            return -AFW_EUFAIL;
        }
    }

    if (*FLASH_OFFSET(scan) == 0xFF && (unsigned long)scan < part->offset + part->size) {
        kvs_mgr.partition_info[index].offset = *write_addr = scan - (uint8_t *)part->offset;
        return AFW_OK;
    }

    return -AFW_ENOMEM;
}

static void update_write_address(fm_flash_partition_t *part, unsigned long new_addr)
{
    for (int i = 0; i < kvs_mgr.partition_count; i++) {
        if (part == kvs_mgr.partition_info[i].partition) {
            kvs_mgr.partition_info[i].offset =  new_addr;
            return;
        }
    }
}

static void reset_write_address(fm_flash_partition_t *part)
{
    update_write_address(part, 0);
}

// Must be aligned with KVS Entries
static uint8_t* find_entry(const char * namespace, const char* key)
{
    // Figure out which partition to look in
    fm_flash_partition_t *part = find_partition(namespace);
    bool shared = (part == shared_part);

    // generate header buffer to use as comparison
    uint8_t header_len = PACKED_NAMESPACE_KEY_LEN(shared, namespace, key);
    char header[header_len];
    populate_entry_header(namespace, key, header, shared);

    // scan through partition and attempt to retrieve entry
    afw_kvs_entry_meta_t meta = {{0}};
    for (uint8_t *scan = (uint8_t *)(part->offset + sizeof(afw_kvs_partition_header_t));
         scan < (uint8_t *)(part->offset + part->size) && *FLASH_OFFSET(scan) != 0xFF; ) {
        // Grab Meta from current address
        if (sizeof(meta)!=
            KVS_FLASH_READ(scan, sizeof(meta), &meta)) {
            return NULL;
        }

        // Grab length of value header
        uint8_t curr_len = meta.namespace_len + meta.key_len;

        // Check #1: continue if invalid or namespace/key len mismatch
        if (!meta.valid || curr_len != header_len) {
            if (!find_next_entry(&scan, part->offset + part->size)) {
                return NULL;
            }
            continue;
        }

        // Check #2: namespace/key cmp mismatch
        if (memcmp(header, FLASH_OFFSET(scan) + sizeof(meta), curr_len)) {
            if (!find_next_entry(&scan, part->offset + part->size)) {
                return NULL;
            }
            continue;
        }

        // Do CRC check to make sure meta we grabbed is intact
        if (!crc_comp_check(meta.meta_crc,
                            &meta,
                            sizeof(meta)- sizeof(meta.meta_crc))) {
            if (!find_next_entry(&scan, part->offset + part->size)) {
                return NULL;
            }
            continue;
        }

        if (!crc_comp_check(meta.value_crc,
                            FLASH_OFFSET(scan + sizeof(meta)),
                            meta.value_len + curr_len + 1)) {
            if (!find_next_entry(&scan, part->offset + part->size)) {
                return NULL;
            }
            continue;
        }

        // We have a match!
        return scan;
    }

    return NULL;
}

static inline bool crc_comp_check(uint16_t comp, void *ptr, uint32_t len)
{
    return (comp == crc16_update(0, ptr, len));
}

/* Caller of this function MUST take lock. Function is not implicitly thread-safe */
static int32_t delete_entry(const uint8_t *pos)
{
    afw_kvs_entry_meta_t meta = {{0}};
    if (sizeof(meta) !=
        KVS_FLASH_READ(pos, sizeof(meta), &meta)) {
        return -AFW_EUFAIL;
    }

    // Check if it's already deleted
    if (!meta.valid) {return AFW_OK;}

    meta.valid = 0;
    int ret = KVS_FLASH_WRITE(pos, sizeof(meta), &meta);
    if (ret != sizeof(meta)) {
        ASD_LOG_E(afw, "failed to delete entry %d != %d", ret, sizeof(meta));
        return -AFW_EINTRL;
    }

    return AFW_OK;
}

static bool aes_ctr_in_place(void *data,
                             uint16_t len,
                             fm_flash_partition_t *part,
                             uint32_t address)
{
    afw_kvs_nonce_counter_t nc;
    memcpy(nc.nonce,
           ((afw_kvs_partition_header_t *)FLASH_OFFSET(part->offset))->nonce,
           NONCE_LEN);
    nc.address = address;

    mbedtls_aes_setkey_enc(&kvs_mgr.kvs_aes_ctx, kvs_mgr.kvs_aes_key, 128);
    unsigned char stream_block[16];
    memset(stream_block, 0, 16);
    size_t nc_off = 0;
    int ret =  mbedtls_aes_crypt_ctr(&kvs_mgr.kvs_aes_ctx, len, &nc_off,
                                     (unsigned char *)&nc,
                                     stream_block, data, data);

    return ret == 0;
}

/**
 * @brief Read large (encrypted/unencrypted) buffers from flash
 *        or ram and write them to a destination buffer
 *        (encrypted/unencrypted)
 *
 * This API allows the caller to take in a large buffer either
 * from flash or RAM and process it in batches. Uses include:
 *      - Taking a byte stream from flash and re-encrypting with
 *        different IV without having to allocate buffer for the
 *        whole byte stream
 *      - Caller of this function MUST take lock. Function is not
 *        implicitly thread-safe
 *
 * @param src_nc
 * @param src_part
 * @param src_addr
 * @param dst_nc
 * @param dst_part
 * @param dst_addr
 * @param buf_len
 *
 * @return int32_t
 */
static int32_t aes_ctr_in_flash(afw_kvs_nonce_counter_t *src_nc,
                                fm_flash_partition_t *src_part,
                                uint32_t src_addr,
                                afw_kvs_nonce_counter_t *dst_nc,
                                fm_flash_partition_t *dst_part,
                                uint32_t dst_addr,
                                const uint32_t buf_len)
{
    if (!src_part || !dst_part) {
        ASD_LOG_E(afw, "Both source and Destination Partitions need to be set");
        return false;
    }

    mbedtls_aes_setkey_enc(&kvs_mgr.kvs_aes_ctx, kvs_mgr.kvs_aes_key, 128);
    unsigned char src_sb[16] = {0};
    size_t src_nc_off = 0;
    unsigned char dst_sb[16] = {0};
    size_t dst_nc_off = 0;
    unsigned char aes_ctr_buf[AES_CTR_BUF_SIZE];

    for (unsigned i = 0; i < buf_len;) {
        uint16_t len = (buf_len - i) > AES_CTR_BUF_SIZE ? AES_CTR_BUF_SIZE : (buf_len - i);
        int32_t ret = 0;

        // Handle src
        ret = KVS_FLASH_READ(src_part, src_addr + i, len, aes_ctr_buf);
        if (ret != len) {
            ASD_LOG_E(afw, "Failed to read from %s\n", src_part->name);
            return ret;
        }

        if (src_nc) {
            if (0 != mbedtls_aes_crypt_ctr(&kvs_mgr.kvs_aes_ctx, len, &src_nc_off,
                                           src_nc->nc_buf, src_sb,
                                           aes_ctr_buf, aes_ctr_buf)) {
                ASD_LOG_E(afw, "Failed to decrypt data");
                return -AFW_EINTRL;
            }
        }

        // Handle dst
        if (dst_nc) {
            if (0 != mbedtls_aes_crypt_ctr(&kvs_mgr.kvs_aes_ctx, len, &dst_nc_off,
                                           dst_nc->nc_buf, dst_sb,
                                           aes_ctr_buf, aes_ctr_buf)) {
                ASD_LOG_E(afw, "failed to encrypt data");
                return -AFW_EINTRL;
            }
        }

        ret = KVS_FLASH_WRITE(dst_part, dst_addr + i, len, aes_ctr_buf);
        if (ret != len) {
            ASD_LOG_E(afw, "Failed to write to %s\n", dst_part->name);
            return ret;
        }

        i += len;
    }

    return AFW_OK;
}

static int32_t aes_ctr_ram_to_flash(const void *data,
                                    uint16_t buf_len,
                                    fm_flash_partition_t *part,
                                    uint32_t dst,
                                    uint16_t *crc,
                                    afw_kvs_nonce_counter_t nc)
{
    // Stack variables for piece-wise AES_CTR encryption
    mbedtls_aes_setkey_enc(&kvs_mgr.kvs_aes_ctx, kvs_mgr.kvs_aes_key, 128);
    unsigned char stream_block[16] = {0};
    size_t nc_off = 0;
    unsigned char aes_ctr_buf[AES_CTR_BUF_SIZE] = {0};

    for (unsigned i = 0; i < buf_len;) {
        uint16_t len = (buf_len - i) > AES_CTR_BUF_SIZE ? AES_CTR_BUF_SIZE : (buf_len - i);

        memcpy(aes_ctr_buf, data + i, len);
        if (0 != mbedtls_aes_crypt_ctr(&kvs_mgr.kvs_aes_ctx, len, &nc_off,
                                       (unsigned char *)&nc, stream_block,
                                       aes_ctr_buf, aes_ctr_buf)) {
            return -AFW_EINTRL;
        }

        *crc = crc16_update(*crc, aes_ctr_buf, len);

        int32_t ret = KVS_FLASH_WRITE(part, dst + i, len, aes_ctr_buf);
        if (ret != len) {
            ASD_LOG_E(afw, "Failed to write encrypted blob to %s\n", part->name);
            return ret;
        }

        i += len;
    }

    return AFW_OK;
}

static int32_t recover_backup()
{
    LOCK_OR_RETURN_ERROR(backup_part);

    // Read in backup_partition header
    afw_kvs_partition_header_t header;
    int ret = KVS_FLASH_READ(backup_part, 0, sizeof(header), &header);
    if (ret != sizeof(header)) {UNLOCK_AND_RETURN(backup_part, ret);}

    // Validate header
    if (!crc_comp_check(header.crc, &header, HEADER_LEN_NO_CRC)) {
        UNLOCK_AND_RETURN(backup_part, AFW_OK);
    }

    // Get target partition
    fm_flash_partition_t *target =
        fm_flash_get_partition_from_address(header.restore_address);
    if (!target) {
        UNLOCK_AND_RETURN(backup_part, -AFW_ENODEV);
    }
    // Macro won't work for this case
    if (afw_kvs_lock(target)) {
        UNLOCK_AND_RETURN(backup_part, -AFW_EBUSY);
    }

    ASD_LOG_I(afw, "Restoring data for %s from backup_partition", target->name);
    header.magic = KVS_MAGIC_BYTE;
    header.crc = crc16_update(0, &header, HEADER_LEN_NO_CRC);

    if (afw_kvs_lock(target)) {
        UNLOCK_AND_RETURN(target, backup_part, -AFW_EBUSY);
    }
    KVS_FLASH_ERASE(target);
    ret = KVS_FLASH_WRITE(target, sizeof(header),
                          target->size - sizeof(header),
                          FLASH_OFFSET(backup_part->offset) + sizeof(header));
    if (ret != (int)(target->size - sizeof(header))) {
        afw_kvs_unlock(target);
        UNLOCK_AND_RETURN(target, backup_part, ret);
    }
    ret = KVS_FLASH_WRITE(target, 0, sizeof(header), &header);
    if (ret != sizeof(header)) {
        afw_kvs_unlock(target);
        UNLOCK_AND_RETURN(target, backup_part, ret);
    }

    if (!crc_comp_check(header.crc, (void *)FLASH_OFFSET(target->offset), HEADER_LEN_NO_CRC)) {
        afw_kvs_unlock(target);
        UNLOCK_AND_RETURN(target, backup_part, ret);
    }

    // invalidate header in backup
    header.restore_address = 0;
    ret = KVS_FLASH_WRITE(backup_part, NONCE_LEN,
                          sizeof(header.restore_address), &header.restore_address);
    if (ret != sizeof(header.restore_address)) {
        afw_kvs_unlock(target);
        UNLOCK_AND_RETURN(target, backup_part, ret);
    }

    afw_kvs_unlock(target);

    UNLOCK_AND_RETURN(target, backup_part, ret);
}

static int32_t afw_kvs_lock(fm_flash_partition_t *partition) {
    bool ret = pdFALSE;
    for (unsigned i = 0; i < KVS_MAX_PARTITION_COUNT; i++) {
        if (partition == kvs_mgr.partition_info[i].partition) {
            ret = xSemaphoreTakeRecursive(kvs_mgr.partition_info[i].write_lock, portMAX_DELAY);
            break;
        }
    }

    if (ret == pdTRUE) {
        return AFW_OK;
    }

    return -AFW_EBUSY;
}

static void afw_kvs_unlock(fm_flash_partition_t *partition) {
    for (unsigned i = 0; i < KVS_MAX_PARTITION_COUNT; i++) {
        if (partition == kvs_mgr.partition_info[i].partition) {
            xSemaphoreGiveRecursive(kvs_mgr.partition_info[i].write_lock);
            return;
        }
    }
}

// Buffer must be set to all zeros before being passed into function
void get_all_namespaces(char *buffer, uint32_t size)
{
    char *buffer_pos = buffer;

    // Handle shared partition
    if (afw_kvs_lock(shared_part)) {return;}
    afw_kvs_entry_meta_t *meta;
    for(uint8_t *scan = (uint8_t *)shared_part->offset + sizeof(afw_kvs_partition_header_t);
        scan < (uint8_t *)shared_part->offset + shared_part->size && *FLASH_OFFSET(scan) != 0xFF;) {
        // Grab meta for current entry (pointed by scan)
        meta = (afw_kvs_entry_meta_t *)FLASH_OFFSET(scan);

        // Ignore default namespace (this is used for CLI and NULL namespace is handled separately)
        if (meta->namespace_len == 0) {
            if (!find_next_entry(&scan, shared_part->offset + shared_part->size)) {
                afw_kvs_unlock(shared_part);
                return;
            }
            continue;
        }

        // Validate meta
        if (!crc_comp_check(meta->meta_crc, meta, META_CRC_COMP_LEN)) {
            if (!find_next_entry(&scan, shared_part->offset + shared_part->size)) {
                afw_kvs_unlock(shared_part);
                return;
            }
            continue;
        }

        // Validate rest of entry
        uint32_t value_crc_comp_len = meta->namespace_len + meta->key_len + meta->value_len + 1;
        if (!crc_comp_check(meta->value_crc, (uint8_t *)meta + sizeof(*meta), value_crc_comp_len)) {
            if (!find_next_entry(&scan, shared_part->offset + shared_part->size)) {
                afw_kvs_unlock(shared_part);
                return;
            }
            continue;
        }


        // Entry is valid. Check that namespace doesn't exist in buffer
        char namespace[meta->namespace_len + 1];
        memcpy(namespace, FLASH_OFFSET(scan) + sizeof(*meta), meta->namespace_len);
        namespace[meta->namespace_len] = '\0';
        char *iterator = buffer;

        while (iterator < buffer_pos) {
            if (!strncmp(namespace, iterator, strlen(namespace) + 1)) {
                break;
            }

            iterator += strlen(iterator) + 1;
        }

        if (iterator == buffer_pos) {
            if (buffer_pos + strlen(namespace) >= buffer + size) {
                afw_kvs_unlock(shared_part);
                return;
            }
            strcpy(buffer_pos, namespace);
            buffer_pos += strlen(buffer_pos) + 1;
        }

        if (!find_next_entry(&scan, shared_part->offset + shared_part->size)) {
            afw_kvs_unlock(shared_part);
            return;
        }
    }
    afw_kvs_unlock(shared_part);

    // Handle rest of flash partition table
    for (int i = 0; i < kvs_mgr.partition_count; i++) {
        if (kvs_mgr.partition_info[i].partition == backup_part ||
            kvs_mgr.partition_info[i].partition == shared_part) {
            continue;
        }
        char *namespace = kvs_mgr.partition_info[i].partition->name;
        if (buffer_pos + strlen(namespace) >= buffer + size) {return;}
        strcpy(buffer_pos, namespace);
        buffer_pos += strlen(buffer_pos) + 1;
    }
}
