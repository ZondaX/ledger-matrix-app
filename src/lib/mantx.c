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
    int8_t err = rlp_parseStream(data, 0, dataLen, &ctx->root, 1, &fieldCount);
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

    // Now parse the extraTo field
    const rlp_field_t *extraToField = &ctx->rootFields[MANTX_FIELD_EXTRATO];
    rlp_field_t extraToFieldInternal;
    err = rlp_readList(data,
                       extraToField,
                       &extraToFieldInternal,
                       1, &fieldCount);
    if (err != MANTX_NO_ERROR)
        return err;
    if (fieldCount != 1)
        return MANTX_ERROR_UNEXPECTED_FIELD_COUNT;
    if (extraToFieldInternal.kind != RLP_KIND_LIST)
        return MANTX_ERROR_UNEXPECTED_FIELD_TYPE;

    // Now parse the extraToInternal field
    err = rlp_readList(data,
                       &extraToFieldInternal,
                       ctx->extraToFields,
                       MANTX_EXTRATOFIELD_COUNT, &fieldCount);
    if (err != MANTX_NO_ERROR)
        return err;
    if (fieldCount != MANTX_EXTRATOFIELD_COUNT)
        return MANTX_ERROR_UNEXPECTED_FIELD_COUNT;

    uint8_t extraToToCount = 0;
    uint8_t JsonCount = 0;

    return MANTX_NO_ERROR;
}

const char *maxtx_getDisplayName(uint8_t displayIndex) {
    if (displayIndex > MANTX_ROOTFIELD_COUNT)
        return "? Unknown";
    return displayFieldNames[displayIndex];
}

void getDisplayTxExtraToType(char *out, uint16_t outLen, uint8_t txtype) {
    switch (txtype) {
        case 0:
            snprintf(out, outLen, "%d Normal", txtype);
            break;
        case 1:
            snprintf(out, outLen, "%d Broadcast", txtype);
            break;
        case 2:
            snprintf(out, outLen, "%d Miner reward", txtype);
            break;
        case 3:
            snprintf(out, outLen, "%d Revocable", txtype);
            break;
        case 4:
            snprintf(out, outLen, "%d Revert", txtype);
            break;
        case 5:
            snprintf(out, outLen, "%d Authorized", txtype);
            break;
        case 6:
            snprintf(out, outLen, "%d Cancel Auth", txtype);
            break;
        case 7:
            snprintf(out, outLen, "%d Normal", txtype);
            break;
        case 8:
            snprintf(out, outLen, "%d Normal", txtype);
            break;
        case 9:
            snprintf(out, outLen, "%d Create curr", txtype);
            break;
        case 10:
            snprintf(out, outLen, "%d Verif reward", txtype);
            break;
        case 11:
            snprintf(out, outLen, "%d Interest reward", txtype);
            break;
        case 12:
            snprintf(out, outLen, "%d Tx Fee reward", txtype);
            break;
        case 13:
            snprintf(out, outLen, "%d Lottery reward", txtype);
            break;
        case 14:
            snprintf(out, outLen, "%d Set blacklist", txtype);
            break;
        case 122:
            snprintf(out, outLen, "%d Super block", txtype);
            break;
        default:
            snprintf(out, outLen, "Tx type %d", txtype);
            break;
    }
};

int8_t mantx_print(mantx_context_t *ctx, uint8_t *data, uint8_t fieldIdx, char *out, uint16_t outLen) {
    MEMSET(out, 0, outLen);
    uint256_t tmp;
    uint8_t err;

    switch (fieldIdx) {
        case MANTX_FIELD_NONCE: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err != RLP_NO_ERROR) { return err; }
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_GASPRICE: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err != RLP_NO_ERROR) { return err; }
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_GASLIMIT: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err != RLP_NO_ERROR) { return err; }
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_TO: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readString(data, f, (char *) out, outLen);
            if (err != RLP_NO_ERROR) { return err; }
            break;
        }
        case MANTX_FIELD_VALUE: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err != RLP_NO_ERROR) { return err; }
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_DATA: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readString(data, f, (char *) out, outLen);
            if (err != RLP_NO_ERROR) { return err; }
            break;
        }
        case MANTX_FIELD_V: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            uint8_t tmpByte;
            err = rlp_readByte(data, f, &tmpByte);
            if (err != RLP_NO_ERROR) { return err; }
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
            err = rlp_readUInt256(data, f, &tmp);
            if (err != RLP_NO_ERROR) { return err; }
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_ISENTRUSTTX: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err != RLP_NO_ERROR) { return err; }
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_COMMITTIME: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err != RLP_NO_ERROR) { return err; }

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
            const rlp_field_t *f = ctx->extraToFields;
            err = rlp_readUInt256(data, f, &tmp);
            if (err != RLP_NO_ERROR) { return err; }
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_EXTRATO_LOCKHEIGHT: {
            const rlp_field_t *f = ctx->extraToFields + 1;
            err = rlp_readUInt256(data, f, &tmp);
            if (err != RLP_NO_ERROR) { return err; }
            tostring256(&tmp, 10, out, outLen);
            break;
        }
            // This is actually dynamic and there is another internal list
        case MANTX_FIELD_EXTRATO_TO: {
//            const rlp_field_t *f = ctx->extraToFields + 2;
//            rlp_readUInt256(data, f, &tmp);
//            tostring256(&tmp, 10, out, outLen);
            break;
        }
        default:
            return MANTX_ERROR_UNEXPECTED_FIELD;
    }
    return MANTX_NO_ERROR;
}

int8_t mantx_getItem(mantx_context_t *ctx, uint8_t *data,
                     uint8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outValue, uint16_t outValueLen) {

    if (displayIdx < MANTX_DISPLAY_COUNT) {
        uint8_t fieldIdx = displayItemFieldIdxs[displayIdx];
        snprintf(outKey, outKeyLen, "%s", maxtx_getDisplayName(displayIdx));
        uint8_t err = mantx_print(ctx, data, fieldIdx, outValue, outValueLen);
        if (err != MANTX_NO_ERROR) {
            return err;
        }
        return MANTX_NO_ERROR;
    }

    if (displayIdx < MANTX_DISPLAY_COUNT + ctx->extraToToCount) {
        return MANTX_NO_ERROR;
    }

    if (displayIdx < MANTX_DISPLAY_COUNT + ctx->extraToToCount + ctx->JsonCount) {
        return MANTX_NO_ERROR;
    }

    return MANTX_ERROR_UNEXPECTED_DISPLAY_IDX;
}
