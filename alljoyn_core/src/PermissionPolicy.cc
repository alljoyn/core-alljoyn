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
    if (level == PEER_LEVEL_NONE) {
        str += "  level: none\n";
    } else if (level == PEER_LEVEL_ENCRYPTED) {
        str += "  level: encrypted\n";
    } else if (level == PEER_LEVEL_AUTHENTICATED) {
        str += "  level: authenticated\n";
    } else if (level == PEER_LEVEL_AUTHORIZED) {
        str += "  level: authorized\n";
    }
    if (type == PEER_ANY) {
        str += "  type: any\n";
    } else if (type == PEER_GUID) {
        str += "  type: GUID\n";
        if (keyInfo) {
            str += keyInfo->ToString();
        }
    } else if (type == PEER_GUILD) {
        str += "  type: guild guildId: " + guildId.ToString() + "\n";
        if (keyInfo) {
            str += keyInfo->ToString();
        }
    }
    return str;
}

qcc::String PermissionPolicy::Term::ToString() const
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

qcc::String PermissionPolicy::ToString() const
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
        if (peers[cnt].GetType() == PermissionPolicy::Peer::PEER_ANY) {
            variants[cnt].Set("(yyayv)", peers[cnt].GetLevel(), peers[cnt].GetType(), (size_t) 0, NULL, new MsgArg("ay", 0, NULL));
        } else {
            const KeyInfoECC* keyInfo = peers[cnt].GetKeyInfo();
            if (!keyInfo) {
                delete [] variants;
                return ER_INVALID_DATA;
            }
            if (!KeyInfoHelper::InstanceOfKeyInfoNISTP256(*keyInfo)) {
                delete [] variants;
                return ER_NOT_IMPLEMENTED;
            }
            KeyInfoNISTP256* keyInfoNISTP256 = (qcc::KeyInfoNISTP256*) keyInfo;
            MsgArg* keyInfoArg = new MsgArg();
            KeyInfoHelper::KeyInfoNISTP256ToMsgArg(*keyInfoNISTP256, *keyInfoArg);
            size_t guildIdLen = 0;
            uint8_t*guildId = NULL;
            if (peers[cnt].GetType() == PermissionPolicy::Peer::PEER_GUILD) {
                guildIdLen = GUID128::SIZE;
                guildId = (uint8_t*) peers[cnt].GetGuildId().GetBytes();
            }
            variants[cnt].Set("(yyayv)", peers[cnt].GetLevel(), peers[cnt].GetType(), guildIdLen, guildId, keyInfoArg);
        }
        variants[cnt].SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }
    *retArgs = variants;
    return ER_OK;
}

static QStatus BuildPeersFromArg(MsgArg& arg, PermissionPolicy::Peer** peers, size_t* count)
{
    MsgArg* peerListArgs = NULL;
    size_t peerCount;
    QStatus status = arg.Get("a(yyayv)", &peerCount, &peerListArgs);
    if (ER_OK != status) {
        return status;
    }
    if (peerCount == 0) {
        *count = peerCount;
        return ER_OK;
    }
    PermissionPolicy::Peer* peerArray = new PermissionPolicy::Peer[peerCount];
    for (size_t cnt = 0; cnt < peerCount; cnt++) {
        uint8_t peerLevel;
        uint8_t peerType;
        size_t guildIdLen;
        uint8_t* guildId;
        MsgArg* variant;
        status = peerListArgs[cnt].Get("(yyayv)", &peerLevel, &peerType, &guildIdLen, &guildId, &variant);
        if (ER_OK != status) {
            delete [] peerArray;
            return status;
        }
        PermissionPolicy::Peer* pp = &peerArray[cnt];
        if ((peerLevel >= PermissionPolicy::Peer::PEER_LEVEL_NONE) && (peerLevel <= PermissionPolicy::Peer::PEER_LEVEL_AUTHORIZED)) {
            pp->SetLevel((PermissionPolicy::Peer::PeerAuthLevel) peerLevel);
        } else {
            delete [] peerArray;
            return ER_INVALID_DATA;
        }
        if ((peerType >= PermissionPolicy::Peer::PEER_ANY) && (peerType <= PermissionPolicy::Peer::PEER_GUILD)) {
            pp->SetType((PermissionPolicy::Peer::PeerType) peerType);
        } else {
            delete [] peerArray;
            return ER_INVALID_DATA;
        }
        if (peerType == PermissionPolicy::Peer::PEER_ANY) {
            continue;
        } else if (peerType == PermissionPolicy::Peer::PEER_GUILD) {
            if (guildIdLen == GUID128::SIZE) {
                GUID128 guid(0);
                guid.SetBytes(guildId);
                pp->SetGuildId(guid);
            }
        }
        KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
        status = KeyInfoHelper::MsgArgToKeyInfoNISTP256(*variant, *keyInfo);
        if (ER_OK != status) {
            delete keyInfo;
            delete [] peerArray;
            return status;
        }
        pp->SetKeyInfo(keyInfo);
    }

    *count = peerCount;
    *peers = peerArray;
    return ER_OK;
}

static QStatus GenerateMemberArgs(MsgArg** retArgs, const PermissionPolicy::Rule::Member* rules, size_t count)
{
    *retArgs = new MsgArg[count];
    for (size_t cnt = 0; cnt < count; cnt++) {
        QStatus status = (*retArgs)[cnt].Set("(syy)",
                                             rules[cnt].GetMemberName().c_str(),
                                             rules[cnt].GetMemberType(),
                                             rules[cnt].GetActionMask());
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
    *retArgs = new MsgArg[count];
    for (size_t cnt = 0; cnt < count; cnt++) {
        PermissionPolicy::Rule* aRule = &rules[cnt];
        MsgArg* ruleMembersArgs = NULL;
        if (aRule->GetMembersSize() > 0) {
            status = GenerateMemberArgs(&ruleMembersArgs, aRule->GetMembers(), aRule->GetMembersSize());
            if (ER_OK != status) {
                goto exit;
            }
        }
        status = (*retArgs)[cnt].Set("(ssa(syy))",
                                     aRule->GetObjPath().c_str(),
                                     aRule->GetInterfaceName().c_str(),
                                     aRule->GetMembersSize(), ruleMembersArgs);
        if (ER_OK != status) {
            goto exit;
        }
        (*retArgs)[cnt].SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }
    return ER_OK;
exit:
    delete [] *retArgs;
    *retArgs = NULL;
    return status;
}

static QStatus BuildMembersFromArg(const MsgArg* arg, PermissionPolicy::Rule::Member** rules, size_t count)
{
    if (count == 0) {
        *rules = NULL;
        return ER_OK;
    }
    PermissionPolicy::Rule::Member* ruleArray = new PermissionPolicy::Rule::Member[count];
    for (size_t cnt = 0; cnt < count; cnt++) {
        char* str;
        uint8_t memberType;
        uint8_t actionMask;
        QStatus status = arg[cnt].Get("(syy)", &str, &memberType, &actionMask);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildMembersFromArg [%d] got status 0x%x\n", cnt, status));
            delete [] ruleArray;
            return status;
        }
        PermissionPolicy::Rule::Member* pr = &ruleArray[cnt];
        pr->SetMemberName(String(str));
        if ((memberType >= PermissionPolicy::Rule::Member::NOT_SPECIFIED) && (memberType <= PermissionPolicy::Rule::Member::PROPERTY)) {
            pr->SetMemberType((PermissionPolicy::Rule::Member::MemberType) memberType);
        } else {
            QCC_DbgPrintf(("BuildMembersFromArg [%d] got invalid member type %d\n", cnt, memberType));
            delete [] ruleArray;
            return ER_INVALID_DATA;
        }
        pr->SetActionMask(actionMask);
    }
    *rules = ruleArray;
    return ER_OK;
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
    if (argCount == 0) {
        *rules = NULL;
        return ER_OK;
    }
    PermissionPolicy::Rule* ruleArray = new PermissionPolicy::Rule[argCount];
    for (size_t cnt = 0; cnt < argCount; cnt++) {
        char* objPath;
        char* interfaceName;
        MsgArg* membersArgs = NULL;
        size_t membersArgsCount = 0;
        status = args[cnt].Get("(ssa(syy))", &objPath, &interfaceName, &membersArgsCount, &membersArgs);
        if (ER_OK != status) {
            QCC_DbgPrintf(("BuildRulesFromArg [%d] got status 0x%x\n", cnt, status));
            delete [] ruleArray;
            return status;
        }
        PermissionPolicy::Rule* pr = &ruleArray[cnt];
        pr->SetObjPath(String(objPath));
        pr->SetInterfaceName(String(interfaceName));
        if (membersArgsCount > 0) {
            PermissionPolicy::Rule::Member* memberRules = NULL;
            status = BuildMembersFromArg(membersArgs, &memberRules, membersArgsCount);
            if (ER_OK != status) {
                QCC_DbgPrintf(("BuildRulesFromArg [%d] got status 0x%x\n", cnt, status));
                delete [] ruleArray;
                return status;
            }
            pr->SetMembers(membersArgsCount, memberRules);
        }
    }

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

QStatus PermissionPolicy::Export(MsgArg& msgArg)
{

    MsgArg* sectionVariants = NULL;
    /* count the sections */
    size_t sectionCount = 0;
    size_t sectionIndex = 0;

    if (GetAdmins()) {
        sectionCount++;
    }
    if (GetTerms()) {
        sectionCount++;
    }
    if (sectionCount > 0) {
        sectionVariants = new MsgArg[sectionCount];
    }

    if (GetAdmins()) {
        MsgArg* adminVariants;
        GeneratePeerArgs(&adminVariants, (PermissionPolicy::Peer*) GetAdmins(), GetAdminsSize());
        sectionVariants[sectionIndex++].Set("(yv)", PermissionPolicy::TAG_ADMINS,
                                            new MsgArg("a(yyayv)", GetAdminsSize(), adminVariants));
    }
    if (GetTerms()) {
        MsgArg* termsVariants = new MsgArg[GetTermsSize()];
        PermissionPolicy::Term* terms = (PermissionPolicy::Term*) GetTerms();
        for (size_t cnt = 0; cnt < GetTermsSize(); cnt++) {
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
                                     new MsgArg("a(yyayv)", aTerm->GetPeersSize(), peerVariants));
            }
            if (aTerm->GetRules()) {
                MsgArg* rulesVariants = NULL;
                GenerateRuleArgs(&rulesVariants, (PermissionPolicy::Rule*) aTerm->GetRules(), aTerm->GetRulesSize());
                termItems[idx++].Set("(yv)", PermissionPolicy::Term::TAG_RULES,
                                     new MsgArg("a(ssa(syy))", aTerm->GetRulesSize(), rulesVariants));
            }
            termsVariants[cnt].Set("a(yv)", termItemCount, termItems);
        }
        sectionVariants[sectionIndex++].Set("(yv)", PermissionPolicy::TAG_TERMS,
                                            new MsgArg("aa(yv)", GetTermsSize(), termsVariants));
    }

    msgArg.Set("(yv)", GetVersion(), new MsgArg("(ua(yv))",
                                                GetSerialNum(), sectionCount, sectionVariants));
    msgArg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    return ER_OK;
}


QStatus PermissionPolicy::Import(uint8_t version, const MsgArg& msgArg)
{
    QStatus status;
    MsgArg* sectionVariants;
    size_t sectionCount = 0;
    uint32_t serialNum;
    status = msgArg.Get("(ua(yv))", &serialNum, &sectionCount, &sectionVariants);
    if (ER_OK != status) {
        QCC_DbgPrintf(("BuildPolicyFromArgs #1 got status 0x%x\n", status));
        return status;
    }
    SetVersion(version);
    SetSerialNum(serialNum);

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
                PermissionPolicy::Peer* peers = NULL;
                size_t count = 0;
                status = BuildPeersFromArg(*section, &peers, &count);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildPolicyFromArgs #3 got status 0x%x\n", status));
                    return status;
                }
                if (count > 0) {
                    SetAdmins(count, peers);
                }
            }
            break;

        case PermissionPolicy::TAG_TERMS:
            {
                PermissionPolicy::Term* terms = NULL;
                size_t count = 0;
                status = BuildTermsFromArg(*section, &terms, &count);
                if (ER_OK != status) {
                    QCC_DbgPrintf(("BuildPolicyFromArgs #4 got status 0x%x\n", status));
                    return status;
                }
                if (count > 0) {
                    SetTerms(count, terms);
                }
            }
            break;
        }
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
    return msg->MarshalMessage("(yv)", "", "", MESSAGE_ERROR, &args, 1, ALLJOYN_FLAG_SESSIONLESS, 0);
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
        uint8_t versionNum;
        MsgArg* variant;
        arg->Get("(yv)", &versionNum, &variant);
        return policy.Import(versionNum, *variant);
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
    MsgArg* rulesVariants = NULL;
    QStatus status = GenerateRuleArgs(&rulesVariants, (Rule*) rules, count);
    if (ER_OK != status) {
        return status;
    }
    msgArg.Set("a(ssa(syy))", count, rulesVariants);
    msgArg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    return status;
}

QStatus PermissionPolicy::ParseRules(MsgArg& msgArg, Rule** rules, size_t* count)
{
    return BuildRulesFromArg(msgArg, rules, count);
}

} /* namespace ajn */

