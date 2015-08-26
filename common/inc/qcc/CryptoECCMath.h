#ifndef CRYPTOECCMATH_H
#define CRYPTOECCMATH_H
/**
 * @file CryptoECCMath.h
 *
 * This file provide wrappers around ECC cryptographic algorithms.
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
#include <qcc/platform.h>
#include <qcc/CryptoECCOldEncoding.h>

#include <qcc/CryptoECCfp.h>

namespace qcc {

typedef enum { MOD_MODULUS = 0, MOD_ORDER } modulus_val_t;

static const int BIGLEN = ECC_BIGVAL_SZ;

struct ECCBigVal {
    uint32_t data[ECC_BIGVAL_SZ];
};

struct ECCAffinePoint {
    ECCBigVal x;
    ECCBigVal y;
    uint32_t infinity;
};

typedef ECCBigVal bigval_t;
typedef ECCAffinePoint affine_point_t;

static const size_t U32_BIGVAL_SZ = ECC_BIGVAL_SZ;
static const size_t U32_AFFINEPOINT_SZ = 2 * ECC_BIGVAL_SZ + 1;

#define m1 0xffffffffU

/* NOTE WELL! The Z component must always be precisely reduced. */
typedef struct {
    bigval_t X;
    bigval_t Y;
    bigval_t Z;
} jacobian_point_t;

static bigval_t const big_zero = { { 0, 0, 0, 0, 0, 0, 0 } };
static bigval_t const big_one = { { 1, 0, 0, 0, 0, 0, 0 } };

/*
 * These constants are the modulus, order and base point of the
 * NIST P256 as defined in the NIST elliptic curve standard.
 */

static bigval_t const modulusP256 = { { m1, m1, m1, 0, 0, 0, 1, m1, 0 } };

static bigval_t const orderP256 =
{ { 0xfc632551, 0xf3b9cac2, 0xa7179e84, 0xbce6faad,
    0xffffffff, 0xffffffff, 0x00000000, 0xffffffff,
    0x00000000 } };

static affine_point_t const baseP256 = {
    { { 0xd898c296, 0xf4a13945, 0x2deb33a0, 0x77037d81,
        0x63a440f2, 0xf8bce6e5, 0xe12c4247, 0x6b17d1f2 } },
    { { 0x37bf51f5, 0xcbb64068, 0x6b315ece, 0x2bce3357,
        0x7c0f9e16, 0x8ee7eb4a, 0xfe1a7f9b, 0x4fe342e2 } },
    B_FALSE
};

#define modulusP modulusP256
#define orderP orderP256
#define base_point baseP256

#define MSW (BIGLEN - 1)

#define big_is_negative(a) ((int32_t)(a)->data[MSW] < 0)

/*
 * The external function get_random_bytes is expected to be avaiable.
 * It must return 0 on success, and -1 on error.  Feel free to raname
 * this function, if necessary.
 */
int get_random_bytes(void* buf, int len);

void big_mpyP(bigval_t* tgt, bigval_t const* a, bigval_t const* b,
              modulus_val_t modselect);

int big_cmp(bigval_t const* a, bigval_t const* b);

void big_precise_reduce(bigval_t* tgt, bigval_t const* a, bigval_t const* modulus);

void big_add(bigval_t* tgt, bigval_t const* a, bigval_t const* b);

boolean_t big_is_zero(bigval_t const* a);

void big_divide(bigval_t* tgt, bigval_t const* num, bigval_t const* den,
                bigval_t const* modulus);

boolean_t in_curveP(affine_point_t const* P);

void pointMpyP(affine_point_t* tgt, bigval_t const* k, affine_point_t const* P);

void toJacobian(jacobian_point_t* tgt, affine_point_t const* a);

void toAffine(affine_point_t* tgt, jacobian_point_t const* a);

void pointAdd(jacobian_point_t* tgt, jacobian_point_t const* P,
              affine_point_t const* Q);

void binary_to_bigval(const void* src, bigval_t* tgt, size_t srclen);

void bigval_to_binary(bigval_t const* src, void* tgt, size_t tgtlen);

void digit256_to_bigval(digit256_tc src, bigval_t* dst);

bool bigval_to_digit256(const bigval_t* src, digit256_t dst);

bool ECDH_derive_pt(affine_point_t* tgt, bigval_t const* k, affine_point_t const* Q);

QStatus ECDH_generate(affine_point_t* P1, bigval_t* k);

QStatus Crypto_ECC_GenerateSharedSecret(const ECCPublicKey* peerPublicKey, const ECCPrivateKey* privateKey, ECCSecretOldEncoding* secret);

void U32ArrayToU8BeArray(const uint32_t* src, size_t len, uint8_t* dest);

void U8BeArrayToU32Array(const uint8_t* src, size_t len, uint32_t* dest);


} /*namespace qcc*/
#endif /*CRYPTOECCMATH_H*/
