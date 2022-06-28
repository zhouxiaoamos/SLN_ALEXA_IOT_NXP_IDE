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
* @file wifi_mgr_tests.h
*
* @brief Wifi mgr tests header file
*
*
*/
#ifndef __WIFI_MGR_TEST_H__
#define __WIFI_MGR_TEST_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
* @ATT_API{ACE_WIFI ACE_TEST}
* Function to get default testing config
*/
void wifi_mgr_test_get_default_config(void);

/**
* @ATT_API{ACE_WIFI ACE_TEST}
* Function to set testing config
*/
void wifi_mgr_test_set_config(int32_t len, const char* param[]);
#endif  // __WIFI_MGR_TEST_H__
