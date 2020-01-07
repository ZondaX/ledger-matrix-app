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
#include "utils/uint256.h"

#define RLP_KIND_BYTE       0
#define RLP_KIND_STRING     1
#define RLP_KIND_LIST       2

#define RLP_NO_ERROR 0
#define RLP_ERROR_INVALID_KIND  -1
#define RLP_ERROR_INVALID_VALUE_LEN  -2
#define RLP_ERROR_INVALID_FIELD_OFFSET  -3
#define RLP_ERROR_BUFFER_TOO_SMALL  -4
#define RLP_ERROR_INVALID_PAGE  -5

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t kind;
    uint16_t fieldOffset;
    uint16_t valueOffset;
    uint16_t valueLen;
} rlp_field_t;

int16_t rlp_decode(const uint8_t *data,
                   uint8_t *kind,
                   uint16_t *len,
                   uint16_t *valueOffset);

// parses and splits the buffer into rootFields
int8_t rlp_parseStream(const uint8_t *data,
                       uint16_t dataOffset,
                       uint64_t dataLen,
                       rlp_field_t *fields,
                       uint8_t maxFieldCount,
                       uint16_t *fieldCount);

// reads a byte from the field
int8_t rlp_readByte(const uint8_t *data,
                    const rlp_field_t *field,
                    uint8_t *value);

// reads a buffer into value. These are not actually zero terminate strings but buffers
int8_t rlp_readStringPaging(const uint8_t *data,
                            const rlp_field_t *field,
                            char *value,
                            uint16_t maxLen,
                            uint16_t *valueLen,
                            uint8_t pageIdx,
                            uint8_t *pageCount);

// reads a buffer into value. These are not actually zero terminate strings but buffers
int8_t rlp_readString(const uint8_t *data,
                      const rlp_field_t *field,
                      char *value,
                      uint16_t maxLen);

// reads a list and splits into rootFields
int8_t rlp_readList(const uint8_t *data,
                    const rlp_field_t *field,
                    rlp_field_t *listFields,
                    uint8_t maxListFieldCount,
                    uint16_t *listFieldCount);

// reads a variable uint256
int8_t rlp_readUInt256(const uint8_t *data,
                       const rlp_field_t *field,
                       uint256_t *value);

#ifdef __cplusplus
}
#endif
