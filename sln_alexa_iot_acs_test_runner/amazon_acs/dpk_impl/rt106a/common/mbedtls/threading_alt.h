/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */
/*******************************************************************************
 * Implementation of Threading for free RTOS MBEDTLS
 * follows same format as pthreading version but uses rtos semaphores (threading.h)
 * Used when set MBEDTLS config: MBEDTLS_THREADING_ALT
 *******************************************************************************
 */

#pragma once

#include "FreeRTOS.h"
#include "semphr.h"

typedef struct
{
    SemaphoreHandle_t mutex;
    char is_valid;
} mbedtls_threading_mutex_t;

// register these functions in mbedtls_threading_set_alt
static inline void threading_mutex_init_freertos( mbedtls_threading_mutex_t *mutex )
{
    if( mutex == NULL )
        return;
    mutex->mutex = xSemaphoreCreateMutex();
    mutex->is_valid = mutex->mutex != NULL;
}

static inline void threading_mutex_free_freertos( mbedtls_threading_mutex_t *mutex )
{
    if( mutex == NULL || ! mutex->is_valid )
        return;

    (void) vSemaphoreDelete(mutex->mutex);
    mutex->is_valid = 0;
}

static inline int threading_mutex_lock_freertos( mbedtls_threading_mutex_t *mutex )
{
    if( mutex == NULL || ! mutex->is_valid )
        return( MBEDTLS_ERR_THREADING_BAD_INPUT_DATA );

    if( xSemaphoreTake( mutex->mutex, portMAX_DELAY ) != pdTRUE )
        return( MBEDTLS_ERR_THREADING_MUTEX_ERROR );

    return( 0 );
}

static inline int threading_mutex_unlock_freertos( mbedtls_threading_mutex_t *mutex )
{
    if( mutex == NULL || ! mutex->is_valid )
        return( MBEDTLS_ERR_THREADING_BAD_INPUT_DATA );

    if( xSemaphoreGive( mutex->mutex ) != pdTRUE )
        return( MBEDTLS_ERR_THREADING_MUTEX_ERROR );

    return( 0 );
}
