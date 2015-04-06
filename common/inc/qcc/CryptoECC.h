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

/**
 * Empty ECC coordinate
 */
static const uint8_t ECC_COORDINATE_EMPTY[ECC_COORDINATE_SZ] = { 0 };

/**
 * The ECC private key big endian byte array
 */
struct ECCPrivateKey {
    /**
     * The ECCPrivateKey data
     */
    uint8_t d[ECC_COORDINATE_SZ];

    /**
     * ECCPrivateKey constructor
     */
    ECCPrivateKey() {
        memset(d, 0, ECC_COORDINATE_SZ);
    }

    /**
     * the assign operator for the ECCPrivateKey
     *
     * @param[in] other the ECCPrivate key to assign
     */
    ECCPrivateKey& operator=(const ECCPrivateKey& other)
    {
        if (this != &other) {
            memcpy(d, other.d, ECC_COORDINATE_SZ);
        }
        return *this;
    }

    /**
     * Equals operator for the ECCPrivateKey.
     *
     * @param[in] k the ECCPrivateKey to compare
     *
     * @return true if the ECCPrivateKeys are equal
     */
    bool operator==(const ECCPrivateKey& k) const
    {
        return memcmp(d, k.d, ECC_COORDINATE_SZ) == 0;
    }
};

/**
 * The ECC public key big endian byte array
 */
struct ECCPublicKey {
    /**
     * The x coordinate of the elliptic curve
     */
    uint8_t x[ECC_COORDINATE_SZ];
    /**
     * The y coordinate of the elliptic curve
     */
    uint8_t y[ECC_COORDINATE_SZ];

    ECCPublicKey() {
        memset(x, 0, ECC_COORDINATE_SZ);
        memset(y, 0, ECC_COORDINATE_SZ);
    }

    /**
     * Check to see if the ECCPublicKey is empty.
     *
     * @return true if the ECCPublicKey is empty
     */
    bool empty() const
    {
        return (memcmp(x, ECC_COORDINATE_EMPTY, ECC_COORDINATE_SZ) == 0) &&
               (memcmp(y, ECC_COORDINATE_EMPTY, ECC_COORDINATE_SZ) == 0);
    }

    /**
     * Equals operator
     *
     * @param[in] k the ECCPublic key to compare
     *
     * @return true if the compared ECCPublicKeys are equal to each other
     */
    bool operator==(const ECCPublicKey& k) const
    {
        int n = memcmp(x, k.x, ECC_COORDINATE_SZ);
        return (n == 0) && (0 == memcmp(y, k.y, ECC_COORDINATE_SZ));
    }

    /**
     * Not equals operator
     *
     * @param[in] k the ECCPublicKey to compare
     *
     * @return true if the compared ECCPublicKeys are not equal to each other
     */
    bool operator!=(const ECCPublicKey& k) const
    {
        return !(*this == k);
    }

    /**
     * The less than operator for the ECCPublicKey
     *
     * The x coordinate are compared first. If the x coordinates match then
     * the y coordinate is compared.
     *
     * @param[in] k the ECCPublicKey to compare
     *
     * @return True if the left ECCPublicKey is less than the right ECCPublicKey
     * false otherwise.
     */
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

    /**
     * Assign operator for ECCPublicKey
     *
     * @param[in] other the ECCPublic key to assign
     */
    ECCPublicKey& operator=(const ECCPublicKey& other)
    {
        if (this != &other) {
            memcpy(x, other.x, ECC_COORDINATE_SZ);
            memcpy(y, other.y, ECC_COORDINATE_SZ);
        }
        return *this;
    }

    /**
     * Exports the key to a byte array.
     * @param[in] data the array to store the data in
     * @param[in,out] size provides the size of the passed buffer as input. On a  successfull return it
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
    /**
     * Return the ECCPublicKey to a string.
     * @return
     * the ECCPublicKey to a string.
     */
    const qcc::String ToString() const;

};

/**
 * The ECC secret
 */

class ECCSecret {
  public:

    /**
     * Opaque type for the internal state.
     */
    struct ECCSecretState;

    /**
     * Default Constructor;
     */
    ECCSecret();

    /**
     * Set the opaque secret state for this object
     * @param   pEccSecretState the internal secret state to set.
     * @return
     *      ER_OK if the secret is successfully set.
     *      ER_FAIL otherwise.
     *      Other error status.
     */
    QStatus SetSecretState(const ECCSecretState* pEccSecretState);

    /**
     * Derives the PreMasterSecret.
     * Current implementaiton uses SHA256 HASH KDF.
     * @param   pbPreMasterSecret buffer to receive premaster secret.
     * @param   cbPreMasterSecret count of buffer to receive premaster secret.
     * @return
     *      ER_OK if the pre-master secret is successfully computed and put in pbPreMasterSecret.
     *      ER_FAIL otherwise.
     *      Other error status.
     */
    QStatus DerivePreMasterSecret(uint8_t* pbPreMasterSecret, size_t cbPreMasterSecret);

    /**
     * Default Destructor
     */

    ~ECCSecret();

  private:
    /* private copy constructor to prevent double delete of eccSecretState */
    ECCSecret(const ECCSecret&);
    /* private assignment operator to prevent double delete of eccSecretState */
    ECCSecret& operator=(const ECCSecret&);

    /**
     * Private internal state
     */
    ECCSecretState* eccSecretState;

};

/**
 * The ECC signature big endian byte array
 */

struct ECCSignature {
    /**
     * The r value for the Elliptic Curve Digital Signature (r,s) signature pair
     */
    uint8_t r[ECC_COORDINATE_SZ];
    /**
     * The s value for the Elliptic Curve Digital Signature (r,s) signature pair
     */
    uint8_t s[ECC_COORDINATE_SZ];

    /**
     * ECCSignature constructor
     *
     * The Elliptic Curve Digital Signature (r,s) signature initialized to zero.
     */
    ECCSignature() {
        memset(r, 0, ECC_COORDINATE_SZ);
        memset(s, 0, ECC_COORDINATE_SZ);
    }

    /**
     * The ECCSignature assign operator
     * @param[in] other the ECC signature to assign
     */
    ECCSignature& operator=(const ECCSignature& other)
    {
        if (this != &other) {
            memcpy(r, other.r, ECC_COORDINATE_SZ);
            memcpy(s, other.s, ECC_COORDINATE_SZ);
        }
        return *this;
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
    Crypto_ECC();

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
    const ECCPublicKey* GetDHPublicKey() const;

    /**
     * Assign the DH public key
     * @param pubKey the public key to copy
     */
    void SetDHPublicKey(const ECCPublicKey* pubKey);

    /**
     * Retrieve the DH private key
     * @return  the DH private key.  Same lifetime as the object.
     */
    const ECCPrivateKey* GetDHPrivateKey() const;

    /**
     * Assign the DH private key
     * @param privateKey the private key to copy
     */
    void SetDHPrivateKey(const ECCPrivateKey* privateKey);

    /**
     * Retrieve the DSA public key
     * @return  the DSA public key.  Same lifetime as the object.
     */
    const ECCPublicKey* GetDSAPublicKey() const;

    /**
     * Assign the DSA public key
     * @param pubKey the public key to copy
     */
    void SetDSAPublicKey(const ECCPublicKey* pubKey);

    /**
     * Retrieve the DSA private key
     * @return  the DSA private key.  Same lifetime as the object.
     */
    const ECCPrivateKey* GetDSAPrivateKey() const;

    /**
     * Assign the DSA private key
     * @param privateKey the private key to copy
     */
    void SetDSAPrivateKey(const ECCPrivateKey* privateKey);

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
    /* private copy constructor to prevent double freeing of eccState */
    Crypto_ECC(const Crypto_ECC&);
    /* private assignment operator to prevent double freeing of eccState */
    Crypto_ECC& operator=(const Crypto_ECC&);

    /**
     * Opaque type for the internal state.
     */
    struct ECCState;

    /**
     * Private internal state
     */
    ECCState* eccState;
};

} /* namespace qcc */


#endif
