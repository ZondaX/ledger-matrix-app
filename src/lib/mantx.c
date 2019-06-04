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

#include <stdio.h>
#include <time.h>

const char *displayFieldNames[] = {
    "Nonce",
    "Gas Price",
    "Gas Limit",
    "To",
    "Value",
    "Data",
    "ChainID",  // According to EIP155
//    "R",
//    "S",
    "EnterType",
    "IsEntrustTx",
    "CommitTime",
//    "UNUSED-ExtraTo",
    "TxType",
    "LockHeight",
    "Tx_to"
};

const uint8_t displayItemFieldIdxs[] = {
    MANTX_FIELD_NONCE,
    MANTX_FIELD_GASPRICE,
    MANTX_FIELD_GASLIMIT,
    MANTX_FIELD_TO,
    MANTX_FIELD_VALUE,
    MANTX_FIELD_DATA,
    MANTX_FIELD_V,
    //MANTX_FIELD_R,        // Do not show according to EIP155
    //MANTX_FIELD_S,
    MANTX_FIELD_ENTERTYPE,
    MANTX_FIELD_ISENTRUSTTX,
    MANTX_FIELD_COMMITTIME,
//    MANTX_FIELD_EXTRATO,
    MANTX_FIELD_EXTRATO_TXTYPE,
    MANTX_FIELD_EXTRATO_LOCKHEIGHT,
};

int8_t mantx_parse(mantx_context_t *ctx, uint8_t *data, uint16_t dataLen) {
    uint16_t fieldCount;

    // we expect a single root list
    int8_t err = rlp_parseStream(data, dataLen, &ctx->root, 1, &fieldCount);
    if (err != MANTX_NO_ERROR)
        return err;
    if (ctx->root.kind != RLP_KIND_LIST)
        return MANTX_ERROR_UNEXPECTED_ROOT;

    // now we can extract all rootFields in that list
    err = rlp_readList(data, &ctx->root, ctx->rootFields, MANTX_ROOTFIELD_COUNT, &fieldCount);
    if (err != MANTX_NO_ERROR)
        return err;
    if (fieldCount != MANTX_ROOTFIELD_COUNT)
        return MANTX_ERROR_UNEXPECTED_FIELD_COUNT;
    ctx->rootData = data + ctx->root.valueOffset;

    // Now parse the extraTo field
    const rlp_field_t *extraToField = &ctx->rootFields[MANTX_FIELD_EXTRATO];
    err = rlp_readList(ctx->rootData,
                       extraToField,
                       ctx->extraToFields,
                       MANTX_EXTRATOFIELD_COUNT, &fieldCount);
    if (err != MANTX_NO_ERROR)
        return err;
    if (fieldCount != MANTX_EXTRATOFIELD_COUNT)
        return MANTX_ERROR_UNEXPECTED_FIELD_COUNT;
    ctx->extraToData = ctx->rootData +
                       ctx->extraToFields[0].fieldOffset +
                       ctx->extraToFields[0].valueOffset;

    // Now parse the extraTo2 field
    const rlp_field_t *extraToField2 = &ctx->extraToFields[0];
    err = rlp_readList(ctx->extraToData,
                       extraToField2,
                       ctx->extraToFields2,
                       MANTX_EXTRATO2FIELD_COUNT, &fieldCount);
    if (err != MANTX_NO_ERROR)
        return err;
    if (fieldCount != MANTX_EXTRATO2FIELD_COUNT)
        return MANTX_ERROR_UNEXPECTED_FIELD_COUNT;
    ctx->extraToData2 = ctx->extraToData +
                        ctx->extraToFields2[0].fieldOffset +
                        ctx->extraToFields2[0].valueOffset;

    return MANTX_NO_ERROR;
}

const char *maxtx_getDisplayName(uint8_t displayIndex) {
    if (displayIndex > MANTX_ROOTFIELD_COUNT)
        return "? Unknown";
    return displayFieldNames[displayIndex];
}

int8_t mantx_print(mantx_context_t *ctx, uint8_t *data, uint8_t fieldIdx, char *out, uint16_t outLen) {
    MEMSET(out, 0, outLen);
    uint256_t tmp;

    switch (fieldIdx) {
        case MANTX_FIELD_NONCE: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            const uint8_t *d = ctx->rootData;
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_GASPRICE: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            const uint8_t *d = ctx->rootData;
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_GASLIMIT: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            const uint8_t *d = ctx->rootData;
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_TO: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            const uint8_t *d = ctx->rootData;
            rlp_readString(d, f, (char *) out, outLen);
            break;
        }
        case MANTX_FIELD_VALUE: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            const uint8_t *d = ctx->rootData;
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_DATA: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            const uint8_t *d = ctx->rootData;
            rlp_readString(d, f, (char *) out, outLen);
            break;
        }
        case MANTX_FIELD_V: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            const uint8_t *d = ctx->rootData;
            uint8_t tmpByte;
            rlp_readByte(d, f, &tmpByte);
            snprintf(out, outLen, "%d", tmpByte);
            break;
        }
        case MANTX_FIELD_R:
            // empty response
            break;
        case MANTX_FIELD_S:
            // empty response
            break;
        case MANTX_FIELD_ENTERTYPE: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            const uint8_t *d = ctx->rootData;
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_ISENTRUSTTX: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            const uint8_t *d = ctx->rootData;
            rlp_readUInt256(d, f, &tmp);
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_COMMITTIME: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            const uint8_t *d = ctx->rootData;
            rlp_readUInt256(d, f, &tmp);

            // this should be limited to uint64_t
            if (tmp.elements[0].elements[0] != 0 ||
                tmp.elements[0].elements[1] != 0 ||
                tmp.elements[1].elements[0] != 0) {
                return MANTX_ERROR_INVALID_TIME;
            }

            time_t t = tmp.elements[1].elements[1] / 1000;
            struct tm *ptm = gmtime(&t);

            strftime(out, outLen, "%d%b%Y %T", ptm);
            break;
        }
        case MANTX_FIELD_EXTRATO: {
            break;
        }
        case MANTX_FIELD_EXTRATO_TXTYPE: {
            // Is there an extra_to field?
            const rlp_field_t *f = ctx->extraToFields2 + 0;
            const uint8_t *d = ctx->extraToData2;
            uint8_t tmpByte;
            rlp_readByte(d, f, &tmpByte);
            snprintf(out, outLen, "%d", tmpByte);
            break;
        }
        case MANTX_FIELD_EXTRATO_LOCKHEIGHT: {
            snprintf(out, outLen, "???");
            // TODO: This will be a list
            break;
        }
        default:
            return MANTX_ERROR_UNEXPECTED_FIELD;
    }
    return MANTX_NO_ERROR;
}

int8_t mantx_getItem(mantx_context_t *ctx, uint8_t *data, uint8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outValue, uint16_t outValueLen) {

    if (displayIdx >= MANTX_DISPLAY_COUNT)
        return MANTX_ERROR_UNEXPECTED_DISPLAY_IDX;

    uint8_t fieldIdx = displayItemFieldIdxs[displayIdx];

    snprintf(outKey, outKeyLen, "%s", maxtx_getDisplayName(displayIdx));
    uint8_t err = mantx_print(ctx, data, fieldIdx, outValue, outValueLen);
    if (err != MANTX_NO_ERROR) {
        return err;
    }

    return MANTX_NO_ERROR;
}
