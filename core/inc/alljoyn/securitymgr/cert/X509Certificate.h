/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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
enum CertificateType {
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

    const qcc::GUID128& GetApplicationID() const;

    void SetApplicationID(const qcc::GUID128& newAppId);

    CertificateType GetType() const;

    const qcc::String& GetIssuerName() const;

    void SetIssuerName(const qcc::String& issuerName);

    const ECCPublicKey* GetIssuer();

    void SetIssuer(const ECCPublicKey* issuer);

    const qcc::String& GetDataDigest() const;

    void SetDataDigest(const qcc::String& digest);

    virtual QStatus LoadDER(const String& der);

    virtual String GetDER();

    virtual const ECCPublicKey* GetSubject();

    void SetSubject(const ECCPublicKey* key);

    virtual const ValidPeriod* GetValidity();

    void SetValidity(const ValidPeriod* validityPeriod);

  protected:
    X509CertificateECC(CertificateType type);

  private:
    CertificateType certType;
    qcc::GUID128 appId;
    qcc::String serialNumber;
    qcc::String issuerName;
    qcc::String dataDigest;
    qcc::String derEncodedCertificate;
    qcc::ECCPublicKey subject;
    qcc::ECCPublicKey issuer;
    qcc::Certificate::ValidPeriod validity;
};

class X509IdentityCertificate :
    public X509CertificateECC {
  public:
    X509IdentityCertificate();

    ~X509IdentityCertificate();

    const qcc::GUID128& GetAlias() const;

    void SetAlias(const qcc::GUID128& alias);

    const qcc::String& GetName() const;

    void SetName(const qcc::String& newName);

  private:
    qcc::GUID128 alias;
    qcc::String name;
};

class X509MemberShipCertificate :
    public X509CertificateECC {
  public:
    X509MemberShipCertificate();
    ~X509MemberShipCertificate();

    const qcc::String& GetGuildId() const;

    void SetGuildId(const qcc::String& guildId);

    bool const IsDelegate();

    void SetDelegate(bool delegate);

  private:
    qcc::String guildID;
    bool delegate;
};

class X509UserEquivalenceCertificate :
    public X509CertificateECC {
  public:
    X509UserEquivalenceCertificate();
    ~X509UserEquivalenceCertificate();
};

class X509PolicyCertificate :
    public X509CertificateECC {
  public:
    X509PolicyCertificate();
    ~X509PolicyCertificate();
};
}  // namespace qcc

#endif /* X509CERTIFICATE_H_ */
