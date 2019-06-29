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

#include "transaction.h"
#include "apdu_codes.h"
#include "buffering.h"
#include "mantx.h"

#if defined(TARGET_NANOX)
#define RAM_BUFFER_SIZE 8192
#define FLASH_BUFFER_SIZE 16384
#elif defined(TARGET_NANOS)
#define RAM_BUFFER_SIZE 416
#define FLASH_BUFFER_SIZE 8192
#endif

// Ram
uint8_t ram_buffer[RAM_BUFFER_SIZE];

// Flash
typedef struct {
    uint8_t buffer[FLASH_BUFFER_SIZE];
} storage_t;

#if defined(TARGET_NANOS)
storage_t N_appdata_impl __attribute__ ((aligned(64)));
#define N_appdata (*(storage_t *)PIC(&N_appdata_impl))

#elif defined(TARGET_NANOX)
storage_t const N_appdata_impl __attribute__ ((aligned(64)));
#define N_appdata (*(volatile storage_t *)PIC(&N_appdata_impl))
#endif

mantx_context_t ctx_parsed_tx;

void transaction_initialize() {
    buffering_init(
        ram_buffer,
        sizeof(ram_buffer),
        N_appdata.buffer,
        sizeof(N_appdata.buffer)
    );
}

void transaction_reset() {
    buffering_reset();
}

uint32_t transaction_append(unsigned char *buffer, uint32_t length) {
    return buffering_append(buffer, length);
}

uint32_t transaction_get_buffer_length() {
    return buffering_get_buffer()->pos;
}

uint8_t *transaction_get_buffer() {
    return buffering_get_buffer()->data;
}

const char *transaction_parse() {
    const char *transaction_buffer = (const char *) transaction_get_buffer();

    uint8_t err = mantx_parse(
        &ctx_parsed_tx,
        (uint8_t *) transaction_buffer,
        transaction_get_buffer_length());

    if (err != MANTX_NO_ERROR) {
        // TODO: convert to error string
        return "ERROR"; // error_msg;
    }

    // TODO: Validate values
    return NULL;
}
