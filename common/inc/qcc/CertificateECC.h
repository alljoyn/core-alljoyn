#ifndef _CERTIFICATE_ECC_H_
#define _CERTIFICATE_ECC_H_
/**
 * @file
 *
 * Certificate ECC utilities
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
#include <qcc/Certificate.h>
#include <qcc/Crypto.h>
#include <qcc/CryptoECC.h>

namespace qcc {

/**
 * CertificateECC Class
 */
class CertificateECC : public Certificate {
  public:
    CertificateECC(uint32_t version) : Certificate(version)
    {
    }

    CertificateECC() : Certificate(0xFFFFFFFF)
    {
    }

    virtual void SetVersion(uint32_t val) {
        Certificate::SetVersion(val);
    }

    virtual const ECCPublicKey* GetIssuer()
    {
        return NULL;
    }

    virtual const ECCPublicKey* GetSubject()
    {
        return NULL;
    }

    virtual const ValidPeriod* GetValidity()
    {
        return NULL;
    }

    virtual const bool IsDelegate()
    {
        return false;
    }

    virtual const uint8_t* GetExternalDataDigest()
    {
        return NULL;
    }

    virtual const ECCSignature* GetSig()
    {
        return NULL;
    }


    virtual String ToString()
    {
        return String::Empty;
    }

    virtual ~CertificateECC()
    {
    }

    static const size_t GUILD_ID_LEN = 16;   /* the Guild ID length */
};

/**
 * CertificateType0 Class
 */
class CertificateType0 : public CertificateECC {

  public:
    CertificateType0() : CertificateECC(0)
    {
        SetVersion(0);
    }
    CertificateType0(const ECCPublicKey* issuer, const uint8_t* externalDigest);

    /**
     * Set the certificate version
     * @param val the certificate version
     */
    virtual void SetVersion(uint32_t val);

    const ECCPublicKey* GetIssuer()
    {
        return (ECCPublicKey*) &encoded[OFFSET_ISSUER];
    }

    void SetIssuer(const ECCPublicKey* issuer);

    const uint8_t* GetExternalDataDigest()
    {
        return &encoded[OFFSET_DIGEST];
    }

    void SetExternalDataDigest(const uint8_t* externalDataDigest);

    const ECCSignature* GetSig()
    {
        return (ECCSignature*) &encoded[OFFSET_SIG];
    }

    void SetSig(const ECCSignature* sig);

    /**
     * sign the certificate
     * @param dsaPrivateKey the issuer private key to sign the cert
     */
    QStatus Sign(const ECCPrivateKey* dsaPrivateKey);

    bool VerifySignature();

    /**
     * Get the encoded bytes for the certificate
     * @return the encoded bytes
     */
    const uint8_t* GetEncoded()
    {
        return encoded;
    }

    /**
     * Get the length of the encoded bytes for the certificate
     * @return the length of the encoded bytes
     */
    size_t GetEncodedLen()
    {
        assert(sizeof(encoded) == ENCODED_LEN);
        return ENCODED_LEN;
    }

    /**
     * Load the encoded bytes for the certificate
     * @param encodedBytes the encoded bytes
     * @param len the length of the encoded bytes
     * @return ER_OK for sucess; otherwise, error code.
     */
    QStatus LoadEncoded(const uint8_t* encodedBytes, size_t len);

    /**
     * Get the PEM encoded bytes for the certificate
     * @return the PEM encoded bytes
     */
    String GetPEM();

    /**
     * Load the PEM encoded bytes for the certificate
     * @param encoded the encoded bytes
     * @return ER_OK for sucess; otherwise, error code.
     */
    QStatus LoadPEM(const String& PEM);

    String ToString();

    ~CertificateType0()
    {
    }

  private:
    /*
     * The encoded is a network-order byte array representing the following fields:
     * version: uint32_t
     * issuer: ECCPublicKey
     * digest: SHA256
     * sig: ECCSignature
     */
    static const size_t OFFSET_VERSION = 0;
    static const size_t OFFSET_ISSUER = OFFSET_VERSION + 4;
    static const size_t OFFSET_DIGEST = OFFSET_ISSUER + ECC_PUBLIC_KEY_SZ;
    static const size_t OFFSET_SIG = OFFSET_DIGEST + Crypto_SHA256::DIGEST_SIZE;
    static const size_t ENCODED_LEN = OFFSET_SIG + ECC_SIGNATURE_SZ;

    uint8_t encoded[ENCODED_LEN];
};

/**
 * CertificateType1 Class
 */
class CertificateType1 : public CertificateECC {

  public:
    CertificateType1() : CertificateECC(1)
    {
        SetVersion(1);
        SetDelegate(false);
    }
    CertificateType1(const ECCPublicKey* issuer, const ECCPublicKey* subject);

    /**
     * Set the certificate version
     * @param val the certificate version
     */
    virtual void SetVersion(uint32_t val);

    const ECCPublicKey* GetIssuer()
    {
        return (ECCPublicKey*) &encoded[OFFSET_ISSUER];
    }

    void SetIssuer(const ECCPublicKey* issuer);

    const ECCPublicKey* GetSubject()
    {
        return (ECCPublicKey*) &encoded[OFFSET_SUBJECT];
    }

    void SetSubject(const ECCPublicKey* subject);

    const ValidPeriod* GetValidity()
    {
        return &validity;
    }

    void SetValidity(const ValidPeriod* validPeriod);

    const bool IsDelegate()
    {
        return (bool) encoded[OFFSET_DELEGATE];
    }

    void SetDelegate(bool enabled)
    {
        encoded[OFFSET_DELEGATE] = enabled;
    }

    const uint8_t* GetExternalDataDigest()
    {
        return &encoded[OFFSET_DIGEST];
    }

    void SetExternalDataDigest(const uint8_t* externalDataDigest);

    const ECCSignature* GetSig()
    {
        return (ECCSignature*) &encoded[OFFSET_SIG];
    }

    void SetSig(const ECCSignature* sig);

    QStatus Sign(const ECCPrivateKey* dsaPrivateKey);

    bool VerifySignature();

    /**
     * Get the encoded bytes for the certificate
     * @return the encoded bytes
     */
    const uint8_t* GetEncoded()
    {
        return encoded;
    }

    /**
     * Get the length of the encoded bytes for the certificate
     * @return the length of the encoded bytes
     */
    size_t GetEncodedLen()
    {
        assert(sizeof(encoded) == ENCODED_LEN);
        return ENCODED_LEN;
    }

    /**
     * Load the encoded bytes for the certificate
     * @param encodedBytes the encoded bytes
     * @param len the length of the encoded bytes
     * @return ER_OK for sucess; otherwise, error code.
     */
    QStatus LoadEncoded(const uint8_t* encodedBytes, size_t len);

    /**
     * Get the PEM encoded bytes for the certificate
     * @return the PEM encoded bytes
     */
    String GetPEM();

    /**
     * Load the PEM encoded bytes for the certificate
     * @param encoded the encoded bytes
     * @return ER_OK for sucess; otherwise, error code.
     */
    QStatus LoadPEM(const String& PEM);

    String ToString();

    ~CertificateType1()
    {
    }

  private:
    /*
     * The encoded is a network-order byte array representing the following fields:
     * version: uint32_t
     * issuer: ECCPublicKey
     * subject: ECCPublicKey
     * validFrom: uint64_t
     * validTo: uint64_t
     * delegate: uint8_t
     * digest: SHA256
     * sig: ECCSignature
     */
    static const size_t OFFSET_VERSION = 0;
    static const size_t OFFSET_ISSUER = OFFSET_VERSION + 4;
    static const size_t OFFSET_SUBJECT = OFFSET_ISSUER + ECC_PUBLIC_KEY_SZ;
    static const size_t OFFSET_VALIDFROM = OFFSET_SUBJECT + ECC_PUBLIC_KEY_SZ;
    static const size_t OFFSET_VALIDTO = OFFSET_VALIDFROM + 8;
    static const size_t OFFSET_DELEGATE = OFFSET_VALIDTO + 8;
    static const size_t OFFSET_DIGEST = OFFSET_DELEGATE + 1;
    static const size_t OFFSET_SIG = OFFSET_DIGEST + Crypto_SHA256::DIGEST_SIZE;
    static const size_t ENCODED_LEN = OFFSET_SIG + ECC_SIGNATURE_SZ;

    uint8_t encoded[ENCODED_LEN];
    ValidPeriod validity;
};

/**
 * CertificateType2 Class
 */
class CertificateType2 : public CertificateECC {

  public:
    CertificateType2() : CertificateECC(2)
    {
        SetVersion(2);
        SetDelegate(false);
    }
    CertificateType2(const ECCPublicKey* issuer, const ECCPublicKey* subject);

    /**
     * Set the certificate version
     * @param val the certificate version
     */
    virtual void SetVersion(uint32_t val);

    const ECCPublicKey* GetIssuer()
    {
        return (ECCPublicKey*) &encoded[OFFSET_ISSUER];
    }

    void SetIssuer(const ECCPublicKey* issuer);

    const ECCPublicKey* GetSubject()
    {
        return (ECCPublicKey*) &encoded[OFFSET_SUBJECT];
    }

    void SetSubject(const ECCPublicKey* subject);

    const ValidPeriod* GetValidity()
    {
        return &validity;
    }

    void SetValidity(const ValidPeriod* validPeriod);

    const bool IsDelegate()
    {
        return (bool) encoded[OFFSET_DELEGATE];
    }

    void SetDelegate(bool enabled)
    {
        encoded[OFFSET_DELEGATE] = enabled;
    }

    const uint8_t* GetGuild()
    {
        return &encoded[OFFSET_GUILD];
    }

    void SetGuild(const uint8_t* newGuild, size_t guildLen);

    const uint8_t* GetExternalDataDigest()
    {
        return &encoded[OFFSET_DIGEST];
    }
    void SetExternalDataDigest(const uint8_t* externalDataDigest);

    const ECCSignature* GetSig()
    {
        return (ECCSignature*) &encoded[OFFSET_SIG];
    }

    void SetSig(const ECCSignature* sig);

    QStatus Sign(const ECCPrivateKey* dsaPrivateKey);

    bool VerifySignature();

    /**
     * Get the encoded bytes for the certificate
     * @return the encoded bytes
     */
    const uint8_t* GetEncoded()
    {
        return encoded;
    }

    /**
     * Get the length of the encoded bytes for the certificate
     * @return the length of the encoded bytes
     */
    size_t GetEncodedLen()
    {
        assert(sizeof(encoded) == ENCODED_LEN);
        return ENCODED_LEN;
    }

    /**
     * Load the encoded bytes for the certificate
     * @param encodedBytes the encoded bytes
     * @param len the length of the encoded bytes
     * @return ER_OK for sucess; otherwise, error code.
     */
    QStatus LoadEncoded(const uint8_t* encodedBytes, size_t len);

    /**
     * Get the PEM encoded bytes for the certificate
     * @return the PEM encoded bytes
     */
    String GetPEM();

    /**
     * Load the PEM encoded bytes for the certificate
     * @param encoded the encoded bytes
     * @return ER_OK for sucess; otherwise, error code.
     */
    QStatus LoadPEM(const String& PEM);

    String ToString();

    ~CertificateType2()
    {
    }

  private:
    /*
     * The encoded is a network-order byte array representing the following fields:
     * version: uint32_t
     * issuer: ECCPublicKey
     * subject: ECCPublicKey
     * validFrom: uint64_t
     * validTo: uint64_t
     * delegate: uint8_t
     * guild: GUILD_ID_LEN
     * digest: SHA256
     * sig: ECCSignature
     */

    static const size_t OFFSET_VERSION = 0;
    static const size_t OFFSET_ISSUER = OFFSET_VERSION + 4;
    static const size_t OFFSET_SUBJECT = OFFSET_ISSUER + ECC_PUBLIC_KEY_SZ;
    static const size_t OFFSET_VALIDFROM = OFFSET_SUBJECT + ECC_PUBLIC_KEY_SZ;
    static const size_t OFFSET_VALIDTO = OFFSET_VALIDFROM + 8;
    static const size_t OFFSET_DELEGATE = OFFSET_VALIDTO + 8;
    static const size_t OFFSET_GUILD = OFFSET_DELEGATE + 1;
    static const size_t OFFSET_DIGEST = OFFSET_GUILD + GUILD_ID_LEN;
    static const size_t OFFSET_SIG = OFFSET_DIGEST + Crypto_SHA256::DIGEST_SIZE;
    static const size_t ENCODED_LEN = OFFSET_SIG + ECC_SIGNATURE_SZ;

    uint8_t encoded[ENCODED_LEN];
    ValidPeriod validity;

};

/**
 * Encode the private key in a PEM string.
 * @param privateKey the private key to encode
 * @param len the private key length
 * @param[out] encoded the output string holding the resulting PEM string
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_EncodePrivateKey(const uint32_t* privateKey, size_t len, String& encoded);

/**
 * Decode the private from a PEM string.
 * @param encoded the input string holding the PEM string
 * @param[out] privateKey the output private key
 * @param len the private key length
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_DecodePrivateKey(const String& encoded, uint32_t* privateKey, size_t len);

/**
 * Encode the public key in a PEM string.
 * @param publicKey the public key to encode
 * @param len the public key length
 * @param[out] encoded the output string holding the resulting PEM string
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_EncodePublicKey(const uint32_t* publicKey, size_t len, String& encoded);

/**
 * Decode the public key from a PEM string.
 * @param encoded the input string holding the PEM string
 * @param[out] publicKey the output public key
 * @param len the private key length
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_DecodePublicKey(const String& encoded, uint32_t* publicKey, size_t len);


/**
 * Count the number of certificates in a PEM string representing a cert chain.
 * @param encoded the input string holding the PEM string
 * @param[out] count the number of certs
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_GetCertCount(const String& encoded, size_t* count);

/**
 * Get the certificate version from the encoded bytes
 * @param encoded the input encoded bytes
 * @param len the length of the input encoded bytes
 * @param[out] certVersion the buffer holding the certificate version
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_GetVersionFromEncoded(const uint8_t* encoded, size_t len, uint32_t* certVersion);

/**
 * Get the certificate version from the PEM string
 * @param pem the input string holding the PEM string
 * @param[out] certVersion the buffer holding the certificate version
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_GetVersionFromPEM(const String& pem, uint32_t* certVersion);

/**
 * Retrieve the number of certificates in a PEM string representing a cert chain.
 * @param encoded the input string holding the PEM string
 * @param[in,out] certChain the input string holding the array of certs.  The array elements needs to be deallocated after used.
 * @param[in] count the expected number of certs
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_GetCertChain(const String &encoded, CertificateECC * certChain[], size_t count);

/**
 * A factory method to generate an instance of the certificate using the encode d bytes.
 * @param encoded the input encoded bytes
 * @param len the size of the encoded bytes
 * @return
 *      - the instance of the certificate using the encoded bytes.  The certificate instance must be allocated after used.
 *      - NULL if the encoded bytes are invalid
 */
CertificateECC* CertECCUtil_GetInstance(const uint8_t* encoded, size_t len);

} /* namespace qcc */

#endif
