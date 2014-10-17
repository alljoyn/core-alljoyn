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
    str += "  action mask:";
    if ((actionMask & ACTION_DENIED) == ACTION_DENIED) {
        str += " Denied";
    }
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

qcc::String PermissionPolicy::Rule::ToString()
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

qcc::String PermissionPolicy::Term::ToString()
{
    qcc::String str;
    str += "Term:\n";
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

qcc::String PermissionPolicy::ToString()
{
    qcc::String str;
    str += "PermissionPolicy:\n";
    str += "  version: " +  U32ToString(version) + "\n";
    str += "  serial number: " + U32ToString(serialNum) + "\n";

    if ((adminsSize > 0) && admins) {
        for (size_t cnt = 0; cnt < adminsSize; cnt++) {
            str += "  admins[" + U32ToString(cnt) + "]: " + admins[cnt].ToString();
        }
    }
    if ((termsSize > 0) && terms) {
        for (size_t cnt = 0; cnt < termsSize; cnt++) {
            str += "  terms[" + U32ToString(cnt) + "]: " + terms[cnt].ToString();
        }
    }
    return str;
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
        /* account for action mask */
        fieldCnt++;
        /* default is true for mutual auth */
        if (!aRule->GetMutualAuth()) {
            fieldCnt++;
        }
        MsgArg* ruleArgs = NULL;
        if (fieldCnt > 0) {
            int idx = 0;
            ruleArgs = new MsgArg[fieldCnt];
            if (!aRule->GetMemberName().empty()) {
                ruleArgs[idx++].Set("(yv)", PermissionPolicy::Rule::Member::TAG_MEMBER_NAME,
                                    new MsgArg("s", aRule->GetMemberName().c_str()));
            }
            if (aRule->GetMemberType() != PermissionPolicy::Rule::Member::NOT_SPECIFIED) {
                ruleArgs[idx++].Set("(yv)", PermissionPolicy::Rule::Member::TAG_MEMBER_TYPE,
                                    new MsgArg("y", aRule->GetMemberType()));
            }
            ruleArgs[idx++].Set("(yv)", PermissionPolicy::Rule::Member::TAG_ACTION_MASK,
                                new MsgArg("y", aRule->GetActionMask()));
            if (!aRule->GetMutualAuth()) {
                ruleArgs[idx++].Set("(yv)", PermissionPolicy::Rule::Member::TAG_MUTUAL_AUTH,
                                    new MsgArg("b", aRule->GetMutualAuth()));
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
        if (!aRule->GetObjPath().empty()) {
            fieldCnt++;
        }
        if (aRule->GetInterfaceName().size() > 0) {
            fieldCnt++;
        }
        if (aRule->GetMembersSize() > 0) {
            fieldCnt++;
        }
        MsgArg* ruleArgs = NULL;
        if (fieldCnt > 0) {
            int idx = 0;
            ruleArgs = new MsgArg[fieldCnt];
            if (!aRule->GetObjPath().empty()) {
                ruleArgs[idx++].Set("(yv)", PermissionPolicy::Rule::TAG_OBJPATH,
                                    new MsgArg("s", aRule->GetObjPath().c_str()));
            }
            if (!aRule->GetInterfaceName().empty()) {
                ruleArgs[idx++].Set("(yv)", PermissionPolicy::Rule::TAG_INTERFACE_NAME,
                                    new MsgArg("s", aRule->GetInterfaceName().c_str()));
            }
            if (aRule->GetMembersSize() > 0) {
                MsgArg* ruleMembersArgs = NULL;
                GenerateMemberArgs(&ruleMembersArgs, aRule->GetMembers(), aRule->GetMembersSize());
                ruleArgs[idx++].Set("(yv)", PermissionPolicy::Rule::TAG_INTERFACE_MEMBERS,
                                    new MsgArg("aa(yv)", aRule->GetMembersSize(), ruleMembersArgs));
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

            case PermissionPolicy::Rule::Member::TAG_ACTION_MASK:
                status = field->Get("y", &oneByte);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildMembersFromArg #9 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete [] ruleArray;
                    return status;
                }
                pr->SetActionMask(oneByte);
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
            delete [] ruleArray;
            return status;
        }
        PermissionPolicy::Rule* pr = &ruleArray[cnt];
        for (size_t fld = 0; fld < fieldCnt; fld++) {
            uint8_t fieldType;
            MsgArg* field;
            status = fieldArgs[fld].Get("(yv)", &fieldType, &field);
            if (ER_OK != status) {
                QCC_DbgPrintf(("BuildRulesFromArg #4 [%d][%d] got status 0x%x\n", cnt, fld, status));
                delete [] ruleArray;
                return status;
            }
            char* str;
            switch (fieldType) {
            case PermissionPolicy::Rule::TAG_OBJPATH:
                status = field->Get("s", &str);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildRulesFromArg #5 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete [] ruleArray;
                    return status;
                }
                pr->SetObjPath(String(str));
                break;

            case PermissionPolicy::Rule::TAG_INTERFACE_NAME:
                status = field->Get("s", &str);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildRulesFromArg #6 [%d][%d] got status 0x%x\n", cnt, fld, status));
                    delete [] ruleArray;
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
                    delete [] ruleArray;
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

static QStatus BuildTermsFromArg(MsgArg& arg, PermissionPolicy::Term** terms, size_t* count)
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
            QCC_DbgPrintf(("BuildTermsFromArg #3 [%d] got status 0x%x.  The signature is %s\n", cnt, status, termListArgs[cnt].Signature().c_str()));
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
            case PermissionPolicy::Term::TAG_PEERS:
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

            case PermissionPolicy::Term::TAG_RULES:
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
                        pt->SetRules(ruleCount, rules);
                    }
                }
                break;
            }
        }
    }

    *count = termCount;
    *terms = termArray;
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
    if (policy.GetTerms()) {
        sectionCount++;
    }
    if (sectionCount > 0) {
        sectionVariants = new MsgArg[sectionCount];
    }

    if (policy.GetAdmins()) {
        MsgArg* adminVariants;
        GeneratePeerArgs(&adminVariants, (PermissionPolicy::Peer*) policy.GetAdmins(), policy.GetAdminsSize());
        sectionVariants[sectionIndex++].Set("(yv)", PermissionPolicy::TAG_ADMINS,
                                            new MsgArg("a(ya(yv))", policy.GetAdminsSize(), adminVariants));
    }
    if (policy.GetTerms()) {
        MsgArg* termsVariants = new MsgArg[policy.GetTermsSize()];
        PermissionPolicy::Term* terms = (PermissionPolicy::Term*) policy.GetTerms();
        for (size_t cnt = 0; cnt < policy.GetTermsSize(); cnt++) {
            PermissionPolicy::Term* aTerm = &terms[cnt];
            /* count the item in a term */
            size_t termItemCount = 0;
            if (aTerm->GetPeers()) {
                termItemCount++;
            }
            if (aTerm->GetRules()) {
                termItemCount++;
            }
            MsgArg* termItems = new MsgArg[termItemCount];
            size_t idx = 0;

            if (aTerm->GetPeers()) {
                MsgArg* peerVariants;
                GeneratePeerArgs(&peerVariants, (PermissionPolicy::Peer*) aTerm->GetPeers(), aTerm->GetPeersSize());
                termItems[idx++].Set("(yv)", PermissionPolicy::Term::TAG_PEERS,
                                     new MsgArg("a(ya(yv))", aTerm->GetPeersSize(), peerVariants));
            }
            if (aTerm->GetRules()) {
                MsgArg* rulesVariants = NULL;
                GenerateRuleArgs(&rulesVariants, (PermissionPolicy::Rule*) aTerm->GetRules(), aTerm->GetRulesSize());
                termItems[idx++].Set("(yv)", PermissionPolicy::Term::TAG_RULES,
                                     new MsgArg("aa(yv)", aTerm->GetRulesSize(), rulesVariants));
            }
            termsVariants[cnt].Set("a(yv)", termItemCount, termItems);
        }
        sectionVariants[sectionIndex++].Set("(yv)", PermissionPolicy::TAG_TERMS,
                                            new MsgArg("aa(yv)", policy.GetTermsSize(), termsVariants));
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
        case PermissionPolicy::TAG_ADMINS:
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

        case PermissionPolicy::TAG_TERMS:
            {
                PermissionPolicy::Term* terms;
                size_t count;
                status = BuildTermsFromArg(*section, &terms, &count);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildPolicyFromArgs #4 got status 0x%x\n", status));
                    return status;
                }
                if (count > 0) {
                    policy.SetTerms(count, terms);
                }
            }
            break;
        }
    }
    QCC_DbgPrintf(("BuildPolicyFromArgs got pocily %s", policy.ToString().c_str()));

    return ER_OK;
}

} /* namespace ajn */

