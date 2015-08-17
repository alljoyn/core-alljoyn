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

#include <alljoyn/Status.h>
#include <assert.h>
#include <qcc/platform.h>

#include <qcc/String.h>

namespace qcc {

/**
 * The ECC coordinate size
 */

static const size_t ECC_COORDINATE_SZ = 8 * sizeof(uint32_t);

/**
 * The ECC private key
 *
 * At the moment, because the code only supports one curve, private keys
 * are not innately tied to a particular curve. In the future, if the code
 * supports more than one curve, a private key should store its curve also.
 */
class ECCPrivateKey {

    /**
     * The ECCPrivateKey data
     */
    uint8_t d[ECC_COORDINATE_SZ];

  public:

    /**
     * ECCPrivateKey constructor
     */
    ECCPrivateKey() {
        memset(d, 0, ECC_COORDINATE_SZ);
    }

    /**
     * ECCPrivateKey destructor
     */
    ~ECCPrivateKey();

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
     * Get the size of the private key value
     *
     * @return Size of the private key in bytes
     */
    const size_t GetSize() const
    {
        return ECC_COORDINATE_SZ;
    }

    /**
     * Return the ECCPrivateKey as a string
     * @return the ECCPrivateKey as a string
     */
    const String ToString() const;

    /**
     * Import the key from a byte array.
     * @param[in] data the array to store the data in
     * @param[in] size the size of the passed buffer
     *
     * @return ER_OK  on success others on failure
     */
    QStatus Import(const uint8_t* data, const size_t size)
    {
        if (ECC_COORDINATE_SZ != size) {
            return ER_BAD_ARG_2;
        }

        memcpy(d, data, size);

        return ER_OK;
    }

    /**
     * Export the key to a byte array.
     * @param[in] data the array to store the data in
     * @param[in,out] size provides the size of the passed buffer as input. On a successful return it
     *   will contain the actual amount of data stored, which is the same value as returned by GetSize().
     *   On ER_BUFFER_TOO_SMALL, contains the amount of storage required, which is also the value returned
     *   by GetSize().
     *
     * @return ER_OK on success others on failure
     */
    QStatus Export(uint8_t* data, size_t* size) const;

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

    /** Support methods for CryptoECCMath **/

    /*
     * Return a buffer containing just the private key value
     *
     * @return Buffer containing the private key value
     */
    const uint8_t* GetD() const
    {
        return this->d;
    }

    const size_t GetDSize() const
    {
        return this->GetSize();
    }

};

/**
 * The ECC public key
 *
 * At the moment, because the code only supports one curve, private keys
 * are not innately tied to a particular curve. In the future, if the code
 * supports more than one curve, a private key should store its curve also.
 */
class ECCPublicKey {

    /**
     * The x coordinate of the elliptic curve
     */
    uint8_t x[ECC_COORDINATE_SZ];
    /**
     * The y coordinate of the elliptic curve
     */
    uint8_t y[ECC_COORDINATE_SZ];

  public:

    /**
     * Clear the key to make it empty.
     */
    void Clear()
    {
        memset(x, 0, GetCoordinateSize());
        memset(y, 0, GetCoordinateSize());
    }

    ECCPublicKey()
    {
        Clear();
    }

    /**
     * Check to see if the ECCPublicKey is empty
     *
     * @return true if the ECCPublicKey is empty
     */
    bool empty() const;

    /**
     * Equals operator
     *
     * @param[in] k the ECCPublic key to compare
     *
     * @return true if the compared ECCPublicKeys are equal to each other
     */
    bool operator==(const ECCPublicKey& k) const
    {
        int n = memcmp(x, k.x, GetCoordinateSize());
        return (n == 0) && (0 == memcmp(y, k.y, GetCoordinateSize()));
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
        int n = memcmp(x, k.x, GetCoordinateSize());
        if (n == 0) {
            n = memcmp(y, k.y, GetCoordinateSize());
        }
        if (n < 0) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * Copy constructor for ECCPublicKey
     *
     * @param[in] other   the ECCPublicKey to copy
     */
    ECCPublicKey(const ECCPublicKey& other)
    {
        memcpy(x, other.x, ECC_COORDINATE_SZ);
        memcpy(y, other.y, ECC_COORDINATE_SZ);
    }

    /**
     * Assign operator for ECCPublicKey
     *
     * @param[in] other the ECCPublic key to assign
     */
    ECCPublicKey& operator=(const ECCPublicKey& other)
    {
        if (this != &other) {
            memcpy(x, other.x, GetCoordinateSize());
            memcpy(y, other.y, GetCoordinateSize());
        }
        return *this;
    }

    /**
     * Export the key to a byte array. The X and Y coordinates are concatenated in that order, and each
     * occupy exactly half of the returned array. The X coordinate is in the first half, and the Y coordinate
     * in the second. Use the returned size divided by two as the length of an individual coordinate.
     * @param[in] data the array to store the data in
     * @param[in,out] size provides the size of the passed buffer as input. On a successful return it
     *   will contain the actual amount of data stored
     *
     * @return ER_OK on success others on failure
     */
    QStatus Export(uint8_t* data, size_t* size) const;

    /**
     * Import the key from a byte array
     * @param[in] data the array to store the data in
     * @param[in] size the size of the passed buffer
     *
     * @return ER_OK  on success others on failure
     */
    QStatus Import(const uint8_t* data, const size_t size);

    /**
     * Import the key from two byte arrays, one containing each coordinate
     * @param[in] xData array containing the bytes of the X coordinate
     * @param[in] xSize length of xData
     * @param[in] yData array containing the bytes of the Y coordinate
     * @param[in] ySize length of yData
     *
     * @return ER_OK   on success others on failure
     */
    QStatus Import(const uint8_t* xData, const size_t xSize, const uint8_t* yData, const size_t ySize);

    /**
     * Return the ECCPublicKey to a string
     * @return the ECCPublicKey as a string.
     */
    const String ToString() const;

    /**
     * Return the size of the public key in exported form
     *
     * @return Size of the exported public key
     */
    inline const size_t GetSize() const
    {
        return 2 * GetCoordinateSize();
    }

    /** Support methods for CryptoECCMath **/

    /*
     * Get a buffer containing just the X coordinate of this public key
     *
     * @return Buffer containing the X value
     */
    const uint8_t* GetX() const
    {
        return this->x;
    }

    /**
     * Get a buffer containing just the Y coordinate of this public key
     *
     * @return Buffer containing the Y value
     */
    const uint8_t* GetY() const
    {
        return this->y;
    }

    /*
     * Get the size of a single coordinate
     *
     * @return The size of a single coordinate
     */
    inline const size_t GetCoordinateSize() const
    {
        assert(sizeof(this->x) == sizeof(this->y));

        return sizeof(this->x);
    }
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
     * The ECCSignature copy operator
     *
     * @param[in] other the ECCSignature to copy
     */

    ECCSignature(const ECCSignature& other)
    {
        memcpy(r, other.r, ECC_COORDINATE_SZ);
        memcpy(s, other.s, ECC_COORDINATE_SZ);
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
