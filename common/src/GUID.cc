/**
 * @file GUID.cc
 *
 * Implements GUID128
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014 AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include <qcc/platform.h>

#include <qcc/GUID.h>
#include <qcc/Crypto.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "GUID"

namespace qcc {

GUID128::GUID128() : guid(), value(), shortValue()
{
    Crypto_GetRandomBytes(guid, SIZE);
}

GUID128::GUID128(uint8_t init) : guid(), value(), shortValue()
{
    memset(guid, init, SIZE);
}

bool GUID128::Compare(const qcc::String& other)
{
    uint8_t them[SIZE];
    if (HexStringToBytes(other, them, SIZE) == SIZE) {
        return memcmp(guid, them, SIZE) == 0;
    } else {
        return false;
    }
}

bool GUID128::IsGUID(const qcc::String& str, bool exactLen)
{
    if (exactLen && str.length() != (2 * SIZE)) {
        return false;
    } else {
        uint8_t hex[SIZE];
        return HexStringToBytes(str, hex, SIZE) == SIZE;
    }
}

const qcc::String& GUID128::ToString() const
{
    if (value.empty()) {
        value = BytesToHexString(guid, SIZE, true);
    }
    return value;
}


const qcc::String& GUID128::ToShortString() const
{
    if (shortValue.empty()) {
        char outBytes[SHORT_SIZE + 1];
        outBytes[SHORT_SIZE] = '\0';
        for (size_t i = 0; i < SHORT_SIZE; ++i) {
            uint8_t cur = (guid[i] & 0x3F); /* gives a number between 0 and 63 */
            if (cur < 10) {
                outBytes[i] = (cur + '0');
            } else if (cur < 36) {
                outBytes[i] = ((cur - 10) + 'A');
            } else if (cur < 62) {
                outBytes[i] = ((cur - 36) + 'a');
            } else if (cur == 63) {
                outBytes[i] = '_';
            } else {
                outBytes[i] = '-';
            }
        }
        shortValue = outBytes;
    }
    return shortValue;
}

GUID128::GUID128(const qcc::String& hexStr) : value(), shortValue()
{
    size_t size = HexStringToBytes(hexStr, guid, SIZE);
    if (size < SIZE) {
        memset(guid + size, 0, SIZE - size);
    }
}

GUID128& GUID128::operator =(const GUID128& other)
{
    if (this != &other) {
        memcpy(guid, other.guid, sizeof(guid));
        value.clear();
        shortValue.clear();
    }
    return *this;
}

GUID128::GUID128(const GUID128& other) : value(), shortValue()
{
    memcpy(guid, other.guid, sizeof(guid));
}

uint8_t* GUID128::Render(uint8_t* data, size_t len) const
{
    if (len < SIZE) {
        len = SIZE;
    }
    memcpy(data, guid, len);
    return data;
}

void GUID128::SetBytes(const uint8_t* rawBytes)
{
    ::memcpy(guid, rawBytes, SIZE);
    value.clear();
    shortValue.clear();
}

}
