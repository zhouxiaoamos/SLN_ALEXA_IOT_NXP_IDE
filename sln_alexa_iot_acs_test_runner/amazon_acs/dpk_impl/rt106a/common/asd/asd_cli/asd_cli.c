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
 * Command-Line-Interface (CLI) commands Acorn
 *******************************************************************************
 */

#include "asd_cli.h"

#include <ace/aceCli.h>
#include <ace/ace_log.h>
#include <asd_crc32.h>

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <time.h>

#include "fsl_debug_console.h"

#define CLI_BUF_SIZE  256

// the max size will allow the buffer to be to keep memory from
// running out unexpectedly
#define CLI_BUF_MAX_SIZE (6144)
#define CLI_TASK_PRIORTY ((configMAX_PRIORITIES - 5) | portPRIVILEGE_BIT)

#define CLI_PROMPT "-->"

ace_status_t checkCRC(int32_t argc, const char** argv) {
    // first arg is crc num  (0x42)
    // second ard is original command
    if (argc != 2)
    {
        PRINTF("Expected 2 args\r\n");
        return ACE_STATUS_CLI_FUNC_ERROR;
    }

    const char *crcStr = argv[0];
    char *cmd = (char*) argv[1]; // getting rid of const to avoid copy, this could be unsafe

    char *endPtr = NULL;
    // number can be 0xhhhh or ddd
    uint32_t crc = (uint32_t) strtoul(crcStr, &endPtr, 0);
    if (endPtr == NULL || *endPtr != '\0')
    {
        PRINTF("expected a number for crc, was: <%s> <%s>\r\n", crcStr, endPtr);
        return ACE_STATUS_CLI_FUNC_ERROR;
    }

    uint32_t resultCrc = asd_crc32_ietf((uint8_t*)cmd, strlen(cmd));
    if (resultCrc != crc)
    {
        ACE_LOGI(ACE_LOG_ID_SYSTEM, "cli", "CRC did not match expected value, command ignored");
        return ACE_STATUS_OK;
    }

    PRINTF("calling: %s\r\n", cmd);
    aceCli_processCmd(cmd);

    return ACE_STATUS_OK;
}

/**
 * allows for overriding the default size of the buffer, only applies if > CLI_BUF_SIZE
 */
uint32_t g_baseBufSize = 0;
ace_status_t setSize(int32_t argc, const char** argv) {
    if (argc != 1)
    {
        PRINTF("Expected 1 arg\r\n");
        return ACE_STATUS_CLI_FUNC_ERROR;
    }

    const char *sizeStr = argv[0];
    // if atoi fails, it will just return 0, and we ignore anything less than CLI_BUF_SIZE
    int size = atoi(sizeStr);

    if (size < CLI_BUF_SIZE || size > CLI_BUF_MAX_SIZE)
    {
        PRINTF("Error: expected size [%u,%u], got %s\r\n", CLI_BUF_SIZE, CLI_BUF_MAX_SIZE, sizeStr);
    }
    else
    {
        g_baseBufSize = (uint32_t)size;
        PRINTF("overridding cli buffer size to :%lu\r\n", g_baseBufSize);
    }

    return ACE_STATUS_OK;
}

const aceCli_moduleCmd_t rtos_cli_sub[] = {
                            {"crc", "run command with crc; use: crc 0x<crc> \"cmd\"", ACE_CLI_SET_LEAF, .command.func=&checkCRC},
                            {"size", "set the initial size of the cli read buffer", ACE_CLI_SET_LEAF, .command.func=&setSize},
                            ACE_CLI_NULL_MODULE
                            };

/**
 * Resets the buffer back to the desiredSize and frees any memory not needed.
 *
 * Switches between malloc'd version and stack version based on the desired size.
 *
 * @param bufPtr[in/out] ptr to the buf to resize
 * @param currentSize the current size of bufPtr
 * @param desiredSize the desired size of bufPtr
 * @param staticBuf the original buffer defined on stack to control, malloc vs realloc
 * @return the new size of bufPtr
 */
uint32_t resetBufferToSize(char **bufPtr, uint32_t currentSize, uint32_t desiredSize, char *staticBuf)
{
    uint32_t newSize = desiredSize;
    if (currentSize != desiredSize)
    {
        if (desiredSize == CLI_BUF_SIZE)
        {
            // if staticBuf already no need to do anything (shouldn't happen)
            if (*bufPtr != staticBuf) {
                free(*bufPtr);
                *bufPtr = staticBuf;
            }
        }
        else
        {
            if (*bufPtr != staticBuf)
            {
                char *newBuf = (char*)realloc(*bufPtr, desiredSize);
                if (newBuf == NULL)
                {
                    ACE_LOGI(ACE_LOG_ID_SYSTEM, "cli",
                             "No space available for buffer of size %u falling back to %u", desiredSize, CLI_BUF_SIZE);
                    free(*bufPtr);
                    *bufPtr = staticBuf;
                    newSize = CLI_BUF_SIZE;
                }
                else
                {
                    *bufPtr = newBuf;
                }
            }
            else
            {
                *bufPtr = (char*)malloc(desiredSize);
                if(*bufPtr == NULL)
                {
                    ACE_LOGI(ACE_LOG_ID_SYSTEM, "cli",
                             "No space available for buffer of size %u falling back to %u", desiredSize, CLI_BUF_SIZE);
                    *bufPtr = staticBuf;
                    newSize = CLI_BUF_SIZE;
                }
            }
        }
    }
    return newSize;
}

/**
 * Takes in current size of buffer and current buffer use and doubles the size
 * if more space is needed (up to CLI_BUF_MAX_SIZE)
 * @param bufPtr[in/out] ptr to the buf to resize
 * @param currentSize the current size of *bufPtr
 * @param useSize the current use of *bufPtr
 * @param staticBuf the original buffer defined on stack to control, malloc vs realloc
 */
uint32_t resizeBufferIfNeeded(char **bufPtr, uint32_t currentSize, uint32_t useSize, char *staticBuf)
{
    uint32_t resultSize = currentSize;
    if (useSize >= currentSize)
    {
        // double the space each time
        uint32_t newBufSize = currentSize*2;
        if (newBufSize > CLI_BUF_MAX_SIZE)
        {
            // maybe we can't double, but the max may be larger than the current.
            newBufSize = CLI_BUF_MAX_SIZE;
            if (newBufSize  <= currentSize)
            {
                ACE_LOGI(ACE_LOG_ID_SYSTEM, "cli", "CLI entry is too large %u, truncating", newBufSize);
                return resultSize;
            }
        }

        resultSize = resetBufferToSize(bufPtr, currentSize, newBufSize, staticBuf);
        if (currentSize == CLI_BUF_SIZE &&
            newBufSize != CLI_BUF_SIZE &&
            resultSize == newBufSize)
        {
            // we switched from stack to heap so copy the data.
            memcpy(*bufPtr, staticBuf, CLI_BUF_SIZE);
        }
    }
    return resultSize;
}
void asd_rtos_cli_loop(void *parameters)
{
    // by default we use a static buf, but if the user enters a long command we will temprorily switch to heap
    // or if user increases size via cli command: 'cli size' then we will also switch to heap,
    // until user sets size back to CLI_BUF_SIZE
    static char staticBuf[CLI_BUF_SIZE] = {0};
    char *buf = staticBuf;
    g_baseBufSize = CLI_BUF_SIZE; // the size of buf to start reading with (increase with "rtos size <size>" cli command)
    uint32_t bufSize = g_baseBufSize;

    uint32_t count = 0;
    char ch = '\0';
    ACE_LOGI(ACE_LOG_ID_SYSTEM, "cli", "starting asd_rtos_cli loop");

    PRINTF(CLI_PROMPT);

    bool isInQuote = false;

    while (1)
    {
        ch = GETCHAR();
        if (ch == 0xff) {
            // do nothing (loop again)
            // 0xff is what getchar returns on failure, here we don't care why it failed
            // buf if you have uart issues not coming in, you might want to print out
            // errno here to figure out why.
            // we don't print here since in errorcase it might spam a lot.
            continue;
        }

#ifdef ECHO_IN_CLI
//        PUTCHAR(ch);
#endif

        if (ch == '\"')
        {
            isInQuote = !isInQuote;
        }
        if (!isInQuote && (ch == '\r' || ch == '\n'))
        {
            if (ch == '\r')
            {
                ch = '\r';
                PUTCHAR(ch);
                ch = '\n';
                PUTCHAR(ch);
            }

            buf[count] = '\0';
            if (count > 0)
            {
                clock_t startTimeMs = clock();
                aceCli_processCmd(buf);
                clock_t endTimeMs = clock();
                PRINTF("cli duration %" PRIu32 "ms\r\n", (uint32_t)(endTimeMs - startTimeMs));
                count = 0;
                bufSize = resetBufferToSize(&buf, bufSize, g_baseBufSize, staticBuf);
                PRINTF("\r\n");
            }

            PRINTF(CLI_PROMPT);
        }
        else if (ch == '\b' || ch == '\x7f') // backspace or delete
        {
            if (count > 0)
            {
                PUTCHAR(' ');
                PUTCHAR('\b');
                --count;
            }
            // would be nice to move cursor to the right if went over the prompt
            // but the echo and the putchar are not in sync.  This code runs after a new line is buffered.
        }
        //Unspported features that could be added in future:
        //  [ESC] to abort the current line support
        //  [TAB] to get a list of commands beginning with what we've typed so far
        else
        {
            buf[count] = ch;
            ++count;
            bufSize = resizeBufferIfNeeded(&buf, bufSize,  count, staticBuf);
            if (count >= bufSize)
            {
                count = bufSize-1;
            }
        }
    }
}

int asd_rtos_cli_init(const aceCli_moduleCmd_t* moduleCmdList, uint32_t cli_stack_size)
{
    aceCli_setCmdList(moduleCmdList);

    return (xTaskCreate(asd_rtos_cli_loop,
                        "cli",
                        cli_stack_size / sizeof(portSTACK_TYPE),
                        (void*)NULL,
                        CLI_TASK_PRIORTY,
                        NULL) == pdPASS ?
            0 : -1);
}

