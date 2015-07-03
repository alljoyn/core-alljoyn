/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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
    QStatus status = ER_OK;

    size_t numGroups = groupInfos.size();

    PermissionPolicy::Acl* acls = nullptr;
    size_t numAcls = numGroups + 1;
    acls = new PermissionPolicy::Acl[numAcls];

    status = AdminAcl(acls[0]);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to set AdminAcl"));
        goto Exit;
    }

    for (size_t i = 0; i < numGroups; i++) {
        status = DefaultGroupPolicyAcl(groupInfos[i], acls[i + 1]);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to generate DefaultGroupPolicyAcl"));
            goto Exit;
        }
    }

    // Policy takes ownership of acls.
    policy.SetAcls(numAcls, acls);
    return status;

Exit:
    delete[] acls;
    return status;
}

QStatus PolicyGenerator::AdminAcl(PermissionPolicy::Acl& acl) const
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

    return ER_OK;
}

QStatus PolicyGenerator::DefaultGroupPolicyAcl(const GroupInfo& group,
                                               PermissionPolicy::Acl& term) const
{
    QStatus status = ER_OK;

    // Will be deleted by term.
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];

    if (ER_OK != (status = DefaultGroupPolicyRule(rules[0]))) {
        delete[] rules;
        return status;
    }

    // Will be deleted by term.
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
    peers[0].SetSecurityGroupId(group.guid);

    // Will be deleted by peer.
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(group.authority.GetPublicKey());
    peers[0].SetKeyInfo(&keyInfo);
    term.SetPeers(1, peers);
    term.SetRules(1, rules);

    return status;
}

QStatus PolicyGenerator::DefaultGroupPolicyRule(PermissionPolicy::Rule& rule) const
{
    // Will be deleted by rule.
    PermissionPolicy::Rule::Member* members = new PermissionPolicy::Rule::Member[1];
    members[0].SetMemberName("*");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                             PermissionPolicy::Rule::Member::ACTION_OBSERVE |
                             PermissionPolicy::Rule::Member::ACTION_MODIFY);

    rule.SetInterfaceName("*");
    rule.SetMembers(1, members);

    return ER_OK;
}

#undef QCC_MODULE
