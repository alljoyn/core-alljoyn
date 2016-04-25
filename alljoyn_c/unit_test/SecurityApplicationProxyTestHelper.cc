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

#include <alljoyn_c/BusAttachment.h>
#include <alljoyn/BusAttachment.h>
#include <qcc/Util.h>
#include <qcc/Crypto.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/StringUtil.h>
#include "ajTestCommon.h"
#include "CredentialAccessor.h"
#include "SecurityApplicationProxyTestHelper.h"

using namespace ajn;
using namespace qcc;
using namespace std;

static void BuildValidity(CertificateX509::ValidPeriod& validity, uint32_t expiredInSecs)
{
    validity.validFrom = GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + expiredInSecs;
}

const uint32_t SecurityApplicationProxyTestHelper::oneHourInSeconds = 3600;

void SecurityApplicationProxyTestHelper::CreateIdentityCert(alljoyn_busattachment issuerBus, alljoyn_busattachment receiverBus, AJ_PSTR* receiverCertificate, bool delegate)
{
    string certificate;
    GUID128 receiverGuid;
    ECCPublicKey receiverPublicKey;
    BusAttachment* issuer = (BusAttachment*)issuerBus;
    BusAttachment* receiver = (BusAttachment*)receiverBus;

    ASSERT_EQ(ER_OK, GetGUID(*receiver, receiverGuid));
    ASSERT_EQ(ER_OK, RetrieveDSAPublicKeyFromKeyStore(receiver, receiverPublicKey));
    ASSERT_EQ(ER_OK, CreateIdentityCert(*issuer,
                                        "0",
                                        receiverGuid.ToString(),
                                        &receiverPublicKey,
                                        delegate ? "delegate" : "non-delegate",
                                        certificate,
                                        delegate));
    *receiverCertificate = CreateStringCopy(certificate);
}

void SecurityApplicationProxyTestHelper::CreateIdentityCertChain(AJ_PCSTR issuerIdentityCert, AJ_PCSTR receiverIdentityCert, AJ_PSTR* certificateChainPem)
{
    *certificateChainPem = CreateStringCopy(string(receiverIdentityCert) + issuerIdentityCert);
}

void SecurityApplicationProxyTestHelper::RetrieveDSAPrivateKeyFromKeyStore(alljoyn_busattachment bus, AJ_PSTR* privateKey)
{
    ECCPrivateKey outputPrivateKey;
    String privateKeyString;
    BusAttachment* inputBus = (BusAttachment*)bus;
    CredentialAccessor ca(*inputBus);

    ASSERT_EQ(ER_OK, ca.GetDSAPrivateKey(outputPrivateKey));
    ASSERT_EQ(ER_OK, CertificateX509::EncodePrivateKeyPEM(&outputPrivateKey, privateKeyString));
    *privateKey = CreateStringCopy(static_cast<std::string>(privateKeyString));
}

void SecurityApplicationProxyTestHelper::RetrieveDSAPublicKeyFromKeyStore(alljoyn_busattachment bus, AJ_PSTR* publicKey)
{
    ECCPublicKey outputPublicKey;
    String publicKeyString;

    ASSERT_EQ(ER_OK, RetrieveDSAPublicKeyFromKeyStore((BusAttachment*)bus, outputPublicKey));
    ASSERT_EQ(ER_OK, CertificateX509::EncodePublicKeyPEM(&outputPublicKey, publicKeyString));
    *publicKey = CreateStringCopy(static_cast<std::string>(publicKeyString));
}

void SecurityApplicationProxyTestHelper::ReplaceString(string& original, AJ_PCSTR from, AJ_PCSTR to)
{
    size_t offset = original.find(from);
    original.replace(offset, strlen(from), to);
}

void SecurityApplicationProxyTestHelper::CreateMembershipCert(alljoyn_busattachment signingBus, alljoyn_busattachment memberBus, const uint8_t* groupId, bool delegate, AJ_PSTR* membershipCertificatePem)
{
    string certificate;
    GUID128 certificateGuid;
    ECCPublicKey memberPublicKey;
    BusAttachment* signer = (BusAttachment*)signingBus;
    BusAttachment* member = (BusAttachment*)memberBus;

    certificateGuid.SetBytes(groupId);
    ASSERT_EQ(ER_OK, RetrieveDSAPublicKeyFromKeyStore(member, memberPublicKey));
    ASSERT_EQ(ER_OK, CreateMembershipCert("1",
                                          *signer,
                                          member->GetUniqueName(),
                                          &memberPublicKey,
                                          certificateGuid,
                                          delegate,
                                          certificate));
    *membershipCertificatePem = CreateStringCopy(certificate);
}

void ajn::SecurityApplicationProxyTestHelper::DestroyCertificate(AJ_PSTR cert)
{
    DestroyStringCopy(cert);
}

void ajn::SecurityApplicationProxyTestHelper::DestroyKey(AJ_PSTR key)
{
    DestroyStringCopy(key);
}

QStatus SecurityApplicationProxyTestHelper::CreateIdentityCert(BusAttachment& issuerBus,
                                                               const string& serial,
                                                               const string& subject,
                                                               const ECCPublicKey* subjectPubKey,
                                                               const string& alias,
                                                               IdentityCertificate& cert,
                                                               bool delegate)
{
    QStatus status;
    CertificateX509::ValidPeriod validity;
    string issuerStr;
    GUID128 issuer(0);
    PermissionConfigurator& pc = issuerBus.GetPermissionConfigurator();

    GetGUID(issuerBus, issuer);
    issuerStr = issuer.ToString();

    cert.SetSerial(reinterpret_cast<const uint8_t*>(serial.data()), serial.size());
    cert.SetIssuerCN((const uint8_t*) issuerStr.data(), issuerStr.size());
    cert.SetSubjectCN((const uint8_t*) subject.data(), subject.size());
    cert.SetSubjectPublicKey(subjectPubKey);
    cert.SetAlias(alias);
    cert.SetCA(delegate);

    BuildValidity(validity, oneHourInSeconds);
    cert.SetValidity(&validity);

    /* use the issuer bus to sign the cert */
    status = pc.SignCertificate(cert);

    if (ER_OK == status) {
        KeyInfoNISTP256 issuerPublicKey;
        pc.GetSigningPublicKey(issuerPublicKey);
        status = cert.Verify(issuerPublicKey.GetPublicKey());
    }

    return status;
}

QStatus SecurityApplicationProxyTestHelper::CreateIdentityCert(BusAttachment& issuerBus,
                                                               const string& serial,
                                                               const string& subject,
                                                               const ECCPublicKey* subjectPubKey,
                                                               const string& alias,
                                                               string& pem,
                                                               bool delegate)
{
    IdentityCertificate cert;
    String qccPem;
    QStatus status = CreateIdentityCert(issuerBus, serial, subject, subjectPubKey, alias, cert, delegate);

    if (ER_OK == status) {
        status = cert.EncodeCertificatePEM(qccPem);
        pem = qccPem.c_str();
    }

    return status;
}

QStatus ajn::SecurityApplicationProxyTestHelper::RetrieveDSAPublicKeyFromKeyStore(BusAttachment* bus, ECCPublicKey& publicKey)
{
    CredentialAccessor ca(*bus);
    return ca.GetDSAPublicKey(publicKey);
}

QStatus SecurityApplicationProxyTestHelper::CreateMembershipCert(const string& serial, BusAttachment& signingBus, const string& subject, const ECCPublicKey* subjectPubKey, const GUID128& guild, bool delegate, MembershipCertificate& cert, bool setEmptyAKI)
{
    QStatus status;
    CertificateX509::ValidPeriod validity;
    string issuerStr;
    GUID128 issuer(0);
    PermissionConfigurator& pc = signingBus.GetPermissionConfigurator();

    GetGUID(signingBus, issuer);
    issuerStr = issuer.ToString();

    cert.SetSerial(reinterpret_cast<const uint8_t*>(serial.data()), serial.size());
    cert.SetIssuerCN((const uint8_t*) issuerStr.data(), issuerStr.size());
    cert.SetSubjectCN((const uint8_t*) subject.data(), subject.size());
    cert.SetSubjectPublicKey(subjectPubKey);
    cert.SetGuild(guild);
    cert.SetCA(delegate);

    BuildValidity(validity, oneHourInSeconds);
    cert.SetValidity(&validity);

    /* use the signing bus to sign the cert */
    if (setEmptyAKI) {
        CredentialAccessor ca(signingBus);
        ECCPrivateKey privateKey;

        status = ca.GetDSAPrivateKey(privateKey);

        if (ER_OK == status) {
            status = cert.Sign(&privateKey);
        }
    } else {
        status = pc.SignCertificate(cert);
    }

    return status;
}

QStatus SecurityApplicationProxyTestHelper::CreateMembershipCert(const string& serial, BusAttachment& signingBus, const string& subject, const ECCPublicKey* subjectPubKey, const GUID128& guild, bool delegate, string& pem)
{
    MembershipCertificate cert;
    String qccPem;
    QStatus status = CreateMembershipCert(serial, signingBus, subject, subjectPubKey, guild, delegate, cert);

    if (ER_OK == status) {
        status = cert.EncodeCertificatePEM(qccPem);
        pem = qccPem.c_str();
    }

    return status;
}

QStatus SecurityApplicationProxyTestHelper::GetGUID(BusAttachment& bus, GUID128& guid)
{
    CredentialAccessor ca(bus);
    return ca.GetGuid(guid);
}