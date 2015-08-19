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

#include "PermissionMgmtTest.h"
#include "KeyInfoHelper.h"
#include "KeyExchanger.h"
#include <qcc/Crypto.h>
#include <qcc/Util.h>
#include <string>

using namespace ajn;
using namespace qcc;

static GUID128 membershipGUID1(1);
static const char* membershipSerial0 = "10000";
static const char* membershipSerial1 = "10001";
static GUID128 membershipGUID2(2);
static GUID128 membershipGUID3(3);
static GUID128 membershipGUID4(4);
static const char* membershipSerial2 = "20002";
static const char* membershipSerial3 = "30003";
static const char* membershipSerial4 = "40004";

static const char* adminMembershipSerial1 = "900001";
static const char* adminMembershipSerial2 = "900002";

static QStatus AddAclsToDefaultPolicy(PermissionPolicy& defaultPolicy, PermissionPolicy::Acl* acls, size_t count, bool keepCAentry, bool keepAdminGroupEntry, bool keepInstallMembershipEntry, bool keepAnyTrusted)
{
    size_t newCount = count;
    /** The default policy has 4 entries:
     *       The FROM_CERTIFICATE_AUTHORITY entry for the certificate authority
     *       The WITH_MEMBERSHIP entry for the admin group
     *       The WITH_PUBLIC_KEY entry for allowing the application to
     *                  self-install membership certificates
     *       The ANY_TRUSTED entry for allowing the application to send
     *                  outbound messages
     * Any of these entries can be copied into the new policy.
     */
    if (keepCAentry) {
        ++newCount;
    }
    if (keepAdminGroupEntry) {
        ++newCount;
    }
    if (keepInstallMembershipEntry) {
        ++newCount;
    }
    if (keepAnyTrusted) {
        ++newCount;
    }

    PermissionPolicy::Acl* newAcls = new PermissionPolicy::Acl[newCount];
    size_t idx = 0;
    for (size_t cnt = 0; cnt < defaultPolicy.GetAclsSize(); ++cnt) {
        assert(idx < newCount);
        if (defaultPolicy.GetAcls()[cnt].GetPeersSize() > 0) {
            if (defaultPolicy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY) {
                if (keepCAentry) {
                    newAcls[idx++] = defaultPolicy.GetAcls()[cnt];
                }
            } else if (defaultPolicy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) {
                if (keepAdminGroupEntry) {
                    newAcls[idx++] = defaultPolicy.GetAcls()[cnt];
                }
            } else if (defaultPolicy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY) {
                if (keepInstallMembershipEntry) {
                    newAcls[idx++] = defaultPolicy.GetAcls()[cnt];
                }
            } else if (defaultPolicy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_ANY_TRUSTED) {
                if (keepAnyTrusted) {
                    newAcls[idx++] = defaultPolicy.GetAcls()[cnt];
                }
            }
        }
    }
    for (size_t cnt = 0; cnt < count; ++cnt) {
        assert(idx < newCount);
        newAcls[idx++] = acls[cnt];
    }
    defaultPolicy.SetAcls(newCount, newAcls);
    delete [] newAcls;
    return ER_OK;
}

static QStatus AddAcls(PermissionPolicy& policy, PermissionPolicy::Acl* acls, size_t count)
{
    if (count == 0) {
        return ER_OK;
    }
    if (policy.GetAclsSize() == 0) {
        policy.SetAcls(count, acls);
        return ER_OK;
    }
    size_t policyAclsSize = policy.GetAclsSize();
    size_t newSize = policyAclsSize + count;
    PermissionPolicy::Acl* newAcls = new PermissionPolicy::Acl[newSize];
    if (newAcls == NULL) {
        return ER_OUT_OF_MEMORY;
    }
    for (size_t cnt = 0; cnt < policyAclsSize; cnt++) {
        newAcls[cnt] = policy.GetAcls()[cnt];
    }
    for (size_t cnt = 0; cnt < count; cnt++) {
        newAcls[cnt + policyAclsSize] = acls[cnt];
    }
    policy.SetAcls(newSize, newAcls);
    delete [] newAcls;
    return ER_OK;
}

static QStatus GenerateWildCardPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(20);

    /* add the acls section */
    PermissionPolicy::Acl acls[1];
    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("org.allseenalliance.control.*");
    PermissionPolicy::Rule::Member prms[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(3, prms);
    acls[0].SetRules(1, rules);

    return AddAcls(policy, acls, ArraySize(acls));
}

static QStatus GenerateAllowAllPeersPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(21);

    /* add the acls section */
    PermissionPolicy::Acl acls[2];
    /* acls record 0  ALL users including anonymous */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
        acls[0].SetPeers(1, peers);
        PermissionPolicy::Rule rules[1];
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("On");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(1, prms);
        acls[0].SetRules(1, rules);
    }
    /* acls record 1  ANY-USER */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
        acls[1].SetPeers(1, peers);
        PermissionPolicy::Rule rules[1];
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("Off");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(1, prms);
        acls[1].SetRules(1, rules);
    }

    return AddAcls(policy, acls, ArraySize(acls));
}

static QStatus GenerateNoOutboundGetPropertyPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(22);

    /* add the acls section */
    PermissionPolicy::Acl acls[1];
    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(ArraySize(peers), peers);
    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("org.allseenalliance.control.*");
    PermissionPolicy::Rule::Member prms[1];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(ArraySize(prms), prms);
    acls[0].SetRules(ArraySize(rules), rules);

    return AddAclsToDefaultPolicy(policy, acls, ArraySize(acls), true, true, false, false);
}

static QStatus GenerateGetAllPropertiesPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy, uint8_t actionMask)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(23);

    /* add the acls section */
    PermissionPolicy::Acl acls[1];
    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(ArraySize(peers), peers);
    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("org.allseenalliance.control.*");
    PermissionPolicy::Rule::Member prms[1];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[0].SetActionMask(actionMask);
    rules[0].SetMembers(ArraySize(prms), prms);
    acls[0].SetRules(ArraySize(rules), rules);

    return AddAclsToDefaultPolicy(policy, acls, ArraySize(acls), true, true, false, false);
}

static QStatus GenerateGetAllPropertiesProvidePolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy)
{
    return GenerateGetAllPropertiesPolicy(bus, targetBus, policy, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
}

static QStatus GenerateGetAllPropertiesObservePolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy)
{
    return GenerateGetAllPropertiesPolicy(bus, targetBus, policy, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
}

/**
 * Add ACLs to existing default policy of the target bus.
 * @param bus the admin bus
 * @param targetBus the target bus
 * @param[in,out] policy the policy
 * @param guildAuthorityBus the bus to retrieve the key info for a membership entry
 */
static QStatus GeneratePolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy, BusAttachment& guildAuthorityBus)
{
    qcc::GUID128 guildAuthorityGUID;
    PermissionMgmtTestHelper::GetGUID(guildAuthorityBus, guildAuthorityGUID);
    ECCPublicKey guildAuthorityPubKey;
    QStatus status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(guildAuthorityBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);

    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(2);

    /* add the acls section */
    PermissionPolicy::Acl acls[4];
    /* acls record 0  ANY-USER */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
        acls[0].SetPeers(1, peers);
        PermissionPolicy::Rule rules[3];
        rules[0].SetObjPath("/control/guide");
        rules[0].SetInterfaceName("allseenalliance.control.*");
        {
            PermissionPolicy::Rule::Member prms[1];
            prms[0].SetMemberName("*");
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            rules[0].SetMembers(1, prms);
        }
        rules[1].SetObjPath("*");
        rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
        {
            PermissionPolicy::Rule::Member prms[2];
            prms[0].SetMemberName("Off");
            prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);  /* setting OBSERVE in fact does not allow the method call to be executed */
            prms[1].SetMemberName("On");
            prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            rules[1].SetMembers(2, prms);
        }
        rules[2].SetObjPath("*");
        rules[2].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
        {
            PermissionPolicy::Rule::Member prms[1];
            prms[0].SetMemberName("ChannelChanged");
            prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            rules[2].SetMembers(1, prms);
        }
        acls[0].SetRules(3, rules);
    }
    /* acls record 1 GUILD membershipGUID1 */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
        peers[0].SetSecurityGroupId(membershipGUID1);
        KeyInfoNISTP256 keyInfo;
        keyInfo.SetKeyId(guildAuthorityGUID.GetBytes(), qcc::GUID128::SIZE);
        keyInfo.SetPublicKey(&guildAuthorityPubKey);
        peers[0].SetKeyInfo(&keyInfo);
        acls[1].SetPeers(1, peers);
        PermissionPolicy::Rule rules[2];
        {
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
            PermissionPolicy::Rule::Member prms[5];
            prms[0].SetMemberName("Up");
            prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            prms[1].SetMemberName("Down");
            prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
            prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            prms[2].SetMemberName("Volume");
            prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
            prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
            prms[3].SetMemberName("InputSource");
            prms[3].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
            prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            prms[4].SetMemberName("Caption");
            prms[4].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
            prms[4].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
            rules[0].SetMembers(5, prms);
        }
        {
            rules[1].SetObjPath("*");
            rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
            PermissionPolicy::Rule::Member prms[1];
            prms[0].SetMemberName("*");
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            rules[1].SetMembers(1, prms);
        }
        acls[1].SetRules(2, rules);
    }
    /* acls record 2 GUILD membershipGUID2 */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
        peers[0].SetSecurityGroupId(membershipGUID2);
        KeyInfoNISTP256 keyInfo;
        keyInfo.SetKeyId(guildAuthorityGUID.GetBytes(), qcc::GUID128::SIZE);
        keyInfo.SetPublicKey(&guildAuthorityPubKey);
        peers[0].SetKeyInfo(&keyInfo);
        acls[2].SetPeers(1, peers);
        PermissionPolicy::Rule rules[3];
        {
            rules[0].SetObjPath("/control/settings");
            rules[0].SetInterfaceName("*");
            PermissionPolicy::Rule::Member prms[1];
            prms[0].SetMemberName("*");
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);  /* only allow read */
            rules[0].SetMembers(1, prms);
        }
        {
            rules[1].SetObjPath("/control/guide");
            rules[1].SetInterfaceName("org.allseenalliance.control.*");
            PermissionPolicy::Rule::Member prms[1];
            prms[0].SetMemberName("*");
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            rules[1].SetMembers(1, prms);
        }
        {
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            PermissionPolicy::Rule::Member prms[1];
            prms[0].SetMemberName("*");
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            rules[2].SetMembers(1, prms);
        }
        acls[2].SetRules(3, rules);
    }
    /* acls record 3 peer specific rule  */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
        KeyInfoNISTP256 keyInfo;
        keyInfo.SetKeyId(guildAuthorityGUID.GetBytes(), qcc::GUID128::SIZE);
        keyInfo.SetPublicKey(&guildAuthorityPubKey);
        peers[0].SetKeyInfo(&keyInfo);
        acls[3].SetPeers(1, peers);
        PermissionPolicy::Rule rules[1];
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("Mute");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(1, prms);
        acls[3].SetRules(1, rules);
    }
    return AddAcls(policy, acls, ArraySize(acls));
}

static void AddSpecificCertAuthorityToPolicy(PermissionPolicy& policy, const KeyInfoNISTP256& restrictedCA)
{
    policy.SetVersion(policy.GetVersion() + 1);

    /* add one more acl */
    size_t newAclsSize = policy.GetAclsSize() + 1;
    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[newAclsSize];
    for (size_t cnt = 0; cnt < policy.GetAclsSize(); cnt++) {
        acls[cnt] = policy.GetAcls()[cnt];
    }
    /* acls record FROM_CERTIFICATE_AUTHORITY */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
        KeyInfoNISTP256 keyInfo(restrictedCA);
        peers[0].SetKeyInfo(&keyInfo);
        acls[newAclsSize - 1].SetPeers(1, peers);
        PermissionPolicy::Rule rules[2];
        {
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
            PermissionPolicy::Rule::Member prms[5];
            prms[0].SetMemberName("Up");
            prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            prms[1].SetMemberName("Down");
            prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
            prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            prms[2].SetMemberName("Volume");
            prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
            prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
            prms[3].SetMemberName("InputSource");
            prms[3].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
            prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            prms[4].SetMemberName("Caption");
            prms[4].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
            prms[4].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            rules[0].SetMembers(5, prms);
        }
        {
            rules[1].SetObjPath("*");
            rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
            PermissionPolicy::Rule::Member prms[1];
            prms[0].SetMemberName("*");
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            rules[1].SetMembers(1, prms);
        }
        acls[newAclsSize - 1].SetRules(2, rules);
    }
    policy.SetAcls(newAclsSize, acls);
    delete [] acls;
}

static QStatus GenerateSmallAnyUserPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(3);

    /* add more acls */
    PermissionPolicy::Acl acls[1];

    /* acls record 0  ANY-USER */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
        acls[0].SetPeers(1, peers);
        PermissionPolicy::Rule rules[2];
        {
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
            PermissionPolicy::Rule::Member prms[2];
            prms[0].SetMemberName("Off");
            prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
            prms[1].SetMemberName("On");
            prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
            prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
            rules[0].SetMembers(2, prms);
        }
        {
            rules[1].SetObjPath("*");
            rules[1].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
            PermissionPolicy::Rule::Member prms[1];
            prms[0].SetMemberName("ChannelChanged");
            prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            rules[1].SetMembers(1, prms);
        }
        acls[0].SetRules(2, rules);
    }
    return AddAcls(policy, acls, ArraySize(acls));
}

static QStatus ReplaceAnyTrustAcl(PermissionPolicy& policy, PermissionPolicy::Acl& anyTrustedAcl)
{
    /* copy all acl entries from the default policy except the ANY_TRUSTED entry*/
    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[policy.GetAclsSize()];
    int anyTrustedIdx = -1;
    for (size_t cnt = 0; cnt < policy.GetAclsSize(); cnt++) {
        if (policy.GetAcls()[cnt].GetPeersSize() > 0) {
            if (policy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_ANY_TRUSTED) {
                anyTrustedIdx = cnt;
            }
        }
        if (anyTrustedIdx == -1) {
            acls[cnt] = policy.GetAcls()[cnt];
        }
    }
    if (anyTrustedIdx == -1) {
        delete [] acls;
        return ER_FAIL;
    }

    /* replacing the ANY_TRUSTED acl */
    acls[anyTrustedIdx] = anyTrustedAcl;
    policy.SetAcls(policy.GetAclsSize(), acls);
    delete [] acls;
    return ER_OK;
}

static QStatus GenerateFullAccessAnyUserPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy, bool allowSignal)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(4);
    PermissionPolicy::Acl acl;

    /* replacing the ANY_TRUSTED acl */
    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acl.SetPeers(1, peers);
    PermissionPolicy::Rule rules[2];
    {
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
        PermissionPolicy::Rule::Member prms[2];
        prms[0].SetMemberName("Off");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        prms[1].SetMemberName("On");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(2, prms);
    }
    {
        rules[1].SetObjPath("*");
        rules[1].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
        size_t count = 0;
        PermissionPolicy::Rule::Member* prms = NULL;
        if (allowSignal) {
            prms = new PermissionPolicy::Rule::Member[2];
            prms[count].SetMemberName("ChannelChanged");
            prms[count].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
            prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            count++;
        } else {
            prms = new PermissionPolicy::Rule::Member[1];
        }
        prms[count].SetMemberName("*");
        prms[count].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[1].SetMembers(count + 1, prms);
        delete [] prms;
    }
    acl.SetRules(2, rules);
    return ReplaceAnyTrustAcl(policy, acl);
}

static QStatus GenerateAnyUserDeniedPrefixPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(5);

    PermissionPolicy::Acl acl;

    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acl.SetPeers(1, peers);
    PermissionPolicy::Rule rules[2];
    {
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
        PermissionPolicy::Rule::Member prms[2];
        prms[0].SetMemberName("Of*");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE); /* disallow execution */
        prms[1].SetMemberName("On");
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(2, prms);
    }
    {
        rules[1].SetObjPath("*");
        rules[1].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("ChannelChanged");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        rules[1].SetMembers(1, prms);
    }
    acl.SetRules(2, rules);
    return ReplaceAnyTrustAcl(policy, acl);
}

static QStatus GenerateFullAccessOutgoingPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy, bool allowIncomingSignal)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(6);
    PermissionPolicy::Acl acl;

    /* acls record 0  ANY TRUSTED */
    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acl.SetPeers(1, peers);
    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member* prms = NULL;
    size_t count = 0;
    if (allowIncomingSignal) {
        prms = new PermissionPolicy::Rule::Member[3];
        prms[count].SetMemberName("*");
        prms[count].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
        prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        count++;
    } else {
        prms = new PermissionPolicy::Rule::Member[2];
    }
    prms[count].SetMemberName("*");
    prms[count].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    count++;
    prms[count].SetMemberName("*");
    prms[count].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    count++;
    rules[0].SetMembers(count, prms);
    acl.SetRules(1, rules);
    delete [] prms;
    return ReplaceAnyTrustAcl(policy, acl);
}

static QStatus GenerateFullAnonymousAccessOutgoingPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(7);
    PermissionPolicy::Acl acls[1];

    /* acls record 0  ALL */
    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule rules[1];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member prms[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    rules[0].SetMembers(1, prms);
    acls[0].SetRules(1, rules);

    return AddAcls(policy, acls, ArraySize(acls));
}

static QStatus GenerateFullAccessOutgoingPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy)
{
    return GenerateFullAccessOutgoingPolicy(bus, targetBus, policy, true);
}

static QStatus GenerateFullAccessOutgoingPolicyWithGuestServices(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy, bool allowIncomingSignal, const KeyInfoNISTP256& guestCA)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(8);

    PermissionPolicy::Acl acls[2];

    /* acls record 0  ANY-USER */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
        acls[0].SetPeers(1, peers);
        PermissionPolicy::Rule rules[1];
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName("*");
        PermissionPolicy::Rule::Member* prms = NULL;
        size_t count = 0;
        if (allowIncomingSignal) {
            prms = new PermissionPolicy::Rule::Member[3];
            prms[count].SetMemberName("*");
            prms[count].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
            prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
            count++;
        } else {
            prms = new PermissionPolicy::Rule::Member[2];
        }
        prms[count].SetMemberName("*");
        prms[count].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        count++;
        prms[count].SetMemberName("*");
        prms[count].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        count++;
        rules[0].SetMembers(count, prms);
        acls[0].SetRules(1, rules);
        delete [] prms;
    }
    /* acls record 1 FROM_CERTIFICATE_AUTHORITY */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
        KeyInfoNISTP256 keyInfo(guestCA);
        peers[0].SetKeyInfo(&keyInfo);
        acls[1].SetPeers(1, peers);
        PermissionPolicy::Rule rules[1];
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName("*");
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("*");
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        rules[0].SetMembers(1, prms);
        acls[1].SetRules(1, rules);
    }
    return AddAcls(policy, acls, ArraySize(acls));
}

static QStatus GenerateGuildSpecificAccessOutgoingPolicy(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy, const GUID128& guildGUID, BusAttachment& guildAuthorityBus)
{
    qcc::GUID128 guildAuthorityGUID;
    PermissionMgmtTestHelper::GetGUID(guildAuthorityBus, guildAuthorityGUID);
    ECCPublicKey guildAuthorityPubKey;
    QStatus status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(guildAuthorityBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);

    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(9);

    PermissionPolicy::Acl acls[2];

    /* acls record 0  ANY-USER */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
        acls[0].SetPeers(1, peers);
        PermissionPolicy::Rule rules[1];
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
        PermissionPolicy::Rule::Member prms[3];
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
        acls[0].SetRules(1, rules);
    }

    /* acls record 1 GUILD specific */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
        peers[0].SetSecurityGroupId(guildGUID);
        KeyInfoNISTP256 keyInfo;
        keyInfo.SetKeyId(guildAuthorityGUID.GetBytes(), qcc::GUID128::SIZE);
        keyInfo.SetPublicKey(&guildAuthorityPubKey);
        peers[0].SetKeyInfo(&keyInfo);
        acls[1].SetPeers(1, peers);
        PermissionPolicy::Rule rules[1];
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
        PermissionPolicy::Rule::Member prms[3];
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
        acls[1].SetRules(1, rules);
    }

    return AddAcls(policy, acls, ArraySize(acls));
}

static QStatus GeneratePolicyPeerPublicKey(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy, qcc::ECCPublicKey& peerPublicKey)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(10);

    /* add more acls */

    PermissionPolicy::Acl acls[1];

    /* acls record 0 peer */
    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(&peerPublicKey);
    peers[0].SetKeyInfo(&keyInfo);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule rules[2];
    {
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
        PermissionPolicy::Rule::Member prms[4];
        prms[0].SetMemberName("Up");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[2].SetMemberName("Volume");
        prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[3].SetMemberName("Caption");
        prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(4, prms);
    }
    {
        rules[1].SetObjPath("*");
        rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("*");
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[1].SetMembers(1, prms);
    }
    acls[0].SetRules(2, rules);
    return AddAcls(policy, acls, ArraySize(acls));
}

static QStatus GeneratePolicyDenyPeerPublicKey(BusAttachment& bus, BusAttachment& targetBus, PermissionPolicy& policy, qcc::ECCPublicKey& peerPublicKey)
{
    SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    policy.SetVersion(11);

    PermissionPolicy::Acl acls[1];

    /* acls record 0 peer */
    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(&peerPublicKey);
    peers[0].SetKeyInfo(&keyInfo);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule rules[2];
    {
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
        PermissionPolicy::Rule::Member prms[4];
        prms[0].SetMemberName("Up");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE); /* disallow execution */
        prms[2].SetMemberName("Volume");
        prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[3].SetMemberName("Caption");
        prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(4, prms);
    }
    {
        rules[1].SetObjPath("*");
        rules[1].SetInterfaceName("*");
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("*");
        prms[0].SetActionMask(0); /* explicit denied */
        rules[1].SetMembers(1, prms);
    }
    acls[0].SetRules(2, rules);
    return AddAcls(policy, acls, ArraySize(acls));
}

static QStatus GenerateAllowAllManifest(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 1;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member prms[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    rules[0].SetMembers(1, prms);
    *retRules = rules;
    return ER_OK;
}

static QStatus GenerateManifestNoInputSource(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    {
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
        PermissionPolicy::Rule::Member prms[4];
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
        prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(4, prms);
    }
    {
        rules[1].SetObjPath("*");
        rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("*");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[1].SetMembers(1, prms);
    }
    *retRules = rules;
    return ER_OK;
}

static QStatus GenerateManifestNoGetAllProperties(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 1;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    {
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName("*");
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("Volume");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        rules[0].SetMembers(1, prms);
    }
    *retRules = rules;
    return ER_OK;
}

static QStatus GenerateManifestDenied(bool denyTVUp, bool denyCaption, PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetObjPath("*");
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    {
        PermissionPolicy::Rule::Member prms[5];
        prms[0].SetMemberName("Up");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        if (denyTVUp) {
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE); /* disallow execution */
        } else {
            prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        }
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[2].SetMemberName("ChannelChanged");
        prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
        prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        prms[3].SetMemberName("Volume");
        prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[4].SetMemberName("Caption");
        prms[4].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        if (denyCaption) {
            prms[4].SetActionMask(0);
        } else {
            prms[4].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
        }
        rules[0].SetMembers(5, prms);
    }
    {
        rules[1].SetObjPath("*");
        rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("*");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[1].SetMembers(1, prms);
    }

    *retRules = rules;
    return ER_OK;
}
static QStatus GenerateManifestTemplate(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    // Rule 1
    {
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
        PermissionPolicy::Rule::Member prms[2];
        prms[0].SetMemberName("Up");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(2, prms);
    }
    // Rule 2
    {
        rules[1].SetObjPath("*");
        rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("*");
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[1].SetMembers(1, prms);
    }

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

    QStatus VerifyIdentity(BusAttachment& bus, BusAttachment& targetBus, IdentityCertificate* identityCertChain, size_t certChainCount)
    {
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        /* retrieve the identity cert chain to compare */
        MsgArg certChainArg;
        EXPECT_EQ(ER_OK, saProxy.GetIdentity(certChainArg)) << "GetIdentity failed.";
        size_t newCertChainCount = certChainArg.v_array.GetNumElements();
        if (newCertChainCount == 0) {
            return ER_FAIL;
        }
        EXPECT_EQ(newCertChainCount, certChainCount) << "GetIdentity returns back a cert chain in different length than the original.";
        IdentityCertificate* newCertChain = new IdentityCertificate[newCertChainCount];
        EXPECT_EQ(ER_OK, SecurityApplicationProxy::MsgArgToIdentityCertChain(certChainArg, newCertChain, newCertChainCount)) << "MsgArgToIdentityCertChain failed.";
        for (size_t cnt = 0; cnt < newCertChainCount; cnt++) {
            qcc::String retIdentity;
            status = newCertChain[cnt].EncodeCertificateDER(retIdentity);
            EXPECT_EQ(ER_OK, status) << "  newCert.EncodeCertificateDER failed.  Actual Status: " << QCC_StatusText(status);
            qcc::String originalIdentity;
            EXPECT_EQ(ER_OK, identityCertChain[cnt].EncodeCertificateDER(originalIdentity)) << "  original cert EncodeCertificateDER failed.";
            EXPECT_EQ(0, memcmp(originalIdentity.data(), retIdentity.data(), originalIdentity.length())) << "  GetIdentity failed.  Return value does not equal original";
        }
        delete [] newCertChain;
        return ER_OK;
    }

    QStatus VerifyIdentityManifestSize(BusAttachment& bus, BusAttachment& targetBus, size_t manifestSize)
    {
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        /* retrieve the manifest to compare */
        MsgArg manifestArg;
        EXPECT_EQ(ER_OK, saProxy.GetManifest(manifestArg)) << "GetManifest failed.";
        size_t newManifestCount = manifestArg.v_array.GetNumElements();
        if (newManifestCount == 0) {
            return ER_FAIL;
        }
        EXPECT_EQ(newManifestCount, manifestSize) << "GetManifest returns back a cert chain in different length than the original.";
        return ER_OK;
    }

    QStatus RetrieveIdentityCertificateId(BusAttachment& bus, BusAttachment& targetBus, qcc::String serial)
    {
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        /* retrieve the identity certificate id property */
        qcc::String serialNum;
        KeyInfoNISTP256 keyInfo;
        EXPECT_EQ(ER_OK, saProxy.GetIdentityCertificateId(serialNum, keyInfo)) << "GetIdentityCertificateId failed.";
        EXPECT_TRUE(serial == serialNum) << " The serial numbers don't match.";
        return ER_OK;
    }

    void TestStateSignalReception()
    {
        if (canTestStateSignalReception) {
            /* sleep a second to see whether the ApplicationState signal is received */
            for (int cnt = 0; cnt < 100; cnt++) {
                if (GetApplicationStateSignalReceived()) {
                    break;
                }
                qcc::Sleep(10);
            }
            EXPECT_TRUE(GetApplicationStateSignalReceived()) << " Fail to receive expected ApplicationState signal.";
        }
    }

    QStatus InvokeClaim(bool useAdminSG, BusAttachment& claimerBus, BusAttachment& claimedBus, qcc::String serial, qcc::String alias, bool expectClaimToFail, BusAttachment* caBus)
    {
        SecurityApplicationProxy saProxy(claimerBus, claimedBus.GetUniqueName().c_str());
        /* retrieve public key from to-be-claimed app to create identity cert */
        ECCPublicKey claimedPubKey;
        EXPECT_EQ(ER_OK, saProxy.GetEccPublicKey(claimedPubKey)) << " Fail to retrieve to-be-claimed public key.";
        qcc::GUID128 guid;
        PermissionMgmtTestHelper::GetGUID(claimedBus, guid);
        IdentityCertificate identityCertChain[3];
        size_t certChainCount = 3;
        PermissionPolicy::Rule* manifest = NULL;
        size_t manifestSize = 0;
        GenerateAllowAllManifest(&manifest, &manifestSize);
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(claimerBus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
        if (caBus != NULL) {
            status = PermissionMgmtTestHelper::CreateIdentityCertChain(*caBus, claimerBus, serial, guid.ToString(), &claimedPubKey, alias, 3600, identityCertChain, certChainCount, digest, Crypto_SHA256::DIGEST_SIZE);
        } else {
            certChainCount = 1;
            status = PermissionMgmtTestHelper::CreateIdentityCert(claimerBus, serial, guid.ToString(), &claimedPubKey, alias, 3600, identityCertChain[0], digest, Crypto_SHA256::DIGEST_SIZE);
        }
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

        QStatus status;
        if (useAdminSG) {
            status = saProxy.Claim(adminAdminGroupAuthority, adminAdminGroupGUID, adminAdminGroupAuthority, identityCertChain, certChainCount, manifest, manifestSize);
        } else {
            status = saProxy.Claim(adminAdminGroupAuthority, consumerAdminGroupGUID, consumerAdminGroupAuthority, identityCertChain, certChainCount, manifest, manifestSize);
        }
        delete [] manifest;
        if (expectClaimToFail) {
            return status;
        }
        EXPECT_EQ(ER_OK, status) << "Claim failed.";
        return VerifyIdentity(claimerBus, claimedBus, identityCertChain, certChainCount);
    }

    QStatus InvokeClaim(bool useAdminCA, BusAttachment& claimerBus, BusAttachment& claimedBus, qcc::String serial, qcc::String alias, bool expectClaimToFail)
    {
        return InvokeClaim(useAdminCA, claimerBus, claimedBus, serial, alias, expectClaimToFail, NULL);
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
        GenerateCAKeys();

        SecurityApplicationProxy saProxy(adminBus, adminBus.GetUniqueName().c_str());
        /* The ManagedApplication::Reset method should not be allowed before Claim */
        EXPECT_EQ(ER_PERMISSION_DENIED, saProxy.Reset()) << "Reset did not fail.";

        EXPECT_EQ(ER_OK, InvokeClaim(true, adminBus, adminBus, "1010101", "Admin User", false)) << " InvokeClaim failed.";

        InstallMembershipToAdmin(adminMembershipSerial1, adminAdminGroupGUID, adminBus);
        InstallMembershipToAdmin(adminMembershipSerial2, consumerAdminGroupGUID, consumerBus);
    }

    /**
     *  Claim the service app
     */
    void ClaimService()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = serviceBus.GetPermissionConfigurator();
        SetFactoryResetReceived(false);
        SetPolicyChangedReceived(false);
        status = pc.Reset();
        EXPECT_TRUE(GetFactoryResetReceived());
        EXPECT_TRUE(GetPolicyChangedReceived());
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);

        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminBus, serviceBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);

        SecurityApplicationProxy saProxy(adminBus, serviceBus.GetUniqueName().c_str());
        ECCPublicKey claimedPubKey;
        EXPECT_EQ(ER_OK, saProxy.GetEccPublicKey(claimedPubKey)) << "GetPeerPublicKey failed.";

        SetApplicationStateSignalReceived(false);

        /* setup state unclaimable */
        PermissionConfigurator::ApplicationState applicationState;
        EXPECT_EQ(ER_OK, pc.GetApplicationState(applicationState));
        EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationState) << "  ApplicationState is not CLAIMABLE";
        applicationState = PermissionConfigurator::NOT_CLAIMABLE;
        EXPECT_EQ(ER_OK, pc.SetApplicationState(applicationState)) << "  SetApplicationState failed.";
        EXPECT_EQ(ER_OK, pc.GetApplicationState(applicationState));
        EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationState) << "  ApplicationState is not UNCLAIMABLE";

        /* try claiming with state unclaimable.  Exptect to fail */
        EXPECT_EQ(ER_PERMISSION_DENIED, InvokeClaim(true, adminBus, serviceBus, "2020202", "Service Provider", true)) << " InvokeClaim is not supposed to succeed.";

        /* now switch it back to claimable */
        applicationState = PermissionConfigurator::CLAIMABLE;
        EXPECT_EQ(ER_OK, pc.SetApplicationState(applicationState)) << "  SetApplicationState failed.";
        EXPECT_EQ(ER_OK, pc.GetApplicationState(applicationState));

        EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationState) << "  ApplicationState is not CLAIMABLE";

        /* try claiming with state claimable.  Expect to succeed */
        SetPolicyChangedReceived(false);
        EXPECT_EQ(ER_OK, InvokeClaim(true, adminBus, serviceBus, "2020202", "Service Provider", false)) << " InvokeClaim failed.";
        EXPECT_TRUE(GetPolicyChangedReceived());

        /* try to claim one more time */
        SetPolicyChangedReceived(false);
        EXPECT_EQ(ER_PERMISSION_DENIED, InvokeClaim(true, adminBus, serviceBus, "2020202", "Service Provider", true)) << " InvokeClaim is not supposed to succeed.";
        EXPECT_FALSE(GetPolicyChangedReceived());

        ECCPublicKey claimedPubKey2;
        /* retrieve public key from claimed app to validate that it is not changed */
        EXPECT_EQ(ER_OK, saProxy.GetEccPublicKey(claimedPubKey2)) << "GetPeerPublicKey failed.";
        EXPECT_EQ(memcmp(&claimedPubKey2, &claimedPubKey, sizeof(ECCPublicKey)), 0) << "  The public key of the claimed app has changed.";

        TestStateSignalReception();
    }

    /**
     *  Claim the consumer
     */
    void ClaimConsumer()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = consumerBus.GetPermissionConfigurator();
        SetFactoryResetReceived(false);
        SetPolicyChangedReceived(false);
        status = pc.Reset();
        EXPECT_TRUE(GetFactoryResetReceived());
        EXPECT_TRUE(GetPolicyChangedReceived());
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);

        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminBus, consumerBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);

        SetApplicationStateSignalReceived(false);
        SetPolicyChangedReceived(false);
        EXPECT_EQ(ER_OK, InvokeClaim(false, adminBus, consumerBus, "3030303", "Consumer", false, &adminBus)) << " InvokeClaim failed.";
        EXPECT_TRUE(GetPolicyChangedReceived());

        /* try to claim a second time */
        SetPolicyChangedReceived(false);
        EXPECT_EQ(ER_PERMISSION_DENIED, InvokeClaim(false, adminBus, consumerBus, "3030303", "Consumer", true, &adminBus)) << "Claim is not supposed to succeed.";
        EXPECT_FALSE(GetPolicyChangedReceived());

        TestStateSignalReception();
        /* add the consumer admin security group membership cert to consumer */
        qcc::String currentAuthMechanisms = GetAuthMechanisms();
        EnableSecurity("ALLJOYN_ECDHE_ECDSA");
        InstallMembershipToConsumer("100000001", consumerAdminGroupGUID, consumerBus);
        EnableSecurity(currentAuthMechanisms.c_str());
    }

    /**
     *  Claim the remote control by the consumer
     */
    void ConsumerClaimsRemoteControl()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = remoteControlBus.GetPermissionConfigurator();
        SetFactoryResetReceived(false);
        SetPolicyChangedReceived(false);
        status = pc.Reset();
        EXPECT_TRUE(GetFactoryResetReceived());
        EXPECT_TRUE(GetPolicyChangedReceived());
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);

        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(consumerBus, remoteControlBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
        SetApplicationStateSignalReceived(false);
        SetPolicyChangedReceived(false);
        EXPECT_EQ(ER_OK, InvokeClaim(false, consumerBus, remoteControlBus, "6060606", "remote control", false, &consumerBus)) << " InvokeClaim failed.";
        EXPECT_TRUE(GetPolicyChangedReceived());

        TestStateSignalReception();
    }

    void Claims(bool usePSK, bool claimRemoteControl)
    {
        if (usePSK) {
            EnableSecurity("ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA");
        } else {
            EnableSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA");
        }
        DetermineStateSignalReachable();
        ClaimAdmin();
        ClaimService();
        ClaimConsumer();
        if (claimRemoteControl) {
            ConsumerClaimsRemoteControl();
        }
    }

    void Claims(bool usePSK)
    {
        /* also claims the remote control */
        Claims(usePSK, true);
    }

    void AppHasAllowAllManifest(BusAttachment& bus, BusAttachment& targetBus)
    {
        PermissionPolicy::Rule* manifest = NULL;
        size_t manifestSize = 0;
        GenerateAllowAllManifest(&manifest, &manifestSize);
        EXPECT_EQ(ER_OK, VerifyIdentityManifestSize(bus, targetBus, manifestSize)) << "VerifyIdentityManifestSize failed.";
        delete [] manifest;
    }

    /**
     *  Install policy to service app
     */
    void InstallPolicyToAdmin(PermissionPolicy& policy)
    {
        SecurityApplicationProxy saProxy(adminBus, adminBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_OK, saProxy.UpdatePolicy(policy)) << "  UpdatePolicy failed.";

        /* After having a new policy installed, the target bus
           clears out all of its peer's secret and session keys, so the
           next call will get security violation.  So just make the call and ignore
           the outcome.
         */
        PermissionPolicy retPolicy;
        saProxy.GetPolicy(retPolicy);

        /* retrieve back the policy to compare */
        EXPECT_EQ(ER_OK, saProxy.GetPolicy(retPolicy)) << "GetPolicy failed.";

        EXPECT_EQ(policy.GetVersion(), retPolicy.GetVersion()) << " GetPolicy failed. Different policy version number.";
        EXPECT_EQ(policy.GetAclsSize(), retPolicy.GetAclsSize()) << " GetPolicy failed. Different incoming acls size.";
    }

    /**
     *  Install policy to app
     */
    void InstallPolicyToNoAdmin(BusAttachment& installerBus, BusAttachment& bus, PermissionPolicy& policy)
    {
        SecurityApplicationProxy saProxy(installerBus, bus.GetUniqueName().c_str());

        SetApplicationStateSignalReceived(false);
        EXPECT_EQ(ER_OK, saProxy.UpdatePolicy(policy)) << "UpdatePolicy failed.";

        /* After having a new policy installed, the target bus
           clears out all of its peer's secret and session keys, so the
           next call will get security violation.  So just make the call and ignore
           the outcome.
         */
        PermissionPolicy retPolicy;
        saProxy.GetPolicy(retPolicy);
        /* retrieve back the policy to compare */
        EXPECT_EQ(ER_OK, saProxy.GetPolicy(retPolicy)) << "GetPolicy failed.";

        EXPECT_EQ(policy.GetVersion(), retPolicy.GetVersion()) << " GetPolicy failed. Different policy version number.";
        EXPECT_EQ(policy.GetAclsSize(), retPolicy.GetAclsSize()) << " GetPolicy failed. Different incoming acls size.";
        /* install a policy with the same policy version number.  Expect to fail. */
        EXPECT_NE(ER_OK, saProxy.UpdatePolicy(policy)) << "UpdatePolicy again with same policy version number expected to fail, but it did not.";
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

    void RetrieveDefaultPolicy(BusAttachment& bus, BusAttachment& targetBus)
    {
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        /* retrieve the default policy */
        PermissionPolicy policy;
        EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";

        EXPECT_EQ(policy.GetVersion(), (uint32_t) 0) << " Default policy is supposed to have policy version 0.";
    }

    /*
     *  Replace service app Identity Certificate
     */
    void ReplaceIdentityCert(BusAttachment& bus, BusAttachment& targetBus, const PermissionPolicy::Rule* manifest, size_t manifestSize, bool generateRandomSubjectKey, bool setWrongManifestDigest)
    {
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        /* retrieve the current identity cert */
        MsgArg certChainArg;
        EXPECT_EQ(ER_OK, saProxy.GetIdentity(certChainArg)) << "GetIdentity failed.";
        size_t count = certChainArg.v_array.GetNumElements();
        ASSERT_GT(count, (size_t) 0) << "No identity cert found.";
        if (count == 0) {
            return;
        }
        IdentityCertificate* certs = new IdentityCertificate[count];
        EXPECT_EQ(ER_OK, SecurityApplicationProxy::MsgArgToIdentityCertChain(certChainArg, certs, count)) << "MsgArgToIdentityCertChain failed.";

        /* create a new identity cert */
        qcc::String subject((const char*) certs[0].GetSubjectCN(), certs[0].GetSubjectCNLength());
        const qcc::ECCPublicKey* subjectPublicKey;
        if (generateRandomSubjectKey) {
            Crypto_ECC ecc;
            ecc.GenerateDSAKeyPair();
            subjectPublicKey = ecc.GetDSAPublicKey();
        } else {
            subjectPublicKey = certs[0].GetSubjectPublicKey();
        }
        IdentityCertificate identityCertChain[1];
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        if (setWrongManifestDigest) {
            for (size_t cnt = 0; cnt < Crypto_SHA256::DIGEST_SIZE; cnt++) {
                digest[cnt] = cnt;
            }
        } else {
            EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(bus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
        }
        status = PermissionMgmtTestHelper::CreateIdentityCert(bus, "4040404", subject, subjectPublicKey, "Service Provider", 3600, identityCertChain[0], digest, Crypto_SHA256::DIGEST_SIZE);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.";

        status = saProxy.UpdateIdentity(identityCertChain, 1, manifest, manifestSize);
        if (generateRandomSubjectKey || setWrongManifestDigest) {
            EXPECT_NE(ER_OK, status) << "InstallIdentity did not fail.";
        } else {
            EXPECT_EQ(ER_OK, status) << "InstallIdentity failed.";
            /* After having a new identity cert installed, the target bus
               clears out all of its peer's secret and session keys, so the
               next call will get security violation.
             */
            PermissionPolicy retPolicy;
            EXPECT_NE(ER_OK, saProxy.GetPolicy(retPolicy)) << "GetPolicy did not fail.";
            /* Try the same call again, it will succeed. */
            EXPECT_EQ(ER_OK, saProxy.GetPolicy(retPolicy)) << "GetPolicy failed.";
        }
        delete [] certs;
    }

    void ReplaceIdentityCert(BusAttachment& bus, BusAttachment& targetBus, const PermissionPolicy::Rule* manifest, size_t manifestSize, bool generateRandomSubjectKey)
    {
        ReplaceIdentityCert(bus, targetBus, manifest, manifestSize, generateRandomSubjectKey, false);
    }

    void ReplaceIdentityCert(BusAttachment& bus, BusAttachment& targetBus, bool generateRandomSubjectKey, bool setWrongManifestDigest)
    {
        PermissionPolicy::Rule* manifest = NULL;
        size_t manifestSize = 0;
        GenerateAllowAllManifest(&manifest, &manifestSize);
        ReplaceIdentityCert(bus, targetBus, manifest, manifestSize, generateRandomSubjectKey, setWrongManifestDigest);
        delete [] manifest;
    }

    void ReplaceIdentityCert(BusAttachment& bus, BusAttachment& targetBus, bool generateRandomSubjectKey)
    {
        ReplaceIdentityCert(bus, targetBus, generateRandomSubjectKey, false);
    }

    void ReplaceIdentityCert(BusAttachment& bus, BusAttachment& targetBus)
    {
        ReplaceIdentityCert(bus, targetBus, false);
    }

    void ReplaceIdentityCertWithBadPublicKey(BusAttachment& bus, BusAttachment& targetBus)
    {
        ReplaceIdentityCert(bus, targetBus, true);
    }

    void ReplaceIdentityCertWithBadManifestDigest(BusAttachment& bus, BusAttachment& targetBus)
    {
        ReplaceIdentityCert(bus, targetBus, false, true);
    }

    void ReplaceIdentityCertWithExpiredCert(BusAttachment& bus, BusAttachment& targetBus)
    {
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        /* retrieve the current identity cert */
        MsgArg certChainArg;
        EXPECT_EQ(ER_OK, saProxy.GetIdentity(certChainArg)) << "GetIdentity failed.";
        size_t count = certChainArg.v_array.GetNumElements();
        ASSERT_GT(count, (size_t) 0) << "No identity cert found.";
        if (count == 0) {
            return;
        }
        IdentityCertificate* certs = new IdentityCertificate[count];
        EXPECT_EQ(ER_OK, SecurityApplicationProxy::MsgArgToIdentityCertChain(certChainArg, certs, count)) << "MsgArgToIdentityCertChain failed.";

        /* create a new identity cert */
        qcc::String subject((const char*) certs[0].GetSubjectCN(), certs[0].GetSubjectCNLength());
        IdentityCertificate identityCertChain[1];
        PermissionPolicy::Rule* manifest = NULL;
        size_t manifestSize = 0;
        GenerateAllowAllManifest(&manifest, &manifestSize);
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(bus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
        /* create a cert that expires in 1 second */
        status = PermissionMgmtTestHelper::CreateIdentityCert(bus, "4040404", subject, certs[0].GetSubjectPublicKey(), "Service Provider", 1, identityCertChain[0], digest, Crypto_SHA256::DIGEST_SIZE);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.";

        /* sleep 2 seconds to get the cert to expire */
        qcc::Sleep(2000);
        EXPECT_NE(ER_OK, saProxy.UpdateIdentity(identityCertChain, 1, manifest, manifestSize)) << "InstallIdentity did not fail.";
        delete [] certs;
        delete [] manifest;
    }

    /**
     *  Install a membership certificate with the given serial number and guild ID to the service bus attachment.
     */
    void InstallMembershipToServiceProvider(const char* serial, qcc::GUID128& guildID)
    {
        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(serviceBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembership RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String subjectCN(serviceGUID.ToString());
        status = PermissionMgmtTestHelper::InstallMembership(serial, adminBus, serviceBus.GetUniqueName(), adminBus, subjectCN, &claimedPubKey, guildID);
        EXPECT_EQ(ER_OK, status) << "  InstallMembership cert1 failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembership(serial, adminBus, serviceBus.GetUniqueName(), adminBus, subjectCN, &claimedPubKey, guildID);
        EXPECT_NE(ER_OK, status) << "  InstallMembership cert1 again is supposed to fail.  Actual Status: " << QCC_StatusText(status);
    }

    void InstallMembershipToServiceProvider()
    {
        InstallMembershipToServiceProvider(membershipSerial3, membershipGUID3);
    }

    /**
     *  RemoveMembership from service provider
     */
    void RemoveMembershipFromServiceProvider()
    {
        SecurityApplicationProxy saProxy(adminBus, serviceBus.GetUniqueName().c_str());
        KeyInfoNISTP256 keyInfo;
        adminBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
        String aki;
        CertificateX509::GenerateAuthorityKeyId(keyInfo.GetPublicKey(), aki);
        keyInfo.SetKeyId((const uint8_t*) aki.data(), aki.size());
        EXPECT_EQ(ER_OK, saProxy.RemoveMembership(membershipSerial3, keyInfo)) << "RemoveMembershipFromServiceProvider failed.";
        /* removing it again */
        EXPECT_NE(ER_OK, saProxy.RemoveMembership(membershipSerial3, keyInfo)) << "RemoveMembershipFromServiceProvider succeeded.  Expect it to fail.";

    }

    /**
     *  Install Membership to a consumer
     */
    void InstallMembershipToConsumer(const char* serial, qcc::GUID128& guildID, BusAttachment& authorityBus)
    {
        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String subjectCN(consumerGUID.ToString());
        status = PermissionMgmtTestHelper::InstallMembership(serial, adminBus, consumerBus.GetUniqueName(), authorityBus, subjectCN, &claimedPubKey, guildID);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer cert1 failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Install Membership to a consumer
     */
    void InstallMembershipToConsumer()
    {
        InstallMembershipToConsumer(membershipSerial1, membershipGUID1, adminBus);
    }

    /**
     *  Install Membership chain to a consumer
     */
    void InstallMembershipChainToTarget(BusAttachment& topBus, BusAttachment& middleBus, BusAttachment& targetBus, const char* serial0, const char* serial1, qcc::GUID128& guildID)
    {
        ECCPublicKey targetPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(targetBus, &targetPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipChainToTarget RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        ECCPublicKey secondPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(middleBus, &secondPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipChainToTarget RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        CredentialAccessor mca(middleBus);
        qcc::GUID128 middleGUID;
        status = mca.GetGuid(middleGUID);
        CredentialAccessor tca(targetBus);
        qcc::GUID128 targetGUID;
        status = tca.GetGuid(targetGUID);
        qcc::String middleCN(middleGUID.ToString());
        qcc::String targetCN(targetGUID.ToString());
        status = PermissionMgmtTestHelper::InstallMembershipChain(topBus, middleBus, serial0, serial1, targetBus.GetUniqueName().c_str(), middleCN, &secondPubKey, targetCN, &targetPubKey, guildID);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipChainToTarget failed.  Actual Status: " << QCC_StatusText(status);

        /* retrieve the membership summaries to verify the issuer public key is provided */
        SecurityApplicationProxy saProxy(middleBus, targetBus.GetUniqueName().c_str());
        MsgArg arg;
        EXPECT_EQ(ER_OK, saProxy.GetMembershipSummaries(arg)) << "GetMembershipSummaries failed.";
        size_t count = arg.v_array.GetNumElements();

        ASSERT_GT(count, (size_t) 0) << "No membership cert found.";
        if (count == 0) {
            return;
        }
        KeyInfoNISTP256* keyInfos = new KeyInfoNISTP256[count];
        String* serials = new String[count];
        EXPECT_EQ(ER_OK, SecurityApplicationProxy::MsgArgToCertificateIds(arg, serials, keyInfos, count)) << " MsgArgToCertificateIds failed.";

        bool nonEmptyPublicKey = false;
        for (size_t cnt = 0; cnt < count; cnt++) {
            if (!keyInfos[cnt].GetPublicKey()->empty()) {
                nonEmptyPublicKey = true;
                break;
            }
        }
        delete [] serials;
        delete [] keyInfos;
        EXPECT_TRUE(nonEmptyPublicKey) << " At least one issuer public key is not supposed to be empty.";
    }

    /**
     *  Install other's membership certificate to a consumer
     */
    void InstallOthersMembershipToConsumer()
    {
        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(adminBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallOthersMembershipToConsumer RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String subjectCN(consumerGUID.ToString());
        EXPECT_NE(ER_OK, PermissionMgmtTestHelper::InstallMembership(membershipSerial1, adminBus, serviceBus.GetUniqueName(), adminBus, subjectCN, &claimedPubKey, membershipGUID1)) << "  InstallOthersMembershipToConsumer InstallMembership is supposed to failed.";
    }

    /**
     *  Install Membership to the admin
     */
    void InstallMembershipToAdmin(const String& serial, const GUID128& membershipGUID, BusAttachment& authorityBus, bool expectSuccess = true)
    {
        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(adminBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToAdmin RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String subjectCN(consumerGUID.ToString());
        status = PermissionMgmtTestHelper::InstallMembership(serial, adminBus, adminBus.GetUniqueName(), authorityBus, subjectCN, &claimedPubKey, membershipGUID);

        if (expectSuccess) {
            EXPECT_EQ(ER_OK, status) << "  InstallMembershipToAdmin cert1 failed.";
        } else {
            EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  InstallMembershipToAdmin cert1 is not support to succeed.";
        }
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

    void AppCannotCallTVDown(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVDown(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AppCannotCallTVDown ExcerciseTVDown should have failed.  Actual Status: " << QCC_StatusText(status);
    }

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

    void AppCanSetTVVolume(BusAttachment& bus, BusAttachment& targetBus, uint32_t tvVolume)
    {
        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::SetTVVolume(bus, clientProxyObject, tvVolume)) << "  AppCanSetTVVolume SetTVVolume failed.";
        uint32_t newTvVolume = 0;
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::GetTVVolume(bus, clientProxyObject, newTvVolume)) << "  AppCanSetTVVolume failed.";
        EXPECT_EQ(newTvVolume, tvVolume) << "  AppCanSetTVVolume GetTVVolume got wrong TV volume.";
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
     *  App gets the Security interfaces' version number
     */
    void AppGetVersionNumber(BusAttachment& bus, BusAttachment& targetBus)
    {
        uint16_t versionNum = 0;
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_OK, saProxy.GetSecurityApplicationVersion(versionNum)) << "AppGetVersionNumber GetSecurityApplicationVersion failed.";
        EXPECT_EQ(1, versionNum) << "AppGetVersionNumber received unexpected version number.";
        EXPECT_EQ(ER_OK, saProxy.GetClaimableApplicationVersion(versionNum)) << "AppGetVersionNumber GetClaimableApplicationVersion failed.";
        EXPECT_EQ(1, versionNum) << "AppGetVersionNumber received unexpected version number.";
        EXPECT_EQ(ER_OK, saProxy.GetManagedApplicationVersion(versionNum)) << "AppGetVersionNumber GetClaimableApplicationVersion failed.";
        EXPECT_EQ(1, versionNum) << "AppGetVersionNumber received unexpected version number.";
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
    void SetManifestTemplateOnServiceProvider()
    {

        PermissionPolicy::Rule* rules = NULL;
        size_t count = 0;
        QStatus status = GenerateManifestTemplate(&rules, &count);
        EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest GenerateManifest failed.  Actual Status: " << QCC_StatusText(status);
        PermissionConfigurator& pc = serviceBus.GetPermissionConfigurator();
        status = pc.SetPermissionManifest(rules, count);
        EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest SetPermissionManifest failed.  Actual Status: " << QCC_StatusText(status);

        SecurityApplicationProxy saProxy(adminBus, serviceBus.GetUniqueName().c_str());
        MsgArg arg;
        EXPECT_EQ(ER_OK, saProxy.GetManifestTemplate(arg)) << "SetPermissionManifest GetManifestTemplate failed.";
        size_t retrievedCount = arg.v_array.GetNumElements();
        ASSERT_EQ(count, retrievedCount) << "Install manifest template size is different than original.";
        if (retrievedCount == 0) {
            delete [] rules;
            return;
        }

        PermissionPolicy::Rule* retrievedRules = new PermissionPolicy::Rule[count];
        EXPECT_EQ(ER_OK, SecurityApplicationProxy::MsgArgToRules(arg, retrievedRules, count)) << "SecurityApplicationProxy::MsgArgToRules failed.";
        delete [] rules;
        delete [] retrievedRules;

        /* retrieve the manifest template digest */
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, saProxy.GetManifestTemplateDigest(digest, Crypto_SHA256::DIGEST_SIZE)) << "SetPermissionManifest GetManifestTemplateDigest failed.";
    }

    /*
     *  Remove Policy from service provider
     */
    void ResetPolicyFromApp(BusAttachment& bus, BusAttachment& targetBus)
    {
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        PermissionPolicy retPolicy;
        EXPECT_EQ(ER_OK, saProxy.GetPolicy(retPolicy)) << "GetPolicy did not fail.";
        uint32_t originalPolicyVersion = retPolicy.GetVersion();
        EXPECT_EQ(ER_OK, saProxy.ResetPolicy()) << "ResetPolicy failed.";
        /* get policy again.  Expect it to return a policy version 0 */
        EXPECT_EQ(ER_OK, saProxy.GetPolicy(retPolicy)) << "GetPolicy did not fail.";
        EXPECT_EQ(retPolicy.GetVersion(), (uint32_t) 0) << " Policy after reset is supposed to have policy version 0.";
        EXPECT_NE(retPolicy.GetVersion(), originalPolicyVersion) << " Policy after reset is not supposed to have same version as the non-default policy.";
    }

    /*
     * Remove Membership from consumer.
     */
    void RemoveMembershipFromConsumer()
    {
        SecurityApplicationProxy saProxy(adminBus, consumerBus.GetUniqueName().c_str());
        /* retrieve the membership summaries */
        MsgArg arg;
        EXPECT_EQ(ER_OK, saProxy.GetMembershipSummaries(arg)) << "GetMembershipSummaries failed.";
        size_t count = arg.v_array.GetNumElements();

        ASSERT_GT(count, (size_t) 0) << "No membership cert found.";
        if (count == 0) {
            return;
        }
        /* will delete the first membership cert */
        KeyInfoNISTP256* keyInfos = new KeyInfoNISTP256[count];
        String* serials = new String[count];
        EXPECT_EQ(ER_OK, SecurityApplicationProxy::MsgArgToCertificateIds(arg, serials, keyInfos, count)) << " MsgArgToCertificateIds failed.";

        if (keyInfos[0].GetPublicKey()->empty()) {
            keyInfos[0] = consumerAdminGroupAuthority;
        }

        EXPECT_EQ(ER_OK, saProxy.RemoveMembership(serials[0], keyInfos[0])) << "RemoveMembershipFromConsumer failed.";
        /* removing it again */
        EXPECT_NE(ER_OK, saProxy.RemoveMembership(serials[0], keyInfos[0])) << "RemoveMembershipFromConsumer succeeded.  Expect it to fail.";
        delete [] serials;
        delete [] keyInfos;
    }

    /**
     * Test PermissionMgmt Reset method on service.  The consumer should not be
     * able to reset the service since the consumer is not an admin.
     */
    void FailResetServiceByConsumer()
    {
        SecurityApplicationProxy saProxy(consumerBus, serviceBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_PERMISSION_DENIED, saProxy.Reset()) << "  Reset is not supposed to succeed.";

    }

    /*
     * Test PermissionMgmt Reset method on service by the admin.  The admin should be
     * able to reset the service.
     */
    void SuccessfulResetServiceByAdmin()
    {
        SecurityApplicationProxy saProxy(adminBus, serviceBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_OK, saProxy.Reset()) << "  Reset failed.";

        /* retrieve the current identity cert */
        MsgArg certChainArg;
        EXPECT_NE(ER_OK, saProxy.GetIdentity(certChainArg)) << "GetIdentity is not supposed to succeed since it was removed by Reset.";
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
        EXPECT_TRUE((ER_OK == status) || (ER_BUS_KEY_UNAVAILABLE == status)) << "  BusAttachment::ClearKeys failed.  Actual Status: " << QCC_StatusText(status);
    }

    void RetrievePropertyApplicationState(BusAttachment& bus, BusAttachment& targetBus, PermissionConfigurator::ApplicationState expectedValue)
    {
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        PermissionConfigurator::ApplicationState applicationState;
        EXPECT_EQ(ER_OK, saProxy.GetApplicationState(applicationState)) << "  GetApplicationState failed.";

        EXPECT_EQ(applicationState, expectedValue) << " The application state does not match.";
    }

    void RetrievePropertyClaimCapabilities(BusAttachment& bus, BusAttachment& targetBus, PermissionConfigurator::ClaimCapabilities expectedValue)
    {
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        PermissionConfigurator::ClaimCapabilities caps;
        EXPECT_EQ(ER_OK, saProxy.GetClaimCapabilities(caps)) << "  GetClaimCapabilities failed.";

        EXPECT_EQ(caps, expectedValue) << " The claim capabilities value does not match.";
    }

    void SetPropertyClaimCapabilities(BusAttachment& bus, PermissionConfigurator::ClaimCapabilities val)
    {
        EXPECT_EQ(ER_OK, bus.GetPermissionConfigurator().SetClaimCapabilities(val)) << "  SetClaimCapabilities failed.";

        PermissionConfigurator::ClaimCapabilities caps;
        EXPECT_EQ(ER_OK, bus.GetPermissionConfigurator().GetClaimCapabilities(caps)) << "  GetClaimCapabilities failed.";
        EXPECT_EQ(caps, val) << " The claim capabilities value does not match.";
    }

    void RetrievePropertyClaimCapabilityAdditionalInfo(BusAttachment& bus, BusAttachment& targetBus, PermissionConfigurator::ClaimCapabilityAdditionalInfo expectedValue)
    {
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        PermissionConfigurator::ClaimCapabilityAdditionalInfo additionalInfo;
        EXPECT_EQ(ER_OK, saProxy.GetClaimCapabilityAdditionalInfo(additionalInfo)) << "  RetrievePropertyClaimCapabilityAdditionalInfo failed.";

        EXPECT_EQ(additionalInfo, expectedValue) << " The claim additional info does not match.";
    }

    void SetPropertyClaimCapabilityAdditionalInfo(BusAttachment& bus, PermissionConfigurator::ClaimCapabilityAdditionalInfo val)
    {
        EXPECT_EQ(ER_OK, bus.GetPermissionConfigurator().SetClaimCapabilityAdditionalInfo(val)) << "  SetClaimCapabilityAdditionalInfo failed.";

        PermissionConfigurator::ClaimCapabilityAdditionalInfo additionalInfo;
        EXPECT_EQ(ER_OK, bus.GetPermissionConfigurator().GetClaimCapabilityAdditionalInfo(additionalInfo)) << "  GetClaimCapabilityAdditionalInfo failed.";
        EXPECT_EQ(additionalInfo, val) << " The claim capabilities value does not match.";
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

    RetrieveDefaultPolicy(adminBus, serviceBus);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    AppHasAllowAllManifest(adminBus, serviceBus);

    RetrieveIdentityCertificateId(adminBus, serviceBus, "2020202");
    RetrieveIdentityCertificateId(consumerBus, remoteControlBus, "6060606");

    ReplaceIdentityCert(adminBus, serviceBus);
    InstallMembershipToServiceProvider();

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    SetChannelChangedSignalReceived(false);
    ConsumerCanTVUpAndDownAndNotChannel();
    /* sleep a second to see whether the ChannelChanged signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetChannelChangedSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetChannelChangedSignalReceived()) << " Fail to receive expected ChannelChanged signal.";

    SetManifestTemplateOnServiceProvider();

    RetrieveServicePublicKey();
    RemoveMembershipFromServiceProvider();
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
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCanTVUpAndDownAndNotChannel();
    SetManifestTemplateOnServiceProvider();

}

/*
 *  Case: outbound message allowed by guild based acls and peer's membership
 */
TEST_F(PathBasePermissionMgmtUseCaseTest, OutboundAllowedByMembership)
{
    Claims(true);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);
    InstallMembershipToServiceProvider("1234", membershipGUID1);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateGuildSpecificAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy, membershipGUID1, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCanTVUpAndDownAndNotChannel();
}

/*
 *  Case: outbound message not allowed by guild based acls since
 *       peer does not have given guild membership.
 */
TEST_F(PermissionMgmtUseCaseTest, OutboundNotAllowedByMissingPeerMembership)
{
    Claims(true);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, consumerBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateGuildSpecificAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy, membershipGUID1, consumerBus)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
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
    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCannotCallTVOn();
}

TEST_F(PermissionMgmtUseCaseTest, AccessByPublicKey)
{
    Claims(true);
    /* generate a policy */
    ECCPublicKey consumerPublicKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &consumerPublicKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicyPeerPublicKey(adminBus, serviceBus, policy, consumerPublicKey)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanTVUpAndDownAndNotChannel();
}

TEST_F(PermissionMgmtUseCaseTest, AccessDeniedForPeerPublicKey)
{
    Claims(true);

    /* generate a policy */
    ECCPublicKey consumerPublicKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &consumerPublicKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicyDenyPeerPublicKey(adminBus, serviceBus, policy, consumerPublicKey)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCannotTurnTVUp();
    AppCannotCallTVDown(consumerBus, serviceBus);
}

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

TEST_F(PermissionMgmtUseCaseTest, InstallIdentityCertWithDifferentPubKey)
{
    Claims(false);
    ReplaceIdentityCertWithBadPublicKey(adminBus, serviceBus);
}

TEST_F(PermissionMgmtUseCaseTest, InstallIdentityCertWithExpiredCert)
{
    Claims(false);
    ReplaceIdentityCertWithExpiredCert(adminBus, consumerBus);
}

TEST_F(PermissionMgmtUseCaseTest, InstallIdentityCertWithBadManifestDigest)
{
    Claims(false);
    ReplaceIdentityCertWithBadManifestDigest(adminBus, serviceBus);
}

/*
 *  Case: claiming, install policy, install wrong membership
 */
TEST_F(PermissionMgmtUseCaseTest, SendingOthersMembershipCert)
{
    Claims(true);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, consumerBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    InstallOthersMembershipToConsumer();
}

TEST_F(PermissionMgmtUseCaseTest, AccessNotAuthorizedBecauseOfWrongActionMask)
{
    Claims(true);
    /* generate a limited policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GenerateSmallAnyUserPolicy(adminBus, serviceBus, policy)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
}

TEST_F(PermissionMgmtUseCaseTest, AccessNotAuthorizedBecauseOfDeniedOnPrefix)
{
    Claims(true);
    /* generate a limited policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GenerateAnyUserDeniedPrefixPolicy(adminBus, serviceBus, policy)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
}

TEST_F(PathBasePermissionMgmtUseCaseTest, ProviderHasNoMatchingGuildForConsumer)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, consumerBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);
    InstallMembershipToServiceProvider();

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer(membershipSerial4, membershipGUID4, adminBus);
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCannotTurnTVUp();
}

TEST_F(PermissionMgmtUseCaseTest, ProviderHasMoreMembershipCertsThanConsumer)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);
    InstallMembershipToServiceProvider();

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCanTVUpAndDownAndNotChannel();

}

TEST_F(PermissionMgmtUseCaseTest, ConsumerHasMoreMembershipCertsThanService)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);
    InstallMembershipToServiceProvider();

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    InstallMembershipToConsumer(membershipSerial2, membershipGUID2, adminBus);
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanCallOnAndOff();
    ConsumerCanChangeChannlel();
}

TEST_F(PermissionMgmtUseCaseTest, ConsumerHasGoodMembershipCertChain)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);
    InstallMembershipToServiceProvider();

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipChainToTarget(adminBus, adminBus, consumerBus, membershipSerial0, membershipSerial1, membershipGUID1);

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanTVUpAndDownAndNotChannel();
}

TEST_F(PermissionMgmtUseCaseTest, ConsumerHasMoreRestrictiveManifest)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);
    InstallMembershipToServiceProvider();

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipChainToTarget(adminBus, adminBus, consumerBus, membershipSerial0, membershipSerial1, membershipGUID1);

    PermissionPolicy::Rule* manifest;
    size_t manifestSize;
    GenerateManifestNoInputSource(&manifest, &manifestSize);
    ReplaceIdentityCert(adminBus, consumerBus, manifest, manifestSize, false);
    delete [] manifest;

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanTVUpAndDownAndNotChannel();
    ConsumerCannotCallTVInputSource();
}

TEST_F(PathBasePermissionMgmtUseCaseTest, ConsumerHasLessAccessInManifestUsingDenied)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, consumerBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);
    InstallMembershipToServiceProvider();

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();

    PermissionPolicy::Rule* manifest;
    size_t manifestSize;
    GenerateManifestDenied(true, true, &manifest, &manifestSize);
    ReplaceIdentityCert(adminBus, consumerBus, manifest, manifestSize, false);
    delete [] manifest;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCannotTurnTVUp();
    ConsumerCannotGetTVCaption();
}

TEST_F(PermissionMgmtUseCaseTest, AllowEverything)
{
    Claims(true);
    /* generate a limited policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GenerateWildCardPolicy(adminBus, serviceBus, policy)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanCallOnAndOff();
}

TEST_F(PermissionMgmtUseCaseTest, SignalAllowedFromAnyUser)
{
    Claims(false);
    /* generate a policy to permit sending a signal */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GenerateFullAccessAnyUserPolicy(adminBus, serviceBus, policy, true)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    SetChannelChangedSignalReceived(false);
    ConsumerCanChangeChannlel();
    /* sleep a second to see whether the ChannelChanged signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetChannelChangedSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetChannelChangedSignalReceived()) << " Fail to receive expected ChannelChanged signal.";
}

TEST_F(PermissionMgmtUseCaseTest, SignalNotAllowedToEmit)
{
    Claims(false);
    /* generate a policy not permit sending a signal */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GenerateFullAccessAnyUserPolicy(adminBus, serviceBus, policy, false)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    SetChannelChangedSignalReceived(false);
    ConsumerCanChangeChannlel();
    /* sleep a second to see whether the ChannelChanged signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetChannelChangedSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_FALSE(GetChannelChangedSignalReceived()) << " Unexpect to receive ChannelChanged signal.";
}

TEST_F(PermissionMgmtUseCaseTest, SignalNotAllowedToReceive)
{
    Claims(false);
    /* generate a policy to permit sending a signal */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GenerateFullAccessAnyUserPolicy(adminBus, serviceBus, policy, true)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    /* full access outgoing but do not accept incoming signal */
    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy, false)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    SetChannelChangedSignalReceived(false);
    ConsumerCanChangeChannlel();
    /* sleep a second to see whether the ChannelChanged signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetChannelChangedSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_FALSE(GetChannelChangedSignalReceived()) << " Unexpect to receive ChannelChanged signal.";
}

TEST_F(PermissionMgmtUseCaseTest, AccessGrantedForPeerFromSpecificCA)
{
    Claims(false);

    /* Setup so remote control is not trusted by service provider */
    PermissionPolicy servicePolicy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, servicePolicy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(servicePolicy);

    PermissionPolicy remoteControlPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(consumerBus, remoteControlBus, remoteControlPolicy)) << "GeneratePolicy failed.";
    EnableSecurity("ALLJOYN_ECDHE_ECDSA");
    InstallPolicyToClientBus(consumerBus, remoteControlBus, remoteControlPolicy);

    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(remoteControlBus, false);

    /* access expected to fail because of lack of trust */
    AppCannotCallOn(remoteControlBus, serviceBus);

    /* Set up the service provider to trust the remote control via the peer
     * type FROM_CERTIFICATE_AUTHORITY.
     */
    ClearPeerKeys(remoteControlBus, serviceBus);
    ClearPeerKeys(serviceBus, remoteControlBus);
    KeyInfoNISTP256 consumerCA;
    consumerBus.GetPermissionConfigurator().GetSigningPublicKey(consumerCA);
    AddSpecificCertAuthorityToPolicy(servicePolicy, consumerCA);
    InstallPolicyToService(servicePolicy);
    InstallMembershipToServiceProvider();

    KeyInfoNISTP256 adminCA;
    adminBus.GetPermissionConfigurator().GetSigningPublicKey(adminCA);
    PermissionPolicy rcPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicyWithGuestServices(consumerBus, remoteControlBus, rcPolicy, false, adminCA)) << "GeneratePolicy failed.";
    InstallPolicyToClientBus(consumerBus, remoteControlBus, rcPolicy);

    InstallMembershipChainToTarget(adminBus, consumerBus, remoteControlBus, membershipSerial0, membershipSerial1, membershipGUID1);
    /* remote control can access the service provider */
    AnyUserCanCallOnAndNotOff(remoteControlBus);
    /* remote control can access specific right for the given CA */
    AppCanSetTVVolume(remoteControlBus, serviceBus, 21);
}

TEST_F(PermissionMgmtUseCaseTest, TestResetPolicy)
{
    Claims(false);

    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, consumerBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);
    ResetPolicyFromApp(adminBus, serviceBus);
}


TEST_F(PermissionMgmtUseCaseTest, RetrievePropertyApplicationState)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, consumerBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    AppHasAllowAllManifest(adminBus, serviceBus);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    RetrievePropertyApplicationState(adminBus, serviceBus, PermissionConfigurator::CLAIMED);
}

TEST_F(PermissionMgmtUseCaseTest, RetrievePropertyClaimCapabilities)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, consumerBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    AppHasAllowAllManifest(adminBus, serviceBus);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    RetrievePropertyClaimCapabilities(adminBus, serviceBus, PermissionConfigurator::CAPABLE_ECDHE_NULL);

    SetPropertyClaimCapabilities(serviceBus, PermissionConfigurator::CAPABLE_ECDHE_PSK | PermissionConfigurator::CAPABLE_ECDHE_NULL);
    RetrievePropertyClaimCapabilities(adminBus, serviceBus, PermissionConfigurator::CAPABLE_ECDHE_PSK | PermissionConfigurator::CAPABLE_ECDHE_NULL);

}

TEST_F(PermissionMgmtUseCaseTest, RetrievePropertyClaimCapabilityAdditionalInfo)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, consumerBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    AppHasAllowAllManifest(adminBus, serviceBus);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    SetPropertyClaimCapabilityAdditionalInfo(serviceBus, PermissionConfigurator::PSK_GENERATED_BY_APPLICATION);
    RetrievePropertyClaimCapabilityAdditionalInfo(adminBus, serviceBus, PermissionConfigurator::PSK_GENERATED_BY_APPLICATION);

}

TEST_F(PermissionMgmtUseCaseTest, AnonymousAccess)
{
    Claims(false);
    /* generate policy that allows PEER_ALL */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GenerateAllowAllPeersPolicy(adminBus, serviceBus, policy)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAnonymousAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    EnableSecurity("ALLJOYN_ECDHE_NULL");
    AnyUserCanCallOnAndNotOff(consumerBus);
}

TEST_F(PermissionMgmtUseCaseTest, PSKAccess)
{
    Claims(true);  /* use PSK */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GenerateAllowAllPeersPolicy(adminBus, serviceBus, policy)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateFullAnonymousAccessOutgoingPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    EnableSecurity("ALLJOYN_ECDHE_PSK");
    AppCanCallOn(consumerBus, serviceBus);
    AppCanCallTVOff(consumerBus, serviceBus);
}

TEST_F(PermissionMgmtUseCaseTest, ClaimFailsWithoutSecurityEnabled)
{
    qcc::GUID128 guid;
    IdentityCertificate certs[1];
    Crypto_ECC ecc;
    ecc.GenerateDSAKeyPair();
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(ecc.GetDSAPublicKey());
    KeyInfoHelper::GenerateKeyId(keyInfo);

    certs[0].SetSerial((const uint8_t*) "a", 1);
    certs[0].SetIssuerCN((const uint8_t*) "b", 1);
    certs[0].SetSubjectCN((const uint8_t*) "c", 1);
    certs[0].SetIssuerOU((const uint8_t*) "d", 1);
    certs[0].SetSubjectOU((const uint8_t*) "e", 1);
    certs[0].SetSubjectPublicKey(keyInfo.GetPublicKey());
    certs[0].SetCA(false);
    CertificateX509::ValidPeriod validity;
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + 24 * 3600;
    certs[0].SetValidity(&validity);
    EXPECT_EQ(ER_OK, certs[0].SignAndGenerateAuthorityKeyId(ecc.GetDSAPrivateKey(), keyInfo.GetPublicKey())) << " sign cert failed.";
    PermissionPolicy::Rule* manifest = NULL;
    size_t manifestSize = 0;
    GenerateAllowAllManifest(&manifest, &manifestSize);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(adminBus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    SecurityApplicationProxy saProxy(adminBus, consumerBus.GetUniqueName().c_str());
    EXPECT_NE(ER_OK, saProxy.Claim(keyInfo, guid, keyInfo, certs, 1, manifest, manifestSize)) << " saProxy.Claim failed";
    delete [] manifest;
}

TEST_F(PermissionMgmtUseCaseTest, GetApplicationStateBeforeEnableSecurity)
{
    PermissionConfigurator& pc = adminBus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationState;
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, pc.GetApplicationState(applicationState));
}

TEST_F(PermissionMgmtUseCaseTest, DisconnectAndReenableSecurity)
{
    Claims(false);
    /* stop the bus */
    EXPECT_EQ(ER_OK, consumerBus.EnablePeerSecurity("", NULL, NULL, true)) << "adminBus.EnablePeerSecurity with empty auth mechanism failed.";
    TeardownBus(consumerBus);
    /* restart the bus */
    SetupBus(consumerBus);
    DefaultECDHEAuthListener cpConsumerAuthListener;
    EXPECT_EQ(ER_OK, consumerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &cpConsumerAuthListener, NULL, false)) << "adminBus.EnablePeerSecurity failed.";

    BusAttachment cpAdminBus("CopyPermissionMgmtTestAdmin", false);
    InMemoryKeyStoreListener cpAdminKSListener = adminKeyStoreListener;
    SetupBus(cpAdminBus);
    EXPECT_EQ(ER_OK, cpAdminBus.RegisterKeyStoreListener(cpAdminKSListener)) << " RegisterKeyStoreListener failed";
    DefaultECDHEAuthListener cpAdminAuthListener;
    EXPECT_EQ(ER_OK, cpAdminBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &cpAdminAuthListener, NULL, false)) << "cpAdminBus.EnablePeerSecurity failed.";

    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = PermissionMgmtTestHelper::JoinPeerSession(cpAdminBus, consumerBus, sessionId);
    EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);

    SecurityApplicationProxy saProxy(cpAdminBus, consumerBus.GetUniqueName().c_str());
    /* retrieve the default policy */
    PermissionPolicy policy;
    EXPECT_EQ(ER_OK, saProxy.GetDefaultPolicy(policy)) << "GetDefaultPolicy failed.";
    TeardownBus(cpAdminBus);
}

static QStatus CreateCert(const qcc::String& serial, const qcc::GUID128& issuer, const qcc::String& organization, const ECCPrivateKey* issuerPrivateKey, const ECCPublicKey* issuerPublicKey, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, CertificateX509::ValidPeriod& validity, bool isCA, CertificateX509& cert)
{
    QStatus status = ER_CRYPTO_ERROR;

    cert.SetSerial((const uint8_t*)serial.data(), serial.size());
    qcc::String issuerName = issuer.ToString();
    cert.SetIssuerCN((const uint8_t*) issuerName.c_str(), issuerName.length());
    qcc::String subjectName = subject.ToString();
    cert.SetSubjectCN((const uint8_t*) subjectName.c_str(), subjectName.length());
    if (!organization.empty()) {
        cert.SetIssuerOU((const uint8_t*) organization.c_str(), organization.length());
        cert.SetSubjectOU((const uint8_t*) organization.c_str(), organization.length());
    }
    cert.SetSubjectPublicKey(subjectPubKey);
    cert.SetCA(isCA);
    cert.SetValidity(&validity);
    status = cert.SignAndGenerateAuthorityKeyId(issuerPrivateKey, issuerPublicKey);
    return status;
}

TEST_F(PermissionMgmtUseCaseTest, ValidCertChainStructure)
{

    CertificateX509::ValidPeriod validity;
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + 24 * 3600;

    qcc::GUID128 subject1;
    Crypto_ECC ecc1;
    ecc1.GenerateDSAKeyPair();
    CertificateX509 cert1;

    /* self signed cert */
    ASSERT_EQ(ER_OK, CreateCert("1010101", subject1, "organization", ecc1.GetDSAPrivateKey(), ecc1.GetDSAPublicKey(), subject1, ecc1.GetDSAPublicKey(), validity, true, cert1)) << " CreateCert failed.";

    qcc::GUID128 subject0;
    Crypto_ECC ecc0;
    ecc0.GenerateDSAKeyPair();
    IdentityCertificate cert0;

    /* leaf cert signed by cert1 */
    ASSERT_EQ(ER_OK, CreateCert("2020202", subject1, "organization", ecc1.GetDSAPrivateKey(), ecc1.GetDSAPublicKey(), subject0, ecc0.GetDSAPublicKey(), validity, false, cert0)) << " CreateCert failed.";

    CertificateX509 certs[2];
    certs[0] = cert0;
    certs[1] = cert1;

    EXPECT_TRUE(KeyExchangerECDHE_ECDSA::IsCertChainStructureValid(certs, 2)) << " cert chain structure is not valid";

}

TEST_F(PermissionMgmtUseCaseTest, ClaimWithInvalidCertChain)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL");
    GenerateCAKeys();
    IdentityCertificate identityCert;
    PermissionPolicy::Rule* manifest = NULL;
    size_t manifestSize = 0;
    GenerateAllowAllManifest(&manifest, &manifestSize);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(adminBus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
    SecurityApplicationProxy saProxy(adminBus, serviceBus.GetUniqueName().c_str());
    /* retrieve public key from to-be-claimed app to create identity cert */
    ECCPublicKey claimedPubKey;
    EXPECT_EQ(ER_OK, saProxy.GetEccPublicKey(claimedPubKey)) << " Fail to retrieve to-be-claimed public key.";
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(adminBus, "1010", "subject", &claimedPubKey, "service alias", 3600, identityCert, digest, Crypto_SHA256::DIGEST_SIZE)) << "  CreateIdentityCert failed.";

    IdentityCertificate signingCert;
    ECCPublicKey consumerPubKey;
    SecurityApplicationProxy consumerProxy(adminBus, consumerBus.GetUniqueName().c_str());
    EXPECT_EQ(ER_OK, consumerProxy.GetEccPublicKey(consumerPubKey)) << " Fail to retrieve to-be-claimed public key.";
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(consumerBus, "1011", "signer", &consumerPubKey, "consumer alias", 3600, signingCert, digest, Crypto_SHA256::DIGEST_SIZE)) << "  CreateIdentityCert failed.";

    IdentityCertificate certChain[2];
    certChain[0] = identityCert;
    certChain[1] = signingCert;
    EXPECT_EQ(ER_INVALID_CERTIFICATE, saProxy.Claim(adminAdminGroupAuthority, adminAdminGroupGUID, adminAdminGroupAuthority, certChain, 2, manifest, manifestSize)) << "Claim did not fail.";
    /* do a second claim and expect the same error code returned */
    EXPECT_EQ(ER_INVALID_CERTIFICATE, saProxy.Claim(adminAdminGroupAuthority, adminAdminGroupGUID, adminAdminGroupAuthority, certChain, 2, manifest, manifestSize)) << "Claim did not fail.";
}

TEST_F(PermissionMgmtUseCaseTest, InvalidCertChainStructure)
{

    CertificateX509::ValidPeriod validity;
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + 24 * 3600;

    qcc::GUID128 subject1;
    Crypto_ECC ecc1;
    ecc1.GenerateDSAKeyPair();
    CertificateX509 certs[2];

    /* self signed cert */
    ASSERT_EQ(ER_OK, CreateCert("1010101", subject1, "organization", ecc1.GetDSAPrivateKey(), ecc1.GetDSAPublicKey(), subject1, ecc1.GetDSAPublicKey(), validity, false, certs[1])) << " CreateCert failed.";

    qcc::GUID128 subject0;
    Crypto_ECC ecc0;
    ecc0.GenerateDSAKeyPair();

    /* leaf cert signed by cert1 */
    ASSERT_EQ(ER_OK, CreateCert("2020202", subject1, "organization", ecc1.GetDSAPrivateKey(), ecc1.GetDSAPublicKey(), subject0, ecc0.GetDSAPublicKey(), validity, false, certs[0])) << " CreateCert failed.";

    EXPECT_FALSE(KeyExchangerECDHE_ECDSA::IsCertChainStructureValid(certs, 2)) << " cert chain structure is not valid";

}

TEST_F(PermissionMgmtUseCaseTest, InstallMembershipBeforeClaimMustFail)
{
    GenerateCAKeys();
    EnableSecurity("ALLJOYN_ECDHE_NULL");
    InstallMembershipToAdmin(adminMembershipSerial1, adminAdminGroupGUID, adminBus, false);
}

TEST_F(PermissionMgmtUseCaseTest, ClaimWithIdentityCertSignedByUnknownCA)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA");
    ClaimAdmin();
    SecurityApplicationProxy saProxy(adminBus, consumerBus.GetUniqueName().c_str());
    /* retrieve public key from to-be-claimed app to create identity cert */
    ECCPublicKey claimedPubKey;
    EXPECT_EQ(ER_OK, saProxy.GetEccPublicKey(claimedPubKey)) << " Fail to retrieve to-be-claimed public key.";
    qcc::GUID128 guid;
    PermissionMgmtTestHelper::GetGUID(consumerBus, guid);
    IdentityCertificate identityCertChain[3];
    size_t certChainCount = 3;
    PermissionPolicy::Rule* manifest = NULL;
    size_t manifestSize = 0;
    GenerateAllowAllManifest(&manifest, &manifestSize);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(adminBus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCertChain(remoteControlBus, adminBus, "303030", guid.ToString(), &claimedPubKey, "alias", 3600, identityCertChain, certChainCount, digest, Crypto_SHA256::DIGEST_SIZE)) << "  CreateIdentityCert failed";

    Crypto_ECC ecc;
    ecc.GenerateDSAKeyPair();
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(ecc.GetDSAPublicKey());
    KeyInfoHelper::GenerateKeyId(keyInfo);
    EXPECT_EQ(ER_OK, saProxy.Claim(keyInfo, consumerAdminGroupGUID, consumerAdminGroupAuthority, identityCertChain, certChainCount, manifest, manifestSize)) << " saProxy.Claim failed";
    delete [] manifest;
}

TEST_F(PermissionMgmtUseCaseTest, ClaimWithEmptyCAPublicKey)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA");
    GenerateCAKeys();
    SecurityApplicationProxy saProxy(adminBus, adminBus.GetUniqueName().c_str());
    /* retrieve public key from to-be-claimed app to create identity cert */
    ECCPublicKey claimedPubKey;
    EXPECT_EQ(ER_OK, saProxy.GetEccPublicKey(claimedPubKey)) << " Fail to retrieve to-be-claimed public key.";
    qcc::GUID128 guid;
    IdentityCertificate identityCertChain[1];
    size_t certChainCount = 1;
    PermissionPolicy::Rule* manifest = NULL;
    size_t manifestSize = 0;
    GenerateAllowAllManifest(&manifest, &manifestSize);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(adminBus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
    certChainCount = 1;
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(adminBus, "1010", guid.ToString(), &claimedPubKey, "alias", 3600, identityCertChain[0], digest, Crypto_SHA256::DIGEST_SIZE)) << "  CreateIdentityCert failed.";

    KeyInfoNISTP256 emptyKeyInfo;
    EXPECT_EQ(ER_BAD_ARG_1, saProxy.Claim(emptyKeyInfo, adminAdminGroupGUID, adminAdminGroupAuthority, identityCertChain, certChainCount, manifest, manifestSize)) << "Claim did not fail.";
}

TEST_F(PermissionMgmtUseCaseTest, ClaimWithEmptyCAAKI)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA");
    GenerateCAKeys();
    SecurityApplicationProxy saProxy(adminBus, adminBus.GetUniqueName().c_str());
    /* retrieve public key from to-be-claimed app to create identity cert */
    ECCPublicKey claimedPubKey;
    EXPECT_EQ(ER_OK, saProxy.GetEccPublicKey(claimedPubKey)) << " Fail to retrieve to-be-claimed public key.";
    qcc::GUID128 guid;
    IdentityCertificate identityCertChain[1];
    size_t certChainCount = 1;
    PermissionPolicy::Rule* manifest = NULL;
    size_t manifestSize = 0;
    GenerateAllowAllManifest(&manifest, &manifestSize);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(adminBus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
    certChainCount = 1;
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(adminBus, "1010", guid.ToString(), &claimedPubKey, "alias", 3600, identityCertChain[0], digest, Crypto_SHA256::DIGEST_SIZE)) << "  CreateIdentityCert failed.";

    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(&claimedPubKey);
    EXPECT_EQ(ER_BAD_ARG_2, saProxy.Claim(keyInfo, adminAdminGroupGUID, adminAdminGroupAuthority, identityCertChain, certChainCount, manifest, manifestSize)) << "Claim did not fail.";
}

TEST_F(PermissionMgmtUseCaseTest, ClaimWithEmptyAdminSecurityGroupAKI)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA");
    GenerateCAKeys();
    SecurityApplicationProxy saProxy(adminBus, adminBus.GetUniqueName().c_str());
    /* retrieve public key from to-be-claimed app to create identity cert */
    ECCPublicKey claimedPubKey;
    EXPECT_EQ(ER_OK, saProxy.GetEccPublicKey(claimedPubKey)) << " Fail to retrieve to-be-claimed public key.";
    qcc::GUID128 guid;
    IdentityCertificate identityCertChain[1];
    size_t certChainCount = 1;
    PermissionPolicy::Rule* manifest = NULL;
    size_t manifestSize = 0;
    GenerateAllowAllManifest(&manifest, &manifestSize);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(adminBus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
    certChainCount = 1;
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(adminBus, "1010", guid.ToString(), &claimedPubKey, "alias", 3600, identityCertChain[0], digest, Crypto_SHA256::DIGEST_SIZE)) << "  CreateIdentityCert failed.";

    KeyInfoNISTP256 emptyKeyInfo;
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(&claimedPubKey);
    EXPECT_EQ(ER_BAD_ARG_5, saProxy.Claim(adminAdminGroupAuthority, adminAdminGroupGUID, keyInfo, identityCertChain, certChainCount, manifest, manifestSize)) << "Claim did not fail.";
}

TEST_F(PermissionMgmtUseCaseTest, ClaimWithEmptyAdminSecurityGroupPublicKey)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA");
    GenerateCAKeys();
    SecurityApplicationProxy saProxy(adminBus, adminBus.GetUniqueName().c_str());
    /* retrieve public key from to-be-claimed app to create identity cert */
    ECCPublicKey claimedPubKey;
    EXPECT_EQ(ER_OK, saProxy.GetEccPublicKey(claimedPubKey)) << " Fail to retrieve to-be-claimed public key.";
    qcc::GUID128 guid;
    IdentityCertificate identityCertChain[1];
    size_t certChainCount = 1;
    PermissionPolicy::Rule* manifest = NULL;
    size_t manifestSize = 0;
    GenerateAllowAllManifest(&manifest, &manifestSize);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(adminBus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
    certChainCount = 1;
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(adminBus, "1010", guid.ToString(), &claimedPubKey, "alias", 3600, identityCertChain[0], digest, Crypto_SHA256::DIGEST_SIZE)) << "  CreateIdentityCert failed.";

    KeyInfoNISTP256 emptyKeyInfo;
    EXPECT_EQ(ER_BAD_ARG_4, saProxy.Claim(adminAdminGroupAuthority, adminAdminGroupGUID, emptyKeyInfo, identityCertChain, certChainCount, manifest, manifestSize)) << "Claim did not fail.";
}

TEST_F(PermissionMgmtUseCaseTest, ResetAndCopyKeyStore)
{
    Claims(false);

    SecurityApplicationProxy saProxy(adminBus, consumerBus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationState;
    EXPECT_EQ(ER_OK, saProxy.GetApplicationState(applicationState)) << "  GetApplicationState failed.";

    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationState) << " The application state is not claimed.";

    EXPECT_EQ(ER_OK, saProxy.Reset()) << "  Reset failed.";
    /* close the consumer bus */
    TeardownBus(consumerBus);

    /* create a copy the consumer bus */
    BusAttachment cpConsumerBus("CopyPermissionMgmtTestConsumer", false);
    InMemoryKeyStoreListener cpConsumerListener = consumerKeyStoreListener;

    SetupBus(cpConsumerBus);
    EXPECT_EQ(ER_OK, cpConsumerBus.RegisterKeyStoreListener(cpConsumerListener)) << " RegisterKeyStoreListener failed.";
    DefaultECDHEAuthListener cpConsumerAuthListener;
    EXPECT_EQ(ER_OK, cpConsumerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &cpConsumerAuthListener, NULL, false)) << "cpConsumerBus.EnablePeerSecurity failed.";

    PermissionConfigurator& pc = cpConsumerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pc.GetApplicationState(applicationState)) << "  GetApplicationState failed.";
    EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationState) << " The application state is not claimable.";

    TeardownBus(cpConsumerBus);
}

TEST_F(PermissionMgmtUseCaseTest, GetAllPropertiesFailByOutgoingPolicy)
{
    Claims(false);

    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateNoOutboundGetPropertyPolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCannotGetTVCaption();
}

TEST_F(PermissionMgmtUseCaseTest, GetAllPropertiesAllowed)
{
    Claims(false);

    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GenerateGetAllPropertiesObservePolicy(adminBus, serviceBus, policy)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateGetAllPropertiesProvidePolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanGetTVCaption();
}

TEST_F(PermissionMgmtUseCaseTest, GetAllPropertiesPriorToClaim)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL");
    SecurityApplicationProxy saProxy(adminBus, serviceBus.GetUniqueName().c_str());
    MsgArg props;
    EXPECT_EQ(ER_OK, saProxy.GetAllProperties(org::alljoyn::Bus::Security::Application::InterfaceName, props));
}

TEST_F(PermissionMgmtUseCaseTest, GetAllPropertiesWithAtLeastOnePropertyAllowedByProviderPolicy)
{
    Claims(false);

    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateGetAllPropertiesProvidePolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanGetTVCaption();
}

TEST_F(PermissionMgmtUseCaseTest, GetAllPropertiesNotAllowedByProviderPolicy)
{
    Claims(false);

    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateGetAllPropertiesProvidePolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCannotGetTVCaption();
}

TEST_F(PermissionMgmtUseCaseTest, GetAllPropertiesFailByProviderManifest)
{
    Claims(false);

    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateGetAllPropertiesProvidePolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();

    PermissionPolicy::Rule* manifest;
    size_t manifestSize;
    GenerateManifestNoGetAllProperties(&manifest, &manifestSize);
    ReplaceIdentityCert(adminBus, serviceBus, manifest, manifestSize, false);
    delete [] manifest;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCannotGetTVCaption();
}

TEST_F(PermissionMgmtUseCaseTest, GetAllPropertiesFailByConsumerManifest)
{
    Claims(false);

    /* generate a policy */
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, GeneratePolicy(adminBus, serviceBus, policy, adminBus)) << "GeneratePolicy failed.";
    InstallPolicyToService(policy);

    PermissionPolicy consumerPolicy;
    ASSERT_EQ(ER_OK, GenerateGetAllPropertiesProvidePolicy(adminBus, consumerBus, consumerPolicy)) << "GeneratePolicy failed.";
    InstallPolicyToConsumer(consumerPolicy);

    InstallMembershipToConsumer();

    PermissionPolicy::Rule* manifest;
    size_t manifestSize;
    GenerateManifestNoGetAllProperties(&manifest, &manifestSize);
    ReplaceIdentityCert(adminBus, consumerBus, manifest, manifestSize, false);
    delete [] manifest;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCannotGetTVCaption();
}

TEST_F(PermissionMgmtUseCaseTest, GetEmptyManifestTemplateBeforeClaim)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL");
    SecurityApplicationProxy saProxy(adminBus, serviceBus.GetUniqueName().c_str());
    MsgArg arg;
    EXPECT_EQ(ER_OK, saProxy.GetManifestTemplate(arg)) << "GetManifestTemplate failed.";
    ASSERT_EQ(static_cast<size_t>(0), arg.v_array.GetNumElements()) << "the manifest template is supposed to be empty.";
}

TEST_F(PermissionMgmtUseCaseTest, GetEmptyManifestTemplateDigestBeforeClaim)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL");
    SecurityApplicationProxy saProxy(adminBus, serviceBus.GetUniqueName().c_str());
    /* retrieve the manifest template digest and expect to be empty */
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, saProxy.GetManifestTemplateDigest(digest, 0)) << "SetPermissionManifest GetManifestTemplateDigest failed.";
}

TEST_F(PermissionMgmtUseCaseTest, GetEmptyCertificateIdBeforeClaim)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL");
    SecurityApplicationProxy saProxy(adminBus, serviceBus.GetUniqueName().c_str());
    qcc::String serialNum;
    KeyInfoNISTP256 keyInfo;
    EXPECT_EQ(ER_OK, saProxy.GetIdentityCertificateId(serialNum, keyInfo));
    EXPECT_TRUE(keyInfo.GetPublicKey()->empty());
}

