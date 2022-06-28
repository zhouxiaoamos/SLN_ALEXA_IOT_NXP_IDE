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
#include "assert.h"
#include "string.h"
#include "stdio.h"
#include "afw_cli_utils.h"
#include "stdlib.h"

typedef enum {
    BEGIN,
    HEX_OR_BIN,
    HEX_BIN_OR_STRING,
    CHARACTER,
    END_CHAR,
    HEX,
    DEC,
    BIN,
    STRING,
    END
} parse_state_t;

int32_t conv_num(const char * input, int64_t *result)
{
    parse_state_t state = BEGIN;
    int32_t sign = 1;
    const char * p = input;
    uint64_t tmp = 0;

    while (*p) {
        switch (state) {
        case BEGIN:
            if (*p == '0') {
                state = HEX_OR_BIN;
                p++;
            } else if (*p == '\'') {
                state = CHARACTER;
                p++;
            } else if (*p >= '1' && *p <= '9') {
                state = DEC;
            } else if (*p == '-') {
                sign = -1;
                p++;
            } else {
                return -1;
            }
            break;
        case HEX_OR_BIN:
            if (*p == 'x') {
                state = HEX;
                p++;
            } else if (*p == 'b') {
                state = BIN;
                p++;
            } else {
                return -1;
            }
            break;
        case CHARACTER:
            tmp = *p;
            state = END_CHAR;
            p++;
            break;
        case END_CHAR:
            if (*p != '\'') {
                return -1;
            }
            p++;
            break;
        case HEX:
            if (*p >= '0' && *p <= '9') {
                tmp = (tmp * 16) + (*p - '0');
                p++;
            } else if (*p >= 'a' && *p <= 'f') {
                tmp = (tmp * 16) + (*p - 'a' + 10);
                p++;
            } else if (*p >= 'A' && *p <= 'F') {
                tmp = (tmp * 16) + (*p - 'A' + 10);
                p++;
            } else {
                return -1;
            }
            break;
        case BIN:
            if (*p >= '0' && *p <= '1') {
                tmp = (tmp * 2) + (*p - '0');
                p++;
            } else {
                return -1;
            }
            break;
        case DEC:
            if (*p >= '0' && *p <= '9') {
                tmp = (tmp * 10) + (*p - '0');
                p++;
            } else {
                return -1;
            }
            break;
        default:
            return -1;
        }
        if (tmp > INT64_MAX / 32) { // Avoid overflow
            return -1;
        }
    }

    *result = tmp * sign;
    return 0;
}

#define UINT8_MIN 0
#define UINT16_MIN 0
#define UINT32_MIN 0


int32_t conv_uint8_t(const char * input, uint8_t * result)
{
    int64_t temp = 0;
    int32_t retval = 0;
    if (0 != (retval = conv_num(input, &temp))) {
        return retval;
    }
    if (temp < UINT8_MIN || temp > UINT8_MAX) {
        return -1;
    }
    *result = temp;
    return 0;
}

int32_t conv_uint16_t(const char * input, uint16_t * result)
{
    int64_t temp = 0;
    int32_t retval = 0;
    if (0 != (retval = conv_num(input, &temp))) {
        return retval;
    }
    if (temp < UINT16_MIN || temp > UINT16_MAX) {
        return -1;
    }
    *result = temp;
    return 0;
}

int32_t conv_uint32_t(const char * input, uint32_t * result)
{
    int64_t temp = 0;
    int32_t retval = 0;
    if (0 != (retval = conv_num(input, &temp))) {
        return retval;
    }
    if (temp < UINT32_MIN || temp > UINT32_MAX) {
        return -1;
    }
    *result = temp;
    return 0;
}

int32_t conv_int8_t(const char * input, int8_t * result)
{
    int64_t temp = 0;
    int32_t retval = 0;
    if (0 != (retval = conv_num(input, &temp))) {
        return retval;
    }
    if (temp < INT8_MIN || temp > INT8_MAX) {
        return -1;
    }
    *result = temp;
    return 0;
}

int32_t conv_int16_t(const char * input, int16_t * result)
{
    int64_t temp = 0;
    int32_t retval = 0;
    if (0 != (retval = conv_num(input, &temp))) {
        return retval;
    }
    if (temp < INT16_MIN || temp > INT16_MAX) {
        return -1;
    }
    *result = temp;
    return 0;
}

int32_t conv_int32_t(const char * input, int32_t * result)
{
    int64_t temp = 0;
    int32_t retval = 0;
    if (0 != (retval = conv_num(input, &temp))) {
        return retval;
    }
    if (temp < INT32_MIN || temp > INT32_MAX) {
        return -1;
    }
    *result = temp;
    return 0;
}

int32_t conv_bool(const char * input, bool * result)
{
    if (0 == strcmp(input, "true")) {
        *result = true;
        return 0;
    } else if (0 == strcmp(input, "false")) {
        *result = false;
        return 0;
    }
    return -1;
}

int32_t conv_buffer_uint8_t(const char * input, uint8_t ** data, int32_t *data_len)
{
    int32_t max_data_len = strlen(input) + 2;
    *data = (uint8_t *)calloc(max_data_len, sizeof(uint8_t));
    parse_state_t state = BEGIN;
    const char * p = input;
    uint8_t tmp = 0;
    int32_t char_count = 0;
    int32_t output_byte_count = 0;
    uint8_t *data_p = *data;

    while (*p) {
        switch (state) {
        case BEGIN:
            if (*p == '0') {
                state = HEX_BIN_OR_STRING;
                p++;
            } else if (*p == '"') {
                state = STRING;
                p++;
            } else {
                state = STRING;
            }
            break;
        case HEX_BIN_OR_STRING:
            if (*p == 'x') {
                state = HEX;
                p++;
            } else if (*p == 'b') {
                state = BIN;
                p++;
            } else {
                state = STRING;
                *data_p = '0';
                data_p++;
                *data_p = *p;
                data_p++;
                p++;
                output_byte_count += 2;
            }
            break;
        case HEX:
            if (*p >= '0' && *p <= '9') {
                tmp = (tmp * 16) + (*p - '0');
                p++;
                char_count++;
            } else if (*p >= 'a' && *p <= 'f') {
                tmp = (tmp * 16) + (*p - 'a' + 10);
                p++;
                char_count++;
            } else if (*p >= 'A' && *p <= 'F') {
                tmp = (tmp * 16) + (*p - 'A' + 10);
                p++;
                char_count++;
            } else if (*p == ' ' && char_count == 0) {
                state = BEGIN;
                p++;
            } else {
                free(*data);
                *data = NULL;
                return -1;
            }
            if (char_count == 2) {
                *data_p = tmp;
                data_p++;
                tmp = 0;
                char_count = 0;
                output_byte_count++;
                if (output_byte_count >= max_data_len) {
                    free(*data);
                    *data = NULL;
                    printf("output_byte_count (%ld) >= max_data_len (%ld)\n", output_byte_count, max_data_len);
                    return -1;
                }
            }
            break;
        case STRING:
            if (*p == '"') {
                state = END;
                p++;
            } else {
                *data_p = *p;
                data_p++;
                p++;
                output_byte_count++;
                if (output_byte_count >= max_data_len) {
                    free(*data);
                    *data = NULL;
                    printf("output_byte_count (%ld) >= max_data_len (%ld)\n", output_byte_count, max_data_len);
                    return -1;
                }
            }
            break;
        case END:
            free(*data);
            *data = NULL;
            return -1;
        default:
            free(*data);
            *data = NULL;
            return -1;
        }
    }

    if (char_count != 0) {
        free(*data);
        *data = NULL;
        printf("char count not zero!\n");
        return -1;
    }
    *data_len = output_byte_count;
    return 0;
}

int32_t bool_to_str(bool data, char * data_str, uint32_t data_str_len)
{
    if (data_str_len < 5)
        return -1;

    if (data) {
        strcpy(data_str, "true");
        return 4;
    } else if (data_str_len > 5) {
        strcpy(data_str, "false");
        return 5;
    }
    return -1;
}

int32_t uint8_to_str(uint8_t data, char * data_str, uint32_t data_str_len)
{
    int32_t i = 0;
    do {
        data_str[i++] = data % 10 + '0';
        if ((uint32_t)i == data_str_len)
            return -1;
    } while ((data /= 10) > 0);
    data_str[i] = '\0';

    int j, k;
    char tmp;
    for (j = 0, k = i - 1; j < k; j++, k--) {
        tmp = data_str[j];
        data_str[j] = data_str[k];
        data_str[k] = tmp;
    }
    return i;
}

int32_t serialize_buffer_uint8_t(const uint8_t * buf, int32_t len, char ** result)
{
    *result = (char *)calloc(len * 3 + 5, sizeof(char));
    char *p = *result;

    *p = '0';
    p++;
    *p = 'x';
    p++;

    for (int i = 0; i < len; i++) {
        uint8_t tmp = 0;
        tmp = buf[i] >> 4; // High Nibble
        if (tmp <= 9) {
            *p = '0' + tmp;
        } else if (tmp >= 10 && tmp <= 15) {
            *p = 'A' + tmp - 10;
        } else {
            return -1;
        }
        p++;
        tmp = buf[i] & 0xF; // Bottom Nibble
        if (tmp <= 9) {
            *p = '0' + tmp;
        } else if (tmp >= 10 && tmp <= 15) {
            *p = 'A' + tmp - 10;
        } else {
            return -1;
        }
        p++;
    }
    return 0;
}

int32_t serialize_buffer_uint32_t(const uint32_t * buf, int32_t len, char ** result)
{
    return serialize_buffer_uint8_t((const uint8_t *) buf, (len * 4), result);
}


int32_t prepare_buffer_uint8_t(int32_t len, uint8_t **buf)
{
    *buf = (uint8_t *)calloc(len, sizeof(uint8_t));
    if (*buf == NULL) {
        return -1;
    }
    return 0;
}

int32_t prepare_buffer_uint16_t(int32_t len, uint16_t **buf)
{
    *buf = (uint16_t *)calloc(len, sizeof(uint16_t));
    if (*buf == NULL) {
        return -1;
    }
    return 0;
}

int32_t prepare_buffer_uint32_t(int32_t len, uint32_t **buf)
{
    *buf = (uint32_t *)calloc(len, sizeof(uint32_t));
    if (*buf == NULL) {
        return -1;
    }
    return 0;
}

int32_t prepare_buffer_int8_t(int32_t len, int8_t **buf)
{
    *buf = (int8_t *)calloc(len, sizeof(int8_t));
    if (*buf == NULL) {
        return -1;
    }
    return 0;
}

int32_t prepare_buffer_int16_t(int32_t len, int16_t **buf)
{
    *buf = (int16_t *)calloc(len, sizeof(int16_t));
    if (*buf == NULL) {
        return -1;
    }
    return 0;
}

int32_t prepare_buffer_int32_t(int32_t len, int32_t **buf)
{
    *buf = (int32_t *)calloc(len, sizeof(int32_t));
    if (*buf == NULL) {
        return -1;
    }
    return 0;
}

static int32_t read_helper(char ** buf, int32_t len)
{
    char * tmp =  *buf;
    int32_t i;
    for (i = 0; i < len; i++) {
        tmp[i] = getchar();
    }

    if (i != len) {
        return -1;
    }
    return 0;
}

int32_t read_stdin(char ** buf, int32_t len)
{
    // Ownership of this malloc gets transfered back to the user.
    // They are in charge of freeing the memory.
    *buf = (char *)malloc(len + 1);
    if (*buf == NULL) {
        return -1;
    }

    int32_t ret = read_helper(buf, len);
    (*buf)[len] = '\0';
    printf("\n");

    return ret;
}

int32_t read_stdin_no_padding(char ** buf, int32_t len)
{
    // Ownership of this malloc gets transfered back to the user.
    // They are in charge of freeing the memory.
    *buf = malloc(len);
    if (*buf == NULL) {
        return -1;
    }

    int32_t ret = read_helper(buf, len);

    printf("\n");

    return ret;
}

