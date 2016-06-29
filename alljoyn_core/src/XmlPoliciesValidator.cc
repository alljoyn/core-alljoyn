/**
 * @file
 * This file defines the XmlPoliciesValidator for reading Security 2.0
 * policies from an XML.
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

#include <cctype>
#include <climits>
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>
#include "KeyInfoHelper.h"
#include "XmlPoliciesValidator.h"

using namespace qcc;

namespace ajn {

std::unordered_map<std::string, PermissionPolicy::Peer::PeerType> XmlPoliciesValidator::s_peerTypeMap;

void XmlPoliciesValidator::Init()
{
    s_peerTypeMap[XML_PEER_ALL] = PermissionPolicy::Peer::PeerType::PEER_ALL;
    s_peerTypeMap[XML_PEER_ANY_TRUSTED] = PermissionPolicy::Peer::PeerType::PEER_ANY_TRUSTED;
    s_peerTypeMap[XML_PEER_FROM_CERTIFICATE_AUTHORITY] = PermissionPolicy::Peer::PeerType::PEER_FROM_CERTIFICATE_AUTHORITY;
    s_peerTypeMap[XML_PEER_WITH_MEMBERSHIP] = PermissionPolicy::Peer::PeerType::PEER_WITH_MEMBERSHIP;
    s_peerTypeMap[XML_PEER_WITH_PUBLIC_KEY] = PermissionPolicy::Peer::PeerType::PEER_WITH_PUBLIC_KEY;
}

void XmlPoliciesValidator::Shutdown()
{
}

QStatus XmlPoliciesValidator::Validate(const XmlElement* policyXml)
{
    QStatus status = ValidateElementName(policyXml, POLICY_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateChildrenCountEqual(policyXml, POLICIES_ROOT_ELEMENT_CHILDREN_COUNT);
    }

    if (ER_OK == status) {
        status = ValidatePolicyVersion(policyXml->GetChildren()[POLICY_VERSION_INDEX]);
    }

    if (ER_OK == status) {
        status = ValidateSerialNumber(policyXml->GetChildren()[SERIAL_NUMBER_INDEX]);
    }

    if (ER_OK == status) {
        status = ValidateAcls(policyXml->GetChildren()[ACLS_INDEX]);
    }

    return (ER_OK == status) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlPoliciesValidator::Validate(const PermissionPolicy& policy)
{
    QStatus status = ValidatePolicyVersion(policy.GetSpecificationVersion());

    if (ER_OK == status) {
        status = ValidateAcls(policy.GetAcls(), policy.GetAclsSize());
    }

    return status;
}

QStatus XmlPoliciesValidator::ValidatePolicyVersion(const XmlElement* policyVersion)
{
    QStatus status = ValidateElementName(policyVersion, POLICY_VERSION_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidatePolicyVersionContent(policyVersion);
    }

    return status;
}

QStatus XmlPoliciesValidator::ValidatePolicyVersionContent(const XmlElement* policyVersion)
{
    uint32_t version = StringToU32(policyVersion->GetContent());

    return (VALID_VERSION_NUMBER != version) ? ER_XML_MALFORMED : ER_OK;
}

QStatus XmlPoliciesValidator::ValidateSerialNumber(const XmlElement* policySerialNumber)
{
    QStatus status = ValidateElementName(policySerialNumber, SERIAL_NUMBER_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateSerialNumberContent(policySerialNumber);
    }

    return status;
}

QStatus XmlPoliciesValidator::ValidateSerialNumberContent(const XmlElement* policySerialNumber)
{
    uint64_t badValue = ULLONG_MAX;
    uint64_t serialNumber = StringToU64(policySerialNumber->GetContent(), DECIMAL_BASE, badValue);

    return (serialNumber == badValue) ? ER_XML_MALFORMED : ER_OK;
}

QStatus XmlPoliciesValidator::ValidateAcls(const XmlElement* acls)
{
    QStatus status = ValidateElementName(acls, ACLS_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateChildrenCountPositive(acls);
    }

    for (auto acl : acls->GetChildren()) {
        status = ValidateAcl(acl);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlPoliciesValidator::ValidateAcl(const XmlElement* acl)
{
    QStatus status = ValidateElementName(acl, ACL_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateChildrenCountEqual(acl, ACL_ELEMENT_WITH_RULES_CHILDREN_COUNT);

        if (ER_OK != status) {
            status = ValidateChildrenCountEqual(acl, ACL_ELEMENT_WITHOUT_RULES_CHILDREN_COUNT);
        }
    }

    if (ER_OK == status) {
        status = ValidatePeers(acl->GetChildren()[PEERS_INDEX]);
    }

    if ((ER_OK == status) &&
        (acl->GetChildren().size() == ACL_ELEMENT_WITH_RULES_CHILDREN_COUNT)) {
        status = XmlRulesValidator::GetInstance()->Validate(acl->GetChildren()[RULES_INDEX]);
    }

    return status;
}

QStatus XmlPoliciesValidator::ValidatePeers(const XmlElement* peers)
{
    PeerValidatorFactory PeerValidatorFactory;
    QStatus status = ValidateElementName(peers, PEERS_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateChildrenCountPositive(peers);
    }

    for (auto peer : peers->GetChildren()) {
        status = ValidatePeer(peer, PeerValidatorFactory);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlPoliciesValidator::ValidatePeer(const XmlElement* peer, PeerValidatorFactory& peerValidatorFactory)
{
    PermissionPolicy::Peer::PeerType peerType = PermissionPolicy::Peer::PeerType::PEER_ALL;
    QStatus status = ValidateChildrenCountPositive(peer);

    if (ER_OK == status) {
        status = ValidateElementName(peer, PEER_XML_ELEMENT);
    }

    if (ER_OK == status) {
        status = PeerValidator::GetValidPeerType(peer, &peerType);
    }

    if (ER_OK == status) {
        status = peerValidatorFactory.ForType(peerType)->Validate(peer);
    }

    return status;
}

QStatus XmlPoliciesValidator::PeerValidator::Validate(const XmlElement* peer)
{
    QStatus status = ValidateCommon(peer);

    if (ER_OK == status) {
        status = ValidateTypeSpecific(peer);
    }

    return status;
}

QStatus XmlPoliciesValidator::PeerValidator::Validate(const PermissionPolicy::Peer& peer)
{
    QStatus status = ValidateCommon();

    if (ER_OK == status) {
        status = ValidateTypeSpecific(peer);
    }

    return status;
}

QStatus XmlPoliciesValidator::PeerValidator::GetValidPeerType(const XmlElement* peer, PermissionPolicy::Peer::PeerType* peerType)
{
    QStatus status = ER_OK;
    AJ_PCSTR stringPeerType = peer->GetChildren()[PEER_TYPE_INDEX]->GetContent().c_str();
    auto foundType = s_peerTypeMap.find(stringPeerType);

    if (foundType != s_peerTypeMap.end()) {
        *peerType = foundType->second;
    } else {
        status = ER_XML_MALFORMED;
    }

    return status;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateAllTypeAbsentOrOnlyPeer()
{
    return (m_allTypeAbsent || m_firstPeer) ? ER_OK : ER_FAIL;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateCommon(const XmlElement* peer)
{
    QStatus status = ValidateChildrenCount(peer);

    if (ER_OK == status) {
        status = ValidateAllTypeAbsentOrOnlyPeer();
    }

    if (ER_OK == status) {
        status = ValidateElementName(peer->GetChildren()[PEER_TYPE_INDEX], TYPE_XML_ELEMENT);
    }

    return status;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateCommon()
{
    return ValidateAllTypeAbsentOrOnlyPeer();
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateChildrenCount(const XmlElement* peer)
{
    return (peer->GetChildren().size() == GetPeerChildrenCount()) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateTypeSpecific(const XmlElement* peer)
{
    /*
     * NOP so that subclasses that don't have any type specific validation don't have to override the method.
     */
    QCC_UNUSED(peer);
    return ER_OK;
}

void XmlPoliciesValidator::PeerValidator::UpdatePeersFlags(bool allTypeAbsent, bool firstPeer, bool anyTrustedTypePresent)
{
    m_allTypeAbsent = allTypeAbsent;
    m_firstPeer = firstPeer;
    m_anyTrustedTypePresent = anyTrustedTypePresent;
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidatePublicKey(const XmlElement* peer)
{
    KeyInfoNISTP256 keyInfo;
    AJ_PCSTR publicKey = peer->GetChildren()[PEER_PUBLIC_KEY_INDEX]->GetContent().c_str();
    return KeyInfoHelper::PEMToKeyInfoNISTP256(publicKey, keyInfo);
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidatePublicKey(const PermissionPolicy::Peer& peer)
{
    return (nullptr != peer.GetKeyInfo()) ? ER_OK : ER_FAIL;
}

std::string XmlPoliciesValidator::PeerWithPublicKeyValidator::GetPeerId(const XmlElement* peer)
{
    return peer->GetChildren()[PEER_PUBLIC_KEY_INDEX]->GetContent().c_str();
}

std::string XmlPoliciesValidator::PeerWithPublicKeyValidator::GetPeerId(const PermissionPolicy::Peer& peer)
{
    return peer.GetKeyInfo()->ToString().c_str();
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidateTypeSpecific(const XmlElement* peer)
{
    QStatus status = ValidateElementName(peer->GetChildren()[PEER_PUBLIC_KEY_INDEX], PUBLIC_KEY_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidatePublicKey(peer);
    }

    if (ER_OK == status) {
        status = ValidatePeerUnique(peer);
    }

    return status;
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidateTypeSpecific(const PermissionPolicy::Peer& peer)
{
    QStatus status = ValidatePublicKey(peer);

    if (ER_OK == status) {
        status = ValidatePeerUnique(peer);
    }

    return status;
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidatePeerUnique(const XmlElement* peer)
{
    std::string id = GetPeerId(peer);
    return InsertUniqueOrFail(id, m_peersIds);
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidatePeerUnique(const PermissionPolicy::Peer& peer)
{
    std::string id = GetPeerId(peer);
    return InsertUniqueOrFail(id, m_peersIds);
}

size_t XmlPoliciesValidator::PeerWithPublicKeyValidator::GetPeerChildrenCount()
{
    return PEER_WITH_PUBLIC_KEY_FROM_CA_ELEMENTS_COUNT;
}

QStatus XmlPoliciesValidator::PeerWithoutPublicKey::ValidateTypeSpecific(const PermissionPolicy::Peer& peer)
{
    return (nullptr == peer.GetKeyInfo()) ? ER_OK : ER_FAIL;
}

size_t XmlPoliciesValidator::AllValidator::GetPeerChildrenCount()
{
    return PEER_ALL_ANY_TRUSTED_ELEMENTS_COUNT;
}

size_t XmlPoliciesValidator::AnyTrustedValidator::GetPeerChildrenCount()
{
    return PEER_ALL_ANY_TRUSTED_ELEMENTS_COUNT;
}

QStatus XmlPoliciesValidator::AnyTrustedValidator::ValidateTypeSpecific(const XmlElement* peer)
{
    QCC_UNUSED(peer);
    return m_anyTrustedTypePresent ? ER_XML_MALFORMED : ER_OK;
}

QStatus XmlPoliciesValidator::AnyTrustedValidator::ValidateTypeSpecific(const PermissionPolicy::Peer& peer)
{
    QStatus status = PeerWithoutPublicKey::ValidateTypeSpecific(peer);

    if (ER_OK == status) {
        status = m_anyTrustedTypePresent ? ER_FAIL : ER_OK;
    }

    return status;
}

QStatus XmlPoliciesValidator::WithMembershipValidator::ValidateTypeSpecific(const XmlElement* peer)
{
    QStatus status = ValidateElementName(peer->GetChildren()[PEER_SGID_INDEX], SGID_KEY_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateSgId(peer);
    }

    if (ER_OK == status) {
        status = PeerWithPublicKeyValidator::ValidateTypeSpecific(peer);
    }

    return status;
}

QStatus XmlPoliciesValidator::WithMembershipValidator::ValidateSgId(const XmlElement* peer)
{
    AJ_PCSTR sgId = peer->GetChildren()[PEER_SGID_INDEX]->GetContent().c_str();
    return GUID128::IsGUID(sgId) ? ER_OK : ER_XML_MALFORMED;
}

size_t XmlPoliciesValidator::WithMembershipValidator::GetPeerChildrenCount()
{
    return PEER_WITH_MEMBERSHIP_ELEMENTS_COUNT;
}

std::string XmlPoliciesValidator::WithMembershipValidator::GetPeerId(const XmlElement* peer)
{
    return PeerWithPublicKeyValidator::GetPeerId(peer) + peer->GetChildren()[PEER_SGID_INDEX]->GetContent().c_str();
}

std::string XmlPoliciesValidator::WithMembershipValidator::GetPeerId(const PermissionPolicy::Peer& peer)
{
    return PeerWithPublicKeyValidator::GetPeerId(peer) + peer.GetSecurityGroupId().ToString().c_str();
}

QStatus XmlPoliciesValidator::ValidatePolicyVersion(uint32_t policyVersion)
{
    return (VALID_VERSION_NUMBER == policyVersion) ? ER_OK : ER_FAIL;
}

QStatus XmlPoliciesValidator::ValidateAcls(const PermissionPolicy::Acl* acls, size_t aclsSize)
{
    QStatus status = (aclsSize > 0) ? ER_OK : ER_FAIL;

    if (ER_OK == status) {
        for (size_t index = 0; index < aclsSize; index++) {
            status = ValidateAcl(acls[index]);

            if (ER_OK != status) {
                break;
            }
        }
    }

    return status;
}

XmlPoliciesValidator::PeerValidator* XmlPoliciesValidator::PeerValidatorFactory::ForType(PermissionPolicy::Peer::PeerType type)
{
    QCC_ASSERT(m_validators.find(type) != m_validators.end());

    PeerValidator* validator = m_validators[type];
    validator->UpdatePeersFlags(m_allTypeAbsent, m_firstPeer, m_anyTrustedTypePresent);

    UpdatePeersFlags(type);
    return validator;
}

void XmlPoliciesValidator::PeerValidatorFactory::UpdatePeersFlags(PermissionPolicy::Peer::PeerType type)
{
    switch (type) {
    case PermissionPolicy::Peer::PeerType::PEER_ALL:
        m_allTypeAbsent = false;
        break;

    case PermissionPolicy::Peer::PeerType::PEER_ANY_TRUSTED:
        m_anyTrustedTypePresent = true;
        break;

    default:
        break;
    }
    m_firstPeer = false;
}

QStatus XmlPoliciesValidator::ValidateAcl(const PermissionPolicy::Acl& acl)
{
    QStatus status = ValidatePeers(acl.GetPeers(), acl.GetPeersSize());

    if ((ER_OK == status) && (acl.GetRulesSize() > 0)) {
        status = XmlRulesValidator::GetInstance()->ValidateRules(acl.GetRules(), acl.GetRulesSize());
    }

    return status;
}

QStatus XmlPoliciesValidator::ValidatePeers(const PermissionPolicy::Peer* peers, size_t peersSize)
{
    PeerValidatorFactory peerValidatorFactory;
    QStatus status = (peersSize > 0) ? ER_OK : ER_FAIL;

    if (ER_OK == status) {
        for (size_t index = 0; index < peersSize; index++) {
            status = peerValidatorFactory.ForType(peers[index].GetType())->Validate(peers[index]);

            if (ER_OK != status) {
                break;
            }
        }
    }

    return status;
}
} /* namespace ajn */
