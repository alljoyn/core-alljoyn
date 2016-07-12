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

#include <gtest/gtest.h>
#include <qcc/GUID.h>
#include "KeyInfoHelper.h"
#include "PermissionPolicyOverwriteUtils.h"

using namespace ajn;
using namespace qcc;

void PolicyOverwriteUtils::ChangeMemberName(PermissionPolicy::Rule& rule, size_t memberIndex, AJ_PCSTR newName)
{
    PermissionPolicy::Rule::Member* mutableMembers = nullptr;
    GetMembersCopy(rule, &mutableMembers);

    mutableMembers[memberIndex].SetMemberName(newName);
    rule.SetMembers(rule.GetMembersSize(), mutableMembers);

    delete[] mutableMembers;
}

void PolicyOverwriteUtils::ChangeMemberActionMask(PermissionPolicy::Rule& rule, size_t memberIndex, uint8_t newActionMask)
{
    PermissionPolicy::Rule::Member* mutableMembers = nullptr;
    GetMembersCopy(rule, &mutableMembers);

    mutableMembers[memberIndex].SetActionMask(newActionMask);
    rule.SetMembers(rule.GetMembersSize(), mutableMembers);

    delete[] mutableMembers;
}

void PolicyOverwriteUtils::ChangeRecommendedSecurityLevel(PermissionPolicy::Rule::SecurityLevel securityLevel, PermissionPolicy& policy)
{
    PermissionPolicy::Rule* mutableRules = nullptr;

    GetRulesCopy(policy.GetAcls()[0], &mutableRules);
    mutableRules[0].SetRecommendedSecurityLevel(securityLevel);
    ChangeRules(policy.GetAcls()[0].GetRulesSize(), mutableRules, policy);

    delete[] mutableRules;
}

void PolicyOverwriteUtils::ChangeRules(size_t rulesCount, PermissionPolicy::Rule* rules, PermissionPolicy& policy)
{
    PermissionPolicy::Acl* mutableAcls = nullptr;
    GetAclsCopy(policy, &mutableAcls);
    mutableAcls[0].SetRules(rulesCount, rules);

    policy.SetAcls(1U, mutableAcls);
}

void PolicyOverwriteUtils::ChangePeers(size_t peersCount, PermissionPolicy::Peer* peers, PermissionPolicy& policy)
{
    PermissionPolicy::Acl* mutableAcls = nullptr;
    GetAclsCopy(policy, &mutableAcls);
    mutableAcls[0].SetPeers(peersCount, peers);

    policy.SetAcls(1U, mutableAcls);
}

void PolicyOverwriteUtils::ChangePeerType(size_t m_peerIndex, PermissionPolicy::Peer::PeerType peerType, PermissionPolicy& policy)
{
    PermissionPolicy::Acl* mutableAcls = nullptr;
    PermissionPolicy::Peer* mutablePeers = nullptr;

    GetAclsCopy(policy, &mutableAcls);
    GetPeersCopy(mutableAcls[0], &mutablePeers);

    mutablePeers[m_peerIndex].SetType(peerType);
    mutableAcls[0].SetPeers(mutableAcls[0].GetPeersSize(), mutablePeers);

    policy.SetAcls(1, mutableAcls);

    delete[] mutableAcls;
    delete[] mutablePeers;
}

void PolicyOverwriteUtils::ChangePeerPublicKey(size_t m_peerIndex, AJ_PCSTR publicKeyPem, PermissionPolicy& policy)
{
    PermissionPolicy::Acl* mutableAcls = nullptr;
    PermissionPolicy::Peer* mutablePeers = nullptr;

    GetAclsCopy(policy, &mutableAcls);
    GetPeersCopy(mutableAcls[0], &mutablePeers);

    SetPeerPublicKey(publicKeyPem, mutablePeers[m_peerIndex]);
    mutableAcls[0].SetPeers(mutableAcls[0].GetPeersSize(), mutablePeers);

    policy.SetAcls(1, mutableAcls);

    delete[] mutableAcls;
    delete[] mutablePeers;
}

void PolicyOverwriteUtils::ChangePeerSgId(size_t m_peerIndex, AJ_PCSTR sgIdHex, PermissionPolicy& policy)
{
    PermissionPolicy::Acl* mutableAcls = nullptr;
    PermissionPolicy::Peer* mutablePeers = nullptr;

    GetAclsCopy(policy, &mutableAcls);
    GetPeersCopy(mutableAcls[0], &mutablePeers);

    mutablePeers[m_peerIndex].SetSecurityGroupId(GUID128(sgIdHex));
    mutableAcls[0].SetPeers(mutableAcls[0].GetPeersSize(), mutablePeers);

    policy.SetAcls(1, mutableAcls);

    delete[] mutableAcls;
    delete[] mutablePeers;
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

void PolicyOverwriteUtils::GetMembersCopy(const PermissionPolicy::Rule& rule, PermissionPolicy::Rule::Member** mutableMembers)
{
    size_t membersSize = rule.GetMembersSize();
    *mutableMembers = new PermissionPolicy::Rule::Member[membersSize];

    for (size_t index = 0; index < membersSize; index++) {
        (*mutableMembers)[index] = rule.GetMembers()[index];
    }
}

void PolicyOverwriteUtils::GetAclsCopy(const PermissionPolicy& policy, PermissionPolicy::Acl** mutableAcls)
{
    size_t aclsSize = policy.GetAclsSize();
    *mutableAcls = new PermissionPolicy::Acl[aclsSize];

    for (size_t index = 0; index < aclsSize; index++) {
        (*mutableAcls)[index] = policy.GetAcls()[index];
    }
}

void ajn::PolicyOverwriteUtils::GetRulesCopy(const PermissionPolicy::Acl& acl, PermissionPolicy::Rule** mutableRules)
{
    size_t rulesSize = acl.GetPeersSize();
    *mutableRules = new PermissionPolicy::Rule[rulesSize];

    for (size_t index = 0; index < rulesSize; index++) {
        (*mutableRules)[index] = acl.GetRules()[index];
    }
}

void PolicyOverwriteUtils::GetPeersCopy(const PermissionPolicy::Acl& acl, PermissionPolicy::Peer** mutablePeers)
{
    size_t peersSize = acl.GetPeersSize();
    *mutablePeers = new PermissionPolicy::Peer[peersSize];

    for (size_t index = 0; index < peersSize; index++) {
        (*mutablePeers)[index] = acl.GetPeers()[index];
    }
}

void PolicyOverwriteUtils::SetPeerPublicKey(AJ_PCSTR publicKeyPem, PermissionPolicy::Peer& peer)
{
    KeyInfoNISTP256* publicKey = nullptr;

    if (nullptr != publicKeyPem) {
        publicKey = new KeyInfoNISTP256();
        ASSERT_EQ(ER_OK, KeyInfoHelper::PEMToKeyInfoNISTP256(publicKeyPem, *publicKey));
    }

    peer.SetKeyInfo(publicKey);
}
