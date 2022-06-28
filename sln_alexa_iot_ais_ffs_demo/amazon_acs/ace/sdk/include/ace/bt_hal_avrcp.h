/*
 * FreeRTOS BLE HAL V5.0.1
 * Copyright (C) 2020-2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * @file bt_hal_avrcp.h
 * @brief BT HAL provides the interfaces for AVRCP.
 *
 * @addtogroup HAL_BLUETOOTH
 * @{
 */

#ifndef _BT_HAL_AVRCP_H
#define _BT_HAL_AVRCP_H

#include <ace/ace_config.h>
#include "bt_hal_manager.h"

/**
 * @brief AVRCP Metadata Attribute String Length
 */
#define BTHAL_AVRCP_METADATA_ATTR_STR_LEN    255

/**
 * @brief Bluetooth AVRCP Track ID Length
 */
#define BTHAL_AVRCP_TRACK_ID_LEN             8

/**
 * @brief AVRCP Metadata Attributes
 */
typedef enum
{
    eBTAvrcpAttrTitle = 0x01,       /**< Title */
    eBTAvrcpAttrArtist = 0x02,      /**< Artist */
    eBTAvrcpAttrAlbum = 0x03,       /**< Album */
    eBTAvrcpAttrTrackNum = 0x04,    /**< Track number */
    eBTAvrcpAttrNumTracks = 0x05,   /**< Total number of tracks */
    eBTAvrcpAttrGenre = 0x06,       /**< Genre */
    eBTAvrcpAttrPlayingTime = 0x07, /**< Playing time */
} BTAvrcpMetadataAttrId_t;

/**
 * @brief AVRCP Play Status Values
 */
typedef enum
{
    eBTAvrcpStopped = 0x00, /**< Stopped */
    eBTAvrcpPlaying = 0x01, /**< Playing */
    eBTAvrcpPaused = 0x02,  /**< Paused  */
    eBTAvrcpFwdSeek = 0x03, /**< Fwd Seek*/
    eBTAvrcpRevSeek = 0x04, /**< Rev Seek*/
    eBTAvrcpError = 0xFF,   /**< Error   */
} BTAvrcpPlayState_t;

/**
 * @brief Struct for AVRCP Metadata Attributes
 */
typedef struct
{
    /** Metadata attribute ID */
    BTAvrcpMetadataAttrId_t xAttrId;
    /** Metadata attribute value */
    uint8_t ucAttrVal[ BTHAL_AVRCP_METADATA_ATTR_STR_LEN ];
} BTAvrcpMetadataAttr_t;

/**
 * @brief Struct for AVRCP Play status Attributes
 */
typedef struct
{
    /** Metadata attribute ID */
    uint32_t ulAudioLen;
    uint32_t ulAudioPos;
    BTAvrcpPlayState_t xPlayState;
} BTAvrcpPlayStatus_t;

/**
 * @brief Bluetooth AVRCP Notification type (AVRCP Specification
 * V1.3, Table 5.28).
 */
typedef enum
{
    eBTAvrcpPlayStatusChanged = 0x01,   /**< Play Status Changed */
    eBTAvrcpTrack_changed = 0x02,       /**< Track Changed */
    eBTAvrcpTrackReachedEnd = 0x03,     /**< Track Ended  */
    eBTAvrcpTrackReachedStart = 0x04,   /**< Track Started */
    eBTAvrcpPlayPosChanged = 0x05,      /**< Play Position Changed */
    eBTAvrcpBattStatusChanged = 0x06,   /**< Play Position Changed */
    eBTAvrcpSystemStatusChanged = 0x07, /**< Play Position Changed */
    eBTAvrcpAppSettingChanged = 0x08,   /**< Applicatioon Settings Changed*/
} BTAvrcpNtfType_t;

/**
 * @brief Bluetooth AVRCP Notification Response type.
 */
typedef enum
{
    eBTAvrcpResponseInterim = 0x00, /**< Interim response */
    eBTAvrcpResponseChanged = 0x01, /**< Changed response */
} BTAvrcpNtfRsp_t;

/**
 * @brief Union for AVRCP Notification value
 */
typedef union
{
    BTAvrcpPlayState_t
        xPlayState;                                 /**< Play State for @ref eBTAvrcpPlayStatusChanged*/
    uint8_t ausTrackId[ BTHAL_AVRCP_TRACK_ID_LEN ]; /**< Track ID for @ref
                                                     * eBTAvrcpTrack_changed*/
    uint32_t ulAudioPos;                            /**< Audio Position for @ref eBTAvrcpPlayPosChanged*/
} BTAvrcpNtfPayload_t;

/**
 * @brief AVRCP Key IDs that are supported for BT
 */
typedef enum
{
    BTHAL_AVRC_CTRL_KEY_ID_VOL_UP = 0x41,       /**< Up */
    BTHAL_AVRC_CTRL_KEY_ID_VOL_DOWN = 0x42,     /**< Down */
    BTHAL_AVRC_CTRL_KEY_ID_VOL_PLAY = 0x44,     /**< Play */
    BTHAL_AVRC_CTRL_KEY_ID_VOL_STOP = 0x45,     /**< Stop */
    BTHAL_AVRC_CTRL_KEY_ID_VOL_PAUSE = 0x46,    /**< Pause */
    BTHAL_AVRC_CTRL_KEY_ID_VOL_REWIND = 0x48,   /**< Rewind */
    BTHAL_AVRC_CTRL_KEY_ID_VOL_FF = 0x49,       /**< Fast Forward */
    BTHAL_AVRC_CTRL_KEY_ID_VOL_FORWARD = 0x4B,  /**< Forward */
    BTHAL_AVRC_CTRL_KEY_ID_VOL_BACKWARD = 0x4C, /**< Backward */
} BTAvrcCtrlKeyId_t;

#define BTHAL_AVRCP_KEY_ID_VOL_UP          BTHAL_AVRC_CTRL_KEY_ID_VOL_UP
#define BTHAL_AVRCP_KEY_ID_VOL_DOWN        BTHAL_AVRC_CTRL_KEY_ID_VOL_DOWN
#define BTHAL_AVRCP_KEY_ID_VOL_PLAY        BTHAL_AVRC_CTRL_KEY_ID_VOL_PLAY
#define BTHAL_AVRCP_KEY_ID_VOL_STOP        BTHAL_AVRC_CTRL_KEY_ID_VOL_STOP
#define BTHAL_AVRCP_KEY_ID_VOL_PAUSE       BTHAL_AVRC_CTRL_KEY_ID_VOL_PAUSE
#define BTHAL_AVRCP_KEY_ID_VOL_REWIND      BTHAL_AVRC_CTRL_KEY_ID_VOL_REWIND
#define BTHAL_AVRCP_KEY_ID_VOL_FF          BTHAL_AVRC_CTRL_KEY_ID_VOL_FF
#define BTHAL_AVRCP_KEY_ID_VOL_FORWARD     BTHAL_AVRC_CTRL_KEY_ID_VOL_FORWARD
#define BTHAL_AVRCP_KEY_ID_VOL_BACKWARD    BTHAL_AVRC_CTRL_KEY_ID_VOL_BACKWARD

typedef BTAvrcCtrlKeyId_t BTAvrcpKeyId_t;

/**
 * @brief AVRCP key states
 */
typedef enum
{
    BTHAL_AVRC_CTRL_KEY_STATE_PRESSED = 0, /**< Pressed */
    BTHAL_AVRC_CTRL_KEY_STATE_RELEASED = 1 /**< Released */
} BTAvrcCtrlKeyState_t;

#define BTHAL_AVRCP_KEY_STATE_PRESSED     BTHAL_AVRC_CTRL_KEY_STATE_PRESSED
#define BTHAL_AVRCP_KEY_STATE_RELEASED    BTHAL_AVRC_CTRL_KEY_STATE_RELEASED

typedef BTAvrcCtrlKeyState_t BTAvrcpKeyState_t;

/**
 * @brief AVRCP Supported features
 */
typedef enum
{
    BTHAL_AVRCP_FEAT_NONE = 0x00,     /**< AVRCP 1.0 */
    BTHAL_AVRCP_FEAT_METADATA = 0x01, /**< AVRCP 1.3 */
    BTHAL_AVRCP_FEAT_ABSOLUTE_VOLUME =
        0x02,                         /**< Supports TG role and volume sync */
    BTHAL_AVRCP_FEAT_BROWSE =
        0x04,                         /**< AVRCP 1.4 and up, with Browsing support */
} BTAvrcpRemoteFeatures_t;

/**
 * @brief AVRCP Supported features
 */
typedef enum
{
    BTHAL_AVRCP_APP_SETTING_EQUALIZER = 0x01, /**< Equalizer */
    BTHAL_AVRCP_APP_SETTING_REPEAT = 0x02,    /**< Repeat */
    BTHAL_AVRCP_APP_SETTING_SHUFFLE = 0x03,   /**< Shuffle */
    BTHAL_AVRCP_APP_SETTING_SCAN = 0x04,      /**< Scan */
} BTAvrcpAppSettingId_t;

/**
 * @brief Bluetooth AV connection states
 */
typedef enum
{
    BTHAL_AVRCP_CONN_STATE_DISCONNECTED = 0, /**< Disconnected */
    BTHAL_AVRCP_CONN_STATE_CONNECTED = 1,    /**< Connecting */
} BTAvrcpConnectionState_t;

#endif /* _BT_HAL_AVRCP_H */
/** @} */
