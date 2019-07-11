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
    uint8_t *signature = G_io_apdu_buffer;

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
    const uint16_t messageLength =transaction_get_buffer_length();

    uint8_t messageDigest[CX_SHA256_SIZE];
    cx_hash_sha256(message, messageLength, messageDigest, CX_SHA256_SIZE);

    // Sign
    unsigned int info = 0;
    const uint8_t signatureLength = cx_ecdsa_sign(&privateKey,
                                                   CX_RND_RFC6979 | CX_LAST,
                                                   CX_SHA256,
                                                   messageDigest,
                                                   CX_SHA256_SIZE,
                                                   signature,
                                                   IO_APDU_BUFFER_SIZE,
                                                   &info);

    os_memset(&privateKey, 0, sizeof(privateKey));

    return signatureLength;
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
