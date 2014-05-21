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
    CertificateECC(uint32_t version, bool generic) : Certificate(version), generic(generic), buf(NULL), bufLen(0)
    {
    }

    CertificateECC(uint32_t version) : Certificate(version), generic(true), buf(NULL), bufLen(0)
    {
    }
    CertificateECC() : Certificate(0xFFFFFFFF), generic(true), buf(NULL), bufLen(0)
    {
    }

    /**
     * Is the cert generic
     */
    bool IsGeneric()
    {
        return generic;
    }

    /**
     * Set the generic flag.
     * @param yesNo the generic flag
     */
    void SetGeneric(bool yesNo)
    {
        generic = yesNo;
    }

    /**
     * get the generic certificate buffer
     * @return buffer
     */
    const uint8_t* GetBuf()
    {
        return buf;
    }

    /**
     * get the length of the generic buffer
     */
    size_t GetBufLength()
    {
        return bufLen;
    }

    virtual const ECCPublicKey* GetIssuer();
    virtual const ECCPublicKey* GetSubject();
    virtual const ValidPeriod* GetValidity();
    virtual const bool IsDelegate();
    virtual const uint8_t* GetExternalDataDigest();
    virtual const ECCSignature* GetSig();

    virtual String GetEncoded();
    virtual QStatus LoadEncoded(const String& encoded);

    String ToString();
    virtual ~CertificateECC();

    static const size_t GUILD_ID_LEN = 16;   /* the Guild ID length */
  private:
    bool generic;
    uint8_t* buf;
    size_t bufLen;
};

/**
 * CertificateType0 Class
 */
class CertificateType0 : public CertificateECC {

  public:
    CertificateType0() : CertificateECC(0, false)
    {
    }
    CertificateType0(const ECCPublicKey* issuer, const uint8_t* externalDigest);

    const ECCPublicKey* GetIssuer()
    {
        return &signable.issuer;
    }
    void SetIssuer(const ECCPublicKey* issuer);

    const uint8_t* GetExternalDataDigest()
    {
        return signable.digest;
    }
    void SetExternalDataDigest(const uint8_t* externalDataDigest);

    const ECCSignature* GetSig()
    {
        return &sig;
    }
    void SetSig(const ECCSignature* sig);

    /**
     * sign the certificate
     * @param dsaPrivateKey the issuer private key to sign the cert
     */
    QStatus Sign(const ECCPrivateKey* dsaPrivateKey);

    bool VerifySignature();

    String GetEncoded();
    QStatus LoadEncoded(const String& encoded);

    String ToString();

    ~CertificateType0()
    {
    }

  private:

    struct Signable {
        ECCPublicKey issuer;
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    };
    Signable signable;
    ECCSignature sig;
};

/**
 * CertificateType1 Class
 */
class CertificateType1 : public CertificateECC {

  public:
    CertificateType1() : CertificateECC(1, false)
    {
        signable.delegate = false;
    }
    CertificateType1(const ECCPublicKey* issuer, const ECCPublicKey* subject);

    const ECCPublicKey* GetIssuer()
    {
        return &signable.issuer;
    }

    void SetIssuer(const ECCPublicKey* issuer);

    const ECCPublicKey* GetSubject()
    {
        return &signable.subject;
    }

    void SetSubject(const ECCPublicKey* subject);

    const ValidPeriod* GetValidity()
    {
        return &signable.validity;
    }

    void SetValidity(const ValidPeriod* validPeriod)
    {
        signable.validity = *validPeriod;
    }

    const bool IsDelegate()
    {
        return signable.delegate;
    }

    void SetDelegate(bool enabled)
    {
        signable.delegate = enabled;
    }

    const uint8_t* GetExternalDataDigest()
    {
        return signable.digest;
    }
    void SetExternalDataDigest(const uint8_t* externalDataDigest);

    const ECCSignature* GetSig()
    {
        return &sig;
    }
    void SetSig(const ECCSignature* sig);

    QStatus Sign(const ECCPrivateKey* dsaPrivateKey);

    bool VerifySignature();

    String GetEncoded();
    QStatus LoadEncoded(const String& encoded);

    String ToString();

    ~CertificateType1()
    {
    }

  private:
    QStatus GenSignable(uint8_t* digest, size_t len);

    struct Signable {
        ECCPublicKey issuer;
        ECCPublicKey subject;
        ValidPeriod validity;
        uint8_t delegate;
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    };
    Signable signable;
    ECCSignature sig;
};

/**
 * CertificateType2 Class
 */
class CertificateType2 : public CertificateECC {

  public:
    CertificateType2() : CertificateECC(2, false)
    {
        signable.delegate = false;
    }
    CertificateType2(const ECCPublicKey* issuer, const ECCPublicKey* subject);

    const ECCPublicKey* GetIssuer()
    {
        return &signable.issuer;
    }
    void SetIssuer(const ECCPublicKey* issuer);
    const ECCPublicKey* GetSubject()
    {
        return &signable.subject;
    }
    void SetSubject(const ECCPublicKey* subject);

    const ValidPeriod* GetValidity()
    {
        return &signable.validity;
    }

    void SetValidity(const ValidPeriod* validPeriod)
    {
        signable.validity.validFrom = validPeriod->validFrom;
        signable.validity.validTo = validPeriod->validTo;
    }

    const bool IsDelegate()
    {
        return signable.delegate;
    }

    void SetDelegate(bool enabled)
    {
        signable.delegate = enabled;
    }
    const uint8_t* GetGuild()
    {
        return signable.guild;
    }
    void SetGuild(const uint8_t* newGuild, size_t guildLen);

    const uint8_t* GetExternalDataDigest()
    {
        return signable.digest;
    }
    void SetExternalDataDigest(const uint8_t* externalDataDigest);

    const ECCSignature* GetSig()
    {
        return &sig;
    }
    void SetSig(const ECCSignature* sig);
    QStatus Sign(const ECCPrivateKey* dsaPrivateKey);
    bool VerifySignature();

    String GetEncoded();
    QStatus LoadEncoded(const String& encoded);

    String ToString();

    ~CertificateType2()
    {
    }

  private:
    QStatus GenSignable(uint8_t* digest, size_t len);

    struct Signable {
        ECCPublicKey issuer;
        ECCPublicKey subject;
        ValidPeriod validity;
        uint8_t delegate;
        uint8_t guild[GUILD_ID_LEN];
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    };
    Signable signable;
    ECCSignature sig;

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
 * Retrieve the leaf cert from a PEM string representing a cert chain.
 * @param encoded the input string holding the PEM string
 * @param[out] cert the output leaf cert
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_GetLeafCert(const String& encoded, CertificateECC& cert);

/**
 * Count the number of certificates in a PEM string representing a cert chain.
 * @param encoded the input string holding the PEM string
 * @param[out] count the number of certs
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_GetCertCount(const String& encoded, size_t* count);

/**
 * Get the certificate version from the PEM string
 * @param encoded the input string holding the PEM string
 * @param[out] certVersion the buffer holding the certificate version
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_GetVersionFromEncoded(const String& encoded, uint32_t* certVersion);

/**
 * Retrieve the number of certificates in a PEM string representing a cert chain.
 * @param encoded the input string holding the PEM string
 * @param[in,out] certChain the input string holding the array of certs.  The array elements needs to be deallocated after used.
 * @param[in] count the expected number of certs
 * @return ER_OK for sucess; otherwise, error code.
 */
QStatus CertECCUtil_GetCertChain(const String &encoded, CertificateECC * certChain[], size_t count);

} /* namespace qcc */

#endif
