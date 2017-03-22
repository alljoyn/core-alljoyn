/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "SecurityTestHelper.h"

#include <qcc/KeyInfoECC.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/PermissionConfigurator.h>
#include <alljoyn/SecurityApplicationProxy.h>
#include <alljoyn/TransportMask.h>

#include "ajTestCommon.h"
#include "CredentialAccessor.h"

QStatus SecurityTestHelper::GetGUID(ajn::BusAttachment& bus, qcc::GUID128& guid)
{
    ajn::CredentialAccessor ca(bus);
    return ca.GetGuid(guid);
}

QStatus SecurityTestHelper::GetPeerGUID(ajn::BusAttachment& bus, qcc::String& peerName, qcc::GUID128& peerGuid)
{
    ajn::CredentialAccessor ca(bus);
    return ca.GetPeerGuid(peerName, peerGuid);
}

QStatus SecurityTestHelper::GetAppPublicKey(ajn::BusAttachment& bus, qcc::ECCPublicKey& publicKey)
{
    qcc::KeyInfoNISTP256 keyInfo;
    QStatus status = bus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
    if (ER_OK != status) {
        return status;
    }
    publicKey = *keyInfo.GetPublicKey();
    return status;
}

QStatus SecurityTestHelper::RetrievePublicKeyFromMsgArg(const ajn::MsgArg& arg, qcc::ECCPublicKey* pubKey)
{
    uint8_t keyFormat;
    ajn::MsgArg* variantArg;
    QStatus status = arg.Get("(yv)", &keyFormat, &variantArg);
    if (ER_OK != status) {
        return status;
    }
    if (keyFormat != qcc::KeyInfo::FORMAT_ALLJOYN) {
        return status;
    }
    uint8_t* kid;
    size_t kidLen;
    uint8_t keyUsageType, keyType;
    ajn::MsgArg* keyVariantArg;
    status = variantArg->Get("(ayyyv)", &kidLen, &kid, &keyUsageType, &keyType, &keyVariantArg);
    if (ER_OK != status) {
        return status;
    }
    if ((keyUsageType != qcc::KeyInfo::USAGE_SIGNING) && (keyUsageType != qcc::KeyInfo::USAGE_ENCRYPTION)) {
        return status;
    }
    if (keyType != qcc::KeyInfoECC::KEY_TYPE) {
        return status;
    }
    uint8_t algorithm, curve;
    ajn::MsgArg* curveVariant;
    status = keyVariantArg->Get("(yyv)", &algorithm, &curve, &curveVariant);
    if (ER_OK != status) {
        return status;
    }
    if (curve != qcc::Crypto_ECC::ECC_NIST_P256) {
        return status;
    }

    uint8_t* xCoord;
    uint8_t* yCoord;
    size_t xLen, yLen;
    status = curveVariant->Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);
    if (ER_OK != status) {
        return status;
    }
    if ((xLen != qcc::ECC_COORDINATE_SZ) || (yLen != qcc::ECC_COORDINATE_SZ)) {
        return status;
    }
    return pubKey->Import(xCoord, xLen, yCoord, yLen);
}

QStatus SecurityTestHelper::RetrieveDSAPublicKeyFromKeyStore(ajn::BusAttachment& bus, qcc::ECCPublicKey* publicKey)
{
    ajn::CredentialAccessor ca(bus);
    return ca.GetDSAPublicKey(*publicKey);
}

void SecurityTestHelper::CreatePermissivePolicyAll(ajn::PermissionPolicy& policy, uint32_t version) {
    policy.SetVersion(version);
    {
        ajn::PermissionPolicy::Acl acls[1];
        {
            ajn::PermissionPolicy::Peer peers[1];
            peers[0].SetType(ajn::PermissionPolicy::Peer::PEER_ALL);
            acls[0].SetPeers(1, peers);
        }
        {
            ajn::PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                ajn::PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               ajn::PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               ajn::PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               ajn::PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               ajn::PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        policy.SetAcls(1, acls);
    }
}

void SecurityTestHelper::CreatePermissivePolicyAnyTrusted(ajn::PermissionPolicy& policy, uint32_t version) {
    policy.SetVersion(version);
    {
        ajn::PermissionPolicy::Acl acls[1];
        {
            ajn::PermissionPolicy::Peer peers[1];
            peers[0].SetType(ajn::PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            ajn::PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                ajn::PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               ajn::PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               ajn::PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               ajn::PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               ajn::PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        policy.SetAcls(1, acls);
    }
}

void SecurityTestHelper::UpdatePolicyWithValuesFromDefaultPolicy(const ajn::PermissionPolicy& defaultPolicy,
                                                                 ajn::PermissionPolicy& policy,
                                                                 bool keepCAentry,
                                                                 bool keepAdminGroupEntry,
                                                                 bool keepInstallMembershipEntry) {
    size_t aclsCount = policy.GetAclsSize();
    if (keepCAentry) {
        ++aclsCount;
    }
    if (keepAdminGroupEntry) {
        ++aclsCount;
    }
    if (keepInstallMembershipEntry) {
        ++aclsCount;
    }

    ajn::PermissionPolicy::Acl* acls = new ajn::PermissionPolicy::Acl[aclsCount];
    size_t idx = 0;
    for (size_t itAcl = 0; itAcl < defaultPolicy.GetAclsSize(); ++itAcl) {
        const ajn::PermissionPolicy::Acl& acl = defaultPolicy.GetAcls()[itAcl];
        if (acl.GetPeersSize() > 0) {
            ajn::PermissionPolicy::Peer::PeerType peerType = acl.GetPeers()[0].GetType();
            if ((peerType == ajn::PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY && keepCAentry) ||
                (peerType == ajn::PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP && keepAdminGroupEntry) ||
                (peerType == ajn::PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY && keepInstallMembershipEntry)) {
                acls[idx++] = acl;
            }
        }
    }

    for (size_t itAcl = 0; itAcl < policy.GetAclsSize(); ++itAcl) {
        QCC_ASSERT(idx < aclsCount);
        acls[idx++] = policy.GetAcls()[itAcl];
    }

    policy.SetAcls(aclsCount, acls);
    delete [] acls;
}

QStatus SecurityTestHelper::CreateAllInclusiveManifest(ajn::Manifest& manifest)
{
    const size_t manifestSize = 1;
    ajn::PermissionPolicy::Rule manifestRules[manifestSize];
    manifestRules[0].SetObjPath("*");
    manifestRules[0].SetInterfaceName("*");
    {
        ajn::PermissionPolicy::Rule::Member member[3];
        member[0].Set("*", ajn::PermissionPolicy::Rule::Member::METHOD_CALL,
                      ajn::PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                      ajn::PermissionPolicy::Rule::Member::ACTION_MODIFY);
        member[1].Set("*", ajn::PermissionPolicy::Rule::Member::SIGNAL,
                      ajn::PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                      ajn::PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        member[2].Set("*", ajn::PermissionPolicy::Rule::Member::PROPERTY,
                      ajn::PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                      ajn::PermissionPolicy::Rule::Member::ACTION_MODIFY |
                      ajn::PermissionPolicy::Rule::Member::ACTION_OBSERVE);

        manifestRules[0].SetMembers(ArraySize(member), member);
    }
    return manifest->SetRules(manifestRules, manifestSize);
}

QStatus SecurityTestHelper::SignManifest(ajn::BusAttachment& issuerBus,
                                         const std::vector<uint8_t>& subjectThumbprint,
                                         ajn::Manifest& manifest)
{
    return issuerBus.GetPermissionConfigurator().SignManifest(subjectThumbprint, manifest);
}

QStatus SecurityTestHelper::SignManifest(ajn::BusAttachment& issuerBus,
                                         const qcc::CertificateX509& subjectCertificate,
                                         ajn::Manifest& manifest)
{
    return issuerBus.GetPermissionConfigurator().ComputeThumbprintAndSignManifest(subjectCertificate, manifest);
}

QStatus SecurityTestHelper::SignManifest(ajn::BusAttachment& issuerBus,
                                         const qcc::CertificateX509& subjectCertificate,
                                         AJ_PCSTR unsignedManifestXml,
                                         std::string& signedManifestXml)
{
    ajn::CredentialAccessor ca(issuerBus);
    qcc::ECCPrivateKey privateKey;
    AJ_PSTR signedManifestXmlC = nullptr;
    QStatus status = ca.GetDSAPrivateKey(privateKey);
    if (ER_OK != status) {
        return status;
    }

    status = ajn::SecurityApplicationProxy::SignManifest(subjectCertificate, privateKey, unsignedManifestXml, &signedManifestXmlC);

    if (ER_OK != status) {
        return status;
    }

    signedManifestXml = signedManifestXmlC;
    ajn::SecurityApplicationProxy::DestroySignedManifest(signedManifestXmlC);

    return ER_OK;
}

QStatus SecurityTestHelper::SignManifests(ajn::BusAttachment& issuerBus,
                                          const qcc::CertificateX509& subjectCertificate,
                                          std::vector<ajn::Manifest>& manifests)
{
    for (ajn::Manifest manifest : manifests) {
        QStatus status = SignManifest(issuerBus, subjectCertificate, manifest);
        if (ER_OK != status) {
            return status;
        }
    }

    return ER_OK;
}

QStatus SecurityTestHelper::CreateIdentityCert(ajn::BusAttachment& issuerBus,
                                               const qcc::String& serial,
                                               const qcc::String& subject,
                                               const qcc::ECCPublicKey* subjectPubKey,
                                               const qcc::String& alias,
                                               qcc::IdentityCertificate& cert,
                                               uint32_t expiredInSecs,
                                               bool setEmptyAKI)
{
    qcc::GUID128 issuer(0);
    GetGUID(issuerBus, issuer);

    QStatus status = ER_CRYPTO_ERROR;

    cert.SetSerial(reinterpret_cast<const uint8_t*>(serial.data()), serial.size());
    qcc::String issuerStr = issuer.ToString();
    cert.SetIssuerCN(reinterpret_cast<const uint8_t*>(issuerStr.data()), issuerStr.size());
    cert.SetSubjectCN(reinterpret_cast<const uint8_t*>(subject.data()), subject.size());
    cert.SetSubjectPublicKey(subjectPubKey);
    cert.SetAlias(alias);
    qcc::CertificateX509::ValidPeriod validity;
    BuildValidity(validity, expiredInSecs);
    cert.SetValidity(&validity);

    /* use the issuer bus to sign the cert */
    ajn::PermissionConfigurator& pc = issuerBus.GetPermissionConfigurator();
    if (setEmptyAKI) {
        ajn::CredentialAccessor ca(issuerBus);
        qcc::ECCPrivateKey privateKey;
        status = ca.GetDSAPrivateKey(privateKey);
        if (ER_OK != status) {
            return status;
        }
        status = cert.Sign(&privateKey);
    } else {
        status = pc.SignCertificate(cert);
    }
    if (ER_OK != status) {
        return status;
    }

    qcc::KeyInfoNISTP256 keyInfo;
    pc.GetSigningPublicKey(keyInfo);
    status = cert.Verify(keyInfo.GetPublicKey());
    if (ER_OK != status) {
        return status;
    }

    return ER_OK;
}

QStatus SecurityTestHelper::CreateIdentityCert(ajn::BusAttachment& issuerBus,
                                               const qcc::String& serial,
                                               const qcc::String& subject,
                                               const qcc::ECCPublicKey* subjectPubKey,
                                               const qcc::String& alias,
                                               qcc::String& der,
                                               uint32_t expiredInSecs)
{
    qcc::IdentityCertificate cert;
    QStatus status = CreateIdentityCert(issuerBus, serial, subject, subjectPubKey, alias, cert, expiredInSecs);
    if (ER_OK != status) {
        return status;
    }
    return cert.EncodeCertificateDER(der);
}

QStatus SecurityTestHelper::CreateIdentityCertChain(ajn::BusAttachment& caBus,
                                                    ajn::BusAttachment& issuerBus,
                                                    const qcc::String& serial,
                                                    const qcc::String& subject,
                                                    const qcc::ECCPublicKey* subjectPubKey,
                                                    const qcc::String& alias,
                                                    qcc::IdentityCertificate* certChain,
                                                    size_t chainCount,
                                                    uint32_t expiredInSecs)
{
    if (chainCount > 3) {
        return ER_INVALID_DATA;
    }
    QStatus status = ER_CRYPTO_ERROR;

    qcc::GUID128 ca(0);
    GetGUID(caBus, ca);
    qcc::String caStr = ca.ToString();
    ajn::PermissionConfigurator& caPC = caBus.GetPermissionConfigurator();
    if (chainCount == 3) {
        /* generate the self signed CA cert */
        qcc::String caSerial = serial + "02";
        certChain[2].SetSerial(reinterpret_cast<const uint8_t*>(caSerial.data()), caSerial.size());
        certChain[2].SetIssuerCN(reinterpret_cast<const uint8_t*>(caStr.data()), caStr.size());
        certChain[2].SetSubjectCN(reinterpret_cast<const uint8_t*>(caStr.data()), caStr.size());
        qcc::CertificateX509::ValidPeriod validity;
        BuildValidity(validity, expiredInSecs);
        certChain[2].SetValidity(&validity);
        certChain[2].SetCA(true);
        qcc::KeyInfoNISTP256 keyInfo;
        caPC.GetSigningPublicKey(keyInfo);
        certChain[2].SetSubjectPublicKey(keyInfo.GetPublicKey());
        status = caPC.SignCertificate(certChain[2]);
        if (ER_OK != status) {
            return status;
        }
    }

    /* generate the issuer cert */
    qcc::GUID128 issuer(0);
    GetGUID(issuerBus, issuer);
    qcc::String issuerStr = issuer.ToString();

    qcc::String issuerSerial = serial + "01";
    certChain[1].SetSerial(reinterpret_cast<const uint8_t*>(issuerSerial.data()), issuerSerial.size());
    certChain[1].SetIssuerCN(reinterpret_cast<const uint8_t*>(caStr.data()), caStr.size());
    certChain[1].SetSubjectCN(reinterpret_cast<const uint8_t*>(issuerStr.data()), issuerStr.size());
    qcc::CertificateX509::ValidPeriod validity;
    BuildValidity(validity, expiredInSecs);
    certChain[1].SetValidity(&validity);
    certChain[1].SetCA(true);
    ajn::PermissionConfigurator& pc = issuerBus.GetPermissionConfigurator();
    qcc::KeyInfoNISTP256 keyInfo;
    pc.GetSigningPublicKey(keyInfo);
    certChain[1].SetSubjectPublicKey(keyInfo.GetPublicKey());

    status = caPC.SignCertificate(certChain[1]);
    if (ER_OK != status) {
        return status;
    }

    /* generate the leaf cert */
    certChain[0].SetSerial(reinterpret_cast<const uint8_t*>(serial.data()), serial.size());
    certChain[0].SetIssuerCN(reinterpret_cast<const uint8_t*>(issuerStr.data()), issuerStr.size());
    certChain[0].SetSubjectCN(reinterpret_cast<const uint8_t*>(subject.data()), subject.size());
    certChain[0].SetSubjectPublicKey(subjectPubKey);
    certChain[0].SetAlias(alias);
    certChain[0].SetValidity(&validity);

    /* use the issuer bus to sign the cert */
    status = pc.SignCertificate(certChain[0]);
    if (ER_OK != status) {
        return status;
    }

    status = certChain[0].Verify(certChain[1].GetSubjectPublicKey());
    if (ER_OK != status) {
        return status;
    }

    return ER_OK;
}

QStatus SecurityTestHelper::CreateMembershipCert(const qcc::String& serial,
                                                 ajn::BusAttachment& signingBus,
                                                 const qcc::String& subject,
                                                 const qcc::ECCPublicKey* subjectPubKey,
                                                 const qcc::GUID128& guild,
                                                 qcc::MembershipCertificate& cert,
                                                 bool delegate,
                                                 uint32_t expiredInSecs,
                                                 bool setEmptyAKI)
{
    qcc::GUID128 issuer(0);
    GetGUID(signingBus, issuer);

    if (subject.empty()) {
        return ER_BAD_ARG_3;
    }

    cert.SetSerial(reinterpret_cast<const uint8_t*>(serial.data()), serial.size());
    qcc::String issuerStr = issuer.ToString();
    cert.SetIssuerCN(reinterpret_cast<const uint8_t*>(issuerStr.data()), issuerStr.size());
    cert.SetSubjectCN(reinterpret_cast<const uint8_t*>(subject.data()), subject.size());
    cert.SetSubjectPublicKey(subjectPubKey);
    cert.SetGuild(guild);
    cert.SetCA(delegate);
    qcc::CertificateX509::ValidPeriod validity;
    BuildValidity(validity, expiredInSecs);
    cert.SetValidity(&validity);
    /* use the signing bus to sign the cert */
    ajn::PermissionConfigurator& pc = signingBus.GetPermissionConfigurator();
    QStatus status = ER_CRYPTO_ERROR;
    if (setEmptyAKI) {
        ajn::CredentialAccessor ca(signingBus);
        qcc::ECCPrivateKey privateKey;
        status = ca.GetDSAPrivateKey(privateKey);
        if (ER_OK != status) {
            return status;
        }
        status = cert.Sign(&privateKey);
    } else {
        status = pc.SignCertificate(cert);
    }
    return status;
}

QStatus SecurityTestHelper::CreateMembershipCert(const qcc::String& serial,
                                                 ajn::BusAttachment& signingBus,
                                                 const qcc::String& subject,
                                                 const qcc::ECCPublicKey* subjectPubKey,
                                                 const qcc::GUID128& guild,
                                                 qcc::String& der,
                                                 bool delegate,
                                                 uint32_t expiredInSecs)
{
    qcc::MembershipCertificate cert;
    QStatus status = CreateMembershipCert(serial, signingBus, subject, subjectPubKey, guild, cert, delegate, expiredInSecs);
    if (ER_OK != status) {
        return status;
    }
    return cert.EncodeCertificateDER(der);
}

QStatus SecurityTestHelper::InstallMembership(const qcc::String& serial,
                                              ajn::BusAttachment& bus,
                                              const qcc::String& remoteObjName,
                                              ajn::BusAttachment& signingBus,
                                              const qcc::String& subject,
                                              const qcc::ECCPublicKey* subjectPubKey,
                                              const qcc::GUID128& guild)
{
    ajn::SecurityApplicationProxy saProxy(bus, remoteObjName.c_str());
    QStatus status;

    qcc::MembershipCertificate certs[1];
    status = CreateMembershipCert(serial, signingBus, subject, subjectPubKey, guild, certs[0]);
    if (status != ER_OK) {
        return status;
    }

    return saProxy.InstallMembership(certs, 1);
}

QStatus SecurityTestHelper::InstallMembershipChain(ajn::BusAttachment& topBus,
                                                   ajn::BusAttachment& secondBus,
                                                   const qcc::String& serial0,
                                                   const qcc::String& serial1,
                                                   const qcc::String& remoteObjName,
                                                   const qcc::String& secondSubject,
                                                   const qcc::ECCPublicKey* secondPubKey,
                                                   const qcc::String& targetSubject,
                                                   const qcc::ECCPublicKey* targetPubKey,
                                                   const qcc::GUID128& guild,
                                                   bool setEmptyAKI)
{
    ajn::SecurityApplicationProxy saSecondProxy(secondBus, remoteObjName.c_str());

    /* create the second cert first -- with delegate on  */
    qcc::MembershipCertificate certs[2];
    QStatus status = CreateMembershipCert(serial1, topBus, secondSubject, secondPubKey, guild, certs[1], true, 3600, setEmptyAKI);
    if (status != ER_OK) {
        return status;
    }

    /* create the leaf cert signed by the subject */
    status = CreateMembershipCert(serial0, secondBus, targetSubject, targetPubKey, guild, certs[0], false, 3600, setEmptyAKI);
    if (status != ER_OK) {
        return status;
    }

    /* install cert chain */
    return saSecondProxy.InstallMembership(certs, 2);
}

QStatus SecurityTestHelper::InstallMembershipChain(ajn::BusAttachment& caBus,
                                                   ajn::BusAttachment& intermediateBus,
                                                   ajn::BusAttachment& targetBus,
                                                   qcc::String& leafSerial,
                                                   const qcc::GUID128& sgID)
{
    qcc::MembershipCertificate certs[3];
    /* create the top cert first self signed CA cert with delegate on  */
    ajn::PermissionConfigurator& caPC = caBus.GetPermissionConfigurator();
    qcc::String caSerial = leafSerial + "02";
    qcc::KeyInfoNISTP256 keyInfo;
    caPC.GetSigningPublicKey(keyInfo);
    qcc::GUID128 subject(0);
    GetGUID(caBus, subject);
    QStatus status = CreateMembershipCert(caSerial, caBus, subject.ToString(), keyInfo.GetPublicKey(), sgID, certs[2], true);
    if (status != ER_OK) {
        return status;
    }
    /* create the intermediate cert with delegate on  */
    ajn::PermissionConfigurator& intermediatePC = intermediateBus.GetPermissionConfigurator();
    qcc::String intermediateSerial = leafSerial + "01";
    intermediatePC.GetSigningPublicKey(keyInfo);
    GetGUID(intermediateBus, subject);
    status = CreateMembershipCert(intermediateSerial, caBus, subject.ToString(), keyInfo.GetPublicKey(), sgID, certs[1], true);
    if (status != ER_OK) {
        return status;
    }

    /* create the leaf cert delegate off */
    ajn::PermissionConfigurator& targetPC = targetBus.GetPermissionConfigurator();
    targetPC.GetSigningPublicKey(keyInfo);
    GetGUID(targetBus, subject);
    status = CreateMembershipCert(leafSerial, intermediateBus, subject.ToString(), keyInfo.GetPublicKey(), sgID, certs[0]);
    if (status != ER_OK) {
        return status;
    }

    /* install cert chain */
    ajn::SecurityApplicationProxy saProxy(intermediateBus, targetBus.GetUniqueName().c_str());
    return saProxy.InstallMembership(certs, ArraySize(certs));
}

QStatus SecurityTestHelper::SetCAFlagOnCert(ajn::BusAttachment& issuerBus, qcc::CertificateX509& certificate)
{
    certificate.SetCA(true);
    ajn::PermissionConfigurator& pc = issuerBus.GetPermissionConfigurator();
    return pc.SignCertificate(certificate);
}

QStatus SecurityTestHelper::LoadCertificateBytes(ajn::Message& msg, qcc::CertificateX509& cert)
{
    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;
    QStatus status = msg->GetArg(0)->Get("(yay)", &encoding, &encodedLen, &encoded);
    if (ER_OK != status) {
        return status;
    }
    status = ER_NOT_IMPLEMENTED;
    if (encoding == qcc::CertificateX509::ENCODING_X509_DER) {
        status = cert.DecodeCertificateDER(qcc::String((const char*) encoded, encodedLen));
    } else if (encoding == qcc::CertificateX509::ENCODING_X509_DER_PEM) {
        status = cert.DecodeCertificatePEM(qcc::String((const char*) encoded, encodedLen));
    }
    return status;
}

bool SecurityTestHelper::IsPermissionDeniedError(QStatus status, ajn::Message& msg)
{
    if (ER_PERMISSION_DENIED == status) {
        return true;
    }
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        qcc::String errorMsg;
        const char* errorName = msg->GetErrorName(&errorMsg);
        if (errorName == nullptr) {
            return false;
        }
        if (strcmp(errorName, "org.alljoyn.Bus.Security.Error.PermissionDenied") == 0) {
            return true;
        }
        if (strcmp(errorName, "org.alljoyn.Bus.ErStatus") != 0) {
            return false;
        }
        if (errorMsg == "ER_PERMISSION_DENIED") {
            return true;
        }
    }
    return false;
}

QStatus SecurityTestHelper::ReadClaimResponse(ajn::Message& msg, qcc::ECCPublicKey* pubKey)
{
    return RetrievePublicKeyFromMsgArg(*msg->GetArg(0), pubKey);
}

QStatus SecurityTestHelper::JoinPeerSession(ajn::BusAttachment& initiator, ajn::BusAttachment& responder, ajn::SessionId& sessionId)
{
    ajn::SessionOpts opts(ajn::SessionOpts::TRAFFIC_MESSAGES, false, ajn::SessionOpts::PROXIMITY_ANY, ajn::TRANSPORT_ANY);
    QStatus status = ER_FAIL;
    const size_t maxAttempts = 30;
    for (size_t attempt = 0; attempt < maxAttempts; attempt++) {
        status = initiator.JoinSession(responder.GetUniqueName().c_str(),
                                       ALLJOYN_SESSIONPORT_PERMISSION_MGMT, nullptr, sessionId, opts);
        if (ER_OK == status) {
            return status;
        }
        /* sleep a few seconds since the responder may not yet setup the listener port */
        qcc::Sleep(WAIT_TIME_100);
    }
    return status;
}

void SecurityTestHelper::CallDeprecatedSetPSK(ajn::DefaultECDHEAuthListener* authListener, const uint8_t* pskBytes, size_t pskLength)
{
    /*
     * This function suppresses compiler warnings when calling SetPSK, which is deprecated.
     * ECHDE_PSK is deprecated as of 16.04, but we still test it, per the Alliance deprecation policy.
     * ASACORE-2762 tracks removal of the ECDHE_PSK tests (and this function can be removed as a part of that work).
     * https://jira.allseenalliance.org/browse/ASACORE-2762
     */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#if defined(QCC_OS_GROUP_WINDOWS)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

    QCC_VERIFY(ER_OK == authListener->SetPSK(pskBytes, pskLength));

#if defined(QCC_OS_GROUP_WINDOWS)
#pragma warning(pop)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

}

void SecurityTestHelper::UnwrapStrings(const std::vector<std::string>& strings, std::vector<AJ_PCSTR>& unwrapped)
{
    unwrapped.resize(strings.size());
    for (std::vector<std::string>::size_type i = 0; i < strings.size(); i++) {
        unwrapped[i] = strings[i].c_str();
    }
}

void SecurityTestHelper::BuildValidity(qcc::CertificateX509::ValidPeriod& validity, uint32_t expiredInSecs)
{
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + expiredInSecs;
}
