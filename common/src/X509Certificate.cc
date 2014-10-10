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

#include <qcc/X509Certificate.h>

namespace qcc {
//X509CertificateECC
X509CertificateECC::X509CertificateECC(CertificateType type)
{
    certType = type;
}

CertificateType X509CertificateECC::GetType() const
{
    return certType;
}

const qcc::String& X509CertificateECC::GetIssuerName() const
{
    return issuerName;
}

void X509CertificateECC::SetIssuerName(const qcc::String& issuerName)
{
    this->issuerName = issuerName;
}

const ECCPublicKey* X509CertificateECC::GetIssuer()
{
    return &issuer;
}

void X509CertificateECC::SetIssuer(const ECCPublicKey* issuer)
{
    memcpy(this->issuer.data, issuer->data, sizeof(this->issuer.data));
}

const qcc::String& X509CertificateECC::GetDataDigest() const
{
    return dataDigest;
}

void X509CertificateECC::SetDataDigest(const qcc::String& digest)
{
    dataDigest = digest;
}

const qcc::String& X509CertificateECC::GetApplicationID() const
{
    return appId;
}

void X509CertificateECC::SetApplicationID(const qcc::String& newAppId)
{
    appId = newAppId;
}

const qcc::String& X509CertificateECC::GetSerialNumber() const
{
    return serialNumber;
}

QStatus X509CertificateECC::LoadPEM(const String& PEM)
{
    pemEncodedCertificate = PEM;
    return ER_OK;
}

String X509CertificateECC::GetPEM()
{
    return pemEncodedCertificate;
}

const ECCPublicKey* X509CertificateECC::GetSubject()
{
    return &subject;
}
void X509CertificateECC::SetSubject(const ECCPublicKey* key)
{
    memcpy(subject.data, key->data, sizeof(subject.data));
}

void X509CertificateECC::SetSerialNumber(const qcc::String& newSerialNumber)
{
    serialNumber = newSerialNumber;
}

const qcc::Certificate::ValidPeriod* X509CertificateECC::GetValidity() {
    return &validity;
}

void X509CertificateECC::SetValidity(const qcc::Certificate::ValidPeriod* validityPeriod) {
    validity.validFrom = validityPeriod->validFrom;
    validity.validTo = validityPeriod->validTo;
}

//IdentityCertificate
IdentityCertificate::IdentityCertificate() :
    X509CertificateECC(CertificateType::IDENTITY_CERTIFICATE)
{
}

IdentityCertificate::~IdentityCertificate()
{
}

const qcc::String& IdentityCertificate::GetAlias() const
{
    return alias;
}

void IdentityCertificate::SetAlias(const qcc::String& newAlias)
{
    alias = newAlias;
}

//MembershipCertificate
MemberShipCertificate::MemberShipCertificate() :
    X509CertificateECC(CertificateType::MEMBERSHIP_CERTIFICATE), delegate(false)
{
}

MemberShipCertificate::~MemberShipCertificate()
{
}

const qcc::String& MemberShipCertificate::GetGuildId() const
{
    return guildID;
}

void MemberShipCertificate::SetGuildId(const qcc::String& guildId)
{
    guildID = guildId;
}

const bool MemberShipCertificate::IsDelegate()
{
    return delegate;
}

void MemberShipCertificate::SetDelegate(bool dlgt)
{
    delegate = dlgt;
}

//UserEquivalenceCertificate
UserEquivalenceCertificate::UserEquivalenceCertificate() :
    X509CertificateECC(CertificateType::USER_EQUIVALENCE_CERTIFICATE)
{
}

UserEquivalenceCertificate::~UserEquivalenceCertificate()
{
}

//Policy Certificate
PolicyCertificate::PolicyCertificate() :
    X509CertificateECC(CertificateType::POLICY_CERTIFICATE)
{
}

PolicyCertificate::~PolicyCertificate()
{
}
} //close qcc namespace.
