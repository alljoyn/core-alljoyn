/**
 * @file  field_p256.c  Implementation of field arithmetic for ECC.
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

#include <Status.h>

#include <qcc/Util.h>      /* for byte swaping function(s) */
#include <qcc/CryptoECC.h>
#include <qcc/CryptoECCfp.h>
#include <qcc/CryptoECCMath.h>

#if defined(_WIN32) & defined(_umul128)
#include <intrin.h>
#pragma intrinsic(_umul128)
#endif

namespace qcc {

/*
 * This file contains modular multiplication (in constant time)
 * for the NIST prime P-256:
 *              2^256-2^224+2^192+2^96-1
 * on 64-bit platforms.
 *
 * based on the approach from
 *
 *     J. A. Solinas. Generalized Mersenne numbers.
 *     Technical Report CORR 99–39, Centre for Applied Cryptographic
 *     Research, University of Waterloo, 1999.
 *
 * but adapted to 64-bit limbs.
 *
 * Let C = \sum_{i=0}^{7} ci * 2^{64*i} be our product
 * such that the different ci are that 0 <= ci < 2^64.
 * We denote
 * s = c3 * 2^192 + c2 * 2^128 + c1 * 2^64 + c0
 * by s = ( c3, c2, c1, c0 ). The higher 32-bit parts of a
 * 64-bit limbs c is denoted by c_h and the lower 32-bit part
 * by c_\ell. We can convert Solinas' scheme to 64-bit platforms
 * as follows
 *
 * s1 = ( c3, c2, c1, c0 ),
 * s2 = ( c7, c6, c5_h||0, 0 )
 * s3 = ( 0||c7_h, c7_\ell||c6_h, c6_\ell||0, 0 ),
 * s4 = ( c7, 0, 0||c5_\ell, c4 )
 * s5 = ( c4_\ell||c6_h, c7, c6_h||c5_h, c5_\ell||c4_h ),
 * s6 = ( c5_\ell||c4_\ell, 0, 0||c6_h, c6_\ell||c5_h )
 * s7 = ( c5_h||c4_h, 0, c7, c6 ),
 * s8 = ( c6_\ell||0, c5_\ell||c4_h, c4_\ell||c7_h, c7_\ell||c6_h )
 * s9 = ( c6_h||0, c5, c4_h||0, c7 )
 * d = s_1+2s_2+2s_3+s_4+s_5 - (s_6+s_7+s_8+s_9)
 *
 * We prefer positive d and instead we compute
 * d = s_1+2s_2+2s_3+s_4+s_5 + 4*p256 - (s_6+s_7+s_8+s_9)
 * such that 0 <= d < 11*p256
 * Next we perform one additional reduction step and a conditional
 * subtraction (in constant time) to ensure the result is between
 * zero and p256.
 */

/*
 * The constant P256_MODULUS is the integer 2^256 - 2^224 + 2^192 + 2^96 - 1,
 * the prime defining the field for the NIST curve P-256.  As defined in FIPS
 * PUB 186-4, "Digital Signature Standard (DSS)", Appendix D "Recommended
 * Elliptic Curves for Federal Government Use", Subsection D.1.2.3 "Curve
 * P-256".
 * http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.186-4.pdf
 */
static digit256_tc P256_MODULUS = { 0xFFFFFFFFFFFFFFFFULL, 0x00000000FFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFF00000001ULL };

/* Macros to extract the lower and upper parts. */
#define getlow_tolow(x) ((x) & (digit_t) 0xFFFFFFFF)
#define getlow_tohigh(x) ((x) << (digit_t) 32)
#define gethigh_tohigh(x) ((x) & (digit_t) 0xFFFFFFFF00000000)
#define gethigh_tolow(x) ((x) >> (digit_t) 32)

/* Is x != 0? */
boolean_t is_digit_nonzero_ct(digit_t x)
{
    return (boolean_t)((x | (0 - x)) >> (RADIX_BITS - 1));
}

/* Is x == 0? */
boolean_t is_digit_zero_ct(digit_t x)
{
    return (boolean_t)(1 ^ is_digit_nonzero_ct(x));
}

static inline unsigned char is_digit_lessthan_ct(digit_t x, digit_t y)
{
    /* Look at the high bit of x, y and (x - y) to determine whether x < y. */
    return (unsigned char)((x ^ ((x ^ y) | ((x - y) ^ y))) >> (RADIX_BITS - 1));
}

/* Software macros for addition, may be substituted if intrinsics are available
 * on a given platform. */
#define add3(c0, c1, c2, c3, a0, a1, a2, b0, b1, b2) \
    {                           \
        digit_t _t;             \
        ADDC(_t, c0, a0, b0, 0);   \
        ADDC(_t, c1, a1, b1, _t);  \
        ADDC(c3, c2, a2, b2, _t);  \
    }

#define add2_ncout(c0, c1, a0, a1, b0, b1) \
    {                           \
        digit_t _t;             \
        ADDC(_t, c0, a0, b0, 0);   \
        ADDC(_t, c1, a1, b1, _t);  \
    }

#define add3_ncout(c0, c1, c2, a0, a1, a2, b0, b1, b2) \
    {                           \
        digit_t _t;             \
        ADDC(_t, c0, a0, b0, 0);   \
        ADDC(_t, c1, a1, b1, _t);  \
        ADDC(_t, c2, a2, b2, _t);  \
    }

#define add4_ncout(c0, c1, c2, c3, a0, a1, a2, a3, b0, b1, b2, b3) \
    {                           \
        digit_t _t;             \
        ADDC(_t, c0, a0, b0, 0);   \
        ADDC(_t, c1, a1, b1, _t);  \
        ADDC(_t, c2, a2, b2, _t);  \
        ADDC(_t, c3, a3, b3, _t);  \
    }

#define add4(c0, c1, c2, c3, c4, a0, a1, a2, a3, b0, b1, b2, b3) \
    {                           \
        digit_t _t;             \
        ADDC(_t, c0, a0, b0, 0);   \
        ADDC(_t, c1, a1, b1, _t);  \
        ADDC(_t, c2, a2, b2, _t);  \
        ADDC(c4, c3, a3, b3, _t);  \
    }

#define add5_ncout(c0, c1, c2, c3, c4, a0, a1, a2, a3, a4, b0, b1, b2, b3, b4) \
    {                           \
        digit_t _t;             \
        ADDC(_t, c0, a0, b0, 0);   \
        ADDC(_t, c1, a1, b1, _t);  \
        ADDC(_t, c2, a2, b2, _t);  \
        ADDC(_t, c3, a3, b3, _t);  \
        ADDC(_t, c4, a4, b4, _t);  \
    }

#define sub4_nborrow(c0, c1, c2, c3, a0, a1, a2, a3, b0, b1, b2, b3) \
    {                           \
        digit_t _t;             \
        SUBC(_t, c0, a0, b0, 0);   \
        SUBC(_t, c1, a1, b1, _t);  \
        SUBC(_t, c2, a2, b2, _t);  \
        SUBC(_t, c3, a3, b3, _t);  \
    }

#define sub5_nborrow(c0, c1, c2, c3, c4, a0, a1, a2, a3, a4, b0, b1, b2, b3, b4) \
    {                           \
        digit_t _t;             \
        SUBC(_t, c0, a0, b0, 0);   \
        SUBC(_t, c1, a1, b1, _t);  \
        SUBC(_t, c2, a2, b2, _t);  \
        SUBC(_t, c3, a3, b3, _t);  \
        SUBC(_t, c4, a4, b4, _t);  \
    }

#define sub5(B, c0, c1, c2, c3, c4, a0, a1, a2, a3, a4, b0, b1, b2, b3, b4) \
    {                           \
        digit_t _t;             \
        SUBC(_t, c0, a0, b0, 0);   \
        SUBC(_t, c1, a1, b1, _t);  \
        SUBC(_t, c2, a2, b2, _t);  \
        SUBC(_t, c3, a3, b3, _t);  \
        SUBC(B, c4, a4, b4, _t);   \
    }

/******* Intrinsics/macros for Fp implementation *********/

/* 64x64 -> 128
 * returns the high 64-bits of the product
 * Software implementation of the _umul128 intrinsic, for when it's not present.
 * stdint.h guarantees we have uint64_t.
 */
uint64_t software_umul128(uint64_t u, uint64_t v, uint64_t* high)
{
    uint64_t u1, v1, t, w3, k, w1;

    u1 = (u & 0xFFFFFFFF);
    v1 = (v & 0xFFFFFFFF);
    t = (u1 * v1);
    u >>= 32;

    w3 = (t & 0xFFFFFFFF);
    k = (t >> 32);

    t = (u * v1) + k;
    v >>= 32;
    k = (t & 0xFFFFFFFF);
    w1 = (t >> 32);

    t = (u1 * v) + k;
    v1 = (u * v);
    k = (t >> 32);
    (*high) = v1 + k + w1;

    return (t << 32) + w3;
}

#ifndef _umul128
#define _umul128 software_umul128
#endif

#if RADIX_BITS != 64
#error Unexpected radix bits; expecting 64.
#endif

/*
 * Macros to encapsulate intrinsics when and if they are defined.
 */

/* 64 x 64 --> 128-bit multiplication
 * (c1,c0) = a * b
 */
#define mul(c0, c1, a, b) \
    { \
        c0 = _umul128(a, b, &c1); \
    }

/* Multiply-and-accumulate
 * (c1,c0) = a*b+c0
 */
#define muladd(c0, c1, a, b) { \
        digit_t _c = c0; \
        mul(c0, c1, a, b); \
        ADD(_c, c0, c0, _c); \
        c1 += _c; \
}

/* Multiply-and-accumulate-accumulate
 * (c1,c0) = a*b+c0+c1
 */
#define muladdadd(c0, c1, a, b) { \
        digit_t _C0 = c0, _C1 = c1, carry; \
        mul(c0, c1, a, b); \
        ADD(carry, c0, c0, _C0); \
        ADDC(carry, c1, c1, 0, carry); \
        ADD(carry, c0, c0, _C1); \
        ADDC(carry, c1, c1, 0, carry); \
}

/* Adds two operands, and produces sum of the two inputs and the carry bit.
 * This is ideally an intrinsic, but is emulated when an intrinsic is not available. */
#define ADD(carryOut, sumOut, addend1, addend2) { \
        digit_t sum = (addend1) + (addend2); \
        carryOut = (digit_t)is_digit_lessthan_ct(sum, (addend1)); \
        (sumOut) = sum; }

/* Adds two operands and a carry bit, produces sum of the three inputs and the carry bit.
 * This is ideally an intrinsic, but is emulated when an intrinsic is not available.*/
#pragma warning(suppress:4296)
#define ADDC(carryOut, sumOut, addend1, addend2, carryIn) { \
        digit_t tempReg = (addend1) + (digit_t)(carryIn); \
        (sumOut) = (addend2) + tempReg; \
        (carryOut) = (is_digit_lessthan_ct(tempReg, (digit_t)(carryIn)) | is_digit_lessthan_ct((sumOut), tempReg)); }

/* Subtract operation.
 * Subtract one number from the other, and return the difference and the borrow (carry) bit. */
#define SUB(borrowOut, differenceOut, minuend, subtrahend) { \
        (borrowOut) = (digit_t)is_digit_lessthan_ct((minuend), (subtrahend)); \
        (differenceOut) = (minuend) - (subtrahend); }

/* Subtraction with borrow (carry).
 * Subtract one number from the other with borrow (carry), and return the difference and the borrow (carry) bit.*/
#define SUBC(borrowOut, differenceOut, minuend, subtrahend, borrowIn) { \
        digit_t tempReg = (minuend) - (subtrahend); \
        digit_t borrowReg = ((digit_t)is_digit_lessthan_ct((minuend), (subtrahend)) | ((borrowIn) & is_digit_zero_ct(tempReg))); \
        (differenceOut) = tempReg - (digit_t)(borrowIn); \
        (borrowOut) = borrowReg; }

/* Move if carry is set. */
#define CMOVC(dest, src, selector) { \
        digit_t mask = (digit_t)is_digit_nonzero_ct(selector) - 1; \
        (dest) = ((~mask) & (src)) | (mask & (dest)); }

/* Zero of a 256-bit field element, a = 0 */
void fpzero_p256(digit256_t a)
{
    QCC_ASSERT(a != NULL);

    ClearMemory((void*)a, sizeof(digit256_t));
}

/* Is a = 0? (as integers, not mod P256)   */
bool fpiszero_p256(digit256_t a)
{
    size_t i;
    digit_t c;

    QCC_ASSERT(a != NULL);

    c = a[0];
    for (i = 1; i < P256_DIGITS; i++) {
        c = c | a[i];
    }

    return is_digit_zero_ct(c);
}

boolean_t validate_256(digit256_tc a, digit256_tc modulus)
{
    boolean_t valid = B_FALSE;
    digit256_t t1;

    QCC_ASSERT(a != NULL);
    QCC_ASSERT(modulus != NULL);

    QCC_UNUSED(modulus);

    fpcopy_p256(a, t1);
    valid = fpneg_p256(t1);                     /* valid = B_TRUE if a <= modulus  */
    valid = (boolean_t)(valid & (fpiszero_p256(t1) ^ 1));  /* valid = B_TRUE if a < modulus (use & instead of && for constant-time execution) */

    /* cleanup  */
    fpzero_p256(t1);

    return valid;
}

/* Validate that a 256-bit value is in [0, P256_MODULUS-1]
 * Returns = True if 0 <= a < modulus, else returns False.*/
boolean_t fpvalidate_p256(digit256_tc a)
{
    QCC_ASSERT(a != NULL);
    return validate_256(a, P256_MODULUS);
}

/* Set a = p256 (the prime that defines this finite field). */
void fpgetprime_p256(digit256_t a)
{
    QCC_ASSERT(a != NULL);
    memcpy(a, P256_MODULUS, sizeof(P256_MODULUS));
}

/* Compute c = a * b for 256-bit a and b
 * Private function used to implement fpmul_p256. */
static void mul_p256(
    digit256_tc a,
    digit256_tc b,
    digit_t* c)             /* Note this must have size at least 2*P256_DIGITS */
{
    digit_t A, t;

    QCC_ASSERT(a != NULL);
    QCC_ASSERT(b != NULL);
    QCC_ASSERT(c != NULL);

    A = a[0];
    mul(c[0], c[1], A, b[0]);
    muladd(c[1], c[2], A, b[1]);
    muladd(c[2], c[3], A, b[2]);
    muladd(c[3], c[4], A, b[3]);

    A = a[1];
    muladd(c[1 + 0], t, A, b[0]);
    muladdadd(c[1 + 1], t, A, b[1]);
    muladdadd(c[1 + 2], t, A, b[2]);
    muladdadd(c[1 + 3], t, A, b[3]);
    c[1 + 4] = t;

    A = a[2];
    muladd(c[2 + 0], t, A, b[0]);
    muladdadd(c[2 + 1], t, A, b[1]);
    muladdadd(c[2 + 2], t, A, b[2]);
    muladdadd(c[2 + 3], t, A, b[3]);
    c[2 + 4] = t;

    A = a[3];
    muladd(c[3 + 0], t, A, b[0]);
    muladdadd(c[3 + 1], t, A, b[1]);
    muladdadd(c[3 + 2], t, A, b[2]);
    muladdadd(c[3 + 3], t, A, b[3]);
    c[3 + 4] = t;

}

/* Compute c = a mod 2^256-2^224+2^192+2^96-1
 * such that 0 <= c < 2^256-2^224+2^192+2^96-1
 * Private function used to implement fpmul_p256. */
static void reduce_p256(
    digit_t*   a,
    digit256_t c)
{
    /* 4*p = 0x 3 FFFFFFFC00000004 0000000000000000 00000003FFFFFFFF FFFFFFFFFFFFFFFC */
    digit_t c0, c1, c2, c3, c4, c5, c6, c7;
    digit_t r0, r1, r2, r3, r4;
    digit_t p0, p1, p2, p3, p4;
    digit_t q0, q1, q2, q3, q4;
    digit_t t;

    QCC_ASSERT(a != NULL);
    QCC_ASSERT(c != NULL);

    c0 = a[0]; c1 = a[1]; c2 = a[2]; c3 = a[3];
    c4 = a[4]; c5 = a[5]; c6 = a[6]; c7 = a[7];

    /* compute s2 + s2  = [r4, r3, r2, r1, 0] */
    t = gethigh_tohigh(c5);
    add3(r1, r2, r3, r4, t, c6, c7, t, c6, c7);

    /* compute s3 + s3  = [p3, p2, p1, 0] */
    p1 = getlow_tohigh(c6);
    p2 = (gethigh_tolow(c6) | getlow_tohigh(c7));
    p3 = gethigh_tolow(c7);
    add3_ncout(p1, p2, p3, p1, p2, p3, p1, p2, p3);

    /* Compute 2s_2 + 2s_3 = [p4, p3, p2, p1, 0] */
    add4_ncout(p1, p2, p3, p4, p1, p2, p3, 0, r1, r2, r3, r4);

    /* Compute s_1+s_4 = [q4, q3, q2, q1, q0] */
    q1 = getlow_tolow(c5);
    add4(q0, q1, q2, q3, q4, c0, c1, c2, c3, c4, q1, 0, c7);

    /* Compute s_1+s_4+2s_2+2s_3 = [q4,q3,q2,q1,q0] */
    add4_ncout(q1, q2, q3, q4, q1, q2, q3, q4, p1, p2, p3, p4);

    /* Compute s_1+s_4+2s_2+2s_3+s_5 = [q4,q3,q2,q1,q0] */
    p0 = (gethigh_tolow(c4) | getlow_tohigh(c5));
    p1 = (gethigh_tolow(c5) | gethigh_tohigh(c6));
    p3 = (gethigh_tolow(c6) | getlow_tohigh(c4));
    add4(q0, q1, q2, q3, p4, q0, q1, q2, q3, p0, p1, c7, p3);
    q4 += p4;

    /* Compute s_1+s_4+2s_2+2s_3+s_5+4p_256 = [q4,q3,q2,q1,q0] */
    add5_ncout(q0, q1, q2, q3, q4, q0, q1, q2, q3, q4, \
               0xFFFFFFFFFFFFFFFC, 0x3FFFFFFFF, 0x0, 0xFFFFFFFC00000004, 0x3);

    /* Compute s_6+s_7 = [p4,p3,p2,p1,p0] */
    t = (gethigh_tolow(c4) | gethigh_tohigh(c5));
    p0 = (gethigh_tolow(c5) | getlow_tohigh(c6));
    p1 = gethigh_tolow(c6);
    p2 = 0;
    p3 = (getlow_tolow(c4) | getlow_tohigh(c5));
    add4(p0, p1, p2, p3, p4, c6, c7, 0, t, p0, p1, p2, p3);


    /* Compute s_6+s_7+s_8 = [p4,p3,p2,p1,p0] */
    r0 = (gethigh_tolow(c6) | getlow_tohigh(c7));
    r1 = (gethigh_tolow(c7) | getlow_tohigh(c4));
    r2 = (gethigh_tolow(c4) | getlow_tohigh(c5));
    r3 = getlow_tohigh(c6);
    add4(p0, p1, p2, p3, r4, r0, r1, r2, r3, p0, p1, p2, p3);
    p4 += r4;

    /* Compute s_6+s_7+s_8+s_9 = [p4,p3,p2,p1,p0] */
    r1 = gethigh_tohigh(c4);
    r3 = gethigh_tohigh(c6);
    add4(p0, p1, p2, p3, r4, c7, r1, c5, r3, p0, p1, p2, p3);
    p4 += r4;

    /* Compute d = s_1+2s_2+2s_3+s_4+s_5 - (s_6+s_7+s_8+s_9) */
    sub5_nborrow(q0, q1, q2, q3, q4, q0, q1, q2, q3, q4, p0, p1, p2, p3, p4);

    /* [q0+q4], [q1-q4*2^32], [q2], [q3-d+d*2^32] */
    p4 = 0;
    add4(q0, q1, q2, q3, p4, q0, q1, q2, q3, q4, 0, 0, 0);
    p0 = (q4 << (digit_t)32);
    sub4_nborrow(q1, q2, q3, p4, q1, q2, q3, p4, p0, 0, 0, 0);
    p0 = p0 - q4;
    add2_ncout(q3, p4, q3, p4, p0, 0);

    /* Compute the conditional subtraction. */
    sub5(p0, c0, c1, c2, c3, c4, q0, q1, q2, q3, p4, \
         0xFFFFFFFFFFFFFFFF, 0x00000000FFFFFFFF, 0x0000000000000000, 0xFFFFFFFF00000001, 0);

    CMOVC(c0, q0, p0);
    CMOVC(c1, q1, p0);
    CMOVC(c2, q2, p0);
    CMOVC(c3, q3, p0);

    c[0] = c0;
    c[1] = c1;
    c[2] = c2;
    c[3] = c3;
}

void fpmul_p256(
    digit256_tc multiplier,
    digit256_tc multiplicand,
    digit256_t product,
    digit_t*  temps)
{
    QCC_ASSERT(multiplier != NULL);
    QCC_ASSERT(multiplicand != NULL);
    QCC_ASSERT(product != NULL);
    QCC_ASSERT(temps != NULL);

    mul_p256(multiplier, multiplicand, temps);
    reduce_p256(temps, product);
}

void fpsqr_p256(
    digit256_tc multiplier,
    digit256_t product,
    digit_t*    temps)
{
    QCC_ASSERT(multiplier != NULL);
    QCC_ASSERT(product != NULL);
    QCC_ASSERT(temps != NULL);

    fpmul_p256(multiplier, multiplier, product, temps);
}

void fpadd_p256(
    digit256_tc addend1,
    digit256_tc addend2,
    digit256_t pSum)
{
    digit_t carry = 0;
    digit_t borrow = 0;
    digit_t mask;

    QCC_ASSERT(addend1 != NULL);
    QCC_ASSERT(addend2 != NULL);
    QCC_ASSERT(pSum != NULL);

    /* (carry,sum) = a1 + a2    */
    ADD(carry, pSum[0], addend1[0], addend2[0]);
    ADDC(carry, pSum[1], addend1[1], addend2[1], carry);
    ADDC(carry, pSum[2], addend1[2], addend2[2], carry);
    ADDC(carry, pSum[3], addend1[3], addend2[3], carry);

    /* Constant time reduction: subtract the modulus, and move only if there is a carry out.
     * (borrow,sum) = (carry,sum) - P256_MODULUS */
    SUB(borrow, pSum[0], pSum[0], P256_MODULUS[0]);
    SUBC(borrow, pSum[1], pSum[1], P256_MODULUS[1], borrow);
    SUBC(borrow, pSum[2], pSum[2], P256_MODULUS[2], borrow);
    SUBC(borrow, pSum[3], pSum[3], P256_MODULUS[3], borrow);
    borrow = borrow ^ carry;        /* bit-level subtraction. Don't use - */

    /* both carry from addition and borrow can't happen, since a1<m and a2<m, and (a1+a2)>m => (a1+a2-m)>0. */
    QCC_ASSERT((carry & borrow) == 0);

    /* Conditional correction without conditional branches */
    /* If there is a borrow bit, revert the subtration by adding back the modulus.
     * 'mask' is either an all-zero or an all-one digit mask.
     * If there is no borrow, add an all-zero P256_MODULUS. Otherwise, add P256_MODULUS.
     * if (borrow) sum = sum + P256_MODULUS */
    mask = 0 - borrow;
    ADD(carry, pSum[0], pSum[0], mask & P256_MODULUS[0]);
    ADDC(carry, pSum[1], pSum[1], mask & P256_MODULUS[1], carry);
    ADDC(carry, pSum[2], pSum[2], mask & P256_MODULUS[2], carry);
    ADDC(carry, pSum[3], pSum[3], mask & P256_MODULUS[3], carry);

    /* If there was no effective addition (borrow=0,mask=0), then carry=0,
     * since adding zero does not produce a carry out.
     * If there was an addition (borrow=1,mask=~0), then carry=1
     * so that the high-order all-1 digits produced after the ill-conceived subtraction
     * would go back to being zero when added 1.*/
    QCC_ASSERT(carry == borrow);
}

void fpsub_p256(
    digit256_tc minuend,
    digit256_tc subtrahend,
    digit256_t pDifference)
{
    digit_t borrow = 0;
    digit_t carry = 0;
    digit_t mask = 0;

    QCC_ASSERT(minuend != NULL);
    QCC_ASSERT(subtrahend != NULL);
    QCC_ASSERT(pDifference != NULL);

    /* Constant time reduction: subtract the modulus, and move only if there is a carry out.
     * Trade-off: instead of conditional mode and using more memory, add the modulus back conditionally. */
    SUB(borrow, pDifference[0], minuend[0], subtrahend[0]);
    SUBC(borrow, pDifference[1], minuend[1], subtrahend[1], borrow);
    SUBC(borrow, pDifference[2], minuend[2], subtrahend[2], borrow);
    SUBC(borrow, pDifference[3], minuend[3], subtrahend[3], borrow);

    /* If there is a borrow bit, revert the subtration by adding back the modulus.
     * What does 'mask' do? It is either an all-zero or an all-one digit mask.
     * If there is no borrow, add an all-zero modulus. Otherwise, add modulus. */
    mask = 0 - borrow;
    ADD(carry, pDifference[0], pDifference[0], mask & P256_MODULUS[0]);
    ADDC(carry, pDifference[1], pDifference[1], mask & P256_MODULUS[1], carry);
    ADDC(carry, pDifference[2], pDifference[2], mask & P256_MODULUS[2], carry);
    ADDC(carry, pDifference[3], pDifference[3], mask & P256_MODULUS[3], carry);

    /* If there was no effective addition (borrow=0,mask=0), then carry=0,
     * since adding zero does not produce a carry out.
     * If there was an addition (borrow=1,mask=~0), then carry=1
     * so that the high-order all-1 digits produced after the ill-conceived subtraction
     * would go back to being zero when added 1.*/
    QCC_ASSERT(carry == borrow);
}

/* Negate a
 * If a <= modulus returns B_TRUE, else returns B_FALSE. */
boolean_t fpneg_p256(
    digit256_t a)
{
    digit_t i, res, carry = 0, borrow = 0;

    QCC_ASSERT(a != NULL);

    for (i = 0; i < P256_DIGITS; i++) {
        res = P256_MODULUS[i] - a[i];
        carry = is_digit_lessthan_ct(P256_MODULUS[i], a[i]);
        a[i] = res - borrow;
        borrow = carry | (borrow & is_digit_zero_ct(res));
    }

    return (boolean_t)(borrow ^ 1);
}

void fpdiv2_p256(
    digit256_tc numerator,
    digit256_t quotient,
    digit_t*    temps)
{
    /* Division by two is done by multiplication by 1/2. The constant "half" is 1/2 (mod P256): */
    digit256_tc half = { 0x0000000000000000ULL, 0x0000000080000000ULL, 0x8000000000000000ULL, 0x7FFFFFFF80000000ULL };

    QCC_ASSERT(numerator != NULL);
    QCC_ASSERT(quotient != NULL);
    QCC_ASSERT(temps != NULL);

    fpmul_p256(half, numerator, quotient, temps);
}

void fpcopy_p256(digit256_tc pSrc, digit256_t pDest)
{
    memcpy(pDest, pSrc, sizeof(digit256_t));
}

boolean_t fpequal_p256(digit256_tc f1, digit256_tc f2)
{
    digit_t temp = 0;
    boolean_t equal;

    QCC_ASSERT(f1 != NULL);
    QCC_ASSERT(f2 != NULL);

    temp |= (f1[0] ^ f2[0]);
    temp |= (f1[1] ^ f2[1]);
    temp |= (f1[2] ^ f2[2]);
    temp |= (f1[3] ^ f2[3]);

    equal = (boolean_t) (~((((sdigit_t)temp) >> (RADIX_BITS - 1)) | (-((sdigit_t)temp) >> (RADIX_BITS - 1))) & 1);

    return equal;
}

/* Set a to the single digit dig0. */
void fpset_p256(digit_t dig0, digit256_t a)
{
    QCC_ASSERT(a != NULL);

    memset(a, 0x00, P256_DIGITS * sizeof(digit_t));
    a[0] = dig0;
}

/* Exponentiation via square-and-multiply.  Not constant time, should not be used when e is private. */
static void fpexp_naive_p256(digit256_tc a, digit256_tc e, digit256_t out, digit_t* temps)
{
    size_t digIdx = 0, bitIdx = 0, bitI = 0;
    size_t i = 0;

    fpset_p256((digit_t)1, out);      /* out = 1; */
    for (i = 0; i < 256; i++) {
        digIdx = (255 - i) / RADIX_BITS;
        bitIdx = (255 - i) % RADIX_BITS;
        bitI = (e[digIdx] >> bitIdx) & 1;
        fpsqr_p256(out, out, temps);
        if (bitI) {
            fpmul_p256(out, a, out, temps);
        }
    }

}

void fpinv_p256(
    digit256_tc a,
    digit256_t inv,
    digit_t*    temps)
{
    /*
     * Inverse modulo P256 is done by exponentiation by (P256-2).
     * The constant P256m2 is P256_MODULUS - 2.
     */
    digit256_tc P256m2 = { 0xFFFFFFFFFFFFFFFDULL, 0x00000000FFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFF00000001ULL };
    fpexp_naive_p256(a, P256m2, inv, temps);
}

/* Swaps the byte order of the digits in a. The order of digits is not changed.
 * i.e., a[i] = ByteSwap(a[i]). */
void fpdigitswap_p256(digit256_t a)
{
    size_t i;

    QCC_ASSERT(sizeof(digit_t) == 8);

    for (i = 0; i < P256_DIGITS; i++) {
        a[i] = EndianSwap64(a[i]);
    }
}

/* Reverse a byte array. */
static void reverse(uint8_t* in, size_t inlen)
{
    size_t i = 0;

    for (i = 0; i < inlen / 2; i++) {
        SWAP(in[i], in[inlen - 1 - i]);
    }
}

/* Create a field element x from a byte string.
 * Input buffer must have length sizeof(digit256_t)
 * Inputs larger than P256 will be reduced mod P256. */
void fpimport_p256(const uint8_t* bytes, digit256_t x, digit_t* temps, bool is_bigendian)
{
    digit256_t tmp;

    memcpy(x, bytes, sizeof(digit256_t));
    if (is_bigendian) {                 /* Input is a big endian octect string. */
        reverse((uint8_t*)x, sizeof(digit256_t));
    }

    fpset_p256(1, tmp);                 /* tmp = 1 */
    fpmul_p256(x, tmp, x, temps);       /* x = tmp*x = x mod P256 */
    fpzero_p256(tmp);

#if (QCC_TARGET_ENDIAN == QCC_BIG_ENDIAN)
    /* Convert words to machine's endianness. */
    fpdigitswap_p256(x);
#endif
}

}
