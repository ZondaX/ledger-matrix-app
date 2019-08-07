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
//    MANTX_FIELD_EXTRA,
    MANTX_FIELD_EXTRA_TXTYPE,
    MANTX_FIELD_EXTRA_LOCKHEIGHT,
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

    ////////// EXTRA
    // Now parse the extra field
    const rlp_field_t *extraField = &ctx->rootFields[MANTX_FIELD_EXTRA];
    rlp_field_t extraFieldInternal;
    err = rlp_readList(data,
                       extraField,
                       &extraFieldInternal,
                       1, &fieldCount);
    if (err != MANTX_NO_ERROR)
        return err;
    if (fieldCount != 1)
        return MANTX_ERROR_UNEXPECTED_FIELD_COUNT;
    if (extraFieldInternal.kind != RLP_KIND_LIST)
        return MANTX_ERROR_UNEXPECTED_FIELD_TYPE;

    // Now parse the extraInternal field
    err = rlp_readList(data,
                       &extraFieldInternal,
                       ctx->extraFields,
                       MANTX_EXTRAFIELD_COUNT, &fieldCount);

    if (err != MANTX_NO_ERROR) {
        return err;
    }
    if (fieldCount != MANTX_EXTRAFIELD_COUNT)
        return MANTX_ERROR_UNEXPECTED_FIELD_COUNT;

    // Extract extra txType and cache it as metadata
    const rlp_field_t *f = ctx->extraFields;
    uint256_t tmp;
    err = rlp_readUInt256(data, f, &tmp);
    if (err != RLP_NO_ERROR) { return err; }
    ctx->extraTxType = tmp.elements[1].elements[1];   // extract last byte

    //////////
    ////////// EXTRA TO
    //////////
    f = ctx->extraFields + 2;
    err = rlp_readList(data, f,
                       ctx->extraToListFields,
                       MANTX_EXTRALISTFIELD_COUNT,
                       &ctx->extraToListCount);
    if (err != MANTX_NO_ERROR)
        return err;
    // Get each extraTo item is a stream of 3 elements (recipient, amount, payload)
    // To avoid using too much memory, we can parse them on demand

    ctx->JsonCount = 0;

    return MANTX_NO_ERROR;
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

uint8_t getDisplayTxExtraType(char *out, uint16_t outLen, uint8_t txtype) {
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
    }

    return RLP_NO_ERROR;
};

int8_t mantx_print(mantx_context_t *ctx,
                   uint8_t *data,
                   int8_t fieldIdx,
                   char *out, uint16_t outLen,
                   uint8_t pageIdx, uint8_t *pageCount) {
    MEMSET(out, 0, outLen);
    uint256_t tmp;
    uint8_t err = RLP_NO_ERROR;

    *pageCount = 1;

    switch (fieldIdx) {
        case MANTX_FIELD_NONCE: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                if (!tostring256(&tmp, 10, out, outLen)) {
                    err = MANTX_ERROR_UNEXPECTED_FIELD;
                }
            }
            break;
        }
        case MANTX_FIELD_GASPRICE: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        case MANTX_FIELD_GASLIMIT: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        case MANTX_FIELD_TO: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            uint16_t valueLen;
            err = rlp_readStringPaging(
                data, f,
                (char *) out, outLen, &valueLen,
                pageIdx, pageCount);
            break;
        }
        case MANTX_FIELD_VALUE: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        case MANTX_FIELD_DATA: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            uint16_t valueLen;

            switch (ctx->extraTxType) {
                case MANTX_TXTYPE_NORMAL:
                    // ---------------- NO DATA FIELD
                    *pageCount = 0;
                    err = RLP_NO_ERROR;
                    break;
                case MANTX_TXTYPE_REVOCABLE:
                case MANTX_TXTYPE_AUTHORIZED:
                    // ---------------- JSON Payload
                    err = rlp_readStringPaging(data, f,
                                               (char *) out, outLen,
                                               &valueLen,
                                               pageIdx, pageCount);
                    break;
                case MANTX_TXTYPE_BROADCAST:
                case MANTX_TXTYPE_REVERT: {
                    // ----------------- HEX payload
                    err = rlp_readStringPaging(data, f,
                                               (char *) out,
                                               (outLen - 1) / 2,  // 2bytes per byte + zero termination
                                               &valueLen,
                                               pageIdx, pageCount);
                    //snprintf(out, outLen, "%d - err %d", valueLen, err);
                    if (err == RLP_NO_ERROR) {
                        // now we need to convert to hexstring in place
                        convertToHexstringInPlace((uint8_t *) out, valueLen, outLen);
                    }
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
            if (err == RLP_NO_ERROR) {
                snprintf(out, outLen, "%d", tmpByte);
            }
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
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        case MANTX_FIELD_ISENTRUSTTX: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        case MANTX_FIELD_COMMITTIME: {
            const rlp_field_t *f = ctx->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                // this should be limited to uint64_t
                if (tmp.elements[0].elements[0] != 0 ||
                    tmp.elements[0].elements[1] != 0 ||
                    tmp.elements[1].elements[0] != 0) {
                    err = MANTX_ERROR_INVALID_TIME;
                } else {
                    uint64_t t = tmp.elements[1].elements[1];
                    printTime(out, outLen, t);
                }
            }
            break;
        }
        case MANTX_FIELD_EXTRA: {
            // empty response
            *pageCount = 0;
            break;
        }
        case MANTX_FIELD_EXTRA_TXTYPE: {
            *pageCount = 1;
            err = getDisplayTxExtraType(out, outLen, ctx->extraTxType);
            break;
        }
        case MANTX_FIELD_EXTRA_LOCKHEIGHT: {
            const rlp_field_t *f = ctx->extraFields + 1;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        default:
            return MANTX_ERROR_UNEXPECTED_FIELD;
    }

    if (err != MANTX_NO_ERROR) {
        snprintf(out, outLen, "err %d", err);
    }

    return err;
}

uint8_t maxtx_getNumItems(mantx_context_t *ctx) {
    return MANTX_DISPLAY_COUNT + ctx->extraToListCount + ctx->JsonCount;
}

int8_t mantx_getItem(mantx_context_t *ctx, uint8_t *data,
                     int8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outValue, uint16_t outValueLen,
                     uint8_t pageIdx, uint8_t *pageCount) {
    if (displayIdx < 0) {
        return MANTX_ERROR_UNEXPECTED_DISPLAY_IDX;
    }

    if (displayIdx < MANTX_DISPLAY_COUNT) {
        snprintf(outValue, outValueLen, " ");

        const uint8_t fieldIdx = PIC(displayItemFieldIdxs[displayIdx]);

        switch (fieldIdx) {
            case MANTX_FIELD_NONCE:
                snprintf(outKey, outKeyLen, "Nonce");
                break;
            case MANTX_FIELD_GASPRICE:
                snprintf(outKey, outKeyLen, "Gas Price");
                break;
            case MANTX_FIELD_GASLIMIT:
                snprintf(outKey, outKeyLen, "Gas Limit");
                break;
            case MANTX_FIELD_TO:
                snprintf(outKey, outKeyLen, "To");
                break;
            case MANTX_FIELD_VALUE:
                snprintf(outKey, outKeyLen, "Value");
                break;
            case MANTX_FIELD_DATA:
                snprintf(outKey, outKeyLen, "Data");
                break;
            case MANTX_FIELD_V:
                snprintf(outKey, outKeyLen, "ChainID");
                break;
            case MANTX_FIELD_ENTERTYPE:
                snprintf(outKey, outKeyLen, "EnterType");
                break;
            case MANTX_FIELD_ISENTRUSTTX:
                snprintf(outKey, outKeyLen, "IsEntrustTx");
                break;
            case MANTX_FIELD_COMMITTIME:
                snprintf(outKey, outKeyLen, "CommitTime");
                break;
            case MANTX_FIELD_EXTRA_TXTYPE:
                snprintf(outKey, outKeyLen, "TxType");
                break;
            case MANTX_FIELD_EXTRA_LOCKHEIGHT:
                snprintf(outKey, outKeyLen, "Lock Height");
                break;
            default:
                snprintf(outKey, outKeyLen, "?");
        }

        int8_t err = mantx_print(
            ctx, data, fieldIdx,
            outValue, outValueLen,
            pageIdx, pageCount);

        return err;
    }

    if (displayIdx < MANTX_DISPLAY_COUNT + ctx->extraToListCount * 3) {
        uint8_t extraToIdx = (displayIdx - MANTX_DISPLAY_COUNT) / 3;
        uint8_t fieldIdx = (displayIdx - MANTX_DISPLAY_COUNT) % 3;

        // Read the stream of three items
        const rlp_field_t *f = &ctx->extraToListFields[extraToIdx];
        rlp_field_t extraToFields[3];
        uint16_t fieldCount;
        int8_t err = rlp_readList(data, f, extraToFields, MANTX_ROOTFIELD_COUNT, &fieldCount);
        if (err != MANTX_NO_ERROR)
            return err;
        if (fieldCount != 3)
            return MANTX_ERROR_UNEXPECTED_FIELD_COUNT;

        uint16_t valueLen;
        switch (fieldIdx) {
            case 0: {
                snprintf(outKey, outKeyLen, "[%d] To", extraToIdx);
                err = rlp_readStringPaging(
                    data, extraToFields + 0,
                    (char *) outValue, outValueLen, &valueLen,
                    pageIdx, pageCount);
                break;
            }
            case 1: {
                uint256_t tmp;
                snprintf(outKey, outKeyLen, "[%d] Amount", extraToIdx);
                rlp_readUInt256(data, extraToFields + 1, &tmp);
                tostring256(&tmp, 10, outValue, outValueLen);
                break;
            }
            case 2: {
                snprintf(outKey, outKeyLen, "[%d] Payload", extraToIdx);
                // ----------------- HEX payload
                err = rlp_readStringPaging(data, extraToFields + 2,
                                           (char *) outValue,
                                           (outValueLen - 1) / 2,  // 2bytes per byte + zero termination
                                           &valueLen,
                                           pageIdx, pageCount);
                //snprintf(out, outLen, "%d - err %d", valueLen, err);
                if (err == RLP_NO_ERROR) {
                    // now we need to convert to hexstring in place
                    convertToHexstringInPlace((uint8_t *) outValue, valueLen, outValueLen);
                }
                break;
            }
        }

        return err;
    }

    if (displayIdx < MANTX_DISPLAY_COUNT + ctx->extraToListCount + ctx->JsonCount) {
        return MANTX_NO_ERROR;
    }

    return MANTX_ERROR_UNEXPECTED_DISPLAY_IDX;
}
