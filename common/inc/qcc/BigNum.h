#ifndef _BIGNUM_H
#define _BIGNUM_H
/**
 * @file
 *
 * This file implements an arbitrary precision (big number) arithmetic class.
 */


/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#include <qcc/String.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BENCHMARKING

namespace qcc {

// This class implements arbitrary precision arithmetic.
class BigNum {
  public:

    // Default constructor - initializes the BigNum to zero
    BigNum() : digits((uint32_t*) &zero_digit), length(1), neg(false), storage(NULL) { }

    // Constructor that initializes a BigNum from a small integer value.
    BigNum(uint32_t v);

    // Copy constructor
    BigNum(const BigNum& other);

    // Assignment operator
    BigNum& operator=(const BigNum& other);

    // The constant value zero
    static const BigNum zero;

    // Generate a crypographically random number.
    //
    // @param len  The length of the number in bytes.
    void gen_rand(size_t len);

    // Set value from a hexadecimal string
    //@returns false if number was not a hex string
    bool set_hex(const qcc::String& number);

    // Set a (positive) value from a byte array in big-endian order.
    // Use the negation operator to make the number negative.
    //
    // @param data  The data to set
    // @param len   The length of the data
    void set_bytes(const uint8_t* data, size_t len);

    // Set value from a decimal string
    //@returns false if number was not a decimal string
    bool set_dec(const qcc::String& number);

    // Render the value as a hexadecimal string
    qcc::String get_hex(bool toLower = false) const;

    // Render the value as bytes in big-endian order. The value is optionally zero padded (most
    // significant bits) if the BigNum value is smaller than the length of buffer.
    //
    // @param data  The buffer to receive the data.
    // @param len   The length of the buffer
    // @param pad   If true leading zeroes are added to fill the buffer.
    //
    // @return The number of bytes gotten.
    size_t get_bytes(uint8_t* buffer, size_t len, bool pad = false) const;

    // Addition operation
    BigNum operator+(const BigNum& n) const;

    // Monadic addition
    BigNum& operator+=(const BigNum& n);

    // Add an integer to an BigNum returning a new BigNum
    BigNum operator+(uint32_t i) const;

    // Monadic addtion of an integer to an BigNum
    BigNum& operator+=(uint32_t i);

    // Subtraction operation
    BigNum operator-(const BigNum& n) const;

    // Monadic subtraction
    BigNum& operator-=(const BigNum& n);

    // Subtract an integer from an BigNum
    BigNum operator-(uint32_t i) const;

    // Mondadic subtraction of an integer from an BigNum
    BigNum& operator-=(uint32_t i);

    // Negation
    BigNum operator-() const;

    // Absolute value
    BigNum abs() const { return neg ? -(*this) : (*this); }

    // Multiplication operation
    BigNum operator*(const BigNum& n) const;

    // Multiplication by an integer
    BigNum operator*(uint32_t i) const;

    // Division operation
    //
    // @param n The divisor
    //
    // @return  The quotient after dividing this BigNum by the BigNum n.
    BigNum operator/(const BigNum& n) const;

    // Division by an integer
    //
    // @param i The divisor
    //
    // @return  The quotient after dividing this BigNum by i.
    //
    BigNum operator/(uint32_t i) const;

    // Modulus operation
    //
    // Reduce a bignum modulo the specified value. That is the remainder after dividing by m.
    //
    // @parm m  The modulus.
    //
    // @return  The remainder after division by the modulus
    BigNum operator%(const BigNum& m) const;

    // Exponentiation
    //
    // Raises a bignum to the specified power.
    //
    // @parm e  The exponent (power) to raise the number to
    //
    // @return  The new BigNum
    BigNum exp(const BigNum& e) const;

    // Less-than operator
    bool operator<(const BigNum& n) const { return compare(*this, n) < 0; }

    // Greater-than operator
    bool operator>(const BigNum& n) const { return compare(*this, n) > 0; }

    // Less-than-or-equal operator
    bool operator<=(const BigNum& n) const { return compare(*this, n) <= 0; }

    // Greater-than-or-equal operator
    bool operator>=(const BigNum& n) const { return compare(*this, n) >= 0; }

    // Equals operator
    bool operator==(const BigNum& n) const { return compare(*this, n) == 0; }

    // Not-equals operator
    bool operator!=(const BigNum& n) const { return compare(*this, n) != 0; }

    // Right-shift operator
    //
    // @param shift  The number of bits for the shift
    //
    BigNum operator>>(uint32_t shift) const;

    // In-place right-shift operator
    //
    // @param shift  The number of bits for the shift
    //
    // @return  Reference to this BigNum shifted right as specified
    BigNum& operator>>=(uint32_t shift);

    // Left-shift operator
    //
    // @param shift  The number of bits for the shift
    //
    BigNum operator<<(uint32_t shift) const;

    // In-place left-shift operator
    //
    // @param shift  The number of bits for the shift
    //
    // @return  Reference to this BigNum shifted left as specified
    BigNum& operator<<=(uint32_t shift);

    // Test if value is even
    bool is_even() const { return !(digits[0] & 1); }

    // Test if value is odd
    bool is_odd() const { return (digits[0] & 1); }

    // Modular exponentiation
    //
    // @param e   The exponent
    // @param mod The modulus
    BigNum mod_exp(const BigNum& e, const BigNum& mod) const;

    // Returns the bit length of this BigNum
    size_t bit_len() const;

    // Returns the byte (octet) length of this BigNum
    size_t byte_len() const { return (7 + bit_len()) / 8; }

    // Test if a specific bit is set.
    bool test_bit(size_t index) const {
        size_t d = index >> 5;
        return (d < length) && (digits[d] & (1 << (index & 0x1F)));
    }

    // Destructor
    ~BigNum();

  private:

    // Montgomery multiplication
    // @param r    Returns the Montgomery product
    // @param n    The multiplicand
    // @param m    The modulus
    // @param rho  The inverse modulus
    BigNum& monty_mul(BigNum& r, const BigNum& n, const BigNum& m, uint32_t rho) const;

    // Mongtomery modular exponentiation
    BigNum monty_mod_exp(const BigNum& n, const BigNum& mod) const;

    // Count the trailing zeroes
    uint32_t num_trailing_zeroes() const;

    // Private constructor that allocates but doesn't initialize storage
    BigNum(size_t len, bool neg);

    // Makes a copy of an BigNum optionally adding extra (zero'd) space
    BigNum clone(size_t extra = 0) const;

    // Extend length zero padding most significant digits
    BigNum& zero_ext(size_t size);

    // reset an BigNum allocating storage if required
    BigNum& reset(size_t len, bool neg = false, bool clear = true);

    // compare two BigNums. Returns -1, 0, or 1
    static int AJ_CALL compare(const BigNum& a, const BigNum& b);

    // Remove leading zeroes
    static BigNum& AJ_CALL strip_lz(BigNum& n) {
        while (n.msdigit() == 0) {
            if (n.length == 1) {
                n.neg = false;
                break;
            } else {
                --n.length;
            }
        }
        return n;
    }

    // Right shift
    static BigNum& AJ_CALL right_shift(BigNum& result, const BigNum& n, uint32_t shift);

    // Multiplication by an integer putting result into an existing BigNum
    static BigNum& AJ_CALL mul(BigNum& result, const BigNum& a, uint32_t b, bool neg);

    // Multiplication putting result into an existing BigNum
    static BigNum& AJ_CALL mul(BigNum& result, const BigNum& a, const BigNum& b);

    // Division with remainder
    BigNum div(const BigNum& y, BigNum& rem) const;

    // Returns reference to the most significant digit
    uint32_t& msdigit() const { return digits[length - 1]; }

    // Check if value has unsuppressed leading zeroes
    bool haslz() const { return length > 1 && digits[length - 1] == 0; }

    // Convenience function for temporary values that don't own storage
    BigNum& Set(uint32_t* newDigits, size_t digitsLength, bool negative = false) {
        this->digits = newDigits;
        this->length = digitsLength;
        this->neg = negative;
        this->storage = NULL;
        return *this;
    }

    // Inplace subtraction for use with temporary values only
    BigNum& sub(const BigNum& n, size_t shift = 0);

    // Pointer to the digits array
    uint32_t* digits;

    // Length of the digits array
    size_t length;

    // True if the value is negative
    bool neg;

    // Opaque type for storage
    class Storage;

    // Pointer to storage
    Storage* storage;

    // Shared zero value
    static uint32_t zero_digit;
};

}
#endif
