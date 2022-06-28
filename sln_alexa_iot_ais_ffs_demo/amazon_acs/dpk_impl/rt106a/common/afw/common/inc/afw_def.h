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
 * AFW common definitions
 *******************************************************************************
 */

#ifndef AFW_DEF_H
#define AFW_DEF_H

#include <afw_error.h>

#define AFW_TRUE    1
#define AFW_FALSE   0

typedef struct {
    double x;
    double y;
    double z;
} afw_xyz_t;

#endif
