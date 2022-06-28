/*
 * FreeRTOS BLE HAL V5.0.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file bt_hal_avrc_ctrl_profile.h
 * @brief BT HAL provides the interfaces for Avrc Controller profile.
 * This interface provides APIs to send and receive passthrough keys for the
 * profile.
 *
 * @addtogroup HAL_BLUETOOTH
 * @{
 */

#ifndef _BT_HAL_BT_AVRC_CTRL_PROFILE_H
#define _BT_HAL_BT_AVRC_CTRL_PROFILE_H

#include "bt_hal_avrcp.h"

/**
 * @brief Incoming AVRC Controller key event callback for passthrough command
 *
 * @param[in] p_remote_addr pointer to remote device address @ref BTBdaddr_t
 * @param[in] key_id The key sent by remote @ref BTAvrcCtrlKeyId_t
 * @param[in] key_state The key state sent @ref BTAvrcCtrlKeyState_t
 */
typedef void (* acebthal_incoming_rc_key_callback)( BTBdaddr_t * p_remote_addr,
                                                    BTAvrcCtrlKeyId_t key_id,
                                                    BTAvrcCtrlKeyState_t key_state );

#ifdef ACE_HAL_BLUETOOTH_AVRCP_CT

/**
 * @brief AVRCP Controller remote device connection state changed
 *
 * @param[in] xState new connection state @ref BTAvrcpConnectionState_t
 * @param[in] pxAddr pointer to remote device address @ref BTBdaddr_t
 */
    typedef void (* BTAvrcpCtConnStateChangedCallback_t)( BTAvrcpConnectionState_t xState,
                                                          BTBdaddr_t * pxBdAddr );

/**
 * @brief Report supported features on connected AVRCP Target
 *
 * @param[in] pxAddr pointer to remote device address @ref BTBdaddr_t
 * @param[in] xFeatures Remote device supported features @ref
 * BTAvrcpRemoteFeatures_t
 */
    typedef void (* BTAvrcpCtGetRcFeaturesCallback_t)( BTBdaddr_t * pxAddr,
                                                       BTAvrcpRemoteFeatures_t xFeatures );

/**
 * @brief Set application setting response
 *
 * @param[in] pxAddr pointer to remote device address @ref BTBdaddr_t
 * @param[in] bAccepted flag to indicate if the set request was accepted or not
 */
    typedef void (* BTAvrcpCtSetAppSettingRspCallback_t)( BTBdaddr_t * pxAddr,
                                                          bool bAccepted );

/**
 * @brief Application settings changed
 *
 * @param[in] pxAddr pointer to remote device address @ref BTBdaddr_t
 * @param[in] uNumAttr number of changed settings
 * @param[in] pxAttrIds array of changed settings ID's
 * @param[in] puAttrVals array of changed settings values
 */
    typedef void (* BTAvrcpCtAppSettingChangedCallback_t)( BTBdaddr_t * pxAddr,
                                                           uint8_t uNumAttr,
                                                           BTAvrcpAppSettingId_t * pxAttrIds,
                                                           uint8_t * puAttrVals );

/**
 * @brief Notification received
 *
 * @param[in] pxAddr pointer to remote device address @ref BTBdaddr_t
 * @param[in] xNotifType notification type @ref BTAvrcpNtfType_t
 * @param[in] pxNotification notification value @ref BTAvrcpNtfPayload_t
 */
    typedef void (* BTAvrcpCtNotificationCallback_t)( BTBdaddr_t * pxAddr,
                                                      BTAvrcpNtfType_t xNotifType,
                                                      BTAvrcpNtfPayload_t * pxNotification );
#endif /* ifdef ACE_HAL_BLUETOOTH_AVRCP_CT */

typedef struct
{
    size_t size;
    acebthal_incoming_rc_key_callback rc_key_cb;
    #ifdef ACE_HAL_BLUETOOTH_AVRCP_CT
        BTAvrcpCtConnStateChangedCallback_t pxConnStsChangedCb;
        BTAvrcpCtGetRcFeaturesCallback_t pxGetRcFeaturesCb;
        BTAvrcpCtSetAppSettingRspCallback_t pxSetAppSettingsRspCb;
        BTAvrcpCtAppSettingChangedCallback_t pxAppSettingsChangedCb;
        BTAvrcpCtNotificationCallback_t pxNotificationCb;
    #endif
} BTAvrcCtrlCallbacks_t;

/** Represents the standard AVRC Controller interface. */
typedef struct
{
    /** Set this to size of BTAvrcCtrlInterface_t*/
    size_t size;

    /**
     * Initializes AVRCP Controller module.
     *
     * @param[in] callbacks callback for AVRC
     * @return Returns eBTStatusSuccess on successful call.
     */
    BTStatus_t ( * pxAvrcCtrlInit )( BTAvrcCtrlCallbacks_t * callbacks );

    /**
     * Cleans up AVRCP Controller module
     *
     * @param None
     * @return Returns eBTStatusSuccess on successful call.
     */
    BTStatus_t ( * pxAvrcCtrlCleanup )( void );

    /**
     * @brief API to send a passthrough key event for AVRC Controller profile
     *  of remote device.
     *
     * @param[in] remote_addr pointer to remote device address @ref
     * BTBdaddr_t
     * @param[in] key_id The key to be sent @ref BTAvrcCtrlKeyId_t
     * @param[in] key_state The key state to be sent @ref BTAvrcCtrlKeyState_t
     * @return Returns eBTStatusSuccess on successful call.
     */
    BTStatus_t ( * pxAvrcCtrlSendKeyEvent )( BTBdaddr_t * remote_addr,
                                             BTAvrcCtrlKeyId_t key_id,
                                             BTAvrcCtrlKeyState_t key_state );

    #ifdef ACE_HAL_BLUETOOTH_AVRCP_CT
        /**
         * @brief API to set player application setting of target device.
         *
         * @param[in] pxAddr pointer to remote device address @ref
         * BTBdaddr_t
         * @param[in] uNumAttr number of settings to be changed
         * @param[in] pxAttrIds array of settings ID's to be changed @ref
         * BTAvrcpAppSettingId_t
         * @param[in] puAttrVals array of settings values to be changed
         * @return Returns eBTStatusSuccess on successful call, else error code.
         */
        BTStatus_t ( * pxAvrcpCtSetAppSetting )( BTBdaddr_t * pxAddr,
                                                 uint8_t uNumAttr,
                                                 BTAvrcpAppSettingId_t * pxAttrIds,
                                                 uint8_t * puAttrVals );
    #endif
} BTAvrcCtrlInterface_t;
const BTAvrcCtrlInterface_t * BT_GetAvrcCtrlInterface( void );
#endif /* _BT_HAL_BT_AVRC_CTRL_PROFILE_H */
/** @} */
