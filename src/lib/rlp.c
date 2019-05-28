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

int16_t rlp_decode(const uint8_t *data,
                   uint8_t *kind,
                   uint16_t *len,
                   uint16_t *dataOffset) {

    // TODO: Do not allow uint64 lengths

    uint8_t p = *data;
    if (p >= 0 && p <= 0x7F) {
        *kind = RLP_KIND_BYTE;
        *len = 0;
        *dataOffset = 0;
        return 1; // 1 byte to consume from the stream
    }

    if (p >= 0x80 && p <= 0xb7) {
        *kind = RLP_KIND_STRING;
        *len = p - 0x80;
        *dataOffset = 1;
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
        *dataOffset = 1 + len_len;
        return 1 + len_len + *len;
    }

    if (p >= 0xc0 && p <= 0xf7) {
        *kind = RLP_KIND_LIST;
        *len = p - 0xc0;
        *dataOffset = 1;
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
        *dataOffset = 1 + len_len;
        return 1 + len_len + *len;
    }

    return RLP_NO_ERROR;
}

int8_t rlp_parseStream(const uint8_t *data,
                       uint64_t dataLen,
                       rlp_field_t *fields,
                       uint8_t maxFieldCount,
                       uint16_t *fieldCount) {
    uint64_t offset = 0;
    *fieldCount = 0;

    while (offset < dataLen && *fieldCount < maxFieldCount) {
        int16_t bytesConsumed = rlp_decode(
            data + offset,
            &fields[*fieldCount].kind,
            &fields[*fieldCount].valueLen,
            &fields[*fieldCount].valueOffset);

        if (bytesConsumed < 0) {
            return bytesConsumed;   // as error
        }

        offset += bytesConsumed;
        (*fieldCount)++;
    }

    return RLP_NO_ERROR;
}

int8_t rlp_readByte(const uint8_t *data, rlp_field_t *field, uint8_t *value) {
    if (field->kind != RLP_KIND_BYTE)
        return RLP_ERROR_INVALID_KIND;

    if (field->valueLen != 0)
        return RLP_ERROR_INVALID_VALUE_LEN;

    if (field->fieldOffset != 0)
        return RLP_ERROR_INVALID_FIELD_OFFSET;

    *value = *(data + field->fieldOffset + field->valueOffset);

    return RLP_NO_ERROR;
}

int8_t rlp_readString(const uint8_t *data, rlp_field_t *field, uint8_t *value, uint16_t maxLen) {
    if (field->kind != RLP_KIND_STRING)
        return RLP_ERROR_INVALID_KIND;

    if (field->valueLen >= maxLen)
        return RLP_ERROR_BUFFER_TOO_SMALL;

    MEMCPY(value, data + field->fieldOffset + field->valueOffset, field->valueLen);
    value[field->valueLen] = 0; // Zero terminate
    return RLP_NO_ERROR;
}

int8_t rlp_readList(const uint8_t *data,
                    rlp_field_t *field,
                    rlp_field_t *listFields,
                    uint8_t maxListFieldCount,
                    uint16_t *listFieldCount) {
    if (field->kind != RLP_KIND_LIST)
        return RLP_ERROR_INVALID_KIND;

    return rlp_parseStream(data + field->fieldOffset + field->valueOffset,
                           field->valueLen,
                           listFields,
                           maxListFieldCount,
                           listFieldCount);
}
