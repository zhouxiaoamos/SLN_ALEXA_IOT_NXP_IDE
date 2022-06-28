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
 * Flash Manager
 *******************************************************************************
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "flash_manager.h"
#include "afw_error.h"
#include "flash_map.h"
#include "semphr.h"
#include "iot_flash.h"
#include "afw_def.h"
#include "efuse_manager.h"

/* #define FM_DEBUG_PRINT */

#ifdef FM_DEBUG_PRINT
#define FM_DEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define FM_DEBUG(fmt, ...)
#endif

#define DEFAULT_FLSH_INSTANCE   1
#define INITIAL_DYNAMIC_ARRAY_SIZE 5


/**
 * NXP: Debug excerpt from mock_flash_manager.c
 */
uint8_t*  __end_asd_log_module_filters = NULL;


/* data structure to hold flash erase callback information*/
typedef struct flash_callback_data_s {
    int client_id;
    fm_flash_callback client_callbk;
    void * client_contx;
} flash_callback_data_t;

/* Dynamic array type */
typedef struct array_s{
  flash_callback_data_t *array;
  size_t used;
  size_t size;
} array_t;

/*AFR HAL flash handle*/
IotFlashHandle_t gIotFlashHandle = NULL;

/* AFR HAL flash handle big-lock */
static SemaphoreHandle_t flash_bl = NULL;

/* semaphore to use in conjunction with lock if async erase is enabled */
static SemaphoreHandle_t flash_async_sem = NULL;

static struct fm_flash_partition *flash_partitions = NULL;
static int flash_partitions_count = 0;

static bool use_asynchronous_erase = false;

/* Dynamic array for storing erase callbacks registered by clients*/
static array_t callback_array;
/* array index for erase callback to be invoked after current erase completes */
static int callback_index = -1;
/* next unique client id to give out */
static int next_client_id = 0;

/* Conver a iot_flash error to a afw_error */
static int error_code_convert(int afr_code)
{
    if (afr_code == IOT_FLASH_SUCCESS) {
        return AFW_OK;
    } else if (afr_code == IOT_FLASH_INVALID_VALUE) {
        return -AFW_EINVAL;
    } else if (afr_code == IOT_FLASH_DEVICE_BUSY) {
        return -AFW_EBUSY;
    } else {
        return -AFW_EUFAIL;
    }
}

/*release AFR HAL flash handle*/
static void fm_flash_unlock(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        //If scheduler is not running, just no lock for flash access.
        return;
    }

    if (flash_async_sem) {
        xSemaphoreGive(flash_async_sem);
    }

    if (flash_bl) {
        xSemaphoreGive(flash_bl);
    }
}

/**
 * We may or may not be able create semaphore depending on whether OS
 * is already running. If semaphore cannot be created during flash
 * manager initialization, flash manager can still operate without
 * semophore for clients like crash dump. Additional attempts will
 * be made to create semophore each time when fm_flash_lock() is called
 * until it is created.
 */
static void fm_flash_lock_init(void)
{
#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        if (!flash_bl) {
            flash_bl = xSemaphoreCreateMutex();
        }

        if (use_asynchronous_erase) {
            flash_async_sem = xSemaphoreCreateBinary();
            /**
             * binary semaphore created using xSemaphoreCreateBinary()
             * must be "give" before it can be "taken"
             */
            if (flash_async_sem) {
                xSemaphoreGive(flash_async_sem);
            }
        }
    }
#endif
}

/*lock AFR HAL flash handle*/
static int32_t fm_flash_lock(void)
{
    bool ret;

    if (!flash_bl) {
        fm_flash_lock_init();
        if (!flash_bl) {
            return AFW_OK;
        }
    }

    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        //If scheduler is not running, just no lock for flash access.
        return AFW_OK;
    }

    ret = xSemaphoreTake(flash_bl, portMAX_DELAY);

    // this semaphore will only exist if we have async erase enabled
    if (flash_async_sem) {
        xSemaphoreTake(flash_async_sem, portMAX_DELAY);
    }

    return (ret == pdTRUE) ? AFW_OK : -AFW_EBUSY;
}


/**
 * @brief check if a partition is writeable when part is not NULL.
 *        check if sector(s) are writeable when part is NULL.
 *
 * @param part partition to check when it is not NULL.
 * @param start the offset from the flash base address. Must be sector aligned.
 *        This parameter is used only when 'part' is NULL.
 * @param len Number of bytes to check. Must be setctor-aligned.
 *        This parameter is used only when 'part' is NULL.
 *
 * @return true if partition/all sectors are writeable.
 *         false if partition/any sector is not writeable.
 *         false if any error occured during the check process.
 */
static bool check_flash_area_writeable( struct fm_flash_partition *part,
                                     unsigned long start,
                                     uint32_t len)
{
    int i;
    uint32_t end = start + len;
    /* NXP: set production_device to false; We do not use any fuses for production devices */
    bool production_device = false;
    bool device_type_unknown = false;

    /* NXP: disabled efuse check since we do not have an efuse hal implemented  */
    /*if (IOT_EFUSE_SUCCESS != efuse_mgr_is_device_production_device(&production_device)) {
        device_type_unknown = true;
    }*/

    /* partition check */
    if (part) {
        if (device_type_unknown) {
            /* if we can't tell its a production or non-production device,
             * either flag not set will cause partition access denial.
             */
            if (!(part->flags & FM_FLASH_SECURED_WRITEABLE) ||
                !(part->flags & FM_FLASH_UNSECURED_WRITEABLE)) {
                return false;
            }
        } else if (production_device) {
            if (!(part->flags & FM_FLASH_SECURED_WRITEABLE)) {
                return false;
            }
        } else {
            if (!(part->flags & FM_FLASH_UNSECURED_WRITEABLE)) {
                return false;
            }
        }
        return true;
    }

    /* sector(s) check */
    for (i = 0; i < flash_partitions_count; i++) {

        if ((flash_partitions[i].offset + flash_partitions[i].size) <= start) {
            continue;
        }

        if (flash_partitions[i].offset >= end) {
            break;
        }

        if (device_type_unknown) {
            /* if we can't tell its a production or non-production device,
             * either flag not set will cause partition access denial.
             */
            if (!(flash_partitions[i].flags & FM_FLASH_SECURED_WRITEABLE) ||
                !(flash_partitions[i].flags & FM_FLASH_UNSECURED_WRITEABLE)) {
                return false;
            }
        } else if (production_device) {
            if (!(flash_partitions[i].flags & FM_FLASH_SECURED_WRITEABLE)) {
                return false;
            }
        } else {
            if (!(flash_partitions[i].flags & FM_FLASH_UNSECURED_WRITEABLE)) {
                return false;
            }
        }
    }

    return true;
}

/* sort partition talble acording to offset addresses. */
static void sort_partitions(void)
{
    int i, j;
    struct fm_flash_partition temp;

    if (!flash_partitions || flash_partitions_count < 2)
        return;

    for(i=1; i<flash_partitions_count; i++){
        for(j=0; j<flash_partitions_count-i; j++){
            if(flash_partitions[j].offset > flash_partitions[j+1].offset){
                memcpy((uint8_t *)&temp, (uint8_t *)&flash_partitions[j], sizeof(struct fm_flash_partition));
                memcpy((uint8_t *)&flash_partitions[j], (uint8_t *)&flash_partitions[j+1], sizeof(struct fm_flash_partition));
                memcpy((uint8_t *)&flash_partitions[j+1], (uint8_t *)&temp, sizeof(struct fm_flash_partition));
            }
        }
    }
}

/* Initialize callback array to a given initial size */
void init_array(array_t *a, size_t initialSize) {
  a->array = (flash_callback_data_t *)malloc(initialSize * sizeof(flash_callback_data_t));
  a->used = 0;
  a->size = initialSize;
}

/*
 * Lookup the array index of the callback entry that matches the client id
 * return array index if found
 * return -1 if not found
 */
int search_array(array_t *a, int id) {
    for (uint32_t i=0; i<a->used; i++) {
        if ((a->array[i]).client_id == id) {
            return i;
        }
    }
    return -1;
}

/*
 * Add a callback entry
 * Replace an existing entry is it has the same client_id.
 * Otherwise, add as a new entry.
 *
 */
void insert_array(array_t *a, flash_callback_data_t element) {
  int idx = search_array(a, element.client_id);
  if (idx >= 0) {
    a->array[idx] = element;
  } else {
    if (a->used == a->size) {
      a->size += 2;
      a->array = (flash_callback_data_t *)realloc(a->array, a->size * sizeof(flash_callback_data_t));
    }
    a->array[a->used++] = element;
  }
}

void freeArray(array_t *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}

/**
 * callback_delegate is executed each time an asynchronous flash erase is completed.
 * It inturn invokes the callback function previously set by current erase caller.
 * Nothing will be done if no callback is set by current erase caller.
 */
void callback_delegate(IotFlashOperationStatus_t status, void * context)
{
    if ( (callback_index >= 0) && (callback_array.array[callback_index].client_callbk) ) {
        callback_array.array[callback_index].client_callbk(status, callback_array.array[callback_index].client_contx);
    }

    if (flash_async_sem) {
        xSemaphoreGive(flash_async_sem);
    }
}

int fm_flash_init(struct fm_flash_partition *parts, int nr_parts)
{
    int i;

    if (!parts || nr_parts <= 0) {
        return -AFW_EINVAL;
    }

    if (nr_parts > FM_FLASH_MAX_PARTITIONS) {
        return -AFW_EINVAL;
    }

    fm_flash_lock_init();

    if (!gIotFlashHandle) {
        gIotFlashHandle = iot_flash_open( DEFAULT_FLSH_INSTANCE );
        if (!gIotFlashHandle) {
            return -AFW_EUFAIL;
        }
    }

// IOT_TODO: Call fm_flash_getinfo() to find out if flash uses asynchronous
// erease or not, and set static variable use_asynchronous_erase accordingly.
// https://issues.labcollab.net/browse/FWPLATFORM-653.
    if (use_asynchronous_erase) {
        init_array(&callback_array, INITIAL_DYNAMIC_ARRAY_SIZE);
        iot_flash_set_callback(gIotFlashHandle,
                               (fm_flash_callback) callback_delegate,
                               NULL);
    }

    flash_partitions = parts;
    flash_partitions_count = nr_parts;
    sort_partitions();

    for (i = 0; i < flash_partitions_count; i++) {
        flash_partitions[i].busy = false;
    }

    return AFW_OK;
}

/**
 * Read from offset from partition base address when 'part' is not NULL
 * Read from offset from flash device base address when 'part' is NULL
 */
int fm_flash_read(struct fm_flash_partition *part,
                        int client_id,
                        unsigned long from,
                        uint32_t len,
                        uint8_t *buf)
{
    int ret;

    if (!gIotFlashHandle || len == 0 || !buf) {
        return -AFW_EINVAL;
    }

    if (part) {
        if (from + len > part->size) {
            return -AFW_EFAULT;
        }
        from = from + part->offset;
    }

    ret = fm_flash_lock();
    if (ret) {
        return ret;
    }

    ret = iot_flash_read_sync(gIotFlashHandle,
                              from,
                              (void * const)buf,
                              (size_t)len);

    fm_flash_unlock();

    if(!ret) {
        return len;
    } else {
        return error_code_convert(ret);
    }
}

/**
 * Write to offset from partition base address when 'part' is not NULL
 * Write to offset from flash device base address when 'part' is NULL
 */
int fm_flash_write( struct fm_flash_partition *part,
                        int client_id,
                        unsigned long to,
                        uint32_t len,
                        const uint8_t *buf)
{
    int ret;

    if (!gIotFlashHandle || len == 0 || !buf) {
        return -AFW_EINVAL;
    }

    if (part) {
        if (to + len > part->size) {
            return -AFW_EFAULT;
        }
        to = to + part->offset;
    }

    if (!check_flash_area_writeable(part, to, len)) {
            return -AFW_EROFS;
    }

    ret = fm_flash_lock();
    if (ret) {
        return ret;
    }

    ret = iot_flash_write_sync(gIotFlashHandle,
                                    to,
                                    (void * const)buf,
                                    (size_t)len);

    fm_flash_unlock();

    if(!ret) {
        return len;
    }else {
        return error_code_convert(ret);
    }
}

/**
 * Erase a partition.
 * Erase entire chip is not allowed so 'part' must not be NULL.
 */
int fm_flash_erase(struct fm_flash_partition *part, int client_id)
{
    if (!gIotFlashHandle) {
        return -AFW_EINVAL;
    }

    if (!part) {
        return -AFW_EINVAL;
    }

    return fm_flash_erase_sectors(part,
                                  client_id,
                                  0,
                                  part->size);

}

int fm_flash_erase_sectors(struct fm_flash_partition *part,
                           int client_id,
                           size_t offset,
                           size_t xBytes)
{
    int ret = IOT_FLASH_SUCCESS;

    if (!gIotFlashHandle) {
        return -AFW_EINVAL;
    }

    if (part) {
        if ( (xBytes == 0) || (offset + xBytes > part->size) ) {
            return -AFW_EFAULT;
        }

        offset = part->offset + offset;
    }

    if (!check_flash_area_writeable(part, offset, xBytes)) {
        return -AFW_EROFS;
    }

    if (use_asynchronous_erase) {
        ret = fm_flash_lock();
        if (ret) {
            return ret;
        }

        /* Set erase callback to be invoked after this erase */
        callback_index = search_array(&callback_array, client_id);

        ret = iot_flash_erase_sectors(gIotFlashHandle,
                                      offset,
                                      xBytes);

        // for asynchronous erase give up the mutex, but keep the semaphore
        if (flash_bl) {
            xSemaphoreGive(flash_bl);
        }
    } else {
        int step_size = 0;
        size_t end = offset + xBytes;

        for (; offset < end; offset += step_size) {

            /* NXP update: we are handling ourselves the step sizes in erase_region function */
            /*
            ret = fm_flash_get_max_erase_block_size(offset, end - offset, &step_size);
            if (ret != AFW_OK)
                return ret;
            */

            step_size = xBytes;

            FM_DEBUG("erase %08x, %d\n", offset, step_size);

            ret = fm_flash_lock();
            if (ret) {
                return ret;
            }

            ret = iot_flash_erase_sectors(gIotFlashHandle,
                                          offset,
                                          step_size
                                          );

            fm_flash_unlock();

            if (ret != IOT_FLASH_SUCCESS) {
                break;
            }
        }
    }

    return error_code_convert(ret);
}

int fm_flash_set_callback(int client_id,
                          fm_flash_callback call_back,
                          void* client_context)
{
    int ret;
    flash_callback_data_t data = {client_id, call_back, client_context};

    /* call back is only supported when asynchronous erase is supported */
    if (!use_asynchronous_erase) {
        return -AFW_EUNSUP;
    }


    if (!call_back || client_id <= FM_BYPASS_CLIENT_ID) {
        return -AFW_EINVAL;
    }

    ret = fm_flash_lock();
    if (ret) {
        return ret;
    }

    insert_array(&callback_array, data);

    fm_flash_unlock();

    return AFW_OK;
}

fm_flash_info* fm_flash_getinfo()
{
    int ret;
    fm_flash_info* flash_info;

    ret = fm_flash_lock();
    if (ret) {
        return NULL;
    }

    flash_info = iot_flash_getinfo(gIotFlashHandle);

    fm_flash_unlock();

    return flash_info;
}

int32_t fm_flash_ioctl(fm_flash_ioctl_request xRequest,
                        size_t xBytes,
                        void * const pvBuffer)
{
    int32_t ret;

    ret = fm_flash_lock();
    if (ret) {
        return ret;
    }

    ret = iot_flash_ioctl(gIotFlashHandle,
                          xRequest,
                          pvBuffer);
    fm_flash_unlock();

    return error_code_convert(ret);
}

int fm_flash_get_unique_client_id()
{
    return next_client_id++;
}

struct fm_flash_partition *fm_flash_get_partition(const char *name)
{
    int i;

    for (i = 0; i < flash_partitions_count; i++) {
        if (!strncmp(flash_partitions[i].name, name, strlen(name) + 1)) {
            return &flash_partitions[i];
        }
    }

    return NULL;
}

struct fm_flash_partition *fm_flash_get_partition_from_address(const int64_t addr)
{
    int i;

    if (addr < 0) {
        return NULL;
    }

    for (i = 0; i < flash_partitions_count; i++) {
        if (flash_partitions[i].offset == addr) {
            return &flash_partitions[i];
        }
    }

    return NULL;
}

struct fm_flash_partition *fm_flash_get_partition_table(int *size)
{
    *size = flash_partitions_count;
    return &flash_partitions[0];
}

int fm_flash_get_max_erase_block_size(uint32_t offset, uint32_t size, int *blocksize)
{
    // NXP: Add support for 256KB Hyperflash sectors
    if ((offset % FM_FLASH_ERASE_SIZE_256K) == 0 &&
        size >= FM_FLASH_ERASE_SIZE_256K) {
        *blocksize = FM_FLASH_ERASE_SIZE_256K;
        return AFW_OK;
    } else if ((offset % FM_FLASH_ERASE_SIZE_64K) == 0 &&
        size >= FM_FLASH_ERASE_SIZE_64K) {
        *blocksize = FM_FLASH_ERASE_SIZE_64K;
        return AFW_OK;
    } else if ((offset % FM_FLASH_ERASE_SIZE_32K) == 0 &&
               size >= FM_FLASH_ERASE_SIZE_32K) {
        *blocksize = FM_FLASH_ERASE_SIZE_32K;
        return AFW_OK;
    } else if ((offset % FM_FLASH_ERASE_SIZE_4K) == 0 &&
               size >= FM_FLASH_ERASE_SIZE_4K) {
        *blocksize = FM_FLASH_ERASE_SIZE_4K;
        return AFW_OK;
    }

    return AFW_EINVAL;
}
