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

#include <qcc/Debug.h>

#include <alljoyn/securitymgr/PolicyGenerator.h>

#define QCC_MODULE "SECMGR_AGENT"

using namespace ajn;
using namespace qcc;
using namespace ajn::securitymgr;

QStatus PolicyGenerator::DefaultPolicy(const vector<GroupInfo>& groupInfos,
                                       PermissionPolicy& policy) const
{
    vector<PermissionPolicy::Acl> acls;

    if (deniedKeys.size() > 0) {
        PermissionPolicy::Acl denyAcl;
        DenyAcl(deniedKeys, denyAcl);
        acls.push_back(denyAcl);
    }

    PermissionPolicy::Acl adminAcl;
    AdminAcl(adminAcl);
    acls.push_back(adminAcl);

    for (size_t i = 0; i < groupInfos.size(); i++) {
        PermissionPolicy::Acl defaultAcl;
        DefaultGroupPolicyAcl(groupInfos[i], defaultAcl);
        acls.push_back(defaultAcl);
    }

    if (acls.size() > 0) {
        policy.SetAcls(acls.size(), &acls[0]);
        return ER_OK;
    }

    return ER_FAIL;
}

void PolicyGenerator::DenyAcl(const vector<KeyInfoNISTP256>& keys,
                              PermissionPolicy::Acl& acl) const
{
    if (keys.size() < 1) {
        return;
    }

    vector<PermissionPolicy::Peer> peers;
    for (vector<KeyInfoNISTP256>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        PermissionPolicy::Peer peer;
        peer.SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
        peer.SetKeyInfo(&(*it));
        peers.push_back(peer);
    }

    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("*");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(0); // explicit deny

    PermissionPolicy::Rule rules[1];
    rules[0].SetInterfaceName("*");
    rules[0].SetObjPath("*");
    rules[0].SetMembers(1, members);

    acl.SetPeers(peers.size(), &peers[0]);
    acl.SetRules(1, rules);
}

void PolicyGenerator::AdminAcl(PermissionPolicy::Acl& acl) const
{
    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
    peers[0].SetSecurityGroupId(adminGroup.guid);
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(adminGroup.authority.GetPublicKey());
    peers[0].SetKeyInfo(&keyInfo);
    acl.SetPeers(1, peers);

    PermissionPolicy::Rule rules[1];
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member prms[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(
        PermissionPolicy::Rule::Member::ACTION_PROVIDE |
        PermissionPolicy::Rule::Member::ACTION_OBSERVE |
        PermissionPolicy::Rule::Member::ACTION_MODIFY
        );
    rules[0].SetMembers(1, prms);
    acl.SetRules(1, rules);
}

void PolicyGenerator::DefaultGroupPolicyAcl(const GroupInfo& group,
                                            PermissionPolicy::Acl& term) const
{
    PermissionPolicy::Rule rules[1];
    PermissionPolicy::Peer peers[1];
    KeyInfoNISTP256 keyInfo;

    DefaultGroupPolicyRule(rules[0]);

    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
    peers[0].SetSecurityGroupId(group.guid);

    keyInfo.SetPublicKey(group.authority.GetPublicKey());
    peers[0].SetKeyInfo(&keyInfo);
    term.SetPeers(1, peers);
    term.SetRules(1, rules);
}

void PolicyGenerator::DefaultGroupPolicyRule(PermissionPolicy::Rule& rule) const
{
    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("*");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                             PermissionPolicy::Rule::Member::ACTION_OBSERVE |
                             PermissionPolicy::Rule::Member::ACTION_MODIFY);

    rule.SetInterfaceName("*");
    rule.SetMembers(1, members);
}

#undef QCC_MODULE
