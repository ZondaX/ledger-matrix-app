/*******************************************************************************
*   (c) 2016 Ledger
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

#include "actions.h"
#include "lib/crypto.h"
#include "lib/transaction.h"
#include "apdu_codes.h"
#include <os_io_seproxyhal.h>

void keysSecp256k1(cx_ecfp_public_key_t *publicKey,
                   cx_ecfp_private_key_t *privateKey,
                   const uint8_t *privateKeyData) {
    cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, privateKey);
    cx_ecfp_init_public_key(CX_CURVE_256K1, NULL, 0, publicKey);
    cx_ecfp_generate_pair(CX_CURVE_256K1, publicKey, privateKey, 1);
}

uint8_t app_sign() {
#if IO_APDU_BUFFER_SIZE < (65 + 80)
#error This method requires requires a bigger buffer size
#endif

#define DER_OFFSET 65
#define SIG_V 0
#define SIG_R 1
#define SIG_S (SIG_R+32)
#define TEMP_SIZE   (IO_APDU_BUFFER_SIZE - DER_OFFSET)
#define HASH_SIZE 32

    uint8_t *der_signature = G_io_apdu_buffer + DER_OFFSET;

    // Generate keys
    cx_ecfp_public_key_t publicKey;
    cx_ecfp_private_key_t privateKey;
    uint8_t privateKeyData[32];

    os_perso_derive_node_bip32(CX_CURVE_256K1,
                               bip44Path,
                               BIP44_LEN_DEFAULT,
                               privateKeyData, NULL);
    keysSecp256k1(&publicKey, &privateKey, privateKeyData);
    memset(privateKeyData, 0, 32);

    // Hash
    const uint8_t *message = transaction_get_buffer();
    const uint16_t messageLength = transaction_get_buffer_length();

    uint8_t messageDigest[HASH_SIZE];
    keccak(messageDigest, HASH_SIZE, message, messageLength);

    // Sign
    unsigned int info = 0;
    unsigned int signatureLen = cx_ecdsa_sign(&privateKey,
                                              CX_RND_RFC6979 | CX_LAST,
                                              CX_SHA256,
                                              messageDigest,
                                              HASH_SIZE,
                                              der_signature,
                                              TEMP_SIZE,
                                              &info);

    os_memset(&privateKey, 0, sizeof(privateKey));

    // https://github.com/libbitcoin/libbitcoin-system/wiki/ECDSA-and-DER-Signatures#serialised-der-signature-sequence
    // [1 byte]   - DER Prefix
    // [1 byte]   - Payload len
    // [1 byte]   - R Marker. Always 02
    // [1 byte]   - R Len
    // [.?. byte] - R
    // [1 byte]   - S Marker. Always 02
    // [1 byte]   - S Len
    // [.?. byte] - S
    // Prepare response
    // V [1]
    // R [32]
    // S [32]

    uint8_t rOffset = 4;
    uint8_t rLen = der_signature[3];
    if (rLen == 33)
        rOffset++;       // get only 32 bytes

    uint8_t sOffset = rOffset + 2 + 32;
    uint8_t sLen = der_signature[rOffset + 32];
    if (sLen == 33)
        sOffset++;       // get only 32 bytes

    G_io_apdu_buffer[SIG_V] = 27;
    if (info & CX_ECCINFO_PARITY_ODD) {
        G_io_apdu_buffer[0]++;
    }
    if (info & CX_ECCINFO_xGTn) {
        G_io_apdu_buffer[0] += 2;
    }

    os_memmove(G_io_apdu_buffer + SIG_R, der_signature + rOffset, 32);
    os_memmove(G_io_apdu_buffer + SIG_S, der_signature + sOffset, 32);

    return 65 + signatureLen;
}

uint8_t app_fill_address() {
    // Put data directly in the apdu buffer
    uint8_t *const pubKey = G_io_apdu_buffer;
    char *const manAddress = (char *) (G_io_apdu_buffer + 65);
    uint8_t ethAddress[20];

    // extract pubkey and generate a MAN address
    extractPublicKey(bip44Path, pubKey);
    ethAddressFromPubKey(ethAddress, pubKey + 1);
    uint8_t addrLen = manAddressFromEthAddr(manAddress, ethAddress);

    return 65 + addrLen;
}

void app_reply_address() {
    const uint8_t replyLen = app_fill_address();
    set_code(G_io_apdu_buffer, replyLen, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, replyLen + 2);
}
