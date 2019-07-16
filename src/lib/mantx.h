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

#include "rlp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MANTX_ROOTFIELD_COUNT 13
#define MANTX_EXTRATOFIELD_COUNT 3
#define MANTX_EXTRATOLISTFIELD_COUNT 10

/////////////// ERRORS
#define MANTX_NO_ERROR 0
#define MANTX_ERROR_UNEXPECTED_ROOT -1
#define MANTX_ERROR_UNEXPECTED_FIELD_COUNT -2
#define MANTX_ERROR_UNEXPECTED_FIELD -3
#define MANTX_ERROR_UNEXPECTED_FIELD_TYPE -4
#define MANTX_ERROR_UNEXPECTED_DISPLAY_IDX -5
#define MANTX_ERROR_INVALID_TIME -6
#define MANTX_ERROR_INVALID_TXTYPE -7

/////////////// TX TYPES
#define MANTX_TXTYPE_NORMAL             0
#define MANTX_TXTYPE_BROADCAST          1
#define MANTX_TXTYPE_MINER_REWARD       2
#define MANTX_TXTYPE_REVOCABLE          3
#define MANTX_TXTYPE_REVERT             4
#define MANTX_TXTYPE_AUTHORIZED         5
#define MANTX_TXTYPE_CANCEL_AUTH        6
#define MANTX_TXTYPE_FIXME1             7  // FIXME: ????????
#define MANTX_TXTYPE_FIXME2             8  // FIXME: ????????
#define MANTX_TXTYPE_CREATE_CURR        9
#define MANTX_TXTYPE_VERIF_REWARD       10
#define MANTX_TXTYPE_INTEREST_REWARD    11
#define MANTX_TXTYPE_TXFEE_REWARD       12
#define MANTX_TXTYPE_LOTTERY_REWARD     13
#define MANTX_TXTYPE_SET_BLACKLIST      14
#define MANTX_TXTYPE_SUPERBLOCK         122

////////////// FIELDS
#define MANTX_FIELD_NONCE        0
#define MANTX_FIELD_GASPRICE     1
#define MANTX_FIELD_GASLIMIT     2
#define MANTX_FIELD_TO           3
#define MANTX_FIELD_VALUE        4
#define MANTX_FIELD_DATA         5
///
#define MANTX_FIELD_V            6
#define MANTX_FIELD_R            7
#define MANTX_FIELD_S            8
///
#define MANTX_FIELD_ENTERTYPE    9
#define MANTX_FIELD_ISENTRUSTTX  10
#define MANTX_FIELD_COMMITTIME   11
#define MANTX_FIELD_EXTRATO      12     // This field is a list so it is not shown

// These field may or may not be available
#define MANTX_FIELD_EXTRATO_TXTYPE  13
#define MANTX_FIELD_EXTRATO_LOCKHEIGHT  14
#define MANTX_FIELD_EXTRATO_TO          15

#define MANTX_DISPLAY_COUNT 12

typedef struct {
    rlp_field_t root;
    rlp_field_t rootFields[MANTX_ROOTFIELD_COUNT];
    rlp_field_t extraToFields[MANTX_EXTRATOFIELD_COUNT];
    rlp_field_t extraToListFields[MANTX_EXTRATOLISTFIELD_COUNT];
    uint8_t extraToTxType;
    uint16_t extraToListCount;
    uint8_t JsonCount;
} mantx_context_t;

// parse and check a tx buffer
int8_t mantx_parse(mantx_context_t *ctx, uint8_t *data, uint16_t dataLen);

const char *maxtx_getDisplayName(uint8_t displayIndex);

// get a readable output for each field
int8_t mantx_print(mantx_context_t *ctx, uint8_t *data,
                              int8_t fieldIdx,
                              char *out, uint16_t outLen,
                              uint8_t pageIdx, uint8_t *pageCount);

uint8_t maxtx_getNumItems(mantx_context_t *ctx);

int8_t mantx_getItem(mantx_context_t *ctx, uint8_t *data,
                     int8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outValue, uint16_t outValueLen,
                     uint8_t pageIdx, uint8_t *pageCount);

#ifdef __cplusplus
}
#endif
