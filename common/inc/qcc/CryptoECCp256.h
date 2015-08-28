#ifndef AJ_CRYPTO_EC_P256_H
#define AJ_CRYPTO_EC_P256_H
/**
 * @file  aj_crypto_ec_p256.h  Implementation of curve arithmetic for ECC.
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
#include <alljoyn/Status.h>
#include <qcc/CryptoECCfp.h>
#include <qcc/CryptoECC.h>

namespace qcc {

/*
 * The algorithms used here to implement elliptic curve arithmetic are described in detail in
 *
 * Joppe W. Bos and Craig Costello and Patrick Longa and Michael Naehrig
 * "Selecting elliptic curves for cryptography: an efficiency and security analysis",
 * Journal of Cryptographic Engineering, 2015, http://eprint.iacr.org/2014/130
 *
 * and parts of this implementation are based on the associated implementation "MSR Elliptic Curve Cryptography Library",
 * available at http://research.microsoft.com/en-us/projects/nums/default.aspx.
 *
 * The above referenced paper gives a proof that the scalar multiplication algorithm implemented
 * here is exception-less, and has constant-time execution.
 */

/* These point types/formats are for the ECC/math implementation only, not the wider
 * AJ library. */


/* Point representation in Jacobian coordinates (X:Y:Z) such that x = X/Z^2, y = Y/Z^3.*/
typedef struct {
    digit256_t X;
    digit256_t Y;
    digit256_t Z;
} ecpoint_jacobian_t;

/* Point representation in affine coordinates (x,y). */
typedef struct {
    digit256_t x;
    digit256_t y;
} ecpoint_t;

/* Point representation in Chudnovsky coordinates (X:Y:Z:Z^2:Z^3) (used for precomputed points).*/
typedef struct {
    digit256_t X;
    digit256_t Y;
    digit256_t Z;
    digit256_t Z2;
    digit256_t Z3;
} ecpoint_chudnovsky_t;

/* An identifier for the curve.  This field may be serialized, so numbers should be re-used
 * for different curves between releases. */
typedef enum {
    NISTP256r1 = 1
} curveid_t;

/* Structure to hold curve related data. */
typedef struct {
    curveid_t curveid;          /* Curve ID */
    size_t rbits;               /* Bitlength of the order of the curve (sub)group */
    size_t pbits;               /* Bitlength of the prime */
    digit_t*  prime;            /* Prime */
    digit_t*  a;                /* Curve parameter a */
    digit_t*  b;                /* Curve parameter b */
    digit_t*  order;            /* Prime order of the curve (sub)group */
    ecpoint_t generator;
    digit_t*  rprime;           /* -(r^-1) mod 2^W */
} ec_t;


/* Functions */

/**
 * Get the curve data for the curve specified by curveid.
 *
 * @param[out] curve   The structure to get the curve data.
 * @param[in]  curveid The ID of the curve.
 *
 * @return AJ_OK if the curve was initialized correctly.
 *
 * @remarks
 *  If ec_getcurve succeeds, callers must call ec_freecurve to free memory
 *  allocated by ec_getcurve.
 */
QStatus ec_getcurve(ec_t* curve, curveid_t curveid);

/**
 * Free a curve initialized by ec_getcurve.
 *
 * @param[in,out] curve The curve to be freed.  If NULL, no action is taken.
 *
 */
void ec_freecurve(ec_t* curve);

/**
 * Get the generator (basepoint) associated with the curve, in affine (x,y) representation.
 *
 * @param[out] P     The point that will be set to the generator.
 * @param[in]  curve The curve.
 */
void ec_get_generator(ecpoint_t* P, ec_t* curve);

/**
 * Test whether the affine point P = (x,y) is the point at infinity (0,0).
 *
 * @param[in] P     The affine point to test.
 * @param[in] curve The curve associated with P.
 *
 * @return true if P is the point at infinity, false otherwise.
 */
boolean_t ec_is_infinity(const ecpoint_t* P, ec_t* curve);

/**
 * Test whether the Jacboian point P = (X:Y:Z) is the point at infinity (0:Y:0).
 *
 * @param[in] P      The affine point to test.
 * @param[in] curve  The curve associated with P.
 *
 * @return true if P is the point at infinity, false otherwise.
 */
boolean_t ec_is_infinity_jacobian(const ecpoint_jacobian_t* P, ec_t* curve);

/**
 * Convert a Jacobian point Q = (X:Y:Z) to an affine point P = (X/Z:Y/Z).
 *
 * @param[in]  Q      An affine point to be converted.
 * @param[out] P      The Jacobian representation of P.
 * @param[in]  curve  The curve that P and Q lie on.
 */
void ec_toaffine(ecpoint_jacobian_t* Q, ecpoint_t* P, ec_t* curve);

/**
 * Convert an affine point Q = (x,y) to a Jacobian point P = (X:Y:1), where X=x, Y=y.
 *
 * @param[in]  Q      An affine point to be converted.
 * @param[out] P      The Jacobian representation of P.
 * @param[in]  curve  The curve that P and Q lie on.
 */
void ec_affine_tojacobian(const ecpoint_t* Q, ecpoint_jacobian_t* P);

/**
 * Compute the scalar multiplication k*P.
 *
 * @param[in]  P     The point to be multiplied.
 * @param[in]  k     The scalar.
 * @param[out] Q     The output point Q = k*P.
 * @param[in]  curve The curve P is on.
 *
 * @return AJ_OK if succcessful
 */
QStatus ec_scalarmul(const ecpoint_t* P, digit256_t k, ecpoint_t* Q, ec_t* curve);

/**
 * Check that a point is on the given curve.
 *
 * @param[in] P     The point to check is on the curve.
 * @param[in] curve The curve P should be on.
 *
 * @return true if P is on the curve, false otherwise.
 */
boolean_t ec_oncurve(const ecpoint_t* P, ec_t* curve);

/**
 * Check that a point is valid.
 * Ensure that the x and y coordinates are in [0, p], that (x,y) is a point on
 * the curve, and that it is not the point at infinity.
 *
 * @param[in] P     The point to validated
 * @param[in] curve The curve P should be on.
 *
 * @return true if P is valid, false otherwise.
 */
boolean_t ecpoint_validation(const ecpoint_t* P, ec_t* curve);

/**
 *  Adds two affine points.
 *
 *  @param[in,out] P      The first point to be added, and to hold the result, i.e., P += Q.
 *  @param[in]     Q      The second point to be added.
 *  @param[in]     curve  The curve that P and Q lie on.
 */
void ec_add(ecpoint_t* P, const ecpoint_t* Q, ec_t* curve);

/* These are internal to the ECC code -- defined here for testing.  */
void ec_double_jacobian(ecpoint_jacobian_t* P);
void ec_add_jacobian(ecpoint_jacobian_t* Q, ecpoint_jacobian_t* P, ec_t* curve);

}

#endif /* AJ_CRYPTO_EC_P256_H */