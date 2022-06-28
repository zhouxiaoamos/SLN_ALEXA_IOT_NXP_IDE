/*
 * Copyright 2021 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <string.h>
#include "FreeRTOS.h"
#include "platform_key.h"

/* TODO: it would be preferable and more secure to generate a random encryption key.
 * For the moment we're using the same static key on all devices. */
#define KVS_AES_KEY_VALUE {0xa5}
#define KVS_AES_KEY_SIZE  16

int platform_key_setup_kvs_key(unsigned char **key)
{
    int status;
    unsigned char kvs_aes_key[KVS_AES_KEY_SIZE] = KVS_AES_KEY_VALUE;

    if (key != NULL)
    {
        status = PLATFORM_KEY_SUCCESS;
    }
    else
    {
        status = PLATFORM_KEY_FAIL;
    }

    if (status == PLATFORM_KEY_SUCCESS)
    {
        *key = (unsigned char *)pvPortMalloc(KVS_AES_KEY_SIZE);
        if (*key != NULL)
        {
            memcpy((*key), kvs_aes_key, KVS_AES_KEY_SIZE);
        }
        else
        {
            status = PLATFORM_KEY_FAIL;
        }
    }

    return status;
}
