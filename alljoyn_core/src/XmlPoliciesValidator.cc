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
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>
#include "KeyInfoHelper.h"
#include "XmlPoliciesValidator.h"

using namespace qcc;

namespace ajn {

std::unordered_map<std::string, PermissionPolicy::Peer::PeerType> XmlPoliciesValidator::s_peerTypeMap;

void XmlPoliciesValidator::Init()
{
    static bool initialized = false;

    if (!initialized) {
        s_peerTypeMap[XML_PEER_ALL] = PermissionPolicy::Peer::PeerType::PEER_ALL;
        s_peerTypeMap[XML_PEER_ANY_TRUSTED] = PermissionPolicy::Peer::PeerType::PEER_ANY_TRUSTED;
        s_peerTypeMap[XML_PEER_FROM_CERTIFICATE_AUTHORITY] = PermissionPolicy::Peer::PeerType::PEER_FROM_CERTIFICATE_AUTHORITY;
        s_peerTypeMap[XML_PEER_WITH_MEMBERSHIP] = PermissionPolicy::Peer::PeerType::PEER_WITH_MEMBERSHIP;
        s_peerTypeMap[XML_PEER_WITH_PUBLIC_KEY] = PermissionPolicy::Peer::PeerType::PEER_WITH_PUBLIC_KEY;

        initialized = true;
    }
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
    uint32_t version = qcc::StringToU32(policyVersion->GetContent());

    if (VALID_VERSION_NUMBER != version) {
        return ER_XML_MALFORMED;
    }

    return ER_OK;
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
    uint32_t badValue = 0;
    uint32_t serialNumber = qcc::StringToU32(policySerialNumber->GetContent(), DECIMAL_BASE, badValue);

    if (serialNumber == badValue) {
        return ER_XML_MALFORMED;
    }

    return ER_OK;
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
        status = ValidateChildrenCountEqual(acl, ACL_ELEMENT_CHILDREN_COUNT);
    }

    if (ER_OK == status) {
        status = ValidatePeers(acl->GetChildren()[PEERS_INDEX]);
    }

    if (ER_OK == status) {
        status = XmlRulesValidator::Validate(acl->GetChildren()[RULES_INDEX]);
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

QStatus XmlPoliciesValidator::PeerValidator::Validate(const qcc::XmlElement* peer)
{
    QStatus status = ValidateCommon(peer);

    if (ER_OK == status) {
        status = ValidateTypeSpecific(peer);
    }

    return status;
}

QStatus XmlPoliciesValidator::PeerValidator::GetValidPeerType(const qcc::XmlElement* peer, PermissionPolicy::Peer::PeerType* peerType)
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

QStatus XmlPoliciesValidator::PeerValidator::ValidateNoAllTypeWithOther()
{
    return (m_allTypeAbsent || m_firstPeer) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateCommon(const qcc::XmlElement* peer)
{
    QStatus status = ValidateChildrenCount(peer);

    if (ER_OK == status) {
        status = ValidateNoAllTypeWithOther();
    }

    if (ER_OK == status) {
        status = ValidateElementName(peer->GetChildren()[PEER_TYPE_INDEX], TYPE_XML_ELEMENT);
    }

    return status;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateChildrenCount(const qcc::XmlElement* peer)
{
    return (peer->GetChildren().size() == GetPeerChildrenCount()) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateTypeSpecific(const qcc::XmlElement* peer)
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

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidatePublicKey(const qcc::XmlElement* peer)
{
    KeyInfoNISTP256 keyInfo;
    AJ_PCSTR publicKey = peer->GetChildren()[PEER_PUBLIC_KEY_INDEX]->GetContent().c_str();
    return KeyInfoHelper::PEMToKeyInfoNISTP256(publicKey, keyInfo);
}

std::string XmlPoliciesValidator::PeerWithPublicKeyValidator::GetPeerId(const qcc::XmlElement* peer)
{
    return peer->GetChildren()[PEER_PUBLIC_KEY_INDEX]->GetContent().c_str();
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidateTypeSpecific(const qcc::XmlElement* peer)
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

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidatePeerUnique(const qcc::XmlElement* peer)
{
    std::string id = GetPeerId(peer);
    return InsertUniqueOrFail(id, m_peersIds);
}

size_t XmlPoliciesValidator::PeerWithPublicKeyValidator::GetPeerChildrenCount()
{
    return PEER_WITH_PUBLIC_KEY_FROM_CA_ELEMENTS_COUNT;
}

size_t XmlPoliciesValidator::AllValidator::GetPeerChildrenCount()
{
    return PEER_ALL_ANY_TRUSTED_ELEMENTS_COUNT;
}

size_t XmlPoliciesValidator::AnyTrustedValidator::GetPeerChildrenCount()
{
    return PEER_ALL_ANY_TRUSTED_ELEMENTS_COUNT;
}

QStatus XmlPoliciesValidator::AnyTrustedValidator::ValidateTypeSpecific(const qcc::XmlElement* peer)
{
    QCC_UNUSED(peer);
    return m_anyTrustedTypePresent ? ER_XML_MALFORMED : ER_OK;
}

QStatus XmlPoliciesValidator::WithMembershipValidator::ValidateTypeSpecific(const qcc::XmlElement* peer)
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

QStatus XmlPoliciesValidator::WithMembershipValidator::ValidateSgId(const qcc::XmlElement* peer)
{
    AJ_PCSTR sgId = peer->GetChildren()[PEER_SGID_INDEX]->GetContent().c_str();
    return qcc::GUID128::IsGUID(sgId) ? ER_OK : ER_XML_MALFORMED;
}

size_t XmlPoliciesValidator::WithMembershipValidator::GetPeerChildrenCount()
{
    return PEER_WITH_MEMBERSHIP_ELEMENTS_COUNT;
}

std::string XmlPoliciesValidator::WithMembershipValidator::GetPeerId(const qcc::XmlElement* peer)
{
    return PeerWithPublicKeyValidator::GetPeerId(peer) + peer->GetChildren()[PEER_SGID_INDEX]->GetContent().c_str();
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
} /* namespace ajn */
