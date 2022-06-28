/*
 * Copyright 2018, 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "ais_continuous_utterance.h"

__attribute__((section(".ocram_non_cacheable_bss"))) static uint8_t ring_buffer[RING_BUFFER_SIZE];

/*
 * Here reuse s_outputStream buffer instead of allocating a new big buffer.
 */
static uint8_t *aux_backup_buffer;
static uint32_t aux_backup_size;

static uint32_t ring_buffer_index;

static void buffer_shift_left(uint32_t shift_size);
static void buffer_shift_right(uint32_t shift_size);

uint8_t continuous_utterance_samples_add(uint8_t *samples_buffer, uint32_t samples_buffer_len)
{
    if (samples_buffer_len > RING_BUFFER_SIZE || !samples_buffer)
    {
        return 1;
    }

    for (int i = 0; i < samples_buffer_len; i++)
    {
        if (ring_buffer_index == RING_BUFFER_SIZE)
        {
            ring_buffer_index = 0;
        }
        ring_buffer[ring_buffer_index++] = samples_buffer[i];
    }

    return 0;
}

uint8_t continuous_utterance_buffer_set(uint8_t **buffer, uint32_t *buffer_len, uint16_t wake_word_started)
{
    uint32_t ais_buffer_size           = 0;
    uint32_t wake_word_size            = 0;
    uint32_t pre_wake_word_start_index = 0;

    if (!buffer || !buffer_len || !wake_word_started)
    {
        return 1;
    }

    /* calculate length of pre-utterance + wake word */
    wake_word_size  = wake_word_started * 160 * PCM_SAMPLE_SIZE_BYTES;
    ais_buffer_size = PRE_UTTERANCE_SIZE + wake_word_size;

    if (ais_buffer_size > RING_BUFFER_SIZE)
        return 1;

    if (ais_buffer_size < ring_buffer_index)
    {
        pre_wake_word_start_index = ring_buffer_index - ais_buffer_size;
    }
    else
    {
        pre_wake_word_start_index = RING_BUFFER_SIZE - (ais_buffer_size - ring_buffer_index);
    }

    /* need to move array segments until the below values are in ascending order:
     * pre_wake_word_start_index and ring_buffer_index
     */

    uint32_t to_shift_size = 0;

    if (pre_wake_word_start_index < ring_buffer_index)
    {
        /*
         *  |------------|------------------------------|-----------------|
         *  0      pre_wake_word_start_index      ring_buffer_index     2000ms
         */

        *buffer = ring_buffer + pre_wake_word_start_index;
    }
    else
    {
        /*
         *  |------------|------------------------------|-----------------|
         *  0      ring_buffer_index        wake_word_start_index       2000ms
         */
        if (ring_buffer_index > (RING_BUFFER_SIZE - pre_wake_word_start_index))
        {
            to_shift_size = RING_BUFFER_SIZE - pre_wake_word_start_index;
            *buffer       = ring_buffer;
            /*
             * the final ring_buffer layout of shifting to right
             * |--------------------------------------|-----------------------|
             * 0                         ring_buffer_index+to_shift_size    2000ms
             * pre_wake_word_start_index
             */
            buffer_shift_right(to_shift_size);
        }
        else
        {
            to_shift_size = ring_buffer_index;
            *buffer       = ring_buffer + (pre_wake_word_start_index - ring_buffer_index);
            /*
             * the final ringbuffer layout of shifting to left
             * |----------------------|----------------------------------------|
             * 0        pre_wake_word_start_index-to_shift_size              2000ms
             *                                                        ring_buffer_index
             */
            buffer_shift_left(to_shift_size);
        }
    }

    *buffer_len = ais_buffer_size;

    return 0;
}

static void buffer_shift_left(uint32_t shift_size)
{
    while (shift_size)
    {
        uint32_t chunk_size = shift_size > aux_backup_size ? aux_backup_size : shift_size;

        /* backup */
        memcpy(aux_backup_buffer, ring_buffer, chunk_size);

        /* shift left using memcpy - should be ok, although overlapping
         * buffers (destination lower than source) */
        memcpy(ring_buffer, ring_buffer + chunk_size, RING_BUFFER_SIZE - chunk_size);

        /* restore */
        memcpy(ring_buffer + RING_BUFFER_SIZE - chunk_size, aux_backup_buffer, chunk_size);

        shift_size -= chunk_size;
    }
}

static void buffer_shift_right(uint32_t shift_size)
{
    while (shift_size)
    {
        uint32_t chunk_size = shift_size > aux_backup_size ? aux_backup_size : shift_size;

        /* backup */
        memcpy(aux_backup_buffer, ring_buffer + RING_BUFFER_SIZE - chunk_size, chunk_size);

        /* shift right using memmove -  overlapping
         * buffers (destination greater than source) */
        memmove(ring_buffer + chunk_size, ring_buffer, RING_BUFFER_SIZE - chunk_size);

        /* restore */
        memcpy(ring_buffer, aux_backup_buffer, chunk_size);

        shift_size -= chunk_size;
    }
}

uint8_t *get_ring_buffer(void)
{
    return ring_buffer;
}

uint32_t get_ring_buffer_index(void)
{
    return ring_buffer_index;
}

void reset_ring_buffer(void)
{
    ring_buffer_index = 0;
    memset(ring_buffer, 0, RING_BUFFER_SIZE);
}

void set_aux_backup_buffer(uint8_t *buffer, uint32_t buffer_size)
{
    aux_backup_buffer = buffer;
    aux_backup_size   = buffer_size;
}
