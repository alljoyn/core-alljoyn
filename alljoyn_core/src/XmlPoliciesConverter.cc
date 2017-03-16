/**
 * @file
 * This file defines the XmlPoliciesConverter for reading Security 2.0
 * policies from an XML.
 */

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

#include <string>
#include <qcc/StringUtil.h>
#include "KeyInfoHelper.h"
#include "XmlPoliciesConverter.h"
#include "XmlRulesConverter.h"

#define QCC_MODULE "XML_CONVERTER"

using namespace qcc;
using namespace std;

namespace ajn {

map<PermissionPolicy::Peer::PeerType, string>* XmlPoliciesConverter::s_inversePeerTypeMap = nullptr;

void XmlPoliciesConverter::Init()
{
    QCC_DbgTrace(("%s: Performing converter init.", __FUNCTION__));

    QCC_ASSERT(nullptr == s_inversePeerTypeMap);

    s_inversePeerTypeMap = new map<PermissionPolicy::Peer::PeerType, string>();

    (*s_inversePeerTypeMap)[PermissionPolicy::Peer::PeerType::PEER_ALL] = XML_PEER_ALL;
    (*s_inversePeerTypeMap)[PermissionPolicy::Peer::PeerType::PEER_ANY_TRUSTED] = XML_PEER_ANY_TRUSTED;
    (*s_inversePeerTypeMap)[PermissionPolicy::Peer::PeerType::PEER_FROM_CERTIFICATE_AUTHORITY] = XML_PEER_FROM_CERTIFICATE_AUTHORITY;
    (*s_inversePeerTypeMap)[PermissionPolicy::Peer::PeerType::PEER_WITH_MEMBERSHIP] = XML_PEER_WITH_MEMBERSHIP;
    (*s_inversePeerTypeMap)[PermissionPolicy::Peer::PeerType::PEER_WITH_PUBLIC_KEY] = XML_PEER_WITH_PUBLIC_KEY;
}

void XmlPoliciesConverter::Shutdown()
{
    QCC_DbgTrace(("%s: Performing converter cleanup.", __FUNCTION__));

    delete s_inversePeerTypeMap;
    s_inversePeerTypeMap = nullptr;
}

QStatus XmlPoliciesConverter::FromXml(AJ_PCSTR policyXml, ajn::PermissionPolicy& policy)
{
    QCC_ASSERT(nullptr != policyXml);
    QCC_ASSERT(nullptr != &policy);

    XmlElement* root = nullptr;
    QStatus status = XmlElement::GetRoot(policyXml, &root);

    if (ER_OK == status) {
        status = XmlPoliciesValidator::Validate(root);
    }

    if (ER_OK == status) {
        BuildPolicy(root, policy);
    }

    delete root;
    return status;
}

QStatus XmlPoliciesConverter::ToXml(const ajn::PermissionPolicy& policy, string& policyXml)
{
    QStatus status = XmlPoliciesValidator::Validate(policy);

    if (ER_OK == status) {
        XmlElement policyXmlElement(POLICY_XML_ELEMENT);

        BuildPolicy(policy, &policyXmlElement);
        policyXml = policyXmlElement.Generate().c_str();
    }

    return status;
}

void XmlPoliciesConverter::BuildPolicy(const XmlElement* root, ajn::PermissionPolicy& policy)
{
    SetPolicyVersion(root->GetChildren()[POLICY_VERSION_INDEX], policy);
    SetPolicySerialNumber(root->GetChildren()[SERIAL_NUMBER_INDEX], policy);
    SetPolicyAcls(root->GetChildren()[ACLS_INDEX], policy);
}

void XmlPoliciesConverter::SetPolicyVersion(const XmlElement* xmlPolicyVersion, PermissionPolicy& policy)
{
    uint32_t policyVersion = StringToU32(xmlPolicyVersion->GetContent());
    policy.SetSpecificationVersion(policyVersion);
}

void XmlPoliciesConverter::SetPolicySerialNumber(const XmlElement* xmlSerialNumber, PermissionPolicy& policy)
{
    uint32_t serialNumber = StringToU32(xmlSerialNumber->GetContent());
    policy.SetVersion(serialNumber);
}

void XmlPoliciesConverter::SetPolicyAcls(const XmlElement* acls, PermissionPolicy& policy)
{
    vector<PermissionPolicy::Acl> aclsVector;

    for (auto acl : acls->GetChildren()) {
        AddAcl(acl, aclsVector);
    }

    policy.SetAcls(aclsVector.size(), aclsVector.data());
}

void XmlPoliciesConverter::AddAcl(const XmlElement* aclXml, vector<PermissionPolicy::Acl>& aclsVector)
{
    PermissionPolicy::Acl acl;
    BuildAcl(aclXml, acl);
    aclsVector.push_back(acl);
}

void XmlPoliciesConverter::BuildAcl(const XmlElement* aclXml, PermissionPolicy::Acl& acl)
{
    SetAclPeers(aclXml->GetChildren()[PEERS_INDEX], acl);
    if (aclXml->GetChildren().size() == ACL_ELEMENT_WITH_RULES_CHILDREN_COUNT) {
        SetAclRules(aclXml->GetChildren()[RULES_INDEX], acl);
    }
}

void XmlPoliciesConverter::SetAclPeers(const XmlElement* peersXml, PermissionPolicy::Acl& acl)
{
    vector<PermissionPolicy::Peer> peers;

    for (auto peer : peersXml->GetChildren()) {
        AddPeer(peer, peers);
    }

    acl.SetPeers(peers.size(), peers.data());
}

void XmlPoliciesConverter::SetAclRules(const XmlElement* rulesXml, PermissionPolicy::Acl& acl)
{
    vector<PermissionPolicy::Rule> rules;
    QCC_VERIFY(ER_OK == XmlRulesConverter::GetInstance()->XmlToRules(rulesXml->Generate().c_str(), rules));
    acl.SetRules(rules.size(), rules.data());
}

void XmlPoliciesConverter::AddPeer(const XmlElement* peerXml, vector<PermissionPolicy::Peer>& peers)
{
    PermissionPolicy::Peer peer;
    BuildPeer(peerXml, peer);
    peers.push_back(peer);
}

void XmlPoliciesConverter::BuildPeer(const XmlElement* peerXml, PermissionPolicy::Peer& peer)
{
    SetPeerType(peerXml, peer);

    if (PeerContainsPublicKey(peerXml)) {
        SetPeerPublicKey(peerXml, peer);
    }

    if (PeerContainsSgId(peerXml)) {
        SetPeerSgId(peerXml, peer);
    }
}

void XmlPoliciesConverter::SetPeerType(const XmlElement* peerXml, PermissionPolicy::Peer& peer)
{
    AJ_PCSTR typeName = peerXml->GetChildren()[PEER_TYPE_INDEX]->GetContent().c_str();
    PermissionPolicy::Peer::PeerType peerType = XmlPoliciesValidator::s_peerTypeMap->at(typeName);

    peer.SetType(peerType);
}

void XmlPoliciesConverter::SetPeerPublicKey(const XmlElement* peerXml, PermissionPolicy::Peer& peer)
{
    KeyInfoNISTP256 keyInfo;
    AJ_PCSTR publicKeyXmlValue = peerXml->GetChildren()[PEER_PUBLIC_KEY_INDEX]->GetContent().c_str();

    QCC_VERIFY(ER_OK == KeyInfoHelper::PEMToKeyInfoNISTP256(publicKeyXmlValue, keyInfo));

    peer.SetKeyInfo(&keyInfo);
}

void XmlPoliciesConverter::SetPeerSgId(const XmlElement* peerXml, PermissionPolicy::Peer& peer)
{
    AJ_PCSTR sgIdXmlValue = peerXml->GetChildren()[PEER_SGID_INDEX]->GetContent().c_str();
    peer.SetSecurityGroupId(GUID128(sgIdXmlValue));
}

bool XmlPoliciesConverter::PeerContainsPublicKey(const XmlElement* peerXml)
{
    return peerXml->GetChildren().size() > 1;
}

bool XmlPoliciesConverter::PeerContainsSgId(const XmlElement* peerXml)
{
    return peerXml->GetChildren().size() > 2;
}

void XmlPoliciesConverter::BuildPolicy(const PermissionPolicy& policy, XmlElement* policyXmlElement)
{
    SetPolicyVersion(policy, policyXmlElement);
    SetPolicySerialNumber(policy, policyXmlElement);
    SetPolicyAcls(policy, policyXmlElement);
}

void XmlPoliciesConverter::SetPolicyVersion(const PermissionPolicy& policy, XmlElement* policyXmlElement)
{
    policyXmlElement->CreateChild(POLICY_VERSION_XML_ELEMENT)->SetContent(U32ToString(policy.GetSpecificationVersion()));
}

void XmlPoliciesConverter::SetPolicySerialNumber(const PermissionPolicy& policy, XmlElement* policyXmlElement)
{
    policyXmlElement->CreateChild(SERIAL_NUMBER_XML_ELEMENT)->SetContent(U32ToString(policy.GetVersion()));
}

void XmlPoliciesConverter::SetPolicyAcls(const PermissionPolicy& policy, XmlElement* policyXmlElement)
{
    const PermissionPolicy::Acl* acls = policy.GetAcls();
    XmlElement* aclsXml = policyXmlElement->CreateChild(ACLS_XML_ELEMENT);

    for (size_t index = 0; index < policy.GetAclsSize(); index++) {
        AddAcl(acls[index], aclsXml);
    }
}

void XmlPoliciesConverter::AddAcl(const PermissionPolicy::Acl& acl, XmlElement* aclsXml)
{
    XmlElement* aclXml = aclsXml->CreateChild(ACL_XML_ELEMENT);

    SetAclPeers(acl.GetPeers(), acl.GetPeersSize(), aclXml);
    if (acl.GetRulesSize() > 0) {
        SetAclRules(acl.GetRules(), acl.GetRulesSize(), aclXml);
    }
}

void XmlPoliciesConverter::SetAclPeers(const PermissionPolicy::Peer* peers, size_t peersSize, XmlElement* aclXml)
{
    XmlElement* peersXml = aclXml->CreateChild(PEERS_XML_ELEMENT);

    for (size_t index = 0; index < peersSize; index++) {
        AddPeer(peers[index], peersXml);
    }
}

void XmlPoliciesConverter::AddPeer(const PermissionPolicy::Peer& peer, XmlElement* peersXml)
{
    XmlElement* peerXml = peersXml->CreateChild(PEER_XML_ELEMENT);

    SetPeerType(peer, peerXml);

    if (nullptr != peer.GetKeyInfo()) {
        SetPeerPublicKey(peer, peerXml);
    }

    if (PermissionPolicy::Peer::PeerType::PEER_WITH_MEMBERSHIP == peer.GetType()) {
        SetPeerSgId(peer, peerXml);
    }
}

void XmlPoliciesConverter::SetPeerType(const PermissionPolicy::Peer& peer, XmlElement* peerXml)
{
    peerXml->CreateChild(TYPE_XML_ELEMENT)->SetContent(s_inversePeerTypeMap->at(peer.GetType()));
}

void XmlPoliciesConverter::SetPeerPublicKey(const PermissionPolicy::Peer& peer, XmlElement* peerXml)
{
    String publicKeyPEM;
    XmlElement* publicKeyXml = peerXml->CreateChild(PUBLIC_KEY_XML_ELEMENT);

    QCC_VERIFY(ER_OK == CertificateX509::EncodePublicKeyPEM(peer.GetKeyInfo()->GetPublicKey(), publicKeyPEM));
    publicKeyXml->SetContent(publicKeyPEM);
}

void XmlPoliciesConverter::SetPeerSgId(const PermissionPolicy::Peer& peer, XmlElement* peerXml)
{
    peerXml->CreateChild(SGID_KEY_XML_ELEMENT)->AddContent(peer.GetSecurityGroupId().ToString());
}

void XmlPoliciesConverter::SetAclRules(const PermissionPolicy::Rule* rules, size_t rulesSize, XmlElement* aclXml)
{
    XmlElement* rulesXml = nullptr;

    QCC_VERIFY(ER_OK == XmlRulesConverter::GetInstance()->RulesToXml(rules,
                                                                     rulesSize,
                                                                     &rulesXml));
    aclXml->AddChild(rulesXml);
}
} /* namespace ajn */