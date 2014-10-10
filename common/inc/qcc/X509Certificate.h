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

#ifndef X509CERTIFICATE_H_
#define X509CERTIFICATE_H_

#include <qcc/CertificateECC.h>

namespace qcc {
enum class CertificateType :
    std::int8_t {
    UNSUPPORTED_CERTIFICATE,
    IDENTITY_CERTIFICATE,
    MEMBERSHIP_CERTIFICATE,
    USER_EQUIVALENCE_CERTIFICATE,
    POLICY_CERTIFICATE
};

class X509CertificateECC :
    public CertificateECC {
  public:
    const qcc::String& GetSerialNumber() const;

    void SetSerialNumber(const qcc::String& serialNumber);

    const qcc::String& GetApplicationID() const;

    void SetApplicationID(const qcc::String& newAppId);

    CertificateType GetType() const;

    const qcc::String& GetIssuerName() const;

    void SetIssuerName(const qcc::String& issuerName);

    const ECCPublicKey* GetIssuer();

    void SetIssuer(const ECCPublicKey* issuer);

    const qcc::String& GetDataDigest() const;

    void SetDataDigest(const qcc::String& digest);

    virtual QStatus LoadPEM(const String& PEM);
    virtual String GetPEM();

    virtual const ECCPublicKey* GetSubject();
    void SetSubject(const ECCPublicKey* key);

    virtual const ValidPeriod* GetValidity();
    void SetValidity(const ValidPeriod* validityPeriod);

  protected:
    X509CertificateECC(CertificateType type);

  private:
    CertificateType certType;
    qcc::String appId;
    qcc::String serialNumber;
    qcc::String issuerName;
    qcc::String dataDigest;
    qcc::String pemEncodedCertificate;
    qcc::ECCPublicKey subject;
    qcc::ECCPublicKey issuer;
    qcc::Certificate::ValidPeriod validity;
};

class IdentityCertificate :
    public X509CertificateECC {
  public:
    IdentityCertificate();

    ~IdentityCertificate();

    const qcc::String& GetAlias() const;

    void SetAlias(const qcc::String& alias);

  private:
    qcc::String alias;
};

class MemberShipCertificate :
    public X509CertificateECC {
  public:
    MemberShipCertificate();
    ~MemberShipCertificate();

    const qcc::String& GetGuildId() const;

    void SetGuildId(const qcc::String& guildId);

    bool const IsDelegate();

    void SetDelegate(bool delegate);

  private:
    qcc::String guildID;
    bool delegate;
};

class UserEquivalenceCertificate :
    public X509CertificateECC {
  public:
    UserEquivalenceCertificate();
    ~UserEquivalenceCertificate();
};

class PolicyCertificate :
    public X509CertificateECC {
  public:
    PolicyCertificate();
    ~PolicyCertificate();
};
}  // namespace qcc

#endif /* X509CERTIFICATE_H_ */
