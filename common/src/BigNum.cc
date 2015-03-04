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
#include <stdio.h>
#include <algorithm>

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/BigNum.h>
#include <qcc/Crypto.h>
#include <qcc/Util.h>
#include <qcc/Debug.h>

using namespace qcc;

uint32_t BigNum::zero_digit = 0;

const BigNum BigNum::zero;

#define USE_DEBRUIJN_POS

static inline uint32_t log2(uint32_t n)
{
#ifdef USE_DEBRUIJN_POS
    /*
     * Combined with the "magic" number below this converts a single set bit into a bit position.
     */
    static const uint32_t DeBruijnPos[] = {
        0,  9,  1, 10, 13, 21,  2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
        8, 12, 20, 28, 15, 17, 24,  7, 19, 27, 23,  6, 26,  5, 4, 31
    };
    static const uint32_t DeBruijnMagic = 0x07C4ACDD;
    /*
     * This process rounds down to one less than a power of 2. This is a variation on
     * the classic bit twiddle for rounding up to an exact power of 2 but eliminates
     * the increment and decrement.
     */
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    /*
     * n is now 1 one of exactly 32 values. The computation below maps the values into an
     * index into the DeBruijnPos value that returns the position of the most significant bit.
     */
    return DeBruijnPos[(n * DeBruijnMagic) >> 27];
#else
    uint32_t l = 0;
    if (n & 0xFFFF0000) {
        n >>= 16; l += 16;
    }
    if (n & 0x0000FF00) {
        n >>= 8;  l += 8;
    }
    if (n & 0x000000F0) {
        n >>= 4;  l += 4;
    }
    if (n & 0x0000000C) {
        n >>= 2;  l += 2;
    }
    if (n & 0x00000002) {
        n >>= 1;  l += 1;
    }
    return l;
#endif
}

// Type for storage
class BigNum::Storage {
  public:

    static Storage* New(size_t sz, uint32_t* val = NULL, size_t extra = 4)
    {
        size_t mallocSz = sizeof(Storage) + (sz + extra) * sizeof(uint32_t);
        uint8_t* p = (uint8_t*)malloc(mallocSz);
        assert(p);
        Storage* s = new (p)Storage();
        s->buffer = reinterpret_cast<uint32_t*>(p + sizeof(Storage));
        s->size = sz + extra;
        s->refCount = 1;
        if (val) {
            memcpy(s->buffer, val, sizeof(uint32_t) * sz);
            if (extra) {
                memset(s->buffer + sz, 0, sizeof(uint32_t) * extra);
            }
        } else {
            memset(s->buffer, 0, sizeof(uint32_t) * s->size);
        }
        return s;
    }

    Storage* AddRef() { ++refCount; return this; }

    bool DecRef() { return --refCount == 0; }

    uint32_t* buffer;
    size_t size;
    uint32_t refCount;

  private:
    Storage() { }

};


BigNum::~BigNum()
{
    if (storage && storage->DecRef()) {
        storage->Storage::~Storage();
        free(storage);
    }
}

// Private constructor that allocates storage
BigNum::BigNum(size_t len, bool neg) : length(len), neg(neg), storage(Storage::New(len))
{
    digits = storage->buffer;
}

// Constructor to initialize a small integer value
BigNum::BigNum(uint32_t v)
{
    neg = false;
    length = 1;
    if (v == 0) {
        storage = NULL;
        digits = zero.digits;
    } else {
        storage = Storage::New(1, (uint32_t*)&v);
        digits = storage->buffer;
    }
}

// Copy constructor
BigNum::BigNum(const BigNum& other)
{
    /*
     * Check if copying from a temporary (has a value but no storage)
     */
    if (other.storage == NULL) {
        storage = Storage::New(other.length, other.digits);
        neg = other.neg;
        length = other.length;
        digits = storage->buffer;
    } else {
        storage = NULL;
        *this = other;
    }
    strip_lz(*this);
}

// Assignment operator
BigNum& BigNum::operator=(const BigNum& other)
{
    if (&other != this) {
        if (storage && storage->DecRef()) {
            storage->Storage::~Storage();
            free(storage);
            storage = NULL;
        }
        neg = other.neg;
        length = other.length;
        if (other.storage) {
            storage = other.storage->AddRef();
            digits = other.digits;
        } else {
            storage = Storage::New(length, other.digits);
            digits = storage->buffer;
        }
    }
    return *this;
}

BigNum BigNum::clone(size_t extra) const
{
    BigNum clone;
    clone.neg = neg;
    clone.length = length;
    clone.storage = Storage::New(length, digits, extra);
    clone.digits = clone.storage->buffer;
    return clone;
}

BigNum& BigNum::zero_ext(size_t size)
{
    assert(size >= length);
    if (size != length) {
        if (storage) {
            assert(digits == storage->buffer);
            if (size <= storage->size) {
                memset(digits + length, 0, (size - length) * sizeof(uint32_t));
            } else {
                Storage* s = Storage::New(length, digits, size - length);
                if (storage->DecRef()) {
                    storage->Storage::~Storage();
                    free(storage);
                    storage = NULL;
                }
                storage = s;
                digits = storage->buffer;
            }
        } else {
            storage = Storage::New(length, digits, size - length);
            digits = storage->buffer;
        }
        length = size;
    }
    return *this;
}

BigNum& BigNum::reset(size_t len, bool neg, bool clear)
{
    // Reuse storage only if it is big enough and has only one reference
    if (storage && ((storage->size < len) || (storage->refCount > 1))) {
        if (storage->DecRef()) {
            storage->Storage::~Storage();
            free(storage);
        }
        storage = NULL;
    }
    if (storage) {
        if (clear) {
            memset(storage->buffer, 0, len * sizeof(uint32_t));
        }
    } else {
        storage = Storage::New(len);
    }
    digits = storage->buffer;
    length = len;
    this->neg = neg;
    return *this;
}

// Set value from a hexadecimal string
bool BigNum::set_hex(const qcc::String& number)
{
    const char* p = number.c_str();
    size_t len = number.size();

    if (storage && storage->DecRef()) {
        storage->Storage::~Storage();
        free(storage);
        storage = NULL;
    }
    // Check for negation
    if (p[0] == '-') {
        ++p;
        --len;
        neg = true;
    } else {
        neg = false;
    }
    // Check for 0x prefix
    if (p[0] == '0' && p[1] == 'x') {
        p += 2;
        len -= 2;
    }
    // Skip leading zeroes
    while (*p == '0') {
        ++p;
        --len;
    }
    length = (len + 7) / 8;
    // Special case for 0
    if (length == 0) {
        *this = zero;
        return true;
    }
    storage = Storage::New(length);
    digits = storage->buffer;
    // Build in little-endian order
    uint32_t* v = digits;
    p += len - 1;
    while (len) {
        uint32_t n = 0;
        for (int i = 0; (i < 32) && len; i += 4, --len) {
            if (*p >= '0' && *p <= '9') {
                n |= (*p - '0') << i;
            } else if (*p >= 'a' && *p <= 'f') {
                n |=  (*p + 10 - 'a') << i;
            } else if (*p >= 'A' && *p <= 'F') {
                n |=  (*p + 10 - 'A') << i;
            } else {
                storage->Storage::~Storage();
                free(storage);
                storage = NULL;
                digits = NULL;
                length = 0;
                return false;
            }
            --p;
        }
        *v++ = n;
    }
    return true;
}

void BigNum::set_bytes(const uint8_t* data, size_t len)
{
    if (storage && storage->DecRef()) {
        storage->Storage::~Storage();
        free(storage);
        storage = NULL;
    }
    length = (len + 3) / 4;
    storage = Storage::New(length);
    digits = storage->buffer;
    neg = false;
    uint32_t* v = digits;
    const uint8_t* p = data + len;
    while (len) {
        uint32_t n = 0;
        for (int i = 0; (i < 32) && len; i += 8, --len) {
            n |= *(--p) << i;
        }
        *v++ = n;
    }
}

void BigNum::gen_rand(size_t len)
{
    // Allocate enough room
    reset((3 + len) / sizeof(uint32_t), false, false);
    Crypto_GetRandomBytes((uint8_t*)digits, length * sizeof(uint32_t));
    // Mask of excess bytes
    size_t excess = length * sizeof(uint32_t) - len;
    digits[length - 1] &= (0xFFFFFFFF >> (8 * excess));
}

size_t BigNum::get_bytes(uint8_t* buffer, size_t len, bool pad) const
{
    uint8_t* p = buffer;
    uint8_t nZ = 0;

    // zero pad leading digits if requested
    if (pad && (len > byte_len())) {
        size_t padLen = len - byte_len();
        memset(p, 0, padLen);
        p += padLen;
        len -= padLen;
    }
    const uint32_t* digit = &digits[length - 1];
    for (size_t i = 0; i < length; ++i) {
        uint32_t d = *digit--;
        for (size_t j = 0; j < 4; ++j) {
            if ((nZ |= (*p = (uint8_t)(d >> 24)))) {
                ++p;
                --len;
            }
            d <<= 8;
            if (len == 0) {
                goto exit;
            }
        }
    }
exit:
    return p - buffer;
}

// Set value from a decimal string
bool BigNum::set_dec(const qcc::String& number)
{
    return false;
}

// Render the value as a hexadecimal string
qcc::String BigNum::get_hex(bool toLower) const
{
    qcc::String str = "0";
    const char* fmt = toLower ? "%08x" : "%08X";

    for (size_t n = length; n > 0; --n) {
        char i[9];
        snprintf(i, 9, fmt, digits[n - 1]);
        str += i;
    }
    // Trim leading zeroes and set sign
    size_t nz = str.find_first_not_of("0", 0);
    if (nz == String::npos) {
        str = "0";
    } else if (nz > 0) {
        str.erase(0, nz);
        if (neg) {
            str.insert(0, "-");
        }
    }
    return str;
}

// Addition operation
BigNum BigNum::operator+(const BigNum& n) const
{
    if (n.neg) {
        return *this - (-n);
    }
    if (neg) {
        return n - (-*this);
    }
    const uint32_t* x;
    const uint32_t* y;
    size_t xLen, yLen, rLen;
    if (length >= n.length) {
        xLen = length;
        yLen = n.length;
        x = digits;
        y = n.digits;
    } else {
        xLen = n.length;
        yLen = length;
        x = n.digits;
        y = digits;
    }
    BigNum result(xLen + 1, false);
    uint32_t* r = result.digits;
    rLen = 0;
    uint64_t carry = 0;
    // Loop while we have both x and y digits
    while (rLen < yLen) {
        uint64_t n = (uint64_t)*x++ + (uint64_t)*y++ + carry;
        carry = n >> 32;
        *r++ = (uint32_t)n;
        ++rLen;
    }
    // Continue propogating carry
    while (rLen < xLen) {
        uint64_t n = (uint64_t)*x++ + carry;
        carry = n >> 32;
        *r++ = (uint32_t)n;
        ++rLen;
    }
    if (carry) {
        *r = carry;
        ++rLen;
    }
    result.length = rLen;
    return result;
}

// Monadic addition
BigNum& BigNum::operator+=(const BigNum& n)
{
    if (!neg && n.neg) {
        return sub(n);
    } else {
        return *this = *this + n;
    }
}

// Add an integer to an BigNum returning a new BigNum
BigNum BigNum::operator+(uint32_t i) const
{
    if (i == 0) {
        return *this;
    } else {
        BigNum n;
        n.length = 1;
        n.digits = &i;
        return *this + n;
    }
}

// Add an integer to an BigNum
BigNum& BigNum::operator+=(uint32_t i)
{
    BigNum n;
    n.length = 1;
    n.digits = &i;
    return *this += n;
}

// Subtraction operation
BigNum BigNum::operator-(const BigNum& n) const
{
    if (n.neg) {
        return *this + (-n);
    }
    if (neg) {
        return -(n - *this);
    }
    const uint32_t* x;
    const uint32_t* y;
    bool neg;
    size_t xLen, yLen;
    // Subtract smaller value from larger value
    if (length > n.length) {
        xLen = length;
        yLen = n.length;
        x = digits;
        y = n.digits;
        neg = false;
    } else if (length < n.length) {
        xLen = n.length;
        yLen = length;
        x = n.digits;
        y = digits;
        neg = true;
    } else {
        xLen = length;
        while (digits[xLen - 1] == n.digits[xLen - 1]) {
            if (--xLen == 0) {
                return 0;
            }
        }
        if (digits[xLen - 1] > n.digits[xLen - 1]) {
            x = digits;
            y = n.digits;
            neg = false;
        } else {
            x = n.digits;
            y = digits;
            neg = true;
        }
        yLen = xLen;
    }
    BigNum result(xLen, neg);
    uint32_t* r = result.digits;
    size_t rLen = 0;
    uint64_t borrow = 0;
    // Loop while we have both x and y values
    while (rLen < yLen) {
        uint64_t n = (uint64_t)*x++ - (uint64_t)*y++ - borrow;
        borrow = n >> 63;
        *r++ = (uint32_t)n;
        ++rLen;
    }
    // Continue propogating borrow
    while (rLen < xLen) {
        uint64_t n = (uint64_t)*x++ - borrow;
        borrow = n >> 63;
        *r++ = (uint32_t)n;
        ++rLen;
    }
    result.length = rLen;
    return strip_lz(result);
}

// Monadic substraction
BigNum& BigNum::operator-=(const BigNum& n)
{
    assert(!haslz());
    assert(!n.haslz());
    if ((this->length > n.length) && (this->neg == n.neg)) {
        return this->sub(n);
    } else {
        return *this = *this - n;
    }
}

// Negation
BigNum BigNum::operator-() const
{
    BigNum result(*this);
    result.neg = !result.neg;
    return result;
}

// Subtract an integer from an BigNum
BigNum BigNum::operator-(uint32_t i) const
{
    assert(!haslz());
    if (i == 0) {
        return *this;
    } else {
        BigNum n;
        n.length = 1;
        n.neg = false;
        n.digits = &i;
        return *this - n;
    }
}

// Subtract an integer from an BigNum
BigNum& BigNum::operator-=(uint32_t i)
{
    assert(!haslz());
    BigNum n;
    n.length = 1;
    n.neg = false;
    n.digits = &i;
    if (this->neg) {
        return *this = *this - n;
    } else {
        return this->sub(n);
    }
}

// multiple-precision multiplication by an integer
BigNum& BigNum::mul(BigNum& result, const BigNum& a, uint32_t b, bool bneg)
{
    assert(!result.storage || (result.storage != a.storage));
    if (b > 2) {
        result.reset(a.length + 1, a.neg ^ bneg);
        // single-precision multiplication
        uint32_t* r = result.digits;
        const uint32_t* v = a.digits;
        uint64_t carry = 0;
        for (size_t i = 0; i < a.length; ++i) {
            uint64_t x = (uint64_t)(*v++) * b + carry;
            *r++ = (uint32_t)x;
            carry = x >> 32;
        }
        *r = (uint32_t)carry;
    } else if (b == 1) {
        result = a.clone();
        result.neg = a.neg ^ bneg;
    } else if (b == 2) {
        result = a << 1;
        result.neg = a.neg ^ bneg;
    } else {
        result = zero;
    }
    return strip_lz(result);
}

// multiple-precision multiplication
BigNum& BigNum::mul(BigNum& result, const BigNum& a, const BigNum& b)
{
    assert(!result.storage || (result.storage != a.storage));
    assert(!result.storage || (result.storage != b.storage));
    // It is more efficient if the inner loop is the larger loop
    if (a.length > b.length) {
        return mul(result, b, a);
    }
    if (a.length == 1) {
        return mul(result, b, a.digits[0], a.neg);
    }
    result.reset(a.length + b.length, a.neg ^ b.neg);
    // multiple-precision multiplication
    uint32_t* r = result.digits;
    for (size_t i = 0; i < a.length; ++i, ++r) {
        uint64_t x = a.digits[i];
        const uint32_t* y = b.digits;
        uint32_t* s = r;
        uint64_t carry = 0;
        for (size_t j = 0; j < b.length; ++j) {
            uint64_t p = x * (uint64_t)*y++ + (uint64_t)*s + carry;
            *s++ = (uint32_t)p;
            carry = p >> 32;
        }
        *s = (uint32_t)carry;
    }
    return strip_lz(result);
}

// Multiplication operation
BigNum BigNum::operator*(const BigNum& n) const
{
    BigNum result;
    return mul(result, *this, n);
}

// Multiplication by an integer
BigNum BigNum::operator*(uint32_t i) const
{
    BigNum result;
    return mul(result, *this, i, false);
}

BigNum BigNum::div(const BigNum& divisor, BigNum& rem) const
{
    BigNum x = this->abs();
    BigNum y = divisor.abs();

    switch (compare(x, y)) {
    case -1:
        rem = *this;
        return 0;

    case 0:
        rem = zero;
        return 1;

    default:
        break;
    }

    size_t n = x.length - 1;
    size_t t = y.length - 1;
    size_t d = n - t;

    // Allocate one larger in case normalization adds a digit.
    BigNum q(d + 2, neg ^ divisor.neg);

    // Special case single digit divisor
    if (t == 0) {
        uint32_t Y = y.digits[0];
        uint64_t carry = 0;
        q.length = n + 1;
        do {
            uint64_t X = (uint64_t)x.digits[n] + (carry << 32);
            uint64_t d = X / Y;
            carry = X - d * Y;
            q.digits[n] = (uint32_t)d;
        } while (n--);
        rem = (uint32_t)carry;
        // Remainder is negative if dividend is negative
        rem.neg = neg && (rem != 0);
        return strip_lz(q);
    }
    // Special case small number of digits
    if (n < 2) {
        BigNum r(2);
        uint64_t X = x.digits[0];
        uint64_t Y = y.digits[0];
        if (n == 1) {
            X += ((uint64_t)x.digits[1] << 32);
        }
        if (t == 1) {
            Y += ((uint64_t)y.digits[1] << 32);
        }
        uint64_t Q = X / Y;
        uint64_t R = X - Y * Q;
        q.digits[0] = (uint32_t)Q;
        q.length = 1;
        Q >>= 32;
        if (Q) {
            q.digits[1] = (uint32_t)Q;
            ++q.length;
        }
        r.digits[0] = (uint32_t)R;
        r.length = 1;
        R >>= 32;
        if (R) {
            r.digits[1] = (uint32_t)R;
            ++r.length;
        }
        rem = r;
        // Remainder is negative if dividend is negative
        rem.neg = neg && (rem != 0);
        return strip_lz(q);
    }

    // Algorithm operates in-place so we need to operate on a clone
    x = x.clone();

    // Normalize divisor. In this case means left-shifting so the most significant digit of divisor is >= 2^31
    uint32_t norm = 31 - log2(y.digits[t]);
    if (norm) {
        x <<= norm;
        y <<= norm;
        // left shift may have added a digit to x
        n = x.length - 1;
        d = n - t;
    }
    q.length = std::max((size_t)1, d);

    // Cheap right shift of x rather than more expensive left shift of y
    x.digits += d;
    x.length -= d;
    while (x >= y) {
        q.length = d + 1;
        ++q.digits[d];
        x.sub(y);
    }
    x.digits -= d;
    x.length += d;

    // y[t]b + y[t-1] - i.e. the two most significant digits of y
    BigNum y2;
    y2.digits = &y.digits[t - 1];
    y2.length = 2;

    // BigNum to hold running pointer to the 3 most significant digits of x - we know there are at
    // least three because we special-cased 1 and 2 digits above.
    BigNum xm3; xm3.length = 3;

    // Temporary for products
    BigNum prod;

    // most significant digit of y
    const uint32_t ymsd = y.digits[t];

    for (size_t i = n; i > t; --i) {
        // d tracks the quotient digit index == (i - t - 1)
        --d;
        uint32_t qdigit;
        // Estimate quotient digit
        if (x.digits[i] == ymsd) {
            qdigit = 0xFFFFFFFF;
        } else {
            uint64_t z = ((uint64_t)x.digits[i] << 32) + x.digits[i - 1];
            qdigit = (uint32_t)(z / ymsd);
        }
        // Adjust estimate
        xm3.digits = &x.digits[i - 2];
        while (mul(prod, y2, qdigit, false) > xm3) {
            --qdigit;
        }
        // Compute quotient digit
        mul(prod, y, qdigit, false);
        // Again cheap right shift of x rather than more expensive left shift of y
        x.digits += d;
        x.length -= d;
        x.sub(prod);
        if (x < 0) {
            x += y;
            --qdigit;
        }
        x.digits -= d;
        x.length += d;
        q.digits[d] = qdigit;
    }
    if (norm) {
        x >>= norm;
    }
    rem = strip_lz(x);
    // Remainder is negative if dividend is negative
    rem.neg = neg && (rem != 0);
    return strip_lz(q);
}

// Division operation
BigNum BigNum::operator/(const BigNum& n) const
{
    BigNum rem;
    return div(n, rem);
}

// Division by an integer
BigNum BigNum::operator/(uint32_t i) const
{
    // TODO - check for power of 2 and shift
    assert(i != 0);
    BigNum n;
    BigNum rem;
    n.digits = &i;
    n.length = 1;
    return div(n, rem);
}

// Modulus operation
BigNum BigNum::operator%(const BigNum& mod) const
{
    BigNum rem;
    div(mod, rem);
    return rem;
}

BigNum& BigNum::right_shift(BigNum& result, const BigNum& n, uint32_t shift)
{
    if (shift == 0) {
        result = n;
    }
    size_t shift32 = shift >> 5;
    if (n.length > shift32) {
        BigNum t = n;
        strip_lz(t);
        size_t len = t.length - shift32;
        shift &= 0x1F;
        result.reset(len, t.neg, false);
        if (shift == 0) {
            // Shift is multiple of 32 bits
            memmove(result.digits, t.digits + shift32, len * sizeof(uint32_t));
        } else {
            uint32_t* x = result.digits + len;
            const uint32_t* y = t.digits + t.length;
            uint32_t ext = 0;
            for (size_t i = 0; i < len; ++i) {
                uint32_t v = *--y;
                *--x = ext | (v >> shift);
                ext = v << (32 - shift);
            }
        }
        return strip_lz(result);
    } else {
        return result.reset(1, false, true);
    }
}

// Right-shift operator
BigNum BigNum::operator>>(uint32_t shift) const
{
    BigNum result;
    return right_shift(result, *this, shift);
}

// In-place right-shift operator
BigNum& BigNum::operator>>=(uint32_t shift)
{
    return right_shift(*this, *this, shift);
}

// Left-shift operator
BigNum BigNum::operator<<(uint32_t shift) const
{
    if (shift == 0) {
        return *this;
    }
    size_t shift32 = shift >> 5;
    BigNum result(length + shift32 + 1, neg);
    shift &= 0x1F;
    if (shift == 0) {
        // Shift is multiple of 32 bits
        memcpy(result.digits + shift32, digits, length * sizeof(uint32_t));
        result.length = length + shift32;
    } else {
        uint32_t* x = result.digits + shift32;
        const uint32_t* y = digits;
        uint32_t ext = 0;
        for (size_t i = 0; i < length; ++i) {
            uint64_t v = (uint64_t)(*y++) << shift;
            *x++ = (uint32_t)v | ext;
            ext = (uint32_t)(v >> 32);
        }
        *x = ext;
    }
    return strip_lz(result);
}

// In-place left-shift operator
BigNum& BigNum::operator<<=(uint32_t shift)
{
    return *this = *this << shift;
}

size_t BigNum::bit_len() const
{
    size_t len = length;

    while (!digits[--len]) {
        if (len == 0) {
            return 0;
        }
    }
    return len * 32 + 1 + log2(digits[len]);
}

// Exponentiation
BigNum BigNum::exp(const BigNum& e) const
{
    BigNum a = 1;
    BigNum x = *this;

    BigNum tmp1;
    BigNum tmp2;

    size_t i = e.bit_len();
    while (i) {
        a = mul(tmp1, a, a);
        if (e.test_bit(--i)) {
            a = mul(tmp2, a, x);
        } else {
            tmp1 = tmp2;
            tmp2 = a;
        }
    }
    return strip_lz(a);
}

// Modular exponentiation
BigNum BigNum::mod_exp(const BigNum& e, const BigNum& m) const
{
    BigNum x = *this;

    if (x.length > m.length) {
        x = x % m;
    }
    if (m.is_odd()) {
        return x.monty_mod_exp(e, m);
    } else {
        BigNum a = 1;

        BigNum tmp1;
        BigNum tmp2;

        size_t i = e.bit_len();
        while (i) {
            a = mul(tmp1, a, a) % m;
            if (e.test_bit(--i)) {
                a = mul(tmp2, a, x) % m;
            } else {
                tmp1 = tmp2;
                tmp2 = a;
            }
        }
        return strip_lz(a);
    }
}

// Modular inverse
BigNum BigNum::mod_inv(const BigNum& mod) const
{
    BigNum u = *this;

    BigNum inv;
    BigNum u1(1);
    BigNum u3(u);
    BigNum v1(0);
    BigNum v3(mod);
    BigNum t1;
    BigNum t3;
    BigNum q;

    int iter = 1;

    while (v3 != 0) {
        q = u3 / v3;
        t3 = u3 % v3;
        t1 = u1 + q * v1;

        u1 = v1;
        v1 = t1;
        u3 = v3;
        v3 = t3;
        iter = -iter;
    }

    if (u3 != 1) {
        return 0;  // error, there is no inverse

    }
    if (iter < 0) {
        inv = mod - u1;
    } else {
        inv = u1;
    }

    return inv;
}

int BigNum::compare(const BigNum& a, const BigNum& b)
{
    // strip leading zeroes
    size_t aLen = a.length;
    const uint32_t* aDigits = &a.digits[aLen];
    while (*--aDigits == 0 && --aLen) {
    }
    ;
    // strip leading zeroes
    size_t bLen = b.length;
    const uint32_t* bDigits = &b.digits[bLen];
    while (*--bDigits == 0 && --bLen) {
    }
    ;
    // Compare signs taking into account zero is always +ve
    if ((a.neg && aLen) != (b.neg && bLen)) {
        return a.neg ? -1 : 1;
    }
    if (aLen == bLen) {
        while (aLen && (*aDigits == *bDigits)) {
            --aDigits;
            --bDigits;
            --aLen;
        }
        if (aLen == 0) {
            return 0;
        } else {
            return (*aDigits > *bDigits) ? 1 : -1;
        }
    }
    return (aLen > bLen) ? 1 : -1;
}

// Inplace subtraction. Assumes that the caller is handling the sign of the result.
BigNum& BigNum::sub(const BigNum& n, size_t shift)
{
    assert(this->abs() >= n.abs());
    size_t len = 0;
    uint64_t borrow = 0;
    uint32_t* l = digits + shift;
    const uint32_t* r = n.digits;

    while (len < n.length) {
        uint64_t n = (uint64_t)*l - (uint64_t)*r++ - borrow;
        borrow = n >> 63;
        *l++ = (uint32_t)n;
        ++len;
    }
    // Continue propogating borrow
    while (borrow) {
        uint64_t n = (uint64_t)*l - borrow;
        borrow = n >> 63;
        *l++ = (uint32_t)n;
    }
    strip_lz(*this);
    return *this;
}

// Count the trailing zeroes
uint32_t BigNum::num_trailing_zeroes() const
{
    // byte-wise lookup table
    static const uint8_t tz[] = {
        8, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
    };
    uint32_t z = 0;

    for (size_t i = 0; i < length; ++i) {
        uint32_t x = digits[i];
        for (size_t j = 0; j < 4; ++j, x >>= 8) {
            uint32_t zx = tz[x & 0xFF];
            z += zx;
            if (zx < 8) {
                return z;
            }
        }
    }
    return 0;
}

static uint32_t monty_rho(uint64_t b)
{
    // Modular inversion not defined for even values
    if (!(b & 1)) {
        return 0;
    }
    uint64_t x = (((b + 2) & 4) << 1) + b;
    x *= 2 - b * x;
    x *= 2 - b * x;
    x *= 2 - b * x;
    return (uint32_t)-x;
}

BigNum& BigNum::monty_mul(BigNum& r, const BigNum& n, const BigNum& m, uint32_t rho) const
{
    size_t len = m.length;
    assert(length <= len);
    assert(n.length <= len);

    // Zero pad inputs if needed
    BigNum x = (length < len) ? clone(len - length) : *this;
    BigNum y = (n.length < len) ? n.clone(len - n.length) : n;

    r.reset(len + 1);
    // This avoids a final shift right
    ++r.digits;
    for (size_t i = 0; i < len; ++i) {
        uint64_t X = x.digits[i];
        uint32_t u = (r.digits[0] + X * y.digits[0]) * rho;
        uint64_t carry = 0;
        uint64_t R;
        uint32_t* rd = r.digits;
        for (size_t j = 0; j < len; ++j, ++rd) {
            uint64_t Y = X * y.digits[j];
            uint64_t M = (uint64_t)u * m.digits[j];
            R = (uint64_t)*rd + carry + (uint32_t)Y + (uint32_t)M;
            carry = (R >> 32) + (Y >> 32) + (M >> 32);
            *(rd - 1) = (uint32_t)R;
        }
        R = (uint64_t)*rd + carry;
        carry = (R >> 32);
        *(rd - 1) = (uint32_t)R;
        *rd = (uint32_t)carry;
    }

    if (r >= m) {
        r.sub(m);
    }
    return strip_lz(r);
}

// Modular exponentiation using Montgomery multiplication
BigNum BigNum::monty_mod_exp(const BigNum& e, const BigNum& m) const
{
    assert(m.is_odd());
    uint32_t rho = monty_rho(m.digits[0]);

    BigNum R = 1;
    BigNum RR = 1;

    R.reset(m.length + 1);
    R.msdigit() = 1;
    RR.reset(m.length * 2 + 1);
    RR.msdigit() = 1;

    BigNum x;
    BigNum a = R % m;

    // Convert to Montgomery domain
    this->monty_mul(x, RR % m, m, rho);

    BigNum tmp1;
    BigNum tmp2;

    size_t i = e.bit_len();
    while (i) {
        a = a.monty_mul(tmp1, a, m, rho);
        if (e.test_bit(--i)) {
            a = a.monty_mul(tmp2, x, m, rho);
        } else {
            tmp1 = tmp2;
            tmp2 = a;
        }
    }
    BigNum r;
    return a.monty_mul(r, 1, m, rho);
}
