/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file. This file is a
 * Modifiable File, as defined in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#pragma once

#include <ace/ace_status.h>
#include <ace/hal_device_info.h>

/**
 * Sets the property entry to data if the property is currently modifiable.
 *
 * This api is not part of hal_device_info.h from ace since being able to modify properties
 * should only be used during prototype and testing stages.
 *
 * @param entry the entry to modify. Not all entries are modifiable.
 * @param data  the data to set.
 * @return ace_status_t on success, else error code. (note will always fail on locked down/ non-prototype devices)
 */
ace_status_t internal_deviceInfo_prototype_setEntry(acehal_device_info_entry_t entry, const char* data);

