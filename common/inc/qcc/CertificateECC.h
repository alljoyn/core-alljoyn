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
 * custom OID for the Allseen certificate type (1.3.6.1.4.1.44924.1.1)
 */
extern const qcc::String OID_CUSTOM_CERT_TYPE;
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
        UNKNOWN_CERTIFICATE,
        IDENTITY_CERTIFICATE,
        MEMBERSHIP_CERTIFICATE
    } CertificateType;

    /**
     * Default Constructor
     */
    CertificateX509() : type(UNKNOWN_CERTIFICATE), encodedLen(0), encoded(NULL), ca(0)
    {
    }

    /**
     * Constructor
     * @param type the certificate type.
     */
    CertificateX509(CertificateType type) : type(type), encodedLen(0), encoded(NULL), ca(0)
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
     * Verify the vadility period of the certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus VerifyValidity() const;

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

    void SetAuthorityKeyId(const qcc::String& authorityKeyId)
    {
        aki = authorityKeyId;
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
     * @param encoded the encoded bytes
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
    virtual ~CertificateX509()
    {
        delete [] encoded;
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
     * @return ER_OK for sucess; otherwise, error code.
     */
    static QStatus AJ_CALL DecodeCertChainPEM(const String& encoded, CertificateX509* certChain, size_t count);

    /**
     * Copy Constructor for CertificateX509
     *
     * @param[in] other    CertificateX509 to copy
     */
    CertificateX509(const CertificateX509& other) :
        type(other.type), tbs(other.tbs), encodedLen(other.encodedLen),
        serial(other.serial), issuer(other.issuer), subject(other.subject),
        validity(other.validity), publickey(other.publickey),
        signature(other.signature), ca(other.ca), digest(other.digest),
        subjectAltName(other.subjectAltName), aki(other.aki) {
        encoded = new uint8_t[encodedLen];
        memcpy(encoded, other.encoded, encodedLen);
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
            encodedLen = other.encodedLen;
            delete[] encoded;
            encoded = new uint8_t[encodedLen];
            memcpy(encoded, other.encoded, encodedLen);

            serial = other.serial;
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

  private:

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
    QStatus GenEncoded();

    CertificateType type;
    qcc::String tbs;
    size_t encodedLen;
    uint8_t* encoded;

    qcc::String serial;
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
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

class BaseCertificateData {
  public:
    virtual ~BaseCertificateData()
    {
    }

    /**
     * Set the name of the issuer of this certificate. This can be the key identifier of the subject public key
     * @param name the issuer friendly name.
     */
    virtual void SetIssuerName(const qcc::String& name) = 0;
    /**
     * Get the name of the issuer of this certificate. This can be the key identifier of the subject public key
     * @return the issuer name.
     */
    virtual const qcc::String GetIssuerName() const = 0;

    /**
     * Set the validity field
     * @param validPeriod the validity period
     */
    virtual void SetValidity(const CertificateX509::ValidPeriod* validPeriod) = 0;
    /**
     * Get the validity period.
     * @return the validity period
     */
    virtual const CertificateX509::ValidPeriod* GetValidity() const = 0;

    /**
     * Set the name of the subject of this certificate. This can be the key identifier of the subject public key
     * @param name the user friendly name.
     */
    virtual void SetSubjectName(const qcc::String& name) = 0;
    /**
     * Set the name of the subject of this certificate. This can be the key identifier of the subject public key
     * @return the subject name.
     */
    virtual const qcc::String GetSubjectName() const = 0;

    /**
     * Set the subject public key field
     * @param key the subject public key
     */
    virtual void SetSubjectPublicKey(const ECCPublicKey* key) = 0;
    /**
     * Get the subject public key
     * @return the subject public key
     */
    virtual const ECCPublicKey* GetSubjectPublicKey() const = 0;

    /**
     * Check if this certificate allows delegation
     * @return true if the certificate has delegation rights.
     */
    virtual bool HasDelegateRights() const = 0;
    /**
     * Updates the delegation rights of this certificate
     * @param canDelegate true to give this certificate delegation rights.
     */
    virtual void SetDelegateRights(bool canDelegate) = 0;
    /**
     * Get the serial number
     * @return the serial number
     */
    virtual const qcc::String& GetSerial() const = 0;
    /**
     * Set the serial number field
     * @param serial the serial number
     */
    virtual void SetSerial(const qcc::String& serial) = 0;

    /**
     * Get the Issuer Key identity. number field
     * @return the key id of the issuer
     */
    virtual const qcc::String& GetAuthorityKeyId() const = 0;
    /**
     * Set the Issuer Key identity. number field
     * @param authorityKeiId the new key id.
     */
    virtual void SetAuthorityKeyId(const qcc::String authorityKeiId) = 0;
};

class MembershipCertificateData : public virtual BaseCertificateData {
  public:
    /**
     * Destructor
     */
    virtual ~MembershipCertificateData()
    {
    }

    /**
     * Check if a guild is set for this certificate.
     *
     * @return true if this certificate has a valid guild set, false otherwise
     */
    virtual bool IsGuildSet() const = 0;
    /**
     * Set the guild GUID
     * @param guid the guild GUID
     */
    virtual void SetGuild(const qcc::GUID128& guid) = 0;

    /**
     * Get the guild GUID
     * @return the guild GUID
     */
    virtual const qcc::GUID128& GetGuild() = 0;
};

class IdentityCertificateData : public virtual BaseCertificateData {
  public:
    virtual ~IdentityCertificateData()
    {
    }

    /**
     * Set the user friendly name of of this identity.
     * @param name the user friendly name.
     */
    virtual void SetUserFriendlyName(const qcc::String& name) = 0;
    /**
     * Get the user friendly name of of this identity.
     * @return the user friendly name.
     */
    virtual const qcc::String GetUserFriendlyName() const = 0;

    /**
     * Set the alias field
     * @param alias the alias
     */
    virtual void SetAlias(const qcc::String& alias) = 0;
    /**
     * Get the alias field
     * @return the alias
     */
    virtual const qcc::String& GetAlias() const = 0;

    /**
     * Sets the manifest data for this identity certificate.
     * @param digest the digest data.
     * @param digestSize the size of the digest.
     */
    virtual void SetManifestDigest(const uint8_t* digest, size_t digestSize) = 0;

    /**
     * Gets the manifest digest of this identity certificate.
     * @param[out] digestSize stores the digest size on a successful return.
     * @return the digest linked to this certificate or NULL if no digest is set
     */
    virtual const uint8_t* GetManifestDigest(size_t& digestSize) const = 0;


};

class BaseCertificate : public virtual BaseCertificateData {
  public:
    BaseCertificate() : x509(), encodedData(NULL), encodedDataSize(0)
    {

    }
    BaseCertificate(CertificateX509::CertificateType type) : x509(type), encodedData(NULL), encodedDataSize(0)
    {
    }

    ~BaseCertificate()
    {
        delete[] encodedData;
    }

    /**
     * Returns a human readable string for a cert if there is one associated with this key.
     *
     * @return A string for the cert or and empty string if there is no cert.
     */
    qcc::String ToString() const {
        return x509.ToString();
    }

    /**
     * Gets a reference to the encapsulated X509 certificate. This should only be called if you need
     * detailed access to the X509 layout of the certificate.
     * @return a reference to the internal CertificateX509 certificate.
     */
    CertificateX509& GetCertificateX509() {
        return x509;
    }

    /**
     * Encodes this certificate in DER format.
     *
     * @param[out] der stores the DER encoded data.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus EncodeCertificateDER(qcc::String& der) const;

    /**
     * Decode a PEM encoded certificate.
     * @param pem the encoded certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus DecodeCertificatePEM(const qcc::String& pem) {
        return x509.DecodeCertificatePEM(pem);
    }

    /**
     * Decode a DER encoded certificate.
     * @param der the encoded certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus DecodeCertificateDER(const qcc::String& der);

    /**
     * Gets the encoded certificate data (in DER).
     * @param[out] encodedSize will on successful return store the length of the encoded data.
     * @return A reference to the internal cached DER encoded value (this pointer remains owned
     *    by the certificate) or NULL in case of an error.
     */
    const uint8_t* GetEncoded(size_t& encodedSize) const;

    /**
     * Verify the certificate.
     * @param key the ECDSA public key.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus Verify(const qcc::ECCPublicKey* key) const {
        return x509.Verify(key);
    }

    /**
     * Verify the vadility period of the certificate.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus VerifyValidity() const {
        return x509.VerifyValidity();
    }

    /**
     * Sign the certificate. Make sure to call SetAuthorityKeyId if needed.
     * @param key the ECDSA private key.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus Sign(const ECCPrivateKey* key) {
        return x509.Sign(key);
    }

    /**
     * Sign the certificate and generate the authority key identifier.
     * @param privateKey the ECDSA private key.
     * @param publicKey the ECDSA public key to generate the authority key
     *                  identifier.
     * @return ER_OK for success; otherwise, error code.
     */
    QStatus SignAndGenerateAuthorityKeyId(const ECCPrivateKey* privateKey, const ECCPublicKey* publicKey) {
        return x509.SignAndGenerateAuthorityKeyId(privateKey, publicKey);
    }

    /**
     * Set the name of the issuer of this certificate. This can be the key identifier of the issuer public key
     * @param name a user friendly name.
     */
    virtual void SetIssuerName(const qcc::String& name)
    {
        x509.SetIssuerCN((const uint8_t*) name.data(), name.size());
    }
    /**
     * Gets the name of the issuer of this certificate. This can be the key identifier of the issuer public key
     * @return the issuer name.
     */
    virtual const qcc::String GetIssuerName() const
    {
        size_t len = x509.GetIssuerCNLength();
        if (len > 0) {
            qcc::String name((const char*) x509.GetIssuerCN(), len);
            return name;
        }
        return "";
    }
    /**
     * Set the validity field
     * @param validPeriod the validity period
     */
    void SetValidity(const CertificateX509::ValidPeriod* validPeriod)
    {
        x509.SetValidity(validPeriod);
    }
    /**
     * Get the validity period.
     * @return the validity period
     */
    virtual const CertificateX509::ValidPeriod* GetValidity() const
    {
        return x509.GetValidity();
    }

    /**
     * Set the name of the subject of this identity. This can be the key identifier of the subject public key
     * @param name the user friendly name.
     */
    virtual void SetSubjectName(const qcc::String& name)
    {
        x509.SetSubjectCN((const uint8_t*) name.data(), name.size());
    }
    /**
     * Set the name of the subject of this identity. This can be the key identifier of the subject public key
     * @return the subject name.
     */
    virtual const qcc::String GetSubjectName() const
    {
        return qcc::String((const char*) x509.GetSubjectCN(), x509.GetSubjectCNLength());
    }

    /**
     * Set the subject public key field
     * @param key the subject public key
     */
    virtual void SetSubjectPublicKey(const ECCPublicKey* key)
    {
        x509.SetSubjectPublicKey(key);
    }
    /**
     * Get the subject public key
     * @return the subject public key
     */
    virtual const ECCPublicKey* GetSubjectPublicKey() const
    {
        return x509.GetSubjectPublicKey();
    }

    /**
     * Check if this certificate allows delegation
     * @return true if the certificate has delegation rights.
     */
    virtual bool HasDelegateRights() const
    {
        return x509.IsCA();
    }
    /**
     * Updates the delegation rights of this certificate
     * @param canDelegate true to give this certificate delegation rights.
     */
    virtual void SetDelegateRights(bool canDelegate)
    {
        x509.SetCA(canDelegate);
    }
    /**
     * Get the serial number
     * @return the serial number
     */
    const qcc::String& GetSerial() const
    {
        return x509.GetSerial();
    }
    /**
     * Set the serial number field
     * @param serial the serial number
     */
    virtual void SetSerial(const qcc::String& serial)
    {
        x509.SetSerial(serial);
    }

    /**
     * Get the Issuer Key identity. number field
     * @return the key id of the issuer
     */
    virtual const qcc::String& GetAuthorityKeyId() const
    {
        return x509.GetAuthorityKeyId();
    }
    /**
     * Set the Issuer Key identity. number field
     * @param authorityKeiId the new key id.
     */
    virtual void SetAuthorityKeyId(const qcc::String authorityKeiId) {
        x509.SetAuthorityKeyId(authorityKeiId);
    }
  protected:
    CertificateX509 x509;
  private:
    mutable uint8_t* encodedData;
    mutable size_t encodedDataSize;
};

class MembershipCertificate : public virtual MembershipCertificateData, public BaseCertificate {
  public:
    MembershipCertificate() : BaseCertificate(CertificateX509::MEMBERSHIP_CERTIFICATE), guildGUID(0), guildSet(false)
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
        return guildSet;
    }

    /**
     * Set the guild GUID
     * @param guid the guild GUID
     */
    void SetGuild(const qcc::GUID128& guid)
    {
        guildGUID = guid;
        qcc::String sgId((const char*) guid.GetBytes(), guid.SIZE);
        x509.SetSubjectAltName(sgId);
        guildSet = true;
    }

    /**
     * Get the guild GUID
     * @return the guild GUID
     */
    const qcc::GUID128& GetGuild()
    {
        if (!guildSet) {
            qcc::String sgId = x509.GetSubjectAltName();
            if (sgId.size() > 0) {
                guildGUID.SetBytes((const uint8_t*) sgId.data());
                guildSet = true;
            }
        }
        return guildGUID;
    }

  private:
    qcc::GUID128 guildGUID;
    bool guildSet;
};

class IdentityCertificate : public virtual IdentityCertificateData, public BaseCertificate {
  public:
    IdentityCertificate() : BaseCertificate(CertificateX509::IDENTITY_CERTIFICATE)
    {
    }
    /**
     * Destructor
     */
    virtual ~IdentityCertificate()
    {
    }

    /**
     * Set the user friendly name of of this identity.
     * @param name the user friendly name.
     */
    virtual void SetUserFriendlyName(const qcc::String& name)
    {
        x509.SetSubjectOU((const uint8_t*) name.data(), name.size());
    }
    /**
     * Get the user friendly name of of this identity.
     * @return the user friendly name.
     */
    virtual const qcc::String GetUserFriendlyName() const
    {
        return qcc::String((const char*) x509.GetSubjectOU(), x509.GetSubjectCNLength());
    }
    /**
     * Set the alias field
     * @param alias the alias
     */
    void SetAlias(const qcc::String& alias)
    {
        x509.SetSubjectAltName(alias);
    }

    /**
     * Get the alias field
     * @return the alias
     */
    const qcc::String& GetAlias() const
    {
        return x509.GetSubjectAltName();
    }

    virtual void SetManifestDigest(const uint8_t* digest, size_t digestSize)
    {
        x509.SetDigest(digest, digestSize);
    }

    virtual const uint8_t* GetManifestDigest(size_t& digestSize) const
    {
        digestSize = x509.GetDigestSize();
        return x509.GetDigest();
    }
};
#ifdef _WIN32
#pragma warning(pop)
#endif
} /* namespace qcc */

#endif
