/*
 * Copyright 2019-2021 Amazon.com, Inc. or its affiliates. All rights reserved.
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
 * @File maplite_db_keys.h
 * @brief ACS Maplite database keys, share KV Storage key names
 *
 * Sample code to read the value:
 * char buf[MAPLITE_KV_CUSTOMER_ID_KEY_MAX_LEN] = {0};
 * int rc;
 * rc = aceKeyValueDsHal_get(MAPLITE_KV_CUSTOMER_ID_KEY, buf, sizeof(buf));
 * Return the length of data actually getting in byte, or negative ace_status_t
 * error code.
 */

#ifndef _MAPLITE_DB_KEYS_H_
#define _MAPLITE_DB_KEYS_H_

#include <ace/ace_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup MAPLITE_DB_KEYS Maplite database KVS keys
 * @{
 * @ingroup MapLite
 */
#define MAPLITE_KV_CUSTOMER_ID_KEY_MAX_LEN 128
#define MAPLITE_KV_COR_KEY_MAX_LEN 8
#define MAPLITE_KV_MARKETPLACE_KEY_MAX_LEN 64
#define MAPLITE_KV_AWS_IOT_THING_NAME_MAX_LEN 128
#define MAPLITE_KV_AWS_IOT_ENDPOINT_MAX_LEN 64

/**
 * @brief Customer information keys
 */
#define MAPLITE_KV_CUSTOMER_ID_KEY ACE_MAPLITE_DS_GROUP ".cid"
#define MAPLITE_KV_COR_KEY ACE_MAPLITE_DS_GROUP ".country_of_residence"
#define MAPLITE_KV_MARKETPLACE_KEY ACE_MAPLITE_DS_GROUP ".preferred_marketplace"
#define MAPLITE_KV_USER_NAME_KEY ACE_MAPLITE_DS_GROUP ".user_name"

/**
 * @brief IOT information keys
 */
#define MAPLITE_KV_AWS_IOT_KEY ACE_MAPLITE_DS_GROUP ".aws_iot_key"
#define MAPLITE_KV_AWS_IOT_CERT ACE_MAPLITE_DS_GROUP ".aws_iot_cert"
#define MAPLITE_KV_AWS_IOT_THING_NAME ACE_MAPLITE_DS_GROUP ".aws_iot_thing_name"
#define MAPLITE_KV_AWS_IOT_ENDPOINT ACE_MAPLITE_DS_GROUP ".aws_iot_endpoint"

/** @} */

#ifdef __cplusplus
}
#endif

#endif /*_MAPLITE_DB_KEYS_H_*/
