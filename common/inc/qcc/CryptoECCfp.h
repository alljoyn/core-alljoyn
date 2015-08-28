#ifndef FIELD_256_H
#define FIELD_256_H
/**
 * @file  aj_crypto_fp.h  Header file for field arithmetic for ECC.
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

using namespace std;

namespace qcc {

typedef enum { B_FALSE, B_TRUE } boolean_t;

/* Digit types for multiprecision integers.*/
typedef uint64_t digit_t;
typedef const uint64_t digit_tc;
typedef int64_t sdigit_t;

/* Number of bits in the large integer radix, i.e., digits are from the set {0, ..., 2^(RADIX_BITS) - 1}. */
#define RADIX_BITS (64)

/* The zero digit_t. */
#define DIGIT_ZERO ((digit_t) 0)

/* Convert a bitlength to the number of digit_t's required to represent it. */
#define NBITS_TO_NDIGITS(x) (((x) + RADIX_BITS - 1) / (RADIX_BITS))

/* Number of temps required by field arithmetic functions */
#define P256_TEMPS (2 * P256_DIGITS)

/* Number of digits required to represent a field element. */
#define P256_DIGITS ((256 + RADIX_BITS - 1) / RADIX_BITS)

/* Swap two values of the same type. */
#ifdef __cplusplus
#define SWAP(a, b) std::swap(a, b)
#else
#define SWAP(a, b)  { \
        (a) = (a) ^ (b); \
        (b) = (a) ^ (b); \
        (a) = (a) ^ (b); \
}
#endif

/* Multiprecision type to represent 256-bit field elements */
typedef digit_t digit256_t[P256_DIGITS];
typedef const digit256_t digit256_tc;

/**
 * Add two field elements (modular addition).
 *
 * @param[in]  addend1  The first addend.
 * @param[in]  paddend1 The second addend.
 * @param[out] sum      The sum addend1 + paddend1 (mod p256).
 *
 */
void fpadd_p256(digit256_tc addend1, digit256_tc paddend1, digit256_t sum);

/**
 * Set a field element to the value zero.
 *
 * @param[in,out] a The value to be zeroed.
 *
 * @remarks
 *     This function uses a platform-specific secure zero function,
 *     to ensure it will not be optimized away.
 */
void fpzero_p256(digit256_t a);

/**
 * Test whether a field element is zero.
 *
 * @param[in] a The field element to test.
 *
 * @return TRUE if a is zero, and FALSE if a is nonzero.
 */
bool fpiszero_p256(digit256_t a);

/**
 * Get the value P256, the prime that defines the field.
 *
 * @param[out] a The field element that will be set to P256.
 *
 */
void fpgetprime_p256(digit256_t a);

/**
 * Test whether a 256-bit value is a valid element of the field defiend
 * by the prime P256.
 *
 * @param[in] a The field element to test.
 *
 * @return TRUE if a is in [0, P256-1], and FALSE otherwise.
 */
boolean_t fpvalidate_p256(digit256_tc a);

/**
 * Test whether a 256-bit value is in [0, modulus-1].
 *
 * @param[in] a The 256-bit value to test.
 *
 * @return TRUE if a is in [0, modulus-1], and FALSE otherwise.
 */
boolean_t validate_256(digit256_tc a, digit256_tc modulus);

/**
 * Test whether a digit is zero, in constant time.
 *
 * @param[in] x the digit to test.
 *
 * @return TRUE if x == 0, FALSE otherwise.
 */
boolean_t is_digit_zero_ct(digit_t x);

/**
 * Test whether a digit is nonzero, in constant time.
 *
 * @param[in] x the digit to test.
 *
 * @return TRUE if x != 0, FALSE otherwise.
 */
boolean_t is_digit_nonzero_ct(digit_t x);

/**
 * Field subtraction (modular subtraction).
 *
 * @param[in]  minuend      The field element to subtract from.
 * @param[in]  subtrahend   The field element to subtract.
 * @param[out] difference   The output difference,  minuend - subtrahend (mod p256).
 */
void fpsub_p256(digit256_tc minuend, digit256_tc subtrahend, digit256_t difference);

/**
 * Negate a field element.
 *
 * @param[in,out] a The field element to be negated.
 *
 * @return If a is less than or equal to modulus returns "1" (TRUE), else returns "0" (FALSE).
 */
boolean_t fpneg_p256(digit256_t a);

/**
 * Divide a field element by two.
 *
 * @param[in]  numerator The numerator in the division.
 * @param[out] quotient  The quotient: numerator/2 (mod p256).
 * @param[in,out] temps  Temporary space for use by this function, must have digit length P256_TEMPS.
 *
 */
void fpdiv2_p256(digit256_tc numerator, digit256_t quotient, digit_t* temps);

/**
 * Modular multiplication.
 *
 * @param[in]     multiplier   The multiplier.
 * @param[in]     multiplicand The multiplicand.
 * @param[out]    product      The product multiplier*multiplicand (mod p256).
 * @param[in,out] temps        Temporary space for use by this function, must have digit length P256_TEMPS.
 *
 */
void fpmul_p256(digit256_tc multiplier, digit256_tc multiplicand, digit256_t product, digit_t* temps);

/**
 * Modular squaring.
 *
 * @param[in]     multiplier The value to be squared.
 * @param[out]    product    The square mutiplier*multiplier (mod p256).
 * @param[in,out] temps      Temporary space for use by this function, must have digit length P256_TEMPS.
 *
 */
void fpsqr_p256(digit256_tc multiplier, digit256_t product, digit_t* temps);

/**
 * Copy one field element to another.
 *
 * @param[in]  src The source field element.
 * @param[out] dst The destination field element.
 */
void fpcopy_p256(digit256_tc src, digit256_t dst);

/**
 * Check whether two field elements are equal.
 *
 * @param[in] a The first field element to compare.
 * @param[in] b The second field element to compare.
 *
 * @return TRUE if the element two elements are equal, FALSE otherwise.
 *
 * @remarks
 * Note that all inputs must be fully reduced mod p256, e.g., p+1 and 1 will not be considered equal.
 * This should not be an issue since all outputs of this implementation are fully reduced.
 */
boolean_t fpequal_p256(digit256_tc a, digit256_tc b);

/**
 * Compute the multiplicative inverse of a field element.
 *
 * @param[in]     a     The element to be inverted.
 * @param[out]    inv   The output result 1/a (mod p256).
 * @param[in,out] temps Temporary space for use by this function, must have digit length P256_TEMPS.
 *
 */
void fpinv_p256(digit256_tc a, digit256_t inv, digit_t* temps);

/**
 * Set a field element to a single digit value.
 *
 * @param[in]  dig0   The value to assign.
 * @param[out] a      The field element to be assigned.
 *
 * @remarks For example, fpset_p256((digit_t)1, a) sets a to the value 1.
 */
void fpset_p256(digit_t dig0, digit256_t a);

/**
 * Swaps the byte order of the digits in a field element. The order of digits
 * is not changed.
 * i.e., fpdigitswap_p256(a) does a[i] = ByteSwap(a[i])
 *
 * @param[in,out] a  The field element to have its digits swapped
 */
void fpdigitswap_p256(digit256_t a);

/**
 * Create a field element x from a byte string.
 * Input buffer must have length sizeof(digit256_t)
 * Inputs larger than P256 will be reduced mod P256.
 *
 * @param[in]     bytes        The byte array to import.
 * @param[out]    x            The field element to create.
 * @param[in,out] temps        Temporary space for use by this function, must have digit length P256_TEMPS.
 * @param[in]     is_bigendian TRUE if bytes has big endian byte ordering, or FALSE if little endian ordering.
 *
 */
void fpimport_p256(const uint8_t* bytes, digit256_t x, digit_t* temps, bool is_bigendian);

} /*namespace qcc*/

#endif /* FIELD_P256_H */
