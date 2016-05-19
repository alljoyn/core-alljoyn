/**
 * @file
 * This file defines the implementation of the Permission Configurator to allow app to setup some permission templates.
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

#include <qcc/CryptoECC.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/StringUtil.h>
#include <alljoyn/PermissionConfigurator.h>
#include <alljoyn/SecurityApplicationProxy.h>
#include "PermissionMgmtObj.h"
#include "BusInternal.h"
#include "CredentialAccessor.h"
#include "KeyInfoHelper.h"
#include "XmlRulesConverter.h"
#include "XmlManifestConverter.h"

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

/* Keep this definition in sync with the doc comment for this constant in PermissionConfigurator.h. */
const uint16_t PermissionConfigurator::CLAIM_CAPABILITIES_DEFAULT = (CAPABLE_ECDHE_NULL | CAPABLE_ECDHE_PSK | CAPABLE_ECDHE_SPEKE);

/**
 * Class for internal state of a PermissionConfigurator object.
 */
class PermissionConfigurator::Internal {

  public:
    /**
     * Constructor.
     */
    Internal(BusAttachment& bus) : m_bus(bus)
    {
    }

    /* Reference to the relevant bus attachment */
    BusAttachment& m_bus;

  private:
    /**
     * Assignment operator is private.
     */
    Internal& operator=(const Internal& other);

    /**
     * Copy constructor is private.
     */
    Internal(const Internal& other);
};

PermissionConfigurator::PermissionConfigurator(BusAttachment& bus) : m_internal(new PermissionConfigurator::Internal(bus))
{
}

PermissionConfigurator::~PermissionConfigurator()
{
    delete m_internal;
    m_internal = nullptr;
}

QStatus PermissionConfigurator::GetManifestTemplateAsXml(std::string& manifestTemplateXml)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        QCC_DbgPrintf(("PermissionConfigurator::SetPermissionManifestTemplate does not have PermissionMgmtObj initialized"));
        return ER_FEATURE_NOT_AVAILABLE;
    }

    std::vector<PermissionPolicy::Rule> manifestTemplate;
    QStatus status = permissionMgmtObj->GetManifestTemplate(manifestTemplate);
    if (ER_OK != status) {
        return status;
    }

    return XmlRulesConverter::RulesToXml(manifestTemplate.data(), manifestTemplate.size(), manifestTemplateXml, MANIFEST_XML_ELEMENT);
}

QStatus PermissionConfigurator::SetPermissionManifestTemplate(PermissionPolicy::Rule* rules, size_t count)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        QCC_DbgPrintf(("PermissionConfigurator::SetPermissionManifestTemplate does not have PermissionMgmtObj initialized"));
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->SetManifestTemplate(rules, count);
}

QStatus PermissionConfigurator::SetManifestTemplateFromXml(AJ_PCSTR manifestXml)
{
    QStatus status;
    std::vector<PermissionPolicy::Rule> rules;

    status = XmlRulesConverter::XmlToRules(manifestXml, rules);

    if (ER_OK == status) {
        status = SetPermissionManifestTemplate(rules.data(), rules.size());
    }

    return status;
}

QStatus PermissionConfigurator::GetApplicationState(ApplicationState& applicationState) const
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    applicationState = permissionMgmtObj->GetApplicationState();
    return ER_OK;
}

QStatus PermissionConfigurator::SetApplicationState(ApplicationState newState)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->SetApplicationState(newState);
}

QStatus PermissionConfigurator::Reset()
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->Reset();
}

QStatus PermissionConfigurator::GetSigningPublicKey(KeyInfoECC& keyInfo)
{
    if (keyInfo.GetCurve() != Crypto_ECC::ECC_NIST_P256) {
        return ER_NOT_IMPLEMENTED;  /* currently only support NIST P256 curve */
    }
    CredentialAccessor ca(m_internal->m_bus);
    ECCPublicKey publicKey;
    QStatus status = ca.GetDSAPublicKey(publicKey);
    if (status != ER_OK) {
        return status;
    }
    KeyInfoNISTP256* pKeyInfo = (KeyInfoNISTP256*) &keyInfo;
    pKeyInfo->SetPublicKey(&publicKey);
    KeyInfoHelper::GenerateKeyId(*pKeyInfo);
    return ER_OK;
}

QStatus PermissionConfigurator::SignCertificate(CertificateX509& cert)
{
    CredentialAccessor ca(m_internal->m_bus);
    ECCPrivateKey privateKey;
    QStatus status = ca.GetDSAPrivateKey(privateKey);
    if (status != ER_OK) {
        return status;
    }
    ECCPublicKey publicKey;
    status = ca.GetDSAPublicKey(publicKey);
    if (status != ER_OK) {
        return status;
    }
    return cert.SignAndGenerateAuthorityKeyId(&privateKey, &publicKey);
}

QStatus PermissionConfigurator::SignManifest(const std::vector<uint8_t>& subjectThumbprint, Manifest& manifest)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CredentialAccessor ca(m_internal->m_bus);
    ECCPrivateKey privateKey;
    QStatus status = ca.GetDSAPrivateKey(privateKey);
    if (status != ER_OK) {
        QCC_LogError(status, ("Could not GetDSAPrivateKey"));
        return status;
    }

    status = manifest->Sign(subjectThumbprint, &privateKey);

    return status;
}

QStatus PermissionConfigurator::ComputeThumbprintAndSignManifest(const qcc::CertificateX509& subjectCertificate, Manifest& manifest)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CredentialAccessor ca(m_internal->m_bus);
    ECCPrivateKey privateKey;
    QStatus status = ca.GetDSAPrivateKey(privateKey);
    if (status != ER_OK) {
        QCC_LogError(status, ("Could not GetDSAPrivateKey"));
        return status;
    }

    status = manifest->ComputeThumbprintAndSign(subjectCertificate, &privateKey);

    return status;
}

QStatus PermissionConfigurator::GetConnectedPeerPublicKey(const GUID128& guid, qcc::ECCPublicKey* publicKey)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->GetConnectedPeerPublicKey(guid, publicKey);
}

QStatus PermissionConfigurator::SetClaimCapabilities(PermissionConfigurator::ClaimCapabilities claimCapabilities)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->SetClaimCapabilities(claimCapabilities);
}

QStatus PermissionConfigurator::SetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo additionalInfo)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->SetClaimCapabilityAdditionalInfo(additionalInfo);
}

QStatus PermissionConfigurator::GetClaimCapabilities(PermissionConfigurator::ClaimCapabilities& claimCapabilities) const
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->GetClaimCapabilities(claimCapabilities);
}

QStatus PermissionConfigurator::GetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo& additionalInfo) const
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->GetClaimCapabilityAdditionalInfo(additionalInfo);
}

QStatus PermissionConfigurator::Claim(
    const KeyInfoNISTP256& certificateAuthority,
    const qcc::GUID128& adminGroupGuid,
    const KeyInfoNISTP256& adminGroupAuthority,
    const qcc::CertificateX509* identityCertChain,
    size_t identityCertChainCount,
    AJ_PCSTR* manifestsXmls,
    size_t manifestsCount)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }

    std::vector<Manifest> manifests;
    QStatus status = XmlManifestConverter::XmlArrayToManifests(manifestsXmls, manifestsCount, manifests);

    if (ER_OK == status) {
        PermissionMgmtObj::TrustAnchor caTrustAnchor(PermissionMgmtObj::TRUST_ANCHOR_CA, certificateAuthority);
        PermissionMgmtObj::TrustAnchor adminGroupAnchor(PermissionMgmtObj::TRUST_ANCHOR_SG_AUTHORITY, adminGroupAuthority);
        adminGroupAnchor.securityGroupId = adminGroupGuid;

        return permissionMgmtObj->Claim(
            caTrustAnchor,
            adminGroupAnchor,
            identityCertChain,
            identityCertChainCount,
            manifests.data(),
            manifests.size());
    } else {
        return status;
    }
}

QStatus PermissionConfigurator::UpdateIdentity(
    const qcc::CertificateX509* certs,
    size_t certCount,
    AJ_PCSTR* manifestsXmls,
    size_t manifestsCount)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }

    std::vector<Manifest> manifests;
    QStatus status = XmlManifestConverter::XmlArrayToManifests(manifestsXmls, manifestsCount, manifests);

    if (ER_OK == status) {
        return permissionMgmtObj->UpdateIdentity(certs, certCount, manifests.data(), manifests.size());
    } else {
        return status;
    }
}

QStatus PermissionConfigurator::GetIdentity(std::vector<qcc::CertificateX509>& certChain)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->GetIdentity(certChain);
}

QStatus PermissionConfigurator::GetManifests(std::vector<Manifest>& manifests)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->RetrieveManifests(manifests);
}

QStatus PermissionConfigurator::GetIdentityCertificateId(qcc::String& serial, qcc::KeyInfoNISTP256& issuerKeyInfo)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->RetrieveIdentityCertificateId(serial, issuerKeyInfo);
}

QStatus PermissionConfigurator::UpdatePolicy(const PermissionPolicy& policy)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->InstallPolicy(policy);
}

QStatus PermissionConfigurator::GetPolicy(PermissionPolicy& policy)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->RetrievePolicy(policy, false);
}

QStatus PermissionConfigurator::GetDefaultPolicy(PermissionPolicy& policy)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->RetrievePolicy(policy, true);
}

QStatus PermissionConfigurator::ResetPolicy()
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->ResetPolicy();
}

QStatus PermissionConfigurator::GetMembershipSummaries(std::vector<String>& serials, std::vector<KeyInfoNISTP256>& keyInfos)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        serials.clear();
        keyInfos.clear();
        return ER_FEATURE_NOT_AVAILABLE;
    }

    MsgArg arg;
    QStatus status = permissionMgmtObj->GetMembershipSummaries(arg);
    if (ER_OK != status) {
        serials.clear();
        keyInfos.clear();
        return status;
    }

    size_t count = arg.v_array.GetNumElements();
    serials.resize(count);
    keyInfos.resize(count);
    status = SecurityApplicationProxy::MsgArgToCertificateIds(arg, serials.data(), keyInfos.data(), count);

    if (ER_OK != status) {
        serials.clear();
        keyInfos.clear();
    }

    return status;
}

QStatus PermissionConfigurator::InstallMembership(const qcc::CertificateX509* certChain, size_t certCount)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->StoreMembership(certChain, certCount);
}

QStatus PermissionConfigurator::RemoveMembership(const qcc::String& serial, const qcc::ECCPublicKey* issuerPubKey, const qcc::String& issuerAki)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->RemoveMembership(serial, issuerPubKey, issuerAki);
}

QStatus PermissionConfigurator::RemoveMembership(const qcc::String& serial, const qcc::KeyInfoNISTP256& issuerKeyInfo)
{
    String issuerAki((const char*)issuerKeyInfo.GetKeyId(), issuerKeyInfo.GetKeyIdLen());

    return RemoveMembership(serial, issuerKeyInfo.GetPublicKey(), issuerAki);
}

QStatus PermissionConfigurator::StartManagement()
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->StartManagement();
}

QStatus PermissionConfigurator::EndManagement()
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->EndManagement();
}

QStatus PermissionConfigurator::InstallManifests(AJ_PCSTR* manifestsXmls, size_t manifestsCount, bool append)
{
    PermissionMgmtObj* permissionMgmtObj = m_internal->m_bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    std::vector<Manifest> manifests;
    QStatus status = XmlManifestConverter::XmlArrayToManifests(manifestsXmls, manifestsCount, manifests);

    if (ER_OK == status) {
        return permissionMgmtObj->StoreManifests(manifests.size(), manifests.data(), append);
    } else {
        return status;
    }
}


}
