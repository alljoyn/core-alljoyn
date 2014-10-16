/**
 * @file
 * This file defines the Permission DB classes that provide the interface to
 * parse the authorization data
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#include <alljoyn/PermissionPolicy.h>

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

qcc::String PermissionPolicy::Rule::Member::ToString()
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
    str += "  read-only: " + U32ToString(readOnly) + "\n";
    return str;
}

qcc::String PermissionPolicy::Rule::ToString()
{
    qcc::String str;
    str += "Rule:\n";
    if (objPath.length() > 0) {
        str += "  objPath: " + objPath + "\n";
    }
    if (interfaceName.length() > 0) {
        str += "  interfaceName: " + interfaceName + "\n";
        for (size_t cnt = 0; cnt < GetNumOfMembers(); cnt++) {
            str += members[cnt].ToString();
        }
    }
    return str;
}

qcc::String PermissionPolicy::Peer::ToString()
{
    qcc::String str;
    str += "Peer:\n";
    if (type == PEER_ANY) {
        str += "  type: ANY\n";
    } else if (type == PEER_PSK) {
        str += "  type: PSK\n";
    } else if (type == PEER_GUID) {
        str += "  type: GUID\n";
    } else if (type == PEER_DSA) {
        str += "  type: DSA\n";
    } else if (type == PEER_GUILD) {
        str += "  type: GUILD\n";
    }
    if (IDLen > 0) {
        str += "  ID: " + BytesToHexString(ID, IDLen) + "\n";
    }
    if (guildAuthorityLen > 0) {
        str += "  GuildAuthority: " + BytesToHexString(guildAuthority, guildAuthorityLen) + "\n";
    }
    if (pskLen > 0) {
        str += "  PSK: " + BytesToHexString(psk, pskLen) + "\n";
    }
    return str;
}

qcc::String PermissionPolicy::ACL::ToString()
{
    qcc::String str;
    str += "ACL:\n";
    if ((allowRuleSize > 0) && allowRules) {
        for (size_t cnt = 0; cnt < allowRuleSize; cnt++) {
            str += "  allow[" + U32ToString(cnt) + "]: " + allowRules[cnt].ToString();
        }
    }
    if ((allowAllExceptRuleSize > 0) && allowAllExceptRules) {
        for (size_t cnt = 0; cnt < allowAllExceptRuleSize; cnt++) {
            str += "  allowAllExceptRules[" + U32ToString(cnt) + "]: " + allowAllExceptRules[cnt].ToString();
        }
    }
    return str;
}

qcc::String PermissionPolicy::Term::ToString()
{
    qcc::String str;
    str += "Term:\n";
    if ((peerSize > 0) && peers) {
        for (size_t cnt = 0; cnt < peerSize; cnt++) {
            str += "  peer[" + U32ToString(cnt) + "]: " + peers[cnt].ToString();
        }
    }
    str += PermissionPolicy::ACL::ToString();
    return str;
}

qcc::String PermissionPolicy::ToString()
{
    qcc::String str;
    str += "PermissionPolicy:\n";
    str += "  version: " +  U32ToString(version) + "\n";
    str += "  serial number: " + U32ToString(serialNum) + "\n";

    if ((adminSize > 0) && admins) {
        for (size_t cnt = 0; cnt < adminSize; cnt++) {
            str += "  admin[" + U32ToString(cnt) + "]: " + admins[cnt].ToString();
        }
    }
    if ((providerSize > 0) && providers) {
        for (size_t cnt = 0; cnt < providerSize; cnt++) {
            str += "  provider[" + U32ToString(cnt) + "]: " + providers[cnt].ToString();
        }
    }
    if ((consumerSize > 0) && consumers) {
        for (size_t cnt = 0; cnt < consumerSize; cnt++) {
            str += "  consumer[" + U32ToString(cnt) + "]: " + consumers[cnt].ToString();
        }
    }
    return str;
}

void PermissionPolicy::ACL::SetAllowAllExceptRules(size_t count, Rule* exceptions)
{
    allowAllExceptRules = exceptions;
    allowAllExceptRuleSize = count;

}

static QStatus GeneratePeerArgs(MsgArg** retArgs, PermissionPolicy::Peer* peers, size_t count)
{
    MsgArg* variants = new MsgArg[count];
    for (size_t cnt = 0; cnt < count; cnt++) {
        int fieldCnt = 0;
        if (peers[cnt].GetID()) {
            fieldCnt++;
        }
        if (peers[cnt].GetGuildAuthority()) {
            fieldCnt++;
        }
        if (peers[cnt].GetPsk()) {
            fieldCnt++;
        }
        MsgArg* peerArgs = NULL;
        if (fieldCnt > 0) {
            peerArgs = new MsgArg[fieldCnt];
            int idx = 0;
            if (peers[cnt].GetID()) {
                peerArgs[idx].Set("(yv)", PermissionPolicy::Peer::TAG_ID, new MsgArg("ay", peers[cnt].GetIDLen(), peers[cnt].GetID()));
                idx++;
            }
            if (peers[cnt].GetGuildAuthority()) {
                peerArgs[idx].Set("(yv)", PermissionPolicy::Peer::TAG_GUILD_AUTHORITY, new MsgArg("ay", peers[cnt].GetGuildAuthorityLen(), peers[cnt].GetGuildAuthority()));
                idx++;
            }
            if (peers[cnt].GetPsk()) {
                peerArgs[idx].Set("(yv)", PermissionPolicy::Peer::TAG_PSK,
                                  new MsgArg("ay", peers[cnt].GetPskLen(), peers[cnt].GetPsk()));
                idx++;
            }
        }
        variants[cnt].Set("(ya(yv))", peers[cnt].GetType(), fieldCnt, peerArgs);
        variants[cnt].SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }
    *retArgs = variants;
    return ER_OK;
}

static QStatus BuildPeersFromArg(MsgArg& arg, PermissionPolicy::Peer** peers, size_t* count)
{
    MsgArg* peerListArgs = NULL;
    size_t peerCount;
    QStatus status = arg.Get("a(ya(yv))", &peerCount, &peerListArgs);
    if (ER_OK != status) {
        QCC_DbgPrintf(("BuildPeersFromArg #1 got status 0x%x\n", status));
        return status;
    }
    if (peerCount == 0) {
        *count = peerCount;
        return ER_OK;
    }
    PermissionPolicy::Peer* peerArray = new PermissionPolicy::Peer[peerCount];
    for (size_t cnt = 0; cnt < peerCount; cnt++) {
        uint8_t peerType;
        MsgArg* peerFieldArgs;
        size_t fieldCnt;
        status = peerListArgs[cnt].Get("(ya(yv))", &peerType, &fieldCnt, &peerFieldArgs);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildPeersFromArg #3 [%d] got status 0x%x\n", cnt, status));
            delete [] peerArray;
            return status;
        }
        PermissionPolicy::Peer* pp = &peerArray[cnt];
        if ((peerType >= PermissionPolicy::Peer::PEER_ANY) && (peerType <= PermissionPolicy::Peer::PEER_GUILD)) {
            pp->SetType((PermissionPolicy::Peer::PeerType) peerType);
        } else {
            delete [] peerArray;
            return ER_INVALID_DATA;
        }
        for (size_t fld = 0; fld < fieldCnt; fld++) {
            uint8_t fieldType;
            MsgArg* field;
            status = peerFieldArgs[fld].Get("(yv)", &fieldType, &field);
            if (ER_OK != status) {
                QCC_DbgPrintf(("BuildPeersFromArg #4 [%d][%d] got status 0x%x\n", cnt, fld, status));
                delete [] peerArray;
                return status;
            }
            uint8_t* data;
            size_t len;
            switch (fieldType) {
            case PermissionPolicy::Peer::TAG_ID:
                status = field->Get("ay", &len, &data);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildPeersFromArg #5 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete [] peerArray;
                    return status;
                }
                pp->SetID(data, len);
                break;

            case PermissionPolicy::Peer::TAG_GUILD_AUTHORITY:
                status = field->Get("ay", &len, &data);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildPeersFromArg #6 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete [] peerArray;
                    return status;
                }
                pp->SetGuildAuthority(data, len);
                break;

            case PermissionPolicy::Peer::TAG_PSK:
                status = field->Get("ay", &len, &data);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildPeersFromArg #7 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete [] peerArray;
                    return status;
                }
                pp->SetPsk(data, len);
                break;
            }
        }
    }

    *count = peerCount;
    *peers = peerArray;
    return ER_OK;
}

static QStatus GenerateMemberArgs(MsgArg** retArgs, const PermissionPolicy::Rule::Member* rules, size_t count)
{
    MsgArg* variants = new MsgArg[count];
    for (size_t cnt = 0; cnt < count; cnt++) {
        /* count the set fields */
        PermissionPolicy::Rule::Member* aRule = (PermissionPolicy::Rule::Member*) &rules[cnt];
        int fieldCnt = 0;
        if (!aRule->GetMemberName().empty()) {
            fieldCnt++;
        }
        if (aRule->GetMemberType() != PermissionPolicy::Rule::Member::NOT_SPECIFIED) {
            fieldCnt++;
        }
        if (!aRule->GetReadOnly()) {
            fieldCnt++;
        }
        if (!aRule->GetMutualAuth()) {
            fieldCnt++;
        }
        MsgArg* ruleArgs = NULL;
        if (fieldCnt > 0) {
            int idx = 0;
            ruleArgs = new MsgArg[fieldCnt];
            if (aRule->GetMemberName().size() > 0) {
                ruleArgs[idx].Set("(yv)", PermissionPolicy::Rule::Member::TAG_MEMBER_NAME,
                                  new MsgArg("s", aRule->GetMemberName().c_str()));
                idx++;
            }
            if (aRule->GetMemberType() != PermissionPolicy::Rule::Member::NOT_SPECIFIED) {
                ruleArgs[idx].Set("(yv)", PermissionPolicy::Rule::Member::TAG_MEMBER_TYPE,
                                  new MsgArg("y", aRule->GetMemberType()));
                idx++;
            }
            if (!aRule->GetReadOnly()) {
                ruleArgs[idx].Set("(yv)", PermissionPolicy::Rule::Member::TAG_READ_ONLY,
                                  new MsgArg("b", aRule->GetReadOnly()));
                idx++;
            }
            if (!aRule->GetMutualAuth()) {
                ruleArgs[idx].Set("(yv)", PermissionPolicy::Rule::Member::TAG_MUTUAL_AUTH,
                                  new MsgArg("b", aRule->GetMutualAuth()));
                idx++;
            }
        }
        variants[cnt].Set("a(yv)", fieldCnt, ruleArgs);
        variants[cnt].SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }
    *retArgs = variants;
    return ER_OK;
}

static QStatus GenerateRuleArgs(MsgArg** retArgs, PermissionPolicy::Rule* rules, size_t count)
{
    MsgArg* variants = new MsgArg[count];
    for (size_t cnt = 0; cnt < count; cnt++) {
        /* count the set fields */
        PermissionPolicy::Rule* aRule = &rules[cnt];
        int fieldCnt = 0;
        if (aRule->GetObjPath().size() > 0) {
            fieldCnt++;
        }
        if (aRule->GetInterfaceName().size() > 0) {
            fieldCnt++;
        }
        if (aRule->GetNumOfMembers() > 0) {
            fieldCnt++;
        }
        MsgArg* ruleArgs = NULL;
        if (fieldCnt > 0) {
            int idx = 0;
            ruleArgs = new MsgArg[fieldCnt];
            if (aRule->GetObjPath().size() > 0) {
                ruleArgs[idx].Set("(yv)", PermissionPolicy::Rule::TAG_OBJPATH,
                                  new MsgArg("s", aRule->GetObjPath().c_str()));
                idx++;
            }
            if (aRule->GetInterfaceName().size() > 0) {
                ruleArgs[idx].Set("(yv)", PermissionPolicy::Rule::TAG_INTERFACE_NAME,
                                  new MsgArg("s", aRule->GetInterfaceName().c_str()));
                idx++;
            }
            if (aRule->GetNumOfMembers() > 0) {
                MsgArg* ruleMembersArgs = NULL;
                GenerateMemberArgs(&ruleMembersArgs, aRule->GetMembers(), aRule->GetNumOfMembers());
                ruleArgs[idx].Set("(yv)", PermissionPolicy::Rule::TAG_INTERFACE_MEMBERS,
                                  new MsgArg("aa(yv)", aRule->GetNumOfMembers(), ruleMembersArgs));
                idx++;
            }
        }
        variants[cnt].Set("a(yv)", fieldCnt, ruleArgs);
        variants[cnt].SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }
    *retArgs = variants;
    return ER_OK;
}

static QStatus BuildMembersFromArg(MsgArg& arg, PermissionPolicy::Rule::Member** rules, size_t* count)
{
    MsgArg* ruleListArgs = NULL;
    size_t ruleCount;
    QStatus status = arg.Get("aa(yv)", &ruleCount, &ruleListArgs);
    if (ER_OK != status) {
        QCC_DbgPrintf(("BuildMembersFromArg #1 got status 0x%x\n", status));
        return status;
    }
    if (ruleCount == 0) {
        *count = ruleCount;
        return ER_OK;
    }
    PermissionPolicy::Rule::Member* ruleArray = new PermissionPolicy::Rule::Member[ruleCount];
    for (size_t cnt = 0; cnt < ruleCount; cnt++) {
        MsgArg* fieldArgs;
        size_t fieldCnt;
        status = ruleListArgs[cnt].Get("a(yv)", &fieldCnt, &fieldArgs);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildMembersFromArg #3 [%d] got status 0x%x\n", cnt, status));
            delete [] ruleArray;
            return status;
        }
        PermissionPolicy::Rule::Member* pr = &ruleArray[cnt];
        for (size_t fld = 0; fld < fieldCnt; fld++) {
            uint8_t fieldType;
            MsgArg* field;
            status = fieldArgs[fld].Get("(yv)", &fieldType, &field);
            if (ER_OK != status) {
                QCC_DbgPrintf(("BuildMembersFromArg #4 [%d][%d] got status 0x%x\n", cnt, fld, status));
                delete [] ruleArray;
                return status;
            }
            char* str;
            bool boolVal;
            uint8_t oneByte;
            switch (fieldType) {
            case PermissionPolicy::Rule::Member::TAG_MEMBER_NAME:
                status = field->Get("s", &str);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildMembersFromArg #6 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete [] ruleArray;
                    return status;
                }
                pr->SetMemberName(String(str));
                break;

            case PermissionPolicy::Rule::Member::TAG_MEMBER_TYPE:
                status = field->Get("y", &oneByte);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildMembersFromArg #7 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete [] ruleArray;
                    return status;
                }
                if ((oneByte >= PermissionPolicy::Rule::Member::METHOD_CALL) && (oneByte <= PermissionPolicy::Rule::Member::PROPERTY)) {
                    pr->SetMemberType((PermissionPolicy::Rule::Member::MemberType) oneByte);
                } else {
                    QCC_DbgPrintf(("BuildMembersFromArg #8 [%d][%d] got invalid member type %d\n", cnt, fld, oneByte));
                    delete [] ruleArray;
                    return ER_INVALID_DATA;
                }
                break;

            case PermissionPolicy::Rule::Member::TAG_READ_ONLY:
                status = field->Get("b", &boolVal);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildMembersFromArg #9 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete [] ruleArray;
                    return status;
                }
                pr->SetReadOnly(boolVal);
                break;

            case PermissionPolicy::Rule::Member::TAG_MUTUAL_AUTH:
                status = field->Get("b", &boolVal);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildMembersFromArg #9 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete [] ruleArray;
                    return status;
                }
                pr->SetMutualAuth(boolVal);
                break;
            }
        }
    }
    *count = ruleCount;
    *rules = ruleArray;
    return ER_OK;
}

static QStatus BuildRulesFromArg(MsgArg& arg, PermissionPolicy::Rule** rules, size_t* count)
{
    MsgArg* ruleListArgs = NULL;
    size_t ruleCount;
    QStatus status = arg.Get("aa(yv)", &ruleCount, &ruleListArgs);
    if (ER_OK != status) {
        QCC_DbgPrintf(("BuildRulesFromArg #1 got status 0x%x\n", status));
        return status;
    }
    if (ruleCount == 0) {
        *count = ruleCount;
        return ER_OK;
    }
    PermissionPolicy::Rule* ruleArray = new PermissionPolicy::Rule[ruleCount];
    for (size_t cnt = 0; cnt < ruleCount; cnt++) {
        MsgArg* fieldArgs;
        size_t fieldCnt;
        status = ruleListArgs[cnt].Get("a(yv)", &fieldCnt, &fieldArgs);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildRulesFromArg #3 [%d] got status 0x%x\n", cnt, status));
            delete ruleArray;
            return status;
        }
        PermissionPolicy::Rule* pr = &ruleArray[cnt];
        for (size_t fld = 0; fld < fieldCnt; fld++) {
            uint8_t fieldType;
            MsgArg* field;
            status = fieldArgs[fld].Get("(yv)", &fieldType, &field);
            if (ER_OK != status) {
                QCC_DbgPrintf(("BuildRulesFromArg #4 [%d][%d] got status 0x%x\n", cnt, fld, status));
                delete ruleArray;
                return status;
            }
            char* str;
            switch (fieldType) {
            case PermissionPolicy::Rule::TAG_OBJPATH:
                status = field->Get("s", &str);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildRulesFromArg #5 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete ruleArray;
                    return status;
                }
                pr->SetObjPath(String(str));
                break;

            case PermissionPolicy::Rule::TAG_INTERFACE_NAME:
                status = field->Get("s", &str);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildRulesFromArg #6 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete ruleArray;
                    return status;
                }
                pr->SetInterfaceName(String(str));
                break;

            case PermissionPolicy::Rule::TAG_INTERFACE_MEMBERS:
                PermissionPolicy::Rule::Member * memberRules;
                size_t ruleCount;
                status = BuildMembersFromArg(*field, &memberRules, &ruleCount);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildRulesFromArg #6 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete ruleArray;
                    return status;
                }
                if (ruleCount > 0) {
                    pr->SetMembers(ruleCount, memberRules);
                }
                break;
            }
        }
    }
    *count = ruleCount;
    *rules = ruleArray;
    return ER_OK;
}

static QStatus BuildProviderFromArg(MsgArg& arg, PermissionPolicy::Term** providerTerms, size_t* count)
{
    MsgArg* termListArgs = NULL;
    size_t termCount;
    QStatus status = arg.Get("aa(yv)", &termCount, &termListArgs);
    if (ER_OK != status) {
        QCC_DbgPrintf(("BuildProviderFromArg #1 got status 0x%x\n", status));
        return status;
    }
    if (termCount == 0) {
        *count = termCount;
        return ER_OK;
    }
    PermissionPolicy::Term* termArray = new PermissionPolicy::Term[termCount];
    for (size_t cnt = 0; cnt < termCount; cnt++) {
        MsgArg* itemArgs;
        size_t itemCount;
        status = termListArgs[cnt].Get("a(yv)", &itemCount, &itemArgs);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildProviderFromArg #3 [%d] got status 0x%x.  The signature is %s\n", cnt, status, termListArgs[cnt].Signature().c_str()));
            delete [] termArray;
            return status;
        }
        PermissionPolicy::Term* pt = &termArray[cnt];
        for (size_t idx = 0; idx < itemCount; idx++) {
            MsgArg* field;
            uint8_t fieldType;
            status = itemArgs[idx].Get("(yv)", &fieldType, &field);
            if (ER_OK != status) {
                QCC_DbgPrintf(("BuildProviderFromArg #4 [%d] got status 0x%x\n", cnt, status));
                delete [] termArray;
                return status;
            }
            switch (fieldType) {
            case PermissionPolicy::Term::TAG_PEER:
                {
                    PermissionPolicy::Peer* peers;
                    size_t peerCount;
                    status = BuildPeersFromArg(*field, &peers, &peerCount);
                    if (ER_OK != status) {
                        QCC_DbgPrintf(("BuildProviderFromArg #5 [%d] got status 0x%x\n", cnt, status));
                        delete [] termArray;
                        return status;
                    }
                    if (peerCount > 0) {
                        pt->SetPeers(peerCount, peers);
                    }
                }
                break;

            case PermissionPolicy::Term::TAG_ALLOW:
                {
                    PermissionPolicy::Rule* rules;
                    size_t ruleCount;
                    status = BuildRulesFromArg(*field, &rules, &ruleCount);
                    if (ER_OK != status) {
                        QCC_DbgPrintf(("BuildProviderFromArg #6 [%d] got status 0x%x\n", cnt, status));
                        delete [] termArray;
                        return status;
                    }
                    if (ruleCount > 0) {
                        pt->SetAllowRules(ruleCount, rules);
                    }
                }
                break;

            case PermissionPolicy::Term::TAG_ALLOWALLEXCEPT:
                {
                    PermissionPolicy::Rule* rules;
                    size_t ruleCount;
                    status = BuildRulesFromArg(*field, &rules, &ruleCount);
                    if (ER_OK != status) {
                        QCC_DbgPrintf(("BuildProviderFromArg #6 [%d] got status 0x%x\n", cnt, status));
                        delete [] termArray;
                        return status;
                    }
                    if (ruleCount > 0) {
                        pt->SetAllowAllExceptRules(ruleCount, rules);
                    }
                }
                break;
            }
        }
    }

    *count = termCount;
    *providerTerms = termArray;
    return ER_OK;

}

static QStatus BuildConsumerFromArg(MsgArg& arg, PermissionPolicy::ACL** consumerACLs, size_t* count)
{
    MsgArg* aclListArgs = NULL;
    size_t aclCount;
    QStatus status = arg.Get("aa(yv)", &aclCount, &aclListArgs);
    if (ER_OK != status) {
        QCC_DbgPrintf(("BuildConsumerFromArg #1 got status 0x%x\n", status));
        return status;
    }
    if (aclCount == 0) {
        *count = aclCount;
        return ER_OK;
    }
    PermissionPolicy::Term* aclArray = new PermissionPolicy::Term[aclCount];
    for (size_t cnt = 0; cnt < aclCount; cnt++) {
        MsgArg* itemArgs;
        size_t itemCount;
        status = aclListArgs[cnt].Get("a(yv)", &itemCount, &itemArgs);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildConsumerFromArg #3 [%d] got status 0x%x.  The signature is %s\n", cnt, status, aclListArgs[cnt].Signature().c_str()));
            delete aclArray;
            return status;
        }
        PermissionPolicy::ACL* acl = &aclArray[cnt];
        for (size_t idx = 0; idx < itemCount; idx++) {
            MsgArg* field;
            uint8_t fieldType;
            status = itemArgs[idx].Get("(yv)", &fieldType, &field);
            if (ER_OK != status) {
                QCC_DbgPrintf(("BuildConsumerFromArg #4 [%d] got status 0x%x\n", cnt, status));
                delete aclArray;
                return status;
            }
            switch (fieldType) {
            case PermissionPolicy::ACL::TAG_ALLOW:
                {
                    PermissionPolicy::Rule* rules;
                    size_t ruleCount;
                    status = BuildRulesFromArg(*field, &rules, &ruleCount);
                    if (ER_OK != status) {
                        QCC_DbgPrintf(("BuildConsumerFromArg #6 [%d] got status 0x%x\n", cnt, status));
                        delete aclArray;
                        return status;
                    }
                    if (ruleCount > 0) {
                        acl->SetAllowRules(ruleCount, rules);
                    }
                }
                break;

            case PermissionPolicy::ACL::TAG_ALLOWALLEXCEPT:
                {
                    PermissionPolicy::Rule* rules;
                    size_t ruleCount;
                    status = BuildRulesFromArg(*field, &rules, &ruleCount);
                    if (ER_OK != status) {
                        QCC_DbgPrintf(("BuildConsumerFromArg #6 [%d] got status 0x%x\n", cnt, status));
                        delete aclArray;
                        return status;
                    }
                    if (ruleCount > 0) {
                        acl->SetAllowAllExceptRules(ruleCount, rules);
                    }
                }
                break;
            }
        }
    }

    *count = aclCount;
    *consumerACLs = aclArray;
    return ER_OK;
}

QStatus PermissionPolicy::GeneratePolicyArgs(MsgArg& msgArg, PermissionPolicy& policy)
{

    MsgArg* sectionVariants = NULL;
    /* count the sections */
    size_t sectionCount = 0;
    size_t sectionIndex = 0;

    if (policy.GetAdmins()) {
        sectionCount++;
    }
    if (policy.GetProviders()) {
        sectionCount++;
    }
    if (policy.GetConsumers()) {
        sectionCount++;
    }
    if (sectionCount > 0) {
        sectionVariants = new MsgArg[sectionCount];
    }

    if (policy.GetAdmins()) {
        MsgArg* adminVariants;
        GeneratePeerArgs(&adminVariants, (PermissionPolicy::Peer*) policy.GetAdmins(), policy.GetAdminSize());
        sectionVariants[sectionIndex++].Set("(yv)", PermissionPolicy::TAG_ADMIN,
                                            new MsgArg("a(ya(yv))", policy.GetAdminSize(), adminVariants));
    }
    if (policy.GetProviders()) {
        MsgArg* providerVariants = new MsgArg[policy.GetProviderSize()];
        PermissionPolicy::Term* providers = (PermissionPolicy::Term*) policy.GetProviders();
        for (size_t cnt = 0; cnt < policy.GetProviderSize(); cnt++) {
            PermissionPolicy::Term* aTerm = &providers[cnt];
            /* count the item in a term */
            size_t termItemCount = 0;
            if (aTerm->GetPeers()) {
                termItemCount++;
            }
            if (aTerm->GetAllowRules()) {
                termItemCount++;
            }
            if (aTerm->GetAllowAllExceptRules()) {
                termItemCount++;
            }
            MsgArg* termItems = new MsgArg[termItemCount];
            size_t idx = 0;

            if (aTerm->GetPeers()) {
                MsgArg* peerVariants;
                GeneratePeerArgs(&peerVariants, (PermissionPolicy::Peer*) aTerm->GetPeers(), aTerm->GetPeerSize());
                termItems[idx++].Set("(yv)", PermissionPolicy::Term::TAG_PEER,
                                     new MsgArg("a(ya(yv))", aTerm->GetPeerSize(), peerVariants));
            }
            if (aTerm->GetAllowRules()) {
                MsgArg* allowVariants = NULL;
                GenerateRuleArgs(&allowVariants, (PermissionPolicy::Rule*) aTerm->GetAllowRules(), aTerm->GetAllowRuleSize());
                termItems[idx++].Set("(yv)", PermissionPolicy::Term::TAG_ALLOW,
                                     new MsgArg("aa(yv)", aTerm->GetAllowRuleSize(), allowVariants));
            }
            if (aTerm->GetAllowAllExceptRules()) {
                MsgArg* allowVariants = NULL;
                GenerateRuleArgs(&allowVariants, (Rule*) aTerm->GetAllowAllExceptRules(), aTerm->GetAllowAllExceptRuleSize());

                termItems[idx++].Set("(yv)", PermissionPolicy::Term::TAG_ALLOWALLEXCEPT,
                                     new MsgArg("aa(yv)", aTerm->GetAllowAllExceptRuleSize(), allowVariants));
            }
            providerVariants[cnt].Set("a(yv)", termItemCount, termItems);
        }
        sectionVariants[sectionIndex++].Set("(yv)", PermissionPolicy::TAG_PROVIDER,
                                            new MsgArg("aa(yv)", policy.GetProviderSize(), providerVariants));
    }

    if (policy.GetConsumers()) {
        MsgArg* consumerVariants = new MsgArg[policy.GetConsumerSize()];
        PermissionPolicy::ACL* consumers = (PermissionPolicy::ACL*) policy.GetConsumers();
        for (size_t cnt = 0; cnt < policy.GetConsumerSize(); cnt++) {
            ACL* anACL = &consumers[cnt];
            /* count the item in an ACL */
            size_t aclItemCount = 0;
            if (anACL->GetAllowRules()) {
                aclItemCount++;
            }
            if (anACL->GetAllowAllExceptRules()) {
                aclItemCount++;
            }
            MsgArg* aclItems = new MsgArg[aclItemCount];
            size_t idx = 0;
            if (anACL->GetAllowRules()) {
                MsgArg* allowVariants = NULL;
                GenerateRuleArgs(&allowVariants, (Rule*) anACL->GetAllowRules(), anACL->GetAllowRuleSize());
                aclItems[idx++].Set("(yv)", Term::TAG_ALLOW,
                                    new MsgArg("aa(yv)", anACL->GetAllowRuleSize(), allowVariants));
            }
            if (anACL->GetAllowAllExceptRules()) {
                MsgArg* allowVariants = NULL;
                GenerateRuleArgs(&allowVariants, (Rule*) anACL->GetAllowAllExceptRules(), anACL->GetAllowAllExceptRuleSize());
                aclItems[idx++].Set("(yv)", PermissionPolicy::Term::TAG_ALLOWALLEXCEPT,
                                    new MsgArg("aa(yv)", anACL->GetAllowAllExceptRuleSize(), allowVariants));
            }
            consumerVariants[cnt].Set("a(yv)", aclItemCount, aclItems);
        }
        sectionVariants[sectionIndex++].Set("(yv)", PermissionPolicy::TAG_CONSUMER,
                                            new MsgArg("aa(yv)", policy.GetConsumerSize(), consumerVariants));
    }

    msgArg.Set("(yv)", policy.GetVersion(), new MsgArg("(ua(yv))",
                                                       policy.GetSerialNum(), sectionCount, sectionVariants));
    msgArg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    return ER_OK;
}


QStatus PermissionPolicy::BuildPolicyFromArgs(uint8_t version, const MsgArg& msgArg, PermissionPolicy& policy)
{
    QStatus status;
    MsgArg* sectionVariants;
    size_t sectionCount;
    uint32_t serialNum;
    status = msgArg.Get("(ua(yv))", &serialNum, &sectionCount, &sectionVariants);
    if (ER_OK != status) {
        QCC_DbgPrintf(("BuildPolicyFromArgs #1 got status 0x%x\n", status));
        return status;
    }
    policy.SetVersion(version);
    policy.SetSerialNum(serialNum);

    for (size_t cnt = 0; cnt < sectionCount; cnt++) {
        uint8_t sectionType;
        MsgArg* section;
        status = sectionVariants[cnt].Get("(yv)", &sectionType, &section);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildPolicyFromArgs #2 got status 0x%x\n", status));
            return status;
        }
        switch (sectionType) {
        case PermissionPolicy::TAG_ADMIN:
            {
                PermissionPolicy::Peer* peers;
                size_t count;
                status = BuildPeersFromArg(*section, &peers, &count);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildPolicyFromArgs #3 got status 0x%x\n", status));
                    return status;
                }
                if (count > 0) {
                    policy.SetAdmins(count, peers);
                }
            }
            break;

        case PermissionPolicy::TAG_PROVIDER:
            {
                PermissionPolicy::Term* terms;
                size_t count;
                status = BuildProviderFromArg(*section, &terms, &count);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildPolicyFromArgs #4 got status 0x%x\n", status));
                    return status;
                }
                if (count > 0) {
                    policy.SetProviders(count, terms);
                }
            }
            break;

        case PermissionPolicy::TAG_CONSUMER:
            {
                PermissionPolicy::ACL* acls;
                size_t count;
                status = BuildConsumerFromArg(*section, &acls, &count);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildPolicyFromArgs #5 got status 0x%x\n", status));
                    return status;
                }
                if (count > 0) {
                    policy.SetConsumers(count, acls);
                }
            }
            break;
        }
    }
    QCC_DbgPrintf(("BuildPolicyFromArgs got pocily %s", policy.ToString().c_str()));

    return ER_OK;
}

} /* namespace ajn */

