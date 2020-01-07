#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// https://github.com/debris/tiny-keccak
// Parameters are based on
// https://github.com/ethereum/solidity/blob/6bbedab383f7c8799ef7bcf4cad2bb008a7fcf2c/libdevcore/Keccak256.cpp

int keccak_hash(uint8_t *out, size_t outlen,
                const uint8_t *in, size_t inlen,
                size_t rate, uint8_t delim);

#ifdef __cplusplus
}
#endif
