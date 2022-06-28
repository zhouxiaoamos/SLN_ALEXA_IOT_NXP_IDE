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
* @file hsm.h
*
* @brief This module implements the Hierarchical State Machine (HSM).
*
*/

#ifndef __ACE_HSM_H__
#define __ACE_HSM_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Special HSM state marker */
#define ACE_HSM_NULL_STATE (0xFFFF)

/** Maximum allowed nesting levels */
#define ACE_HSM_MAX_NESTING (10)

/** Return HANDLED or NOT_HANDLED in each state's event handler **/
#define ACE_HSM_NOT_HANDLED 0
#define ACE_HSM_HANDLED 1

typedef struct aceHsm_state aceHsm_state_t;

/** HSM data structure */
typedef struct aceHsm_hsm {
    aceHsm_state_t* states; /**< Points to an array of states */
    uint16_t currentState;  /**< The current state of the hsm */
    uint16_t handlerState;  /**< Used to iterate parent states */
} aceHsm_hsm_t;

/**
 * Event handler for an individual state.
 *
 * @return 1 if event is handled, 0 otherwise.
 */
typedef int (*aceHsm_handler_t)(aceHsm_hsm_t* hsm, int type, void* data,
                                uint32_t size);

/**
 * Function type for actions of an individual state (i.e., entry, exit, init).
 */
typedef void (*aceHsm_action_t)(aceHsm_hsm_t* hsm);

/** Data structure for an individual state.
 */
struct aceHsm_state {
    char* name;              /**< state name as string literal */
    uint16_t parent;         /**< parent state enum */
    uint16_t child;          /**< child state enum */
    aceHsm_handler_t handle; /**< event handler for this state */
    aceHsm_action_t entry;   /**< state entry function */
    aceHsm_action_t exit;    /**< state exit function */
    aceHsm_action_t init;    /**< state init function */
};

/**
 * Convenient macro for HSM construction.
 */
#define ACE_HSM_STATE(name, parent, child, handle, entry, exit, init) \
    { #name, parent, child, handle, entry, exit, init }

/**
 * HSM init actions. Must be called first to initialize hsm.
 *
 */
void aceHsm_init(aceHsm_hsm_t* hsm, uint16_t state);

/**
 * HSM event dispatcher.
 *
 */
void aceHsm_dispatch(aceHsm_hsm_t* hsm, int type, void* data, uint32_t size);

/**
 * HSM transition to the handler state, for UML compliant event handling.
 *
 */
void aceHsm_preTransition(aceHsm_hsm_t* hsm);

/**
 * HSM transition to the target state.
 *
 * @param hsm: The hsm to transition.
 * @param target: The target state to transition to.
 */
void aceHsm_transition(aceHsm_hsm_t* hsm, int target);

/**
 * Get the HSM's current state.
 *
 */
inline uint16_t aceHsm_getState(aceHsm_hsm_t* hsm) {
    return hsm->currentState;
}

/**
 * Check if the HSM's current state is inside the given state.
 *
 * @param hsm: The hsm.
 * @param state: The state to check.
 * @return int: 1 if the hsm is currently in state, 0 otherwise.
 */
int aceHsm_inState(aceHsm_hsm_t* hsm, uint16_t state);

/**
 * Validate the HSM structure. Useful during dev time.
 *
 * @param hsm: The hsm.
 * @param max: The max (last) defined state in the hsm.
 * @return int: 1 if valid, 0 otherwise.
 */
int aceHsm_validate(aceHsm_hsm_t* hsm, uint16_t max);

/**
 * HSM handler for debugging. It prints the state name and event type.
 *
 */
int aceHsm_debugHandler(aceHsm_hsm_t* hsm, int type, void* data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif  // __ACE_HSM_H__
