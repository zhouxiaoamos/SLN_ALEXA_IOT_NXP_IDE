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
 * @File maplite_svc.h
 * @brief ACS MapLite Service Layer. Service layer API will not be used directly
 *   by Maplite clients.
 */

#ifndef _ACEMAP_SVC_H_
#define _ACEMAP_SVC_H_

#include <ace/ace_status.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup MAPLITE_SVC_DATA Maplite client data types
 * @{
 * @ingroup MapLite MapLiteAsync AceMap
 */

/* for future extensibility */
typedef struct {
} aceMaplite_svc_server_params_t;

/** @} */

/**
 * @defgroup MAPLITE_SVC_API Maplite service layer APIs
 * @{
 * @ingroup MapLite MapLiteAsync AceMap
 */

/**
 * @brief Initializes the Client service layer
 * @return ACE_STATUS_OK if no errors
 * @return ACE_GENERAL_ERROR for failures
 */
ace_status_t aceMaplite_initClientSvc(void);

/**
 * @brief Initializes the Server service layer
 * @param[in] p, service server configuration parameters (can be NULL)
 * @return ACE_STATUS_OK if no errors
 * @return ACE_GENERAL_ERROR for failures
 */
ace_status_t aceMaplite_initServerSvc(aceMaplite_svc_server_params_t* p);

/**
 * @brief Service layer routine to emit Map event
 * @param[in] type, Maplite event
 * @param[in] data, Maplite event data
 * @param[in] type, Maplite event data size
 * @return ACE_STATUS_OK if no errors
 * @return ACE_GENERAL_ERROR for failures
 */
ace_status_t aceMaplite_emitEvent(uint32_t type, const void* data, size_t size);

/** @} */

/** @} */
#ifdef __cplusplus
}
#endif /* def __cplusplus */

#endif /*_ACEMAP_SVC_H_*/
