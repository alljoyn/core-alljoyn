#ifndef _CERTIFICATE_ECC_H_
#define _CERTIFICATE_ECC_H_
/**
 * @file
 *
 * Certificate ECC utilities
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

#include <assert.h>
#include <qcc/platform.h>
#include <qcc/CryptoECC.h>

namespace qcc {

/**
 * @defgroup 509_oids X.509 OIDs
 *
 * The X.509 object identifiers OIDs
 * @{
 */
/**
 * The sha256ECDSA Hash Algorithm OID (1.2.840.10045.4.3.2)
 */
extern const qcc::String OID_SIG_ECDSA_SHA256;
/**
 * The ECC Public Key OID (1.2.840.10045.4.3.2)
 */
extern const qcc::String OID_KEY_ECC;
/**
 * ECDSA_P256 Public Key OID (1.2.840.10045.3.1.7)
 */
extern const qcc::String OID_CRV_PRIME256V1;
/**
 * Organization Unit Name OID (2.5.4.11)
 */
extern const qcc::String OID_DN_OU;
/**
 * Common Name OID (2.5.4.3)
 */
extern const qcc::String OID_DN_CN;
/**
 * Basic Contraints OID (2.5.29.19)
 */
extern const qcc::String OID_BASIC_CONSTRAINTS;
/**
 * The sha256NoSign Hash Algorithm OID (2.16.840.1.101.3.4.2.1)
 */
extern const qcc::String OID_DIG_SHA256;
/** @} */
/**
 * X.509 Certificate
 */
class CertificateX509 {

  public:

    /**
     * The validity period
     */
    struct ValidPeriod {
        uint64_t validFrom; /**< the date time when the cert becomes valid
                                expressed in the number of seconds in EPOCH Jan 1, 1970 */
        uint64_t validTo;  /**< the date time after which the cert becomes invalid
                                expressed in the number of seconds in EPOCH Jan 1, 1970 */
    };

    /**
     * encoding format
     */
    typedef enum {
        ENCODING_X509_DER = 0,     ///< X.509 DER format
        ENCODING_X509_DER_PEM = 1  ///< X.509 DER PEM format
    } EncodingType;

    /**
     * Default Constructor
     */
    CertificateX509() : encodedLen(0), encoded(NULL), ca(0)
    {
    }

    /**
     * Decode a PEM encoded certificate.
     * @param pem the encoded certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus DecodeCertificatePEM(const qcc::String& pem);

    /**
     * Export the certificate as PEM encoded.
     * @param[out] pem the encoded certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus EncodeCertificatePEM(qcc::String& pem);

    /**
     * Helper function to generate PEM encoded string using a DER encoded string.
     * @param der the encoded certificate.
     * @param[out] pem the encoded certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    static QStatus AJ_CALL EncodeCertificatePEM(qcc::String& der, qcc::String& pem);

    /**
     * Decode a DER encoded certificate.
     * @param der the encoded certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus DecodeCertificateDER(const qcc::String& der);

    /**
     * Export the certificate as DER encoded.
     * @param[out] der the encoded certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus EncodeCertificateDER(qcc::String& der);

    /**
     * Encode the private key in a PEM string.
     * @param privateKey the private key to encode
     * @param len the private key length
     * @param[out] encoded the output string holding the resulting PEM string
     * @return ER_OK for sucess; otherwise, error code.
     */
    static QStatus AJ_CALL EncodePrivateKeyPEM(const uint8_t* privateKey, size_t len, String& encoded);

    /**
     * Decode the private from a PEM string.
     * @param encoded the input string holding the PEM string
     * @param[out] privateKey the output private key
     * @param len the private key length
     * @return ER_OK for sucess; otherwise, error code.
     */
    static QStatus AJ_CALL DecodePrivateKeyPEM(const String& encoded, uint8_t* privateKey, size_t len);

    /**
     * Encode the public key in a PEM string.
     * @param publicKey the public key to encode
     * @param len the public key length
     * @param[out] encoded the output string holding the resulting PEM string
     * @return ER_OK for sucess; otherwise, error code.
     */
    static QStatus AJ_CALL EncodePublicKeyPEM(const uint8_t* publicKey, size_t len, String& encoded);

    /**
     * Decode the public from a PEM string.
     * @param encoded the input string holding the PEM string
     * @param[out] publicKey the output public key
     * @param len the public key length
     * @return ER_OK for sucess; otherwise, error code.
     */
    static QStatus AJ_CALL DecodePublicKeyPEM(const String& encoded, uint8_t* publicKey, size_t len);

    /**
     * Sign the certificate.
     * @param key the ECDSA private key.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus Sign(const ECCPrivateKey* key);

    /**
     * Verify a self-signed certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus Verify();

    /**
     * Verify the certificate.
     * @param key the ECDSA public key.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus Verify(const ECCPublicKey* key);

    /**
     * Verify the vadility period of the certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus VerifyValidity();

    /**
     * Set the serial number field
     * @param serial the serial number
     */
    void SetSerial(const qcc::String& serial)
    {
        this->serial = serial;
    }

    /**
     * Get the serial number
     * @return the serial number
     */
    const qcc::String& GetSerial() const
    {
        return serial;
    }
    /**
     * Set the issuer organization unit field
     * @param ou the organization unit
     * @param len the length of the organization unit field
     */
    void SetIssuerOU(const uint8_t* ou, size_t len)
    {
        issuer.SetOU(ou, len);
    }
    /**
     * Get the length of the issuer organization unit field
     * @return the length of the issuer organization unit field
     */
    const size_t GetIssuerOULength() const
    {
        return issuer.ouLen;
    }

    /**
     * Get the issuer organization unit field
     * @return the issuer organization unit field
     */
    const uint8_t* GetIssuerOU() const
    {
        return issuer.ou;
    }
    /**
     * Set the issuer common name field
     * @param cn the common name
     * @param len the length of the common name field
     */
    void SetIssuerCN(const uint8_t* cn, size_t len)
    {
        issuer.SetCN(cn, len);
    }
    /**
     * Get the length of the issuer common name field
     * @return the length of the issuer common name field
     */
    const size_t GetIssuerCNLength() const
    {
        return issuer.cnLen;
    }
    /**
     * Get the issuer common name field
     * @return the issuer common name field
     */
    const uint8_t* GetIssuerCN() const
    {
        return issuer.cn;
    }
    /**
     * Set the subject organization unit field
     * @param ou the organization unit
     * @param len the length of the organization unit field
     */
    void SetSubjectOU(const uint8_t* ou, size_t len)
    {
        subject.SetOU(ou, len);
    }
    /**
     * Get the length of the subject organization unit field
     * @return the length of the subject organization unit field
     */
    const size_t GetSubjectOULength() const
    {
        return subject.ouLen;
    }

    /**
     * Get the subject organization unit field
     * @return the subject organization unit field
     */
    const uint8_t* GetSubjectOU() const
    {
        return subject.ou;
    }
    /**
     * Set the subject common name field
     * @param cn the common name
     * @param len the length of the common name field
     */
    void SetSubjectCN(const uint8_t* cn, size_t len)
    {
        subject.SetCN(cn, len);
    }
    /**
     * Get the length of the subject common name field
     * @return the length of the subject common name field
     */
    const size_t GetSubjectCNLength() const
    {
        return subject.cnLen;
    }
    /**
     * Get the subject common name field
     * @return the subject common name field
     */
    const uint8_t* GetSubjectCN() const
    {
        return subject.cn;
    }
    /**
     * Set the validity field
     * @param validPeriod the validity period
     */
    void SetValidity(const ValidPeriod* validPeriod)
    {
        validity.validFrom = validPeriod->validFrom;
        validity.validTo = validPeriod->validTo;
    }
    /**
     * Get the validity period.
     * @return the validity period
     */
    const ValidPeriod* GetValidity() const
    {
        return &validity;
    }
    /**
     * Set the subject public key field
     * @param key the subject public key
     */
    void SetSubjectPublicKey(const ECCPublicKey* key)
    {
        publickey = *key;
    }
    /**
     * Get the subject public key
     * @return the subject public key
     */
    const ECCPublicKey* GetSubjectPublicKey() const
    {
        return &publickey;
    }

    /**
     * Indicate that the subject may act as a certificate authority.
     * @param flag flag indicating the subject may act as a CA
     */
    void SetCA(bool flag)
    {
        ca = flag;
    }

    /**
     * Can the subject act as a certificate authority?
     * @return true if so.
     */
    const bool IsCA() const
    {
        return ca;
    }

    /**
     * Get the encoded bytes for the certificate
     * @return the encoded bytes
     */
    const uint8_t* GetEncoded();

    /**
     * Get the length of the encoded bytes for the certificate
     * @return the length of the encoded bytes
     */
    size_t GetEncodedLen();

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
     * @param PEM the encoded bytes
     * @return ER_OK for sucess; otherwise, error code.
     */
    QStatus LoadPEM(const String& PEM);
    /**
     * Returns a human readable string for a cert if there is one associated with this key.
     *
     * @return A string for the cert or and empty string if there is no cert.
     */
    qcc::String ToString() const;

    /**
     * Destructor
     */
    ~CertificateX509()
    {
        delete [] encoded;
    }

    /**
     * Retrieve the number of X.509 certificates in a PEM string representing a cert chain.
     * @param encoded the input string holding the PEM string
     * @param[in,out] certChain the input string holding the array of certs.
     * @param[in] count the expected number of certs
     * @return ER_OK for sucess; otherwise, error code.
     */
    static QStatus AJ_CALL DecodeCertChainPEM(const String& encoded, CertificateX509* certChain, size_t count);

  private:

    /**
     * Assignment operator is private
     */
    CertificateX509& operator=(const CertificateX509& other);

    /**
     * Copy constructor is private
     */
    CertificateX509(const CertificateX509& other);

    struct DistinguishedName {
        uint8_t* ou;
        size_t ouLen;
        uint8_t* cn;
        size_t cnLen;

        DistinguishedName() : ou(NULL), ouLen(0), cn(NULL), cnLen(0)
        {
        }

        void SetOU(const uint8_t* ou, size_t len)
        {
            ouLen = len;
            delete [] this->ou;
            this->ou = new uint8_t[len];
            memcpy(this->ou, ou, len);
        }
        void SetCN(const uint8_t* cn, size_t len)
        {
            cnLen = len;
            delete [] this->cn;
            this->cn = new uint8_t[len];
            memcpy(this->cn, cn, len);
        }
        ~DistinguishedName()
        {
            delete [] ou;
            delete [] cn;
        }
      private:
        /* private copy constructor to prevent double freeing of dynamic memory */
        DistinguishedName(const DistinguishedName&);
        /* private assign operator to prevent double freeing of dynamic memory */
        DistinguishedName& operator=(const DistinguishedName&);
    };

    QStatus DecodeCertificateTBS();
    QStatus EncodeCertificateTBS();
    QStatus DecodeCertificateName(const qcc::String& dn, CertificateX509::DistinguishedName& name);
    QStatus EncodeCertificateName(qcc::String& dn, CertificateX509::DistinguishedName& name);
    QStatus DecodeCertificateTime(const qcc::String& time);
    QStatus EncodeCertificateTime(qcc::String& time);
    QStatus DecodeCertificatePub(const qcc::String& pub);
    QStatus EncodeCertificatePub(qcc::String& pub);
    QStatus DecodeCertificateExt(const qcc::String& ext);
    QStatus EncodeCertificateExt(qcc::String& ext);
    QStatus DecodeCertificateSig(const qcc::String& sig);
    QStatus EncodeCertificateSig(qcc::String& sig);
    QStatus GenEncoded();

    qcc::String tbs;
    size_t encodedLen;
    uint8_t* encoded;

    qcc::String serial;
    DistinguishedName issuer;
    DistinguishedName subject;
    ValidPeriod validity;
    ECCPublicKey publickey;
    ECCSignature signature;
    uint32_t ca;
};

} /* namespace qcc */

#endif
