/**
 * @file CryptoECCOldEncoding.cc
 *
 * Class for ECC public/private key encryption with the Old encoding
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
#include <qcc/Util.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <qcc/Debug.h>
#include <qcc/CryptoECC.h>
#include <qcc/CryptoECCOldEncoding.h>
#include <qcc/Crypto.h>

#include <qcc/CryptoECCMath.h>

#include <Status.h>

using namespace std;

namespace qcc {

#define QCC_MODULE "CRYPTO"

#ifdef ECC_TEST
#define COND_STATIC
#else /* ECC_TEST not defined */
#define COND_STATIC static
#endif /* ECC_TEST not defined */


/* takes the point sent by the other party, and verifies that it is a
   valid point.  If 1 <= k < orderP and the point is valid, it stores
   the resuling point *tgt and returns B_TRUE.  If the point is invalid it
   returns B_FALSE.  The behavior with k out of range is unspecified,
   but safe. */

COND_STATIC boolean_t ECDH_derive_pt(affine_point_t* tgt, bigval_t const* k, affine_point_t const* Q)
{
    if (Q->infinity) {
        return (B_FALSE);
    }
    if (big_is_negative(&Q->x)) {
        return (B_FALSE);
    }
    if (big_cmp(&Q->x, &modulusP) >= 0) {
        return (B_FALSE);
    }
    if (big_is_negative(&Q->y)) {
        return (B_FALSE);
    }
    if (big_cmp(&Q->y, &modulusP) >= 0) {
        return (B_FALSE);
    }
    if (!in_curveP(Q)) {
        return (B_FALSE);
    }

    /* [HMV] Section 4.3 states that the above steps, combined with the
     * fact the h=1 for the curves used here, implies that order*Q =
     * Infinity, which is required by ANSI X9.63.
     */

    pointMpyP(tgt, k, Q);
    /* Q2 can't be infinity if 1 <= k < orderP, which is supposed to be
       the case, but the test is so cheap, we just do it. */
    if (tgt->infinity) {
        return (B_FALSE);
    }
    return (B_TRUE);
}

QStatus Crypto_ECC_OldEncoding::GenerateSharedSecret(Crypto_ECC& ecc, const ECCPublicKey* peerPublicKey, ECCSecretOldEncoding* secret)
{
    return Crypto_ECC_GenerateSharedSecret(peerPublicKey, ecc.GetDHPrivateKey(), secret);
}

QStatus Crypto_ECC_OldEncoding::ReEncode(const ECCPublicKey* newenc, ECCPublicKeyOldEncoding* oldenc)
{
    affine_point_t ap;
    ap.infinity = 0;
    binary_to_bigval(newenc->x, &ap.x, sizeof(newenc->x));
    binary_to_bigval(newenc->y, &ap.y, sizeof(newenc->y));
    U32ArrayToU8BeArray((const uint32_t*) &ap, U32_AFFINEPOINT_SZ, (uint8_t*) oldenc);
    return ER_OK;
}

QStatus Crypto_ECC_OldEncoding::ReEncode(const ECCPublicKeyOldEncoding* oldenc, ECCPublicKey* newenc)
{
    affine_point_t ap;
    ap.infinity = 0;
    U8BeArrayToU32Array((const uint8_t*) oldenc, sizeof(ECCPublicKeyOldEncoding), (uint32_t*) &ap);
    bigval_to_binary(&ap.x, newenc->x, sizeof(newenc->x));
    bigval_to_binary(&ap.y, newenc->y, sizeof(newenc->y));
    return ER_OK;
}

} /*namespace qcc*/