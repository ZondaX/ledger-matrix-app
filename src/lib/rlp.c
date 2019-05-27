/*******************************************************************************
*   (c) 2019 ZondaX GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "rlp.h"

uint64_t rlp_decode(const uint8_t *data,
                    uint8_t *kind,
                    uint64_t *len,
                    uint64_t *data_offset) {

    uint8_t p = *data;
    if (p >= 0 && p <= 0x7F) {
        *kind = RLP_KIND_BYTE;
        *len = 0;
        *data_offset = 0;
        return 1; // 1 byte to consume from the stream
    }

    if (p >= 0x80 && p <= 0xb7) {
        *kind = RLP_KIND_STRING;
        *len = p - 0x80;
        *data_offset = 1;
        return 1 + *len;
    }

    if (p >= 0xb8 && p <= 0xbf) {
        *kind = RLP_KIND_STRING;
        uint8_t len_len = p - 0xb7;
        *len = 0;
        for (uint8_t i = 0; i < len_len; i++) {
            *len <<= 8u;
            *len += *(data + 1 + i);
        }
        *data_offset = 1 + len_len;
        return 1 + len_len + *len;
    }

    if (p >= 0xc0 && p <= 0xf7) {
        *kind = RLP_KIND_LIST;
        *len = p - 0xc0;
        *data_offset = 1;
        return 1 + *len;
    }

    if (p >= 0xf8 && p <= 0xff) {
        *kind = RLP_KIND_LIST;
        uint8_t len_len = p - 0xf7;
        *len = p - 0xf7;
        *len = 0;
        for (uint8_t i = 0; i < len_len; i++) {
            *len <<= 8u;
            *len += *(data + 1 + i);
        }
        *data_offset = 1 + len_len;
        return 1 + len_len + *len;
    }

    return 0;
}
