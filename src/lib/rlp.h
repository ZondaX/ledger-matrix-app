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

#pragma once

#include "base58.h"
#include <zxmacros.h>

#define RLP_KIND_BYTE       0
#define RLP_KIND_STRING     1
#define RLP_KIND_LIST       2

#ifdef __cplusplus
extern "C" {
#endif

uint64_t rlp_decode(const uint8_t *data, uint8_t *kind, uint64_t *len, uint64_t *data_offset);

#ifdef __cplusplus
}
#endif
