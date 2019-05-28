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

#pragma once

#include <zxmacros.h>

#define RLP_KIND_BYTE       0
#define RLP_KIND_STRING     1
#define RLP_KIND_LIST       2

#define RLP_NO_ERROR 0
#define RLP_ERROR_INVALID_KIND  -1
#define RLP_ERROR_INVALID_VALUE_LEN  -2
#define RLP_ERROR_INVALID_FIELD_OFFSET  -3
#define RLP_ERROR_BUFFER_TOO_SMALL  -4

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t kind;
    uint16_t fieldOffset;
    uint16_t valueOffset;
    uint16_t valueLen;
} rlp_field_t;

int16_t rlp_decode(const uint8_t *data, uint8_t *kind, uint16_t *len, uint16_t *dataOffset);

// parses and splits the buffer into fields
int8_t rlp_parseStream(const uint8_t *data,
                       uint64_t dataLen,
                       rlp_field_t *fields,
                       uint8_t maxFieldCount,
                       uint16_t *fieldCount);

// reads a byte from the field
int8_t rlp_readByte(const uint8_t *data,
                    rlp_field_t *field,
                    uint8_t *value);

// reads a string into value, it will zero terminate so maxLen is expected to be larger than the actual data
int8_t rlp_readString(const uint8_t *data,
                      rlp_field_t *field,
                      uint8_t *value,
                      uint16_t maxLen);

// reads a list and splits into fields
int8_t rlp_readList(const uint8_t *data,
                    rlp_field_t *field,
                    rlp_field_t *listFields,
                    uint8_t maxListFieldCount,
                    uint16_t *listFieldCount);

#ifdef __cplusplus
}
#endif
