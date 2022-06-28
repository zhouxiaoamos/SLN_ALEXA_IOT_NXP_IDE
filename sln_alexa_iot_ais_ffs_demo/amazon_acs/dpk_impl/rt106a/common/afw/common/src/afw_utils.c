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
 * AFW Utils
 *******************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include "ctype.h"

#include "afw_error.h"
#include "afw_def.h"
#include "afw_utils.h"
#include "asd_log_platform_api.h"

#define IsNum(c) ((c >= '0') && (c <= '9'))
#define IsLowerHex(c) ((c >= 'a') && (c <= 'f'))
#define IsUpperHex(c) ((c >= 'A') && (c<='F'))
#define IsLowerAlpha(c) ((c >= 'a') && (c <= 'z'))
#define IsUpperAlpha(c) ((c >= 'A') && (c<='Z'))

static char base64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/*******************************************************************************
 * compare_float
 *******************************************************************************
 */
static int compare_float(const void *p, const void *q)
{
    float a = *(const float *)p;
    float b = *(const float *)q;

    if (a < b)
        return -1;
    else if (a > b)
        return 1;

    return 0;
}

/*******************************************************************************
 * sort_float
 *******************************************************************************
 */
void sort_float(float arr[], int len)
{
    qsort(arr, len, sizeof(arr[0]), compare_float);
}

/*******************************************************************************
 * median_float
 *
 * TODO: Use a selection algorithm that doesn't require sorting
 *******************************************************************************
 */
float median_float(float arr[], int len)
{
    if (len <= 0)
        return 0.0;

    int m = len / 2;

    sort_float(arr, len);

    if (len % 2)
        return arr[m];

    return (arr[m] + arr[m - 1]) / 2.0f;
}

static char data[80];
#define hex2ch(h) ((h)<10?(h)+'0':(h)-10+'A')
#define ADDR_LEN    8
#define HEX_START   10
#define ASCII_START 60
#define LINE_END    (ASCII_START+16)
/* routine for displaying one line (16 bytes) of data as a hex dump */
static void write_line(char *buf, int addr, int len)
{
    if (len>0) {
        char *ap, *hp;
        unsigned char   ch;
        int    i, x;
        memset(data,' ',LINE_END);
        data[LINE_END] = '\0';
        for (i=7; i>=0; i--) {
            x=addr&0xf;
            data[i]=hex2ch(x);
            addr>>=4;
        }
        hp = &(data[HEX_START]);
        ap = &(data[ASCII_START]);
        for (i=0; i<len; i++) {
            ch = (unsigned char)buf[i];
            x = ch / 16;
            *hp++=hex2ch(x);
            x = ch & 0x000F;
            *hp++=hex2ch(x);
            hp++;    /*Space between hex*/
            *ap++=( (ch > 31) && (ch < 127) )?ch:'.';
        }
        puts(data);
    }
}

void afw_hexdump(char *buf, int addr, int len)
{
    int  i;
    for (i = 0; i < len/16; i++,buf+=16,addr+=16) {
        write_line(buf, addr,16);
    }
    write_line(buf, addr,len%16);
}

uint32_t afw_strnlen(const char *s, uint32_t maxlen)
{
    register const char *e;
    uint32_t n;
    for (e = s, n = 0; *e && n < maxlen; e++, n++)
        ;
    return n;
}


bool afw_util_is_hex_str(const char *inputStr)
{

    if (inputStr == NULL) {
        return AFW_FALSE;
    }
    while (*inputStr) {
        if (IsNum(*inputStr) || IsLowerHex(*inputStr) || IsUpperHex(*inputStr)) {
            inputStr++;
        } else {
            return AFW_FALSE;
        }
    }
    return AFW_TRUE;
}

uint8_t afw_util_is_alphaNumeric(const char *inputStr)
{

    if (inputStr == NULL) {
        return AFW_FALSE;
    }
    while (*inputStr) {
        if (IsNum(*inputStr) || IsLowerAlpha(*inputStr) || IsUpperAlpha(*inputStr)) {
            inputStr++;
        } else {
            return AFW_FALSE;
        }
    }
    return AFW_TRUE;
}

uint32_t afw_util_input_to_hex_str(const char *input, uint32_t input_sz, char *out_buff,
                                   uint32_t buff_sz, bool useLowerCase)
{
    uint32_t bytes_written = 0;
    char *write_ptr = out_buff;
    char *read_ptr = (char *)input;

    if (input == NULL || write_ptr == NULL || buff_sz == 0) {
        return 0;
    }
    if (buff_sz < (2 * input_sz)) {
        return 0;
    }
    for (uint32_t i = 0; i<input_sz; ++i, ++read_ptr) {
        if (useLowerCase) {
            sprintf(write_ptr, "%02x", *read_ptr);
        } else {
            sprintf(write_ptr, "%02X", *read_ptr);
        }
        write_ptr += (2 * sizeof(uint8_t));
    }
    bytes_written = (write_ptr - out_buff);

    return bytes_written;
}

uint8_t nib(char c)
{
    if (IsNum(c))return c - '0';
    if (IsLowerHex(c))return c - 'a' + 10;
    if (IsUpperHex(c))return c - 'A' + 10;
    return 0;
}

uint32_t afw_util_hex_str_to_val(const char *input, uint8_t *out_buff, uint32_t buff_sz)
{
    uint32_t bytes_written = 0;
    uint32_t input_sz;
    uint8_t *write_ptr = out_buff;
    char *read_ptr = (char *)input;

    if (input == NULL || write_ptr == NULL || buff_sz == 0) {
        return 0;
    }
    if (!afw_util_is_hex_str((char*)input)) {
        return 0;
    }
    input_sz = strlen(input);
    if ((input_sz & 0x01) || (buff_sz < (input_sz >> 1))) {
        return 0;
    }

    for (bytes_written = 0; bytes_written < (buff_sz); bytes_written++) {
        if (!IsNum(read_ptr[(bytes_written << 1)]) &&
            !IsLowerHex(read_ptr[(bytes_written << 1)]) &&
            !IsUpperHex(read_ptr[(bytes_written << 1)])) {
            ASD_LOG_W(afw, "Invalid char `%c` (ascii 0x%2x)\r\n",
                   read_ptr[(bytes_written << 1)],
                   read_ptr[(bytes_written << 1)]);
            return bytes_written;
        }

        if (!IsNum(read_ptr[(bytes_written << 1) + 1]) &&
            !IsLowerHex(read_ptr[(bytes_written << 1) + 1]) &&
            !IsUpperHex(read_ptr[(bytes_written << 1) + 1])) {
            ASD_LOG_W(afw, "Invalid char `%c` (ascii 0x%2x)\r\n",
                   read_ptr[(bytes_written << 1) + 1],
                   read_ptr[(bytes_written << 1) + 1]);
            return bytes_written;
        }

        write_ptr[bytes_written] = (nib(read_ptr[(bytes_written << 1)]) << 4) +
                                   nib(read_ptr[(bytes_written << 1) + 1]);
    }

    return bytes_written;
}

static void hex_encode_byte(uint8_t b, char *out)
{
    static const char hex_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    unsigned char idx1 = (b & 0xF0) >> 4;
    unsigned char idx2 = b & 0x0F;
    out[0] = hex_chars[idx1];
    out[1] = hex_chars[idx2];
}

void afw_hexlify(uint8_t *bin_in, size_t n, char *hex_out)
{
    size_t i;

    for (i = 0; i < n; i++) {
        hex_encode_byte(bin_in[i], hex_out);
        hex_out += 2;
    }
    *hex_out = '\0';
}

int afw_unhexlify(const char *hex_in, uint8_t *bin_out)
{
    int byte;
    const char *p;
    uint8_t *q;

    if (strlen(hex_in) % 2 || !afw_util_is_hex_str(hex_in))
        return 0;

    for (p = hex_in, q = bin_out; *p; p += 2, q++) {
        sscanf(p, "%2x", &byte);
        *q = (uint8_t)byte;
    }

    return (int)(q - bin_out);
}

void print_success(void)
{
    char success[] = "Success";
    puts(success);
}

void print_fail(void)
{
    char fail[] = "Fail";
    puts(fail);
}

int afw_is_printable(const char* data)
{
    size_t idx;
    for(idx=0; idx<strlen(data); idx++) {
        if(!isprint((int)(data[idx])))
            return AFW_FALSE;
    }
    return AFW_TRUE;
}

static int tlv_get_ie_val(uint16_t *val, uint8_t *buf, uint8_t ie_len);
static int tlv_set_ie_val(uint16_t val, uint8_t *buf, uint8_t ie_len);

static int tlv_get_ie_val(uint16_t *val, uint8_t *buf, uint8_t ie_len)
{
    if (TLV_IE_LEN_1BYTE == ie_len)
       *val = *buf;
    else if (TLV_IE_LEN_2BYTE == ie_len)
       *val = (*buf  << 8) | *(buf + 1);
    else {
        ASD_LOG_E(afw,"Invalid IE Len(%d)",ie_len);
        return AFW_ERR;
    }
    return AFW_OK;
}

static int tlv_set_ie_val(uint16_t val, uint8_t *buf, uint8_t ie_len)
{
    if (TLV_IE_LEN_1BYTE == ie_len)
       *buf = val;
    else if (TLV_IE_LEN_2BYTE == ie_len) {
        *buf++ = (val >> 8) & 0xFF;
        *buf = val & 0xFF;
    }
    else {
        ASD_LOG_E(afw,"Invalid IE Len(%d)",ie_len);
        return AFW_ERR;
    }
    return AFW_OK;
}

int16_t afw_encode_tlv(uint8_t *buf, uint16_t buf_len, uint8_t type,
        uint8_t type_ie_len, uint8_t length_ie_len, uint16_t len,  uint8_t *payload)
{
    uint8_t *start = buf;

    if (NULL == buf || !buf_len) {
        ASD_LOG_E(afw,"Invalid bufP(%p) len(%d)",buf,buf_len);
        return AFW_ERR;
    }

    if(!type_ie_len || !length_ie_len
        || ((!payload) && ((type_ie_len + length_ie_len) > buf_len))
        || ((payload) && ((type_ie_len + length_ie_len + len) > buf_len))) {
           ASD_LOG_E(afw,"Invalid typeLen:%d lenIELen: %d bufLen:%d payload:%p len:%d",
                         type_ie_len, length_ie_len, buf_len, payload, len);
           return AFW_ERR;
        }

    // Encode Type IE
    if (AFW_ERR == tlv_set_ie_val(type, buf, type_ie_len)) {
        ASD_LOG_E(afw,"Type IE Set Failed");
        return AFW_ERR;
    }
    buf += type_ie_len;
    buf_len -= type_ie_len;

    // Encode Length IE
    if (AFW_ERR == tlv_set_ie_val(len, buf, length_ie_len)) {
        ASD_LOG_E(afw,"Length IE Set Failed");
        return AFW_ERR;
    }
    buf += length_ie_len;
    buf_len -= length_ie_len;

    // Validate Payload Length
    if (payload) {
        memcpy(buf, payload, len);
        buf += len;
    }
    return buf - start;
}

// Do not support Type Only
int16_t afw_decode_tlv(uint8_t *buf, uint16_t buf_len, afw_tlv_t *tlv,
                                         uint8_t type_ie_len, uint16_t length_ie_len)
{
    tlv->len = 0;
    if (NULL == buf || !buf_len || NULL == tlv) {
        ASD_LOG_E(afw,"TLV Invalid Input: bufP(%p) len(%d)",buf,buf_len);
        return AFW_ERR;
    }

    // Get Type IE
    if(!type_ie_len || type_ie_len > buf_len) {
        ASD_LOG_E(afw,"TLV Type IE Invalid Length:%d %d",type_ie_len, buf_len);
        return AFW_ERR;
    }
    if (-1 == tlv_get_ie_val(&(tlv->type), buf, type_ie_len)) {
        ASD_LOG_E(afw,"TLV Type IE Retrieve Failed");
        return AFW_ERR;
    }
    buf += type_ie_len;
    buf_len -= type_ie_len;

    // Get Length IE
    if(!length_ie_len || length_ie_len > buf_len) {
        ASD_LOG_E(afw,"TLV Length IE Invalid Length:%d %d",length_ie_len, buf_len);
        return AFW_ERR;
    }
    if (-1 == tlv_get_ie_val(&(tlv->len), buf, length_ie_len)) {
        ASD_LOG_E(afw,"TLV Length IE Retrieve Failed");
        return AFW_ERR;
    }
    buf += length_ie_len;
    buf_len -= length_ie_len;

    // Validate Payload Length
    if (tlv->len  && tlv->len > buf_len) {
        ASD_LOG_E(afw,"Payload Length Not valid, len:%d bufLen:%d",tlv->len, buf_len);
        return AFW_ERR;
    }
    return (type_ie_len + length_ie_len + tlv->len);
}

int afw_b64dec(char *inp, int inlen, char *outp)
{
    int outlen = 0;
    char *cp;
    uint32_t bs_bits = 0;
    int bs_offs = 0;;

    while (inlen > 0) {
        if ((cp = strchr(base64, *inp++)) == NULL)
            break;
        bs_bits = (bs_bits << 6) | (cp - base64);
        inlen--;
        bs_offs += 6;
        if (bs_offs >= 8) {
            *outp++ = bs_bits >> (bs_offs - 8);
            outlen++;
            bs_offs -= 8;
        }
    }
    return (outlen);
}
