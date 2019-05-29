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

#include "mantx.h"
#include "rlp.h"
#include "uint256.h"

int8_t mantx_parse(mantx_context_t *ctx, uint8_t *data, uint16_t dataLen) {
    uint16_t fieldCount;

    int8_t err = rlp_parseStream(data, dataLen, &ctx->root, 1, &fieldCount);
    if (err != MANTX_NO_ERROR)
        return err;

    // we expect a single list
    if (ctx->root.kind != RLP_KIND_LIST)
        return MANTX_ERROR_UNEXPECTED_ROOT;

    // now we can extract all fields in that list
    err = rlp_readList(data, &ctx->root, ctx->fields, 13, &fieldCount);
    if (err != MANTX_NO_ERROR)
        return err;

    ctx->rootData = data + ctx->root.fieldOffset + ctx->root.valueOffset;

    if (fieldCount != 13)
        return MANTX_ERROR_UNEXPECTED_FIELD_COUNT;

    return MANTX_NO_ERROR;
}

int8_t mantx_print(mantx_context_t *ctx, uint8_t *data, uint8_t fieldIdx, char *out, uint16_t outLen) {
//01 {name: 'nonce', length: 32, allowLess: true, index: 0, default: <Buffer>},
//02 {name: 'gasPrice',length: 32,allowLess: true,index: 1,default: <Buffer>},
//03 {name: 'gasLimit',alias: 'gas',length: 32,allowLess: true,index: 2,default: <Buffer>},
//04 {name: 'to', allowZero: true, index: 3, default: '' },
//05 {name: 'value',length: 32,allowLess: true,index: 4,default: <Buffer>},
//06 {name: 'data',alias: 'input',allowZero: true,index: 5,default: <Buffer>},

//07 {name: 'v', allowZero: true, index: 6, default: <Buffer 1c> },
//08 {name: 'r',length: 32,allowZero: true,allowLess: true,index: 7,default: <Buffer>},
//09 {name: 's',length: 32,allowZero: true,allowLess: true,index: 8,default: <Buffer>},

//10 {name: 'TxEnterType',allowZero: true,allowLess: true,index: 9,default: <Buffer>},
//11 {name: 'IsEntrustTx',allowZero: true,allowLess: true,index: 10,default: <Buffer>},
//12 {name: 'CommitTime',allowZero: true,allowLess: true,index: 11,default: <Buffer>},
//13 {name: 'extra_to',allowZero: true,allowLess: true,index: 12,default: <Buffer>}

    const rlp_field_t *f = ctx->fields + fieldIdx;
    const uint8_t *d = ctx->rootData;

    uint256_t tmp;

    switch (fieldIdx) {
        case MANTX_FIELD_NONCE: {
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_GASPRICE: {
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_GASLIMIT: {
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_TO: {
            break;
        }
        case MANTX_FIELD_VALUE: {
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_DATA: {
            break;
        }
        case MANTX_FIELD_V: {
            // TODO fix chain id
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_R:
            // empty response
            out[0] = 0;
            break;
        case MANTX_FIELD_S:
            // empty response
            out[0] = 0;
            break;

        case MANTX_FIELD_ENTERTYPE:
            break;
        case MANTX_FIELD_ISENTRUSTTX:
            break;
        case MANTX_FIELD_COMMITTIME: {
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_EXTRATO:
            // TODO: This will be a list
            break;
        default:
            return MANTX_ERROR_UNEXPECTED_FIELD;
    }
    return MANTX_NO_ERROR;
}
