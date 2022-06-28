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
 * @file heap_4_ext.c
 *
 * @brief provides implementation for memory functions
 *******************************************************************************
 */
#include "FreeRTOS.h"
#include "task.h"

#include "heap_4_wrap.h"
#include <string.h>


// These functions cannot be defined in heap_4_wrap.c because it defines
// pvPortMalloc and vPortFree which may be wrapped, which will result in the wrong function called.
/*-----------------------------------------------------------*/

void* pvPortCalloc(size_t nmemb, size_t size)
{
    if (nmemb == 0 || size == 0) {
        return NULL;
    } else if (((nmemb * size) / nmemb) != size) {
        return NULL;
    }

    void* pvReturn;
    pvReturn = pvPortMalloc(nmemb * size);
    if (pvReturn) {
        memset(pvReturn, 0, nmemb * size);
    }

    return pvReturn;
}

void *pvPortRealloc( void *pv, size_t size )
{
    void *pvReturn = NULL;
    size_t xBlockSize = 0;
    uint8_t *puc = (uint8_t *) pv;
    xBlockLink_t *pxLink = NULL;

    if(size) {
        pvReturn = pvPortMalloc(size);
        if (pvReturn) {
            memset(pvReturn, 0, size);
        }
    }

    //if( (pv != NULL) && (pvReturn != NULL) )
    if(pv != NULL) {
        // The memory being freed will have an xBlockLink_t structure immediately before it.
        puc -= getHeapStructSize();

        // This casting is to keep the compiler from issuing warnings.
        pxLink = ( void * ) puc;

        // Check the block is actually allocated
        configASSERT((pxLink->xBlockSize & getBlockAllocatedBit()) != 0);
        configASSERT(pxLink->pxNextFreeBlock == NULL);

        // Get Original Block Size
        xBlockSize = (pxLink->xBlockSize & ~(getBlockAllocatedBit()));

        // Get Original data length
        xBlockSize = (xBlockSize - getHeapStructSize());

        if(xBlockSize < size) {
            memcpy(pvReturn, pv, xBlockSize);
        } else {
            memcpy(pvReturn, pv, size);
        }

        // Free Original Ptr
        vPortFree(pv);
    }

    return pvReturn;
}
