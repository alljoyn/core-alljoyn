/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include "alljoyn/securitymgr/PolicyGenerator.h"

using namespace ajn;
using namespace qcc;
using namespace securitymgr;

QStatus PolicyGenerator::DefaultPolicy(const std::vector<GuildInfo>& guildInfos,
                                       PermissionPolicy& policy)
{
    QStatus status = ER_OK;

    size_t numGuilds = guildInfos.size();

    // will be deleted by policy
    PermissionPolicy::Term* terms = new PermissionPolicy::Term[numGuilds];

    for (size_t i = 0; i < numGuilds; i++) {
        if (ER_OK !=
            (status = DefaultGuildPolicyTerm(guildInfos[i].guid.GetBytes(),
                                             GUID128::SIZE, guildInfos[i].authority, terms[i]))) {
            break;
        }
    }

    if (ER_OK == status) {
        policy.SetTerms(numGuilds, terms);
    } else {
        delete[] terms;
    }

    return status;
}

QStatus PolicyGenerator::DefaultGuildPolicyTerm(const uint8_t* guildId,
                                                const size_t guildIdLen,
                                                const ECCPublicKey& authority,
                                                PermissionPolicy::Term& term)
{
    QStatus status = ER_OK;

    // will be deleted by term
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];

    if (ER_OK != (status = DefaultGuildPolicyRule(rules[0]))) {
        delete[] rules;
        return status;
    }

    // will be deleted by term
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);

    // will be deleted by peer
    KeyInfoNISTP256* info = new KeyInfoNISTP256();
    info->SetKeyId(guildId, guildIdLen);
    info->SetPublicKey(&authority);
    peers[0].SetKeyInfo(info);
    term.SetPeers(1, peers);
    term.SetRules(1, rules);

    return status;
}

QStatus PolicyGenerator::DefaultGuildPolicyRule(PermissionPolicy::Rule& rule)
{
    // will be deleted by rule
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
