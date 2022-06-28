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
 * @file wifi_mgr_err.h
 *
 * @brief Wifi Manager API function return codes.
 * @defgroup ACE_WIFI_DEF structs, defines, enums
 * @{
 * @ingroup ACE_WIFI
 */

#ifndef ACE_WIFI_MGR_ERROR_H
#define ACE_WIFI_MGR_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/ace_status.h>

/**
 * @cond DEPRECATED
 * @deprecated Please use the ace_status_t instead.
 * @{
 */

typedef ace_status_t aceWifiMgr_error_t;

#define aceWifiMgr_ERROR_SUCCESS \
    ACE_WIFI_MGR_ERROR_SUCCESS /**< API call success */
#define aceWifiMgr_ERROR_FAILURE \
    ACE_WIFI_MGR_ERROR_FAILURE /**< API call failure */
#define aceWifiMgr_ERROR_INVALID_PARAM \
    ACE_WIFI_MGR_ERROR_INVALID_PARAM /**< API call has invalid parameters */
#define aceWifiMgr_ERROR_NOT_SUPPORTED \
    ACE_WIFI_MGR_ERROR_NOT_SUPPORTED /**< API call not supported */
#define aceWifiMgr_ERROR_WRITE \
    ACE_WIFI_MGR_ERROR_WRITE /**< API call failed to write to lower layer */
#define aceWifiMgr_ERROR_READ \
    ACE_WIFI_MGR_ERROR_READ /**< API call failed to read from lower layer */
#define aceWifiMgr_ERROR_NOMEM                                            \
    ACE_WIFI_MGR_ERROR_NOMEM /**< Internal error indicating out of memory \
                                  situation */
#define aceWifiMgr_ERROR_PROTOCOL                                           \
    ACE_WIFI_MGR_ERROR_PROTOCOL /**< Internal error indicating violation of \
                                   the                                      \
                                     agreed data exchange protocol */
#define aceWifiMgr_ERROR_HANG                                            \
    ACE_WIFI_MGR_ERROR_HANG /**< Internal error indicating thread is not \
                                 responding */
#define aceWifiMgr_ERROR_MOREDATA \
    ACE_WIFI_MGR_ERROR_MOREDATA /**< Indicating sender will send more data */
#define aceWifiMgr_ERROR_IGNORE                                          \
    ACE_WIFI_MGR_ERROR_IGNORE /**< Internal code indicating that further \
                                   processing should be done */
#define aceWifiMgr_ERROR_UNKNOWN \
    ACE_WIFI_MGR_ERROR_UNKNOWN /**< API call failed for unknown reasons */

// Make sure the value here matches with aceWifiHal_xxx
// For example
// #define ACE_WIFI_MGR_ERROR_SUCCESS   ACE_STATUS_OK
// #define aceWifiHal_ERROR_SUCCESS   ACE_STATUS_OK

/**< API call success */
#define ACE_WIFI_MGR_ERROR_SUCCESS ACE_STATUS_OK
/**< API call failure */
#define ACE_WIFI_MGR_ERROR_FAILURE ACE_STATUS_GENERAL_ERROR
/**< API call has invalid parameters */
#define ACE_WIFI_MGR_ERROR_INVALID_PARAM ACE_STATUS_BAD_PARAM
/**< API call not supported */
#define ACE_WIFI_MGR_ERROR_NOT_SUPPORTED ACE_STATUS_NOT_SUPPORTED
/**< API call failed to write to lower layer */
// This macro is not defined correctly, it is only used to align with
// HAL aceWifiHal_ERROR_WRITE, and it's really a type of bad param.
// Using ERROR_WRITE here is confusing.
// Use BAD_PARAM here instead of adding new status code to ace_status
#define ACE_WIFI_MGR_ERROR_WRITE ACE_STATUS_BAD_PARAM
/**< API call failed to read from lower layer */
// For all the cases, this error case was used in hal layer
// while doing getxxx api call, which can be treated with
// ACE_STATUS_GENERAL_ERROR. Using error read here is confusing
#define ACE_WIFI_MGR_ERROR_READ ACE_STATUS_GENERAL_ERROR
/**< Internal error indicating out of memory situation */
#define ACE_WIFI_MGR_ERROR_NOMEM ACE_STATUS_OUT_OF_MEMORY
/**< Internal error indicating violation of the agreed
     data exchange protocol */
#define ACE_WIFI_MGR_ERROR_PROTOCOL ACE_STATUS_PROTOCOL_ERROR
/**< Internal error indicating thread is not responding */
// Only one case using this error code, which is when using aipc sync
// and timeout, so use ACE TIMEOUT code.
#define ACE_WIFI_MGR_ERROR_HANG ACE_STATUS_TIMEOUT
/**< Indicating sender will send more data */
#define ACE_WIFI_MGR_ERROR_MOREDATA ACE_STATUS_MORE_DATA
/**< Internal code indicating that further
     processing should be done */
// This status code is only used in wifi_eap.c indicating
// that fragment is needed or not. It's confusing and should not
// be used, modified the code so that IRNOGRE code is not needed.
#define ACE_WIFI_MGR_ERROR_IGNORE ACE_STATUS_OK
/**< API call failed for unknown reasons */
// This is the same as gernal eror
#define ACE_WIFI_MGR_ERROR_UNKNOWN ACE_STATUS_GENERAL_ERROR

typedef char aceWifiMgr_errStr, *paceWifiMgr_errStr;
#define ACE_ASSERT(x)                                                      \
    {                                                                      \
        if (!(x)) {                                                        \
            aceLog_log(LOG_ERR, "ERROR: [%s] failed at %d line.\n", __func__, \
                    __LINE__);                                             \
            raise(SIGABRT);                                                \
        }                                                                  \
    }
#define ACE_WIFI_NO_ERROR(x) (ACE_STATUS_OK == x)
#define ACE_WIFI_ERROR(x) (ACE_STATUS_OK != x)

/**
 * @}
 * @endcond
 */  // cond DEPRECATED

#ifdef __cplusplus
}
#endif

#endif  // ACE_WIFI_MGR_ERROR_H
/** @} */
