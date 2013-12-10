/**
 * @file
 *
 * This file implements string related utility functions
 */

/******************************************************************************
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
#include <cctype>
#include <math.h>

#include <qcc/String.h>
#include <qcc/StringUtil.h>

using namespace std;
using namespace qcc;

#ifndef NAN
// IEEE-754 quiet NaN constant for systems that lack one.
static const unsigned long __qcc_nan = 0x7fffffff;
#define NAN (*reinterpret_cast<const float*>(&__qcc_nan))
#endif


static const char* hexCharsUC = "0123456789ABCDEF";
static const char* hexCharsLC = "0123456789abcdef";

qcc::String qcc::BytesToHexString(const uint8_t* bytes, size_t len, bool toLower, char separator)
{
    qcc::String outBuf;
    const char* hexChars = toLower ? hexCharsLC : hexCharsUC;
    for (size_t i = 0; i < len; i++) {
        if (separator && (i != 0)) {
            outBuf.push_back(separator);
        }
        outBuf.push_back(hexChars[bytes[i] >> 4]);
        outBuf.push_back(hexChars[bytes[i] & 0x0F]);
    }
    return outBuf;
}


size_t qcc::HexStringToBytes(const qcc::String& hex, uint8_t* outBytes, size_t len, char separator)
{
    if (separator) {
        len = min((1 + hex.length()) / 3, len);
    } else {
        len = min(hex.length() / 2, len);
    }
    qcc::String::const_iterator it = hex.begin();
    for (size_t i = 0; i < len; i++) {
        if (separator && (i != 0)) {
            if (*it++ != separator) {
                len = i;
                break;
            }
        }
        uint8_t h = CharToU8(*it++);
        uint8_t l = CharToU8(*it++);
        if ((h > 15) || (l > 15)) {
            len = i;
            break;
        }
        outBytes[i] = (uint8_t)((h << 4) + l);
    }
    return len;
}


qcc::String qcc::HexStringToByteString(const qcc::String& hex, char separator)
{
    size_t len;
    if (separator) {
        len = (1 + hex.length()) / 3;
    } else {
        len = hex.length() / 2;
    }
    qcc::String result(0, '\0', len);
    qcc::String::const_iterator it = hex.begin();
    for (size_t i = 0; i < len; i++) {
        if (separator && (i != 0)) {
            if (*it++ != separator) {
                break;
            }
        }
        uint8_t h = CharToU8(*it++);
        uint8_t l = CharToU8(*it++);
        if ((h > 15) || (l > 15)) {
            break;
        }
        result.push_back((char)((h << 4) + l));
    }
    return result;
}


qcc::String qcc::U32ToString(uint32_t num, unsigned int base, size_t width, char fill)
{
    qcc::String outStr;
    uint32_t tmp = num;
    size_t pos = 0;

    // Just incase someone overrides the default width with 0.
    width = (width == 0) ? 1 : width;

    while (--width > 0) {
        if (tmp >= base) {
            tmp /= base;
        } else {
            ++pos;
            outStr.push_back(fill);
        }
    }

    if ((0 < base) && (16 >= base)) {
        do {
            outStr.insert((size_t)pos, &hexCharsUC[num % base], 1);
            num = num / base;
        } while (num);
    }

    if (outStr.empty()) {
        outStr.push_back('0');
    }

    return outStr;
}


qcc::String qcc::I32ToString(int32_t num, unsigned int base, size_t width, char fill)
{
    const char* sign;
    uint32_t unum;

    // Just incase caller overrides the default width with 0.
    width = (width == 0) ? 1 : width;

    if (num >= 0) {
        sign = "";
        unum = (uint32_t)num;
    } else {
        sign = "-";
        if ((num << 1) == 0) {
            unum = (uint32_t)(num);
        } else {
            unum = (uint32_t)(-num);
        }
        --width;
    }
    return sign + U32ToString(unum, base, width, fill);
}


qcc::String qcc::U64ToString(uint64_t num, unsigned int base, size_t width, char fill)
{
    qcc::String outStr;
    uint64_t tmp = num;
    size_t pos = 0;

    // Just incase someone overrides the default width with 0.
    width = (width == 0) ? 1 : width;

    while (--width > 0) {
        if (tmp >= base) {
            tmp /= base;
        } else {
            ++pos;
            outStr.push_back(fill);
        }
    }

    if ((0 < base) && (16 >= base)) {
        do {
            outStr.insert((size_t)pos, &hexCharsUC[num % base], 1);
            num = num / base;
        } while (num);
    }

    if (outStr.empty()) {
        outStr.push_back('0');
    }

    return outStr;
}


qcc::String qcc::I64ToString(int64_t num, unsigned int base, size_t width, char fill)
{
    const char* sign;
    uint64_t unum;

    // Just incase caller overrides the default width with 0.
    width = (width == 0) ? 1 : width;

    if (num >= 0) {
        sign = "";
        unum = (uint64_t)num;
    } else {
        sign = "-";
        if ((num << 1) == 0) {
            unum = (uint64_t)(num);
        } else {
            unum = (uint64_t)(-num);
        }
        --width;
    }
    return sign + U64ToString(unum, base, width, fill);
}


uint32_t qcc::StringToU32(const qcc::String& inStr, unsigned int base, uint32_t badValue)
{
    uint32_t val = 0;

    if (base > 16) {
        return badValue;
    }
    // Convert inStr to val
    bool isBad = true;
    qcc::String::const_iterator it = inStr.begin();
    if (base == 0) {
        if (*it == '0') {
            ++it;
            if (it == inStr.end()) {
                return 0;
            } else if ((*it == 'x') || (*it == 'X')) {
                ++it;
                base = 16;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*it == '0') {
            ++it;
            if ((*it == 'x') || (*it == 'X')) {
                ++it;
            }
        }
    }
    while (it != inStr.end()) {
        const char c = *it++;
        if (!IsWhite(c)) {
            uint8_t n = CharToU8(c);
            isBad = (n >= base);
            if (isBad) {
                break;
            } else {
                val *= base;
                val += n;
            }
        } else if (!isBad) {
            break;
        }
    }

    return isBad ? badValue : val;
}


int32_t qcc::StringToI32(const qcc::String& inStr, unsigned int base, int32_t badValue)
{
    if (!inStr.empty()) {
        if (inStr[0] == '-') {
            uint32_t i = StringToU32(inStr.substr(1, inStr.npos), base, (uint32_t)badValue);
            if ((i != (uint32_t)badValue) && (i <= 0x80000000)) {
                return -(int32_t)i;
            }
        } else {
            uint32_t i = StringToU32(inStr, base, (uint32_t)badValue);
            if ((i != (uint32_t)badValue) && (i < 0x80000000)) {
                return (int32_t)i;
            }
        }
    }
    return badValue;
}


uint64_t qcc::StringToU64(const qcc::String& inStr, unsigned int base, uint64_t badValue)
{
    uint64_t val = 0;

    if (base > 16) {
        return badValue;
    }
    // Convert inStr to val
    bool isBad = true;
    qcc::String::const_iterator it = inStr.begin();
    if (base == 0) {
        if (*it == '0') {
            ++it;
            if (it == inStr.end()) {
                return 0;
            } else if ((*it == 'x') || (*it == 'X')) {
                ++it;
                base = 16;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*it == '0') {
            ++it;
            if ((*it == 'x') || (*it == 'X')) {
                ++it;
            }
        }
    }
    while (it != inStr.end()) {
        const char c = *it++;
        if (!IsWhite(c)) {
            uint8_t n = CharToU8(c);
            isBad = (n >= base);
            if (isBad) {
                break;
            } else {
                val *= base;
                val += n;
            }
        } else if (!isBad) {
            break;
        }
    }

    return isBad ? badValue : val;
}


int64_t qcc::StringToI64(const qcc::String& inStr, unsigned int base, int64_t badValue)
{
    if (!inStr.empty()) {
        if (inStr[0] == '-') {
            uint64_t i = StringToU64(inStr.substr(1, inStr.npos), base, (uint32_t)badValue);
            if ((i != (uint64_t)badValue) && (i <= ((uint64_t)1 << 63))) {
                return -(int64_t)i;
            }
        } else {
            uint64_t i = StringToU64(inStr, base, (uint32_t)badValue);
            if ((i != (uint64_t)badValue) && (i < ((uint64_t)1 << 63))) {
                return (int64_t)i;
            }
        }
    }
    return badValue;
}

double qcc::StringToDouble(const qcc::String& inStr)
{
    const double decimal_base = 10.0;
    if (!inStr.empty()) {
        double val = 0.0;
        bool neg = false;
        qcc::String::const_iterator it = inStr.begin();
        if (*it == '-') {
            neg = true;
            ++it;
        }
        while ((it != inStr.end()) && (*it != '.') && ((*it != 'e') && (*it != 'E'))) {
            uint8_t v = CharToU8(*it);
            if (v >= 10) {
                return NAN;
            }
            val *= decimal_base;
            val += static_cast<double>(v);
            ++it;
        }
        if (*it == '.') {
            double divisor = 1.0;
            ++it;
            while ((it != inStr.end()) && ((*it != 'e') && (*it != 'E'))) {
                uint8_t v = CharToU8(*it);
                if (v >= 10) {
                    return NAN;
                }
                val *= decimal_base;
                val += static_cast<double>(v);
                divisor *= decimal_base;
                ++it;
            }
            val /= divisor;
        }
        if ((*it == 'e') || (*it == 'E')) {
            ++it;
            qcc::String exponentString = qcc::String(it, inStr.end() - it);

            // verify that the exponent portion is sane
            qcc::String::const_iterator expStrIter = exponentString.begin();
            if (*expStrIter == '-') {
                ++expStrIter;
            }
            while (expStrIter != exponentString.end()) {
                if (0 > CharToU8(*expStrIter) || 9 < CharToU8(*expStrIter)) {
                    return NAN;
                }
                ++expStrIter;
            }

            int32_t exp = StringToI32(exponentString);
            while (exp < 0) {
                val /= decimal_base;
                ++exp;
            }
            while (exp > 0) {
                val *= decimal_base;
                --exp;
            }
        }
        if (neg) {
            val = -val;
        }
        return val;

    }
    return NAN;
}


qcc::String qcc::LineBreak(const qcc::String& inStr, size_t maxLen, size_t indent)
{
    qcc::String indentStr(' ', indent);
    qcc::String outStr;
    outStr.reserve(inStr.size() + maxLen + (inStr.size() / maxLen) * (indent + 1));
    size_t pos = 0;
    while (pos < inStr.size()) {
        outStr += indentStr + inStr.substr(pos, maxLen);
        outStr.push_back('\n');
        pos += maxLen;
    }
    return outStr;

}

qcc::String qcc::Trim(const qcc::String& str)
{
    size_t start = str.find_first_not_of(" \t\n\r\v");
    size_t end = str.find_last_not_of(" \t\n\r\v");
    if ((0 == start) && (str.size() == (end + 1))) {
        return str;
    } else if ((qcc::String::npos == start) && (qcc::String::npos == end)) {
        return qcc::String();
    } else if (qcc::String::npos == end) {
        return str.substr(start);
    } else if (qcc::String::npos == start) {
        return str.substr(0, end);
    } else {
        return str.substr(start, end - start + 1);
    }
}

bool qcc::IsWhite(char c, const char* whiteChars) {
    if (!whiteChars) {
        whiteChars = " \t\n\r\v";
    }
    while (*whiteChars) {
        if (c == *whiteChars++) {
            return true;
        }
    }
    return false;
}

qcc::String qcc::StringVectorToString(const vector<qcc::String>* list, const char* sep)
{
    qcc::String out;
    if (list) {
        std::vector<qcc::String>::const_iterator it = list->begin();
        if (it != list->end()) {
            while (true) {
                out += *it++;
                if (it == list->end()) {
                    break;
                }
                out += sep;
            }
        }
    }
    return out;
}

bool qcc::IsDecimalDigit(char c)
{
    if (c >= '0' && c <= '9') {
        return true;
    }
    return false;
}

bool qcc::IsAlpha(char c)
{
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        return true;
    }
    return false;
}

bool qcc::IsAlphaNumeric(char c)
{
    return IsAlpha(c) || IsDecimalDigit(c);
}

bool qcc::IsOctalDigit(char c)
{
    if (c >= '0' && c <= '7') {
        return true;
    }
    return false;
}

bool qcc::IsHexDigit(char c)
{
    if (c >= '0' && c <= '9') {
        return true;
    } else if (c >= 'a' && c <= 'f') {
        return true;
    } else if (c >= 'A' && c <= 'F') {
        return true;
    }
    return false;
}

uint8_t qcc::CharToU8(const char c)
{
    if (c >= '0' && c <= '9') {
        return (uint8_t)(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return (uint8_t)(10 + c - 'A');
    }
    if (c >= 'a' && c <= 'f') {
        return (uint8_t)(10 + c - 'a');
    }
    return 255;
}

char qcc::U8ToChar(uint8_t d)
{
    if (d >= 0 && d <= 9) {
        return (char)(d + '0');
    } else if (d >= 0xa && d <= 0xf) {
        return (char)(d - 0xa + 'a');
    }
    return '\0';
}
