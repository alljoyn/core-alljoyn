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

#include <alljoyn/securitymgr/cert/X509Certificate.h>

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
    memcpy(this->issuer.x, issuer->x, sizeof(this->issuer.x));
    memcpy(this->issuer.y, issuer->y, sizeof(this->issuer.y));
}

const qcc::String& X509CertificateECC::GetDataDigest() const
{
    return dataDigest;
}

void X509CertificateECC::SetDataDigest(const qcc::String& digest)
{
    dataDigest = digest;
}

const qcc::GUID128& X509CertificateECC::GetApplicationID() const
{
    return appId;
}

void X509CertificateECC::SetApplicationID(const qcc::GUID128& newAppId)
{
    appId = newAppId;
}

const qcc::String& X509CertificateECC::GetSerialNumber() const
{
    return serialNumber;
}

QStatus X509CertificateECC::LoadDER(const String& der)
{
    derEncodedCertificate = der;
    return ER_OK;
}

String X509CertificateECC::GetDER()
{
    return derEncodedCertificate;
}

const ECCPublicKey* X509CertificateECC::GetSubject()
{
    return &subject;
}

void X509CertificateECC::SetSubject(const ECCPublicKey* key)
{
    memcpy(subject.x, key->x, sizeof(subject.x));
    memcpy(subject.y, key->y, sizeof(subject.y));
}

void X509CertificateECC::SetSerialNumber(const qcc::String& newSerialNumber)
{
    serialNumber = newSerialNumber;
}

const qcc::Certificate::ValidPeriod* X509CertificateECC::GetValidity()
{
    return &validity;
}

void X509CertificateECC::SetValidity(const qcc::Certificate::ValidPeriod* validityPeriod)
{
    validity.validFrom = validityPeriod->validFrom;
    validity.validTo = validityPeriod->validTo;
}

//IdentityCertificate
X509IdentityCertificate::X509IdentityCertificate() :
    X509CertificateECC(IDENTITY_CERTIFICATE)
{
}

X509IdentityCertificate::~X509IdentityCertificate()
{
}

const qcc::GUID128& X509IdentityCertificate::GetAlias() const
{
    return alias;
}

void X509IdentityCertificate::SetAlias(const qcc::GUID128& newAlias)
{
    alias = newAlias;
}

const qcc::String& X509IdentityCertificate::GetName() const
{
    return name;
}

void X509IdentityCertificate::SetName(const qcc::String& newName)
{
    name = newName;
}

//MembershipCertificate
X509MemberShipCertificate::X509MemberShipCertificate() :
    X509CertificateECC(MEMBERSHIP_CERTIFICATE), delegate(false)
{
}

X509MemberShipCertificate::~X509MemberShipCertificate()
{
}

const qcc::String& X509MemberShipCertificate::GetGuildId() const
{
    return guildID;
}

void X509MemberShipCertificate::SetGuildId(const qcc::String& guildId)
{
    guildID = guildId;
}

const bool X509MemberShipCertificate::IsDelegate()
{
    return delegate;
}

void X509MemberShipCertificate::SetDelegate(bool dlgt)
{
    delegate = dlgt;
}

//UserEquivalenceCertificate
X509UserEquivalenceCertificate::X509UserEquivalenceCertificate() :
    X509CertificateECC(USER_EQUIVALENCE_CERTIFICATE)
{
}

X509UserEquivalenceCertificate::~X509UserEquivalenceCertificate()
{
}

//Policy Certificate
X509PolicyCertificate::X509PolicyCertificate() :
    X509CertificateECC(POLICY_CERTIFICATE)
{
}

X509PolicyCertificate::~X509PolicyCertificate()
{
}
} //close qcc namespace.
