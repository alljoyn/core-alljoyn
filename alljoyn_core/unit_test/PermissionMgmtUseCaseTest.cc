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

#include "PermissionMgmtTest.h"
#include <string>

using namespace ajn;
using namespace qcc;

static const char* PERMISSION_MGMT_PATH = "/org/allseen/Security/PermissionMgmt";

static GUID128 membershipGUID1;
static const char* membershipSerial1 = "10001";
static GUID128 membershipGUID2;
static GUID128 membershipGUID3;
static GUID128 membershipGUID4;
static const char* membershipSerial2 = "20002";
static const char* membershipSerial3 = "30003";
static const char* membershipSerial4 = "40004";

static const char sampleCertificatePEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAf8thIwHzhCU8qsedyuEldP/TouX6w7rZI/cJYST/kexAAAAAMvbuy8JDCJI\n"
    "Ms8vwkglUrf/infSYMNRYP/gsFvl5FutAAAAAAAAAAD/LYSMB84QlPKrHncrhJXT\n"
    "/06Ll+sO62SP3CWEk/5HsQAAAADL27svCQwiSDLPL8JIJVK3/4p30mDDUWD/4LBb\n"
    "5eRbrQAAAAAAAAAAAAAAAAASgF0AAAAAABKBiQABMa7uTLSqjDggO0t6TAgsxKNt\n"
    "+Zhu/jc3s242BE0drFU12USXXIYQdqps/HrMtqw6q9hrZtaGJS+e9y7mJegAAAAA\n"
    "APpeLT1cHNm3/OupnEcUCmg+jqi4SUEi4WTWSR4OzvCSAAAAAA==\n"
    "-----END CERTIFICATE-----"
};

static PermissionPolicy* GenerateWildCardPolicy(qcc::GUID128& guid, qcc::ECCPublicKey& adminPublicKey)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(52516);

    /* add the admin section */
    PermissionPolicy::Peer* admins = new PermissionPolicy::Peer[1];
    admins[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guid.GetBytes(), guid.SIZE);
    keyInfo->SetPublicKey(&adminPublicKey);
    admins[0].SetKeyInfo(keyInfo);
    policy->SetAdmins(1, admins);

    /* add the terms section */
    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("org.allseenalliance.control.*");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(3, prms);
    terms[0].SetRules(1, rules);
    policy->SetTerms(1, terms);

    return policy;
}
static PermissionPolicy* GeneratePolicy(qcc::GUID128& guid, qcc::ECCPublicKey& adminPublicKey, qcc::ECCPublicKey& guildAuthority)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(74892317);

    /* add the admin section */
    PermissionPolicy::Peer* admins = new PermissionPolicy::Peer[1];
    admins[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guid.GetBytes(), guid.SIZE);
    keyInfo->SetPublicKey(&adminPublicKey);
    admins[0].SetKeyInfo(keyInfo);
    policy->SetAdmins(1, admins);

    /* add the terms section */
    PermissionPolicy::Term* terms = new PermissionPolicy::Term[4];

    /* terms record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[3];
    rules[0].SetObjPath("/control/guide");
    rules[0].SetInterfaceName("allseenalliance.control.*");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Off");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    prms[1].SetMemberName("*");
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(2, prms);
    rules[2].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("ChannelChanged");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    rules[2].SetMembers(1, prms);
    terms[0].SetRules(3, rules);

    /* terms record 1 GUILD membershipGUID1 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(membershipGUID1.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[5];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("Volume");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[3].SetMemberName("InputSource");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[4].SetMemberName("Caption");
    prms[4].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[4].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(5, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);
    terms[1].SetRules(2, rules);

    /* terms record 2 GUILD membershipGUID2 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(membershipGUID2.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[2].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[3];
    rules[0].SetObjPath("/control/settings");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    rules[0].SetMembers(1, prms);
    rules[1].SetObjPath("/control/guide");
    rules[1].SetInterfaceName("org.allseenalliance.control.*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);
    rules[2].SetObjPath("*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[2].SetMembers(1, prms);
    terms[2].SetRules(3, rules);

    /* terms record 3 peer specific rule  */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[3].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("Mute");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    terms[3].SetRules(1, rules);

    policy->SetTerms(4, terms);

    return policy;
}

static PermissionPolicy* GenerateAnyUserPolicyWithLevel(qcc::GUID128& guid, qcc::ECCPublicKey& adminPublicKey)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(726129);

    /* add the admin section */
    PermissionPolicy::Peer* admins = new PermissionPolicy::Peer[1];
    admins[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guid.GetBytes(), guid.SIZE);
    keyInfo->SetPublicKey(&adminPublicKey);
    admins[0].SetKeyInfo(keyInfo);
    policy->SetAdmins(1, admins);

    /* add the terms ection */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[2];

    /* terms record 0  ANY-USER encrypted level */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    peers[0].SetLevel(PermissionPolicy::Peer::PEER_LEVEL_ENCRYPTED);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("On");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    terms[0].SetRules(1, rules);

    /* terms record 1  ANY-USER authenticated level */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    peers[0].SetLevel(PermissionPolicy::Peer::PEER_LEVEL_AUTHENTICATED);
    terms[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Off");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    terms[1].SetRules(1, rules);

    policy->SetTerms(2, terms);
    return policy;
}

static PermissionPolicy* GenerateSmallAnyUserPolicy(qcc::GUID128& guid, qcc::ECCPublicKey& adminPublicKey)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(552317);

    /* add the admin section */
    PermissionPolicy::Peer* admins = new PermissionPolicy::Peer[1];
    admins[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guid.GetBytes(), guid.SIZE);
    keyInfo->SetPublicKey(&adminPublicKey);
    admins[0].SetKeyInfo(keyInfo);
    policy->SetAdmins(1, admins);

    /* add the terms ection */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Off");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("On");
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);
    rules[1].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("ChannelChanged");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    rules[1].SetMembers(1, prms);
    terms[0].SetRules(2, rules);

    policy->SetTerms(1, terms);

    return policy;
}

static PermissionPolicy* GenerateAnyUserDeniedPrefixPolicy(qcc::GUID128& guid, qcc::ECCPublicKey& adminPublicKey)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(552317);

    /* add the admin section */
    PermissionPolicy::Peer* admins = new PermissionPolicy::Peer[1];
    admins[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guid.GetBytes(), guid.SIZE);
    keyInfo->SetPublicKey(&adminPublicKey);
    admins[0].SetKeyInfo(keyInfo);
    policy->SetAdmins(1, admins);

    /* add the incoming section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Of*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    prms[1].SetMemberName("*");
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);
    rules[1].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("ChannelChanged");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    rules[1].SetMembers(1, prms);
    terms[0].SetRules(2, rules);

    policy->SetTerms(1, terms);
    return policy;
}

static PermissionPolicy* GenerateFullAccessOutgoingPolicy()
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(3827326);

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    rules[0].SetMembers(3, prms);
    terms[0].SetRules(1, rules);

    policy->SetTerms(1, terms);
    return policy;
}

static PermissionPolicy* GenerateGuildSpecificAccessOutgoingPolicy(const GUID128& guildGUID, qcc::ECCPublicKey& guildAuthority)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(3827326);

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[2];

    /* terms record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    rules[0].SetMembers(3, prms);
    terms[0].SetRules(1, rules);

    /* terms record 1 GUILD specific */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guildGUID.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    rules[0].SetMembers(3, prms);
    terms[1].SetRules(1, rules);

    policy->SetTerms(2, terms);
    return policy;
}

static PermissionPolicy* GenerateGuildSpecificAccessProviderAuthData(const GUID128& guildGUID, qcc::ECCPublicKey& guildAuthority)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(3827326);

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0 GUILD specific */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guildGUID.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    rules[0].SetMembers(3, prms);
    terms[0].SetRules(1, rules);

    policy->SetTerms(1, terms);
    return policy;
}

static PermissionPolicy* GeneratePolicyPeerPublicKey(qcc::GUID128& guid, qcc::ECCPublicKey& adminPublicKey, qcc::ECCPublicKey& peerPublicKey)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(8742198);

    /* add the admin section */
    PermissionPolicy::Peer* admins = new PermissionPolicy::Peer[1];
    admins[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guid.GetBytes(), guid.SIZE);
    keyInfo->SetPublicKey(&adminPublicKey);
    admins[0].SetKeyInfo(keyInfo);
    policy->SetAdmins(1, admins);

    /* add the provider section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0 peer */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetPublicKey(&peerPublicKey);
    peers[0].SetKeyInfo(keyInfo);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[4];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("Volume");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[3].SetMemberName("Caption");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(4, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    terms[0].SetRules(2, rules);
    policy->SetTerms(1, terms);

    return policy;
}

static PermissionPolicy* GeneratePolicyDenyPeerPublicKey(qcc::GUID128& guid, qcc::ECCPublicKey& adminPublicKey, qcc::ECCPublicKey& peerPublicKey)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(32445);

    /* add the admin section */
    PermissionPolicy::Peer* admins = new PermissionPolicy::Peer[1];
    admins[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guid.GetBytes(), guid.SIZE);
    keyInfo->SetPublicKey(&adminPublicKey);
    admins[0].SetKeyInfo(keyInfo);
    policy->SetAdmins(1, admins);

    /* add the provider section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0 peer */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetPublicKey(&peerPublicKey);
    peers[0].SetKeyInfo(keyInfo);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[4];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    prms[2].SetMemberName("Volume");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[3].SetMemberName("Caption");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(4, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    terms[0].SetRules(2, rules);
    policy->SetTerms(1, terms);

    return policy;
}

static PermissionPolicy* GenerateMembershipAuthData(const GUID128* guildGUID)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(88473);

    /* add the outgoing section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* outgoing terms record 0 */
    if (guildGUID) {
        PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
        KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
        keyInfo->SetKeyId(guildGUID->GetBytes(), qcc::GUID128::SIZE);
        peers[0].SetKeyInfo(keyInfo);
        terms[0].SetPeers(1, peers);
    }
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[5];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("ChannelChanged");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[3].SetMemberName("Volume");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[4].SetMemberName("Caption");
    prms[4].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[4].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(5, prms);

    rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    terms[0].SetRules(2, rules);
    policy->SetTerms(1, terms);

    return policy;
}

static PermissionPolicy* GenerateOverReachingMembershipAuthData(const GUID128* guildGUID)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(463621);

    /* add the outgoing section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* outgoing terms record 0 */
    if (guildGUID) {
        PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
        KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
        keyInfo->SetKeyId(guildGUID->GetBytes(), qcc::GUID128::SIZE);
        peers[0].SetKeyInfo(keyInfo);
        terms[0].SetPeers(1, peers);
    }
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[5];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("ChannelChanged");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    prms[3].SetMemberName("Volume");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[4].SetMemberName("InputSource");
    prms[4].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[4].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(5, prms);

    rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    terms[0].SetRules(2, rules);
    policy->SetTerms(1, terms);

    return policy;
}

static PermissionPolicy* GenerateMembershipAuthData()
{
    return GenerateMembershipAuthData(NULL);
}

static PermissionPolicy* GenerateLesserMembershipAuthData(bool useDenied, const GUID128* guildGUID)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(88473);

    /* add the outgoing section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* outgoing terms record 0 */
    if (guildGUID) {
        PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
        KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
        keyInfo->SetKeyId(guildGUID->GetBytes(), qcc::GUID128::SIZE);
        peers[0].SetKeyInfo(keyInfo);
        terms[0].SetPeers(1, peers);
    }
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[5];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("ChannelChanged");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    prms[3].SetMemberName("Volume");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[4].SetMemberName("Caption");
    prms[4].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    if (useDenied) {
        prms[4].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    }
    rules[0].SetMembers(5, prms);

    rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    terms[0].SetRules(2, rules);
    policy->SetTerms(1, terms);

    return policy;
}
static PermissionPolicy* GenerateMembershipAuthDataForAdmin()
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(5672);

    /* add the outgoing section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0 */
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(3, prms);

    rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    terms[0].SetRules(2, rules);
    policy->SetTerms(1, terms);

    return policy;
}

static PermissionPolicy* GenerateAuthDataProvideSignal()
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(88473);

    /* add the outgoing section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0 */
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("ChannelChanged");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    terms[0].SetRules(1, rules);
    policy->SetTerms(1, terms);

    return policy;
}

static void GenerateMembershipAuthChain(PermissionPolicy** authDataArray, size_t count)
{
    for (size_t cnt = 0; cnt < count; cnt++) {
        authDataArray[cnt] = GenerateMembershipAuthData();
    }
    authDataArray[0]->SetSerialNum(88474);
}

static void GenerateOverReachingMembershipAuthChain(PermissionPolicy** authDataArray, size_t count)
{
    if (count == 2) {
        authDataArray[1] = GenerateMembershipAuthData();
        authDataArray[0] = GenerateOverReachingMembershipAuthData(NULL);
    }
}

static QStatus GenerateManifest(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    *retRules = rules;
    return ER_OK;
}

class PermissionMgmtUseCaseTest : public BasePermissionMgmtTest {

  public:
    PermissionMgmtUseCaseTest() : BasePermissionMgmtTest("/app")
    {
    }
    PermissionMgmtUseCaseTest(const char* path) : BasePermissionMgmtTest(path)
    {
    }

    /**
     *  Claim the admin app
     */
    void ClaimAdmin()
    {
        QStatus status = ER_OK;

        /* factory reset */
        PermissionConfigurator& pc = adminBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        /* Gen DSA keys */
        status = pc.GenerateSigningKeyPair();
        EXPECT_EQ(ER_OK, status) << "  GenerateSigningKeyPair failed.  Actual Status: " << QCC_StatusText(status);

        /* Retrieve the DSA keys */
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminProxyBus, adminBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
        ProxyBusObject clientProxyObject(adminProxyBus, adminBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
        ECCPublicKey claimedPubKey;
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);

        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("1010101", issuerGUID, &issuerPrivateKey, issuerGUID, &issuerPubKey, "Admin User", der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::Claim(adminProxyBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, der);
        EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

        /* retrieve back the identity cert to compare */
        IdentityCertificate newCert;
        status = PermissionMgmtTestHelper::GetIdentity(adminProxyBus, clientProxyObject, newCert);
        EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String retIdentity;
        status = newCert.EncodeCertificateDER(retIdentity);
        EXPECT_EQ(ER_OK, status) << "  newCert.EncodeCertificateDER failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_STREQ(der.c_str(), retIdentity.c_str()) << "  GetIdentity failed.  Return value does not equal original";

        /* reload the shared key store because of change on one bus */
        adminProxyBus.ReloadKeyStore();
        adminBus.ReloadKeyStore();
        EnableSecurity("ALLJOYN_ECDHE_ECDSA");
        PermissionPolicy* policy = GenerateMembershipAuthDataForAdmin();
        InstallMembershipToAdmin(policy);
        delete policy;
        policy = GenerateFullAccessOutgoingPolicy();
        InstallPolicyToAdmin(*policy);
        delete policy;
    }

    /**
     *  Claim the service app
     */
    void ClaimService()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = serviceBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminBus, serviceBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);

        SetNotifyConfigSignalReceived(false);

        /* setup state unclaimable */
        PermissionConfigurator::ClaimableState claimableState = pc.GetClaimableState();
        EXPECT_EQ(PermissionConfigurator::STATE_CLAIMABLE, claimableState) << "  ClaimableState is not CLAIMABLE";
        status = pc.SetClaimable(false);
        EXPECT_EQ(ER_OK, status) << "  SetClaimable failed.  Actual Status: " << QCC_StatusText(status);
        claimableState = pc.GetClaimableState();
        EXPECT_EQ(PermissionConfigurator::STATE_UNCLAIMABLE, claimableState) << "  ClaimableState is not UNCLAIMABLE";
        qcc::GUID128 subjectGUID;
        PermissionMgmtTestHelper::GetGUID(serviceBus, subjectGUID);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

        ECCPublicKey claimedPubKey;
        /* retrieve public key from to-be-claimed app to create identity cert */
        status = PermissionMgmtTestHelper::GetPeerPublicKey(adminBus, clientProxyObject, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  GetPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
        /* create identity cert for the claimed app */
        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("2020202", issuerGUID, &issuerPrivateKey, subjectGUID, &claimedPubKey, "Service Provider", der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

        /* try claiming with state unclaimable.  Exptect to fail */
        status = PermissionMgmtTestHelper::Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, der, false);
        EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

        /* now switch it back to claimable */
        status = pc.SetClaimable(true);
        EXPECT_EQ(ER_OK, status) << "  SetClaimable failed.  Actual Status: " << QCC_StatusText(status);
        claimableState = pc.GetClaimableState();
        EXPECT_EQ(PermissionConfigurator::STATE_CLAIMABLE, claimableState) << "  ClaimableState is not CLAIMABLE";

        /* try claiming with state laimable.  Exptect to succeed */
        status = PermissionMgmtTestHelper::Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, der, false);
        EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

        /* try to claim one more time */
        status = PermissionMgmtTestHelper::Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, der);
        EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

        ECCPublicKey claimedPubKey2;
        /* retrieve public key from claimed app to validate that it is not changed */
        status = PermissionMgmtTestHelper::GetPeerPublicKey(adminBus, clientProxyObject, &claimedPubKey2);
        EXPECT_EQ(ER_OK, status) << "  GetPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(memcmp(&claimedPubKey2, &claimedPubKey, sizeof(ECCPublicKey)), 0) << "  The public key of the claimed app has changed.";

        /* sleep a second to see whether the NotifyConfig signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetNotifyConfigSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";

        //status = adminBus.LeaveSession(sessionId);
        //EXPECT_EQ(ER_OK, status) << "  LeaveSession failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Claim the consumer
     */
    void ClaimConsumer()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = consumerBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminBus, consumerBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
        ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
        ECCPublicKey claimedPubKey;

        qcc::GUID128 subjectGUID;
        PermissionMgmtTestHelper::GetGUID(consumerBus, subjectGUID);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);
        /* retrieve public key from to-be-claimed app to create identity cert */
        status = PermissionMgmtTestHelper::GetPeerPublicKey(adminBus, clientProxyObject, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  GetPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
        /* create identity cert for the claimed app */
        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("3030303", issuerGUID, &issuerPrivateKey, subjectGUID, &claimedPubKey, "Consumer", der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);
        SetNotifyConfigSignalReceived(false);
        status = PermissionMgmtTestHelper::Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, der);
        EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

        /* try to claim a second time */
        status = PermissionMgmtTestHelper::Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, der);
        EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

        /* sleep a second to see whether the NotifyConfig signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetNotifyConfigSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
        //status = adminBus.LeaveSession(sessionId);
        //EXPECT_EQ(ER_OK, status) << "  LeaveSession failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Claim the remote control by the consumer
     */
    void ConsumerClaimsRemoteControl()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = remoteControlBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(consumerBus, remoteControlBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
        ProxyBusObject clientProxyObject(consumerBus, remoteControlBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
        ECCPublicKey claimedPubKey;

        qcc::GUID128 remoteControlGUID;
        PermissionMgmtTestHelper::GetGUID(remoteControlBus, remoteControlGUID);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(consumerBus, issuerGUID);
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(consumerBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);
        /* retrieve public key from to-be-claimed app to create identity cert */
        status = PermissionMgmtTestHelper::GetPeerPublicKey(consumerBus, clientProxyObject, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  GetPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
        /* create identity cert for the claimed app */
        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("6060606", issuerGUID, &issuerPrivateKey, remoteControlGUID, &claimedPubKey, "remote control", der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);
        SetNotifyConfigSignalReceived(false);
        status = PermissionMgmtTestHelper::Claim(consumerBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, der);
        EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

        /* sleep a second to see whether the NotifyConfig signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetNotifyConfigSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
        //status = consumerBus.LeaveSession(sessionId);
        //EXPECT_EQ(ER_OK, status) << "  LeaveSession failed.  Actual Status: " << QCC_StatusText(status);
    }

    void Claims(bool usePSK, bool claimRemoteControl)
    {
        if (usePSK) {
            EnableSecurity("ALLJOYN_ECDHE_PSK");
        } else {
            EnableSecurity("ALLJOYN_ECDHE_NULL");
        }
        ClaimAdmin();
        if (usePSK) {
            EnableSecurity("ALLJOYN_ECDHE_PSK");
        } else {
            EnableSecurity("ALLJOYN_ECDHE_NULL");
        }
        ClaimService();
        ClaimConsumer();
        if (claimRemoteControl) {
            ConsumerClaimsRemoteControl();
        }
        EnableSecurity("ALLJOYN_ECDHE_ECDSA");
    }

    void Claims(bool usePSK)
    {
        /* also claims the remote control */
        Claims(usePSK, true);
    }

    /**
     *  Install policy to service app
     */
    void InstallPolicyToAdmin(PermissionPolicy& policy)
    {
        ProxyBusObject clientProxyObject(adminProxyBus, adminBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        status = PermissionMgmtTestHelper::InstallPolicy(adminProxyBus, clientProxyObject, policy);
        EXPECT_EQ(ER_OK, status) << "  InstallPolicy failed.  Actual Status: " << QCC_StatusText(status);

        /* retrieve back the policy to compare */
        PermissionPolicy retPolicy;
        status = PermissionMgmtTestHelper::GetPolicy(adminProxyBus, clientProxyObject, retPolicy);
        EXPECT_EQ(ER_OK, status) << "  GetPolicy failed.  Actual Status: " << QCC_StatusText(status);

        EXPECT_EQ(policy.GetSerialNum(), retPolicy.GetSerialNum()) << " GetPolicy failed. Different serial number.";
        EXPECT_EQ(policy.GetAdminsSize(), retPolicy.GetAdminsSize()) << " GetPolicy failed. Different admin size.";
        EXPECT_EQ(policy.GetTermsSize(), retPolicy.GetTermsSize()) << " GetPolicy failed. Different incoming terms size.";
    }

    /**
     *  Install policy to app
     */
    void InstallPolicyToNoAdmin(BusAttachment& installerBus, BusAttachment& bus, PermissionPolicy& policy)
    {
        ProxyBusObject clientProxyObject(installerBus, bus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        /* retrieve the policy */
        PermissionPolicy aPolicy;
        status = PermissionMgmtTestHelper::GetPolicy(installerBus, clientProxyObject, aPolicy);
        EXPECT_NE(ER_OK, status) << "  GetPolicy not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

        SetNotifyConfigSignalReceived(false);
        status = PermissionMgmtTestHelper::InstallPolicy(installerBus, clientProxyObject, policy);
        EXPECT_EQ(ER_OK, status) << "  InstallPolicy failed.  Actual Status: " << QCC_StatusText(status);

        /* retrieve back the policy to compare */
        PermissionPolicy retPolicy;
        status = PermissionMgmtTestHelper::GetPolicy(installerBus, clientProxyObject, retPolicy);
        EXPECT_EQ(ER_OK, status) << "  GetPolicy failed.  Actual Status: " << QCC_StatusText(status);

        EXPECT_EQ(policy.GetSerialNum(), retPolicy.GetSerialNum()) << " GetPolicy failed. Different serial number.";
        EXPECT_EQ(policy.GetAdminsSize(), retPolicy.GetAdminsSize()) << " GetPolicy failed. Different admin size.";
        EXPECT_EQ(policy.GetTermsSize(), retPolicy.GetTermsSize()) << " GetPolicy failed. Different incoming terms size.";
        /* sleep a second to see whether the NotifyConfig signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetNotifyConfigSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
        /* install a policy with the same serial number.  Expect to fail. */
        status = PermissionMgmtTestHelper::InstallPolicy(installerBus, clientProxyObject, policy);
        EXPECT_NE(ER_OK, status) << "  InstallPolicy again with same serial number expected to fail, but it did not.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Install policy to service app
     */
    void InstallPolicyToService(PermissionPolicy& policy)
    {
        InstallPolicyToNoAdmin(adminBus, serviceBus, policy);
    }

    /**
     *  Install policy to app
     */
    void InstallPolicyToClientBus(BusAttachment& installerBus, BusAttachment& targetBus, PermissionPolicy& policy)
    {
        InstallPolicyToNoAdmin(installerBus, targetBus, policy);
    }

    /**
     *  Install policy to app
     */
    void InstallPolicyToConsumer(PermissionPolicy& policy)
    {
        InstallPolicyToNoAdmin(adminBus, consumerBus, policy);
    }

    /*
     *  Replace service app Identity Certificate
     */
    void ReplaceServiceIdentityCert()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        /* retrieve the current identity cert */
        IdentityCertificate cert;
        status = PermissionMgmtTestHelper::GetIdentity(adminBus, clientProxyObject, cert);
        EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);

        /* create a new identity cert */
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("4040404", issuerGUID, &issuerPrivateKey, cert.GetSubject(), cert.GetSubjectPublicKey(), "Service Provider", der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

        status = PermissionMgmtTestHelper::InstallIdentity(adminBus, clientProxyObject, der);
        EXPECT_EQ(ER_OK, status) << "  InstallIdentity failed.  Actual Status: " << QCC_StatusText(status);

        /* retrieve back the identity cert to compare */
        IdentityCertificate newCert;
        status = PermissionMgmtTestHelper::GetIdentity(adminBus, clientProxyObject, newCert);
        EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String retIdentity;
        status = newCert.EncodeCertificateDER(retIdentity);
        EXPECT_EQ(ER_OK, status) << "  newCert.EncodeCertificateDER failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_STREQ(der.c_str(), retIdentity.c_str()) << "  GetIdentity failed.  Return value does not equal original";

    }

    void ReplaceServiceIdentityCertWithBadPublicKey()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        /* retrieve the current identity cert */
        IdentityCertificate cert;
        status = PermissionMgmtTestHelper::GetIdentity(adminBus, clientProxyObject, cert);
        EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);

        /* create a new identity cert */
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("5050505", issuerGUID, &issuerPrivateKey, issuerGUID, &issuerPubKey, "Service Provider", der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

        status = PermissionMgmtTestHelper::InstallIdentity(adminBus, clientProxyObject, der);
        EXPECT_NE(ER_OK, status) << "  InstallIdentity did not fail.  Actual Status: " << QCC_StatusText(status);
    }

    void ReplaceIdentityCertWithExpiredCert(BusAttachment& installerBus, BusAttachment& targetBus)
    {
        ProxyBusObject clientProxyObject(installerBus, targetBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        /* retrieve the current identity cert */
        IdentityCertificate cert;
        status = PermissionMgmtTestHelper::GetIdentity(installerBus, clientProxyObject, cert);
        EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);

        /* create a new identity cert that will expire in 1 second */
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(installerBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("5050505", issuerGUID, &issuerPrivateKey, cert.GetSubject(), cert.GetSubjectPublicKey(), "Service Provider", 1, der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

        /* sleep 2 seconds to get the cert to expire */
        qcc::Sleep(2000);
        status = PermissionMgmtTestHelper::InstallIdentity(adminBus, clientProxyObject, der);
        EXPECT_NE(ER_OK, status) << "  InstallIdentity did not fail.  Actual Status: " << QCC_StatusText(status);
    }

    void InstallAdditionalIdentityTrustAnchor(BusAttachment& installerBus, BusAttachment& sourceBus, BusAttachment& targetBus, bool testForDuplicates)
    {
        qcc::GUID128 installerGUID;
        PermissionMgmtTestHelper::GetGUID(installerBus, installerGUID);
        qcc::GUID128 targetGUID;
        PermissionMgmtTestHelper::GetGUID(targetBus, targetGUID);
        qcc::GUID128 sourceGUID;
        PermissionMgmtTestHelper::GetGUID(sourceBus, sourceGUID);
        ECCPublicKey sourcePublicKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(sourceBus, &sourcePublicKey);

        ProxyBusObject targetClient(installerBus, targetBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        status = PermissionMgmtTestHelper::InstallCredential(PermissionMgmtObj::TRUST_ANCHOR_IDENTITY, installerBus, targetClient, sourceGUID, &sourcePublicKey);
        EXPECT_EQ(ER_OK, status) << "  InstallCredential failed.  Actual Status: " << QCC_StatusText(status);

        if (testForDuplicates) {
            status = PermissionMgmtTestHelper::InstallCredential(PermissionMgmtObj::TRUST_ANCHOR_IDENTITY, installerBus, targetClient, sourceGUID, &sourcePublicKey);
            EXPECT_NE(ER_OK, status) << "  Test for duplicate: InstallCredential did not fail.  Actual Status: " << QCC_StatusText(status);
        }
    }

    void InstallAdditionalIdentityTrustAnchor(BusAttachment& installerBus, BusAttachment& sourceBus, BusAttachment& targetBus)
    {
        InstallAdditionalIdentityTrustAnchor(installerBus, sourceBus, targetBus, false);
    }

    void RemoveIdentityTrustAnchor(BusAttachment& installerBus, BusAttachment& sourceBus, BusAttachment& targetBus)
    {
        qcc::GUID128 installerGUID;
        PermissionMgmtTestHelper::GetGUID(installerBus, installerGUID);
        qcc::GUID128 targetGUID;
        PermissionMgmtTestHelper::GetGUID(targetBus, targetGUID);
        qcc::GUID128 sourceGUID;
        PermissionMgmtTestHelper::GetGUID(sourceBus, sourceGUID);
        ECCPublicKey sourcePublicKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(sourceBus, &sourcePublicKey);

        ProxyBusObject targetClient(installerBus, targetBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        status = PermissionMgmtTestHelper::RemoveCredential(PermissionMgmtObj::TRUST_ANCHOR_IDENTITY, installerBus, targetClient, sourceGUID, &sourcePublicKey);
        EXPECT_EQ(ER_OK, status) << "  RemoveCredential failed.  Actual Status: " << QCC_StatusText(status);

    }

    /**
     *  Install Membership to service provider
     */
    void InstallMembershipToServiceProvider(const char* serial, qcc::GUID128& guildID, PermissionPolicy* membershipAuthData)
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(serviceBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembership RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembership(serial, adminBus, clientProxyObject, adminBus, serviceGUID, &claimedPubKey, guildID, membershipAuthData);
        EXPECT_EQ(ER_OK, status) << "  InstallMembership cert1 failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembership(serial, adminBus, clientProxyObject, adminBus, serviceGUID, &claimedPubKey, guildID, membershipAuthData);
        EXPECT_NE(ER_OK, status) << "  InstallMembership cert1 again is supposed to fail.  Actual Status: " << QCC_StatusText(status);
    }

    void InstallMembershipToServiceProvider(PermissionPolicy* membershipAuthData)
    {
        InstallMembershipToServiceProvider(membershipSerial3, membershipGUID3, membershipAuthData);
    }

    /**
     *  RemoveMembership from service provider
     */
    void RemoveMembershipFromServiceProvider()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        EXPECT_EQ(ER_OK, status) << "  GetGuid failed.  Actual Status: " << QCC_StatusText(status);

        status = PermissionMgmtTestHelper::RemoveMembership(adminBus, clientProxyObject, membershipSerial3, issuerGUID);
        EXPECT_EQ(ER_OK, status) << "  RemoveMembershipFromServiceProvider failed.  Actual Status: " << QCC_StatusText(status);

        /* removing it again */
        status = PermissionMgmtTestHelper::RemoveMembership(adminBus, clientProxyObject, membershipSerial3, issuerGUID);
        EXPECT_NE(ER_OK, status) << "  RemoveMembershipFromServiceProvider succeeded.  Expect it to fail.  Actual Status: " << QCC_StatusText(status);

    }

    /**
     *  Install Membership to a consumer
     */
    void InstallMembershipToConsumer(const char* serial, qcc::GUID128& guildID, PermissionPolicy* membershipAuthData)
    {
        ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembership(serial, adminBus, clientProxyObject, adminBus, consumerGUID, &claimedPubKey, guildID, membershipAuthData);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer cert1 failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Install Membership to a consumer
     */
    void InstallMembershipToConsumer(PermissionPolicy* membershipAuthData)
    {
        InstallMembershipToConsumer(membershipSerial1, membershipGUID1, membershipAuthData);
    }

    /**
     *  Install Membership chain to a consumer
     */
    void InstallMembershipChainToConsumer(const char* serial, qcc::GUID128& guildID, PermissionPolicy** authDataArray)
    {
        ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipChainToConsumer RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembershipChain(serial, adminBus, clientProxyObject, adminBus, consumerBus, consumerGUID, &claimedPubKey, guildID, authDataArray);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipChainToConsumer failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Install Membership to a consumer
     */
    void InstallOthersMembershipToConsumer(PermissionPolicy* membershipAuthData)
    {
        ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(adminBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallOthersMembershipToConsumer RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembership(membershipSerial1, adminBus, clientProxyObject, adminBus, consumerGUID, &claimedPubKey, membershipGUID1, membershipAuthData);
        EXPECT_EQ(ER_OK, status) << "  InstallOthersMembershipToConsumer InstallMembership failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Install Membership to the admin
     */
    void InstallMembershipToAdmin(PermissionPolicy* membershipAuthData)
    {
        ProxyBusObject clientProxyObject(adminProxyBus, adminBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(adminBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToAdmin RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembership(membershipSerial1, adminProxyBus, clientProxyObject, adminBus, consumerGUID, &claimedPubKey, membershipGUID1, membershipAuthData);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToAdmin cert1 failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Test PermissionMgmt InstallGuildEquivalence method
     */
    void InstallGuildEquivalence()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        status = PermissionMgmtTestHelper::InstallGuildEquivalence(adminBus, clientProxyObject, sampleCertificatePEM);
        EXPECT_EQ(ER_OK, status) << "  InstallGuildEquivalence failed.  Actual Status: " << QCC_StatusText(status);

    }

    /**
     *  App can call On
     */
    void AppCanCallOn(BusAttachment& bus, BusAttachment& targetBus)
    {
        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(bus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AppCanCallOn ExcerciseOn failed.  Actual Status: " << QCC_StatusText(status);
        //bus.LeaveSession(clientProxyObject.GetSessionId());
    }

    /**
     *  App can't call On
     */
    void AppCannotCallOn(BusAttachment& bus, BusAttachment& targetBus)
    {
        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AppCannotCallOn ExcerciseOn did not fail.  Actual Status: " << QCC_StatusText(status);
        //bus.LeaveSession(clientProxyObject.GetSessionId());
    }

    /**
     *  any user can call TV On but not Off
     */
    void AnyUserCanCallOnAndNotOff(BusAttachment& bus)
    {

        ProxyBusObject clientProxyObject(bus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(bus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AnyUserCanCallOnAndNotOff ExcerciseOn failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseOff(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AnyUserCanCallOnAndNotOff ExcersizeOff did not fail.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  consumer can call TV On and Off
     */
    void ConsumerCanCallOnAndOff()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanCallOnAndOff ExcerciseOn failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseOff(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanCallOnAndOff ExcersizeOff failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  app can't call TV On
     */
    void AppCannotCallTVOn(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AppCannotCallTVOn ExcerciseOn should have failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  app can't call TV On
     */
    void AppCannotCallTVDown(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVDown(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AppCannotCallTVDown ExcerciseTVDown should have failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  app can't call TV On
     */
    void AppCanCallTVUp(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVUp(bus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AppCanCallTVUp ExcerciseTVUp failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  app can't call TV Off
     */
    void AppCannotCallTVOff(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOff(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AppCannotCallTVOff ExcerciseOff should have failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Consumer can't call TV On
     */
    void ConsumerCannotCallTVOn()
    {
        AppCannotCallTVOn(consumerBus, serviceBus);
    }

    /**
     *  Consumer can't call TV InputSource
     */
    void ConsumerCannotCallTVInputSource()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVInputSource(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  ConsumerCannotCallTVInputSource ExcerciseTVInputSource should have failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  App gets the PermissionMgmt version number
     */
    void AppGetVersionNumber(BusAttachment& bus, BusAttachment& targetBus)
    {
        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        uint16_t versionNum = 0;
        status = PermissionMgmtTestHelper::GetPermissionMgmtVersion(bus, clientProxyObject, versionNum);
        EXPECT_EQ(ER_OK, status) << "  AppGetVersionNumber GetPermissionMgmtVersion failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(1, versionNum) << "  AppGetVersionNumber received unexpected version number.";
    }

    /**
     *  App can call TV Off
     */
    void AppCanCallTVOff(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOff(bus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AppCanCallTVOff ExcerciseOff failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Consumer can call TV Off
     */
    void ConsumerCanCallTVOff()
    {
        AppCanCallTVOff(consumerBus, serviceBus);
    }

    /**
     *  Guild member can turn up/down but can't specify a channel
     */
    void ConsumerCanTVUpAndDownAndNotChannel()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVUp(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanTVUpAndDownAndNotChannel ExcerciseTVUp failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseTVDown(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanTVUpAndDownAndNotChannel ExcerciseTVDown failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseTVChannel(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  ConsumerCanTVUpAndDownAndNotChannel ExcerciseTVChannel did not fail.  Actual Status: " << QCC_StatusText(status);

        uint32_t tvVolume = 35;
        status = PermissionMgmtTestHelper::SetTVVolume(consumerBus, clientProxyObject, tvVolume);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanTVUpAndDownAndNotChannel SetTVVolume failed.  Actual Status: " << QCC_StatusText(status);
        uint32_t newTvVolume = 0;
        status = PermissionMgmtTestHelper::GetTVVolume(consumerBus, clientProxyObject, newTvVolume);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanTVUpAndDownAndNotChannel GetTVVolume failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(newTvVolume, tvVolume) << "  ConsumerCanTVUpAndDownAndNotChannel GetTVVolume got wrong TV volume.";
    }

    /**
     *  consumer cannot turn TV up
     */
    void ConsumerCannotTurnTVUp()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVUp(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  ConsumerCannotTurnTVUp ExcerciseTVUp failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  consumer cannot get the TV caption
     */
    void ConsumerCannotGetTVCaption()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::GetTVCaption(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  ConsumerCannotGetTVCaption GetTVCaption did not fail.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  consumer can get the TV caption
     */
    void ConsumerCanGetTVCaption()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::GetTVCaption(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCannotGetTVCaption GetTVCaption failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Admin can change channel
     */
    void AdminCanChangeChannlel()
    {

        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        status = PermissionMgmtTestHelper::ExcerciseTVChannel(adminBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AdminCanChangeChannlel failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  consumer can change channel
     */
    void ConsumerCanChangeChannlel()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        status = PermissionMgmtTestHelper::ExcerciseTVChannel(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanChangeChannlel failed.  Actual Status: " << QCC_StatusText(status);
    }

    /*
     *  Set the manifest for the service provider
     */
    void SetPermissionManifestOnServiceProvider()
    {

        PermissionPolicy::Rule* rules = NULL;
        size_t count = 0;
        QStatus status = GenerateManifest(&rules, &count);
        EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest GenerateManifest failed.  Actual Status: " << QCC_StatusText(status);
        PermissionConfigurator& pc = serviceBus.GetPermissionConfigurator();
        status = pc.SetPermissionManifest(rules, count);
        EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest SetPermissionManifest failed.  Actual Status: " << QCC_StatusText(status);

        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        PermissionPolicy::Rule* retrievedRules = NULL;
        size_t retrievedCount = 0;
        status = PermissionMgmtTestHelper::GetManifest(consumerBus, clientProxyObject, &retrievedRules, &retrievedCount);
        EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest GetManifest failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(count, retrievedCount) << "  SetPermissionManifest GetManifest failed to retrieve the same count.";
        delete [] rules;
        delete [] retrievedRules;
    }

    /*
     *  Remove Policy from service provider
     */
    void RemovePolicyFromServiceProvider()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        /* remove the policy */
        SetNotifyConfigSignalReceived(false);
        status = PermissionMgmtTestHelper::RemovePolicy(adminBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  RemovePolicy failed.  Actual Status: " << QCC_StatusText(status);

        /* get policy again.  Expect it to fail */
        PermissionPolicy retPolicy;
        status = PermissionMgmtTestHelper::GetPolicy(adminBus, clientProxyObject, retPolicy);
        EXPECT_NE(ER_OK, status) << "  GetPolicy did not fail.  Actual Status: " << QCC_StatusText(status);
        /* sleep a second to see whether the NotifyConfig signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetNotifyConfigSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
    }

    /*
     * Remove Membership from consumer.
     */
    void RemoveMembershipFromConsumer()
    {
        ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        status = PermissionMgmtTestHelper::RemoveMembership(adminBus, clientProxyObject, membershipSerial1, issuerGUID);
        EXPECT_EQ(ER_OK, status) << "  RemoveMembershipFromConsumer failed.  Actual Status: " << QCC_StatusText(status);

        /* removing it again */
        status = PermissionMgmtTestHelper::RemoveMembership(adminBus, clientProxyObject, membershipSerial1, issuerGUID);
        EXPECT_NE(ER_OK, status) << "  RemoveMembershipFromConsumer succeeded.  Expect it to fail.  Actual Status: " << QCC_StatusText(status);

    }

    /**
     * Test PermissionMgmt Reset method on service.  The consumer should not be
     * able to reset the service since the consumer is not an admin.
     */
    void FailResetServiceByConsumer()
    {
        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        status = PermissionMgmtTestHelper::Reset(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  Reset is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

    }

    /*
     * Test PermissionMgmt Reset method on service by the admin.  The admin should be
     * able to reset the service.
     */
    void SuccessfulResetServiceByAdmin()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        status = PermissionMgmtTestHelper::Reset(adminBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        /* retrieve the current identity cert */
        IdentityCertificate cert;
        status = PermissionMgmtTestHelper::GetIdentity(adminBus, clientProxyObject, cert);
        EXPECT_NE(ER_OK, status) << "  GetIdentity is not supposed to succeed since it was removed by Reset.  Actual Status: " << QCC_StatusText(status);

    }

    /*
     * retrieve the peer public key.
     */
    void RetrieveServicePublicKey()
    {
        PermissionConfigurator& pc = consumerBus.GetPermissionConfigurator();
        GUID128 serviceGUID(0);
        String peerName = serviceBus.GetUniqueName();
        status = PermissionMgmtTestHelper::GetPeerGUID(consumerBus, peerName, serviceGUID);
        EXPECT_EQ(ER_OK, status) << "  ca.GetPeerGuid failed.  Actual Status: " << QCC_StatusText(status);
        ECCPublicKey publicKey;
        status = pc.GetConnectedPeerPublicKey(serviceGUID, &publicKey);
        EXPECT_EQ(ER_OK, status) << "  GetConnectedPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
    }

    /*
     * retrieve the peer public key.
     */
    void ClearPeerKeys(BusAttachment& bus, BusAttachment& peerBus)
    {
        String peerName = peerBus.GetUniqueName();
        GUID128 peerGUID(0);
        QStatus status = PermissionMgmtTestHelper::GetPeerGUID(bus, peerName, peerGUID);
        EXPECT_EQ(ER_OK, status) << "  PermissionMgmtTestHelper::GetPeerGuid failed.  Actual Status: " << QCC_StatusText(status);
        status = bus.ClearKeys(peerGUID.ToString());
        EXPECT_EQ(ER_OK, status) << "  BusAttachment::ClearKeys failed.  Actual Status: " << QCC_StatusText(status);
    }
};

class PathBasePermissionMgmtUseCaseTest : public PermissionMgmtUseCaseTest {
  public:
    PathBasePermissionMgmtUseCaseTest() : PermissionMgmtUseCaseTest("/control/guide")
    {
    }
};

/*
 *  Test all the possible calls provided PermissionMgmt interface
 */
TEST_F(PermissionMgmtUseCaseTest, TestAllCalls)
{
    Claims(false);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    ReplaceServiceIdentityCert();
    policy = GenerateAuthDataProvideSignal();
    InstallMembershipToServiceProvider(policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(policy);
    delete policy;
    InstallGuildEquivalence();

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    SetChannelChangedSignalReceived(false);
    ConsumerCanTVUpAndDownAndNotChannel();
    ConsumerCanGetTVCaption();
    /* sleep a second to see whether the ChannelChanged signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetChannelChangedSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetChannelChangedSignalReceived()) << " Fail to receive expected ChannelChanged signal.";

    SetPermissionManifestOnServiceProvider();

    RetrieveServicePublicKey();
    RemoveMembershipFromServiceProvider();
    RemovePolicyFromServiceProvider();
    RemoveMembershipFromConsumer();
    FailResetServiceByConsumer();
    SuccessfulResetServiceByAdmin();
    AppGetVersionNumber(consumerBus, serviceBus);
}

/*
 *  Case: claiming, install policy, install membership, and access
 */
TEST_F(PermissionMgmtUseCaseTest, ClaimPolicyMembershipAccess)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData(&membershipGUID1);
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCanTVUpAndDownAndNotChannel();
    SetPermissionManifestOnServiceProvider();

}

/*
 *  Case: outbound message allowed by guild based terms and peer's membership
 */
TEST_F(PathBasePermissionMgmtUseCaseTest, OutboundAllowedByMembership)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    policy = GenerateGuildSpecificAccessProviderAuthData(membershipGUID1, guildAuthorityPubKey);
    InstallMembershipToServiceProvider("1234", membershipGUID1, policy);
    delete policy;

    policy = GenerateGuildSpecificAccessOutgoingPolicy(membershipGUID1, guildAuthorityPubKey);
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCanTVUpAndDownAndNotChannel();
}

/*
 *  Case: outbound message not allowed by guild based terms since
 *       peer does not have given guild membership.
 */
TEST_F(PermissionMgmtUseCaseTest, OutboundNotAllowedByMissingPeerMembership)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateGuildSpecificAccessOutgoingPolicy(membershipGUID1, guildAuthorityPubKey);
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCannotTurnTVUp();

}

/*
 *  Service Provider has no policy: claiming, access
 */
TEST_F(PermissionMgmtUseCaseTest, ClaimNoPolicyAccess)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    PermissionPolicy* authData = GenerateMembershipAuthData();
    InstallMembershipToConsumer(authData);
    delete authData;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCannotCallTVOn();
}

/*
 *  Access granted for peer public key
 */
TEST_F(PermissionMgmtUseCaseTest, AccessByPublicKey)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey consumerPublicKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &consumerPublicKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicyPeerPublicKey(issuerGUID, issuerPubKey, consumerPublicKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanTVUpAndDownAndNotChannel();
}

/*
 *  Access denied for peer public key
 */
TEST_F(PermissionMgmtUseCaseTest, AccessDeniedForPeerPublicKey)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey consumerPublicKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &consumerPublicKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicyDenyPeerPublicKey(issuerGUID, issuerPubKey, consumerPublicKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AppCanCallTVUp(consumerBus, serviceBus);
    AppCannotCallTVDown(consumerBus, serviceBus);
}

/*
 *  Case: admin has full access after claim
 */
TEST_F(PermissionMgmtUseCaseTest, AdminHasFullAccess)
{
    Claims(true);

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(adminBus, false);

    AdminCanChangeChannlel();
}

/*
 *  Case: unclaimed app does not have restriction
 */
TEST_F(PermissionMgmtUseCaseTest, UnclaimedProviderAllowsEverything)
{
    EnableSecurity("ALLJOYN_ECDHE_PSK");

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanChangeChannlel();
}

/*
 *  Case: install identity cert with different subject public key from target's
 */
TEST_F(PermissionMgmtUseCaseTest, InstallIdentityCertWithDifferentPubKey)
{
    Claims(false);
    ReplaceServiceIdentityCertWithBadPublicKey();
}

/*
 *  Case: install identity cert with expired cert
 */
TEST_F(PermissionMgmtUseCaseTest, InstallIdentityCertWithExpiredCert)
{
    Claims(false);
    ReplaceIdentityCertWithExpiredCert(adminBus, consumerBus);
}

/*
 *  Case: claiming, install policy, install wrong membership, and fail access
 */
TEST_F(PermissionMgmtUseCaseTest, SendingOthersMembershipCert)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallOthersMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCannotTurnTVUp();
}

/*
 *  Case: claiming, install limited policy, and fail access because of no
 *  matching action mask.
 */
TEST_F(PermissionMgmtUseCaseTest, AccessNotAuthorizedBecauseOfWrongActionMask)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a limited policy */
    PermissionPolicy* policy = GenerateSmallAnyUserPolicy(issuerGUID, issuerPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
}

/*
 *  Case: claiming, install limited policy with a denied in a prefix match, and fail access
 */
TEST_F(PermissionMgmtUseCaseTest, AccessNotAuthorizedBecauseOfDeniedOnPrefix)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a limited policy */
    PermissionPolicy* policy = GenerateAnyUserDeniedPrefixPolicy(issuerGUID, issuerPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
}

/*
 *  Case: provider has no matching guild terms for consumer
 */
TEST_F(PathBasePermissionMgmtUseCaseTest, NoMatchGuildForConsumer)
{
    Claims(false);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    policy = GenerateAuthDataProvideSignal();
    InstallMembershipToServiceProvider(policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(membershipSerial4, membershipGUID4, policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCannotTurnTVUp();
}

/*
 *  Case: provider has more membership certs than consumer
 */
TEST_F(PermissionMgmtUseCaseTest, ProviderHasMoreMembershipCertsThanConsumer)
{
    Claims(false);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    policy = GenerateAuthDataProvideSignal();
    InstallMembershipToServiceProvider(policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCanTVUpAndDownAndNotChannel();

}

/*
 *  Case: consumer has more membership certs than provider
 */
TEST_F(PermissionMgmtUseCaseTest, ConsumerHasMoreMembershipCertsThanService)
{
    Claims(false);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    policy = GenerateAuthDataProvideSignal();
    InstallMembershipToServiceProvider(policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(policy);
    InstallMembershipToConsumer(membershipSerial2, membershipGUID2, policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanCallTVOff();
}

/*
 *  Case: consumer has a valid membeship cert chain
 */
TEST_F(PermissionMgmtUseCaseTest, ConsumerHasGoodMembershipCertChain)
{
    Claims(false);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    policy = GenerateAuthDataProvideSignal();
    InstallMembershipToServiceProvider(policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    PermissionPolicy** authDataArray = new PermissionPolicy * [2];
    GenerateMembershipAuthChain(authDataArray, 2);
    InstallMembershipChainToConsumer(membershipSerial1, membershipGUID1, authDataArray);
    for (size_t cnt = 0; cnt < 2; cnt++) {
        delete authDataArray[cnt];
    }
    delete [] authDataArray;

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanTVUpAndDownAndNotChannel();
}

/*
 *  Case: consumer has an overreaching membeship cert chain
 */
TEST_F(PermissionMgmtUseCaseTest, ConsumerHasOverreachingMembershipCertChain)
{
    Claims(false);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    policy = GenerateAuthDataProvideSignal();
    InstallMembershipToServiceProvider(policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    PermissionPolicy** authDataArray = new PermissionPolicy * [2];
    GenerateOverReachingMembershipAuthChain(authDataArray, 2);
    InstallMembershipChainToConsumer(membershipSerial1, membershipGUID1, authDataArray);
    for (size_t cnt = 0; cnt < 2; cnt++) {
        delete authDataArray[cnt];
    }
    delete [] authDataArray;

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    QCC_UseOSLogging(true);
    ConsumerCanTVUpAndDownAndNotChannel();
    ConsumerCannotCallTVInputSource();
}

/*
 *  Case: provider allow access for the guild but the consumer membership auth does not allow
 */
TEST_F(PathBasePermissionMgmtUseCaseTest, ConsumerHasLessAccessInMembershipUsingDenied)
{
    Claims(false);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    policy = GenerateAuthDataProvideSignal();
    InstallMembershipToServiceProvider(policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateLesserMembershipAuthData(true, NULL);
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCannotTurnTVUp();
    ConsumerCannotGetTVCaption();
}

/*
 *  Case: provider allow access for the guild but the consumer membership auth does not allow
 */
TEST_F(PathBasePermissionMgmtUseCaseTest, ConsumerHasLessAccessInMembershipUsingEmptyAuthMask)
{
    Claims(false);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    policy = GenerateAuthDataProvideSignal();
    InstallMembershipToServiceProvider(policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateLesserMembershipAuthData(false, NULL);
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCannotTurnTVUp();
    ConsumerCannotGetTVCaption();
}

/*
 *  Case: claiming, install limited policy, and fail access because of no
 *  matching action mask.
 */
TEST_F(PermissionMgmtUseCaseTest, AllowEverything)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a limited policy */
    PermissionPolicy* policy = GenerateWildCardPolicy(issuerGUID, issuerPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData();
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanCallOnAndOff();
}

/*
 *  Case: multiple trust anchors in the local network
 */
TEST_F(PermissionMgmtUseCaseTest, TwoTrustAnchors)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData(&membershipGUID1);
    InstallMembershipToConsumer(policy);
    delete policy;

    InstallAdditionalIdentityTrustAnchor(adminBus, consumerBus, serviceBus, true);
    InstallAdditionalIdentityTrustAnchor(consumerBus, adminBus, remoteControlBus);

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToClientBus(consumerBus, remoteControlBus, *policy);
    delete policy;

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);
    CreateAppInterfaces(remoteControlBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    AnyUserCanCallOnAndNotOff(remoteControlBus);
    ConsumerCanTVUpAndDownAndNotChannel();

}

/*
 *  Case: add and delete identity trust anchors
 */
TEST_F(PermissionMgmtUseCaseTest, AddDeleteIdentityTrustAnchors)
{
    Claims(true);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToConsumer(*policy);
    delete policy;

    policy = GenerateMembershipAuthData(&membershipGUID1);
    InstallMembershipToConsumer(policy);
    delete policy;

    /* install additional crentials on service and remote control so they
       can authenticate each other */
    InstallAdditionalIdentityTrustAnchor(adminBus, consumerBus, serviceBus);
    InstallAdditionalIdentityTrustAnchor(consumerBus, adminBus, remoteControlBus);

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToClientBus(consumerBus, remoteControlBus, *policy);
    delete policy;

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);
    CreateAppInterfaces(remoteControlBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    AppCanCallOn(remoteControlBus, serviceBus);

    /* remove the identity trust anchor from service */
    RemoveIdentityTrustAnchor(adminBus, consumerBus, serviceBus);

    /* now need to prove that the remote control bus can't access the service */
    /* since the remote control already have the master secret with the service. it needs to be cleared first and re-enable ECDSA key exchange */
    ClearPeerKeys(remoteControlBus, serviceBus);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA");
    AppCannotCallTVOn(remoteControlBus, serviceBus);
}

/*
 *  Case: different peer level in ANY-USER policy
 */
TEST_F(PermissionMgmtUseCaseTest, DifferentPeerLevelsInAnyUserPolicy)
{
    /* claims the apps with the exception of the remote control app */
    Claims(true, false);
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    EnableSecurity("ALLJOYN_ECDHE_NULL");

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(remoteControlBus, false);

    /* service has no policy so expect to fail */
    AppCannotCallOn(remoteControlBus, serviceBus);

    /* generate a policy for service */
    EnableSecurity("ALLJOYN_ECDHE_ECDSA");
    PermissionPolicy* policy = GenerateAnyUserPolicyWithLevel(issuerGUID, issuerPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    EnableSecurity("ALLJOYN_ECDHE_NULL");

    /* the unauthenticated remote control can turn on the TV */
    AppCanCallOn(remoteControlBus, serviceBus);

    /* since the remote control is not authenticated, expect the Off call fails since it requires authenticated peer */
    AppCannotCallTVOff(remoteControlBus, serviceBus);

    /* Claims the remote so it can participate in privilege calls */
    ConsumerClaimsRemoteControl();
    EnableSecurity("ALLJOYN_ECDHE_ECDSA");

    policy = GenerateFullAccessOutgoingPolicy();
    InstallPolicyToClientBus(consumerBus, remoteControlBus, *policy);
    delete policy;

    /* install additional crentials on service and remote control so they
       can authenticate each other */
    InstallAdditionalIdentityTrustAnchor(adminBus, consumerBus, serviceBus);
    InstallAdditionalIdentityTrustAnchor(consumerBus, adminBus, remoteControlBus);
    AppCanCallTVOff(remoteControlBus, serviceBus);
}
