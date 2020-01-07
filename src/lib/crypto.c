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

#include "crypto.h"
#include "coin.h"

#include "utils/base58.h"
#include "apdu_codes.h"
#include "zxmacros.h"
#include "utils/utils.h"

uint32_t bip44Path[BIP44_LEN_DEFAULT];

void keccak(uint8_t *out, size_t out_len, uint8_t *in, size_t in_len);

#if defined(TARGET_NANOS) || defined(TARGET_NANOX)
#include "cx.h"

void crypto_extractPublicKey(uint32_t bip44Path[BIP44_LEN_DEFAULT], uint8_t *pubKey) {
    cx_ecfp_public_key_t cx_publicKey;
    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[32];

    BEGIN_TRY
    {
        TRY {
            // Generate keys
            os_perso_derive_node_bip32_seed_key(
                    HDW_NORMAL,
                    CX_CURVE_256K1,
                    bip44Path,
                    BIP44_LEN_DEFAULT,
                    privateKeyData,
                    NULL,
                    NULL,
                    0);

            cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, &cx_privateKey);
            cx_ecfp_init_public_key(CX_CURVE_256K1, NULL, 0, &cx_publicKey);
            cx_ecfp_generate_pair(CX_CURVE_256K1, &cx_publicKey, &cx_privateKey, 1);
        }
        FINALLY {
            MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
            MEMZERO(privateKeyData, 32);
        }
    }
    END_TRY;

    MEMCPY(pubKey, cx_publicKey.W, 65);
}

#define DER_OFFSET 65

uint16_t crypto_sign(uint8_t *signature,
                     uint16_t signatureMaxlen,
                     const uint8_t *message,
                     uint16_t messageLength) {

    if (signatureMaxlen < DER_OFFSET + 80) {
        return 0;
    }

    uint8_t messageDigest[CX_SHA256_SIZE];
    int signatureLength;
    uint8_t *der_signature = signature + DER_OFFSET;

    // Hash it
    keccak(messageDigest, sizeof(messageDigest), (uint8_t *) message, messageLength);

    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[32];
    BEGIN_TRY
    {
        TRY
        {
            // Generate keys
            os_perso_derive_node_bip32_seed_key(
                    HDW_NORMAL,
                    CX_CURVE_256K1,
                    bip44Path,
                    BIP44_LEN_DEFAULT,
                    privateKeyData,
                    NULL,
                    NULL,
                    0);
            cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, &cx_privateKey);

            // Sign
            unsigned int info = 0;
            signatureLength = cx_eddsa_sign(&cx_privateKey,
                                            CX_RND_RFC6979 | CX_LAST,
                                            CX_SHA256,
                                            messageDigest,
                                            CX_SHA256_SIZE,
                                            NULL,
                                            0,
                                            der_signature,
                                            signatureMaxlen - DER_OFFSET,
                                            &info);
        #define SIG_V 0
        #define SIG_R 1
        #define SIG_S (SIG_R+32)

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

            signature[SIG_V] = 27;
            if (info & CX_ECCINFO_PARITY_ODD) {
                signature[0]++;
            }
            if (info & CX_ECCINFO_xGTn) {
                signature[0] += 2;
            }

            os_memmove(signature + SIG_R, der_signature + rOffset, 32);
            os_memmove(signature + SIG_S, der_signature + sOffset, 32);
        }
        FINALLY {
            MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
            MEMZERO(privateKeyData, 32);
        }
    }
    END_TRY;

    return DER_OFFSET + signatureLength;
}

void keccak(uint8_t *out, size_t out_len, uint8_t *in, size_t in_len){
    cx_sha3_t sha3;
    cx_keccak_init(&sha3, 256);
    cx_hash((cx_hash_t*)&sha3, CX_LAST, in, in_len, out, out_len);
}

#else

#include "mocks/keccak.h"

void keccak(uint8_t *out, size_t out_len, uint8_t *in, size_t in_len) {
    keccak_hash(out, out_len, in, in_len, 136, 0x01);
}

void crypto_extractPublicKey(uint32_t path[BIP44_LEN_DEFAULT], uint8_t *pubKey) {
    // Empty version for non-Ledger devices
    MEMZERO(pubKey, 32);
}

uint16_t crypto_sign(uint8_t *signature,
                     uint16_t signatureMaxlen,
                     const uint8_t *message,
                     uint16_t messageLen) {
    // Empty version for non-Ledger devices
    return 0;
}

#endif

// calculate ethereum address
// expects ethAddress 20bytes and pubkey 64 bytes
void ethAddressFromPubKey(uint8_t *ethAddress, uint8_t *pubkey) {
    uint8_t tmp[32];
    keccak(tmp, 32, pubkey, 64);
    MEMCPY(ethAddress, tmp + 12, 20);
}

// calculate MAN address and return number of bytes
uint8_t manAddressFromEthAddr(char *manAddress, uint8_t *ethAddress) {
    manAddress[0] = 'M';
    manAddress[1] = 'A';
    manAddress[2] = 'N';
    manAddress[3] = '.';
    char *p = manAddress + 4;

    size_t outlen = 100;
    encode_base58(ethAddress, 20, (unsigned char *) p, &outlen);
    p += outlen;

    // calculate CRC
    *p = (uint8_t) encode_base58_clip(crc8((uint8_t *) manAddress, p - manAddress));
    p++;

    // zero terminate and return
    *p = 0;
    return (uint8_t) (p - manAddress);
}

uint16_t crypto_fillAddress(uint8_t *buffer, uint16_t buffer_len) {
    if (buffer_len < PK_LEN + 50) {
        return 0;
    }

    MEMZERO(buffer, buffer_len);

    // extract pubkey and generate a MAN address
    char *addr = (char *) (buffer + PK_LEN);
    crypto_extractPublicKey(bip44Path, buffer);

    // extract pubkey and generate a MAN address
    uint8_t ethAddress[20];
    ethAddressFromPubKey(ethAddress, buffer + 1);                   // FIXME: why + 1?
    uint8_t addrLen = manAddressFromEthAddr(addr, ethAddress);

    return PK_LEN + addrLen;
}
