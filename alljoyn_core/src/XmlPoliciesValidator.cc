/**
 * @file
 * This file defines the XmlPoliciesValidator for reading Security 2.0
 * policies from an XML.
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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <cctype>
#include <climits>
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>
#include "KeyInfoHelper.h"
#include "XmlPoliciesValidator.h"

#define QCC_MODULE "XML_CONVERTER"

using namespace qcc;
using namespace std;

namespace ajn {

unordered_map<string, PermissionPolicy::Peer::PeerType>* XmlPoliciesValidator::s_peerTypeMap = nullptr;

void XmlPoliciesValidator::Init()
{
    QCC_DbgTrace(("%s: Performing validator init.", __FUNCTION__));
    QCC_ASSERT(nullptr == s_peerTypeMap);

    s_peerTypeMap = new unordered_map<string, PermissionPolicy::Peer::PeerType>();

    (*s_peerTypeMap)[XML_PEER_ALL] = PermissionPolicy::Peer::PeerType::PEER_ALL;
    (*s_peerTypeMap)[XML_PEER_ANY_TRUSTED] = PermissionPolicy::Peer::PeerType::PEER_ANY_TRUSTED;
    (*s_peerTypeMap)[XML_PEER_FROM_CERTIFICATE_AUTHORITY] = PermissionPolicy::Peer::PeerType::PEER_FROM_CERTIFICATE_AUTHORITY;
    (*s_peerTypeMap)[XML_PEER_WITH_MEMBERSHIP] = PermissionPolicy::Peer::PeerType::PEER_WITH_MEMBERSHIP;
    (*s_peerTypeMap)[XML_PEER_WITH_PUBLIC_KEY] = PermissionPolicy::Peer::PeerType::PEER_WITH_PUBLIC_KEY;
}

void XmlPoliciesValidator::Shutdown()
{
    QCC_DbgTrace(("%s: Performing validator cleanup.", __FUNCTION__));

    delete s_peerTypeMap;
    s_peerTypeMap = nullptr;
}

QStatus XmlPoliciesValidator::Validate(const XmlElement* policyXml)
{
    QCC_DbgHLPrintf(("%s: Validating security policy XML: %s",
                     __FUNCTION__, policyXml->Generate().c_str()));

    QStatus status = ValidateElementName(policyXml, POLICY_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateChildrenCountEqual(policyXml, POLICIES_ROOT_ELEMENT_CHILDREN_COUNT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidatePolicyVersion(policyXml->GetChildren()[POLICY_VERSION_INDEX]);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateSerialNumber(policyXml->GetChildren()[SERIAL_NUMBER_INDEX]);
    if (ER_OK != status) {
        return status;
    }

    return ValidateAcls(policyXml->GetChildren()[ACLS_INDEX]);
}

QStatus XmlPoliciesValidator::Validate(const PermissionPolicy& policy)
{
    QCC_DbgHLPrintf(("%s: Validating security policy object: %s",
                     __FUNCTION__, policy.ToString().c_str()));

    QStatus status = ValidatePolicyVersion(policy.GetSpecificationVersion());
    if (ER_OK != status) {
        return status;
    }

    return ValidateAcls(policy.GetAcls(), policy.GetAclsSize());
}

QStatus XmlPoliciesValidator::ValidatePolicyVersion(const XmlElement* policyVersion)
{
    QStatus status = ValidateElementName(policyVersion, POLICY_VERSION_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    return ValidatePolicyVersionContent(policyVersion);
}

QStatus XmlPoliciesValidator::ValidatePolicyVersionContent(const XmlElement* policyVersion)
{
    uint32_t version = StringToU32(policyVersion->GetContent());
    if (VALID_VERSION_NUMBER != version) {
        QCC_LogError(ER_XML_INVALID_POLICY_VERSION,
                     ("%s: Invalid security policy version. Expected: %u. Was: %u.",
                      __FUNCTION__, VALID_VERSION_NUMBER, version));

        return ER_XML_INVALID_POLICY_VERSION;
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::ValidateSerialNumber(const XmlElement* policySerialNumber)
{
    QStatus status = ValidateElementName(policySerialNumber, SERIAL_NUMBER_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    return ValidateSerialNumberContent(policySerialNumber);
}

QStatus XmlPoliciesValidator::ValidateSerialNumberContent(const XmlElement* policySerialNumber)
{
    String serialNumberString = policySerialNumber->GetContent();
    uint64_t badValue = ULLONG_MAX;
    uint64_t serialNumber = StringToU64(serialNumberString, DECIMAL_BASE, badValue);

    if (serialNumber == badValue) {
        QCC_LogError(ER_XML_INVALID_POLICY_SERIAL_NUMBER,
                     ("%s: Invalid security policy serial number value. Expected a decimal based number. Was: %s",
                      __FUNCTION__, serialNumberString.c_str()));

        return ER_XML_INVALID_POLICY_SERIAL_NUMBER;
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::ValidateAcls(const XmlElement* acls)
{
    QStatus status = ValidateElementName(acls, ACLS_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateChildrenCountPositive(acls);
    if (ER_OK != status) {
        return status;
    }

    for (auto acl : acls->GetChildren()) {
        status = ValidateAcl(acl);
        if (ER_OK != status) {
            return status;
        }
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::ValidateAcl(const XmlElement* acl)
{
    QStatus status = ValidateElementName(acl, ACL_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateAclChildrenCount(acl);
    if (ER_OK != status) {
        return status;
    }

    status = ValidatePeers(acl->GetChildren()[PEERS_INDEX]);
    if (ER_OK != status) {
        return status;
    }

    if (acl->GetChildren().size() == ACL_ELEMENT_WITH_RULES_CHILDREN_COUNT) {
        status = XmlRulesValidator::GetInstance()->Validate(acl->GetChildren()[RULES_INDEX]);
    }

    return status;
}

QStatus XmlPoliciesValidator::ValidateAclChildrenCount(const XmlElement* acl)
{
    QStatus status = ValidateChildrenCountEqual(acl, ACL_ELEMENT_WITH_RULES_CHILDREN_COUNT);
    if (ER_OK != status) {
        status = ValidateChildrenCountEqual(acl, ACL_ELEMENT_WITHOUT_RULES_CHILDREN_COUNT);
    }

    return status;
}

QStatus XmlPoliciesValidator::ValidatePeers(const XmlElement* peers)
{
    PeerValidatorFactory PeerValidatorFactory;
    QStatus status = ValidateElementName(peers, PEERS_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateChildrenCountPositive(peers);
    if (ER_OK != status) {
        return status;
    }

    for (auto peer : peers->GetChildren()) {
        status = ValidatePeer(peer, PeerValidatorFactory);
        if (ER_OK != status) {
            return status;
        }
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::ValidatePeer(const XmlElement* peer, PeerValidatorFactory& peerValidatorFactory)
{
    PermissionPolicy::Peer::PeerType peerType;
    QStatus status = ValidateChildrenCountPositive(peer);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateElementName(peer, PEER_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = PeerValidator::GetValidPeerType(peer, &peerType);
    if (ER_OK != status) {
        return status;
    }

    return peerValidatorFactory.ForType(peerType)->Validate(peer);
}

QStatus XmlPoliciesValidator::PeerValidator::Validate(const XmlElement* peer)
{
    QCC_DbgHLPrintf(("%s: Validating security policy XML peer: %s",
                     __FUNCTION__, peer->Generate().c_str()));

    QStatus status = ValidateCommon(peer);
    if (ER_OK != status) {
        return status;
    }

    return ValidateTypeSpecific(peer);
}

QStatus XmlPoliciesValidator::PeerValidator::Validate(const PermissionPolicy::Peer& peer)
{
    QCC_DbgHLPrintf(("%s: Validating security policy peer object: %s",
                     __FUNCTION__, peer.ToString().c_str()));

    QStatus status = ValidateCommon(peer);
    if (ER_OK != status) {
        return status;
    }

    return ValidateTypeSpecific(peer);
}

QStatus XmlPoliciesValidator::PeerValidator::GetValidPeerType(const XmlElement* peer, PermissionPolicy::Peer::PeerType* peerType)
{
    AJ_PCSTR stringPeerType;
    const XmlElement* peerTypeElement = peer->GetChildren()[PEER_TYPE_INDEX];
    QStatus status = ValidateElementName(peerTypeElement, TYPE_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    stringPeerType = peerTypeElement->GetContent().c_str();
    auto foundType = s_peerTypeMap->find(stringPeerType);
    if (foundType != s_peerTypeMap->end()) {
        *peerType = foundType->second;
    } else {
        QCC_LogError(ER_XML_INVALID_ACL_PEER_TYPE,
                     ("%s: Invalid ACL peer type: %s.",
                      __FUNCTION__, stringPeerType));

        return ER_XML_INVALID_ACL_PEER_TYPE;
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateTypeSpecific(const XmlElement* peer)
{
    /**
     * NOP so that subclasses that don't have any type specific validation don't have to override the method.
     */
    QCC_UNUSED(peer);
    return ER_OK;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateTypeSpecific(const PermissionPolicy::Peer& peer)
{
    /**
     * NOP so that subclasses that don't have any type specific validation don't have to override the method.
     */
    QCC_UNUSED(peer);
    return ER_OK;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateAllTypeAbsentOrOnlyPeer()
{
    if (!m_allTypeAbsent) {
        QCC_LogError(ER_XML_ACL_ALL_TYPE_PEER_WITH_OTHERS,
                     ("%s: \"ALL\" type peer is present with other peers in one ACL.",
                      __FUNCTION__));

        return ER_XML_ACL_ALL_TYPE_PEER_WITH_OTHERS;
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateCommon(const XmlElement* peer)
{
    QStatus status = ValidateChildrenCount(peer);
    if (ER_OK != status) {
        return status;
    }

    status = ValidatePeerUnique(peer);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateAllTypeAbsentOrOnlyPeer();
    if (ER_OK != status) {
        return status;
    }

    return ValidateElementName(peer->GetChildren()[PEER_TYPE_INDEX], TYPE_XML_ELEMENT);
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateCommon(const PermissionPolicy::Peer& peer)
{
    QStatus status = ValidatePeerUnique(peer);
    if (ER_OK != status) {
        return status;
    }

    return ValidateAllTypeAbsentOrOnlyPeer();
}

QStatus XmlPoliciesValidator::PeerValidator::ValidateChildrenCount(const XmlElement* peer)
{
    size_t peerChildrenCount = peer->GetChildren().size();
    size_t expectedChildrenCount = GetPeerChildrenCount();

    if (peerChildrenCount != expectedChildrenCount) {
        QCC_LogError(ER_XML_INVALID_ACL_PEER_CHILDREN_COUNT,
                     ("%s: Invalid ACL peer children count. Expected: %u. Was: %u.",
                      __FUNCTION__, expectedChildrenCount, peerChildrenCount));

        return ER_XML_INVALID_ACL_PEER_CHILDREN_COUNT;
    }

    return ER_OK;
}

void XmlPoliciesValidator::PeerValidator::UpdatePeersFlags(bool allTypePresent, bool firstPeer)
{
    m_allTypeAbsent = allTypePresent;
    m_firstPeer = firstPeer;
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidateTypeSpecific(const XmlElement* peer)
{
    QStatus status = ValidateElementName(peer->GetChildren()[PEER_PUBLIC_KEY_INDEX], PUBLIC_KEY_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    return ValidatePublicKey(peer);
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::GetPeerId(const XmlElement* peer, string& peerId)
{
    peerId = peer->GetChildren()[PEER_PUBLIC_KEY_INDEX]->GetContent().c_str();

    return ER_OK;
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::GetPeerId(const PermissionPolicy::Peer& peer, string& peerId)
{
    QStatus status = ValidatePublicKey(peer);
    if (ER_OK != status) {
        return status;
    }

    peerId = peer.GetKeyInfo()->ToString().c_str();

    return ER_OK;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidatePeerUnique(const XmlElement* peer)
{
    string id;
    QStatus status = GetPeerId(peer, id);
    if (ER_OK != status) {
        return status;
    }

    status = InsertUniqueOrFail(id, m_peersIds);
    if (ER_OK != status) {
        QCC_LogError(ER_XML_ACL_PEER_NOT_UNIQUE,
                     ("%s: ACL peer already exists: %s",
                      __FUNCTION__, peer->Generate().c_str()));

        return ER_XML_ACL_PEER_NOT_UNIQUE;
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::PeerValidator::ValidatePeerUnique(const PermissionPolicy::Peer& peer)
{
    string id;
    QStatus status = GetPeerId(peer, id);
    if (ER_OK != status) {
        return status;
    }

    status = InsertUniqueOrFail(id, m_peersIds);
    if (ER_OK != status) {
        QCC_LogError(ER_XML_ACL_PEER_NOT_UNIQUE,
                     ("%s: ACL peer already exists: %s",
                      __FUNCTION__, peer.ToString().c_str()));

        return ER_XML_ACL_PEER_NOT_UNIQUE;
    }

    return ER_OK;
}

size_t XmlPoliciesValidator::PeerWithPublicKeyValidator::GetPeerChildrenCount()
{
    return PEER_WITH_PUBLIC_KEY_FROM_CA_ELEMENTS_COUNT;
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidatePublicKey(const XmlElement* peer)
{
    KeyInfoNISTP256 keyInfo;
    AJ_PCSTR publicKey = peer->GetChildren()[PEER_PUBLIC_KEY_INDEX]->GetContent().c_str();
    QStatus status = KeyInfoHelper::PEMToKeyInfoNISTP256(publicKey, keyInfo);

    if (ER_OK != status) {
        QCC_LogError(ER_XML_INVALID_ACL_PEER_PUBLIC_KEY,
                     ("%s: ACL peer public key not in valid PEM format: %s.",
                      __FUNCTION__, publicKey));

        return ER_XML_INVALID_ACL_PEER_PUBLIC_KEY;
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::PeerWithPublicKeyValidator::ValidatePublicKey(const PermissionPolicy::Peer& peer)
{
    if (nullptr == peer.GetKeyInfo()) {
        QCC_LogError(ER_XML_INVALID_ACL_PEER_PUBLIC_KEY,
                     ("%s: ACL peer public key missing.",
                      __FUNCTION__));

        return ER_XML_INVALID_ACL_PEER_PUBLIC_KEY;
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::PeerWithoutPublicKey::ValidateTypeSpecific(const PermissionPolicy::Peer& peer)
{
    if (nullptr != peer.GetKeyInfo()) {
        QCC_LogError(ER_XML_ACL_PEER_PUBLIC_KEY_SET,
                     ("%s: ACL peer public key should not be set for this peer type(%x).",
                      __FUNCTION__, peer.GetType()));

        return ER_XML_ACL_PEER_PUBLIC_KEY_SET;
    }

    return ER_OK;
}

size_t XmlPoliciesValidator::AllValidator::GetPeerChildrenCount()
{
    return PEER_ALL_ANY_TRUSTED_ELEMENTS_COUNT;
}

QStatus XmlPoliciesValidator::AllValidator::GetPeerId(const XmlElement* peer, string& peerId)
{
    QCC_UNUSED(peer);

    peerId = XML_PEER_ALL;

    return ER_OK;
}

QStatus XmlPoliciesValidator::AllValidator::GetPeerId(const PermissionPolicy::Peer& peer, string& peerId)
{
    QCC_UNUSED(peer);

    peerId = XML_PEER_ALL;

    return ER_OK;
}

QStatus XmlPoliciesValidator::AllValidator::ValidateAllTypeAbsentOrOnlyPeer()
{
    if (!m_firstPeer) {
        QCC_LogError(ER_XML_ACL_ALL_TYPE_PEER_WITH_OTHERS,
                     ("%s: \"ALL\" type peer is present with other peers in one ACL.",
                      __FUNCTION__));

        return ER_XML_ACL_ALL_TYPE_PEER_WITH_OTHERS;
    }

    return ER_OK;
}

size_t XmlPoliciesValidator::AnyTrustedValidator::GetPeerChildrenCount()
{
    return PEER_ALL_ANY_TRUSTED_ELEMENTS_COUNT;
}

QStatus XmlPoliciesValidator::AnyTrustedValidator::GetPeerId(const XmlElement* peer, string& peerId)
{
    QCC_UNUSED(peer);

    peerId = XML_PEER_ANY_TRUSTED;

    return ER_OK;
}

QStatus XmlPoliciesValidator::AnyTrustedValidator::GetPeerId(const PermissionPolicy::Peer& peer, string& peerId)
{
    QCC_UNUSED(peer);

    peerId = XML_PEER_ANY_TRUSTED;

    return ER_OK;
}

QStatus XmlPoliciesValidator::WithMembershipValidator::ValidateTypeSpecific(const XmlElement* peer)
{
    QStatus status = PeerWithPublicKeyValidator::ValidateTypeSpecific(peer);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateElementName(peer->GetChildren()[PEER_SGID_INDEX], SGID_KEY_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    return ValidateSgId(peer);
}

QStatus XmlPoliciesValidator::WithMembershipValidator::ValidateSgId(const XmlElement* peer)
{
    AJ_PCSTR sgId = peer->GetChildren()[PEER_SGID_INDEX]->GetContent().c_str();

    if (!GUID128::IsGUID(sgId)) {
        QCC_LogError(ER_INVALID_GUID,
                     ("%s: Peer's security group GUID is in invalid GUID format: %s.",
                      __FUNCTION__, sgId));

        return ER_INVALID_GUID;
    }

    return ER_OK;
}

size_t XmlPoliciesValidator::WithMembershipValidator::GetPeerChildrenCount()
{
    return PEER_WITH_MEMBERSHIP_ELEMENTS_COUNT;
}

QStatus XmlPoliciesValidator::WithMembershipValidator::GetPeerId(const XmlElement* peer, string& peerId)
{
    QStatus status = PeerWithPublicKeyValidator::GetPeerId(peer, peerId);
    if (ER_OK != status) {
        return status;
    }

    peerId += peer->GetChildren()[PEER_SGID_INDEX]->GetContent().c_str();

    return ER_OK;
}

QStatus XmlPoliciesValidator::WithMembershipValidator::GetPeerId(const PermissionPolicy::Peer& peer, string& peerId)
{
    QStatus status = PeerWithPublicKeyValidator::GetPeerId(peer, peerId);
    if (ER_OK != status) {
        return status;
    }

    peerId += peer.GetSecurityGroupId().ToString().c_str();

    return ER_OK;
}

QStatus XmlPoliciesValidator::ValidatePolicyVersion(uint32_t policyVersion)
{
    if (VALID_VERSION_NUMBER != policyVersion) {
        QCC_LogError(ER_XML_INVALID_POLICY_VERSION,
                     ("%s: Invalid security policy version. Expected: %u. Was: %u.",
                      __FUNCTION__, VALID_VERSION_NUMBER, policyVersion));

        return ER_XML_INVALID_POLICY_VERSION;
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::ValidateAcls(const PermissionPolicy::Acl* acls, size_t aclsSize)
{
    QStatus status = ValidateAclsCount(aclsSize);
    if (ER_OK != status) {
        return status;
    }

    for (size_t index = 0; index < aclsSize; index++) {
        status = ValidateAcl(acls[index]);
        if (ER_OK != status) {
            return status;
        }
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::ValidateAclsCount(size_t aclsSize)
{
    if (0 == aclsSize) {
        QCC_LogError(ER_XML_ACLS_MISSING,
                     ("%s: Policy contains no ACLs.",
                      __FUNCTION__));

        return ER_XML_ACLS_MISSING;
    }

    return ER_OK;
}

XmlPoliciesValidator::PeerValidator* XmlPoliciesValidator::PeerValidatorFactory::ForType(PermissionPolicy::Peer::PeerType type)
{
    QCC_ASSERT(m_validators.find(type) != m_validators.end());

    PeerValidator* validator = m_validators[type];
    validator->UpdatePeersFlags(m_allTypeAbsent, m_firstPeer);

    UpdatePeersFlags(type);
    return validator;
}

void XmlPoliciesValidator::PeerValidatorFactory::UpdatePeersFlags(PermissionPolicy::Peer::PeerType type)
{
    if (PermissionPolicy::Peer::PeerType::PEER_ALL == type) {
        m_allTypeAbsent = false;
    }
    m_firstPeer = false;
}

QStatus XmlPoliciesValidator::ValidateAcl(const PermissionPolicy::Acl& acl)
{
    QStatus status = ValidatePeers(acl.GetPeers(), acl.GetPeersSize());
    if (ER_OK != status) {
        return status;
    }

    if (acl.GetRulesSize() > 0) {
        status = XmlRulesValidator::GetInstance()->ValidateRules(acl.GetRules(), acl.GetRulesSize());
    }

    return status;
}

QStatus XmlPoliciesValidator::ValidatePeers(const PermissionPolicy::Peer* peers, size_t peersSize)
{
    PeerValidatorFactory peerValidatorFactory;
    QStatus status = ValidatePeersCount(peersSize);
    if (ER_OK != status) {
        return status;
    }

    for (size_t index = 0; index < peersSize; index++) {
        status = peerValidatorFactory.ForType(peers[index].GetType())->Validate(peers[index]);
        if (ER_OK != status) {
            return status;
        }
    }

    return ER_OK;
}

QStatus XmlPoliciesValidator::ValidatePeersCount(size_t peersSize)
{
    if (0 == peersSize) {
        QCC_LogError(ER_XML_ACL_PEERS_MISSING,
                     ("%s: ACL contains no peers.",
                      __FUNCTION__));

        return ER_XML_ACL_PEERS_MISSING;
    }

    return ER_OK;
}
} /* namespace ajn */