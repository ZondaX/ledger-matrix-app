/*******************************************************************************
*  (c) 2019 ZondaX GmbH
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

#include <rlp.h>
#include <coin.h>
#include <zxtypes.h>
#include "parser_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define MANTX_ROOTFIELD_COUNT 13
#define MANTX_EXTRAFIELD_COUNT 3
#define MANTX_EXTRALISTFIELD_COUNT 10

/////////////// TX TYPES
#define MANTX_TXTYPE_NORMAL             0
#define MANTX_TXTYPE_BROADCAST          1
#define MANTX_TXTYPE_MINER_REWARD       2
#define MANTX_TXTYPE_SCHEDULED          3
#define MANTX_TXTYPE_REVERT             4
#define MANTX_TXTYPE_AUTHORIZED         5
#define MANTX_TXTYPE_CANCEL_AUTH        6
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
#define MANTX_FIELD_EXTRA        12     // This field is a list so it is not shown

// These field may or may not be available
#define MANTX_FIELD_EXTRA_TXTYPE  13
#define MANTX_FIELD_EXTRA_LOCKHEIGHT  14
#define MANTX_FIELD_EXTRA_TO          15

#define MANTX_DISPLAY_COUNT 12

typedef struct {
    rlp_field_t root;
    rlp_field_t rootFields[MANTX_ROOTFIELD_COUNT];
    rlp_field_t extraFields[MANTX_EXTRAFIELD_COUNT];
    rlp_field_t extraToListFields[MANTX_EXTRALISTFIELD_COUNT];
    uint8_t extraTxType;
    uint16_t extraToListCount;
    uint8_t JsonCount;
} parser_tx_t;

#ifdef __cplusplus
}
#endif
