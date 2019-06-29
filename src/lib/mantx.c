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
#include "utils.h"

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

    // Extract extrato txType and cache it as metadata
    const rlp_field_t *f = ctx->extraToFields;
    uint256_t tmp;
    err = rlp_readUInt256(data, f, &tmp);
    if (err != RLP_NO_ERROR) { return err; }
    ctx->extraToTxType = tmp.elements[1].elements[1];   // extract last byte

    // TODO: Parse JSON and estimate number of pages for variable fields
    ctx->extraToToCount = 0;
    ctx->JsonCount = 0;

    return MANTX_NO_ERROR;
}

const char *maxtx_getDisplayName(uint8_t displayIndex) {
    if (displayIndex > MANTX_ROOTFIELD_COUNT)
        return "? Unknown";
    return displayFieldNames[displayIndex];
}

const char *getError(int8_t errorCode) {
    switch (errorCode) {
        case MANTX_ERROR_UNEXPECTED_ROOT:
            return "Unexpected root";
        case MANTX_ERROR_UNEXPECTED_FIELD_COUNT:
            return "Unexpected field count";
        case MANTX_ERROR_UNEXPECTED_FIELD:
            return "Unexpected field";
        case MANTX_ERROR_UNEXPECTED_FIELD_TYPE:
            return "Unexpected field type";
        case MANTX_ERROR_UNEXPECTED_DISPLAY_IDX:
            return "Unexpected display idx";
        case MANTX_ERROR_INVALID_TIME:
            return "Invalid TxType";
        case MANTX_ERROR_INVALID_TXTYPE:
            return "";
        case MANTX_NO_ERROR:
            return "No error";
        default:
            return "Unrecognized error code";
    }
}

uint8_t getDisplayTxExtraToType(char *out, uint16_t outLen, uint8_t txtype) {
    switch (txtype) {
        case MANTX_TXTYPE_NORMAL:
            snprintf(out, outLen, "Normal");
            break;
        case MANTX_TXTYPE_BROADCAST:
            snprintf(out, outLen, "Broadcast");
            break;
        case MANTX_TXTYPE_MINER_REWARD:
            snprintf(out, outLen, "Miner reward");
            break;
        case MANTX_TXTYPE_REVOCABLE:
            snprintf(out, outLen, "Revocable");
            break;
        case MANTX_TXTYPE_REVERT:
            snprintf(out, outLen, "Revert");
            break;
        case MANTX_TXTYPE_AUTHORIZED:
            snprintf(out, outLen, "Authorize");
            break;
        case MANTX_TXTYPE_CANCEL_AUTH:
            snprintf(out, outLen, "Cancel Auth");
            break;
//        case MANTX_TXTYPE_FIXME1:
//            // FIXME: ????????
//            snprintf(out, outLen, "%d Normal");
//            break;
//        case MANTX_TXTYPE_FIXME2:
//            // FIXME: ????????
//            snprintf(out, outLen, "%d Normal");
//            break;
        case MANTX_TXTYPE_CREATE_CURR:
            snprintf(out, outLen, "Create curr");
            break;
        case MANTX_TXTYPE_VERIF_REWARD:
            snprintf(out, outLen, "Verif reward");
            break;
        case MANTX_TXTYPE_INTEREST_REWARD:
            snprintf(out, outLen, "Interest reward");
            break;
        case MANTX_TXTYPE_TXFEE_REWARD:
            snprintf(out, outLen, "Tx Fee reward");
            break;
        case MANTX_TXTYPE_LOTTERY_REWARD:
            snprintf(out, outLen, "Lottery reward");
            break;
        case MANTX_TXTYPE_SET_BLACKLIST:
            snprintf(out, outLen, "Set blacklist");
            break;
        case MANTX_TXTYPE_SUPERBLOCK:
            snprintf(out, outLen, "Super block");
            break;
        default:
            return MANTX_ERROR_INVALID_TXTYPE;
            break;
    }

    return RLP_NO_ERROR;
};

int8_t mantx_print(mantx_context_t *ctx,
                   uint8_t *data,
                   uint8_t fieldIdx,
                   char *out, uint16_t outLen,
                   uint8_t pageIdx, uint8_t *pageCount) {
    MEMSET(out, 0, outLen);
    uint256_t tmp;
    uint8_t err;

    *pageCount = 1;

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
            uint16_t valueLen;

            // FIXME: Use string or hexstring depending on TXTYPE -----------------------
            switch (ctx->extraToTxType) {
                case MANTX_TXTYPE_NORMAL:
                    // ---------------- NO DATA FIELD
                    *pageCount = 0;
                    err = RLP_NO_ERROR;
                    break;
                case MANTX_TXTYPE_REVOCABLE:
                case MANTX_TXTYPE_AUTHORIZED:
                    // ---------------- JSON Payload
                    err = rlp_readStringPaging(data, f,
                                               (char *) out, (outLen - 1) / 2, &valueLen,
                                               pageIdx, pageCount);
                    break;
                case MANTX_TXTYPE_BROADCAST:
                case MANTX_TXTYPE_REVERT: {
                    // ----------------- HEX payload
                    err = rlp_readStringPaging(
                        data, f,
                        (char *) out,
                        (outLen - 1) / 2,  // 2bytes per byte + zero termination
                        &valueLen,
                        pageIdx, pageCount);
                    // now we need to convert to hexstring in place
                    convertToHexstringInPlace((uint8_t *) out, valueLen, outLen);
                    break;
                }
                default:
                    err = MANTX_ERROR_INVALID_TXTYPE;
                    break;
            }

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
            *pageCount = 0;
            break;
        case MANTX_FIELD_S:
            // empty response
            *pageCount = 0;
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
            // empty response
            *pageCount = 0;
            break;
        }
        case MANTX_FIELD_EXTRATO_TXTYPE: {
            *pageCount = 1;
            err = getDisplayTxExtraToType(out, outLen, ctx->extraToTxType);
            if (err != RLP_NO_ERROR) { return err; }
            break;
        }
        case MANTX_FIELD_EXTRATO_LOCKHEIGHT: {
            const rlp_field_t *f = ctx->extraToFields + 1;
            err = rlp_readUInt256(data, f, &tmp);
            if (err != RLP_NO_ERROR) { return err; }
            tostring256(&tmp, 10, out, outLen);
            break;
        }
        case MANTX_FIELD_EXTRATO_TO: {
            // empty response
            *pageCount = 0;
            // This is actually dynamic and there is another internal list
            // FIXME: ???????
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
                     char *outValue, uint16_t outValueLen,
                     uint8_t pageIdx, uint8_t *pageCount) {

    if (displayIdx < MANTX_DISPLAY_COUNT) {
        uint8_t fieldIdx = displayItemFieldIdxs[displayIdx];
        snprintf(outKey, outKeyLen, "%s", maxtx_getDisplayName(displayIdx));
        uint8_t err = mantx_print(ctx, data, fieldIdx, outValue, outValueLen, pageIdx, pageCount);
        if (err != MANTX_NO_ERROR) {
            return err;
        }
        return MANTX_NO_ERROR;
    }

    if (displayIdx < MANTX_DISPLAY_COUNT + ctx->extraToToCount) {
        // TODO: return extraToData
        return MANTX_NO_ERROR;
    }

    if (displayIdx < MANTX_DISPLAY_COUNT + ctx->extraToToCount + ctx->JsonCount) {
        // TODO: return JSON data fields
        return MANTX_NO_ERROR;
    }

    return MANTX_ERROR_UNEXPECTED_DISPLAY_IDX;
}
