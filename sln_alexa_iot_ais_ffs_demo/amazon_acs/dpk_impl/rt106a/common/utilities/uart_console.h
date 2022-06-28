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
 * uart_console.h
 *******************************************************************************
 */

#include "iot_uart.h"

/*!
 * @brief Initializes the uart console for debug messages.
 *
 * Call this function to enable debug log messages to be output via uart,
 * After this function has returned, stdout and stdin are connected to the selected uart.
 *
 * @param portNum       Indicates the uart port number used to send debug messages.
 *
 * @return              Indicates whether initialization was successful or not.
 * @retval 0            Execution successfully
 * @retval 1            Execution failure
 */
int UARTConsole_Init(uint32_t portNum);

/*!
 * @brief De-initializes the uart used for debug messages.
 *
 * Call this function to disable debug log messages to be output via the uart port.
 *
 * @return Indicates whether de-initialization was successful (0) or not (1).
 */
int UARTConsole_Deinit(void);

/*!
 * @brief uart log push interface
 *
 * Call this function to print log
 * @param buf, buffer pointer
 * @param size, available size
 * @return indicate the push size
 * @retval-1 indicate buffer is full or transfer fail.
 * @retval size return the push log size.
 */
int UARTConsole_Push(uint8_t *buf, size_t size);

/*!
 * @brief uart log pop function
 *
 * Call this function to pop log from buffer.
 * @param buf buffer address to pop
 * @param size log size to pop
 * @return pop log size.
 */
int UARTConsole_Pop(uint8_t *buf, size_t size);

/*!
 * @brief log read one line function
 *
 * Call this function to print log
 * @param buf, buffer pointer
 * @param size, available size
 * @reutrn the number of the received character
 */
int UARTConsole_ReadLine(uint8_t *buf, size_t size);

/*!
 * @brief log read one character function
 *
 * Call this function to GETCHAR
 * @param ch receive address
 * @reutrn the number of the received character
 */
int UARTConsole_ReadCharacter(uint8_t *ch);

/*!
 * @brief set uart configuration::IotUARTConfig_t
 *
 * Call this function to change uart configuration
 * @param uartConfig uart configuration to be set
 * @reutrn Indicates whether set uart config was successful (0) or not (1).
 */
int UARTConsole_SetUartConfig(IotUARTConfig_t *uartConfig);
