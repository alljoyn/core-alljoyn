/**
 * @file
 * PermissionConfigurator is responsible for managing an application's Security 2.0 settings.
 */

/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
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
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <alljoyn/BusAttachment.h>
#include <alljoyn/PermissionConfigurator.h>
#include <alljoyn/PermissionPolicy.h>
#include <alljoyn_c/PermissionConfigurator.h>
#include <alljoyn_c/SecurityApplicationProxy.h>
#include <stdio.h>
#include <qcc/Debug.h>
#include <qcc/StringUtil.h>
#include "XmlManifestConverter.h"
#include "XmlPoliciesConverter.h"
#include "KeyInfoHelper.h"
#include "CertificateUtilities.h"

#define QCC_MODULE "ALLJOYN_C"

using namespace qcc;
using namespace ajn;

alljoyn_claimcapabilities AJ_CALL alljoyn_permissionconfigurator_getdefaultclaimcapabilities()
{
    return PermissionConfigurator::CLAIM_CAPABILITIES_DEFAULT;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getapplicationstate(const alljoyn_permissionconfigurator configurator, alljoyn_applicationstate* state)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator::ApplicationState returnedState;
    QStatus status = ((const PermissionConfigurator*)configurator)->GetApplicationState(returnedState);

    if (ER_OK == status) {
        *state = (alljoyn_applicationstate)returnedState;
    }

    return status;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_setapplicationstate(alljoyn_permissionconfigurator configurator, const alljoyn_applicationstate state)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->SetApplicationState((PermissionConfigurator::ApplicationState)state);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getpublickey(alljoyn_permissionconfigurator configurator, AJ_PSTR* publicKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    KeyInfoNISTP256 keyInfo;

    QStatus status = ((PermissionConfigurator*)configurator)->GetSigningPublicKey(keyInfo);
    if (ER_OK != status) {
        return status;
    }

    String encodedPem;

    status = CertificateX509::EncodePublicKeyPEM(keyInfo.GetPublicKey(), encodedPem);
    if (ER_OK != status) {
        return status;
    }

    *publicKey = CreateStringCopy(static_cast<std::string>(encodedPem));

    if (nullptr == *publicKey) {
        return ER_OUT_OF_MEMORY;
    }

    return ER_OK;
}

void AJ_CALL alljoyn_permissionconfigurator_publickey_destroy(AJ_PSTR publicKey)
{
    DestroyStringCopy(publicKey);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getconnectedpeerpublickey(alljoyn_permissionconfigurator configurator, const uint8_t* groupId, AJ_PSTR* publicKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::ECCPublicKey eccKey;
    qcc::String eccKeyStr;
    GUID128 gid;
    gid.SetBytes(groupId);

    QStatus status = ((PermissionConfigurator*)configurator)->GetConnectedPeerPublicKey(gid, &eccKey);
    if (ER_OK != status) {
        return status;
    }

    eccKeyStr = eccKey.ToString();
    *publicKey = CreateStringCopy(static_cast<std::string>(eccKeyStr));

    if (nullptr == *publicKey) {
        return ER_OUT_OF_MEMORY;
    }

    return ER_OK;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getmanifesttemplate(alljoyn_permissionconfigurator configurator, AJ_PSTR* manifestTemplateXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    std::string manifestTemplateString;

    QStatus status = ((PermissionConfigurator*)configurator)->GetManifestTemplateAsXml(manifestTemplateString);
    if (ER_OK != status) {
        return status;
    }

    *manifestTemplateXml = CreateStringCopy(manifestTemplateString);
    if (nullptr == *manifestTemplateXml) {
        return ER_OUT_OF_MEMORY;
    }

    return ER_OK;
}

void AJ_CALL alljoyn_permissionconfigurator_manifesttemplate_destroy(AJ_PSTR manifestTemplateXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    DestroyStringCopy(manifestTemplateXml);
}

void AJ_CALL alljoyn_permissionconfigurator_manifest_destroy(AJ_PSTR manifestXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    DestroyStringCopy(manifestXml);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_setmanifesttemplatefromxml(alljoyn_permissionconfigurator configurator, AJ_PCSTR manifestTemplateXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->SetManifestTemplateFromXml(manifestTemplateXml);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getclaimcapabilities(const alljoyn_permissionconfigurator configurator, alljoyn_claimcapabilities* claimCapabilities)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator::ClaimCapabilities returnedClaimCapabilities;
    QStatus status = ((const PermissionConfigurator*)configurator)->GetClaimCapabilities(returnedClaimCapabilities);

    if (ER_OK == status) {
        *claimCapabilities = (alljoyn_claimcapabilities)returnedClaimCapabilities;
    }

    return status;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_setclaimcapabilities(alljoyn_permissionconfigurator configurator, alljoyn_claimcapabilities claimCapabilities)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->SetClaimCapabilities((PermissionConfigurator::ClaimCapabilities)claimCapabilities);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(const alljoyn_permissionconfigurator configurator, alljoyn_claimcapabilitiesadditionalinfo* additionalInfo)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator::ClaimCapabilityAdditionalInfo returnedClaimCapabilitiesAdditionalInfo;
    QStatus status = ((const PermissionConfigurator*)configurator)->GetClaimCapabilityAdditionalInfo(returnedClaimCapabilitiesAdditionalInfo);

    if (ER_OK == status) {
        *additionalInfo = (alljoyn_claimcapabilitiesadditionalinfo)returnedClaimCapabilitiesAdditionalInfo;
    }

    return status;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(alljoyn_permissionconfigurator configurator, alljoyn_claimcapabilitiesadditionalinfo additionalInfo)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->SetClaimCapabilityAdditionalInfo((PermissionConfigurator::ClaimCapabilityAdditionalInfo)additionalInfo);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_reset(alljoyn_permissionconfigurator configurator)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->Reset();
}

QStatus AJ_CALL alljoyn_permissionconfigurator_claim(alljoyn_permissionconfigurator configurator,
                                                     AJ_PCSTR caKey,
                                                     AJ_PCSTR identityCertificateChain,
                                                     const uint8_t* groupId,
                                                     size_t groupSize,
                                                     AJ_PCSTR groupAuthority,
                                                     AJ_PCSTR* manifestsXmls,
                                                     size_t manifestsCount)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    size_t identCertCount = 0;
    KeyInfoNISTP256 caPublicKey;
    KeyInfoNISTP256 groupPublicKey;
    GUID128 groupGuid;
    PermissionConfigurator* pc = (PermissionConfigurator*)configurator;
    CertificateX509* identityCerts = nullptr;

    status = GetGroupId(groupId, groupSize, groupGuid);

    if (ER_OK == status) {
        status = KeyInfoHelper::PEMToKeyInfoNISTP256(caKey, caPublicKey);
    }

    if (ER_OK == status) {
        status = KeyInfoHelper::PEMToKeyInfoNISTP256(groupAuthority, groupPublicKey);
    }

    if (ER_OK == status) {
        status = ExtractCertificates(identityCertificateChain, &identCertCount, &identityCerts);
    }

    if (ER_OK == status) {
        status = pc->Claim(caPublicKey,
                           groupGuid,
                           groupPublicKey,
                           identityCerts, identCertCount,
                           manifestsXmls, manifestsCount);
    }

    delete[] identityCerts;

    return status;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_updateidentity(alljoyn_permissionconfigurator configurator,
                                                              AJ_PCSTR identityCertificateChain,
                                                              AJ_PCSTR* manifestsXmls, size_t manifestsCount)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    size_t certCount = 0;
    PermissionConfigurator* pc = (PermissionConfigurator*)configurator;
    CertificateX509* certs = nullptr;

    status = ExtractCertificates(identityCertificateChain, &certCount, &certs);

    if (ER_OK == status) {
        status = pc->UpdateIdentity(certs,
                                    certCount,
                                    manifestsXmls,
                                    manifestsCount);
    }

    delete[] certs;

    return status;

}

QStatus AJ_CALL alljoyn_permissionconfigurator_getidentity(alljoyn_permissionconfigurator configurator,
                                                           AJ_PSTR* identityCertificateChain)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    PermissionConfigurator* pc = (PermissionConfigurator*)configurator;
    std::vector<CertificateX509> certChain;

    status = pc->GetIdentity(certChain);
    if (ER_OK != status) {
        return status;
    }

    qcc::String chainPEM;
    qcc::String individualCertificate;

    for (std::vector<CertificateX509>::size_type i = 0; i < certChain.size(); i++) {
        status = certChain[i].EncodeCertificatePEM(individualCertificate);
        if (ER_OK != status) {
            return status;
        }

        chainPEM.append(individualCertificate);
    }

    *identityCertificateChain = CreateStringCopy(static_cast<std::string>(chainPEM));

    if (nullptr == *identityCertificateChain) {
        return ER_OUT_OF_MEMORY;
    }

    return ER_OK;
}

void AJ_CALL alljoyn_permissionconfigurator_certificatechain_destroy(AJ_PSTR certificateChain)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    DestroyStringCopy(certificateChain);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getmanifests(alljoyn_permissionconfigurator configurator,
                                                            alljoyn_manifestarray* manifestArray)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    PermissionConfigurator* pc = (PermissionConfigurator*)configurator;
    std::vector<Manifest> manifests;

    status = pc->GetManifests(manifests);
    if (ER_OK != status) {
        return status;
    }

    QCC_ASSERT(manifests.size() > 0);

    std::vector<std::string> manifestsStrings;
    status = XmlManifestConverter::ManifestsToXmlArray(manifests.data(), manifests.size(), manifestsStrings);
    if (ER_OK != status) {
        return status;
    }

    std::vector<std::string>::size_type count = manifestsStrings.size();
    manifestArray->xmls = new (std::nothrow) AJ_PSTR[count];
    if (nullptr == manifestArray->xmls) {
        manifestArray->count = 0;
        return ER_OUT_OF_MEMORY;
    }

    memset(manifestArray->xmls, 0, sizeof(AJ_PSTR) * count);

    manifestArray->count = manifestsStrings.size();

    for (std::vector<std::string>::size_type i = 0; i < count; i++) {
        manifestArray->xmls[i] = CreateStringCopy(manifestsStrings[i]);
        if (nullptr == manifestArray->xmls[i]) {
            alljoyn_permissionconfigurator_manifestarray_cleanup(manifestArray);
            memset(manifestArray, 0, sizeof(*manifestArray));
            return ER_OUT_OF_MEMORY;
        }
    }

    return ER_OK;
}

void AJ_CALL alljoyn_permissionconfigurator_manifestarray_cleanup(alljoyn_manifestarray* manifestArray)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QCC_ASSERT(nullptr != manifestArray);

    for (size_t i = 0; i < manifestArray->count; i++) {
        DestroyStringCopy(manifestArray->xmls[i]);
    }

    delete[] manifestArray->xmls;

    memset(manifestArray, 0, sizeof(*manifestArray));
}

QStatus AJ_CALL alljoyn_permissionconfigurator_installmanifests(alljoyn_permissionconfigurator configurator,
                                                                AJ_PCSTR* manifestsXmls,
                                                                size_t manifestsCount,
                                                                QCC_BOOL append)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->InstallManifests(manifestsXmls, manifestsCount, (QCC_TRUE == append));
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getidentitycertificateid(alljoyn_permissionconfigurator configurator,
                                                                        alljoyn_certificateid* certificateId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    PermissionConfigurator* pc = (PermissionConfigurator*)configurator;
    String serialString;
    KeyInfoNISTP256 keyInfo;
    String keyInfoString;

    status = pc->GetIdentityCertificateId(serialString, keyInfo);
    if (ER_OK != status) {
        return status;
    }

    status = KeyInfoHelper::KeyInfoNISTP256ToPEM(keyInfo, keyInfoString);
    if (ER_OK != status) {
        return status;
    }

    uint8_t* serialBuffer = new (std::nothrow) uint8_t[serialString.size()];
    if (nullptr == serialBuffer) {
        return ER_OUT_OF_MEMORY;
    }
    memcpy(serialBuffer, serialString.data(), serialString.size());
    certificateId->serial = serialBuffer;
    certificateId->serialLen = serialString.size();

    certificateId->issuerPublicKey = CreateStringCopy(keyInfoString);
    if (nullptr == certificateId->issuerPublicKey) {
        delete[] certificateId->serial;
        certificateId->serial = nullptr;
        certificateId->serialLen = 0;
        return ER_OUT_OF_MEMORY;
    }

    certificateId->issuerAki = nullptr;
    certificateId->issuerAkiLen = 0;

    return ER_OK;
}

void AJ_CALL alljoyn_permissionconfigurator_certificateid_cleanup(alljoyn_certificateid* certificateId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QCC_ASSERT(nullptr != certificateId);

    delete[] certificateId->serial;
    DestroyStringCopy(certificateId->issuerPublicKey);
    delete[] certificateId->issuerAki;

    memset(certificateId, 0, sizeof(*certificateId));
}

QStatus AJ_CALL alljoyn_permissionconfigurator_updatepolicy(alljoyn_permissionconfigurator configurator,
                                                            AJ_PCSTR policyXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    PermissionPolicy policy;

    status = XmlPoliciesConverter::FromXml(policyXml, policy);
    if (ER_OK != status) {
        return status;
    }

    return ((PermissionConfigurator*)configurator)->UpdatePolicy(policy);
}

QStatus PolicyToString(const PermissionPolicy& policy, AJ_PSTR* policyXml)
{
    std::string policyString;
    QStatus status = XmlPoliciesConverter::ToXml(policy, policyString);
    if (ER_OK != status) {
        *policyXml = nullptr;
        return status;
    }

    *policyXml = CreateStringCopy(policyString);
    if (nullptr == policyXml) {
        return ER_OUT_OF_MEMORY;
    }

    return ER_OK;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getpolicy(alljoyn_permissionconfigurator configurator,
                                                         AJ_PSTR* policyXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    PermissionPolicy policy;

    status = ((PermissionConfigurator*)configurator)->GetPolicy(policy);
    if (ER_OK != status) {
        *policyXml = nullptr;
        return status;
    }

    return PolicyToString(policy, policyXml);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getdefaultpolicy(alljoyn_permissionconfigurator configurator,
                                                                AJ_PSTR* policyXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    PermissionPolicy policy;

    status = ((PermissionConfigurator*)configurator)->GetDefaultPolicy(policy);
    if (ER_OK != status) {
        *policyXml = nullptr;
        return status;
    }

    return PolicyToString(policy, policyXml);
}

void AJ_CALL alljoyn_permissionconfigurator_policy_destroy(AJ_PSTR policyXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    DestroyStringCopy(policyXml);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_resetpolicy(alljoyn_permissionconfigurator configurator)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->ResetPolicy();
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getmembershipsummaries(alljoyn_permissionconfigurator configurator,
                                                                      alljoyn_certificateidarray* certificateIds)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    PermissionConfigurator* pc = (PermissionConfigurator*)configurator;
    std::vector<String> serialsVector;
    std::vector<KeyInfoNISTP256> keyInfosVector;

    memset(certificateIds, 0, sizeof(*certificateIds));

    status = pc->GetMembershipSummaries(serialsVector, keyInfosVector);
    if (ER_OK != status) {
        return status;
    }

    QCC_ASSERT(serialsVector.size() == keyInfosVector.size());

    if (serialsVector.empty()) {
        return ER_OK;
    }

    certificateIds->ids = new (std::nothrow) alljoyn_certificateid[serialsVector.size()];
    if (nullptr == certificateIds->ids) {
        return ER_OUT_OF_MEMORY;
    }
    memset(certificateIds->ids, 0, sizeof(alljoyn_certificateid) * serialsVector.size());
    certificateIds->count = serialsVector.size();

    for (std::vector<KeyInfoNISTP256>::size_type i = 0; i < keyInfosVector.size(); i++) {
        String publicKeyString;
        String akiString;

        status = KeyInfoHelper::KeyInfoNISTP256ToPEM(keyInfosVector[i], publicKeyString);
        if (ER_OK != status) {
            alljoyn_permissionconfigurator_certificateidarray_cleanup(certificateIds);
            return status;
        }
        status = KeyInfoHelper::KeyInfoNISTP256ExtractAki(keyInfosVector[i], akiString);
        if (ER_OK != status) {
            alljoyn_permissionconfigurator_certificateidarray_cleanup(certificateIds);
            return status;
        }

        uint8_t* serialBuffer = new (std::nothrow) uint8_t[serialsVector[i].size()];
        if (nullptr == serialBuffer) {
            alljoyn_permissionconfigurator_certificateidarray_cleanup(certificateIds);
            return ER_OUT_OF_MEMORY;
        }
        memcpy(serialBuffer, serialsVector[i].data(), serialsVector[i].size());
        certificateIds->ids[i].serial = serialBuffer;
        certificateIds->ids[i].serialLen = serialsVector[i].size();

        certificateIds->ids[i].issuerPublicKey = CreateStringCopy(publicKeyString);
        if (nullptr == certificateIds->ids[i].issuerPublicKey) {
            alljoyn_permissionconfigurator_certificateidarray_cleanup(certificateIds);
            return ER_OUT_OF_MEMORY;
        }

        uint8_t* akiBuffer = new (std::nothrow) uint8_t[akiString.size()];
        if (nullptr == akiBuffer) {
            alljoyn_permissionconfigurator_certificateidarray_cleanup(certificateIds);
            return ER_OUT_OF_MEMORY;
        }
        memcpy(akiBuffer, akiString.data(), akiString.size());
        certificateIds->ids[i].issuerAki = akiBuffer;
        certificateIds->ids[i].issuerAkiLen = akiString.size();
    }

    return ER_OK;
}

void AJ_CALL alljoyn_permissionconfigurator_certificateidarray_cleanup(alljoyn_certificateidarray* certificateIdArray)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QCC_ASSERT(nullptr != certificateIdArray);

    for (size_t i = 0; i < certificateIdArray->count; i++) {
        alljoyn_permissionconfigurator_certificateid_cleanup(&certificateIdArray->ids[i]);
    }

    delete[] certificateIdArray->ids;

    memset(certificateIdArray, 0, sizeof(*certificateIdArray));
}

QStatus AJ_CALL alljoyn_permissionconfigurator_installmembership(alljoyn_permissionconfigurator configurator,
                                                                 AJ_PCSTR membershipCertificateChain)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    size_t certCount = 0;
    PermissionConfigurator* pc = (PermissionConfigurator*)configurator;
    CertificateX509* certs = nullptr;

    status = ExtractCertificates(membershipCertificateChain, &certCount, &certs);

    if (ER_OK == status) {
        status = pc->InstallMembership(certs, certCount);
    }

    delete[] certs;

    return status;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_removemembership(alljoyn_permissionconfigurator configurator,
                                                                const uint8_t* serial,
                                                                size_t serialLen,
                                                                AJ_PCSTR issuerPublicKey,
                                                                const uint8_t* issuerAki,
                                                                size_t issuerAkiLen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    qcc::ECCPublicKey pubKey;

    status = qcc::CertificateX509::DecodePublicKeyPEM(issuerPublicKey, &pubKey);
    if (ER_OK != status) {
        return status;
    }

    return ((PermissionConfigurator*)configurator)->RemoveMembership(String(reinterpret_cast<AJ_PCSTR>(serial), serialLen), &pubKey, String(reinterpret_cast<AJ_PCSTR>(issuerAki), issuerAkiLen));
}

QStatus AJ_CALL alljoyn_permissionconfigurator_startmanagement(alljoyn_permissionconfigurator configurator)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->StartManagement();
}

QStatus AJ_CALL alljoyn_permissionconfigurator_endmanagement(alljoyn_permissionconfigurator configurator)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->EndManagement();
}

QStatus AJ_CALL alljoyn_permissionconfigurator_signcertificate(alljoyn_permissionconfigurator configurator, AJ_PCSTR unsignedCertificate, AJ_PSTR* signedCertificate)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    PermissionConfigurator* pc = (PermissionConfigurator*)configurator;
    CertificateX509 cert;
    qcc::String certificateStr(unsignedCertificate);

    status = cert.LoadPEM(certificateStr);

    if (ER_OK != status) {
        return status;
    }

    status = pc->SignCertificate(cert);

    if (ER_OK != status) {
        return status;
    }

    certificateStr = cert.GetPEM();
    *signedCertificate = CreateStringCopy(certificateStr);

    if (nullptr == *signedCertificate) {
        return ER_OUT_OF_MEMORY;
    }

    return status;
}

AJ_API QStatus AJ_CALL alljoyn_permissionconfigurator_signmanifest(alljoyn_permissionconfigurator configurator,
                                                                   AJ_PCSTR subjectCertificate,
                                                                   AJ_PCSTR unsignedManifestXml,
                                                                   AJ_PSTR* signedManifestXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status;
    PermissionConfigurator* pc = (PermissionConfigurator*)configurator;
    CertificateX509 cert;
    std::string input_str(unsignedManifestXml);
    qcc::String certificate_str(subjectCertificate);

    status = cert.LoadPEM(certificate_str);

    if (ER_OK != status) {
        return status;
    }

    status = pc->ComputeThumbprintAndSignManifestXml(cert, input_str);

    if (ER_OK != status) {
        return status;
    }

    *signedManifestXml = CreateStringCopy(input_str);

    if (nullptr == *signedManifestXml) {
        return ER_OUT_OF_MEMORY;
    }

    return status;
}