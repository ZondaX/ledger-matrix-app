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

#include <stdio.h>
#include <zxmacros.h>
#include <bech32.h>
#include <utils/utils.h>
#include "lib/parser_impl.h"
#include "bignum.h"
#include "view_internal.h"
#include "parser.h"
#include "parser_txdef.h"
#include "coin.h"

#if defined(TARGET_NANOX)
// For some reason NanoX requires this function
void __assert_fail(const char * assertion, const char * file, unsigned int line, const char * function){
    while(1) {};
}
#endif

parser_error_t parser_parse(parser_context_t *ctx, const uint8_t *data, uint16_t dataLen) {
    CHECK_PARSER_ERR(parser_init(ctx, data, dataLen))
    return parser_read(ctx, &parser_tx_obj);
}

parser_error_t parser_validate(const parser_context_t *ctx) {
    CHECK_PARSER_ERR(_validateTx(ctx, &parser_tx_obj))

    uint8_t numItems = parser_getNumItems(ctx);

    char tmpKey[40];
    char tmpVal[40];

    for (uint8_t idx = 0; idx < numItems; idx++) {
        uint8_t pageCount;
        CHECK_PARSER_ERR(parser_getItem(ctx, idx, tmpKey, sizeof(tmpKey), tmpVal, sizeof(tmpVal), 0, &pageCount))
    }

    return parser_ok;
}

uint8_t parser_getNumItems(const parser_context_t *ctx) {
    return _getNumItems(ctx, &parser_tx_obj);
}

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

int8_t mantx_print(parser_tx_t *v,
                   const uint8_t *data,
                   int8_t fieldIdx,
                   char *out, uint16_t outLen,
                   uint8_t pageIdx, uint8_t *pageCount) {
    MEMSET(out, 0, outLen);
    uint256_t tmp;
    uint8_t err = RLP_NO_ERROR;

    *pageCount = 1;

    switch (fieldIdx) {
        case MANTX_FIELD_NONCE: {
            const rlp_field_t *f = v->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                if (!tostring256(&tmp, 10, out, outLen)) {
                    err = parser_unexpected_field;
                }
            }
            break;
        }
        case MANTX_FIELD_GASPRICE: {
            const rlp_field_t *f = v->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        case MANTX_FIELD_GASLIMIT: {
            const rlp_field_t *f = v->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        case MANTX_FIELD_TO: {
            const rlp_field_t *f = v->rootFields + fieldIdx;
            uint16_t valueLen;
            err = rlp_readStringPaging(
                    data, f,
                    (char *) out, outLen, &valueLen,
                    pageIdx, pageCount);
            break;
        }
        case MANTX_FIELD_VALUE: {
            const rlp_field_t *f = v->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        case MANTX_FIELD_DATA: {
            const rlp_field_t *f = v->rootFields + fieldIdx;
            uint16_t valueLen;

            switch (v->extraTxType) {
                case MANTX_TXTYPE_NORMAL:
                case MANTX_TXTYPE_SCHEDULED: {
                    // ---------------- Optional DATA FIELD. Show hex if there is data
                    err = rlp_readStringPaging(data, f,
                                               (char *) out,
                                               (outLen - 1) / 2,  // 2bytes per byte + zero termination
                                               &valueLen,
                                               pageIdx, pageCount);
                    //snprintf(out, outLen, "%d - err %d", valueLen, err);
                    if (err == RLP_NO_ERROR) {
                        if (valueLen > 0) {
                            // now we need to convert to hexstring in place
                            convertToHexstringInPlace((uint8_t *) out, valueLen, outLen);
                        } else {
                            *pageCount = 0;
                            err = RLP_NO_ERROR;
                            break;
                        }
                    }
                    break;
                }

                case MANTX_TXTYPE_AUTHORIZED:
                case MANTX_TXTYPE_CREATE_CURR:
                case MANTX_TXTYPE_CANCEL_AUTH:
                    // ---------------- JSON Payload
                    err = rlp_readStringPaging(data, f,
                                               (char *) out, outLen,
                                               &valueLen,
                                               pageIdx, pageCount);
                    break;
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
                case MANTX_TXTYPE_BROADCAST:
                case MANTX_TXTYPE_MINER_REWARD:
                case MANTX_TXTYPE_VERIF_REWARD:
                case MANTX_TXTYPE_INTEREST_REWARD:
                case MANTX_TXTYPE_TXFEE_REWARD:
                case MANTX_TXTYPE_LOTTERY_REWARD:
                case MANTX_TXTYPE_SET_BLACKLIST:
                case MANTX_TXTYPE_SUPERBLOCK:
                default:
                    err = parser_invalid_tx_type;
                    break;
            }

            if (err != RLP_NO_ERROR) { return err; }
            break;
        }
        case MANTX_FIELD_V: {
            const rlp_field_t *f = v->rootFields + fieldIdx;
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
            const rlp_field_t *f = v->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        case MANTX_FIELD_ISENTRUSTTX: {
            const rlp_field_t *f = v->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        case MANTX_FIELD_COMMITTIME: {
            const rlp_field_t *f = v->rootFields + fieldIdx;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                // this should be limited to uint64_t
                if (tmp.elements[0].elements[0] != 0 ||
                    tmp.elements[0].elements[1] != 0 ||
                    tmp.elements[1].elements[0] != 0) {
                    err = parser_invalid_time;
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
            err = getDisplayTxExtraType(out, outLen, v->extraTxType);
            break;
        }
        case MANTX_FIELD_EXTRA_LOCKHEIGHT: {
            const rlp_field_t *f = v->extraFields + 1;
            err = rlp_readUInt256(data, f, &tmp);
            if (err == RLP_NO_ERROR) {
                tostring256(&tmp, 10, out, outLen);
            }
            break;
        }
        default:
            return parser_unexpected_field;
    }

    if (err != parser_ok) {
        snprintf(out, outLen, "err %d", err);
    }

    return err;
}

parser_error_t parser_getItem(const parser_context_t *ctx,
                              int8_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen,
                              uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outKey, outKeyLen);
    MEMZERO(outVal, outValLen);
    snprintf(outKey, outKeyLen, "?");
    snprintf(outVal, outValLen, " ");

    if (displayIdx < 0 || displayIdx >= parser_getNumItems(ctx)) {
        return parser_no_data;
    }

    if (displayIdx < MANTX_DISPLAY_COUNT) {
        snprintf(outVal, outValLen, " ");

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

        int8_t err = mantx_print(&parser_tx_obj, ctx->buffer, fieldIdx,
                                 outVal, outValLen,
                                 pageIdx, pageCount);

        return err;
    }

    if (displayIdx < MANTX_DISPLAY_COUNT + parser_tx_obj.extraToListCount * 3) {
        uint8_t extraToIdx = (displayIdx - MANTX_DISPLAY_COUNT) / 3;
        uint8_t fieldIdx = (displayIdx - MANTX_DISPLAY_COUNT) % 3;

        // Read the stream of three items
        const rlp_field_t *f = &parser_tx_obj.extraToListFields[extraToIdx];
        rlp_field_t extraToFields[3];
        uint16_t fieldCount;
        int8_t err = rlp_readList(ctx->buffer, f, extraToFields, MANTX_ROOTFIELD_COUNT, &fieldCount);
        if (err != parser_ok)
            return err;
        if (fieldCount != 3)
            return parser_unexpected_field_count;

        uint16_t valueLen;
        switch (fieldIdx) {
            case 0: {
                snprintf(outKey, outKeyLen, "[%d] To", extraToIdx);
                err = rlp_readStringPaging(
                        ctx->buffer, extraToFields + 0,
                        (char *) outVal, outValLen, &valueLen,
                        pageIdx, pageCount);
                break;
            }
            case 1: {
                uint256_t tmp;
                snprintf(outKey, outKeyLen, "[%d] Amount", extraToIdx);
                rlp_readUInt256(ctx->buffer, extraToFields + 1, &tmp);
                tostring256(&tmp, 10, outVal, outValLen);
                break;
            }
            case 2: {
                snprintf(outKey, outKeyLen, "[%d] Payload", extraToIdx);
                // ----------------- HEX payload
                err = rlp_readStringPaging(ctx->buffer,
                                           extraToFields + 2,
                                           (char *) outVal,
                                           (outValLen - 1) / 2,  // 2bytes per byte + zero termination
                                           &valueLen,
                                           pageIdx, pageCount);
                //snprintf(out, outLen, "%d - err %d", valueLen, err);
                if (err == RLP_NO_ERROR) {
                    // now we need to convert to hexstring in place
                    convertToHexstringInPlace((uint8_t *) outVal, valueLen, outValLen);
                }
                break;
            }
        }

        return err;
    }

    return parser_display_idx_out_of_range;
}
