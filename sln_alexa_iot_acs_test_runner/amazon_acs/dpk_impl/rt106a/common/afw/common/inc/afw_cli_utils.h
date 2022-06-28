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
 * Amazon Firmware Platform CLI Utilities
 *******************************************************************************
 */

/**
 * @file
 *
 * This file contains utilities which are useful for writing CLI command handlers.
 */

#ifndef AFW_CLI_UTIL_H_
#define AFW_CLI_UTIL_H_

#include "stdint.h"
#include "stdbool.h"


/**
 * @name String to int conversion methods
 * @anchor string_to_int_conversions
 *
 * These methods convert string representations of integers into integers. They support decimal,
 * hexadecimal, binary and ASCII character representations of integers.
 */
/**@{*/
int32_t conv_uint8_t(const char * input, uint8_t * result);
int32_t conv_uint16_t(const char * input, uint16_t * result);
int32_t conv_uint32_t(const char * input, uint32_t * result);
int32_t conv_int8_t(const char * input, int8_t * result);
int32_t conv_int16_t(const char * input, int16_t * result);
int32_t conv_int32_t(const char * input, int32_t * result);
int32_t conv_bool(const char * input, bool * result);
int32_t conv_buffer_uint8_t(const char * input, uint8_t ** data, int32_t *data_len);
/**@}*/

/**
 * @name _to_str conversion methods
 *
 * @param data_str_len Size of data_str buffer
 * @returns length of string or -1 on failure
 *
 * @remark uint8_to_str is modified version of itoa() in cstdlib.h for uint
 */
int32_t bool_to_str(bool data, char * data_str, uint32_t data_str_len);
int32_t uint8_to_str(uint8_t data, char * data_str, uint32_t data_str_len);

int32_t serialize_buffer_uint8_t(const uint8_t * buf, int32_t len, char ** result);
int32_t serialize_buffer_uint32_t(const uint32_t * buf, int32_t len, char ** result);

int32_t prepare_buffer_uint8_t(int32_t len, uint8_t ** buf);
int32_t prepare_buffer_uint16_t(int32_t len, uint16_t ** buf);
int32_t prepare_buffer_uint32_t(int32_t len, uint32_t ** buf);
int32_t prepare_buffer_int8_t(int32_t len, int8_t ** buf);
int32_t prepare_buffer_int16_t(int32_t len, int16_t ** buf);
int32_t prepare_buffer_int32_t(int32_t len, int32_t ** buf);

int32_t read_stdin(char ** buf, int32_t len);
int32_t read_stdin_no_padding(char ** buf, int32_t len);

/**@}*/

#endif //   AFW_CLI_UTIL_H_
