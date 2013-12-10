/**
 * @file
 *
 * This file implements methods from IPAddress.h.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <algorithm>
#include <ctype.h>
#include <string.h>

#include <qcc/Debug.h>
#include <qcc/IPAddress.h>
#include <qcc/SocketTypes.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <Status.h>

#define QCC_MODULE "NETWORK"

using namespace std;
using namespace qcc;

IPAddress::IPAddress(const uint8_t* addrBuf, size_t addrBufSize)
{
    assert(addrBuf != NULL);
    assert(addrBufSize == IPv4_SIZE || addrBufSize == IPv6_SIZE);
    addrSize = (uint16_t)addrBufSize;
    if (addrSize == IPv4_SIZE) {
        // Encode the IPv4 address in the IPv6 address space for easy
        // conversion.
        memset(addr, 0, sizeof(addr) - 6);
        memset(&addr[IPv6_SIZE - IPv4_SIZE - sizeof(uint16_t)], 0xff, sizeof(uint16_t));
    }
    memcpy(&addr[IPv6_SIZE - addrSize], addrBuf, addrSize);
}

IPAddress::IPAddress(uint32_t ipv4Addr)
{
    addrSize = IPv4_SIZE;
    memset(addr, 0, sizeof(addr) - 6);
    addr[IPv6_SIZE - IPv4_SIZE - sizeof(uint16_t) + 0] = 0xff;
    addr[IPv6_SIZE - IPv4_SIZE - sizeof(uint16_t) + 1] = 0xff;
    addr[IPv6_SIZE - IPv4_SIZE + 0] = (uint8_t)((ipv4Addr >> 24) & 0xff);
    addr[IPv6_SIZE - IPv4_SIZE + 1] = (uint8_t)((ipv4Addr >> 16) & 0xff);
    addr[IPv6_SIZE - IPv4_SIZE + 2] = (uint8_t)((ipv4Addr >> 8) & 0xff);
    addr[IPv6_SIZE - IPv4_SIZE + 3] = (uint8_t)(ipv4Addr & 0xff);
}

IPAddress::IPAddress(const qcc::String& addrString)
{
    QStatus status = SetAddress(addrString, false);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not resolve \"%s\". Defaulting to INADDR_ANY", addrString.c_str()));
        SetAddress("");
    }
}

qcc::String IPAddress::IPv4ToString(const uint8_t addr[])
{
    qcc::String result = "";

    while (true) {
        if (NULL == addr) {
            break;
        }

        char addressChars[4 * 3 + 3 + 1];
        int32_t addressCharCounter = 0;
        size_t addrCounter = 0;

        while (addrCounter < IPv4_SIZE) {
            int32_t val = addr[addrCounter++];
            int32_t num[3];
            int32_t numIndex = 2;

            if (val > 0) {
                while (val > 0) {
                    int32_t digit = val % 10;
                    val /= 10;
                    num[numIndex--] = digit;
                }
            } else {
                num[numIndex--] = 0;
            }

            for (size_t c = numIndex + 1; c < ArraySize(num); c++) {
                addressChars[addressCharCounter++] = U8ToChar(num[c]);
            }

            if (addrCounter < IPv4_SIZE) {
                addressChars[addressCharCounter++] = '.';
            }
        }

        addressChars[addressCharCounter++] = '\0';
        result = addressChars;
        break;
    }

    return result;
}

qcc::String IPAddress::IPv6ToString(const uint8_t addr[])
{
    qcc::String result = "";

    while (true) {
        if (NULL == addr) {
            break;
        }

        char addressChars[8 * 4 + 7 + 1];
        int32_t addressCharCounter = 0;
        size_t addrCounter = 0;
        int32_t firstGroupZero = -1;
        int32_t lastGroupZero = -1;
        int32_t minFirstGroupZero = -1;
        int32_t maxLastGroupZero = -1;

        // Scan for 0s to collapse
        while (addrCounter < IPv6_SIZE) {
            int32_t val = addr[addrCounter++];
            int32_t val2 = addr[addrCounter++];
            if (val == 0 && val2 == 0) {
                if (firstGroupZero == -1) {
                    firstGroupZero = addrCounter >> 1;
                }

                lastGroupZero = addrCounter >> 1;
            } else {
                if (firstGroupZero != -1 && lastGroupZero != -1) {
                    if (minFirstGroupZero == -1 && maxLastGroupZero == -1) {
                        // Initial scan result
                        minFirstGroupZero = firstGroupZero;
                        maxLastGroupZero = lastGroupZero;
                    } else if ((lastGroupZero - firstGroupZero) > (maxLastGroupZero - minFirstGroupZero)) {
                        minFirstGroupZero = firstGroupZero;
                        maxLastGroupZero = lastGroupZero;
                    }
                }

                firstGroupZero = -1;
                lastGroupZero = -1;
            }
        }

        if (minFirstGroupZero == -1 && maxLastGroupZero == -1) {
            // Initial scan result
            minFirstGroupZero = firstGroupZero;
            maxLastGroupZero = lastGroupZero;
        }

        addrCounter = 0;
        size_t minFirstZero = (minFirstGroupZero - 1) << 1;
        size_t maxLastZero = (maxLastGroupZero - 1) << 1;

        if (minFirstZero == 0 && maxLastZero == 8 &&
            addr[0xa] == 0xff && addr[0xb] == 0xff) {
            // IPV4 mapped IPV6
            addressChars[addressCharCounter++] = ':';
            addressChars[addressCharCounter++] = ':';
            addressChars[addressCharCounter++] = 'f';
            addressChars[addressCharCounter++] = 'f';
            addressChars[addressCharCounter++] = 'f';
            addressChars[addressCharCounter++] = 'f';
            addressChars[addressCharCounter++] = ':';
            addrCounter += 12;

            for (size_t i = addrCounter; i < IPv6_SIZE; i++) {
                int32_t val = addr[i];
                int32_t num[3];
                int32_t numIndex = 2;

                if (val > 0) {
                    while (val > 0) {
                        int32_t digit = val % 10;
                        val /= 10;
                        num[numIndex--] = digit;
                    }
                } else {
                    num[numIndex--] = 0;
                }

                for (size_t c = numIndex + 1; c < ArraySize(num); c++) {
                    addressChars[addressCharCounter++] = U8ToChar(num[c]);
                }

                if (i + 1 < IPv6_SIZE) {
                    addressChars[addressCharCounter++] = '.';
                }
            }
        } else {
            while (addrCounter < IPv6_SIZE) {
                if (addrCounter >= minFirstZero && addrCounter <= maxLastZero) {
                    if (addrCounter == minFirstZero) {
                        addressChars[addressCharCounter++] = ':';
                        addressChars[addressCharCounter++] = ':';
                    }
                    // skip group
                    addrCounter += 2;
                } else {
                    int32_t val = addr[addrCounter++];
                    int32_t temp;
                    bool leadingZero = true;

                    // High nibble (high byte)
                    temp = val >> 4;
                    if (temp != 0 || !leadingZero) {
                        addressChars[addressCharCounter++] = U8ToChar(temp);
                        leadingZero = false;
                    }

                    // Low nible (high byte)
                    temp = val & 0xf;
                    if (temp != 0 || !leadingZero) {
                        addressChars[addressCharCounter++] = U8ToChar(temp);
                        leadingZero = false;
                    }

                    val = addr[addrCounter++];

                    // High nibble (low byte)
                    temp = val >> 4;
                    if (temp != 0 || !leadingZero) {
                        addressChars[addressCharCounter++] = U8ToChar(temp);
                    }

                    // Low nible (low byte)
                    temp = val & 0xf;
                    addressChars[addressCharCounter++] = U8ToChar(temp);

                    if (addrCounter != minFirstZero && addrCounter < IPv6_SIZE) {
                        addressChars[addressCharCounter++] = ':';
                    }
                }
            }
        }

        addressChars[addressCharCounter++] = '\0';
        result = addressChars;
        break;
    }

    return result;
}

inline void SetBits(uint64_t set[], int32_t offset, uint64_t bits)
{
    int32_t index = offset >> 6;
    if (0 == index) {
        set[index] |= (bits << offset);
    } else {
        set[index] |= (bits << (offset - 64));
    }
}

inline int64_t AccumulateDigits(char digits[], int32_t startIndex, int32_t lastIndex, int32_t radix)
{
    if (radix == 16) {
        uint64_t temp = 0;
        for (int32_t index = startIndex; index < lastIndex; index++) {
            temp <<= 4;
            temp |= CharToU8(digits[index]);
        }
        return temp;
    } else if (radix == 10) {
        uint64_t temp = 0;
        for (int32_t index = startIndex; index < lastIndex; index++) {
            temp *= 10;
            char c = digits[index];
            if (!IsDecimalDigit(c)) {
                return -1;
            }
            temp += CharToU8(c);
        }
        return temp;
    } else if (radix == 8) {
        uint64_t temp = 0;
        for (int32_t index = startIndex; index < lastIndex; index++) {
            temp <<= 3;
            char c = digits[index];
            if (!IsOctalDigit(c)) {
                return -1;
            }
            temp |= CharToU8(c);
        }
        return temp;
    }
    return -1;
}

QStatus IPAddress::StringToIPv6(const qcc::String& address, uint8_t addrBuf[], size_t addrBufSize)
{
    QStatus result = ER_OK;

    while (true) {
        if (NULL == addrBuf) {
            result = ER_BAD_ARG_2;
            break;
        }

        if (IPv6_SIZE != addrBufSize) {
            result = ER_BAD_ARG_3;
            break;
        }

        uint64_t leftBits[2] = { 0 };
        uint64_t rightBits[2] = { 0 };
        int32_t leftBitCount = 0;
        int32_t rightBitCount = 0;
        int32_t groupExpansionCount = 0;
        bool groupProcessed = false;
        int32_t octetCount = 0;
        bool leftCounter = false;
        size_t digitCount = 0;
        char digits[4];
        bool parseModeAny = true;

        for (int32_t i = address.size() - 1; i >= 0; --i) {
            if (parseModeAny) {
                if (address[i] == ':' && (i - 1 >= 0) && address[i - 1] == ':') {
                    // ::
                    if (++groupExpansionCount > 1) {
                        // invalid data
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    if (digitCount > 0) {
                        // 16 bit group, convert nibbles to hex
                        int64_t temp = AccumulateDigits(digits, ArraySize(digits) - digitCount, ArraySize(digits), 16);
                        if (temp < 0) {
                            // invalid data (should never happen)
                            result = ER_PARSE_ERROR;
                            break;
                        }

                        // Store in upper or lower 64 bits
                        if (leftCounter) {
                            SetBits(leftBits, leftBitCount, temp);
                            leftBitCount += 16;
                        } else {
                            SetBits(rightBits, rightBitCount, temp);
                            rightBitCount += 16;
                        }
                        // reset digitCount
                        digitCount = 0;
                    }

                    leftCounter = true;

                    // adjust for token
                    --i;
                } else if (address[i] == ':') {
                    // :
                    if (digitCount == 0) {
                        // invalid data
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    // 16 bit group, convert nibbles to hex
                    int64_t temp = AccumulateDigits(digits, ArraySize(digits) - digitCount, ArraySize(digits), 16);
                    if (temp < 0) {
                        // invalid data (should never happen)
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    // Store in upper or lower 64 bits
                    if (leftCounter) {
                        SetBits(leftBits, leftBitCount, temp);
                        leftBitCount += 16;
                    } else {
                        SetBits(rightBits, rightBitCount, temp);
                        rightBitCount += 16;
                    }
                    // reset digitCount
                    digitCount = 0;
                    groupProcessed = true;
                } else if (IsHexDigit(address[i])) {
                    // 0-F
                    if (++digitCount > ArraySize(digits)) {
                        // too much data
                        result = ER_PARSE_ERROR;
                        break;
                    }
                    digits[ArraySize(digits) - digitCount] = address[i];
                } else if (address[i] == '.') {
                    // . (mode transition)
                    if (++octetCount > 4) {
                        // too many octets
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    if (groupProcessed || groupExpansionCount > 0) {
                        // octets have to be the first group
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    if (digitCount == 0) {
                        // invalid data
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    int64_t temp = AccumulateDigits(digits, ArraySize(digits) - digitCount, ArraySize(digits), 10);
                    if (temp < 0 || temp > 0xff) {
                        // invalid decimal digits or range
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    // Store in upper or lower 64 bits
                    if (leftCounter) {
                        SetBits(leftBits, leftBitCount, temp);
                        leftBitCount += 8;
                    } else {
                        SetBits(rightBits, rightBitCount, temp);
                        rightBitCount += 8;
                    }
                    // reset digitCount
                    digitCount = 0;
                    parseModeAny = false;
                } else {
                    // invalid data
                    result = ER_PARSE_ERROR;
                    break;
                }
            } else {
                // Parse octets
                if (IsDecimalDigit(address[i])) {
                    // 0-9
                    if (++digitCount > ArraySize(digits)) {
                        // too much data
                        result = ER_PARSE_ERROR;
                        break;
                    }
                    digits[ArraySize(digits) - digitCount] = address[i];
                } else if (address[i] == '.' || address[i] == ':') {
                    ++octetCount;

                    if (address[i] == ':') {
                        // :
                        if (octetCount != 4) {
                            // too few octets
                            result = ER_PARSE_ERROR;
                            break;
                        }
                        parseModeAny = true;
                    }

                    // .
                    if (octetCount > 4) {
                        // too many octets
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    if (digitCount == 0) {
                        // invalid data
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    int64_t temp = AccumulateDigits(digits, ArraySize(digits) - digitCount, ArraySize(digits), 10);
                    if (temp < 0 || temp > 0xff) {
                        // invalid decimal digits or range
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    // Store in upper or lower 64 bits
                    if (leftCounter) {
                        SetBits(leftBits, leftBitCount, temp);
                        leftBitCount += 8;
                    } else {
                        SetBits(rightBits, rightBitCount, temp);
                        rightBitCount += 8;
                    }
                    // reset digitCount
                    digitCount = 0;
                } else {
                    // invalid data
                    result = ER_PARSE_ERROR;
                    break;
                }
            }
        }

        if (ER_OK != result) {
            break;
        }

        // Handle tail cases
        if (!parseModeAny) {
            // impartial ipv4 address in string termination
            result = ER_PARSE_ERROR;
            break;
        }

        if (digitCount > 0) {
            // 16 bit group, convert nibbles to hex
            int64_t temp = AccumulateDigits(digits, ArraySize(digits) - digitCount, ArraySize(digits), 16);
            if (temp < 0) {
                // invalid data (should never happen)
                result = ER_PARSE_ERROR;
                break;
            }

            // Store in upper or lower 64 bits
            if (leftCounter) {
                SetBits(leftBits, leftBitCount, temp);
                leftBitCount += 16;
            } else {
                SetBits(rightBits, rightBitCount, temp);
                rightBitCount += 16;
            }
        }

        // default initialization
        for (size_t i = 0; i < IPv6_SIZE; i++) {
            addrBuf[i] = 0;
        }

        // Pack results
        if (rightBitCount > 0 && leftBitCount > 0) {
            // Expansion in the middle
            int32_t expansionBits = 128 - rightBitCount - leftBitCount;
            int32_t index = IPv6_SIZE - 1;
            int32_t bitCount = 0;

            while (bitCount < rightBitCount) {
                if (bitCount < 64) {
                    addrBuf[index] = (uint8_t)(rightBits[0] & 0xff);
                    rightBits[0] >>= 8;
                    --index;
                    addrBuf[index] = (uint8_t)(rightBits[0] & 0xff);
                    rightBits[0] >>= 8;
                    --index;
                } else {
                    addrBuf[index] = (uint8_t)(rightBits[1] & 0xff);
                    rightBits[1] >>= 8;
                    --index;
                    addrBuf[index] = (uint8_t)(rightBits[1] & 0xff);
                    rightBits[1] >>= 8;
                    --index;
                }

                bitCount += 16;
            }

            // skip the expansion
            index -= (expansionBits >> 3);
            bitCount = 0;

            while (bitCount < leftBitCount) {
                if (bitCount < 64) {
                    addrBuf[index] = (uint8_t)(leftBits[0] & 0xff);
                    leftBits[0] >>= 8;
                    --index;
                    addrBuf[index] = (uint8_t)(leftBits[0] & 0xff);
                    leftBits[0] >>= 8;
                    --index;
                } else {
                    addrBuf[index] = (uint8_t)(leftBits[1] & 0xff);
                    leftBits[1] >>= 8;
                    --index;
                    addrBuf[index] = (uint8_t)(leftBits[1] & 0xff);
                    leftBits[1] >>= 8;
                    --index;
                }

                bitCount += 16;
            }

            result = ER_OK;
            break;
        } else if (rightBitCount == 0 && leftBitCount == 0) {
            if (groupExpansionCount > 0) {
                result = ER_OK;
                break;
            }
        } else if (leftBitCount > 0) {
            int32_t index = IPv6_SIZE - 1;
            int32_t bitCount = 0;

            if (groupExpansionCount > 0) {
                int32_t expansionBits = 128 - leftBitCount;
                // skip the expansion (already initialized)
                index -= (expansionBits >> 3);
            } else if (leftBitCount != 128) {
                // not enough data
                result = ER_PARSE_ERROR;
                break;
            }

            while (bitCount < leftBitCount) {
                if (bitCount < 64) {
                    addrBuf[index] = (uint8_t)(leftBits[0] & 0xff);
                    leftBits[0] >>= 8;
                    --index;
                    addrBuf[index] = (uint8_t)(leftBits[0] & 0xff);
                    leftBits[0] >>= 8;
                    --index;
                } else {
                    addrBuf[index] = (uint8_t)(leftBits[1] & 0xff);
                    leftBits[1] >>= 8;
                    --index;
                    addrBuf[index] = (uint8_t)(leftBits[1] & 0xff);
                    leftBits[1] >>= 8;
                    --index;
                }

                bitCount += 16;
            }

            result = ER_OK;
            break;
        } else if (rightBitCount > 0) {
            int32_t index = IPv6_SIZE - 1;
            int32_t bitCount = 0;

            // check for expansion
            if (groupExpansionCount == 0 && rightBitCount != 128) {
                // not enough data
                result = ER_PARSE_ERROR;
                break;
            }

            while (bitCount < rightBitCount) {
                if (bitCount < 64) {
                    addrBuf[index] = (uint8_t)(rightBits[0] & 0xff);
                    rightBits[0] >>= 8;
                    --index;
                    addrBuf[index] = (uint8_t)(rightBits[0] & 0xff);
                    rightBits[0] >>= 8;
                    --index;
                } else {
                    addrBuf[index] = (uint8_t)(rightBits[1] & 0xff);
                    rightBits[1] >>= 8;
                    --index;
                    addrBuf[index] = (uint8_t)(rightBits[1] & 0xff);
                    rightBits[1] >>= 8;
                    --index;
                }

                bitCount += 16;
            }

            result = ER_OK;
            break;
        }

        // insufficient data
        result = ER_PARSE_ERROR;
        break;
    }

    return result;
}

// 01.01.01.01, octal
// 0x01.0x01.0x01.0x1, hexadecimal
// 1.1.1.1, decimal, four octets
// 1.1.1, two octets, last is 16 bit,
// 1.1, one octet, second is 24 bits
// 1, value is 32 bits
QStatus IPAddress::StringToIPv4(const qcc::String& address, uint8_t addrBuf[], size_t addrBufSize)
{
    QStatus result = ER_OK;

    while (true) {
        if (NULL == addrBuf) {
            result = ER_BAD_ARG_2;
            break;
        }

        if (IPv4_SIZE != addrBufSize) {
            result = ER_BAD_ARG_3;
            break;
        }

        size_t digitCount = 0;
        // 32-bit integer in octal takes 11 digits
        char digits[11];
        //0 - decimal, 1 - hexadecimal, 2 - octal
        int32_t digitsMode = 0;
        uint32_t parts[4];
        size_t partCount = 0;

        for (size_t i = 0; i < address.size(); i++) {
            if (address[i] == '.') {
                // .
                if (digitCount == 0) {
                    // no data specified
                    result = ER_PARSE_ERROR;
                    break;
                }

                if (partCount >= ArraySize(parts)) {
                    // too many parts
                    result = ER_PARSE_ERROR;
                    break;
                }

                int64_t temp;

                if (digitsMode == 0) {
                    // decimal
                    temp = AccumulateDigits(digits, 0, digitCount, 10);
                } else if (digitsMode == 1) {
                    // hex
                    temp = AccumulateDigits(digits, 0, digitCount, 16);
                } else if (digitsMode == 2) {
                    // octal
                    temp = AccumulateDigits(digits, 0, digitCount, 8);
                } else {
                    // invalid mode
                    result = ER_PARSE_ERROR;
                    break;
                }

                if (temp < 0) {
                    // conversion was invalid
                    result = ER_PARSE_ERROR;
                    break;
                }

                if (temp > 0xFFFFFFFF) {
                    // part overflowed
                    result = ER_PARSE_ERROR;
                    break;
                }

                parts[partCount++] = (uint32_t)temp;
                digitCount = 0;
                // back to decimal default
                digitsMode = 0;
            } else if (address[i] == '0') {
                if (i + 1 < address.size() && address[i + 1] == 'x') {
                    if (digitCount != 0) {
                        // mode change only allowed at the beginning of the value
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    // hexadecimal
                    digitsMode = 1;

                    // adjust for token
                    ++i;
                } else if (digitCount == 0 &&
                           i + 1 < address.size() && IsOctalDigit(address[i + 1])) {
                    // octal
                    digitsMode = 2;
                } else {
                    if (digitCount >= ArraySize(digits)) {
                        // too much data
                        result = ER_PARSE_ERROR;
                        break;
                    }

                    // 0 is valid in all 3 supported bases
                    digits[digitCount++] = address[i];
                }
            } else if (IsHexDigit(address[i])) {
                // accumulate digits

                // validate mode data
                if (digitsMode == 0 && !IsDecimalDigit(address[i])) {
                    // invalid decimal digit
                    result = ER_PARSE_ERROR;
                    break;
                } else if (digitsMode == 1 && !IsHexDigit(address[i])) {
                    // invalid hex digit
                    result = ER_PARSE_ERROR;
                    break;
                } else if (digitsMode == 2 && !IsOctalDigit(address[i])) {
                    // invalid octal digit
                    result = ER_PARSE_ERROR;
                    break;
                } else if (digitsMode < 0 || digitsMode > 2) {
                    // invalid mode
                    result = ER_PARSE_ERROR;
                    break;
                }

                if (digitCount >= ArraySize(digits)) {
                    // too much data
                    result = ER_PARSE_ERROR;
                    break;
                }

                digits[digitCount++] = address[i];
            } else {
                // invalid data
                result = ER_PARSE_ERROR;
                break;
            }
        }

        if (ER_OK != result) {
            break;
        }

        // handle tail case
        if (digitCount > 0) {
            if (partCount >= ArraySize(parts)) {
                // too many parts
                result = ER_PARSE_ERROR;
                break;
            }

            int64_t temp;

            if (digitsMode == 0) {
                // decimal
                temp = AccumulateDigits(digits, 0, digitCount, 10);
            } else if (digitsMode == 1) {
                // hex
                temp = AccumulateDigits(digits, 0, digitCount, 16);
            } else if (digitsMode == 2) {
                // octal
                temp = AccumulateDigits(digits, 0, digitCount, 8);
            } else {
                // invalid mode
                result = ER_PARSE_ERROR;
                break;
            }

            if (temp < 0) {
                // conversion was invalid
                result = ER_PARSE_ERROR;
                break;
            }

            if (temp > 0xFFFFFFFF) {
                // part overflowed
                result = ER_PARSE_ERROR;
                break;
            }

            parts[partCount++] = (uint32_t)temp;
        }

        // check ranges against parts & pack
        if (partCount == 1) {
#if (QCC_TARGET_ENDIAN == QCC_LITTLE_ENDIAN)
            addrBuf[3] = (uint8_t)(parts[0] & 0xFF);
            addrBuf[2] = (uint8_t)((parts[0] >> 8) & 0xFF);
            addrBuf[1] = (uint8_t)((parts[0] >> 16) & 0xFF);
            addrBuf[0] = (uint8_t)((parts[0] >> 24) & 0xFF);
#else
            addrBuf[3] = (uint8_t)((parts[0] >> 24) & 0xFF);
            addrBuf[2] = (uint8_t)((parts[0] >> 16) & 0xFF);
            addrBuf[1] = (uint8_t)((parts[0] >> 8)  & 0xFF);
            addrBuf[0] = (uint8_t)(parts[0] & 0xFF);
#endif
            result = ER_OK;
            break;
        } else if (partCount == 2) {
            if (parts[0] > 0xFF || parts[1] > 0xFFFFFF) {
                // range invalid
                result = ER_PARSE_ERROR;
                break;
            }
#if (QCC_TARGET_ENDIAN == QCC_LITTLE_ENDIAN)
            addrBuf[3] = (uint8_t)(parts[0] & 0xFF);
            addrBuf[2] = (uint8_t)((parts[0] >> 8) & 0xFF);
            addrBuf[1] = (uint8_t)((parts[0] >> 16) & 0xFF);
#else
            addrBuf[3] = (uint8_t)((parts[0] >> 24) & 0xFF);
            addrBuf[2] = (uint8_t)((parts[0] >> 16) & 0xFF);
            addrBuf[1] = (uint8_t)((parts[0] >> 8)  & 0xFF);
#endif
            addrBuf[0] = (uint8_t)parts[0];

            result = ER_OK;
            break;
        } else if (partCount == 3) {
            if (parts[0] > 0xFF || parts[1] > 0xFF ||
                parts[2] > 0xFFFF) {
                // range invalid
                result = ER_PARSE_ERROR;
                break;
            }
#if (QCC_TARGET_ENDIAN == QCC_LITTLE_ENDIAN)
            addrBuf[3] = (uint8_t)(parts[0] & 0xFF);
            addrBuf[2] = (uint8_t)((parts[0] >> 8) & 0xFF);
#else
            addrBuf[3] = (uint8_t)((parts[0] >> 24) & 0xFF);
            addrBuf[2] = (uint8_t)((parts[0] >> 16) & 0xFF);
#endif
            addrBuf[1] = (uint8_t)parts[1];
            addrBuf[0] = (uint8_t)parts[0];

            result = ER_OK;
            break;
        } else if (partCount == 4) {
            if (parts[0] > 0xFF || parts[1] > 0xFF ||
                parts[2] > 0xFF || parts[3] > 0xFF) {
                // range invalid
                result = ER_PARSE_ERROR;
                break;
            }

            addrBuf[3] = (uint8_t)parts[3];
            addrBuf[2] = (uint8_t)parts[2];
            addrBuf[1] = (uint8_t)parts[1];
            addrBuf[0] = (uint8_t)parts[0];

            result = ER_OK;
            break;
        }

        // not enough data specified
        result = ER_PARSE_ERROR;
        break;
    }

    return result;
}

QStatus IPAddress::SetAddress(const qcc::String& addrString, bool allowHostNames, uint32_t timeoutMs)
{
    QStatus status = ER_PARSE_ERROR;

    addrSize = 0;
    memset(addr, 0xFF, sizeof(addr));

    if (addrString.empty()) {
        // INADDR_ANY
        addrSize = IPv6_SIZE;
        status = StringToIPv6("::", addr, addrSize);
    } else if (addrString.find_first_of(':') != addrString.npos) {
        // IPV6
        addrSize = IPv6_SIZE;
        status = StringToIPv6(addrString, addr, addrSize);
    } else {
        // Try IPV4
        addrSize = IPv4_SIZE;
        status = StringToIPv4(addrString, &addr[IPv6_SIZE - IPv4_SIZE], addrSize);
        if (ER_OK != status && allowHostNames) {
            size_t addrLen;
            status = ResolveHostName(addrString, addr, IPv6_SIZE, addrLen, timeoutMs);
            if (ER_OK == status) {
                if (addrLen == IPv6_SIZE) {
                    addrSize = IPv6_SIZE;
                } else {
                    addrSize = IPv4_SIZE;
                }
            }
        }
    }

    return status;
}

QStatus IPAddress::RenderIPv4Binary(uint8_t addrBuf[], size_t addrBufSize) const
{
    QStatus status = ER_OK;
    assert(addrSize == IPv4_SIZE);
    if (addrBufSize < IPv4_SIZE) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Copying IPv4 address to buffer"));
        goto exit;
    }
    memcpy(addrBuf, &addr[IPv6_SIZE - IPv4_SIZE], IPv4_SIZE);

exit:
    return status;
}
QStatus IPAddress::RenderIPv6Binary(uint8_t addrBuf[], size_t addrBufSize) const
{
    QStatus status = ER_OK;
    assert(addrSize == IPv6_SIZE);
    if (addrBufSize < IPv6_SIZE) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Copying IPv6 address to buffer"));
        goto exit;
    }
    memcpy(addrBuf, addr, IPv6_SIZE);

exit:
    return status;
}

QStatus IPAddress::RenderIPBinary(uint8_t addrBuf[], size_t addrBufSize) const
{
    QStatus status = ER_OK;
    if (addrBufSize < addrSize) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Copying IP address to buffer"));
        goto exit;
    }
    memcpy(addrBuf, &addr[IPv6_SIZE - addrSize], addrSize);

exit:
    return status;
}


uint32_t IPAddress::GetIPv4AddressCPUOrder(void) const
{
    return ((static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 0]) << 24) |
            (static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 1]) << 16) |
            (static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 2]) << 8) |
            static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 3]));
}

uint32_t IPAddress::GetIPv4AddressNetOrder(void) const
{
    uint32_t addr4;
    memcpy(&addr4, &addr[IPv6_SIZE - IPv4_SIZE], IPv4_SIZE);
    return addr4;
}

qcc::String IPEndpoint::ToString() const
{
    String ret = addr.ToString();
    ret.append(":");
    ret.append(U32ToString(port, 10));
    return ret;
}
