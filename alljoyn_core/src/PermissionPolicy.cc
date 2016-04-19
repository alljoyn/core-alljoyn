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
#include <qcc/Util.h>
#include <alljoyn/PermissionPolicy.h>
#include <alljoyn/BusAttachment.h>
#include "KeyInfoHelper.h"
#include "PeerState.h"
#include <memory>

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

void PermissionPolicy::Rule::Member::Set(const qcc::String& memberName, PermissionPolicy::Rule::Member::MemberType memberType, uint8_t actionMask) {
    SetMemberName(memberName);
    SetMemberType(memberType);
    SetActionMask(actionMask);
}

void PermissionPolicy::Rule::Member::SetMemberName(const qcc::String& memberName)
{
    this->memberName = memberName;
}

const qcc::String PermissionPolicy::Rule::Member::GetMemberName() const
{
    return memberName;
}

void PermissionPolicy::Rule::Member::SetMemberType(PermissionPolicy::Rule::Member::MemberType memberType)
{
    this->memberType = memberType;
}

const PermissionPolicy::Rule::Member::MemberType PermissionPolicy::Rule::Member::GetMemberType() const
{
    return memberType;
}

void PermissionPolicy::Rule::Member::SetActionMask(uint8_t actionMask)
{
    this->actionMask = actionMask;
}

const uint8_t PermissionPolicy::Rule::Member::GetActionMask() const
{
    return actionMask;
}

qcc::String PermissionPolicy::Rule::Member::ToString(size_t indent) const
{
    qcc::String str;
    qcc::String in = qcc::String(indent, ' ');
    str += in + "<member>\n";
    if (memberName.length() > 0) {
        str += in + "  <name>" + memberName + "</name>\n";
    }
    if (memberType == METHOD_CALL) {
        str += in + "  <type>method call</type>\n";
    } else if (memberType == SIGNAL) {
        str += in + "  <type>signal</type>\n";
    } else if (memberType == PROPERTY) {
        str += in + "  <type>property</type>\n";
    }
    if ((actionMask & ACTION_PROVIDE) == ACTION_PROVIDE) {
        str += in + "  <action>Provide</action>\n";
    }
    if ((actionMask & ACTION_OBSERVE) == ACTION_OBSERVE) {
        str += in + "  <action>Observe</action>\n";
    }
    if ((actionMask & ACTION_MODIFY) == ACTION_MODIFY) {
        str += in + "  <action>Modify</action>\n";
    }
    str += in + "</member>\n";
    return str;
}

bool PermissionPolicy::Rule::Member::operator==(const PermissionPolicy::Rule::Member& other) const
{
    if (memberName != other.memberName) {
        return false;
    }

    if (memberType != other.memberType) {
        return false;
    }

    if (actionMask != other.actionMask) {
        return false;
    }

    return true;
}

bool PermissionPolicy::Rule::Member::operator!=(const PermissionPolicy::Rule::Member& other) const
{
    return !(*this == other);
}


void PermissionPolicy::Rule::SetObjPath(const qcc::String& objPath)
{
    this->objPath = objPath;
}

const qcc::String PermissionPolicy::Rule::GetObjPath() const
{
    return objPath;
}

void PermissionPolicy::Rule::SetInterfaceName(const qcc::String& interfaceName)
{
    this->interfaceName = interfaceName;
}

const qcc::String PermissionPolicy::Rule::GetInterfaceName() const
{
    return interfaceName;
}

void PermissionPolicy::Rule::SetMembers(size_t count, PermissionPolicy::Rule::Member* members)
{
    delete [] this->members;
    this->members = NULL;
    membersSize = 0;
    if (count == 0) {
        return;
    }
    this->members = new Member[count];
    if (this->members == NULL) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        this->members[i] = members[i];
    }
    membersSize = count;
}

const PermissionPolicy::Rule::Member* PermissionPolicy::Rule::GetMembers() const
{
    return members;
}

const size_t PermissionPolicy::Rule::GetMembersSize() const
{
    return membersSize;
}

qcc::String PermissionPolicy::Rule::ToString(size_t indent) const
{
    qcc::String str;
    qcc::String in = qcc::String(indent, ' ');
    str += in + "<rule>\n";
    if (objPath.length() > 0) {
        str += in + "  <objPath>" + objPath + "</objPath>\n";
    }
    if (interfaceName.length() > 0) {
        str += in + "  <interfaceName>" + interfaceName + "</interfaceName>\n";
    }
    for (size_t cnt = 0; cnt < GetMembersSize(); cnt++) {
        str += members[cnt].ToString(indent + 2);
    }
    str += in + "</rule>\n";
    return str;
}

bool PermissionPolicy::Rule::operator==(const PermissionPolicy::Rule& other) const
{
    if (objPath != other.objPath) {
        return false;
    }

    if (interfaceName != other.interfaceName) {
        return false;
    }

    if (membersSize != other.membersSize) {
        return false;
    }

    for (size_t i = 0; i < membersSize; i++) {
        if (!(members[i] == other.members[i])) {
            return false;
        }
    }

    return true;
}

bool PermissionPolicy::Rule::operator!=(const PermissionPolicy::Rule& other) const
{
    return !(*this == other);
}

PermissionPolicy::Rule& PermissionPolicy::Rule::operator=(const PermissionPolicy::Rule& other) {
    if (&other != this) {
        objPath = other.objPath;
        interfaceName = other.interfaceName;
        delete [] members;
        members = NULL;
        if (other.membersSize > 0) {
            members = new Member[other.membersSize];
            if (members == NULL) {
                return *this;
            }
            for (size_t i = 0; i < other.membersSize; i++) {
                members[i] = other.members[i];
            }
        }
        membersSize = other.membersSize;
    }
    return *this;
}

PermissionPolicy::Rule::Rule(const PermissionPolicy::Rule& other) :
    objPath(other.objPath), interfaceName(other.interfaceName),
    membersSize(other.membersSize) {
    members = new Member[membersSize];
    if (members == NULL) {
        return;
    }
    for (size_t i = 0; i < membersSize; i++) {
        members[i] = other.members[i];
    }
}

void PermissionPolicy::Peer::SetType(PermissionPolicy::Peer::PeerType peerType)
{
    type = peerType;
}

const PermissionPolicy::Peer::PeerType PermissionPolicy::Peer::GetType() const
{
    return type;
}

void PermissionPolicy::Peer::SetSecurityGroupId(const qcc::GUID128& guid)
{
    securityGroupId = guid;
}

const qcc::GUID128& PermissionPolicy::Peer::GetSecurityGroupId() const
{
    return securityGroupId;
}

void PermissionPolicy::Peer::SetKeyInfo(const qcc::KeyInfoNISTP256* keyInfo)
{
    delete this->keyInfo;
    this->keyInfo = NULL;
    if (keyInfo != NULL) {
        this->keyInfo = new qcc::KeyInfoNISTP256(*keyInfo);
    }
}

const qcc::KeyInfoNISTP256* PermissionPolicy::Peer::GetKeyInfo() const
{
    return keyInfo;
}

qcc::String PermissionPolicy::Peer::ToString(size_t indent) const
{
    qcc::String str;
    qcc::String in = qcc::String(indent, ' ');
    str += in + "<Peer>\n";
    if (type == PEER_ALL) {
        str += in + "  <type>ALL</type>\n";
    } else if (type == PEER_ANY_TRUSTED) {
        str += in + "  <type>ANY_TRUSTED</type>\n";
    } else if (type == PEER_FROM_CERTIFICATE_AUTHORITY) {
        str += in + "  <type>FROM_CERTIFICATE_AUTHORITY</type>\n";
        if (keyInfo) {
            str += keyInfo->ToString(indent + 2);
        }
    } else if (type == PEER_WITH_PUBLIC_KEY) {
        str += in + "  <type>WITH_PUBLIC_KEY</type>\n";
        if (keyInfo) {
            str += keyInfo->ToString(indent + 2);
        }
    } else if (type == PEER_WITH_MEMBERSHIP) {
        str += in + "  <type>WITH_MEMBERSHIP</type>\n";
        str += in + "  <groupId>" + securityGroupId.ToString() + "</groupId>\n";
        if (keyInfo) {
            str += keyInfo->ToString(indent + 2);
        }
    }
    str += in + "<Peer>\n";
    return str;
}

bool PermissionPolicy::Peer::operator==(const Peer& other) const
{
    if (type != other.type) {
        return false;
    }

    if (type == PEER_WITH_MEMBERSHIP) {
        if (securityGroupId != other.GetSecurityGroupId()) {
            return false;
        }
    }
    if (keyInfo == NULL || other.keyInfo == NULL) {
        return keyInfo == other.keyInfo;
    }

    // as defined in HLD, only PublicKey should be compared for Peers
    if (!(*(keyInfo->GetPublicKey()) == *(other.keyInfo->GetPublicKey()))) {
        return false;
    }

    return true;
}


bool PermissionPolicy::Peer::operator!=(const Peer& other) const
{
    return !(*this == other);
}

PermissionPolicy::Peer& PermissionPolicy::Peer::operator=(const PermissionPolicy::Peer& other) {
    if (&other != this) {
        type = other.type;
        securityGroupId = other.securityGroupId;
        delete keyInfo;
        keyInfo = NULL;
        if (other.keyInfo != NULL) {
            keyInfo = new qcc::KeyInfoNISTP256(*other.keyInfo);
        }
    }
    return *this;
}

PermissionPolicy::Peer::Peer(const PermissionPolicy::Peer& other) :
    type(other.type),
    securityGroupId(other.securityGroupId),
    keyInfo(nullptr) {
    if (nullptr != other.keyInfo) {
        keyInfo = new qcc::KeyInfoNISTP256(*other.keyInfo);
    }
}

void PermissionPolicy::Acl::SetPeers(size_t count, const PermissionPolicy::Peer* peers)
{
    delete [] this->peers;
    this->peers = NULL;
    peersSize = 0;
    if (count == 0) {
        return;
    }
    this->peers = new Peer[count];
    if (this->peers == NULL) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        this->peers[i] = peers[i];
    }
    peersSize = count;
}

const size_t PermissionPolicy::Acl::GetPeersSize() const
{
    return peersSize;
}

const PermissionPolicy::Peer* PermissionPolicy::Acl::GetPeers() const
{
    return peers;
}

void PermissionPolicy::Acl::SetRules(size_t count, const PermissionPolicy::Rule* rules)
{
    delete [] this->rules;
    this->rules = NULL;
    rulesSize = 0;
    if (count == 0) {
        return;
    }
    this->rules = new Rule[count];
    if (this->rules == NULL) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        this->rules[i] = rules[i];
    }
    rulesSize = count;
}

const size_t PermissionPolicy::Acl::GetRulesSize() const
{
    return rulesSize;
}

const PermissionPolicy::Rule* PermissionPolicy::Acl::GetRules() const
{
    return rules;
}

qcc::String PermissionPolicy::Acl::ToString(size_t indent) const
{
    qcc::String str;
    qcc::String in = qcc::String(indent, ' ');
    str += in + "<acl>\n";
    if ((peersSize > 0) && peers) {
        for (size_t cnt = 0; cnt < peersSize; cnt++) {
            str += peers[cnt].ToString(indent + 2);
        }
    }
    if ((rulesSize > 0) && rules) {
        for (size_t cnt = 0; cnt < rulesSize; cnt++) {
            str += rules[cnt].ToString(indent + 2);
        }
    }
    str += in + "</acl>\n";
    return str;
}

bool PermissionPolicy::Acl::operator==(const PermissionPolicy::Acl& other) const
{
    if (peersSize != other.peersSize) {
        return false;
    }

    for (size_t i = 0; i < peersSize; i++) {
        if (!(peers[i] == other.peers[i])) {
            return false;
        }
    }

    if (rulesSize != other.rulesSize) {
        return false;
    }

    for (size_t i = 0; i < rulesSize; i++) {
        if (!(rules[i] == other.rules[i])) {
            return false;
        }
    }

    return true;
}

bool PermissionPolicy::Acl::operator!=(const PermissionPolicy::Acl& other) const
{
    return !(*this == other);
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
        delete [] rules;
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

qcc::String PermissionPolicy::ToString(size_t indent) const
{
    qcc::String str;
    qcc::String in = qcc::String(indent, ' ');
    str += in + "<permissionPolicy>\n";
    str += in + "  <specificationVersion>" +  U32ToString(specificationVersion) + "</specificationVersion>\n";
    str += in + "  <version>" + U32ToString(version) + "</version>\n";

    if ((aclsSize > 0) && acls) {
        for (size_t cnt = 0; cnt < aclsSize; cnt++) {
            str += acls[cnt].ToString(indent + 2);
        }
    }
    str += in + "</permissionPolicy>\n";
    return str;
}

bool PermissionPolicy::operator==(const PermissionPolicy& other) const
{
    if (specificationVersion != other.specificationVersion) {
        return false;
    }

    if (version != other.version) {
        return false;
    }

    if (aclsSize != other.aclsSize) {
        return false;
    }

    for (size_t i = 0; i < aclsSize; i++) {
        if (!(acls[i] == other.acls[i])) {
            return false;
        }
    }

    return true;
}

bool PermissionPolicy::operator!=(const PermissionPolicy& other) const
{
    return !(*this == other);
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
            const KeyInfoNISTP256* keyInfo = peers[cnt].GetKeyInfo();
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
            KeyInfoHelper::KeyInfoNISTP256PubKeyToMsgArg(*keyInfoNISTP256, keyInfoArg[0], true);  /* send the key id in addition to public key */
        }
        const uint8_t* securityGroupId = NULL;
        size_t securityGroupLen = 0;
        if (peers[cnt].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) {
            securityGroupId = peers[cnt].GetSecurityGroupId().GetBytes();
            securityGroupLen = GUID128::SIZE;
        }
        status = (*retArgs)[cnt].Set("(ya(yyayayay)ay)",
                                     peers[cnt].GetType(), keyInfoCount, keyInfoArg,
                                     securityGroupLen, securityGroupId);
        if (ER_OK == status) {
            (*retArgs)[cnt].Stabilize();
        }
        delete [] keyInfoArg;
        if (ER_OK != status) {
            break;
        }
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
        status = arg[cnt].Get("(ya(yyayayay)ay)", &peerType, &pubKeysCnt, &pubKeys, &sgIdLen, &sgId);
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
        KeyInfoNISTP256 keyInfo;
        status = KeyInfoHelper::MsgArgToKeyInfoNISTP256PubKey(pubKeys[0], keyInfo, true);
        if (ER_OK != status) {
            break;
        }
        (*peers)[cnt].SetKeyInfo(&keyInfo);

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

static QStatus GenerateMemberArgs(MsgArg* retArgs, const PermissionPolicy::Rule::Member* members, size_t count)
{
    if (count == 0) {
        return ER_OK;
    }
    for (size_t cnt = 0; cnt < count; cnt++) {
        String memberName = members[cnt].GetMemberName();
        QStatus status = retArgs[cnt].Set("(syy)",
                                          memberName.c_str(), members[cnt].GetMemberType(),
                                          members[cnt].GetActionMask());
        if (ER_OK != status) {
            return status;
        }
        retArgs[cnt].Stabilize();
    }
    return ER_OK;
}

static QStatus GenerateRuleArgs(MsgArg** retArgs, const PermissionPolicy::Rule* rules, size_t count)
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
            ruleMembersArgs = new MsgArg[rules[cnt].GetMembersSize()];
            status = GenerateMemberArgs(ruleMembersArgs, rules[cnt].GetMembers(), rules[cnt].GetMembersSize());
            if (ER_OK != status) {
                delete [] ruleMembersArgs;
                goto exit;
            }
        }
        String objPath = rules[cnt].GetObjPath();
        String interfaceName = rules[cnt].GetInterfaceName();
        status = (*retArgs)[cnt].Set("(ssa(syy))",
                                     objPath.c_str(),
                                     interfaceName.c_str(),
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
            delete [] memberRules;
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
    QStatus status = msgArg.Get(_Manifest::s_TemplateMsgArgSignature, &argCount, &args);
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
        status = arg[cnt].Get("(a(ya(yyayayay)ay)a(ssa(syy)))", &peersArgsCount, &peersArgs, &rulesArgsCount, &rulesArgs);
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
            delete [] peers;
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
            delete [] rules;
        }
    }
    if (ER_OK != status) {
        delete [] *acls;
        *acls = NULL;
    }
    return status;
}

QStatus PermissionPolicy::Export(MsgArg& msgArg) const
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
            status = aclsArgs[cnt].Set("(a(ya(yyayayay)ay)a(ssa(syy)))",
                                       acls[cnt].GetPeersSize(), peersArgs, acls[cnt].GetRulesSize(), rulesArgs);
            if (ER_OK != status) {
                delete [] peersArgs;
                delete [] rulesArgs;
                break;
            }
        }
    }

    if (ER_OK == status) {
        status = msgArg.Set("(qua(a(ya(yyayayay)ay)a(ssa(syy))))",
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
    QStatus status = msgArg.Get("(qua(a(ya(yyayayay)ay)a(ssa(syy))))",
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
        delete [] aclArray;
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
     * Use an error message as it is the simplest message without many validation rules.
     * The ALLJOYN_FLAG_SESSIONLESS is set in order to skip the serial number
     * check since the data can be stored for a long time.
     */
    msg->ErrorMsg("/", 0);
    MsgArg variant("v", &args);
    return msg->MarshalMessage("v", "", "", MESSAGE_ERROR, &variant, 1, ALLJOYN_FLAG_SESSIONLESS, 0);
}

QStatus DefaultPolicyMarshaller::MarshalPrep(const PermissionPolicy::Rule* rules, size_t count)
{
    MsgArg msgArg;
    QStatus status = PermissionPolicy::GenerateRules(rules, count, msgArg);
    if (ER_OK != status) {
        return status;
    }
    /**
     * Use an error message as it is the simplest message without many validation rules.
     * The ALLJOYN_FLAG_SESSIONLESS is set in order to skip the serial number
     * check since the data can be stored for a long time*/
    msg->ErrorMsg("/", 0);
    return msg->MarshalMessage(_Manifest::s_TemplateMsgArgSignature, "", "", MESSAGE_ERROR, &msgArg, 1, ALLJOYN_FLAG_SESSIONLESS, 0);
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

QStatus DefaultPolicyMarshaller::Digest(const PermissionPolicy::Rule* rules, size_t count, uint8_t* digest, size_t len)
{
    if (len != Crypto_SHA256::DIGEST_SIZE) {
        return ER_INVALID_DATA;
    }
    Crypto_SHA256 hashUtil;
    QStatus status = MarshalPrep(rules, count);
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
    QStatus status = GenerateRuleArgs(&rulesArgs, rules, count);
    if (ER_OK != status) {
        return status;
    }
    status = msgArg.Set(_Manifest::s_TemplateMsgArgSignature, count, rulesArgs);
    if (ER_OK != status) {
        return status;
    }
    msgArg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    return status;
}

QStatus PermissionPolicy::ParseRules(const MsgArg& msgArg, Rule** rules, size_t* count)
{
    return BuildRulesFromArg(msgArg, rules, count);
}

PermissionPolicy& PermissionPolicy::operator=(const PermissionPolicy& other) {
    if (&other != this) {
        specificationVersion = other.specificationVersion;
        version = other.version;
        aclsSize = other.aclsSize;
        delete [] acls;
        acls = NULL;
        if (aclsSize > 0) {
            acls = new Acl[aclsSize];
            for (size_t i = 0; i < aclsSize; i++) {
                acls[i] = other.acls[i];
            }
        }
    }
    return *this;
}

PermissionPolicy::PermissionPolicy(const PermissionPolicy& other) :
    specificationVersion(other.specificationVersion), version(other.version),
    aclsSize(other.aclsSize) {
    acls = NULL;
    if (aclsSize > 0) {
        acls = new Acl[aclsSize];
        for (size_t i = 0; i < aclsSize; i++) {
            acls[i] = other.acls[i];
        }
    }
}

void PermissionPolicy::SetAcls(size_t count, const PermissionPolicy::Acl* acls) {
    delete [] this->acls;
    this->acls = NULL;
    aclsSize = 0;
    if (count == 0) {
        return;
    }
    this->acls = new Acl[count];
    if (this->acls == NULL) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        this->acls[i] = acls[i];
    }
    aclsSize = count;
}

/* _Manifest */
#undef QCC_MODULE
#define QCC_MODULE "PERMISSION_MANIFEST"

AJ_PCSTR _Manifest::s_MsgArgArraySignature = "a(ua(ssa(syy))saysay)";
AJ_PCSTR _Manifest::s_MsgArgSignature = "(ua(ssa(syy))saysay)";
AJ_PCSTR _Manifest::s_MsgArgDigestSignature = "(ua(ssa(syy))says)";
AJ_PCSTR _Manifest::s_TemplateMsgArgSignature = "a(ssa(syy))";
const uint32_t _Manifest::DefaultVersion = 1;

/* Static objects used just to serialize/deserialize messages */
static BusAttachment* s_serializeBus = nullptr;
static qcc::String* s_serializeSender = nullptr;
static PeerStateTable* s_serializePeerStateTable = nullptr;

void PermissionPolicyInit()
{
    /*
     * These objects might not be used by the current process, but they are small enough
     * and therefore acceptable until a better way to serialize data will be designed
     * (see ASACORE-2866).
     */
    s_serializeSender = new qcc::String("");
    s_serializePeerStateTable = new PeerStateTable;
}

void PermissionPolicyShutdown()
{
    delete s_serializeSender;
    s_serializeSender = nullptr;

    delete s_serializePeerStateTable;
    s_serializePeerStateTable = nullptr;

    delete s_serializeBus;
    s_serializeBus = nullptr;
}

static BusAttachment& GetStaticBusAttachment()
{
    /* Allocate s_serializeBus when it is needed, because it is relatively large & complex */
    if (s_serializeBus == nullptr) {
        BusAttachment* newBus = new BusAttachment("PermissionPolicyStaticBusAttachment");
        if (!CompareAndExchangePointer((void* volatile*)&s_serializeBus, nullptr, newBus)) {
            delete newBus;
        }
    }

    return *s_serializeBus;
}

_Manifest::_Manifest() : m_version(DefaultVersion), m_rules(), m_thumbprintAlgorithmOid(), m_thumbprint(), m_signatureAlgorithmOid(), m_signature()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QCC_ASSERT(0 == strcmp(_Manifest::s_MsgArgArraySignature + 1, _Manifest::s_MsgArgSignature));
}

_Manifest::_Manifest(const _Manifest& other) :
    m_version(other.m_version),
    m_rules(other.m_rules),
    m_thumbprintAlgorithmOid(other.m_thumbprintAlgorithmOid),
    m_thumbprint(other.m_thumbprint),
    m_signatureAlgorithmOid(other.m_signatureAlgorithmOid),
    m_signature(other.m_signature)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_Manifest& _Manifest::operator=(const _Manifest& other)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (this != &other) {
        m_version = other.m_version;
        m_rules = other.m_rules;
        m_thumbprintAlgorithmOid = other.m_thumbprintAlgorithmOid;
        m_thumbprint = other.m_thumbprint;
        m_signatureAlgorithmOid = other.m_signatureAlgorithmOid;
        m_signature = other.m_signature;
    }

    return *this;
}

_Manifest::~_Manifest()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

QStatus _Manifest::SetFromMsgArg(const MsgArg& manifestArg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /* MsgArg::Get will give us pointers into the MsgArg object. We have to make copies
     * since we can't make assumptions about the lifetime of the MsgArg.
     */
    uint32_t inVersion;
    MsgArg* inRulesArgs;
    size_t inRuleCount;
    AJ_PSTR inThumbprintAlgorithmOid;
    uint8_t* inThumbprint;
    size_t inThumbprintSize;
    AJ_PSTR inSignatureAlgorithmOid;
    uint8_t* inSignature;
    size_t inSignatureSize;

    QStatus status = manifestArg.Get(_Manifest::s_MsgArgSignature,
                                     &inVersion,
                                     &inRuleCount, &inRulesArgs,
                                     &inThumbprintAlgorithmOid, &inThumbprintSize, &inThumbprint,
                                     &inSignatureAlgorithmOid, &inSignatureSize, &inSignature);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not get arguments from MsgArg"));
        return status;
    }

    if (!_Manifest::IsVersionSupported(inVersion)) {
        QCC_LogError(ER_INVALID_DATA, ("Manifest is unknown version; received version %u", inVersion));
        return ER_INVALID_DATA;
    }

    PermissionPolicy::Rule* parsedRules = nullptr;
    status = BuildRulesFromArgArray(inRulesArgs, inRuleCount, &parsedRules);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not parse rules from rules MsgArg array"));
        return status;
    }

    m_version = inVersion;
    /* BuildRulesFromArgArray allocates memory which we copy into the vector and then delete. */
    m_rules.assign(parsedRules, parsedRules + inRuleCount);
    delete[] parsedRules;
    m_thumbprintAlgorithmOid.assign(inThumbprintAlgorithmOid);
    m_thumbprint.assign(inThumbprint, inThumbprint + inThumbprintSize);
    m_signatureAlgorithmOid.assign(inSignatureAlgorithmOid);
    m_signature.assign(inSignature, inSignature + inSignatureSize);

    return ER_OK;
}

QStatus _Manifest::GetArrayMsgArg(const std::vector<Manifest>& manifests, MsgArg& outputArg)
{
    return GetArrayMsgArg(manifests.data(), manifests.size(), outputArg);
}

QStatus _Manifest::GetArrayMsgArg(const Manifest* manifests, size_t manifestCount, MsgArg& outputArg)
{
    QStatus status = ER_OK;

    if (0 == manifestCount) {
        return outputArg.Set(_Manifest::s_MsgArgArraySignature, 0, nullptr);
    }

    std::vector<MsgArg> msgArgs(manifestCount);

    for (size_t i = 0; i < manifestCount; i++) {
        status = manifests[i]->GetMsgArg(MANIFEST_FULL, msgArgs[i]);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not get MsgArg for manifest"));
            return status;
        }
    }

    status = outputArg.Set(_Manifest::s_MsgArgArraySignature, msgArgs.size(), msgArgs.data());
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not set array MsgArg"));
        return status;
    }

    outputArg.Stabilize();

    return status;
}

QStatus _Manifest::GetMsgArg(ManifestPurpose manifestPurpose, MsgArg& outputArg) const
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    MsgArg* rulesArgsRaw = NULL;
    QStatus status = GenerateRuleArgs(&rulesArgsRaw, m_rules.data(), m_rules.size());
    if (ER_OK != status) {
        return status;
    }
    /* GenerateRuleArgs allocates memory; take ownership of it. */
    std::unique_ptr<MsgArg[]> rulesArgs(rulesArgsRaw);
    rulesArgsRaw = nullptr;

    switch (manifestPurpose) {
    case MANIFEST_FULL:
        status = outputArg.Set(_Manifest::s_MsgArgSignature,
                               m_version,
                               m_rules.size(), rulesArgs.get(),
                               m_thumbprintAlgorithmOid.c_str(), m_thumbprint.size(), m_thumbprint.data(),
                               m_signatureAlgorithmOid.c_str(), m_signature.size(), m_signature.data());
        break;

    case MANIFEST_FOR_DIGEST:
        status = outputArg.Set(_Manifest::s_MsgArgDigestSignature,
                               m_version,
                               m_rules.size(), rulesArgs.get(),
                               m_thumbprintAlgorithmOid.c_str(), m_thumbprint.size(), m_thumbprint.data(),
                               m_signatureAlgorithmOid.c_str());
        break;

    default:
        QCC_ASSERT(false && "Unknown value of manifestPurpose");
        return ER_BAD_ARG_1;
    }

    if (ER_OK != status) {
        return status;
    }

    outputArg.Stabilize();

    return ER_OK;
}

QStatus _Manifest::ComputeThumbprintAndSign(const CertificateX509& subjectCertificate, const ECCPrivateKey* issuerPrivateKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    m_thumbprint.resize(Crypto_SHA256::DIGEST_SIZE);
    QStatus status = subjectCertificate.GetSHA256Thumbprint(m_thumbprint.data());
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not compute certificate thumbprint"));
        return status;
    }

    return Sign(m_thumbprint, issuerPrivateKey);
}

QStatus _Manifest::Sign(const std::vector<uint8_t>& subjectThumbprint, const ECCPrivateKey* issuerPrivateKey)
{
    if (&subjectThumbprint != &m_thumbprint) {
        m_thumbprint = subjectThumbprint;
    }

    /* Only SHA-256 is supported as the thumbprint algorithm. */
    m_thumbprintAlgorithmOid.assign(qcc::OID_DIG_SHA256.c_str());
    /* Only ECDSA with SHA-256 is supported as the signature algorithm. */
    m_signatureAlgorithmOid.assign(qcc::OID_SIG_ECDSA_SHA256.c_str());

    std::vector<uint8_t> digest(Crypto_SHA256::DIGEST_SIZE);
    QStatus status = GetDigest(digest);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not compute manifest digest"));
        return status;
    }

    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(issuerPrivateKey);
    ECCSignature signature;
    status = ecc.DSASignDigest(digest.data(), digest.size(), &signature);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not sign manifest digest"));
        return status;
    }

    status = SetECCSignature(signature);

    return status;
}

QStatus _Manifest::ComputeThumbprintAndVerify(const CertificateX509& subjectCertificate, const ECCPublicKey* issuerPublicKey) const
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    std::vector<uint8_t> thumbprint(Crypto_SHA256::DIGEST_SIZE);
    QStatus status = subjectCertificate.GetSHA256Thumbprint(thumbprint.data());
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not compute certificate hash"));
        return status;
    }

    return Verify(thumbprint, issuerPublicKey);
}

QStatus _Manifest::Verify(const std::vector<uint8_t>& subjectThumbprint, const ECCPublicKey* issuerPublicKey) const
{
    /* Only SHA-256 is supported as the thumbprint algorithm. */
    if (m_thumbprintAlgorithmOid != std::string(qcc::OID_DIG_SHA256.c_str())) {
        QCC_LogError(ER_NOT_IMPLEMENTED, ("Unsupported thumbprint algorithm \'%s\'", m_thumbprintAlgorithmOid.c_str()));
        return ER_NOT_IMPLEMENTED;
    }

    if (subjectThumbprint != m_thumbprint) {
        /* Manifest is not for this subject's certificate. */
        QCC_LogError(ER_UNKNOWN_CERTIFICATE, ("Thumbprint is not for this subject certificate"));
        return ER_UNKNOWN_CERTIFICATE;
    }

    /* Compute our view of the digest. */
    std::vector<uint8_t> digest;
    QStatus status = GetDigest(digest);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not compute manifest digest"));
        return status;
    }

    /* Make sure the signature agrees. */
    ECCSignature signature;
    status = GetECCSignature(signature);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not get ECC signature from manifest"));
        return status;
    }

    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(issuerPublicKey);
    status = ecc.DSAVerifyDigest(digest.data(), digest.size(), &signature);
    if (ER_OK != status) {
        QCC_LogError(status, ("Manifest signature failed to verify"));
        if (ER_CRYPTO_ERROR == status) {
            return ER_DIGEST_MISMATCH;
        } else {
            return status;
        }
    }

    return status;
}

QStatus _Manifest::GetDigest(std::vector<uint8_t>& digest) const
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (m_signatureAlgorithmOid != std::string(qcc::OID_SIG_ECDSA_SHA256.c_str())) {
        return ER_NOT_IMPLEMENTED;
    }

    Crypto_SHA256 hash;
    QStatus status = hash.Init();
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not initialize hash object"));
        return status;
    }

    /* Serialize manifest without signature field to byte array and hash. */
    std::vector<uint8_t> serializedForm;
    status = Serialize(MANIFEST_FOR_DIGEST, serializedForm);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not serialize manifest to hash"));
        return status;
    }

    status = hash.Update(serializedForm.data(), serializedForm.size());
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not hash serialized bytes"));
        return status;
    }

    digest.resize(Crypto_SHA256::DIGEST_SIZE);
    status = hash.GetDigest(digest.data(), false);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not get digest from hash object"));
        return status;
    }

    return status;
}

QStatus _Manifest::GetECCSignature(ECCSignature& signature) const
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (m_signatureAlgorithmOid != std::string(qcc::OID_SIG_ECDSA_SHA256.c_str())) {
        QCC_LogError(ER_NOT_IMPLEMENTED, ("Unsupported manifest signature algorithm: %s", m_signatureAlgorithmOid.c_str()));
        return ER_NOT_IMPLEMENTED;
    }

    if (m_signature.size() != (sizeof(signature.r) + sizeof(signature.s))) {
        QCC_LogError(ER_INVALID_DATA, ("Wrong size for signature; got %" PRIuSIZET ", expected %" PRIuSIZET,
                                       m_signature.size(), sizeof(signature.r) + sizeof(signature.s)));
        return ER_INVALID_DATA;
    }

    memcpy(signature.r, m_signature.data(), sizeof(signature.r));
    memcpy(signature.s, m_signature.data() + sizeof(signature.r), sizeof(signature.s));

    return ER_OK;
}

QStatus _Manifest::SetECCSignature(const ECCSignature& signature)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (m_signatureAlgorithmOid != std::string(qcc::OID_SIG_ECDSA_SHA256.c_str())) {
        QCC_LogError(ER_NOT_IMPLEMENTED, ("Unsupported manifest signature algorithm: %s", m_signatureAlgorithmOid.c_str()));
        return ER_NOT_IMPLEMENTED;
    }

    m_signature.resize(sizeof(signature.r) + sizeof(signature.s));
    memcpy(m_signature.data(), signature.r, sizeof(signature.r));
    memcpy(m_signature.data() + sizeof(signature.r), signature.s, sizeof(signature.s));

    return ER_OK;
}

QStatus _Manifest::SetRules(const PermissionPolicy::Rule* rules, size_t rulesCount)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    m_rules.resize(rulesCount);
    for (size_t i = 0; i < m_rules.size(); i++) {
        m_rules[i] = rules[i];
    }

    return ER_OK;
}

QStatus _Manifest::Serialize(std::vector<uint8_t>& serializedForm) const
{
    return this->Serialize(MANIFEST_FULL, serializedForm);
}

QStatus _Manifest::Serialize(ManifestPurpose manifestPurpose, std::vector<uint8_t>& serializedForm) const
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    MsgArg msgArg[1];
    AJ_PCSTR messageSignature;
    QStatus status;

    switch (manifestPurpose) {
    case MANIFEST_FULL:
        messageSignature = _Manifest::s_MsgArgSignature;
        break;

    case MANIFEST_FOR_DIGEST:
        messageSignature = _Manifest::s_MsgArgDigestSignature;
        break;

    default:
        QCC_ASSERT(false && "Unknown value of manifestPurpose");
        return ER_BAD_ARG_1;
    }

    status = GetMsgArg(manifestPurpose, msgArg[0]);
    if (ER_OK != status) {
        return status;
    }

    char savedEndian = _Message::outEndian;
    _Message::SetEndianess(ALLJOYN_LITTLE_ENDIAN);
    Message tmpMsg(GetStaticBusAttachment());
    tmpMsg->ErrorMsg(*s_serializeSender, "/", 0);
    status = tmpMsg->MarshalMessage(messageSignature, "", "", MESSAGE_ERROR, msgArg, 1, ALLJOYN_FLAG_SESSIONLESS, 0);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not marshal message"));
        _Message::SetEndianess(savedEndian);
        return status;
    }

    switch (manifestPurpose) {
    case MANIFEST_FULL:
        /*
         * Form for storing in the keystore: store the message headers as well.
         * Multiple Serialize() calls for the same manifest must return the same
         * byte array, so reset the serial number value in the message header.
         */
        tmpMsg->SetSerialNumber(0);
        serializedForm.assign(tmpMsg->GetBuffer(), tmpMsg->GetBuffer() + tmpMsg->GetBufferSize());
        break;

    case MANIFEST_FOR_DIGEST:
        /* Omit the message headers and the signature field. */
        serializedForm.assign(tmpMsg->GetBodyBuffer(), tmpMsg->GetBodyBuffer() + tmpMsg->GetBodyBufferSize());
        break;

    default:
        QCC_ASSERT(false && "Unknown value of manifestPurpose");
        status = ER_BAD_ARG_1;
        break;
    }

    _Message::SetEndianess(savedEndian);
    return status;
}

QStatus _Manifest::Deserialize(const std::vector<uint8_t>& serializedForm)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String endpointName("local");

    Message tmpMsg(GetStaticBusAttachment());
    std::vector<uint8_t> serializedFormCopy(serializedForm); /* LoadBytes doesn't take const. */
    QStatus status = tmpMsg->LoadBytes(serializedFormCopy.data(), serializedFormCopy.size());
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not populate Message object with serialized form"));
        return status;
    }

    status = tmpMsg->Unmarshal(s_serializePeerStateTable, endpointName, false, false, false);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not Unmarshal"));
        return status;
    }

    status = tmpMsg->UnmarshalArgs(s_serializePeerStateTable, "*");
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not UnmarshalArgs"));
        return status;
    }

    status = SetFromMsgArg(*tmpMsg->GetArg(0));
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not SetFromMsgArg"));
        return status;
    }

    return ER_OK;
}

QStatus _Manifest::SerializeArray(const std::vector<Manifest>& manifests, std::vector<uint8_t>& serializedForm)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    MsgArg msgArg[1];
    QStatus status = GetArrayMsgArg(manifests, msgArg[0]);
    if (ER_OK != status) {
        return status;
    }

    char savedEndian = _Message::outEndian;
    _Message::SetEndianess(ALLJOYN_LITTLE_ENDIAN);
    Message tmpMsg(GetStaticBusAttachment());
    tmpMsg->ErrorMsg(*s_serializeSender, "/", 0);
    status = tmpMsg->MarshalMessage(_Manifest::s_MsgArgArraySignature, "", "", MESSAGE_ERROR, msgArg, 1, ALLJOYN_FLAG_SESSIONLESS, 0);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not marshal message"));
    } else {
        serializedForm.assign(tmpMsg->GetBuffer(), tmpMsg->GetBuffer() + tmpMsg->GetBufferSize());
    }

    _Message::SetEndianess(savedEndian);
    return status;
}

QStatus _Manifest::DeserializeArray(const std::vector<uint8_t>& serializedForm, std::vector<Manifest>& manifests)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return DeserializeArray(serializedForm.data(), serializedForm.size(), manifests);
}

QStatus _Manifest::DeserializeArray(const uint8_t* serializedForm, size_t serializedSize, std::vector<Manifest>& manifests)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String endpointName("local");

    Message tmpMsg(GetStaticBusAttachment());
    std::vector<uint8_t> serializedFormCopy(serializedSize); /* LoadBytes doesn't take const. */
    serializedFormCopy.assign(serializedForm, serializedForm + serializedSize);

    QStatus status = tmpMsg->LoadBytes(serializedFormCopy.data(), serializedFormCopy.size());
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not populate Message object with serialized form"));
        return status;
    }

    status = tmpMsg->Unmarshal(s_serializePeerStateTable, endpointName, false, false, false);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not Unmarshal"));
        return status;
    }

    status = tmpMsg->UnmarshalArgs(s_serializePeerStateTable, "*");
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not UnmarshalArgs"));
        return status;
    }

    MsgArg* signedManifestArgs;
    uint32_t signedManifestCount;

    status = tmpMsg->GetArg(0)->Get(_Manifest::s_MsgArgArraySignature, &signedManifestCount, &signedManifestArgs);
    if (ER_OK != status) {
        return status;
    }
    if (signedManifestCount == 0) {
        return ER_OK; /* nothing to do */
    }

    for (uint32_t i = 0; i < signedManifestCount; i++) {
        Manifest manifest;
        status = manifest->SetFromMsgArg(signedManifestArgs[i]);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not SetFromMsgArg"));
            return status;
        }
        manifests.push_back(manifest);
    }

    return ER_OK;
}

bool _Manifest::IsVersionSupported(uint32_t version)
{
    /* Only one version exists at , and so it is the only supported version. */
    return (version == _Manifest::DefaultVersion);
}

uint32_t _Manifest::GetVersion() const
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return m_version;
}

const std::vector<PermissionPolicy::Rule>& _Manifest::GetRules() const
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return m_rules;
}

std::string _Manifest::GetThumbprintAlgorithmOid() const
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return m_thumbprintAlgorithmOid;
}

std::vector<uint8_t> _Manifest::GetThumbprint() const
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return m_thumbprint;
}

std::string _Manifest::GetSignatureAlgorithmOid() const
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return m_signatureAlgorithmOid;
}

std::vector<uint8_t> _Manifest::GetSignature() const
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return m_signature;
}

bool _Manifest::operator==(const _Manifest& other) const
{
    if ((m_version != other.m_version) ||
        (m_rules.size() != other.m_rules.size()) ||
        (m_thumbprintAlgorithmOid != other.m_thumbprintAlgorithmOid) ||
        (m_thumbprint != other.m_thumbprint) ||
        (m_signatureAlgorithmOid != other.m_signatureAlgorithmOid) ||
        (m_signature != other.m_signature)) {
        return false;
    }

    for (size_t i = 0; i < m_rules.size(); i++) {
        if (m_rules.data()[i] != other.m_rules.data()[i]) {
            return false;
        }
    }

    return true;
}

bool _Manifest::operator!=(const _Manifest& other) const
{
    return !(*this == other);
}

std::string _Manifest::ToString() const
{
    std::string output;
    char versionBuf[20];

    /*
     * Not all platforms support std::to_string, so we stringfy the version number
     * with snprintf.
     */
    snprintf(versionBuf, sizeof(versionBuf), "%u", m_version);

    output += "Manifest:\n";
    output += "=====================\n";
    output += "Version: ";
    output += versionBuf;
    output += "Thumbprint algorithm OID: " + m_thumbprintAlgorithmOid + "\n";
    output += "Thumbprint: ";
    output += BytesToHexString(m_thumbprint.data(), m_thumbprint.size()).c_str();
    output += "\n";
    output += "Signature algorithm OID: " + m_signatureAlgorithmOid + "\n";
    output += "Signature: ";
    output += BytesToHexString(m_signature.data(), m_signature.size()).c_str();
    output += "\n";
    output += "Rules:\n";

    for (const PermissionPolicy::Rule& rule : GetRules()) {
        output += rule.ToString().c_str();
        output += "\n";
    }

    output += "=====================\n";

    return output;
}

bool _Manifest::HasSignature() const
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    if (m_thumbprintAlgorithmOid.empty()) {
        QCC_DbgTrace(("Manifest has empty thumbprint algorithm OID"));
        return false;
    }

    if (m_thumbprint.empty()) {
        QCC_DbgTrace(("Manifest has empty thumbprint"));
        return false;
    }

    if (m_signatureAlgorithmOid.empty()) {
        QCC_DbgTrace(("Manifest has empty signature algorithm OID"));
        return false;
    }

    if (m_signature.empty()) {
        QCC_DbgTrace(("Manifest has empty signature"));
        return false;
    }

    return true;
}

} /* namespace ajn */

