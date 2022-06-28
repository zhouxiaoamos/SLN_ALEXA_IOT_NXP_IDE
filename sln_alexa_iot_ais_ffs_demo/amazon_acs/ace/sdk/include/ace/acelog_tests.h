/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * AMAZON PROPRIETARY/CONFIDENTIAL
 *
 * You may not use this file except in compliance with the terms and
 * conditions set forth in the accompanying LICENSE.TXT file.This file
 * is a Modifiable File, as defined in the accompanying LICENSE.TXT file.
 *
 * THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */
#ifndef _LIBS_ACE_LOG_TESTS_H
#define _LIBS_ACE_LOG_TESTS_H

ace_status_t test_init(void);
ace_status_t test_logconst(void);
ace_status_t test_filter(void);
ace_status_t test_filter_setup_teardown(void);
ace_status_t test_binary_msg(void);
ace_status_t test_flushAllLogBuffers(void);
ace_status_t test_setLogBufferOutputDestState(void);


#endif
