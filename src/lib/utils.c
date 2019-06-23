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

#include "utils.h"

uint8_t hexdigit(uint8_t v) {
    v &= 0x0F;
    if (v >= 0x0A) {
        return 'A' + v - 0x0A;
    }
    return '0' + v;
}

// Converts data bytes into a hexstring !! IN PLACE !!
uint8_t convertToHexstringInPlace(uint8_t *data, uint16_t dataLen, uint8_t dataLenMax) {
    if (dataLen == 0) {
        return UTILS_NOT_ENOUGH_DATA;
    }

    if (dataLenMax < dataLen * 2 + 1) {
        return UTILS_NOT_ENOUGH_DATA;
    }

    // clear all empty space so string is terminated
    MEMSET(data + dataLen, 0, dataLenMax - dataLen);

    for (int i = 0; i < dataLen; i++) {
        const uint8_t p = (dataLen - i - 1);
        const uint8_t q = p << 1u;

        data[q] = hexdigit(data[p] >> 4);
        data[q + 1] = hexdigit(data[p] & 0x0F);
    }

    return UTILS_NOERROR;
}
