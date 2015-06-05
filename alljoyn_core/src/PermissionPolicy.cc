/**
 * @file
 * This file defines the Permission DB classes that provide the interface to
 * parse the authorization data
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

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Message.h>
#include <qcc/Debug.h>
#include <qcc/StringUtil.h>
#include <qcc/Crypto.h>
#include <alljoyn/PermissionPolicy.h>
#include <alljoyn/BusAttachment.h>
#include "KeyInfoHelper.h"

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

qcc::String PermissionPolicy::Rule::Member::ToString() const
{
    qcc::String str;
    str += "Member:\n";
    if (memberName.length() > 0) {
        str += "  memberName: " + memberName + "\n";
    }
    if (memberType == METHOD_CALL) {
        str += "  method call\n";
    } else if (memberType == SIGNAL) {
        str += "  signal\n";
    } else if (memberType == PROPERTY) {
        str += "  property\n";
    }
    str += "  action mask:";
    if ((actionMask & ACTION_PROVIDE) == ACTION_PROVIDE) {
        str += " Provide";
    }
    if ((actionMask & ACTION_OBSERVE) == ACTION_OBSERVE) {
        str += " Observe";
    }
    if ((actionMask & ACTION_MODIFY) == ACTION_MODIFY) {
        str += " Modify";
    }
    str += "\n";
    return str;
}

qcc::String PermissionPolicy::Rule::ToString() const
{
    qcc::String str;
    str += "Rule:\n";
    if (objPath.length() > 0) {
        str += "  objPath: " + objPath + "\n";
    }
    if (interfaceName.length() > 0) {
        str += "  interfaceName: " + interfaceName + "\n";
    }
    for (size_t cnt = 0; cnt < GetMembersSize(); cnt++) {
        str += members[cnt].ToString();
    }
    return str;
}

qcc::String PermissionPolicy::Peer::ToString() const
{
    qcc::String str;
    str += "Peer:\n";
    if (type == PEER_ALL) {
        str += "  type: ALL\n";
    } else if (type == PEER_ANY_TRUSTED) {
        str += "  type: ANY_TRUSTED\n";
    } else if (type == PEER_FROM_CERTIFICATE_AUTHORITY) {
        str += "  type: FROM_CERTIFICATE_AUTHORITY\n";
        if (keyInfo) {
            str += keyInfo->ToString();
        }
    } else if (type == PEER_WITH_PUBLIC_KEY) {
        str += "  type: WITH_PUBLIC_KEY\n";
        if (keyInfo) {
            str += keyInfo->ToString();
        }
    } else if (type == PEER_WITH_MEMBERSHIP) {
        str += "  type: WITH_MEMBERSHIP security group Id: " + securityGroupId.ToString() + "\n";
        if (keyInfo) {
            str += keyInfo->ToString();
        }
    }
    return str;
}

qcc::String PermissionPolicy::Acl::ToString() const
{
    qcc::String str;
    str += "Acl:\n";
    if ((peersSize > 0) && peers) {
        for (size_t cnt = 0; cnt < peersSize; cnt++) {
            str += "  peers[" + U32ToString(cnt) + "]: " + peers[cnt].ToString();
        }
    }
    if ((rulesSize > 0) && rules) {
        for (size_t cnt = 0; cnt < rulesSize; cnt++) {
            str += "  rules[" + U32ToString(cnt) + "]: " + rules[cnt].ToString();
        }
    }
    return str;
}

qcc::String PermissionPolicy::ToString() const
{
    qcc::String str;
    str += "PermissionPolicy:\n";
    str += "  specification version: " +  U32ToString(specificationVersion) + "\n";
    str += "  version: " + U32ToString(version) + "\n";

    if ((aclsSize > 0) && acls) {
        for (size_t cnt = 0; cnt < aclsSize; cnt++) {
            str += "  acls[" + U32ToString(cnt) + "]: " + acls[cnt].ToString();
        }
    }
    return str;
}

static QStatus GeneratePeerArgs(MsgArg** retArgs, PermissionPolicy::Peer* peers, size_t count)
{
    if (count == 0) {
        *retArgs = NULL;
        return ER_OK;
    }
    *retArgs = new MsgArg[count];
    QStatus status = ER_OK;
    for (size_t cnt = 0; cnt < count; cnt++) {
        MsgArg* keyInfoArg = NULL;
        size_t keyInfoCount = 0;
        if ((peers[cnt].GetType() != PermissionPolicy::Peer::PEER_ALL) &&
            (peers[cnt].GetType() != PermissionPolicy::Peer::PEER_ANY_TRUSTED)) {
            const KeyInfoECC* keyInfo = peers[cnt].GetKeyInfo();
            if (!keyInfo) {
                status = ER_INVALID_DATA;
                break;
            }
            if (!KeyInfoHelper::InstanceOfKeyInfoNISTP256(*keyInfo)) {
                status = ER_NOT_IMPLEMENTED;
                break;
            }
            KeyInfoNISTP256* keyInfoNISTP256 = (qcc::KeyInfoNISTP256*) keyInfo;
            keyInfoCount = 1;
            keyInfoArg = new MsgArg[keyInfoCount];
            KeyInfoHelper::KeyInfoNISTP256PubKeyToMsgArg(*keyInfoNISTP256, keyInfoArg[0]);
        }
        const uint8_t* securityGroupId = NULL;
        size_t securityGroupLen = 0;
        if (peers[cnt].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) {
            securityGroupId = peers[cnt].GetSecurityGroupId().GetBytes();
            securityGroupLen = GUID128::SIZE;
        }
        status = (*retArgs)[cnt].Set("(ya(yyayay)ay)",
                                     peers[cnt].GetType(), keyInfoCount, keyInfoArg,
                                     securityGroupLen, securityGroupId);
        if (ER_OK != status) {
            delete [] keyInfoArg;
            break;
        }
        (*retArgs)[cnt].SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }
    if (ER_OK != status) {
        delete [] *retArgs;
        *retArgs = NULL;
    }
    return status;
}

static QStatus BuildPeersFromArg(MsgArg* arg, PermissionPolicy::Peer** peers, size_t count)
{
    if (count == 0) {
        *peers = NULL;
        return ER_OK;
    }
    *peers = new PermissionPolicy::Peer[count];
    QStatus status = ER_OK;
    for (size_t cnt = 0; cnt < count; cnt++) {
        uint8_t peerType;
        MsgArg* pubKeys;
        size_t pubKeysCnt;
        size_t sgIdLen;
        uint8_t* sgId;
        status = arg[cnt].Get("(ya(yyayay)ay)", &peerType, &pubKeysCnt, &pubKeys, &sgIdLen, &sgId);
        if (ER_OK != status) {
            break;
        }
        if ((peerType >= PermissionPolicy::Peer::PEER_ALL) && (peerType <= PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP)) {
            (*peers)[cnt].SetType((PermissionPolicy::Peer::PeerType) peerType);
        } else {
            status = ER_INVALID_DATA;
            break;
        }
        if (peerType == PermissionPolicy::Peer::PEER_ALL) {
            continue;
        } else if (peerType == PermissionPolicy::Peer::PEER_ANY_TRUSTED) {
            continue;
        } else if (pubKeysCnt == 0) {
            status = ER_INVALID_DATA;
            break;
        }
        if ((peerType == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) &&
            (sgIdLen != GUID128::SIZE)) {
            status = ER_INVALID_DATA;
            break;
        }
        KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
        status = KeyInfoHelper::MsgArgToKeyInfoNISTP256PubKey(pubKeys[0], *keyInfo);
        if (ER_OK != status) {
            delete keyInfo;
            break;
        }
        (*peers)[cnt].SetKeyInfo(keyInfo);

        if (peerType == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) {
            GUID128 guid(0);
            guid.SetBytes(sgId);
            (*peers)[cnt].SetSecurityGroupId(guid);
        }
    }

    if (ER_OK != status) {
        delete [] *peers;
        *peers = NULL;
    }
    return status;
}

static QStatus GenerateMemberArgs(MsgArg** retArgs, const PermissionPolicy::Rule::Member* members, size_t count)
{
    if (count == 0) {
        *retArgs = NULL;
        return ER_OK;
    }
    *retArgs = new MsgArg[count];
    for (size_t cnt = 0; cnt < count; cnt++) {
        QStatus status = (*retArgs)[cnt].Set("(syy)",
                                             members[cnt].GetMemberName().c_str(), members[cnt].GetMemberType(),
                                             members[cnt].GetActionMask());
        if (ER_OK != status) {
            delete [] *retArgs;
            *retArgs = NULL;
            return status;
        }
    }
    return ER_OK;
}

static QStatus GenerateRuleArgs(MsgArg** retArgs, PermissionPolicy::Rule* rules, size_t count)
{
    QStatus status = ER_OK;
    if (count == 0) {
        *retArgs = NULL;
        return status;
    }
    *retArgs = new MsgArg[count];
    for (size_t cnt = 0; cnt < count; cnt++) {
        MsgArg* ruleMembersArgs = NULL;
        if (rules[cnt].GetMembersSize() > 0) {
            status = GenerateMemberArgs(&ruleMembersArgs, rules[cnt].GetMembers(), rules[cnt].GetMembersSize());
            if (ER_OK != status) {
                delete [] ruleMembersArgs;
                goto exit;
            }
        }
        status = (*retArgs)[cnt].Set("(ssa(syy))",
                                     rules[cnt].GetObjPath().c_str(),
                                     rules[cnt].GetInterfaceName().c_str(),
                                     rules[cnt].GetMembersSize(), ruleMembersArgs);
        if (ER_OK != status) {
            delete [] ruleMembersArgs;
            goto exit;
        }
        /* make sure having own copy of the string and array args */
        (*retArgs)[cnt].Stabilize();
        delete [] ruleMembersArgs;  /* clean memory since it has been copied */
    }
    return ER_OK;
exit:
    delete [] *retArgs;
    *retArgs = NULL;
    return status;
}

static QStatus BuildMembersFromArg(const MsgArg* arg, PermissionPolicy::Rule::Member** members, size_t count)
{
    if (count == 0) {
        *members = NULL;
        return ER_OK;
    }
    *members = new PermissionPolicy::Rule::Member[count];
    QStatus status = ER_OK;
    for (size_t cnt = 0; cnt < count; cnt++) {
        char* str;
        uint8_t memberType;
        uint8_t actionMask;
        status = arg[cnt].Get("(syy)", &str, &memberType, &actionMask);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildMembersFromArg [%d] got status 0x%x\n", cnt, status));
            break;
        }
        PermissionPolicy::Rule::Member* pr = &(*members)[cnt];
        pr->SetMemberName(String(str));
        if ((memberType >= PermissionPolicy::Rule::Member::NOT_SPECIFIED) && (memberType <= PermissionPolicy::Rule::Member::PROPERTY)) {
            pr->SetMemberType((PermissionPolicy::Rule::Member::MemberType) memberType);
        } else {
            QCC_DbgPrintf(("BuildMembersFromArg [%d] got invalid member type %d\n", cnt, memberType));
            status = ER_INVALID_DATA;
            break;
        }
        pr->SetActionMask(actionMask);
    }

    if (ER_OK != status) {
        delete [] *members;
        *members = NULL;
    }
    return status;
}

static QStatus BuildRulesFromArgArray(const MsgArg* args, size_t argCount, PermissionPolicy::Rule** rules)
{
    if (argCount == 0) {
        *rules = NULL;
        return ER_OK;
    }

    *rules = new PermissionPolicy::Rule[argCount];
    QStatus status = ER_OK;
    for (size_t cnt = 0; cnt < argCount; cnt++) {
        char* objPath;
        char* interfaceName;
        MsgArg* membersArgs = NULL;
        size_t membersArgsCount = 0;
        status = args[cnt].Get("(ssa(syy))", &objPath, &interfaceName, &membersArgsCount, &membersArgs);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildRulesFromArg [%d] got status 0x%x\n", cnt, status));
            break;
        }
        (*rules)[cnt].SetObjPath(String(objPath));
        (*rules)[cnt].SetInterfaceName(String(interfaceName));
        if (membersArgsCount > 0) {
            PermissionPolicy::Rule::Member* memberRules = NULL;
            status = BuildMembersFromArg(membersArgs, &memberRules, membersArgsCount);
            if (ER_OK != status) {
                QCC_DbgPrintf(("BuildRulesFromArg [%d] got status 0x%x\n", cnt, status));
                delete [] memberRules;
                break;
            }
            (*rules)[cnt].SetMembers(membersArgsCount, memberRules);
        }
    }

    if (ER_OK != status) {
        delete [] *rules;
        *rules = NULL;
    }
    return status;
}

static QStatus BuildRulesFromArg(const MsgArg& msgArg, PermissionPolicy::Rule** rules, size_t* count)
{
    MsgArg* args;
    size_t argCount;
    QStatus status = msgArg.Get("a(ssa(syy))", &argCount, &args);
    if (ER_OK != status) {
        return status;
    }
    *count = argCount;
    return BuildRulesFromArgArray(args, argCount, rules);
}

static QStatus BuildAclsFromArg(MsgArg* arg, PermissionPolicy::Acl** acls, size_t count)
{
    if (count == 0) {
        *acls = NULL;
        return ER_OK;
    }
    QStatus status = ER_OK;
    *acls = new PermissionPolicy::Acl[count];
    for (size_t cnt = 0; cnt < count; cnt++) {
        MsgArg* peersArgs;
        size_t peersArgsCount = 0;
        MsgArg* rulesArgs;
        size_t rulesArgsCount = 0;
        status = arg[cnt].Get("(a(ya(yyayay)ay)a(ssa(syy)))", &peersArgsCount, &peersArgs, &rulesArgsCount, &rulesArgs);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildAclsFromArg [%d] got status 0x%x\n", cnt, status));
            break;
        }
        if (peersArgsCount > 0) {
            PermissionPolicy::Peer* peers = NULL;
            status = BuildPeersFromArg(peersArgs, &peers, peersArgsCount);
            if (ER_OK != status) {
                QCC_DbgPrintf(("BuildAclsFromArg [%d] got status 0x%x\n", cnt, status));
                delete [] peers;
                break;
            }
            (*acls)[cnt].SetPeers(peersArgsCount, peers);
        }
        if (rulesArgsCount > 0) {
            PermissionPolicy::Rule* rules = NULL;
            status = BuildRulesFromArgArray(rulesArgs, rulesArgsCount, &rules);
            if (ER_OK != status) {
                QCC_DbgPrintf(("BuildProviderFromArg #6 [%d] got status 0x%x\n", cnt, status));
                delete [] rules;
                break;
            }
            (*acls)[cnt].SetRules(rulesArgsCount, rules);
        }
    }
    if (ER_OK != status) {
        delete [] *acls;
        *acls = NULL;
    }
    return status;
}

QStatus PermissionPolicy::Export(MsgArg& msgArg)
{
    QStatus status = ER_OK;
    MsgArg* aclsArgs = NULL;
    if (acls) {
        aclsArgs = new MsgArg[GetAclsSize()];
        for (size_t cnt = 0; cnt < GetAclsSize(); cnt++) {
            MsgArg* peersArgs = NULL;
            if (acls[cnt].GetPeers()) {
                status = GeneratePeerArgs(&peersArgs, (PermissionPolicy::Peer*) acls[cnt].GetPeers(), acls[cnt].GetPeersSize());
                if (ER_OK != status) {
                    delete [] peersArgs;
                    break;
                }
            }
            MsgArg* rulesArgs = NULL;
            if (acls[cnt].GetRules()) {
                status = GenerateRuleArgs(&rulesArgs, (PermissionPolicy::Rule*) acls[cnt].GetRules(), acls[cnt].GetRulesSize());
                if (ER_OK != status) {
                    delete [] peersArgs;
                    delete [] rulesArgs;
                    break;
                }
            }
            status = aclsArgs[cnt].Set("(a(ya(yyayay)ay)a(ssa(syy)))",
                                       acls[cnt].GetPeersSize(), peersArgs, acls[cnt].GetRulesSize(), rulesArgs);
            if (ER_OK != status) {
                delete [] peersArgs;
                delete [] rulesArgs;
                break;
            }
        }
    }

    if (ER_OK == status) {
        status = msgArg.Set("(qua(a(ya(yyayay)ay)a(ssa(syy))))",
                            GetSpecificationVersion(), GetVersion(), GetAclsSize(), aclsArgs);
        if (ER_OK == status) {
            msgArg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
        }
    }

    if (ER_OK != status) {
        delete [] aclsArgs;
    }
    return status;
}

QStatus PermissionPolicy::Import(uint16_t expectedVersion, const MsgArg& msgArg)
{
    uint16_t specVersion;
    MsgArg* aclsArgs;
    size_t aclsArgsCount = 0;
    uint32_t policyVersion;
    QStatus status = msgArg.Get("(qua(a(ya(yyayay)ay)a(ssa(syy))))",
                                &specVersion, &policyVersion, &aclsArgsCount, &aclsArgs);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionPolicy::Import got status 0x%x\n", status));
        return status;
    }
    if (specificationVersion != expectedVersion) {
        QCC_DbgPrintf(("PermissionPolicy::Import got unexcepted specification version %d\n", specVersion));
        return ER_INVALID_DATA;
    }
    SetSpecificationVersion(specVersion);
    SetVersion(policyVersion);

    if (aclsArgsCount > 0) {
        PermissionPolicy::Acl* aclArray = NULL;
        status = BuildAclsFromArg(aclsArgs, &aclArray, aclsArgsCount);
        if (ER_OK != status) {
            QCC_DbgPrintf(("PermissionPolicy::Import #4 got status 0x%x\n", status));
            delete [] aclArray;
            return status;
        }
        SetAcls(aclsArgsCount, aclArray);
    }

    return ER_OK;
}

QStatus DefaultPolicyMarshaller::MarshalPrep(PermissionPolicy& policy)
{
    MsgArg args;
    QStatus status = policy.Export(args);
    if (ER_OK != status) {
        return status;
    }
    /**
     * Use an error message as it is the simplest message without many validition rules.
     * The ALLJOYN_FLAG_SESSIONLESS is set in order to skip the serial number
     * check since the data can be stored for a long time*/
    msg->ErrorMsg("/", 0);
    MsgArg variant("v", &args);
    return msg->MarshalMessage("v", "", "", MESSAGE_ERROR, &variant, 1, ALLJOYN_FLAG_SESSIONLESS, 0);
}

QStatus DefaultPolicyMarshaller::Marshal(PermissionPolicy& policy, uint8_t** buf, size_t* size)
{
    *buf = NULL;
    *size = 0;
    QStatus status = MarshalPrep(policy);
    if (ER_OK != status) {
        return status;
    }
    *size = msg->GetBufferSize();
    *buf = new uint8_t[*size];
    if (!*buf) {
        *size = 0;
        return ER_OUT_OF_MEMORY;
    }
    memcpy(*buf, msg->GetBuffer(), *size);
    return ER_OK;
}

QStatus DefaultPolicyMarshaller::Unmarshal(PermissionPolicy& policy, const uint8_t* buf, size_t size)
{
    QStatus status = msg->LoadBytes((uint8_t*) buf, size);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionPolicy::Import (%d bytes) failed to load status 0x%x\n", size, status));
        return status;
    }
    qcc::String endpointName("local");
    status = msg->Unmarshal(endpointName, false, false, false, 0);
    if (ER_OK != status) {
        return status;
    }
    status = msg->UnmarshalArgs("*");
    if (ER_OK != status) {
        return status;
    }
    const MsgArg* arg = msg->GetArg(0);
    if (arg) {
        MsgArg* variant;
        status = arg->Get("v", &variant);
        if (ER_OK != status) {
            return status;
        }
        return policy.Import(PermissionPolicy::SPEC_VERSION, *variant);
    }
    return ER_INVALID_DATA;
}

QStatus DefaultPolicyMarshaller::Digest(PermissionPolicy& policy, uint8_t* digest, size_t len)
{
    if (len != Crypto_SHA256::DIGEST_SIZE) {
        return ER_INVALID_DATA;
    }
    Crypto_SHA256 hashUtil;
    QStatus status = MarshalPrep(policy);
    if (ER_OK != status) {
        return status;
    }
    status = hashUtil.Init();
    if (ER_OK != status) {
        return status;
    }
    status = hashUtil.Update(msg->GetBodyBuffer(), msg->GetBodyBufferSize());
    if (ER_OK != status) {
        return status;
    }
    return hashUtil.GetDigest(digest);
}

QStatus PermissionPolicy::Digest(Marshaller& marshaller, uint8_t* digest, size_t len)
{
    return marshaller.Digest(*this, digest, len);
}

QStatus PermissionPolicy::Export(Marshaller& marshaller, uint8_t** buf, size_t* size)
{
    return marshaller.Marshal(*this, buf, size);
}

QStatus PermissionPolicy::Import(Marshaller& marshaller, const uint8_t* buf, size_t size)
{
    return marshaller.Unmarshal(*this, buf, size);
}

QStatus PermissionPolicy::GenerateRules(const Rule* rules, size_t count, MsgArg& msgArg)
{
    MsgArg* rulesArgs = NULL;
    QStatus status = GenerateRuleArgs(&rulesArgs, (Rule*) rules, count);
    if (ER_OK != status) {
        return status;
    }
    msgArg.Set("a(ssa(syy))", count, rulesArgs);
    msgArg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    return status;
}

QStatus PermissionPolicy::ParseRules(const MsgArg& msgArg, Rule** rules, size_t* count)
{
    return BuildRulesFromArg(msgArg, rules, count);
}

PermissionPolicy::Rule::Member& PermissionPolicy::Rule::Member::operator=(const PermissionPolicy::Rule::Member& other) {
    if (&other != this) {
        memberName = other.memberName;
        memberType = other.memberType;
        actionMask = other.actionMask;
    }
    return *this;
}

PermissionPolicy::Rule::Member::Member(const PermissionPolicy::Rule::Member& other) :
    memberName(other.memberName), memberType(other.memberType),
    actionMask(other.actionMask) {
}

PermissionPolicy::Rule& PermissionPolicy::Rule::operator=(const PermissionPolicy::Rule& other) {
    if (&other != this) {
        objPath = other.objPath;
        interfaceName = other.interfaceName;
        membersSize = other.membersSize;
        delete [] members;
        members = new Member[membersSize];
        for (size_t i = 0; i < membersSize; i++) {
            members[i] = other.members[i];
        }
    }
    return *this;
}

PermissionPolicy::Rule::Rule(const PermissionPolicy::Rule& other) :
    objPath(other.objPath), interfaceName(other.interfaceName),
    membersSize(other.membersSize) {
    members = new Member[membersSize];
    for (size_t i = 0; i < membersSize; i++) {
        members[i] = other.members[i];
    }
}

PermissionPolicy::Acl& PermissionPolicy::Acl::operator=(const PermissionPolicy::Acl& other) {
    if (&other != this) {
        peersSize = other.peersSize;
        rulesSize = other.rulesSize;
        delete [] peers;
        peers = new Peer[peersSize];
        for (size_t i = 0; i < peersSize; i++) {
            peers[i] = other.peers[i];
        }
        rules = new Rule[rulesSize];
        for (size_t i = 0; i < rulesSize; i++) {
            rules[i] = other.rules[i];
        }
    }
    return *this;
}

PermissionPolicy::Acl::Acl(const PermissionPolicy::Acl& other) :
    peersSize(other.peersSize), rulesSize(other.rulesSize) {
    peers = new Peer[peersSize];
    for (size_t i = 0; i < peersSize; i++) {
        peers[i] = other.peers[i];
    }
    rules = new Rule[rulesSize];
    for (size_t i = 0; i < rulesSize; i++) {
        rules[i] = other.rules[i];
    }
}

PermissionPolicy::Peer& PermissionPolicy::Peer::operator=(const PermissionPolicy::Peer& other) {
    if (&other != this) {
        type = other.type;
        securityGroupId = other.securityGroupId;
        delete keyInfo;
        keyInfo = new qcc::KeyInfoECC(*other.keyInfo);
    }
    return *this;
}

PermissionPolicy::Peer::Peer(const PermissionPolicy::Peer& other) :
    type(other.type),
    securityGroupId(other.securityGroupId) {
    keyInfo = new qcc::KeyInfoECC(*other.keyInfo);
}


PermissionPolicy& PermissionPolicy::operator=(const PermissionPolicy& other) {
    if (&other != this) {
        specificationVersion = other.specificationVersion;
        version = other.version;
        aclsSize = other.aclsSize;
        delete [] acls;
        acls = new Acl[aclsSize];
        for (size_t i = 0; i < aclsSize; i++) {
            acls[i] = other.acls[i];
        }
    }
    return *this;
}

PermissionPolicy::PermissionPolicy(const PermissionPolicy& other) :
    specificationVersion(other.specificationVersion), version(other.version),
    aclsSize(other.aclsSize) {
    acls = new Acl[aclsSize];
    for (size_t i = 0; i < aclsSize; i++) {
        acls[i] = other.acls[i];
    }
}

} /* namespace ajn */

