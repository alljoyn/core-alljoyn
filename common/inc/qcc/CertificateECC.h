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
#include <qcc/GUID.h>
#include <qcc/CryptoECC.h>
#include <qcc/KeyInfoECC.h>

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
/**
 * custom OID for the digest of external data (1.3.6.1.4.1.44924.1.2)
 */
extern const qcc::String OID_CUSTOM_DIGEST;
/**
 * custom OID for the Allseen identity certificate EKU (1.3.6.1.4.1.44924.1.1)
 */
extern const qcc::String OID_CUSTOM_EKU_IDENTITY;
/**
 * custom OID for the Allseen membership certificate EKU (1.3.6.1.4.1.44924.1.5)
 */
extern const qcc::String OID_CUSTOM_EKU_MEMBERSHIP;
/**
 * Authority Key Identifier OID (2.5.29.35)
 */
extern const qcc::String OID_AUTHORITY_KEY_IDENTIFIER;

/**
 * custom OID for the Allseen security group Id (1.3.6.1.4.1.44924.1.3)
 */
extern const qcc::String OID_CUSTOM_SECURITY_GROUP_ID;

/**
 * custom OID for the Allseen identity alias (1.3.6.1.4.1.44924.1.4)
 */
extern const qcc::String OID_CUSTOM_IDENTITY_ALIAS;

/** @} */
/**
 * X.509 Certificate
 */
class CertificateX509 {

  public:

    /**
     * The Authority key identifier size in bytes
     */
    static const size_t AUTHORITY_KEY_ID_SZ = 8;

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

    typedef enum {
        UNRESTRICTED_CERTIFICATE,  ///< Unrestricted certificate
        IDENTITY_CERTIFICATE,      ///< identity certificate
        MEMBERSHIP_CERTIFICATE,    ///< membership certificate
        INVALID_CERTIFICATE        ///< certificate not valid for any AllJoyn purpose
    } CertificateType;

    /**
     * Default Constructor
     */
    CertificateX509() : type(UNRESTRICTED_CERTIFICATE), serialLen(0), serial(NULL), ca(0)
    {
    }

    /**
     * Constructor
     * @param type the certificate type.
     */
    CertificateX509(CertificateType type) : type(type), serialLen(0), serial(NULL), ca(0)
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
    QStatus EncodeCertificatePEM(qcc::String& pem) const;

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
    virtual QStatus DecodeCertificateDER(const qcc::String& der);

    /**
     * Export the certificate as DER encoded.
     * @param[out] der the encoded certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus EncodeCertificateDER(qcc::String& der) const;

    /**
     * Encode the private key in a PEM string.
     *
     * @deprecated May 2015 for 15.08 release
     * @see CertificateX509::EncodePrivateKeyPEM(const ECCPrivateKey* privateKey, String& encoded)
     *
     * @param privateKey the private key to encode
     * @param len the private key length
     * @param[out] encoded the output string holding the resulting PEM string
     * @return ER_OK for success; otherwise, error code.
     */
    QCC_DEPRECATED(static QStatus AJ_CALL EncodePrivateKeyPEM(const uint8_t * privateKey, size_t len, String & encoded));

    /**
     * Encode the private key in a PEM string.
     * @param privateKey the private key to encode
     * @param[out] encoded the output string holding the resulting PEM string
     * @return ER_OK for success; otherwise, error code.
     */
    static QStatus AJ_CALL EncodePrivateKeyPEM(const ECCPrivateKey* privateKey, String& encoded);

    /**
     * Decode the private key from a PEM string.
     *
     * @deprecated May 2015 for 15.08 release
     * @see CertificateX509::DecodePrivateKeyPEM(const String& encode, ECCPrivateKey* privateKey)
     *
     * @param encoded the input string holding the PEM string
     * @param[out] privateKey the output private key
     * @param len the private key length
     * @return ER_OK for success; otherwise, error code.
     */
    QCC_DEPRECATED(static QStatus AJ_CALL DecodePrivateKeyPEM(const String &encoded, uint8_t * privateKey, size_t len));

    /**
     * Decode the private key from a PEM string.
     * @param encoded the input string holding the PEM string
     * @param[out] privateKey the output private key
     * @return ER_OK for success; otherwise, error code.
     */
    static QStatus AJ_CALL DecodePrivateKeyPEM(const String& encoded, ECCPrivateKey* privateKey);

    /**
     * Encode the public key in a PEM string.
     *
     * @deprecated May 2015 for 15.08 release
     * @see CertificateX509::EncodePublicKeyPEM(const ECCPublicKey* publicKey, String& encoded)
     *
     * @param publicKey the public key to encode
     * @param len the public key length
     * @param[out] encoded the output string holding the resulting PEM string
     * @return ER_OK for success; otherwise, error code.
     */
    QCC_DEPRECATED(static QStatus AJ_CALL EncodePublicKeyPEM(const uint8_t * publicKey, size_t len, String & encoded));

    /**
     * Encode the public key in a PEM string.
     * @param publicKey the public key to encode
     * @param[out] encoded the output string holding the resulting PEM string
     * @return ER_OK for success; otherwise, error code.
     */
    static QStatus AJ_CALL EncodePublicKeyPEM(const ECCPublicKey* publicKey, String& encoded);

    /**
     * Decode the public key from a PEM string.
     *
     * @deprecated May 2015 for 15.08 release
     * @see CertificateX509::DecodePublicKeyPEM(const String& encoded, ECCPublicKey* publicKey)
     *
     * @param encoded the input string holding the PEM string
     * @param[out] publicKey the output public key
     * @param len the public key length
     * @return ER_OK for success; otherwise, error code.
     */
    QCC_DEPRECATED(static QStatus AJ_CALL DecodePublicKeyPEM(const String &encoded, uint8_t * publicKey, size_t len));

    /**
     * Decode the public key from a PEM string.
     * @param encoded the input string holding the PEM string
     * @param[out] publicKey the output public key
     * @return ER_OK for success; otherwise, error code.
     */
    static QStatus AJ_CALL DecodePublicKeyPEM(const String& encoded, ECCPublicKey* publicKey);

    /**
     * Sign the certificate.
     * @param key the ECDSA private key.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus Sign(const ECCPrivateKey* key);

    /**
     * Sign the certificate and generate the authority key identifier.
     * @param privateKey the ECDSA private key.
     * @param publicKey the ECDSA public key to generate the authority key
     *                  identifier.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus SignAndGenerateAuthorityKeyId(const ECCPrivateKey* privateKey, const ECCPublicKey* publicKey);

    /**
     * Verify a self-signed certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus Verify() const;

    /**
     * Verify the certificate.
     * @param key the ECDSA public key.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus Verify(const ECCPublicKey* key) const;

    /**
     * Verify the certificate against the trust anchor.
     * @param trustAnchor the trust anchor
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus Verify(const KeyInfoNISTP256& trustAnchor) const;

    /**
     * Verify the validity period of the certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus VerifyValidity() const;

    /**
     * Set the serial number field
     * @param serialNumber The serial number
     * @param len          Length of the serial array
     */
    void SetSerial(const uint8_t* serialNumber, const size_t len)
    {
        serialLen = len;
        delete[] this->serial;
        if ((serialLen > 0) && serialNumber) {
            this->serial = new uint8_t[serialLen];
            memcpy(this->serial, serialNumber, serialLen);
        } else {
            this->serial = NULL;
        }
    }

    /**
     * Get the serial number
     * @return the serial number
     */
    const uint8_t* GetSerial() const
    {
        return serial;
    }

    /**
     * Get the length of the serial number
     * @return Length of the serial number
     */
    const size_t GetSerialLen() const
    {
        return serialLen;
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
     * Set the subject alt name field
     * @param subjectAltName the subject alt name
     */
    void SetSubjectAltName(const qcc::String& subjectAltName)
    {
        this->subjectAltName = subjectAltName;
    }
    /**
     * Get the subject alt name field
     * @return the subject alt name
     */
    const qcc::String& GetSubjectAltName() const
    {
        return subjectAltName;
    }

    /**
     * Generate the authority key identifier.
     * @param issuerPubKey the issuer's public key
     * @param[out] authorityKeyId the authority key identifier
     * @return ER_OK for success; otherwise, error code.
     */
    static QStatus AJ_CALL GenerateAuthorityKeyId(const qcc::ECCPublicKey* issuerPubKey, qcc::String& authorityKeyId);

    /**
     * Generate the issuer authority key identifier for the certificate.
     * @param issuerPubKey the issuer's public key
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus GenerateAuthorityKeyId(const qcc::ECCPublicKey* issuerPubKey);

    const qcc::String& GetAuthorityKeyId() const
    {
        return aki;
    }

    /**
     * Updates the current AuthorityKeyId with a new one.
     *
     * @param[in] newAki the new AuthorityKeyId for this certificate.
     */
    void SetAuthorityKeyId(const qcc::String& newAki)
    {
        aki = newAki;
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
        ca = flag ? 1 : 0;
    }

    /**
     * Can the subject act as a certificate authority?
     * @return true if so.
     */
    const bool IsCA() const
    {
        return ca ? true : false;
    }

    /**
     * Set the digest of the external data.
     * @param digest the digest of the external data
     * @param count the size of the digest.
     */
    void SetDigest(const uint8_t* digest, size_t size)
    {
        this->digest = qcc::String((char*) digest, size);
    }

    /**
     * Get the digest of the external data.
     * @param digest the digest of the external data
     */
    const uint8_t* GetDigest() const
    {
        return (const uint8_t*) digest.data();
    }

    /**
     * Get the size of the digest of the external data.
     * @return the size of the digest of the external data
     */
    const size_t GetDigestSize() const
    {
        return digest.size();
    }

    /**
     * Is the optional digest field present in the certificate?
     * @return whether this optional field is present in the certificate.
     */
    bool IsDigestPresent()
    {
        return !digest.empty();
    }

    /**
     * Get the PEM encoded bytes for the certificate
     * @return the PEM encoded bytes
     */
    String GetPEM();

    /**
     * Load the PEM encoded bytes for the certificate
     * @param PEM the encoded bytes
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus LoadPEM(const String& PEM);
    /**
     * Returns a human readable string for a cert if there is one associated with this key.
     *
     * @return A string for the cert or and empty string if there is no cert.
     */
    qcc::String ToString() const;


    /**
     * Determine if this certificate issued a given certificate by comparing the distinguished
     * name and verifying the digital signature.
     * @param issuedCertificate Certificate to check if it was issued by this certificate.
     * @return true if so.
     */
    bool IsIssuerOf(const CertificateX509& issuedCertificate) const;

    /**
     * Is the subject DN of this certificate equal to a given DN?
     * @param cn Common Name component of the DN to compare to.
     * @param cnLength Length of the cn array. Zero if null.
     * @param ou Organizational Unit component of the DN to compare to.
     * @param ouLength Length of the ou array. Zero if null.
     * @return true if so.
     */
    bool IsDNEqual(const uint8_t* cn, const size_t cnLength, const uint8_t* ou, const size_t ouLength) const;

    /**
     * Is the subject DN of this certificate equal to a given certificate's DN?
     * @param other CertificateX509 to compare to.
     * @return true if so.
     */
    bool IsDNEqual(const CertificateX509& other) const;

    /**
     * Is the subject public key of this certificate equal to a given key?
     * @param publicKey Public key to compare to.
     * @return true if so.
     */
    bool IsSubjectPublicKeyEqual(const ECCPublicKey* publicKey) const;

    /**
     * Destructor
     */
    virtual ~CertificateX509()
    {
        delete [] serial;
    }

    const CertificateType GetType() const
    {
        return type;
    }

    /**
     * Retrieve the number of X.509 certificates in a PEM string representing a cert chain.
     * @param encoded the input string holding the PEM string
     * @param[in,out] certChain the input string holding the array of certs.
     * @param[in] count the expected number of certs
     * @return ER_OK for success; otherwise, error code.
     */
    static QStatus AJ_CALL DecodeCertChainPEM(const String& encoded, CertificateX509* certChain, size_t count);

    /**
     * Copy Constructor for CertificateX509
     *
     * @param[in] other    CertificateX509 to copy
     */
    CertificateX509(const CertificateX509& other) :
        type(other.type), tbs(other.tbs), serial(NULL),
        issuer(other.issuer), subject(other.subject),
        validity(other.validity), publickey(other.publickey),
        signature(other.signature), ca(other.ca), digest(other.digest),
        subjectAltName(other.subjectAltName), aki(other.aki) {
        SetSerial(other.serial, other.serialLen);
    }

    /**
     * Assign operator for CertificateX509
     *
     * @param[in] other    CertificateX509 to assign from
     */
    CertificateX509& operator=(const CertificateX509& other) {
        if (&other != this) {
            type = other.type;
            tbs = other.tbs;

            SetSerial(other.serial, other.serialLen);
            issuer = other.issuer;
            subject = other.subject;
            validity = other.validity;
            publickey = other.publickey;
            signature = other.signature;

            ca = other.ca;
            digest = other.digest;
            subjectAltName = other.subjectAltName;
            aki = other.aki;
        }
        return *this;
    }

    /**
     * Validate the certificate type of each cert in the certificate chain.
     * The end-entity cert must have a type.
     * Any signing cert in the chain must have the same type or unrestricted
     * type in order to sign the next cert in the chain.
     * @param[in] certChain the array of certs.
     * @param[in] count the number of certs
     * @return true if valid; false, otherwise;
     */
    static bool AJ_CALL ValidateCertificateTypeInCertChain(const CertificateX509* certChain, size_t count);

  private:

    struct DistinguishedName {
        uint8_t* ou;
        size_t ouLen;
        uint8_t* cn;
        size_t cnLen;

        DistinguishedName() : ou(NULL), ouLen(0), cn(NULL), cnLen(0)
        {
        }

        void SetOU(const uint8_t* newOu, size_t len)
        {
            ouLen = len;
            delete [] this->ou;
            this->ou = new uint8_t[len];
            memcpy(this->ou, newOu, len);
        }
        void SetCN(const uint8_t* newCn, size_t len)
        {
            cnLen = len;
            delete [] this->cn;
            this->cn = new uint8_t[len];
            memcpy(this->cn, newCn, len);
        }
        ~DistinguishedName()
        {
            delete [] ou;
            delete [] cn;
        }

        DistinguishedName(const DistinguishedName& other) :
            ouLen(other.ouLen), cnLen(other.cnLen) {
            ou = new uint8_t[ouLen];
            memcpy(ou, other.ou, ouLen);
            cn = new uint8_t[cnLen];
            memcpy(cn, other.cn, cnLen);
        }

        DistinguishedName& operator=(const DistinguishedName& other) {
            if (&other != this) {
                ouLen = other.ouLen;
                cnLen = other.cnLen;
                delete[] ou;
                ou = new uint8_t[ouLen];
                memcpy(ou, other.ou, ouLen);
                delete[] cn;
                cn = new uint8_t[cnLen];
                memcpy(cn, other.cn, cnLen);
            }
            return *this;
        }
    };

    QStatus DecodeCertificateTBS();
    QStatus EncodeCertificateTBS();
    QStatus DecodeCertificateName(const qcc::String& dn, CertificateX509::DistinguishedName& name);
    QStatus EncodeCertificateName(qcc::String& dn, const CertificateX509::DistinguishedName& name) const;
    QStatus DecodeCertificateTime(const qcc::String& time);
    QStatus EncodeCertificateTime(qcc::String& time) const;
    QStatus DecodeCertificatePub(const qcc::String& pub);
    QStatus EncodeCertificatePub(qcc::String& pub) const;
    QStatus DecodeCertificateExt(const qcc::String& ext);
    QStatus EncodeCertificateExt(qcc::String& ext) const;
    QStatus DecodeCertificateSig(const qcc::String& sig);
    QStatus EncodeCertificateSig(qcc::String& sig) const;

    CertificateType type;
    qcc::String tbs;

    size_t serialLen;
    uint8_t* serial;
    DistinguishedName issuer;
    DistinguishedName subject;
    ValidPeriod validity;
    ECCPublicKey publickey;
    ECCSignature signature;
    /*
     * Extensions
     */
    uint32_t ca;
    qcc::String digest;
    qcc::String subjectAltName;
    qcc::String aki;
};

/**
 * An X.509 certificate that represents a membership certificate
 */
class MembershipCertificate : public CertificateX509 {

  public:
    MembershipCertificate() : CertificateX509(MEMBERSHIP_CERTIFICATE)
    {
    }
    /**
     * Destructor
     */
    virtual ~MembershipCertificate()
    {
    }

    /**
     * Check if a guild is set for this certificate.
     *
     * @return true if this certificate has a valid guild set, false otherwise
     */
    bool IsGuildSet() const
    {
        return GetSubjectAltName().size() > 0;
    }

    /**
     * Set the guild GUID
     * @param guid the guild GUID
     */
    void SetGuild(const qcc::GUID128& guid)
    {
        qcc::String sgId((const char*) guid.GetBytes(), guid.SIZE);
        SetSubjectAltName(sgId);
    }

    /**
     * Get the guild GUID
     * @return the guild GUID
     */
    GUID128 GetGuild() const
    {
        GUID128 guid(0);
        qcc::String sgId = GetSubjectAltName();
        if (sgId.size() == GUID128::SIZE) {
            guid.SetBytes((const uint8_t*) sgId.data());
        } else if (sgId.size() > 0) {
            uint8_t bytes[GUID128::SIZE];
            memset(bytes, 0, GUID128::SIZE);
            memcpy(bytes, sgId.data(), sgId.size() > GUID128::SIZE ? GUID128::SIZE : sgId.size());
            guid.SetBytes(bytes);
        }
        return guid;
    }
};

/**
 * An X.509 certificate that represents an identity certificate
 */
class IdentityCertificate : public CertificateX509 {

  public:
    IdentityCertificate() : CertificateX509(IDENTITY_CERTIFICATE)
    {
    }
    /**
     * Destructor
     */
    virtual ~IdentityCertificate()
    {
    }

    /**
     * Set the alias field
     * @param alias the alias
     */
    void SetAlias(const qcc::String& alias)
    {
        SetSubjectAltName(alias);
    }

    /**
     * Get the alias field
     * @return the alias
     */
    const qcc::String& GetAlias() const
    {
        return GetSubjectAltName();
    }

};

} /* namespace qcc */

#endif
