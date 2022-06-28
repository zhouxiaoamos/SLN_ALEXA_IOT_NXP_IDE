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
 * heap_4_wrap.c
 *******************************************************************************
 */


// include the .c file to add our changes on top of it. the .c will only be compiled here.
#include "heap_4_wrap.h"
#include "heap_4.c"

#include <assert.h>

// make sure our xBlockLink_t is the same as the one from heap_4.c (BlockLink_t)
_Static_assert( sizeof(BlockLink_t) == sizeof(xBlockLink_t), "xBlockLink_t must match BlockLink_t from heap_4.c");

_Static_assert(offsetof(xBlockLink_t, pxNextFreeBlock) == offsetof(BlockLink_t, pxNextFreeBlock), "pxNextFreeBlock Location must be same in BlockLink_t");
_Static_assert(offsetof(xBlockLink_t, xBlockSize) == offsetof(BlockLink_t, xBlockSize), "xBlockSize Location must be same in BlockLink_t");


size_t getHeapStructSize( void )
{
    return xHeapStructSize;
}
size_t getBlockAllocatedBit( void )
{
    if (xBlockAllocatedBit == 0)
    {
        xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
    }

    return xBlockAllocatedBit;
}

/* NXP: we don't need this function and it triggers a compilation error, commenting it */
#if 0
/*
 * Walk through heap chunks and print out usage info on each chunk
 */
void walkHeap(int (*print_func)(const char*, ...))
{
size_t uxHeapStart;
size_t uxHeapEnd;
size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;
bool xUsed;

    /* Need to align with heap_4.c impl. to calculate starting chunk address in heap*/
    uxHeapStart = ( size_t ) ucHeap;
    if( ( uxHeapStart & portBYTE_ALIGNMENT_MASK ) != 0 )
    {
        uxHeapStart += ( portBYTE_ALIGNMENT - 1 );
        uxHeapStart &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
        xTotalHeapSize -= uxHeapStart - ( size_t ) ucHeap;
    }

    // Calcuate heap end to make sure to be the same with heap_4.c
    uxHeapEnd = ( ( size_t ) uxHeapStart ) + xTotalHeapSize;
    uxHeapEnd -= xHeapStructSize;
    uxHeapEnd &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
    if ( ( void * )uxHeapEnd != pxEnd ) {
        print_func("Heap size doesn't match internally\n");
        return;
    }

    uint8_t *ptr = ( uint8_t * )uxHeapStart;
    print_func("Heap starts @ %p\n", ptr);
    print_func("%s\t%s\n", "size", "task id");
    while (ptr < ( uint8_t * )pxEnd) {
        char* pcTaskName = NULL;
        UBaseType_t uxTaskNumber = 0;

        xBlockLink_t* header = ( xBlockLink_t * ) ptr;
        xUsed = (header->xBlockSize & xBlockAllocatedBit ) != 0;
        size_t blockSize = (header->xBlockSize & ~xBlockAllocatedBit);

        #if( (configHEAP_TASK_ACCOUNTING == 1) )
            if (xUsed)
            {
                uint32_t ulIndex;
                TaskTableEntry_t* xEntry;
                ulIndex = TASK_TOKEN_TO_INDEX(header->xTag);
                if (ulIndex < configMAX_TASK_TABLE_SIZE)
                {
                    xEntry = &xTaskTable[ulIndex];
                    pcTaskName = xEntry->pcTaskName;
                    uxTaskNumber = xEntry->uxTaskNumber;
                } else {
                    // heap block header corruption. Can't recover so far, so give
                    // it up.
                    print_func("Heap block corruption. End walking.\n");
                    return;

                }
            }
        #endif

        if( xUsed ) {
            // if allocated, print task id.
            print_func("%d\t%ld\n", blockSize, uxTaskNumber);
        }
        else
        {
            // if not allocated, no task id.
            print_func("%d\n", blockSize);
        }

        ptr += blockSize;
    }
}
#endif
