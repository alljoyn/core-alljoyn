#ifndef _CRYPTOECC_H
#define _CRYPTOECC_H
/**
 * @file
 *
 * This file provide wrappers around ECC cryptographic algorithms.
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/KeyBlob.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>


namespace qcc {

static const size_t ECC_BIGVAL_SZ = 9;  /* the size of big value */

/*
 * For P256 bigval_t types hold 288-bit 2's complement numbers (9
 * 32-bit words).  For P192 they hold 224-bit 2's complement numbers
 * (7 32-bit words).
 *
 * The representation is little endian by word and native endian
 * within each word.
 */

struct ECCBigVal {
    uint32_t data[ECC_BIGVAL_SZ];
};

struct ECCAffinePoint {
    ECCBigVal x;
    ECCBigVal y;
    uint32_t infinity;
};

struct ECDSASig {
    ECCBigVal r;
    ECCBigVal s;
};

typedef ECCBigVal ECCPrivateKey;
typedef ECCAffinePoint ECCPublicKey;
typedef ECCAffinePoint ECCSecret;
typedef ECDSASig ECCSignature;

typedef ECCBigVal ECCPrivateKey;


/**
 * Elliptic Curve Cryptography
 */
class Crypto_ECC {

  public:

    /**
     * Default constructor.
     */
    Crypto_ECC()
    {
    }

    /**
     * Generates the Ephemeral Diffie-Hellman key pair.
     *
     * @return
     *      ER_OK if the key pair is successfully generated.
     *      ER_FAIL otherwise
     *      Other error status.
     */
    QStatus GenerateDHKeyPair();

    /**
     * Generates the Diffie-Hellman shared secret.
     * @param   peerPublicKey the peer's public key
     * @param   secret the output shared secret
     * @return
     *      ER_OK if the shared secret is successfully generated.
     *      ER_FAIL otherwise
     *      Other error status.
     */
    QStatus GenerateSharedSecret(const ECCPublicKey* peerPublicKey, ECCPublicKey* secret);

    /**
     * Retrieve the DH public key
     * @return  the DH public key.  It's a pointer to an internal buffer. Its lifetime is the same as the object's lifetime.
     */
    const ECCPublicKey* GetDHPublicKey()
    {
        return &dhPublicKey;
    }

    /**
     * Assign the DH public key
     * @param pubKey the public key to copy
     */
    void SetDHPublicKey(const ECCPublicKey* pubKey)
    {
        dhPublicKey = *pubKey;
    }

    /**
     * Retrieve the DH private key
     * @return  the DH private key.  Same lifetime as the object.
     */
    const ECCPrivateKey* GetDHPrivateKey()
    {
        return &dhPrivateKey;
    }

    /**
     * Assign the DH private key
     * @param privateKey the private key to copy
     */
    void SetDHPrivateKey(const ECCPrivateKey* privateKey)
    {
        dhPrivateKey = *privateKey;
    }

    /**
     * Retrieve the DSA public key
     * @return  the DSA public key.  Same lifetime as the object.
     */
    const ECCPublicKey* GetDSAPublicKey()
    {
        return &dsaPublicKey;
    }

    /**
     * Assign the DSA public key
     * @param pubKey the public key to copy
     */
    void SetDSAPublicKey(const ECCPublicKey* pubKey)
    {
        dsaPublicKey = *pubKey;
    }

    /**
     * Retrieve the DSA private key
     * @return  the DSA private key.  Same lifetime as the object.
     */
    const ECCPrivateKey* GetDSAPrivateKey()
    {
        return &dsaPrivateKey;
    }

    /**
     * Assign the DSA private key
     * @param privateKey the private key to copy
     */
    void SetDSAPrivateKey(const ECCPrivateKey* privateKey)
    {
        dsaPrivateKey = *privateKey;
    }

    /**
     * Generates the DSA key pair.
     *
     * @return
     *      ER_OK if the key pair is successfully generated.
     *      ER_FAIL otherwise
     *      Other error status.
     */
    QStatus GenerateDSAKeyPair();

    /**
     * Sign a buffer using the DSA key
     * @param buf The buffer to sign
     * @param len The buffer len
     * @param sig The output signature
     * @return
     *      ER_OK if the signing process succeeds
     *      ER_FAIL otherwise
     *      Other error status.
     */
    QStatus DSASignDigest(const uint8_t* digest, uint16_t len, ECCSignature* sig);

    /**
     * Sign a buffer using the DSA key
     * @param buf The buffer to sign
     * @param len The buffer len
     * @param sig The output signature
     * @return
     *      ER_OK if the signing process succeeds
     *      ER_FAIL otherwise
     *      Other error status.
     */
    QStatus DSASign(const uint8_t* buf, uint16_t len, ECCSignature* sig);

    /**
     * Verify DSA signature of a digest
     * @param digest The digest to sign
     * @param len The digest len
     * @param sig The signature
     * @return  - ER_OK if the signature verification succeeds
     *          - ER_FAIL otherwise
     *          - Other error status.
     */
    QStatus DSAVerifyDigest(const uint8_t* digest, uint16_t len, const ECCSignature*sig);

    /**
     * Verify DSA signature of a buffer
     * @param buf The buffer to sign
     * @param len The buffer len
     * @param sig The signature
     * @return
     *      ER_OK if the signature verification succeeds
     *      ER_FAIL otherwise
     *      Other error status.
     */
    QStatus DSAVerify(const uint8_t* buf, uint16_t len, const ECCSignature* sig);

    ~Crypto_ECC();

  private:

    ECCPrivateKey dhPrivateKey;
    ECCPublicKey dhPublicKey;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;

};

} /* namespace qcc */


#endif
