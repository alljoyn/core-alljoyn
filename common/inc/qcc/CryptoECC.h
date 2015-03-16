#ifndef _CRYPTOECC_H
#define _CRYPTOECC_H
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

#include <Status.h>
#include <qcc/platform.h>

#include <qcc/String.h>


namespace qcc {

/**
 * The ECC coordinate size
 */

static const size_t ECC_COORDINATE_SZ = 8 * sizeof(uint32_t);

/*
 * Empty ECC coordinate
 */
static const uint8_t ECC_COORDINATE_EMPTY[ECC_COORDINATE_SZ] = { 0 };

/**
 * The ECC private key big endian byte array
 */

struct ECCPrivateKey {
    uint8_t x[ECC_COORDINATE_SZ];
    ECCPrivateKey() {
        memset(x, 0, ECC_COORDINATE_SZ);
    }

    void operator=(const ECCPrivateKey& k)
    {
        memcpy(x, k.x, ECC_COORDINATE_SZ);
    }

    bool operator==(const ECCPrivateKey& k) const
    {
        return memcmp(x, k.x, ECC_COORDINATE_SZ) == 0;
    }
};

/**
 * The ECC public key big endian byte array
 */
struct ECCPublicKey {
    uint8_t x[ECC_COORDINATE_SZ];
    uint8_t y[ECC_COORDINATE_SZ];

    ECCPublicKey() {
        memset(x, 0, ECC_COORDINATE_SZ);
        memset(y, 0, ECC_COORDINATE_SZ);
    }

    bool empty() const
    {
        return (memcmp(x, ECC_COORDINATE_EMPTY, ECC_COORDINATE_SZ) == 0) &&
               (memcmp(y, ECC_COORDINATE_EMPTY, ECC_COORDINATE_SZ) == 0);
    }

    bool operator==(const ECCPublicKey& k) const
    {
        int n = memcmp(x, k.x, ECC_COORDINATE_SZ);
        return (n == 0) && (0 == memcmp(y, k.y, ECC_COORDINATE_SZ));
    }

    bool operator!=(const ECCPublicKey& k) const
    {
        int n = memcmp(x, k.x, ECC_COORDINATE_SZ);
        return (n != 0) || (0 != memcmp(y, k.y, ECC_COORDINATE_SZ));
    }

    bool operator<(const ECCPublicKey& k) const
    {
        int n = memcmp(x, k.x, ECC_COORDINATE_SZ);
        if (n == 0) {
            n = memcmp(y, k.y, ECC_COORDINATE_SZ);
        }
        if (n < 0) {
            return true;
        } else {
            return false;
        }
    }

    void operator=(const ECCPublicKey& k)
    {
        memcpy(x, k.x, ECC_COORDINATE_SZ);
        memcpy(y, k.y, ECC_COORDINATE_SZ);
    }

    /**
     * Exports the key to a byte array.
     * @param[in] data the array to store the data in
     * @param[in, out] size provides the size of the passed buffer as input. On a  successfull return it
     *   will contain the actual amount of data stored
     *
     * @return ER_OK on success others on failure
     */
    QStatus Export(uint8_t* data, size_t* size) const;

    /**
     * Imports the key from a byte array.
     * @param[in] data the array to store the data in
     * @param[in] size the size of the passed buffer.
     *
     * @return ER_OK  on success others on failure
     */
    QStatus Import(const uint8_t* data, size_t size);
    const qcc::String ToString() const;

};

typedef ECCPrivateKey ECCSecret;

/**
 * The ECC signature big endian byte array
 */

struct ECCSignature {
    uint8_t r[ECC_COORDINATE_SZ];
    uint8_t s[ECC_COORDINATE_SZ];

    ECCSignature() {
        memset(r, 0, ECC_COORDINATE_SZ);
        memset(s, 0, ECC_COORDINATE_SZ);
    }

    void operator=(const ECCSignature& k)
    {
        memcpy(r, k.r, ECC_COORDINATE_SZ);
        memcpy(s, k.s, ECC_COORDINATE_SZ);
    }
};

/**
 * Elliptic Curve Cryptography
 */
class Crypto_ECC {

  public:

    /**
     * The NIST recommended elliptic curve P-256
     */
    static const uint8_t ECC_NIST_P256 = 0;

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
    QStatus GenerateSharedSecret(const ECCPublicKey* peerPublicKey, ECCSecret* secret);

    /**
     * Retrieve the DH public key
     * @return  the DH public key.  It's a pointer to an internal buffer. Its lifetime is the same as the object's lifetime.
     */
    const ECCPublicKey* GetDHPublicKey() const
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
     * Sign a digest using the DSA key
     * @param digest The digest to sign
     * @param len The digest len
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
    QStatus DSAVerifyDigest(const uint8_t* digest, uint16_t len, const ECCSignature* sig);

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

    /**
     * Retrieve the ECC curve type.
     * @return the ECC curve type
     */
    const uint8_t GetCurveType()
    {
        return ECC_NIST_P256;
    }

    ~Crypto_ECC();

  private:

    ECCPrivateKey dhPrivateKey;
    ECCPublicKey dhPublicKey;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;

};

} /* namespace qcc */


#endif
