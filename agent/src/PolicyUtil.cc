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

#include <alljoyn/securitymgr/PolicyUtil.h>

#define QCC_MODULE "SECMGR_AGENT"

using namespace ajn;
using namespace ajn::securitymgr;

struct MemberCompare {
    bool operator()(const PermissionPolicy::Rule::Member& lhs,
                    const PermissionPolicy::Rule::Member& rhs)
    {
        int comp = lhs.GetMemberName() < rhs.GetMemberName();

        return (comp != 0 ? comp : lhs.GetMemberType() < rhs.GetMemberType());
    }
};

struct RuleCompare {
    bool operator()(const PermissionPolicy::Rule& lhs,
                    const PermissionPolicy::Rule& rhs)
    {
        int comp = lhs.GetInterfaceName() < rhs.GetInterfaceName();

        return (comp != 0 ? comp : lhs.GetObjPath() < rhs.GetObjPath());
    }
};

typedef map<PermissionPolicy::Rule::Member, uint8_t, MemberCompare> MemberMap;
typedef map<PermissionPolicy::Rule, MemberMap, RuleCompare> RuleMap;

static void AddMembers(const PermissionPolicy::Rule& rule, MemberMap& mmap)
{
    for (size_t i = 0; i < rule.GetMembersSize(); i++) {
        PermissionPolicy::Rule::Member member = rule.GetMembers()[i];
        auto it = mmap.find(member);
        if (it != mmap.end()) {
            it->second |= member.GetActionMask();
        } else {
            mmap[member] = member.GetActionMask();
        }
    }
}

static void AddRules(const PermissionPolicy::Acl acl, RuleMap& rmap)
{
    for (size_t i = 0; i < acl.GetRulesSize(); i++) {
        PermissionPolicy::Rule rule = acl.GetRules()[i];
        auto it = rmap.find(rule);
        if (it != rmap.end()) {
            AddMembers(rule, it->second);
        } else {
            MemberMap mmap;
            AddMembers(rule, mmap);
            rmap[rule] = mmap;
        }
    }
}

static void SetRules(const RuleMap& rmap, PermissionPolicy::Acl& acl)
{
    vector<PermissionPolicy::Rule> rules;

    for (RuleMap::const_iterator rit = rmap.begin(); rit != rmap.end(); rit++) {
        vector<PermissionPolicy::Rule::Member> members;
        for (MemberMap::const_iterator mit = rit->second.begin(); mit != rit->second.end(); mit++) {
            PermissionPolicy::Rule::Member member = mit->first;
            member.SetActionMask(mit->second);
            members.push_back(member);
        }
        PermissionPolicy::Rule rule = rit->first;
        rule.SetMembers(members.size(), &members[0]);
        rules.push_back(rule);
    }

    acl.SetRules(rules.size(), &rules[0]);
}

void PolicyUtil::NormalizePolicy(PermissionPolicy& policy)
{
    vector<PermissionPolicy::Acl> acls;

    for (size_t i = 0; i < policy.GetAclsSize(); i++) {
        PermissionPolicy::Acl acl = policy.GetAcls()[i];
        RuleMap rmap;
        AddRules(acl, rmap);
        SetRules(rmap, acl);
        acls.push_back(acl);
    }

    policy.SetAcls(acls.size(), &acls[0]);
}
