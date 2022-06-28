/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */
/**
 * @file osal_atomic.h
 * @addtogroup ACE_OSAL_ATOMIC
 * @{
 */
#ifndef OSAL_ATOMIC_H
#define OSAL_ATOMIC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Defines the atomic type
 */
typedef uint32_t aceAtomic_t;

/**
 * @brief   Defines the atomic pointer type
 */
typedef void* aceAtomic_ptr_t;

/**
 * @brief   Atomic compare and set operation.
 * @details Compares the contents of *dest with the contents of expected.
 *          If equal, writes new_value into *dest.
 *
 * @param[out] dest       Pointer to the atomic value.
 * @param[in]  new_value  The new value of *dest.
 * @param[in]  expected   The original value of *dest.
 *
 * @return  True if set successfully, False if set failed.
 */
bool aceAtomic_compareAndSet(aceAtomic_t* dest, uint32_t new_value,
                             uint32_t expected);

/**
 * @brief   Atomic compare and set operation for pointer.
 * @details Compares the contents of *dest with the contents of expected.
 *          If equal, writes new_value into *dest.
 *
 * @param[out] dest       Pointer to the atomic pointer value.
 * @param[in]  new_value  The new value of *dest.
 * @param[in]  expected   The original value of *dest.
 *
 * @return  True if set successfully, False if set failed.
 */
bool aceAtomic_compareAndSetWithPointer(aceAtomic_ptr_t* dest, void* new_value,
                                        void* expected);

/**
 * @brief   Atomic addition.
 * @details Perform add operation with *dest and addend.
 *
 * @param[out] dest    Pointer to the atomic value.
 * @param[in]  addend  The value added to *dest.
 *
 * @return  Return the value that had previously been in *dest.
 */
uint32_t aceAtomic_add(aceAtomic_t* dest, uint32_t addend);

/**
 * @brief   Atomic subtraction.
 * @details Perform subtract operation with *dest and subtrahend.
 *
 * @param[out] dest        Pointer to the atomic value.
 * @param[in]  subtrahend  The value subtracted from the minuend.
 *
 * @return  Return the value that had previously been in *dest.
 */
uint32_t aceAtomic_subtract(aceAtomic_t* dest, uint32_t subtrahend);

/**
 * @brief   Atomic increment.
 * @details Perform increment operation for *dest.
 *
 * @param[out] dest  Pointer to the atomic value.
 *
 * @return  Return the value that had previously been in *dest.
 */
uint32_t aceAtomic_increment(aceAtomic_t* dest);

/**
 * @brief   Atomic decrement.
 * @details Perform decrement operation for *dest.
 *
 * @param[out] dest  Pointer to the atomic value.
 *
 * @return  Return the value that had previously been in *dest.
 */
uint32_t aceAtomic_decrement(aceAtomic_t* dest);

/**
 * @brief   Atomic get.
 * @details Perform read operation on dest.
 *
 * @param[in] dest  Pointer to the atomic value.
 *
 * @return  Return the value of *dest.
 */
uint32_t aceAtomic_get(const aceAtomic_t* dest);

/**
 * @brief   Atomic get with pointer value.
 * @details Perform read operation on dest.
 *
 * @param[in] dest  Pointer to the atomic pointer value.
 *
 * @return  Return the value of *dest.
 */
void* aceAtomic_getWithPointer(const aceAtomic_ptr_t* dest);

/**
 * @brief   Atomic set.
 * @details Perform get_and_set operation on dest.
 *
 * @param[out] dest      Pointer to the atomic value.
 * @param[in]  new_value The new value of *dest.
 *
 * @return  Return the previous value of *dest.
 */
uint32_t aceAtomic_set(aceAtomic_t* dest, uint32_t new_value);

/**
 * @brief   Atomic set with pointer value.
 * @details Perform get_and_set operation on dest.
 *
 * @param[out] dest      Pointer to the atomic pointer value.
 * @param[in]  new_value The new value of *dest.
 *
 * @return  Return the previous value of *dest.
 */
void* aceAtomic_setWithPointer(aceAtomic_ptr_t* dest, void* new_value);

/**
 * @brief   Atomic test a bit.
 * @details Test whether the specified bit of *dest is set or not.
 *
 * @param[out] dest     Pointer to the atomic value.
 * @param[in]  bit_num  Bit number (starting from 0).
 *
 * @return  True if the bit was set, False if it wasn't.
 */
bool aceAtomic_testBit(aceAtomic_t* dest, uint32_t bit_num);

/**
 * @brief   Atomic set a bit.
 * @details Set the specified bit of *dest.
 *
 * @param[out] dest     Pointer to the atomic value.
 * @param[in]  bit_num  Bit number (starting from 0).
 *
 * @return  True if the bit was set, False if it wasn't.
 */
bool aceAtomic_setBit(aceAtomic_t* dest, uint32_t bit_num);

/**
 * @brief   Atomic clear a bit.
 * @details Clear the specified bit of *dest.
 *
 * @param[out] dest     Pointer to the atomic value.
 * @param[in]  bit_num  Bit number (starting from 0).
 *
 * @return  True if the bit was set, False if it wasn't.
 */
bool aceAtomic_clearBit(aceAtomic_t* dest, uint32_t bit_num);

#ifdef __cplusplus
}
#endif

/** @} */
#endif /* OSAL_ATOMIC_H */
