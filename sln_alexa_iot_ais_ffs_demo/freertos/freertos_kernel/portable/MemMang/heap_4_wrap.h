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
 * heap_4_wrap.h
 *******************************************************************************
 */

#pragma once
#include <stdlib.h>

#include "FreeRTOS.h"

// this must match A_BLOCK_LINK from heap_4.c (or it won't compile due to compile checks in heap_4_wrap.c)
// if you modify this make sure to add checks in heap_4_wrap.c for any new fields (or remove old fields)
typedef struct B_BLOCK_LINK
{
    struct B_BLOCK_LINK *pxNextFreeBlock;    /*<< The next free block in the list. */
    size_t xBlockSize;                        /*<< The size of the free block. */
    #if( configHEAP_TASK_ACCOUNTING == 1 )
        TaskToken_t xTag;
    #endif
} xBlockLink_t;

size_t getHeapStructSize( void );
size_t getBlockAllocatedBit( void );

/**
 * @brief Walk through heap chunks and print out usage info on each chunk
 * in the format:
 *
 * 1) configHEAP_TASK_ACCOUNTING not enabled
 * start_address  end_address  size  allocated
 *
 * 2) configHEAP_TASK_ACCOUNTING enabled
 * start_address  end_address  size  allocated  task_name  task_number
 *
 * @param [in] print_func: callback of print function. See definition of printf.
 *              The callback should return number of bytes printed, or negative for
 *              error.
 */
void walkHeap( int (*print_func)(const char*, ...) );
