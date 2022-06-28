
#ifndef GENERAL_UTILS_H
#define GENERAL_UTILS_H

/*
 * $copyright$
 *
 * $license$
 *
 */
#include <stdint.h>

/*!
 * @file    general_utils.h
 * @brief   This file provides general helper functions.
 */

/*!
 * @ingroup libcommon
 * @brief   smallest_int
 * @details Function to determine the smalles of two integers.
 * @param a    first integer
 * @param b    second integer
 * @returns value of the smallest integer
 */
int32_t smallest_int(int32_t a, int32_t b);

/*!
* @ingroup libcommon
* @brief Extend 16 bit per sample to 32bit per sample
* @param src [IN] 16bit sample's buffer
* @param src_len [IN] Number of bytes in the input
* @param dst [OUT] Output buffer where the 32-bit data is going to be written
* @param dst_len [IN] Maximum num of bytes that should be written on the output
* @return uint32_t Number of bytes that were written in the destination buffer
* @details This function uses src_len as the number byes and the
* number of bytes in dst_len to determine how many
* bytes will be copied in the output buffer dst. The return value will be the
* number in bytes that was copied to the out buffer
*
*/
uint32_t _transform_16_to_32(void* src, uint32_t src_len, void* dst, uint32_t dst_len);

/*!
* @ingroup libcommon
* @brief Extend 24 bit per sample to 32bit per sample
* @param src [IN] 24 bit sample's buffer
* @param src_len [IN] Number of bytes in the input
* @param dst [OUT] Output buffer where the 32-bit data is going to be written
* @param dst_len [IN] Maximum num of bytes that should be written on the output
* @return uint32_t Number of bytes that were written in the destination buffer
* @details This function uses src_len as the number byes and the
* number of bytes in dst_len to determine how many
* bytes will be copied in the output buffer dst. The return value will be the
* number in bytes that was copied to the out buffer
*/
uint32_t _transform_24_to_32(void* src, uint32_t src_len, void* dst, uint32_t dst_len);

/*!
* @ingroup libcommon
* @brief Compress 32 bit per sample to 24 bit per sample
* @param src [IN] 32 bit sample's buffer
* @param src_len [IN] Number of bytes in the input
* @param dst [OUT] Output buffer where the 24-bit data is going to be written
* @param dst_len [IN] Maximum num of bytes that should be written on the output
* @return uint32_t Number of bytes that were written in the destination buffer
* @details This function uses src_len as the number byes and the
* number of bytes in dst_len to determine how many
* bytes will be copied in the output buffer dst. The return value will be the
* number in bytes that was copied to the out buffer
*/
uint32_t _transform_32_to_24(void* src, uint32_t src_len, void* dst, uint32_t dst_len);

/*!
* @ingroup libcommon
* @brief Compress 32 bit per sample to 16 bit per sample
* @param src [IN] 32 bit sample's buffer
* @param src_len [IN] Number of bytes in the input
* @param dst [OUT] Output buffer where the 16-bit data is going to be written
* @param dst_len [IN] Maximum num of bytes that should be written on the output
* @return uint32_t Number of bytes that were written in the destination buffer
* @details This function uses src_len as the number byes and the
* number of bytes in dst_len to determine how many
* bytes will be copied in the output buffer dst. The return value will be the
* number in bytes that was copied to the out buffer
*/
uint32_t _transform_32_to_16(void* src, uint32_t src_len, void* dst, uint32_t dst_len);

/*!
* @ingroup libcommon
* @brief Equivalent to a memcpy used with src and dest size validation to
* determine the amount of bytes to copy
* @param src [IN] 32 bit sample's buffer
* @param src_len [IN] Number of bytes in the input
* @param dst [OUT] Output buffer where the 32-bit data is going to be written
* @param dst_len [IN] Maximum num of bytes that should be written on the output
* @return uint32_t Number of bytes that were written in the destination buffer
*/
uint32_t data_copy_32bits(void* src, uint32_t src_len, void* dst, uint32_t dst_len);

/*!
* @ingroup libcommon
* @brief Used to interleave samples from two input buffers on an output buffer
* @param src [IN] 16 bit sample's buffer
* @param src_len [IN] Number of bytes in the input.
* @param dst [OUT] Output buffer where the 32-bit data is going to be written
* @param dst_len [IN] Maximum num of bytes that should be written on the output
* @return uint32_t Number of bytes that were written in the destination buffer
*/
uint32_t _interleave_16_to_32(
    void* src, uint32_t src_len, void* dst, uint32_t dst_len);

/*!
* @ingroup libcommon
* @brief Used to interleave samples from two input buffers on an output buffer
* @param src [IN] 24 bit sample's buffer
* @param src_len [IN] Number of bytes in the input.
* @param dst [OUT] Output buffer where the 32-bit data is going to be written
* @param dst_len [IN] Maximum num of bytes that should be written on the output
* @return uint32_t Number of bytes that were written in the destination buffer
*/
uint32_t _interleave_24_to_32(
    void* src, uint32_t src_len, void* dst, uint32_t dst_len);

    /*!
* @ingroup libcommon
* @brief Used to interleave samples from two input buffers on an output buffer
* @param src [IN] 32 bit sample's buffer
* @param src_len [IN] Number of bytes in the input. Applies to both input buffers
* @param dst [OUT] Output buffer where the 32-bit data is going to be written
* @param dst_len [IN] Maximum num of bytes that should be written on the output
* @return uint32_t Number of bytes that were written in the destination buffer
*/
uint32_t _interleave_32bits(
    void* src, uint32_t src_len, void* dst, uint32_t dst_len);

#endif

