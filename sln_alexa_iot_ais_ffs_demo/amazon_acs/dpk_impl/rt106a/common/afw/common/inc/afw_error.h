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
 * AFW Error codes
 *******************************************************************************
 */

/**
 * @file
 *
 * @brief This file contains the error codes used by the Amazon Firmware Platform modules.
 */

#ifndef AFW_ERROR_H
#define AFW_ERROR_H

/**
 * @name AFW Error codes
 * @anchor afw_error_codes
 */
/**@{*/

#define AFW_ERR -1 // Kept for historical reasons, prefer using -afw_error_t below

// these are positive for historical reasons, user is expected to use negation.
enum afw_error_t {
    AFW_OK           = 0,
    AFW_EBUSY        = 1, /** @brief File is already open */
    AFW_EMFILE       = 2, /** @brief Too many open files */
    AFW_EROFS        = 3, /** @brief Read only file system */
    AFW_ENOSPC       = 4, /** @brief No space left on device */
    AFW_ENOENT       = 5, /** @brief No such file or directory */
    AFW_EBADF        = 6, /** @brief Bad file descriptor */
    AFW_ENAMETOOLONG = 7, /** @brief File name too long */
    AFW_EINVAL       = 8, /** @brief Invalid argument */
    AFW_EIO          = 9, /** @brief I/O Error */
    AFW_EFAULT       = 10, /** @brief Bad address */
    AFW_ENOTIMPL     = 11, /** @brief Not Implemented */
    AFW_ENOMEM       = 12, /** @brief Out of memory */
    AFW_EPERM        = 13, /** @brief Permission denied */
    AFW_ENODEV       = 14, /** @brief No such device */
    AFW_ETIMEOUT     = 15, /** @brief Timed Out */
    AFW_ENOISR       = 16, /** @brief no ISR support */
    AFW_EINTRL       = 17, /** @brief Internal Error */
    AFW_EDISBL       = 18, /** @brief Disabled */
    AFW_ENOINIT      = 19, /** @brief Init not done Error */
    AFW_EUNSUP       = 20, /** @brief Unsupported operation Error */

    // last one
    AFW_EUFAIL       = 255 /** @brief Unknown Failure */
};
/**@}*/



#undef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0])			      \
	+ sizeof(typeof(int[1 - 2*!!__builtin_types_compatible_p(typeof(arr), \
		 typeof(&arr[0]))]))*0)
/**
 * @brief Return error string given negative error code.
 */
const char *afw_strerror(int err);

#endif /* AFW_ERROR_H */
