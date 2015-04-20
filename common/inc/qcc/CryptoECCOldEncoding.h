#ifndef _CRYPTOECC_OLDENCODING_H
#define _CRYPTOECC_OLDENCODING_H
/**
 * @file
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

#include <qcc/CryptoECC.h>

namespace qcc {

/**
 * The size of the ECC big value
 */
static const size_t ECC_BIGVAL_SZ = 9;

/**
 * The size of the ECC public key
 */
static const size_t ECC_PUBLIC_KEY_SZ = (2 * ECC_BIGVAL_SZ * sizeof(uint32_t)) + sizeof(uint32_t);

/**
 * The Old encoding ECC public key big endian byte array
 */
struct ECCPublicKeyOldEncoding {
    uint8_t data[ECC_PUBLIC_KEY_SZ];
};

typedef ECCPublicKeyOldEncoding ECCSecretOldEncoding;

/**
 * Elliptic Curve Cryptography Old Encoding
 */
class Crypto_ECC_OldEncoding {

  public:

    /**
     * Re encode public key to old encoding
     * @param newenc public key in current encoding
     * @param oldenc[out] public key in old encoding
     * @return
     *      ER_OK if the process succeeds
     *      ER_FAIL otherwise
     */
    static QStatus AJ_CALL ReEncode(const ECCPublicKey* newenc, ECCPublicKeyOldEncoding* oldenc);
    /**
     * Re encode old encoding public key to current encoding
     * @param oldenc public key in old encoding
     * @param newenc[out] public key in current encoding
     * @return
     *      ER_OK if the process succeeds
     *      ER_FAIL otherwise
     */
    static QStatus AJ_CALL ReEncode(const ECCPublicKeyOldEncoding* oldenc, ECCPublicKey* newenc);


    /**
     * Generates the Diffie-Hellman shared secret in the old encoding.
     * @param   ecc the Crypto_ECC object
     * @param   peerPublicKey the peer's public key
     * @param   secret the output shared secret
     * @return
     *      ER_OK if the shared secret is successfully generated.
     *      ER_FAIL otherwise
     *      Other error status.
     */
    static QStatus AJ_CALL GenerateSharedSecret(Crypto_ECC& ecc, const ECCPublicKey* peerPublicKey, ECCSecretOldEncoding* secret);

};

} /* namespace qcc */


#endif
