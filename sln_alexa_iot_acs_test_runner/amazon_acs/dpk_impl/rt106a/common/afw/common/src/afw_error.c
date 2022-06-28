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
 * AFW Error
 *******************************************************************************
 */

#include <stdlib.h>
#include "afw_error.h"

static const char *afw_error_str[] = {
    "Success",                   /*  0 */
    "File or device is busy",    /*  1 */
    "Too many open files",       /*  2 */
    "Read only file system",     /*  3 */
    "No space left on device",   /*  4 */
    "No such file or directory", /*  5 */
    "Bad file descriptor",       /*  6 */
    "File name too long",        /*  7 */
    "Invalid argument",          /*  8 */
    "I/O Error",                 /*  9 */
    "Bad address",               /* 10 */
    "Not Implemented",           /* 11 */
    "No Memory",                 /* 12 */
    "Permission denied",         /* 13 */
    "No such device",            /* 14 */
    "Time out",                  /* 15 */
    "No ISR Support",            /* 16 */
    "Internal Error",            /* 17 */
    "Disabled",                  /* 18 */
    "Init not done",             /* 19 */
    "Unsupported operation",     /* 20 */
};

const char *afw_strerror(int err)
{
    err = -err;
    if (err < 0 || (size_t)err >= ARRAY_SIZE(afw_error_str))
        return "Unknown error";

    return afw_error_str[err];
}
