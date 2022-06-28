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

#ifndef AFW_UTILS_H_
#define AFW_UTILS_H_

#include <FreeRTOS.h>
#include <portmacro.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ace/aceCli.h>
#include <ace/osal_common.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)               (sizeof(a) / sizeof(a[0]))
#endif

#ifndef ARRAY_INDEX
// base and pointer must have the same type.
#define ARRAY_INDEX(base, pointer) ((pointer) - (base))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif


#ifndef false
#define false                       (uint32_t)(0)  // making this 32-bit wide
#define true                        (!false)
#endif

#define MSBYTE(val)                 ((uint8_t)(((val) >> ((sizeof(val) - 1) * 8)) & 0xFF))
#define LSBYTE(val)                 ((uint8_t)((val) & 0xFF))

#define ROUND_UP_4(x)               ((x + 0x00000003) & (~0x00000003))

#define MS_TO_SYS_TICKS(ms)         (((ms) / portTICK_PERIOD_MS) ? ((ms) / portTICK_PERIOD_MS) : 1)

#define FLASH_CACHEABLE_ADDR(x)     (((uint32_t)x >= FLASH_BASE) && \
                                    ((uint32_t)x < (FLASH_BASE + FLASH_TOTAL_LENGTH)))

#define MAC_RAW_TO_STR(mac, buf)    {                                               \
    do {                                                                            \
        sprintf(buf,"%02X:%02X:%02X:%02X:%02X:%02X",                                \
                (mac)[0], (mac)[1], (mac)[2], (mac)[3], (mac)[4], (mac)[5]);        \
    } while(0);                                                                     \
}

#define OBFUSCATED_STRING       "XXXX"
#define OBFUSCATED_INT            (-1)
#define OBFUSCATE_SECURED_STR(STR) afw_defm_secure_status(0)?OBFUSCATED_STRING:STR
#define OBFUSCATE_SECURED_INT(INT) afw_defm_secure_status(0)?OBFUSCATED_INT:INT

#define TLV_IE_LEN_1BYTE        1
#define TLV_IE_LEN_2BYTE        2

//Semaphore macro support both ISR and non-ISR context.
#define SEMA_TAKE(sem, timeout, retv)     \
    if (xPortIsInsideInterrupt()) {         \
        BaseType_t _pxHigherPriorityTaskWoken = 0; \
        (retv) = xSemaphoreTakeFromISR(sem, &_pxHigherPriorityTaskWoken); \
        portYIELD_FROM_ISR_WRAP(_pxHigherPriorityTaskWoken); \
    } else {  \
        (retv) = xSemaphoreTake(sem, timeout); \
    }

#define SEMA_GIVE(sem)     \
    if (xPortIsInsideInterrupt()) { \
        BaseType_t _pxHigherPriorityTaskWoken = 0; \
        xSemaphoreGiveFromISR(sem, &_pxHigherPriorityTaskWoken); \
        portYIELD_FROM_ISR_WRAP(_pxHigherPriorityTaskWoken); \
    } else {  \
        xSemaphoreGive(sem); \
    }

#define BASE64_ENC_LEN(x)    ((4 * (((x) + 2) / 3)) + 1)

typedef struct
{
    uint16_t type;
    uint16_t len;
}afw_tlv_t;


#ifdef __cplusplus
extern "C" {
#endif
    /**
    * @brief Safe string length function. This is used when standard function is not available from
    * the library
    *
    * @param [in]           const char* input character string
    * @param [in]           uint32_t maximum length of the string
    *
    * @return [uint32_t]    Number of bytes occupied by the string (not including the NULL) or
    * maximum length parameter
    *
    * Example Usage:
    * @code
    *    afw_strnlen(input_string, (sizeof(buff) - 1));
    * @endcode
    */
    uint32_t    afw_strnlen(const char *s, uint32_t maxlen);

    /**
    * @brief Helper function to check if input string is HEX encoded, i.e. all characters in input
    * are 0-9/A-F/a-f
    *
    * @param [in]       char* input character string
    *
    * @return [bool]    true if all in HEX chars else false
    *
    * Example Usage:
    * @code
    *    afw_util_is_hex_str("ABCD123"); // true
    *    afw_util_is_hex_str("Amazon"); // false
    * @endcode
    */
    bool        afw_util_is_hex_str(const char *inputStr);

    /**
    * @brief Helper function to check if input string is HEX encoded, i.e. all characters in input
    * are 0-9/A-F/a-f
    *
    * @param [in]       char* input character string
    *
    * @return [bool]    true if all in HEX chars else false
    *
    * Example Usage:
    * @code
    *    afw_util_is_hex_str("ABCD123"); // true
    *    afw_util_is_hex_str("Amazon"); // false
    * @endcode
    */
    uint8_t     afw_util_is_alphaNumeric(const char *inputStr);

    /**
    * @brief Helper function to convert an input to a HEX encoded string. It converts each
    * byte to its corresponding 2 digit HEX equivalent in ASCII. The function adds the NULL
    * character.
    *
    * @param [in]           const char* input byte sequence (does not need to be null terminated)
    * @param [in]           input buffer size
    * @param [out]          output buffer to store the converted string
    * @param [in]           output buffer size
    * @param [in]           useLowerCase if true prints hex using lower case ascii
    * @return [uint32_t]    number of bytes in converted string, usually 2 times the input
    *
    */
    uint32_t    afw_util_input_to_hex_str(const char *input, uint32_t input_sz, char *out_buff, uint32_t buff_sz, bool useLowerCase);

    /**
    * @brief Helper function to convert HEX encoded string to its ASCII representation
    *
    * @param [in]           const char* HEX encoded input character string
    * @param [out]          output buffer to store the converted string
    * @param [in]           output buffer size
    * @return [uint32_t]    number of bytes in converted string, usually 1/2 the input length
    *
    * Example Usage:
    * @code
    *    afw_util_hex_str_to_val("416D617A6F6E", buff, sizeof(buff)); // "Amazon"
    * @endcode
    */
    uint32_t    afw_util_hex_str_to_val(const char *input, uint8_t *out_buff, uint32_t buff_sz);

    /**
     * @brief Helper function to output data to stdout in hexdump utility format
     *
     * @param [in]  input data buffer
     * @param [in]  starting address of the data(first column in the output)
     * @param [in]  Number of bytes to dump
     */
    void        afw_hexdump(char *buf, int addr, int len);

    ace_status_t     afw_util_cli_rand(int32_t len, const char *param[]);

    float       median_float(float arr[], int len);
    void        sort_float(float arr[], int len);
    void        print_success(void);
    void        print_fail(void);
    ace_status_t     afw_util_memory_cli(int32_t len, const char *param[]);

    /**
    * @brief Convert binary data to a hex string
    *
    * Convert each byte in the input binary buffer to the corresponding 2-digit
    * hex string representation. Terminates the output string with a null byte.
    * Similar to the Python binascii.hexlify function.
    *
    * @param bin_in   input buffer containing binary data
    * @param n        number of bytes to convert
    * @param hex_out  output buffer; must be at least (2 * n + 1) bytes long
    */
    void        afw_hexlify(uint8_t *bin_in, size_t n, char *hex_out);

    /**
    * @brief Convert hex string representation into corresponding binary data
    *
    * Convert every two hex digits in the input string to the corresponding
    * byte. Similar to the Python binascii.unhexlify function.
    *
    * @param hex_in   input string containing an even number of hex digits; must be null-terminated
    * @param bin_out  output buffer; must be at least (strlen(hex_in) / 2) bytes (will work in place if == hex_in)
    *
    * @return Number of bytes written to the output buffer
    */
    int         afw_unhexlify(const char *hex_in, uint8_t *bin_out);

    /**
    * @brief api to detect if the execution is in thread or interrupt mode
    *
    * checks if the VECTACTIVE bits of the Interrupt control register has a non-zero value
    *
    * @return 1-interrupt mode, 0-thread mode
    */
    uint8_t afw_is_interrupt(void);

    /**
    * @brief api to check if str is printable
    *
    * Checks all bytes upto strlen of the input are printable characters
    *
    * @return AFW_TRUE if all bytes are printable or AFW_FALSE
    */
    int afw_is_printable(const char* data);

    /**
    * @brief api to get current system time in milliseconds
    *
    * System GPT timer starts from 0 at boot-up
    *
    * @return [uint32_t]    current time in milliseconds
    */
    unsigned int afw_get_current_time_in_ms(void);

    /**
    * @brief add tag tlv for buffer to be encrypted
    *
    * @param buf input buffer need to be encrypted
    *
    * @param buf_len total input buffer length
    *
    * @param type tag tlv type ie
    *
    * @param type_ie_len tag tlv type ie length
    *
    * @param length_ie_len tag tlv length ie
    *
    * @param len tag tlv length ie's length
    *
    * @param pay_load the payload need to be encrypted (except the tag tlv)
    *
    * @return total tag tlv length
    */
    int16_t afw_encode_tlv(uint8_t *buf, uint16_t buf_len, uint8_t type,
            uint8_t type_ie_len, uint8_t length_ie_len, uint16_t len,  uint8_t *pay_load);

    /**
    * @brief add tag tlv for buffer to be decrypted
    *
    * @param buf input buffer need to be decrypted
    *
    * @param buf_len total input buffer length
    *
    * @param type tag tlv type ie
    *
    * @param type_ie_len tag tlv type ie length
    *
    * @param length_ie_len tag tlv length ie
    *
    * @param len tag tlv length ie's length
    *
    * @return total tag tlv length, return -1 if decode fail
    */
    int16_t afw_decode_tlv(uint8_t *buf, uint16_t buf_len, afw_tlv_t *tlv,
            uint8_t type_ie_len, uint16_t length_ie_len);

    /**
    * @brief do base 64bits decode
    *
    * @param inp input buffer need to be decoded
    *
    * @param inlen input buffer length
    *
    * @param outp output buffer after decoded
    *
    * @return output buffer length
    */
    int afw_b64dec(char *inp, int inlen, char *outp);
#ifdef __cplusplus
}
#endif

#endif /* AFW_UTILS_H_ */
