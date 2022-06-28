/*
 * Copyright 2018-2020 Amazon.com, Inc. or its affiliates. All rights reserved.
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
 * @File:factory_reset_api.h
 * @brief ACE Factory Reset APIs and structures
 * @addtogroup FactoryReset
 * @{
 */

#ifndef _ACE_FR_H_
#define _ACE_FR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/ace_status.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Factory Reset types
 */
typedef enum {
    ACEFR_SOFT = 0,
    ACEFR_DEEP,
} aceFR_type_t;

/**
 * @brief aceFR_reset_platform, API to start factory reset referecing
 *        factory reset HAL callbacks (resolved in link time)
 * @param type, Factory Reset type
 * @return ACE_STATUS_BAD_PARAM for invalid input type
 * @return ACE_STATUS_GENERAL_ERROR if FR hal open failed
 * @return ACE_STATUS_CANCELED if FR is halted (determined by platform callback)
 * @return ACE_STATUS_OK if everything goes well (might not return)
 */
ace_status_t aceFR_reset(aceFR_type_t type);

/** @} */
#ifdef __cplusplus
}
#endif /* def __cplusplus */

#endif
