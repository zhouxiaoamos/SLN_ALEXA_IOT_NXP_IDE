/*
 * FreeRTOS BLE HAL V5.0.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file bt_avsrc_profile.h
 * @brief BT HAL provides the interfaces for A2DP Source profile for local device.
 * @addtogroup HAL_BLUETOOTH
 * USAGE
 * -----
 *
 * @{
 */


#ifndef _BT_HAL_AVSRC_PROFILE_H
#define _BT_HAL_AVSRC_PROFILE_H

#include <stddef.h>
#include <stdint.h>
#include "bt_hal_manager_types.h"

/**
 * @brief Bluetooth AV connection states
 */
typedef enum
{
    eBTAvsrcConnectionStateDisconnected = 0, /**< Disconnected */
    eBTAvsrcConnectionStateConnecting = 1,   /**< Connecting */
    eBTAvsrcConnectionStateConnected = 2,    /**< Connected */
    eBTAvsrcConnectionStateDisconnecting = 3 /**< Disconnecting */
} BTAvConnectionState_t;

/**
 * @brief Bluetooth AV data path states
 */
typedef enum
{
    eBTAvsrcAudioStateRemoteSuspend = 0, /**< Audio Suspended */
    eBTAvsrcAudioStateStopped = 1,       /**< Audio Stopped */
    eBTAvsrcAudioStateStarted = 2,       /**< Audio Started */
} BTAvAudioState_t;

/** Audio callback structure */

/**
 * @brief Callback invoked in to notify AV connection state change
 *
 * @param[in] xState Connection state
 * @param[in] pxBdAddr Address of the Remote device
 */
typedef void (* BTAvsrcConnectionStateCallback_t)( BTAvConnectionState_t xState,
                                                   BTBdaddr_t * pxBdAddr );

/**
 * @brief Callback invoked in to notify AV Audio state change
 *
 * @param[in] xState Audio state
 * @param[in] pxBdAddr Address of the Remote device
 */
typedef void (* BTAvsrcAudioStateCallback_t)( BTAvAudioState_t xState,
                                              BTBdaddr_t * pxBdAddr );

typedef struct
{
    size_t xSize;
    BTAvsrcConnectionStateCallback_t xConnStateCback;
    BTAvsrcAudioStateCallback_t xAudioStateCback;
} BTAvsrcCallbacks_t;

/** Represents the standard AV connection interface. */
typedef struct
{
    /** Set this to size of  BTAvsrcInterface_t*/
    size_t xSize;

    /**
     * @brief Initializes A2DP module.
     * @param[in] callbacks, BTAvsrcCallbacks_t callbacks.
     * @return Returns eBTStatusSuccess on successful call.
     */
    BTStatus_t ( * pxAvsrcInit )( BTAvsrcCallbacks_t * pxCallbacks );

    /**
     * @brief Cleans up A2DP module
     * @param None
     * @return Returns eBTStatusSuccess on successful call.
     */
    BTStatus_t ( * pxAvsrcCleanup )( void );

    /**
     * @brief Starts connect for A2DP Source profile
     * @param[in] pxRemote_addr BT Address of remote device
     * @return Returns eBTStatusSuccess on successful call.
     */
    BTStatus_t ( * pxAvsrcConnect )( BTBdaddr_t * pxRemote_addr );

    /**
     * @brief Starts disconnect for A2DP Source profile
     *
     * @param[in] pxRemote_addr BT Address of remote device
     * @return Returns eBTStatusSuccess on successful call.
     */
    BTStatus_t ( * pxAvsrcDisconnect )( BTBdaddr_t * pxRemote_addr );
} BTAvsrcInterface_t;

const BTAvsrcInterface_t * BT_GetAvsrcInterface( void );

#endif /* _BT_HAL_AVSRC_PROFILE_H */
/** @} */
