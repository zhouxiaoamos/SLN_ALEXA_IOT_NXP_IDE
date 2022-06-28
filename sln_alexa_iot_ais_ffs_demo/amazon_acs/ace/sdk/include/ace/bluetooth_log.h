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
#ifndef BT_LOG_H
#define BT_LOG_H

#include "stdio.h"
#include <sys/types.h>
#include <unistd.h>
#include <ace/ace_log.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BT_LOGTAG
#define BT_LOGTAG "ACEBT"
#endif

#undef BT_LOGI
#define BT_LOGI(...) ACE_LOGI(ACE_LOG_ID_MAIN, BT_LOGTAG, __VA_ARGS__)

#undef BT_LOGV
#ifdef ACEBT_LITE
// Disable Verbose logs for ACE Lite builds
#define BT_LOGV(...)
#else
#define BT_LOGV(...) ACE_LOGV(ACE_LOG_ID_MAIN, BT_LOGTAG, __VA_ARGS__)
#endif  // ACEBT_LITE

#undef BT_LOGD
#define BT_LOGD(...) ACE_LOGD(ACE_LOG_ID_MAIN, BT_LOGTAG, __VA_ARGS__)

#undef BT_LOGE
#define BT_LOGE(...) ACE_LOGE(ACE_LOG_ID_MAIN, BT_LOGTAG, __VA_ARGS__)

#undef BT_LOGW
#define BT_LOGW(...) ACE_LOGW(ACE_LOG_ID_MAIN, BT_LOGTAG, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif  // BT_LOG_H
