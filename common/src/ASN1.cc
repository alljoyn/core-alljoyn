/**
 * @file ANS1.cc
 *
 * Implementation for methods from Crypto.h
 */

/******************************************************************************
 * Copyright (c) 2011, 2014, AllSeen Alliance. All rights reserved.
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

#include <assert.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/StringUtil.h>

#include <Status.h>

using namespace qcc;

#define QCC_MODULE "CRYPTO"

namespace qcc {


// Forward mapping table for base-64
static const char B64Encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Reverse mapping table for base-64
static const uint8_t B64Decode[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff
};

// Helper function for putting lines breaks at the appropriate location.
static inline void LineBreak(size_t& n, size_t lim, qcc::String& str)
{
    if (++n == lim) {
        n = 0;
        str.push_back('\n');
    }
}

QStatus Crypto_ASN1::EncodeBase64(const qcc::String& bin, qcc::String& b64)
{
    size_t count = bin.size() / 3;
    size_t final = bin.size() % 3;
    b64.reserve(1 + (bin.size() * 3) / 4);
    const uint8_t* p = (const uint8_t*)bin.data();
    size_t lbreak = 0;
    uint32_t accum;

    while (count--) {
        uint32_t x = *p++;
        uint32_t y = *p++;
        uint32_t z = *p++;
        accum = (x << 16) + (y << 8) + z;
        b64.push_back(B64Encode[(accum >> 18) & 0x3F]);
        b64.push_back(B64Encode[(accum >> 12) & 0x3F]);
        b64.push_back(B64Encode[(accum >> 6) & 0x3F]);
        b64.push_back(B64Encode[(accum) & 0x3F]);
        LineBreak(lbreak, 16, b64);
    }
    if (final == 1) {
        uint32_t x = *p++;
        accum = (x << 16);
        b64.push_back(B64Encode[(accum >> 18) & 0x3F]);
        b64.push_back(B64Encode[(accum >> 12) & 0x3F]);
        b64.push_back('=');
        b64.push_back('=');
        LineBreak(lbreak, 16, b64);
    } else if (final == 2) {
        uint32_t x = *p++;
        uint32_t y = *p++;
        accum = (x << 16) + (y << 8);
        b64.push_back(B64Encode[(accum >> 18) & 0x3F]);
        b64.push_back(B64Encode[(accum >> 12) & 0x3F]);
        b64.push_back(B64Encode[(accum >> 6) & 0x3F]);
        b64.push_back('=');
        LineBreak(lbreak, 16, b64);
    }
    if (lbreak) {
        b64.push_back('\n');
    }
    return ER_OK;
}

QStatus Crypto_ASN1::DecodeBase64(const qcc::String& b64in, qcc::String& bin)
{
    QStatus status = ER_OK;

    // Strip whitespace and validate input string
    qcc::String b64;
    b64.reserve(b64in.size());
    qcc::String::const_iterator it = b64in.begin();
    size_t pad = 0;
    while (it != b64in.end()) {
        char c = *it++;
        if (B64Decode[(int)c] != 0xFF && !pad) {
            b64.push_back(c);
        } else if (c != '\n' && c != '\r') {
            if (c != '=') {
                return ER_FAIL;
            }
            pad++;
            b64.push_back(B64Encode[0]);
        }
    }
    // Check we have no more than 2 pad values and a multiple of 4 values
    if (pad > 2 || (b64.size() % 4)) {
        return ER_FAIL;
    }
    // Reserve space for the decoded binary
    bin.reserve((b64.size() * 3) / 4);
    it = b64.begin();
    while (it != b64.end()) {
        uint32_t a = B64Decode[(int)*it++];
        uint32_t b = B64Decode[(int)*it++];
        uint32_t c = B64Decode[(int)*it++];
        uint32_t d = B64Decode[(int)*it++];
        uint32_t triad = (a << 18) | (b << 12) | (c << 6) | d;
        bin.push_back((char)(triad >> 16));
        bin.push_back((char)(triad >> 8));
        bin.push_back((char)triad);
    }
    // Truncate to correct length
    if (pad) {
        bin.erase(bin.size() - pad);
    }
    return status;
}

QStatus Crypto_ASN1::EncodeV(const char*& syntax, qcc::String& asn, va_list* argpIn)
{
    va_list& argp = *argpIn;

    QStatus status = ER_OK;
    size_t len;
    qcc::String* val;

    while ((status == ER_OK) && (*syntax != 0) && (*syntax != ')') && (*syntax != '}')) {
        switch (*syntax++) {
        case 'i':
            {
                asn.push_back((char)ASN_INTEGER);
                uint32_t v = va_arg(argp, uint32_t);
                if (v) {
                    uint8_t n[5];
                    //c++0x does not allow implicit typecast from uint32_t to uint8_t
                    n[0] = 0;
                    n[1] = ((uint8_t)(v >> 24));
                    n[2] = ((uint8_t)(v >> 16));
                    n[3] = ((uint8_t)(v >> 8));
                    n[4] = ((uint8_t)(v & 0xFF));
                    size_t i = 0;
                    while (!n[i]) {
                        ++i;
                    }
                    if (n[i] & 0x80) {
                        --i;
                    }
                    len = 5 - i;
                    EncodeLen(asn, len);
                    asn.append((char*)(n + i), len);
                } else {
                    asn.push_back((char)1);
                    asn.push_back((char)0);
                }
            }
            break;

        case 'l':
            val = va_arg(argp, qcc::String*);
            len = val->size();
            if (len == 0) {
                status = ER_FAIL;
            } else {
                const char* p = val->data();
                // Suppress leading zeroes
                while (len && (*p == 0)) {
                    ++p;
                    --len;
                }
                asn.push_back((char)ASN_INTEGER);
                if (*p & 0x80) {
                    EncodeLen(asn, len + 1);
                    asn.push_back((char)0);
                } else {
                    EncodeLen(asn, len);
                }
                asn.append(p, len);
            }
            break;

        case 'o':
            {
                val = va_arg(argp, qcc::String*);
                asn.push_back((char)ASN_OID);
                qcc::String oid;
                status = EncodeOID(oid, *val);
                if (status == ER_OK) {
                    EncodeLen(asn, oid.size());
                    asn += oid;
                }
            }
            break;

        case 'x':
            val = va_arg(argp, qcc::String*);
            asn.push_back((char)ASN_OCTETS);
            EncodeLen(asn, val->size());
            asn += *val;
            break;

        case 'b':
            {
                val = va_arg(argp, qcc::String*);
                size_t bitLen = va_arg(argp, size_t);
                if (bitLen <= (val->size() * 8)) {
                    size_t unusedBits = (8 - bitLen) & 7;
                    size_t len = (bitLen + 7) / 8;
                    asn.push_back((char)ASN_BITS);
                    EncodeLen(asn, len + 1);
                    asn.push_back((char)unusedBits);
                    asn.append((char*)val->data(), len - 1);
                    // In DER encoding unused bits must be zero
                    asn.push_back((char)(*val)[len - 1] & (0xFF >> unusedBits));
                } else {
                    status = ER_FAIL;
                }
            }
            break;

        case 'n':
            asn.push_back((char)ASN_NULL);
            asn.push_back((char)0);
            break;

        case '(':
            {
                qcc::String seq;
                status = EncodeV(syntax, seq, argpIn);
                if (*syntax++ != ')') {
                    status = ER_FAIL;
                } else if (status == ER_OK) {
                    asn.push_back((char)ASN_SEQ | 0x20);
                    EncodeLen(asn, seq.size());
                    asn += seq;
                }
            }
            break;

        case '{':
            {
                qcc::String set;
                status = EncodeV(syntax, set, argpIn);
                if (*syntax++ != '}') {
                    status = ER_FAIL;
                } else if (status == ER_OK) {
                    asn.push_back((char)ASN_SET_OF | 0x20);
                    EncodeLen(asn, set.size());
                    asn += set;
                }
            }
            break;

        case 'a':
            val = va_arg(argp, qcc::String*);
            asn.push_back((char)ASN_ASCII);
            EncodeLen(asn, val->size());
            asn += *val;
            break;

        case 't':
            val = va_arg(argp, qcc::String*);
            asn.push_back((char)ASN_UTC_TIME);
            EncodeLen(asn, val->size());
            asn += *val;
            break;

        case 'p':
            val = va_arg(argp, qcc::String*);
            asn.push_back((char)ASN_PRINTABLE);
            EncodeLen(asn, val->size());
            asn += *val;
            break;

        case 'u':
            val = va_arg(argp, qcc::String*);
            asn.push_back((char)ASN_UTF8);
            EncodeLen(asn, val->size());
            asn += *val;
            break;

        // This is a raw string inserted into the ASN.1 output.
        // It will only decode correctly if the argument is pre-encoded.
        case 'R':
            val = va_arg(argp, qcc::String*);
            asn += *val;
            break;

        default:
            status = ER_BAD_ARG_1;
            QCC_LogError(status, ("Invalid syntax character \'%c\'", *(syntax - 1)));
        }
    }
    return status;
}

QStatus Crypto_ASN1::DecodeV(const char*& syntax, const uint8_t* asn, size_t asnLen, va_list* argpIn)
{
    va_list& argp = *argpIn;

    if (asnLen == 0) {
        return ER_FAIL;
    }

    QStatus status = ER_OK;
    const uint8_t* eod = asn + asnLen;
    qcc::String* val;

    while ((asn < eod) && (status == ER_OK)) {
        size_t len = 0;
        uint8_t tag = *asn++ & 0x1F;
        switch (*syntax++) {
        case '/':
            --asn;
            continue;

        case 'i':
            if ((tag != ASN_INTEGER) || !DecodeLen(asn, eod, len) || (len > 5) || (len < 1)) {
                status = ER_FAIL;
            } else {
                uint32_t* v = va_arg(argp, uint32_t*);
                *v = 0;
                while (len--) {
                    *v = (*v << 8) + *asn++;
                }
            }
            continue;

        case 'l':
            if ((tag != ASN_INTEGER) || !DecodeLen(asn, eod, len) || (len < 1)) {
                status = ER_FAIL;
            }
            // Supress leading zero
            if (*asn == 0) {
                --len;
                ++asn;
            }
            break;

        case 'o':
            if ((tag != ASN_OID) || !DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
            } else {
                qcc::String* oid = va_arg(argp, qcc::String*);
                *oid = DecodeOID(asn, len);
                asn += len;
            }
            continue;

        case 'x':
            if ((tag != ASN_OCTETS) || !DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
            }
            break;

        case 'b':
            if ((tag != ASN_BITS) || !DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
            } else {
                size_t unusedBits = *asn++;
                if (unusedBits > 7) {
                    status = ER_FAIL;
                } else {
                    --len;
                    val = va_arg(argp, qcc::String*);
                    val->assign((char*)asn, len);
                    asn += len;
                    *va_arg(argp, size_t*) = len * 8 - unusedBits;
                }
            }
            continue;

        case 'n':
            if ((tag != ASN_NULL) || *asn) {
                status = ER_FAIL;
            }
            ++asn;
            continue;

        case '(':
            if ((tag != ASN_SEQ) || !DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
            } else {
                status = DecodeV(syntax, asn, len, &argp);
                if (status == ER_OK) {
                    asn += len;
                }
                if (*syntax++ != ')') {
                    status = ER_FAIL;
                }
            }
            continue;

        case '{':
            if ((tag != ASN_SET_OF) || !DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
            } else {
                status = DecodeV(syntax, asn, len, &argp);
                if (status == ER_OK) {
                    asn += len;
                }
                if (*syntax++ != '}') {
                    status = ER_FAIL;
                }
            }
            continue;

        case '[':
            if (!DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
            } else {
                status = DecodeV(syntax, asn, len, &argp);
                if (status == ER_OK) {
                    asn += len;
                }
                if (*syntax++ != ']') {
                    status = ER_FAIL;
                }
            }
            continue;

        case 'a':
            if ((tag != ASN_ASCII) || !DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
            }
            break;

        case 'p':
            if ((tag != ASN_PRINTABLE) || !DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
            }
            break;

        case 'u':
            if ((tag != ASN_UTF8) || !DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
            }
            break;

        case 't':
            if ((tag != ASN_UTC_TIME) || !DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
            }
            break;

        case '?':
            {
                const uint8_t* start = asn - 1;
                if (!DecodeLen(asn, eod, len)) {
                    status = ER_FAIL;
                } else {
                    asn += len;
                    val = va_arg(argp, qcc::String*);
                    if (val) {
                        val->assign((char*)start, asn - start);
                    }
                }
            }
            continue;

        case '*':
            // Continue consuming items
            --syntax;
            if (!DecodeLen(asn, eod, len)) {
                status = ER_FAIL;
                break;
            }
            asn += len;
            continue;

        case '.':
            {
                // consume the rest of the items
                const uint8_t* start = asn - 1;
                len = eod - start;
                val = va_arg(argp, qcc::String*);
                if (val) {
                    val->assign((char*)start, len);
                }
                asn += len; // move asn forward
                continue;
            }

        default:
            status = ER_BAD_ARG_1;
            QCC_LogError(status, ("Invalid syntax character \'%c\'", *(syntax - 1)));
        }
        // Shared code for all cases that fall through here
        if (status == ER_OK) {
            val = va_arg(argp, qcc::String*);
            val->assign((char*)asn, len);
            asn += len;
        }
    }
    // Consume wildcard if we ran out of data
    if (*syntax == '*') {
        ++syntax;
    } else if (*syntax == '/') {
        // Optional arg was not present
        val = va_arg(argp, qcc::String*);
        val->clear();
        if (syntax[1]) {
            syntax += 2;
        } else {
            status = ER_BAD_ARG_1;
        }
    }
    return status;
}

bool Crypto_ASN1::DecodeLen(const uint8_t*& p, const uint8_t* eod, size_t& l)
{
    if (p >= eod) {
        return false;
    }
    l = *p++;
    if (l & 0x80) {
        size_t n = l & 0x7F;
        l = 0;
        while (n--) {
            if (p >= eod) {
                return false;
            }
            if ((l << 8) < l) {
                // Length decode overflow.
                return false;
            }
            l = (l << 8) + *p++;
        }
    }
    return l <= (uintptr_t)eod - (uintptr_t)p;
}

void Crypto_ASN1::EncodeLen(qcc::String& asn, size_t len)
{
    if (len < 128) {
        asn.push_back((char)len);
    } else {
        uint8_t v[4];
        //c++0x does not allow implicit typecast from uint32_t to uint8_t
        v[0] = ((uint8_t)(len >> 24));
        v[1] = ((uint8_t)(len >> 16));
        v[2] = ((uint8_t)(len >> 8));
        v[3] = ((uint8_t)(len & 0xFF));
        size_t n = 0;
        // Suppress leading zeroes
        while (!v[n]) {
            ++n;
        }
        asn.push_back((char)((4 - n) | 0x80));
        asn.append((char*)(v + n), 4 - n);
    }
}

// ASN1 encode an OID in dotted notation (e.g. 1.2.840.113549.1.5.13)
QStatus Crypto_ASN1::EncodeOID(qcc::String& asn, const qcc::String& oid)
{
    QStatus status = ER_OK;
    // First decode oid into an array of integers
    uint32_t*nums = new uint32_t[oid.size() + 1];
    size_t numNums = 0;
    qcc::String::const_iterator it = oid.begin();
    uint32_t accum = 0;
    while (it != oid.end()) {
        if (*it == '.') {
            nums[numNums++] = accum;
            accum = 0;
        } else if (*it >= '0' && *it <= '9') {
            accum = accum * 10 + *it - '0';
        } else {
            numNums = 0;
            break;
        }
        ++it;
    }
    nums[numNums++] = accum;
    if (numNums < 2) {
        status = ER_FAIL;
    } else {
        // First 2 numbers are packed into a single byte
        asn.push_back((char)(nums[0] * 40 + nums[1]));
        for (size_t i = 2; i < numNums; ++i) {
            uint32_t v = nums[i];
            // Encode bytes base 128
            uint8_t b128[5];
            //c++0x does not allow implicit typecast from uint32_t to uint8_t
            b128[0] = (v >> 28) | 0x80;
            b128[1] =  (v >> 21) | 0x80;
            b128[2] = (v >> 14) | 0x80;
            b128[3] = (v >> 7) | 0x80;
            b128[4] = v  & 0x7f;
            size_t n = 0;
            // Suppress leading zeroes
            while (b128[n] == 0x80) {
                ++n;
            }
            asn.append((char*)(b128 + n), 5 - n);
        }
    }
    delete [] nums;
    return status;
}

qcc::String Crypto_ASN1::DecodeOID(const uint8_t* p, size_t len)
{
    qcc::String oid;
    if (p && len > 0) {
        oid += U32ToString(*p / 40);
        oid.push_back('.');
        oid += U32ToString(*p % 40);
        uint32_t v = 0;
        while (--len) {
            v = (v << 7) + (*(++p) % 128);
            if (!(*p & 0x80)) {
                oid.push_back('.');
                oid += U32ToString(v);
                v = 0;
            }
        }
    }
    return oid;
}

qcc::String Crypto_ASN1::ToString(const uint8_t* asn, size_t len, size_t indent)
{
    qcc::String dump;
    QStatus status = ER_OK;
    const uint8_t* p = asn;
    const uint8_t* eod = asn + len;
    qcc::String ins(indent, ' ');

    while ((status == ER_OK) && (p < eod)) {
        size_t l;
        uint8_t tag = *p++;
        dump += ins;

        switch (tag & 0x1F) {

        case ASN_BOOLEAN:
            {
                dump += "BOOLEAN ";
                dump += *p++ ? "true\n" : "false\n";
            }
            break;

        case ASN_INTEGER:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else {
                if (l <= 4) {
                    uint32_t v = 0;
                    while (l--) {
                        v = (v << 8) + *p++;
                    }
                    dump += "INT ";
                    dump += U32ToString(v);
                    dump += '\n';
                } else {
                    dump += "INT len ";
                    dump += U32ToString(l);
                    dump += LineBreak(BytesToHexString(p, l), 64, indent);
                    p += l;
                }
            }
            break;

        case ASN_BITS:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else {
                uint8_t unused = *p;
                dump += "BIT STRING len ";
                dump += U32ToString(l * 8 - unused);
                dump += LineBreak(BytesToHexString(p, l), 64, indent);
                p += l;
            }
            break;

        case ASN_OCTETS:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else {
                dump += "OCTET STRING len ";
                dump += U32ToString(l);
                dump += LineBreak(BytesToHexString(p, l), 64, indent);
                p += l;
            }
            break;

        case ASN_NULL:
            dump += "NULL\n";
            ++p;
            break;

        case ASN_OID:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else {
                dump += "OID ";
                dump += DecodeOID(p, l);
                dump += '\n';
                p += l;
            }
            break;

        case ASN_SEQ:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else {
                dump += "SEQUENCE len ";
                dump += U32ToString(l);
                dump += '\n';
                dump += ToString(p, l, indent + 2);
                p += l;
            }
            break;

        case ASN_SET_OF:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else {
                dump += "SET_OF len ";
                dump += U32ToString(l);
                dump += '\n';
                dump += ToString(p, l, indent + 2);
                p += l;
            }
            break;

        case ASN_UTF8:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else if (l) {
                qcc::String str((char*)p, l);
                dump += "UTF8 STRING len ";
                dump += U32ToString(l);
                dump += " \"";
                dump += str;
                dump += "\"\n";
                p += l;
            } else {
                dump += "UTF8 STRING len 0\n";
            }
            break;

        case ASN_UTC_TIME:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else {
                dump += "UTC TIME ";
                dump += BytesToHexString(p, l);
                dump += '\n';
                p += l;
            }
            break;

        case ASN_PRINTABLE:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else if (l) {
                qcc::String str((char*)p, l);
                dump += "PRINTABLE STRING len ";
                dump += U32ToString(l);
                dump += " \"";
                dump += str;
                dump += "\"\n";
                p += l;
            } else {
                dump += "PRINTABLE STRING len 0\n";
            }
            break;

        case ASN_ASCII:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else if (l) {
                qcc::String str((char*)p, l);
                dump += "ASCII STRING len ";
                dump += U32ToString(l);
                dump += " \"";
                dump += str;
                dump += "\"\n";
                p += l;
            } else {
                dump += "ASCII STRING len 0\n";
            }
            break;

        default:
            if (!DecodeLen(p, eod, l)) {
                status = ER_FAIL;
            } else {
                dump += "TAG ";
                dump += U32ToString(tag);
                dump += " len ";
                dump += U32ToString(l);
                dump += '\n';
                p += l;
            }
            break;
        }
        if (status == ER_FAIL) {
            dump += "!!!ASN.1 PARSE ERROR!!!\n";
        }
    }
    return dump;
}

}
