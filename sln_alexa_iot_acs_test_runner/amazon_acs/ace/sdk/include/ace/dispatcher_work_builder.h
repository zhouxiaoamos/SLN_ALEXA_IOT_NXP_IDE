/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All rights reserved.
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
 * @file       dispatcher_work_builder.h
 * @brief      Dispatcher Work Builder API.
 *
 * @details    This API allows the user to post work to the dispatcher using
 *             more complex constraints on its execution.
 *
 * @addtogroup ACE_DISPATCHER_WORK_BUILDER_API
 * @{
 */

#ifndef DISPATCHER_WORK_BUILDER_DOT_H
#define DISPATCHER_WORK_BUILDER_DOT_H

#include <ace/dispatcher_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Initialize work item for the dispatcher.
 * @details    This function initializes the work item and sets the core
 *             attributes of work as defined by the dispatcher. The work pointer
 *             also serves as a handle to refer to the deferred work.
 * @note       Avoid stack allocation for the work item if the user wishes
 *             to be able to defer or cancel work. On the other hand, if the
 *             user's intent is to schedule work execution without delay and the
 *             need to cancel in the future, the work item can be on the
 *             stack.
 *
 * @param[out] w      Pointer to the memory where the work item is
 *                    initialized.
 * @param[in]  event  Event to post to the dispatcher.
 * @param[in]  msg    Pointer to the message.
 * @param[in]  len    Length of the message.
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 */
ace_status_t aceDispatcher_createWork(aceDispatcher_work_t* w, int event,
                                      void* msg, size_t len);

/**
 * @brief      Set time attributes in the work item.
 * @details    This function sets the delay and interval attributes in the work
 *             item as defined by the dispatcher.
 *
 * @param[out] w            Pointer to the memory where the work item is
 *                          initialized.
 * @param[in]  delay_ms     Time in milliseconds to defer scheduling of the
 *                          work.
 * @param[in]  interval_ms  Time in milliseconds to repeated scheduling of the
 *                          work.
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 */
ace_status_t aceDispatcher_setPeriod(aceDispatcher_work_t* w, uint32_t delay_ms,
                                     uint32_t interval_ms);

/**
 * @brief      Set priority attribute in the work item.
 * @details    This function sets the priority attribute in the work item as
 *             defined by the dispatcher.
 *
 * @param[out] w         Pointer to the memory where the work item is
 *                       initialized.
 * @param[in]  priority  Priority value of the work item whose value is one of
 *                       ACE_DISPATCHER_PRIORITY_* macros.
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 */
ace_status_t aceDispatcher_setPriority(aceDispatcher_work_t* w,
                                       aceDispatcher_priority_t priority);
#ifdef __cplusplus
}
#endif

#endif  // DISPATCHER_WORK_BUILDER_DOT_H
/** @} */
