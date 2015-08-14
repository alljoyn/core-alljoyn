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

#include <gtest/gtest.h>

#include <alljoyn/securitymgr/PolicyUtil.h>

#include "TestUtil.h"

using namespace ajn;
using namespace securitymgr;

/** @file PolicyUtilTests.cc */

namespace secmgr_tests {
class PolicyUtilTest :
    public BasicTest {
};

/**
 * @test Normalize a rule with matching members, and see whether the resulting action mask
 *       corresponds to the OR value of the ActionMasks of all individual members.
 *       -# Create a rule with three matching members with different action masks: one with
 *          ACTION_PROVIDE, one with ACTION_OBSERVE and one with ACTION_MODIFY.
 *       -# Add this rule to a policy.
 *       -# Normalize this policy.
 *       -# Verify that the normalized policy has only one member.
 *       -# Verify that this member has a full ActionMask.
 **/
TEST_F(PolicyUtilTest, NormalizePolicyMembers) {
    PermissionPolicy::Rule::Member members[3];
    members[0].SetMemberName("foo");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    members[1].SetMemberName("foo");
    members[1].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    members[2].SetMemberName("foo");
    members[2].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);

    PermissionPolicy::Rule rules[1];
    rules[0].SetMembers(3, members);

    PermissionPolicy::Acl acls[1];
    acls[0].SetRules(1, rules);
    PermissionPolicy pol;
    pol.SetAcls(1, acls);

    ASSERT_EQ((size_t)3, pol.GetAcls()[0].GetRules()[0].GetMembersSize());
    PolicyUtil::NormalizePolicy(pol);
    ASSERT_EQ((size_t)1, pol.GetAcls()[0].GetRules()[0].GetMembersSize());
    ASSERT_EQ(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
              PermissionPolicy::Rule::Member::ACTION_OBSERVE |
              PermissionPolicy::Rule::Member::ACTION_MODIFY,
              pol.GetAcls()[0].GetRules()[0].GetMembers()[0].GetActionMask());
}

/**
 * @test Normalize a policy with matching rules and members, and see whether the resulting action
 *       mask corresponds to the OR of the ActionMasks of all members.
 *       -# Create a rule with two matching members with different action masks: one with
 *          ACTION_OBSERVE and one with ACTION_MODIFY.
 *       -# Create another rule that matches the previous members, but with a different action mask
 *          ACTION_PROVIDE.
 *       -# Add those rules to a policy.
 *       -# Normalize this policy.
 *       -# Verify that the normalized policy has only one rule.
 *       -# Verify that the member of this rule has a full ActionMask.
 **/
TEST_F(PolicyUtilTest, NormalizePolicyRules) {
    PermissionPolicy::Rule::Member members[2];
    members[0].SetMemberName("foo");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    members[1].SetMemberName("foo");
    members[1].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    PermissionPolicy::Rule::Member barmembers[1];
    barmembers[0].SetMemberName("foo");
    barmembers[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    barmembers[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);

    PermissionPolicy::Rule rules[2];
    rules[0].SetInterfaceName("bar");
    rules[0].SetMembers(2, members);
    rules[1].SetInterfaceName("bar");
    rules[1].SetMembers(1, barmembers);

    PermissionPolicy::Acl acls[1];
    acls[0].SetRules(2, rules);
    PermissionPolicy pol;
    pol.SetAcls(1, acls);

    ASSERT_EQ((size_t)2, pol.GetAcls()[0].GetRulesSize());
    PolicyUtil::NormalizePolicy(pol);
    ASSERT_EQ((size_t)1, pol.GetAcls()[0].GetRulesSize());
    ASSERT_EQ(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
              PermissionPolicy::Rule::Member::ACTION_OBSERVE |
              PermissionPolicy::Rule::Member::ACTION_MODIFY,
              pol.GetAcls()[0].GetRules()[0].GetMembers()[0].GetActionMask());
}

/**
 * @test Normalize a policy with partially matching rules, and check whether the resulting policy
 *       matches the expected outcome.
 *       -# Create a rule with a specific InterfaceName, and one Members with a specific MemberName,
 *          and with the ActionMask set to ACTION_OBSERVE.
 *       -# Create another rule with the same InterfaceName, and two Members. One member with a
 *          matching name to the previous rule, and one with a different name, each having the
 *          ActionMask set to ACTION_MODIFY.
 *       -# Create a third rule with a different InterfaceName but with a member of a matching
 *          MemberName. The ActionMask should be set to ACTION_PROVIDE.
 *        -# Add those rules to a policy.
 *        -# Normalize the policy.
 *       -# Verify that the normalized policy has two rules (one for each InterfaceName).
 *       -# Verify that the rule with the matching InterfaceName contains two members and verify
 *          that the ActionMasks are collapsed successfully.
 *       -# Verify that the rule with the unique InterfaceName has only one member and verify
 *          the resulting ActionMask.
 **/
TEST_F(PolicyUtilTest, NormalizePolicy) {
    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("foo");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    PermissionPolicy::Rule::Member barmembers[2];
    barmembers[0].SetMemberName("zoo");
    barmembers[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    barmembers[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    barmembers[1].SetMemberName("foo");
    barmembers[1].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    barmembers[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);

    PermissionPolicy::Rule::Member bazmembers[1];
    bazmembers[0].SetMemberName("foo");
    bazmembers[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    bazmembers[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);

    PermissionPolicy::Rule rules[3];
    rules[0].SetInterfaceName("bar");
    rules[0].SetMembers(1, members);
    rules[1].SetInterfaceName("bar");
    rules[1].SetMembers(2, barmembers);
    rules[2].SetInterfaceName("baz");
    rules[2].SetMembers(1, bazmembers);

    PermissionPolicy::Acl acls[1];
    acls[0].SetRules(3, rules);
    PermissionPolicy pol;
    pol.SetAcls(1, acls);

    ASSERT_EQ((size_t)3, pol.GetAcls()[0].GetRulesSize());
    PolicyUtil::NormalizePolicy(pol);
    ASSERT_EQ((size_t)2, pol.GetAcls()[0].GetRulesSize());
    ASSERT_EQ(PermissionPolicy::Rule::Member::ACTION_OBSERVE |
              PermissionPolicy::Rule::Member::ACTION_MODIFY,
              pol.GetAcls()[0].GetRules()[0].GetMembers()[0].GetActionMask()); // bar.foo
    ASSERT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY | 0, // avoid link issue
              pol.GetAcls()[0].GetRules()[0].GetMembers()[1].GetActionMask()); // bar.zoo
    ASSERT_EQ(PermissionPolicy::Rule::Member::ACTION_PROVIDE | 0, // avoid link issue
              pol.GetAcls()[0].GetRules()[1].GetMembers()[0].GetActionMask()); // baz.foo
}
}
