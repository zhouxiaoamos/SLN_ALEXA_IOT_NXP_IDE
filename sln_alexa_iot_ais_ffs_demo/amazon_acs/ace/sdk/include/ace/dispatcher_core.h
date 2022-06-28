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

#ifndef DISPATCHER_CORE_DOT_H
#define DISPATCHER_CORE_DOT_H

/**
 * @file       dispatcher_core.h
 * @brief      Implementation of the dispatcher pattern for Ace.
 * @ingroup    ACE_DISPATCHER_API
 */

#include <ace/queue.h>
#include <ace/ace.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The config max size depends on configuration in FreeRTOSConfig.h
#define ACE_DISPATCHER_CONFIG_MAX_SIZE 800
#define ACE_DISPATCHER_WORK_MAX_SIZE 96
#define ACE_DISPATCHER_MAX_QUEUE_ITEMS 20
#define ACE_DISPATCHER_MODULE_NAME_MAXLEN 16

#define ACE_DISPATCHER_VERBOSITY_NONE 0b00000000
#define ACE_DISPATCHER_VERBOSITY_DEBUG 0b00000001
#define ACE_DISPATCHER_VERBOSITY_TIMING 0b00000010

typedef enum {
    ACE_DISPATCHER_PRIORITY_NORMAL = 5,
    ACE_DISPATCHER_PRIORITY_URGENT = 10
} aceDispatcher_priority_t;

typedef struct aceDispatcher_dispatchHandle_s aceDispatcher_dispatchHandle_t;

/**
 * @brief      This type defines the dispatcher handle
 */
typedef union {
    /** Type with max alignment in C */
    long long maxalign;
    /** Type with max alignment in C */
    double altmaxalign;
    char buf[ACE_DISPATCHER_CONFIG_MAX_SIZE];
} aceDispatcher_config_t;

/**
 * @brief      Function prototype for the module handler.
 */
typedef ace_status_t (*OnMsgCb)(int event, void* msg, size_t len, void* ctx);

/**
 * @brief      Function prototype for the onregister cb.
 */
typedef ace_status_t (*OnRegisteredCb)(aceDispatcher_dispatchHandle_t* h,
                                       void* ctx);

/**
 * @brief      Function prototype for the onderegister cb.
 */
typedef ace_status_t (*OnDeRegisterCb)(void* ctx);

/**
 * @brief      Dispatch handler interface definition structure.
 * @details    Modules implement the dispatch handler interface. Dispatcher
 *             calls the appropriate callbacks defined here when lifecycle
 *             events occur or when work scheduled for the module arrives.
 */
typedef struct aceDispatcher_module_s {
    /** Module name */
    char mod_name[ACE_DISPATCHER_MODULE_NAME_MAXLEN + 1];
    /** Module's function to call to process an incoming msg */
    OnMsgCb on_msg;
    /** Module's function to call upon registering with dispatcher */
    OnRegisteredCb on_reg;
    /** Module's function to call upon deregistering with dispatcher */
    OnDeRegisterCb on_dereg;
    /** Data passed back in the on_reg and on_msg cb, opaque to dispatcher */
    void* modctx;
    /** Reserved for future */
    int32_t queue_size;
} aceDispatcher_module_t;

/**
 * @brief      Dispatch handle for the module to use.
 */
struct aceDispatcher_dispatchHandle_s {
    /** Dispatcher to which the module is registered */
    aceDispatcher_config_t* dispatcher;
    aceDispatcher_module_t module;
    uint32_t magic;
    TAILQ_ENTRY(aceDispatcher_dispatchHandle_s) peers;
};

/**
 * @brief      This type defines the work object
 */
typedef union {
    /* Type with max alignment in C */
    long long maxalign;
    /* Type with max alignment in C */
    double altmaxalign;
    char buf[ACE_DISPATCHER_WORK_MAX_SIZE];
} aceDispatcher_work_t;

/**
 * @brief      Initialize the dispatcher module.
 * @details    This function must be called to initialize the dispatcher module
 *             before any other API in this header is used. Must not be called
 *             more than once.
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_init(void);

/**
 * @brief      Deinitialize the dispatcher module.
 * @details    This function must be called to deinitialize the dispatcher
 *             module.
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_deinit(void);

/**
 * @brief      Create an instance of the dispatcher.
 * @details    This function creates an instance of the dispatcher by creating a
 *             thread and a queue. It returns a dispatcher handle back to the
 *             caller that can be used for subsequent operations.
 * @note       The queue_size and thread priority must be appropriately chosen
 *             based on the modules that are expected to run inside the
 *             dispatcher.
 * @warning    Using copies of the dispatcher handle results in undefined
 *             behavior.
 *
 * @param[in]  dp               Pointer to the memory for the dispatcher
 *                              handle
 * @param[in]  name             Name of the queue
 * @param[in]  queue_size       Number of elements in the queue
 * @param[in]  stack_size       Stack size of the thread
 * @param[in]  thread_priority  Priority of the thread
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_create(aceDispatcher_config_t* dp, const char* name,
                                  int32_t queue_size, int32_t stack_size,
                                  int32_t thread_priority);

/**
 * @brief      Destroy an instance of the dispatcher.
 * @details    This function destroys an instance of the dispatcher by ending
 *             the thread and deleting the queue. All registered modules must
 *             have been unregistered previously.
 * @note       This function blocks and may only be called from outside the
 *             dispatcher itself. Do not post a message to the dispatcher that
 *             in turn calls destroy. Instead directly call this api from an
 *             external user thread.
 *
 * @param[in]  dp    Pointer to the dispatcher handle
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_destroy(aceDispatcher_config_t* dp);

/**
 * @brief         Register a module with a dispatcher.
 * @details       This function registers a module with an instance of the
 *                dispatcher. Once registered, the module's on_reg callback is
 *                called with a reference to the dispatch handle. Modules can
 *                then schedule work for execution in the dispatcher after this
 *                point if registration is successful.
 *
 * @param[in]     dp    Pointer to the dispatcher handle
 * @param[in]     mod   Pointer to the dispatch handler interface struct
 * @param[in,out] h     Module dispatch handle
 *
 * @return        ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup       ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_registerModule(aceDispatcher_config_t* dp,
                                          const aceDispatcher_module_t* mod,
                                          aceDispatcher_dispatchHandle_t* h);

/**
 * @brief      Deregister a module from a dispatcher.
 * @details    This function deregisters a module from an instance of the
 *             dispatcher. Once deregistered, the module's on_dereg cb is called
 *             if not NULL.
 * @note       For any message already in the dispatch queue would still result
 *             in its on_msg cb getting called. However, modules may track the
 *             fact that on_dereg cb has been called and drop the message.
 *
 * @param[in]  h     Module dispatch handle
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_deregisterModule(aceDispatcher_dispatchHandle_t* h);

/**
 * @brief      Post a message to a dispatcher without blocking.
 * @details    This function posts a message to the execution queue of a
 *             dispatcher or returns immediately without blocking. Upon dequeue,
 *             the module's on_msg callback is called.
 * @warning    Care should be taken while allocating memory to which msg points.
 *             If the caller is blocking on the response from dispatcher, the
 *             msg may be placed on the stack, else if the post triggers
 *             asynchronous actions, its best to allocate memory from the heap
 *             or mempool.
 * @warning    Do not block for results of execution when posting from within a
 *             dispatcher context.
 *
 * @param[in]  h      Module dispatch handle
 * @param[in]  event  Event to post to the dispatcher
 * @param[in]  msg    Pointer to the message
 * @param[in]  len    Length of the message
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_DISPATCH_API
 */
ace_status_t aceDispatcher_postTry(aceDispatcher_dispatchHandle_t* h, int event,
                                   void* msg, size_t len);

/**
 * @brief      Post a message to a dispatcher with brief blocking.
 * @details    This function posts a message to the queue of a dispatcher
 *             blocking briefly for the operation to succeed. Upon dequeue,
 *             the module's on_msg callback is called.
 * @warning    Care should be taken while allocating memory to which msg points.
 *             If the caller is blocking on the response from dispatcher, the
 *             msg may be placed on the stack, else if the post triggers
 *             asynchronous actions, its best to allocate memory from the heap
 *             or mempool.
 * @warning    Do not block for results of execution when posting from within a
 *             dispatcher context.
 *
 * @param[in]  h      Module dispatch handle
 * @param[in]  event  Event to post to the dispatcher
 * @param[in]  msg    Pointer to the message
 * @param[in]  len    Length of the message
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_DISPATCH_API
 */
ace_status_t aceDispatcher_post(aceDispatcher_dispatchHandle_t* h, int event,
                                void* msg, size_t len);

/**
 * @brief      Post work to a dispatcher.
 * @details    This function posts work to the queue of a dispatcher for
 *             execution in FIFO order. If the user wishes to be able to cancel
 *             the work in the future, the same work pointer serves as a handle
 *             to refer to the deferred work. For such use-cases avoid stack
 *             allocation for the work struct. On the other hand, if the user's
 *             intent is to schedule work execution without delay and the need
 *             to cancel in the future, the work struct can be on the user's
 *             stack.
 *
 * @param[in]  h     Pointer to module dispatch handle
 * @param[in]  w     Pointer to the work to be dispatched
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_DISPATCH_API
 */
ace_status_t aceDispatcher_postEx(aceDispatcher_dispatchHandle_t* h,
                                  aceDispatcher_work_t* w);

/**
 * @brief      Cancel work posted to a dispatcher.
 * @details    This function cancels work posted to the dispatcher previously.
 *             This operation may fail if the work is already in the execution
 *             queue.
 *
 * @param[in]  h     Pointer to module dispatch handle
 * @param[in]  w     Pointer to the work to be canceled
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_DISPATCH_API
 */
ace_status_t aceDispatcher_cancel(aceDispatcher_dispatchHandle_t* h,
                                  aceDispatcher_work_t* w);

/**
 * @brief      Join the dispatcher thread.
 * @details    This function joins the underlying dispatcher thread with the
 *             calling thread. It blocks the calling thread until the dispatcher
 *             thread is done. Hence,this call blocks indefinitely as a
 *             dispatcher thread never exits.
 *
 * @param[in]  dp    Dispatcher handle
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_join(aceDispatcher_config_t* dp);

/**
 * @brief      Get registered module count.
 * @details    This function returns the number of modules registered to the
 *             dispatcher.
 *
 * @param[in]  dp    Dispatcher handle
 *
 * @return     0 on if dp is NULL, else module count.
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
int aceDispatcher_getModuleCount(aceDispatcher_config_t* dp);

/**
 * @brief      Control log verbosity.
 * @details    This function allows user to turn on more verbose logging for a
 *             specific dispatcher. This API is useful when a dispatcher module
 *             wants to enable debugging from code.
 *
 * @param[in]  h      Module dispatch handle
 * @param[in]  flags  One or more verbosity flags ORed together
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_setVerbosity(aceDispatcher_dispatchHandle_t* h,
                                        uint8_t flags);

/**
 * @brief      Control log verbosity.
 * @details    This function allows user to turn on more verbose logging for a
 *             specific dispatcher. This API is useful to turn on debugging of a
 *             specific dispatcher and is typically used in conjunction with the
 *             aceDispatcher_debug to find the index of the dispatcher to turn
 *             on debugging.
 *
 * @param[in]  idx    Dispatcher index from CLI output
 * @param[in]  flags  One or more verbosity flags ORed together
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_setVerbosityByIdx(uint8_t idx, uint8_t flags);

/**
 * @brief      Print debugging information about dispatcher.
 * @details    This function prints a list of dispatchers and the modules
 *             registered to each one. The debug information is for all
 *             dispatchers in the current process. RTOS is treated as though
 *             there is only one process in the entire system.
 *
 * @return     The sum of dispatchers and modules found or a status from
 *             ace_status_t on error
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_debug(void);

/**
 * @brief         Initialize a dispatch handle to post to the system dispatcher.
 * @details       This function initializes a dispatch handle for the module to
 *                post to the system dispatcher.
 * @note          Unlike `aceDispatcher_registerModule`, this API will not
 *                result in the module's on_reg callback to be called.
 * @note          Users of this API must not call
 *                `aceDispatcher_deregisterModule` as the module is not
 *                registered to the dispatcher.
 *
 * @param[in]     mod   Pointer to the dispatch handler interface struct.
 * @param[in,out] h     Module dispatch handle.
 *
 * @return        ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup       ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t
aceDispatcher_getSystemDispatchHandle(const aceDispatcher_module_t* mod,
                                      aceDispatcher_dispatchHandle_t* h);

/**
 * @brief      Designate a dispatcher as the system dispatcher.
 * @details    This function sets a specific dispatcher as the system
 *             dispatcher.
 *
 * @param[in]  dp     Pointer to the memory for the dispatcher.
 *
 * @return     ACE_STATUS_OK on success, else error code from ace_status_t.
 * @ingroup    ACE_DISPATCHER_CONFIGURATION_API
 */
ace_status_t aceDispatcher_setSystemDispatcher(aceDispatcher_config_t* dp);

#ifdef __cplusplus
}
#endif

#endif  // DISPATCHER_CORE_DOT_H
