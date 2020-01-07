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

#include <zxmacros.h>
#include "parser_impl.h"

parser_tx_t parser_tx_obj;

parser_error_t parser_init_context(parser_context_t *ctx,
                                   const uint8_t *buffer,
                                   uint16_t bufferSize) {
    ctx->offset = 0;

    if (bufferSize == 0 || buffer == NULL) {
        // Not available, use defaults
        ctx->buffer = NULL;
        ctx->bufferLen = 0;
        return parser_init_context_empty;
    }

    ctx->buffer = buffer;
    ctx->bufferLen = bufferSize;

    return parser_ok;
}

parser_error_t parser_init(parser_context_t *ctx, const uint8_t *buffer, uint16_t bufferSize) {
    parser_error_t err = parser_init_context(ctx, buffer, bufferSize);
    if (err != parser_ok)
        return err;

    return err;
}

parser_error_t getDisplayTxExtraType(char *out, uint16_t outLen, uint8_t txtype) {
    switch (txtype) {
        case MANTX_TXTYPE_NORMAL:
            snprintf(out, outLen, "Normal");
            break;
//        case MANTX_TXTYPE_BROADCAST:
//            snprintf(out, outLen, "Broadcast");
//            break;
//        case MANTX_TXTYPE_MINER_REWARD:
//            snprintf(out, outLen, "Miner reward");
//            break;
        case MANTX_TXTYPE_SCHEDULED:
            snprintf(out, outLen, "Scheduled");
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
//        case MANTX_TXTYPE_VERIF_REWARD:
//            snprintf(out, outLen, "Verif reward");
//            break;
//        case MANTX_TXTYPE_INTEREST_REWARD:
//            snprintf(out, outLen, "Interest reward");
//            break;
//        case MANTX_TXTYPE_TXFEE_REWARD:
//            snprintf(out, outLen, "Tx Fee reward");
//            break;
//        case MANTX_TXTYPE_LOTTERY_REWARD:
//            snprintf(out, outLen, "Lottery reward");
//            break;
//        case MANTX_TXTYPE_SET_BLACKLIST:
//            snprintf(out, outLen, "Set blacklist");
//            break;
//        case MANTX_TXTYPE_SUPERBLOCK:
//            snprintf(out, outLen, "Super block");
//            break;
        default:
            return parser_invalid_tx_type;
    }

    return parser_ok;
}

parser_error_t parser_read(parser_context_t *ctx, parser_tx_t *v) {
    uint16_t fieldCount;

    // we expect a single root list
    int8_t err = rlp_parseStream(ctx->buffer, 0, ctx->bufferLen, &v->root, 1, &fieldCount);
    if (err != parser_ok)
        return err;
    if (v->root.kind != RLP_KIND_LIST)
        return parser_unexpected_root;

    // now we can extract all rootFields in that list
    err = rlp_readList(ctx->buffer, &v->root, v->rootFields, MANTX_ROOTFIELD_COUNT, &fieldCount);
    if (err != parser_ok)
        return err;
    if (fieldCount != MANTX_ROOTFIELD_COUNT)
        return parser_unexpected_field_count;

    ////////// EXTRA
    // Now parse the extra field
    const rlp_field_t *extraField = &v->rootFields[MANTX_FIELD_EXTRA];
    rlp_field_t extraFieldInternal;
    err = rlp_readList(ctx->buffer,
                       extraField,
                       &extraFieldInternal,
                       1, &fieldCount);
    if (err != parser_ok)
        return err;
    if (fieldCount != 1)
        return parser_unexpected_field_count;
    if (extraFieldInternal.kind != RLP_KIND_LIST)
        return parser_unexpected_field_type;

    // Now parse the extraInternal field
    err = rlp_readList(ctx->buffer,
                       &extraFieldInternal,
                       v->extraFields,
                       MANTX_EXTRAFIELD_COUNT, &fieldCount);

    if (err != parser_ok) {
        return err;
    }
    if (fieldCount != MANTX_EXTRAFIELD_COUNT)
        return parser_unexpected_field_count;

    // Extract extra txType and cache it as metadata
    const rlp_field_t *f = v->extraFields;
    uint256_t tmp;
    err = rlp_readUInt256(ctx->buffer, f, &tmp);
    if (err != RLP_NO_ERROR) { return err; }
    v->extraTxType = tmp.elements[1].elements[1];   // extract last byte

    // Validate txtype
    char tmpBuf[2] = {0, 0};
    err = getDisplayTxExtraType(tmpBuf, 2, v->extraTxType);
    if (err != parser_ok)
        return err;

    //////////
    ////////// EXTRA TO
    //////////
    f = v->extraFields + 2;
    err = rlp_readList(ctx->buffer, f,
                       v->extraToListFields,
                       MANTX_EXTRALISTFIELD_COUNT,
                       &v->extraToListCount);
    if (err != parser_ok)
        return err;
    // Get each extraTo item is a stream of 3 elements (recipient, amount, payload)
    // To avoid using too much memory, we can parse them on demand

    v->JsonCount = 0;

    return parser_ok;
}

parser_error_t _validateTx(const parser_context_t *c, const parser_tx_t *v) {
    return parser_ok;
}

const char *parser_getErrorDescription(parser_error_t err) {
    switch (err) {
        // General errors
        case parser_ok:
            return "No error";
        case parser_no_data:
            return "No more data";
        case parser_init_context_empty:
            return "Initialized empty context";
        case parser_display_idx_out_of_range:
            return "display_idx_out_of_range";
        case parser_display_page_out_of_range:
            return "display_page_out_of_range";
            // Coin specific
        case parser_unexpected_root:
            return "Unexpected root";
        case parser_unexpected_field_count:
            return "Unexpected field count";
        case parser_unexpected_field:
            return "Unexpected field";
        case parser_unexpected_field_type:
            return "Unexpected field type";
        case parser_invalid_time:
            return "Unsupported TxType";
        case parser_invalid_tx_type:
            return "Invalid tx type";
            // Required fields error
        case parser_required_nonce:
            return "Required field nonce";
        case parser_required_method:
            return "Required field method";
        default:
            return "Unrecognized error code";
    }
}

uint8_t _getNumItems(const parser_context_t *c, const parser_tx_t *v) {
    return MANTX_DISPLAY_COUNT + v->extraToListCount * 3 + v->JsonCount;
}
