/**
 * @file
 * This program tests the PermissionPolicy for Alljoyn security 2.0 .It uses
 * google test as the test automation framework.
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
/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "ajTestCommon.h"

#include <PermissionPolicyUtil.h>

using namespace ajn;

TEST(PermissionPolicyUtilTest, validate_deny_rules)
{
    PermissionPolicy::Peer peers[2];

    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);
    peers[0].SetKeyInfo(&keyInfoECC);
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);

    qcc::KeyInfoNISTP256 otherKeyInfoECC;
    uint8_t otherDummyKeyId[10] = { 0, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    keyInfoECC.SetKeyId(otherDummyKeyId, 10);
    peers[1].SetKeyInfo(&otherKeyInfoECC);
    peers[1].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);

    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("*");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(0); // explicit deny

    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("*");
    rules[0].SetMembers(1, members);

    PermissionPolicy::Acl acls[1];
    acls[0].SetPeers(2, peers);
    acls[0].SetRules(1, rules);

    PermissionPolicy permissionPolicy;
    permissionPolicy.SetAcls(1, acls);

    ASSERT_TRUE(PermissionPolicyUtil::HasValidDenyRules(permissionPolicy));
}

TEST(PermissionPolicyUtilTest, validate_deny_rules_invalid_peertype)
{
    PermissionPolicy::Peer peers[2];

    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);
    peers[0].SetKeyInfo(&keyInfoECC);
    peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);

    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("*");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(0); // explicit deny

    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("*");
    rules[0].SetMembers(1, members);

    PermissionPolicy::Acl acls[1];
    acls[0].SetPeers(1, peers);
    acls[0].SetRules(1, rules);

    PermissionPolicy permissionPolicy;
    permissionPolicy.SetAcls(1, acls);

    ASSERT_FALSE(PermissionPolicyUtil::HasValidDenyRules(permissionPolicy));
}

TEST(PermissionPolicyUtilTest, validate_deny_rules_multiple_peers_same_keyinfo)
{
    PermissionPolicy::Peer peers[2];

    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);
    peers[0].SetKeyInfo(&keyInfoECC);
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
    peers[1].SetKeyInfo(&keyInfoECC);
    peers[1].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);

    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("*");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(0); // explicit deny

    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("*");
    rules[0].SetMembers(1, members);

    PermissionPolicy::Acl acls[1];
    acls[0].SetPeers(2, peers);
    acls[0].SetRules(1, rules);

    PermissionPolicy permissionPolicy;
    permissionPolicy.SetAcls(1, acls);

    ASSERT_TRUE(PermissionPolicyUtil::HasValidDenyRules(permissionPolicy));
}

TEST(PermissionPolicyUtilTest, validate_deny_rules_multiple_members)
{
    PermissionPolicy::Peer peers[1];

    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);
    peers[0].SetKeyInfo(&keyInfoECC);
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);

    PermissionPolicy::Rule::Member members[2];
    members[0].SetMemberName("*");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(0); // explicit deny
    members[1].SetMemberName("foo");
    members[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    members[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);

    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("*");
    rules[0].SetMembers(2, members);

    PermissionPolicy::Acl acls[1];
    acls[0].SetPeers(1, peers);
    acls[0].SetRules(1, rules);

    PermissionPolicy permissionPolicy;
    permissionPolicy.SetAcls(1, acls);

    ASSERT_FALSE(PermissionPolicyUtil::HasValidDenyRules(permissionPolicy));
}

TEST(PermissionPolicyUtilTest, validate_deny_rules_multiple_rules)
{
    PermissionPolicy::Peer peers[1];

    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);
    peers[0].SetKeyInfo(&keyInfoECC);
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);

    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("*");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(0); // explicit deny

    PermissionPolicy::Rule::Member otherMembers[1];
    otherMembers[0].SetMemberName("foo");
    otherMembers[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    otherMembers[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);

    PermissionPolicy::Rule rules[2];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("*");
    rules[0].SetMembers(1, members);
    rules[1].SetObjPath("/foo1/bar");
    rules[1].SetInterfaceName("baz");
    rules[1].SetMembers(1, otherMembers);

    PermissionPolicy::Acl acls[1];
    acls[0].SetPeers(1, peers);
    acls[0].SetRules(2, rules);

    PermissionPolicy permissionPolicy;
    permissionPolicy.SetAcls(1, acls);

    ASSERT_FALSE(PermissionPolicyUtil::HasValidDenyRules(permissionPolicy));
}

TEST(PermissionPolicyUtilTest, validate_deny_rules_no_deny_rules)
{
    PermissionPolicy::Peer peers[1];

    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);
    peers[0].SetKeyInfo(&keyInfoECC);
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);

    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("foo");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);

    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("*");
    rules[0].SetMembers(1, members);

    PermissionPolicy::Acl acls[1];
    acls[0].SetPeers(1, peers);
    acls[0].SetRules(1, rules);

    PermissionPolicy permissionPolicy;
    permissionPolicy.SetAcls(1, acls);

    ASSERT_TRUE(PermissionPolicyUtil::HasValidDenyRules(permissionPolicy));
}

TEST(PermissionPolicyUtilTest, validate_deny_rules_invalid_member)
{
    PermissionPolicy::Peer peers[1];

    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);
    peers[0].SetKeyInfo(&keyInfoECC);
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);

    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("foo");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    members[0].SetActionMask(0); // explicit deny

    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("*");
    rules[0].SetMembers(1, members);

    PermissionPolicy::Acl acls[1];
    acls[0].SetPeers(1, peers);
    acls[0].SetRules(1, rules);

    PermissionPolicy permissionPolicy;
    permissionPolicy.SetAcls(1, acls);

    ASSERT_FALSE(PermissionPolicyUtil::HasValidDenyRules(permissionPolicy));
}

TEST(PermissionPolicyUtilTest, validate_deny_rules_invalid_rule)
{
    PermissionPolicy::Peer peers[1];

    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);
    peers[0].SetKeyInfo(&keyInfoECC);
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);

    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("*");
    members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    members[0].SetActionMask(0); // explicit deny

    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("/foo1/bar");
    rules[0].SetInterfaceName("baz");
    rules[0].SetMembers(1, members);

    PermissionPolicy::Acl acls[1];
    acls[0].SetPeers(1, peers);
    acls[0].SetRules(1, rules);

    PermissionPolicy permissionPolicy;
    permissionPolicy.SetAcls(1, acls);

    ASSERT_FALSE(PermissionPolicyUtil::HasValidDenyRules(permissionPolicy));
}
