/*
 * Copyright 2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __RT106A_IOT_LOG_H__
#define __RT106A_IOT_LOG_H__

#include <ace/ace_log.h>

#define RT106A_LOGI(TAG, formatString, args...) do {\
    ACE_LOGI(ACE_LOG_ID_SYSTEM, TAG, formatString, ##args);\
 }while(0)

#define RT106A_LOGD(TAG, formatString,  args...) do {\
    ACE_LOGD(ACE_LOG_ID_SYSTEM, TAG, formatString,  ##args);\
}while(0)

#define RT106A_LOGE(TAG, formatString,  args... ) do {\
     ACE_LOGE(ACE_LOG_ID_SYSTEM, TAG, formatString,  ##args);\
 }while(0)

#endif
