/**
 * @file
 * This file defines the XmlPoliciesConverter for reading Security 2.0
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

#include <qcc/StringUtil.h>
#include "KeyInfoHelper.h"
#include "XmlPoliciesConverter.h"
#include "XmlRulesConverter.h"

using namespace qcc;

namespace ajn {

QStatus XmlPoliciesConverter::FromXml(AJ_PCSTR policyXml, ajn::PermissionPolicy& policy)
{
    QCC_ASSERT(nullptr != policyXml);
    QCC_ASSERT(nullptr != &policy);

    XmlElement* root = nullptr;
    QStatus status = qcc::XmlElement::GetRoot(policyXml, &root);

    if (ER_OK == status) {
        status = XmlPoliciesValidator::Validate(root);
    }

    if (ER_OK == status) {
        BuildPolicy(root, policy);
    }

    delete root;
    return status;
}

void XmlPoliciesConverter::BuildPolicy(const qcc::XmlElement* root, ajn::PermissionPolicy& policy)
{
    SetPolicyVersion(root->GetChildren()[POLICY_VERSION_INDEX], policy);
    SetPolicySerialNumber(root->GetChildren()[SERIAL_NUMBER_INDEX], policy);
    SetPolicyAcls(root->GetChildren()[ACLS_INDEX], policy);
}

void XmlPoliciesConverter::SetPolicyVersion(const qcc::XmlElement* xmlPolicyVersion, PermissionPolicy& policy)
{
    uint32_t policyVersion = qcc::StringToU32(xmlPolicyVersion->GetContent());
    policy.SetSpecificationVersion(policyVersion);
}

void XmlPoliciesConverter::SetPolicySerialNumber(const qcc::XmlElement* xmlSerialNumber, PermissionPolicy& policy)
{
    uint32_t serialNumber = qcc::StringToU32(xmlSerialNumber->GetContent());
    policy.SetVersion(serialNumber);
}

void XmlPoliciesConverter::SetPolicyAcls(const qcc::XmlElement* acls, PermissionPolicy& policy)
{
    std::vector<PermissionPolicy::Acl> aclsVector;

    for (auto acl : acls->GetChildren()) {
        AddAcl(acl, aclsVector);
    }

    policy.SetAcls(aclsVector.size(), aclsVector.data());
}

void XmlPoliciesConverter::AddAcl(const qcc::XmlElement* aclXml, std::vector<PermissionPolicy::Acl>& aclsVector)
{
    PermissionPolicy::Acl acl;
    BuildAcl(aclXml, acl);
    aclsVector.push_back(acl);
}

void XmlPoliciesConverter::BuildAcl(const qcc::XmlElement* aclXml, PermissionPolicy::Acl& acl)
{
    SetAclPeers(aclXml->GetChildren()[PEERS_INDEX], acl);
    SetAclRules(aclXml->GetChildren()[RULES_INDEX], acl);
}

void XmlPoliciesConverter::SetAclPeers(const qcc::XmlElement* peersXml, PermissionPolicy::Acl& acl)
{
    std::vector<PermissionPolicy::Peer> peers;

    for (auto peer : peersXml->GetChildren()) {
        AddPeer(peer, peers);
    }

    acl.SetPeers(peers.size(), peers.data());
}

void XmlPoliciesConverter::SetAclRules(const qcc::XmlElement* rulesXml, PermissionPolicy::Acl& acl)
{
    std::vector<PermissionPolicy::Rule> rules;
    QCC_VERIFY(ER_OK == XmlRulesConverter::XmlToRules(rulesXml->Generate().c_str(), rules));
    acl.SetRules(rules.size(), rules.data());
}

void XmlPoliciesConverter::AddPeer(const qcc::XmlElement* peerXml, std::vector<PermissionPolicy::Peer>& peers)
{
    PermissionPolicy::Peer peer;
    BuildPeer(peerXml, peer);
    peers.push_back(peer);
}

void XmlPoliciesConverter::BuildPeer(const qcc::XmlElement* peerXml, PermissionPolicy::Peer& peer)
{
    SetPeerType(peerXml, peer);

    if (PeerContainsPublicKey(peerXml)) {
        SetPeerPublicKey(peerXml, peer);
    }

    if (PeerContainsSgId(peerXml)) {
        SetPeerSgId(peerXml, peer);
    }
}

void XmlPoliciesConverter::SetPeerType(const qcc::XmlElement* peerXml, PermissionPolicy::Peer& peer)
{
    AJ_PCSTR typeName = peerXml->GetChildren()[PEER_TYPE_INDEX]->GetContent().c_str();
    PermissionPolicy::Peer::PeerType peerType = XmlPoliciesValidator::s_peerTypeMap.find(typeName)->second;

    peer.SetType(peerType);
}

void XmlPoliciesConverter::SetPeerPublicKey(const qcc::XmlElement* peerXml, PermissionPolicy::Peer& peer)
{
    KeyInfoNISTP256 keyInfo;
    AJ_PCSTR publicKeyXmlValue = peerXml->GetChildren()[PEER_PUBLIC_KEY_INDEX]->GetContent().c_str();

    QCC_VERIFY(ER_OK == KeyInfoHelper::PEMToKeyInfoNISTP256(publicKeyXmlValue, keyInfo));

    peer.SetKeyInfo(&keyInfo);
}

void XmlPoliciesConverter::SetPeerSgId(const qcc::XmlElement* peerXml, PermissionPolicy::Peer& peer)
{
    AJ_PCSTR sgIdXmlValue = peerXml->GetChildren()[PEER_SGID_INDEX]->GetContent().c_str();
    peer.SetSecurityGroupId(GUID128(sgIdXmlValue));
}

bool XmlPoliciesConverter::PeerContainsPublicKey(const qcc::XmlElement* peerXml)
{
    return peerXml->GetChildren().size() > 1;
}

bool XmlPoliciesConverter::PeerContainsSgId(const qcc::XmlElement* peerXml)
{
    return peerXml->GetChildren().size() > 2;
}
} /* namespace ajn */
