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
/*******************************************************************************
 * Base 64 url module
 *******************************************************************************
 */

// This module is based on the mbed base64 code, which was provided under the
// following license:

/*
 *  RFC 1521 base64 encoding/decoding
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#include "base64_url.h"

static const unsigned char base64_url_enc_map[64] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '-', '_'
};

/*
 * Encode a buffer into base64 format
 */
acorn_base64_url_error_t acorn_base64_url_encode( unsigned char *dst, size_t dlen, size_t *olen,
                   const unsigned char *src, size_t slen )
{
    size_t i, n;
    int C1, C2, C3;
    unsigned char *p;

    if ( slen == 0 ) {
        *olen = 0;
        return( 0 );
    }

    n = ( slen << 3 ) / 6;
   
    if ((( slen << 3 ) - ( n * 6 )) != 0) {
        n += 1;
    }

    if ( dlen < n + 1 ) {
        *olen = n + 1;
        return( ACORN_BASE64_URL_BUFFER_TOO_SMALL );
    }

    n = ( slen / 3 ) * 3;

    for ( i = 0, p = dst; i < n; i += 3 ) {
        C1 = *src++;
        C2 = *src++;
        C3 = *src++;

        *p++ = base64_url_enc_map[(C1 >> 2) & 0x3F];
        *p++ = base64_url_enc_map[(((C1 &  3) << 4) + (C2 >> 4)) & 0x3F];
        *p++ = base64_url_enc_map[(((C2 & 15) << 2) + (C3 >> 6)) & 0x3F];
        *p++ = base64_url_enc_map[C3 & 0x3F];
    }

    if ( i < slen ) {
        C1 = *src++;
        C2 = (( i + 1 ) < slen ) ? *src++ : 0;

        *p++ = base64_url_enc_map[(C1 >> 2) & 0x3F];
        *p++ = base64_url_enc_map[(((C1 & 3) << 4) + (C2 >> 4)) & 0x3F];

        if (( i + 1 ) < slen )
             *p++ = base64_url_enc_map[((C2 & 15) << 2) & 0x3F];
    }

    *olen = p - dst;
    *p = 0;

    return ACORN_BASE64_URL_ENCODE_OK;
}

