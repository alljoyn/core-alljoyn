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

#include <PermissionPolicy.h>
#include <cstdio>

using namespace ajn;

TEST(PermissionPolicyTest, construct)
{
    PermissionPolicy permissionPolicy;
    EXPECT_EQ((uint32_t)1, permissionPolicy.GetSpecificationVersion());
    EXPECT_EQ((uint16_t)0, permissionPolicy.GetVersion());

}

TEST(PermissionPolicyTest, SetSerialNum)
{
    PermissionPolicy permissionPolicy;
    permissionPolicy.SetVersion(555777666);
    EXPECT_EQ((uint32_t)555777666, permissionPolicy.GetVersion());
}

TEST(PermissionPolicyTest, Peer_constructor)
{
    PermissionPolicy::Peer peer;
    EXPECT_EQ(PermissionPolicy::Peer::PEER_ANY_TRUSTED, peer.GetType());
    EXPECT_EQ(qcc::GUID128(0), peer.GetSecurityGroupId());
}

TEST(PermissionPolicyTest, peer_set_get_type)
{
    PermissionPolicy::Peer peer;
    EXPECT_EQ(PermissionPolicy::Peer::PEER_ANY_TRUSTED, peer.GetType());
    peer.SetType(PermissionPolicy::Peer::PEER_ALL);
    EXPECT_EQ(PermissionPolicy::Peer::PEER_ALL, peer.GetType());
    peer.SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
    EXPECT_EQ(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY, peer.GetType());
    peer.SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
    EXPECT_EQ(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY, peer.GetType());
    peer.SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
    EXPECT_EQ(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP, peer.GetType());
}

TEST(PermissionPolicyTest, peer_set_get_SecurityGroupId)
{
    PermissionPolicy::Peer peer;
    qcc::GUID128 inputguid;
    peer.SetSecurityGroupId(inputguid);
    EXPECT_EQ(inputguid, peer.GetSecurityGroupId());
}

TEST(PermissionPolicyTest, peer_set_get_KeyInfo)
{
    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);

    PermissionPolicy::Peer peer;
    peer.SetKeyInfo(&keyInfoECC);
    EXPECT_EQ(keyInfoECC, *peer.GetKeyInfo());
}

TEST(PermissionPolicyTest, peer_equality)
{
    qcc::KeyInfoNISTP256 keyInfoECC1;
    uint8_t dummyKeyId1[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC1.SetKeyId(dummyKeyId1, 10);

    qcc::KeyInfoNISTP256 keyInfoECC2;
    uint8_t dummyKeyId2[10] = { 0, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    keyInfoECC2.SetKeyId(dummyKeyId2, 10);

    PermissionPolicy::Peer peer1;
    peer1.SetKeyInfo(&keyInfoECC1);

    PermissionPolicy::Peer peer2;
    peer2.SetKeyInfo(&keyInfoECC1);

    EXPECT_TRUE(peer1 == peer2);

    PermissionPolicy::Peer peer3;
    peer3.SetKeyInfo(&keyInfoECC2);
    EXPECT_TRUE(peer1 == peer3);
    EXPECT_TRUE(peer2 == peer3);
}

TEST(PermissionPolicyTest, peer_owns_key)
{
    PermissionPolicy::Peer peer1;

    {
        qcc::KeyInfoNISTP256 keyInfoECC1;
        uint8_t dummyKeyId1[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
        keyInfoECC1.SetKeyId(dummyKeyId1, 10);
        peer1.SetKeyInfo(&keyInfoECC1);
    }

    qcc::KeyInfoNISTP256 keyInfoECC2;
    uint8_t dummyKeyId2[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC2.SetKeyId(dummyKeyId2, 10);

    // If peer1 did take owner ship of dummyKeyId1 it would have gone out of
    // scope and this test would have crashed.
    EXPECT_EQ(keyInfoECC2, *peer1.GetKeyInfo());
}

TEST(PermissionPolicyTest, peer_copy_assign)
{
    PermissionPolicy::Peer peer;
    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);
    peer.SetKeyInfo(&keyInfoECC);

    PermissionPolicy::Peer peerCopy(peer);
    PermissionPolicy::Peer peerAssign = peer;

    ASSERT_EQ(peer, peerCopy);
    ASSERT_EQ(peer, peerAssign);
    ASSERT_EQ(peerCopy, peerAssign);
}

TEST(PermissionPolicyTest, rule_member_constructor)
{
    PermissionPolicy::Rule::Member member;
    EXPECT_STREQ("", member.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::NOT_SPECIFIED, member.GetMemberType());
    EXPECT_EQ(0, member.GetActionMask());
}

TEST(PermissionPolicyTest, rule_member_set_get_name)
{
    PermissionPolicy::Rule::Member member;
    member.SetMemberName("foo");
    EXPECT_STREQ("foo", member.GetMemberName().c_str());
}

TEST(PermissionPolicyTest, rule_member_set_get_type)
{
    PermissionPolicy::Rule::Member member;
    member.SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    EXPECT_EQ(PermissionPolicy::Rule::Member::METHOD_CALL, member.GetMemberType());
    member.SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    EXPECT_EQ(PermissionPolicy::Rule::Member::PROPERTY, member.GetMemberType());
    member.SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    EXPECT_EQ(PermissionPolicy::Rule::Member::SIGNAL, member.GetMemberType());
}

TEST(PermissionPolicyTest, rule_member_set_get_ActionMask)
{
    PermissionPolicy::Rule::Member member;
    EXPECT_EQ(0, member.GetActionMask());
    member.SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    EXPECT_EQ((uint8_t)PermissionPolicy::Rule::Member::ACTION_MODIFY, member.GetActionMask());
    member.SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    EXPECT_EQ((uint8_t)PermissionPolicy::Rule::Member::ACTION_OBSERVE, member.GetActionMask());
    member.SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    EXPECT_EQ((uint8_t)PermissionPolicy::Rule::Member::ACTION_PROVIDE, member.GetActionMask());
    member.SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    EXPECT_EQ((uint8_t)PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE, member.GetActionMask());
}

TEST(PermissionPolicyTest, rule_member_set)
{
    PermissionPolicy::Rule::Member member;
    member.Set("foo", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_MODIFY);
    EXPECT_STREQ("foo", member.GetMemberName().c_str());
    EXPECT_EQ((uint8_t)PermissionPolicy::Rule::Member::METHOD_CALL, member.GetMemberType());
    EXPECT_EQ((uint8_t)PermissionPolicy::Rule::Member::ACTION_MODIFY, member.GetActionMask());
}

TEST(PermissionPolicyTest, rule_member_default_assignment_copy)
{
    PermissionPolicy::Rule::Member member;
    member.Set("foo", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_MODIFY);
    PermissionPolicy::Rule::Member memberCopy(member);
    PermissionPolicy::Rule::Member memberAssign = member;

    EXPECT_EQ(member, memberCopy);
    EXPECT_EQ(member, memberAssign);
    EXPECT_EQ(memberCopy, memberAssign);

    member.Set("", PermissionPolicy::Rule::Member::NOT_SPECIFIED, 0);
    EXPECT_NE(member, memberCopy);
    EXPECT_NE(member, memberAssign);
    EXPECT_EQ(memberCopy, memberAssign);
}

TEST(PermissionPolicyTest, rule_constructor) {
    PermissionPolicy::Rule rule;
    EXPECT_STREQ("", rule.GetObjPath().c_str());
    EXPECT_STREQ("", rule.GetInterfaceName().c_str());
    EXPECT_TRUE(NULL == rule.GetMembers());
}

TEST(PermissionPolicyTest, rule_set_get_ObjectPath)
{
    PermissionPolicy::Rule rule;
    rule.SetObjPath("/foo/bar");
    EXPECT_STREQ("/foo/bar", rule.GetObjPath().c_str());
}

TEST(PermissionPolicyTest, rule_set_get_InterfaceName)
{
    PermissionPolicy::Rule rule;
    rule.SetInterfaceName("baz");
    EXPECT_STREQ("baz", rule.GetInterfaceName().c_str());
}

TEST(PermissionPolicyTest, rule_set_get_members)
{
    PermissionPolicy::Rule::Member members[2];
    members[0].Set("foo", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_MODIFY);
    members[1].Set("bar", PermissionPolicy::Rule::Member::SIGNAL, PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    PermissionPolicy::Rule rule;
    rule.SetMembers(2, members);
    ASSERT_EQ((size_t)2, rule.GetMembersSize());
    const PermissionPolicy::Rule::Member* outMembers = rule.GetMembers();
    EXPECT_EQ(members[0], outMembers[0]);
    EXPECT_EQ(members[1], outMembers[1]);

    //If the original members are changes the rule should not change
    members[0].Set("", PermissionPolicy::Rule::Member::NOT_SPECIFIED, 0);
    members[1].Set("", PermissionPolicy::Rule::Member::NOT_SPECIFIED, 0);

    outMembers = rule.GetMembers();
    EXPECT_STREQ("foo", outMembers[0].GetMemberName().c_str());
    EXPECT_EQ((uint8_t)PermissionPolicy::Rule::Member::METHOD_CALL, outMembers[0].GetMemberType());
    EXPECT_EQ((uint8_t)PermissionPolicy::Rule::Member::ACTION_MODIFY, outMembers[0].GetActionMask());
    EXPECT_STREQ("bar", outMembers[1].GetMemberName().c_str());
    EXPECT_EQ((uint8_t)PermissionPolicy::Rule::Member::SIGNAL, outMembers[1].GetMemberType());
    EXPECT_EQ((uint8_t)PermissionPolicy::Rule::Member::ACTION_OBSERVE, outMembers[1].GetActionMask());

}

TEST(PermissionPolicyTest, rule_ToString)
{
    PermissionPolicy::Rule::Member members[2];
    members[0].Set("foo", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_MODIFY);
    members[1].Set("bar", PermissionPolicy::Rule::Member::SIGNAL, PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    PermissionPolicy::Rule rule;
    rule.SetObjPath("/foo/bar");
    rule.SetInterfaceName("baz");
    rule.SetMembers(2, members);

    const char* expected = "Rule:\n"
                           "  objPath: /foo/bar\n"
                           "  interfaceName: baz\n"
                           "Member:\n"
                           "  memberName: foo\n"
                           "  method call\n"
                           "  action mask: Modify\n"
                           "Member:\n"
                           "  memberName: bar\n"
                           "  signal\n"
                           "  action mask: Observe\n";
    EXPECT_STREQ(expected, rule.ToString().c_str());
}

TEST(PermissionPolicyTest, rule_copy_assign)
{
    PermissionPolicy::Rule::Member members[2];
    members[0].Set("foo", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_MODIFY);
    members[1].Set("bar", PermissionPolicy::Rule::Member::SIGNAL, PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    PermissionPolicy::Rule rule;
    rule.SetObjPath("/foo/bar");
    rule.SetInterfaceName("baz");
    rule.SetMembers(2, members);

    PermissionPolicy::Rule ruleCopy(rule);
    PermissionPolicy::Rule ruleAssign = rule;

    EXPECT_EQ(rule, ruleCopy);
    EXPECT_EQ(rule, ruleAssign);
    EXPECT_EQ(ruleCopy, ruleAssign);
}

TEST(PermissionPolicyTest, acl_constructor)
{
    PermissionPolicy::Acl acl;
    EXPECT_TRUE(NULL == acl.GetPeers());
    EXPECT_EQ((size_t)0, acl.GetPeersSize());
    EXPECT_TRUE(NULL == acl.GetRules());
    EXPECT_EQ((size_t)0, acl.GetRulesSize());
}


TEST(PermissionPolicyTest, acl_set_get_Peers)
{
    PermissionPolicy::Peer peer[3];
    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);

    peer[0].SetKeyInfo(&keyInfoECC);
    peer[1].SetKeyInfo(&keyInfoECC);
    peer[2].SetKeyInfo(&keyInfoECC);

    PermissionPolicy::Acl acl;
    acl.SetPeers(3, peer);

    ASSERT_EQ((size_t)3, acl.GetPeersSize());
    const PermissionPolicy::Peer* outPeers = acl.GetPeers();
    EXPECT_EQ(outPeers[0], peer[0]);
    EXPECT_EQ(outPeers[1], peer[1]);
    EXPECT_EQ(outPeers[2], peer[2]);
}

TEST(PermissionPolicyTest, acl_set_get_Rules)
{
    PermissionPolicy::Rule::Member members[2];
    members[0].Set("foo", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_MODIFY);
    members[1].Set("bar", PermissionPolicy::Rule::Member::SIGNAL, PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    PermissionPolicy::Rule rule[2];
    rule[0].SetObjPath("/foo0/bar");
    rule[0].SetInterfaceName("baz0");
    rule[0].SetMembers(2, members);

    rule[1].SetObjPath("/foo1/bar");
    rule[1].SetInterfaceName("baz1");
    rule[1].SetMembers(2, members);

    PermissionPolicy::Acl acl;
    acl.SetRules(2, rule);
    ASSERT_EQ((size_t)2, acl.GetRulesSize());
    const PermissionPolicy::Rule* outRules = acl.GetRules();
    EXPECT_EQ((size_t)2, outRules[0].GetMembersSize());
    EXPECT_EQ((size_t)2, outRules[1].GetMembersSize());
    EXPECT_EQ(outRules[0], rule[0]);
    EXPECT_EQ(outRules[1], rule[1]);
}

TEST(PermissionPolicyTest, acl_assign_copy)
{
    PermissionPolicy::Peer peer[3];
    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);

    peer[0].SetKeyInfo(&keyInfoECC);
    peer[1].SetKeyInfo(&keyInfoECC);
    peer[2].SetKeyInfo(&keyInfoECC);

    PermissionPolicy::Rule::Member members[2];
    members[0].Set("foo", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_MODIFY);
    members[1].Set("bar", PermissionPolicy::Rule::Member::SIGNAL, PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    PermissionPolicy::Rule rule[2];
    rule[0].SetObjPath("/foo0/bar");
    rule[0].SetInterfaceName("baz0");
    rule[0].SetMembers(2, members);

    rule[1].SetObjPath("/foo1/bar");
    rule[1].SetInterfaceName("baz1");
    rule[1].SetMembers(2, members);

    PermissionPolicy::Acl acl;
    acl.SetPeers(3, peer);
    acl.SetRules(2, rule);

    PermissionPolicy::Acl aclCopy(acl);
    PermissionPolicy::Acl aclAssign = acl;

    ASSERT_EQ(acl, aclCopy);
    ASSERT_EQ(acl, aclAssign);
    ASSERT_EQ(aclCopy, aclAssign);
}

TEST(PermissionPolicyTest, set_get_Acls)
{
    PermissionPolicy::Peer peer[3];
    qcc::KeyInfoNISTP256 keyInfoECC;
    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);

    peer[0].SetKeyInfo(&keyInfoECC);
    peer[1].SetKeyInfo(&keyInfoECC);
    peer[2].SetKeyInfo(&keyInfoECC);

    PermissionPolicy::Rule::Member members[2];
    members[0].Set("foo", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_MODIFY);
    members[1].Set("bar", PermissionPolicy::Rule::Member::SIGNAL, PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    PermissionPolicy::Rule rule[2];
    rule[0].SetObjPath("/foo0/bar");
    rule[0].SetInterfaceName("baz0");
    rule[0].SetMembers(2, members);

    rule[1].SetObjPath("/foo1/bar");
    rule[1].SetInterfaceName("baz1");
    rule[1].SetMembers(2, members);

    PermissionPolicy::Acl acls[2];
    acls[0].SetPeers(3, peer);
    acls[1].SetRules(2, rule);

    PermissionPolicy permissionPolicy;
    permissionPolicy.SetAcls(2, acls);
    const PermissionPolicy::Acl* aclsOut = permissionPolicy.GetAcls();

    ASSERT_EQ((size_t)2, permissionPolicy.GetAclsSize());

    EXPECT_EQ(acls[0], aclsOut[0]);
    EXPECT_EQ(acls[1], aclsOut[1]);
}
