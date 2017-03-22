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

#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include <qcc/GUID.h>
#include "KeyInfoHelper.h"
#include "PermissionPolicyOverwriteUtils.h"

using namespace ajn;
using namespace qcc;
using namespace std;

void PolicyOverwriteUtils::ChangeMemberName(PermissionPolicy::Rule& rule, size_t memberIndex, AJ_PCSTR newName)
{
    vector<PermissionPolicy::Rule::Member> mutableMembers(rule.GetMembers(), rule.GetMembers() + rule.GetMembersSize());

    mutableMembers[memberIndex].SetMemberName(newName);

    rule.SetMembers(mutableMembers.size(), mutableMembers.data());
}

void PolicyOverwriteUtils::ChangeMemberType(PermissionPolicy::Rule& rule, size_t memberIndex, PermissionPolicy::Rule::Member::MemberType newType)
{
    vector<PermissionPolicy::Rule::Member> mutableMembers(rule.GetMembers(), rule.GetMembers() + rule.GetMembersSize());

    mutableMembers[memberIndex].SetMemberType(newType);

    rule.SetMembers(mutableMembers.size(), mutableMembers.data());
}

void PolicyOverwriteUtils::ChangeMemberActionMask(PermissionPolicy::Rule& rule, size_t memberIndex, uint8_t newActionMask)
{
    vector<PermissionPolicy::Rule::Member> mutableMembers(rule.GetMembers(), rule.GetMembers() + rule.GetMembersSize());

    mutableMembers[memberIndex].SetActionMask(newActionMask);

    rule.SetMembers(mutableMembers.size(), mutableMembers.data());
}

void PolicyOverwriteUtils::ChangeRecommendedSecurityLevel(PermissionPolicy::Rule::SecurityLevel securityLevel, PermissionPolicy& policy)
{
    const PermissionPolicy::Acl& acl = policy.GetAcls()[0];
    vector<PermissionPolicy::Rule> mutableRules(acl.GetRules(), acl.GetRules() + acl.GetRulesSize());

    mutableRules[0].SetRecommendedSecurityLevel(securityLevel);

    ChangeRules(mutableRules.size(), mutableRules.data(), policy);
}

void PolicyOverwriteUtils::ChangeRules(size_t rulesCount, PermissionPolicy::Rule* rules, PermissionPolicy& policy)
{
    vector<PermissionPolicy::Acl> mutableAcls(policy.GetAcls(), policy.GetAcls() + policy.GetAclsSize());

    mutableAcls[0].SetRules(rulesCount, rules);

    policy.SetAcls(mutableAcls.size(), mutableAcls.data());
}

void PolicyOverwriteUtils::ChangePeers(size_t peersCount, PermissionPolicy::Peer* peers, PermissionPolicy& policy)
{
    vector<PermissionPolicy::Acl> mutableAcls(policy.GetAcls(), policy.GetAcls() + policy.GetAclsSize());

    mutableAcls[0].SetPeers(peersCount, peers);

    policy.SetAcls(mutableAcls.size(), mutableAcls.data());
}

void PolicyOverwriteUtils::ChangePeerType(size_t m_peerIndex, PermissionPolicy::Peer::PeerType peerType, PermissionPolicy& policy)
{
    vector<PermissionPolicy::Acl> mutableAcls(policy.GetAcls(), policy.GetAcls() + policy.GetAclsSize());
    vector<PermissionPolicy::Peer> mutablePeers(mutableAcls[0].GetPeers(), mutableAcls[0].GetPeers() + mutableAcls[0].GetPeersSize());

    mutablePeers[m_peerIndex].SetType(peerType);
    mutableAcls[0].SetPeers(mutablePeers.size(), mutablePeers.data());

    policy.SetAcls(mutableAcls.size(), mutableAcls.data());
}

void PolicyOverwriteUtils::ChangePeerPublicKey(size_t m_peerIndex, AJ_PCSTR publicKeyPem, PermissionPolicy& policy)
{
    vector<PermissionPolicy::Acl> mutableAcls(policy.GetAcls(), policy.GetAcls() + policy.GetAclsSize());
    const PermissionPolicy::Acl& acl = mutableAcls[0];
    vector<PermissionPolicy::Peer> mutablePeers(acl.GetPeers(), acl.GetPeers() + acl.GetPeersSize());

    SetPeerPublicKey(publicKeyPem, mutablePeers[m_peerIndex]);
    mutableAcls[0].SetPeers(mutablePeers.size(), mutablePeers.data());

    policy.SetAcls(mutableAcls.size(), mutableAcls.data());
}

void PolicyOverwriteUtils::ChangePeerSgId(size_t m_peerIndex, AJ_PCSTR sgIdHex, PermissionPolicy& policy)
{
    vector<PermissionPolicy::Acl> mutableAcls(policy.GetAcls(), policy.GetAcls() + policy.GetAclsSize());
    const PermissionPolicy::Acl& acl = mutableAcls[0];
    vector<PermissionPolicy::Peer> mutablePeers(acl.GetPeers(), acl.GetPeers() + acl.GetPeersSize());

    mutablePeers[m_peerIndex].SetSecurityGroupId(GUID128(sgIdHex));
    mutableAcls[0].SetPeers(mutablePeers.size(), mutablePeers.data());

    policy.SetAcls(mutableAcls.size(), mutableAcls.data());
}

PermissionPolicy::Peer PolicyOverwriteUtils::BuildPeer(PermissionPolicy::Peer::PeerType peerType, AJ_PCSTR publicKeyPem, AJ_PCSTR sgIdHex)
{
    PermissionPolicy::Peer result;

    result.SetType(peerType);
    SetPeerPublicKey(publicKeyPem, result);

    if (nullptr != sgIdHex) {
        result.SetSecurityGroupId(GUID128(sgIdHex));
    }

    return result;
}

void PolicyOverwriteUtils::SetPeerPublicKey(AJ_PCSTR publicKeyPem, PermissionPolicy::Peer& peer)
{
    unique_ptr<KeyInfoNISTP256> publicKey(nullptr);

    if (nullptr != publicKeyPem) {
        publicKey.reset(new KeyInfoNISTP256());
        ASSERT_EQ(ER_OK, KeyInfoHelper::PEMToKeyInfoNISTP256(publicKeyPem, *publicKey));
    }

    peer.SetKeyInfo(publicKey.get());
}
