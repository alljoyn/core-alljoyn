/**
 * @file
 * AllJoyn-Daemon Policy database class
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>

#include <qcc/Debug.h>
#include <qcc/Logger.h>
#include <qcc/Util.h>

#include "PolicyDB.h"

using namespace ajn;
using namespace qcc;
using namespace std;

uint32_t _PolicyDB::GetStringIDMapUpdate(const qcc::String& key)
{
    uint32_t id;

    if (key.empty()) {
        id = NIL_MATCH;
    } else {
        StringIDMap::const_iterator it(stringIDs.find(key));

        if (it == stringIDs.end()) {
            id = stringIDs.size();
            stringIDs[key] = id;
        } else {
            id = it->second;
        }
    }
    return id;
}


uint32_t _PolicyDB::LookupStringID(const qcc::String& key) const
{
    uint32_t id;

    if (!key.empty()) {
        StringIDMap::const_iterator it(stringIDs.find(key));
        if (it == stringIDs.end()) {
            id = ID_NOT_FOUND;
        } else {
            id = it->second;
        }
    } else {
        id = WILDCARD;
    }
    return id;
}


uint32_t _PolicyDB::LookupStringID(const char* key) const
{
    uint32_t id;

    if (key && (key[0] != '\0')) {
        StringIDMap::const_iterator it(stringIDs.find(key));

        if (it == stringIDs.end()) {
            id = ID_NOT_FOUND;
        } else {
            id = it->second;
        }
    } else {
        id = WILDCARD;
    }
    return id;
}


bool _PolicyDB::MsgTypeStrToEnum(const qcc::String& str, AllJoynMessageType& type)
{
    bool success = true;
    if (str.compare("method_call") == 0) {
        type = MESSAGE_METHOD_CALL;
    } else if (str.compare("method_return") == 0) {
        type = MESSAGE_METHOD_RET;
    } else if (str.compare("signal") == 0) {
        type = MESSAGE_SIGNAL;
    } else if (str.compare("error") == 0) {
        type = MESSAGE_ERROR;
    } else {
        Log(LOG_ERR, "Invalid type for policy rule: \"%s\"\n", str.c_str());
        success = false;
    }
    return success;
}


bool _PolicyDB::TrueFalseStrToBool(const qcc::String& str, bool& b)
{
    bool success = true;
    if (str.compare("true") == 0) {
        b = true;
    } else if (str.compare("false") == 0) {
        b = false;
    } else {
        Log(LOG_ERR, "Invalid boolean in policy rule: \"%s\"\n", str.c_str());
        success = false;
    }
    return success;
}


_PolicyDB::_PolicyDB() : eavesdrop(false)
{
    stringIDs[""] = WILDCARD;
    stringIDs["*"] = WILDCARD;
}


bool _PolicyDB::AddRule(PolicyRuleList& ownList,
                        PolicyRuleList& sendList,
                        PolicyRuleList& receiveList,
                        PolicyRuleList& connectList,
                        policydb::PolicyPermission permission,
                        const map<qcc::String, qcc::String>& ruleAttrs)
{
    std::map<qcc::String, qcc::String>::const_iterator attr;
    bool success = true;
    PolicyRule rule(permission);

    static const unsigned int UNKNOWN =   0x0;
    static const unsigned int OWN =       0x1;
    static const unsigned int SEND =      0x2;
    static const unsigned int RECEIVE =   0x4;
    static const unsigned int CONNECT =   0x8;
    unsigned int policyGroup = UNKNOWN;

    ALLJOYN_POLICY_DEBUG(rule.ruleString = (permission == policydb::POLICY_ALLOW) ? "<allow" : "<deny");

    for (attr = ruleAttrs.begin(); success && (attr != ruleAttrs.end()); ++attr) {
        ALLJOYN_POLICY_DEBUG(rule.ruleString += " " + attr->first + "=\"" + attr->second + "\"");
        if (attr->first.compare("send_interface") == 0) {
            success = !(policyGroup & (RECEIVE | OWN | CONNECT));
            policyGroup |= SEND;
            rule.interface = GetStringIDMapUpdate(attr->second);
        } else if (attr->first.compare("send_member") == 0) {
            success = !(policyGroup & (RECEIVE | OWN | CONNECT));
            policyGroup |= SEND;
            rule.member = GetStringIDMapUpdate(attr->second);
        } else if (attr->first.compare("send_error") == 0) {
            success = !(policyGroup & (RECEIVE | OWN | CONNECT));
            policyGroup |= SEND;
            rule.error = GetStringIDMapUpdate(attr->second);
        } else if (attr->first.compare("send_destination") == 0) {
            success = !(policyGroup & (RECEIVE | OWN | CONNECT));
            policyGroup |= SEND;
            rule.busName = GetStringIDMapUpdate(attr->second);
            busNameMap[attr->second] = rule.busName;
        } else if (attr->first.compare("send_type") == 0) {
            success = !(policyGroup & (RECEIVE | OWN | CONNECT));
            policyGroup |= SEND;
            success = MsgTypeStrToEnum(attr->second, rule.type);
        } else if (attr->first.compare("send_path") == 0) {
            success = !(policyGroup & (RECEIVE | OWN | CONNECT));
            policyGroup |= SEND;
            rule.path = GetStringIDMapUpdate(attr->second);
        } else if (attr->first.compare("send_requested_reply") == 0) {
            policyGroup = 1 << (8 * sizeof(policyGroup) - 1);
            break;

        } else if (attr->first.compare("receive_interface") == 0) {
            success = !(policyGroup & (SEND | OWN | CONNECT));
            policyGroup |= RECEIVE;
            rule.interface = GetStringIDMapUpdate(attr->second);
        } else if (attr->first.compare("receive_member") == 0) {
            success = !(policyGroup & (SEND | OWN | CONNECT));
            policyGroup |= RECEIVE;
            rule.member = GetStringIDMapUpdate(attr->second);
        } else if (attr->first.compare("receive_error") == 0) {
            success = !(policyGroup & (SEND | OWN | CONNECT));
            policyGroup |= RECEIVE;
            rule.error = GetStringIDMapUpdate(attr->second);
        } else if (attr->first.compare("receive_sender") == 0) {
            success = !(policyGroup & (SEND | OWN | CONNECT));
            policyGroup |= RECEIVE;
            rule.busName = GetStringIDMapUpdate(attr->second);
            busNameMap[attr->second] = rule.busName;
        } else if (attr->first.compare("receive_type") == 0) {
            success = !(policyGroup & (SEND | OWN | CONNECT));
            policyGroup |= RECEIVE;
            success = MsgTypeStrToEnum(attr->second, rule.type);
        } else if (attr->first.compare("receive_path") == 0) {
            success = !(policyGroup & (SEND | OWN | CONNECT));
            policyGroup |= RECEIVE;
            rule.path = GetStringIDMapUpdate(attr->second);
        } else if (attr->first.compare("receive_requested_reply") == 0) {
            policyGroup = 1 << (8 * sizeof(policyGroup) - 1);
            break;

        } else if (attr->first.compare("own") == 0) {
            success = !(policyGroup & (SEND | RECEIVE | CONNECT)) && !rule.eavesdrop;
            policyGroup |= OWN;
            rule.own = GetStringIDMapUpdate(attr->second);
            busNameMap[attr->second] = rule.own;

        } else if (attr->first.compare("eavesdrop") == 0) {
            success = !(policyGroup & OWN);
            success = TrueFalseStrToBool(attr->second, rule.eavesdrop);
            eavesdrop |= rule.eavesdrop;

        } else if (attr->first.compare("user") == 0) {
            success = !(policyGroup & (SEND | RECEIVE | OWN));
            policyGroup |= CONNECT;
            rule.user = GetUsersUid(attr->second.c_str());
            rule.userSet = true;
        } else if (attr->first.compare("group") == 0) {
            success = !(policyGroup & (SEND | RECEIVE | OWN));
            policyGroup |= CONNECT;
            rule.user = GetUsersGid(attr->second.c_str());
            rule.groupSet = true;

        } else {
            Log(LOG_ERR, "Unknown policy attribute: \"%s\"\n", attr->first.c_str());
            success = false;
        }
    }
    ALLJOYN_POLICY_DEBUG(rule.ruleString += "/>");

    if (success) {
        policyGroup = (policyGroup == UNKNOWN) ? (SEND | RECEIVE) : policyGroup;
        if (policyGroup & OWN) {
            ownList.push_back(rule);
        }

        if (policyGroup & SEND) {
            sendList.push_back(rule);
        }

        if (policyGroup & RECEIVE) {
            receiveList.push_back(rule);
        }

        if (policyGroup & CONNECT) {
            connectList.push_back(rule);
        }
#ifndef NDEBUG
    } else {
        if (policyGroup != UNKNOWN) {
            Log(LOG_ERR, "Invalid combination of attributes in \"%s\".", rule.ruleString.c_str());
        }
#endif
    }

    return success;
}


bool _PolicyDB::AddRule(policydb::PolicyCategory cat,
                        qcc::String& catValue,
                        policydb::PolicyPermission permission,
                        const map<qcc::String, qcc::String>& ruleAttrs)
{
    bool success = false;

    switch (cat) {
    case policydb::POLICY_CONTEXT:
        if (catValue.compare("default") == 0) {
            success = AddRule(ownRS.defaultRules, sendRS.defaultRules,
                              receiveRS.defaultRules, connectRS.defaultRules,
                              permission, ruleAttrs);
        } else if (catValue.compare("mandatory") == 0) {
            success = AddRule(ownRS.mandatoryRules, sendRS.mandatoryRules,
                              receiveRS.mandatoryRules, connectRS.mandatoryRules,
                              permission, ruleAttrs);
        }
        break;

    case policydb::POLICY_USER: {
            uint32_t uid = GetUsersUid(catValue.c_str());
            success = AddRule(ownRS.userRules[uid], sendRS.userRules[uid],
                              receiveRS.userRules[uid], connectRS.userRules[uid],
                              permission, ruleAttrs);
            break;
        }

    case policydb::POLICY_GROUP: {
            uint32_t gid = GetUsersGid(catValue.c_str());
            success = AddRule(ownRS.groupRules[gid], sendRS.groupRules[gid],
                              receiveRS.groupRules[gid], connectRS.groupRules[gid],
                              permission, ruleAttrs);
            break;
        }

    case policydb::POLICY_AT_CONSOLE:
        if (catValue.compare("true") == 0) {
            success = AddRule(ownRS.atConsoleRules, sendRS.atConsoleRules,
                              receiveRS.atConsoleRules, connectRS.atConsoleRules,
                              permission, ruleAttrs);
        } else if (catValue.compare("false") == 0) {
            success = AddRule(ownRS.notAtConsoleRules, sendRS.notAtConsoleRules,
                              receiveRS.notAtConsoleRules, connectRS.notAtConsoleRules,
                              permission, ruleAttrs);
        }
        break;
    }
    return success;
}


void _PolicyDB::NameOwnerChanged(const qcc::String& alias,
                                 const qcc::String* oldOwner,
                                 const qcc::String* newOwner)
{
    StringIDMap::const_iterator bnit(busNameMap.find(alias));

    if (bnit != busNameMap.end()) {
        if (oldOwner) {
            UniqueNameIDMap::iterator unit = uniqueNameMap.find(*oldOwner);
            if (unit != uniqueNameMap.end()) {
                bnLock.Lock(MUTEX_CONTEXT);
                unit->second.erase(bnit->second);
                if (unit->second.empty()) {
                    uniqueNameMap.erase(unit);
                }
                bnLock.Unlock(MUTEX_CONTEXT);
            }
        }
        if (newOwner) {
            if (bnit != busNameMap.end()) {
                bnLock.Lock(MUTEX_CONTEXT);
                uniqueNameMap[*newOwner].insert(bnit->second);
                bnLock.Unlock(MUTEX_CONTEXT);
            }
        }
    }
}


bool _PolicyDB::CheckConnect(bool& allow, const PolicyRuleList& ruleList,
                             uint32_t uid, uint32_t gid) const
{
    PolicyRuleList::const_reverse_iterator it;
    bool ruleMatch(false);

    for (it = ruleList.rbegin(); !ruleMatch && (it != ruleList.rend()); ++it) {
        ruleMatch = (it->CheckUser(uid) && it->CheckGroup(gid));
    }
    if (ruleMatch) {
        allow = (it->permission == policydb::POLICY_ALLOW);
    }
    return ruleMatch;;
}

bool _PolicyDB::CheckOwn(bool& allow, const PolicyRuleList& ruleList,
                         uint32_t bnid) const
{
    PolicyRuleList::const_reverse_iterator it;
    bool ruleMatch(false);
    policydb::PolicyPermission permission;

    for (it = ruleList.rbegin(); !ruleMatch && (it != ruleList.rend()); ++it) {
        ruleMatch = it->CheckOwn(bnid);
        permission = it->permission;

        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "        checking rule: %s - %s - %s\n",
                                 it->permission == policydb::POLICY_ALLOW ? "ALLOW" : "DENY",
                                 it->ruleString.c_str(), ruleMatch ? "MATCH" : "no match"));
    }
    if (ruleMatch) {
        allow = permission;
    }
    return ruleMatch;;
}

bool _PolicyDB::CheckMessage(bool& allow, const PolicyRuleList& ruleList,
                             const NormalizedMsgHdr& nmh,
                             const BusNameIDSet& bnIDSet,
                             bool eavesdrop) const
{
    PolicyRuleList::const_reverse_iterator it;
    bool ruleMatch(false);
    policydb::PolicyPermission permission;

    for (it = ruleList.rbegin(); !ruleMatch && (it != ruleList.rend()); ++it) {
        ruleMatch = (it->CheckType(nmh.type) &&
                     it->CheckInterface(nmh.ifcID) &&
                     it->CheckMember(nmh.memberID) &&
                     it->CheckPath(nmh.pathID) &&
                     it->CheckError(nmh.errorID) &&
                     it->CheckEavesdrop(eavesdrop) &&
                     it->CheckBusName(bnIDSet));
        permission = it->permission;

        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "        checking rule: %s - %s - %s\n",
                                 it->permission == policydb::POLICY_ALLOW ? "ALLOW" : "DENY",
                                 it->ruleString.c_str(), ruleMatch ? "MATCH" : "no match"));
    }

    if (ruleMatch) {
        allow = (permission == policydb::POLICY_ALLOW);

        // TODO - Implement code to support matching the requested_reply
        // criteria.  This depends on maintaining a collection of outstanding
        // method calls for matching up replies.
    }

    return ruleMatch;
}


bool _PolicyDB::OKToConnect(uint32_t uid, uint32_t gid) const
{
    bool allow(false);
    bool ruleMatch(false);

    std::unordered_map<uint32_t, PolicyRuleList>::const_iterator it;

    ruleMatch = CheckConnect(allow, connectRS.mandatoryRules, uid, gid);

#if defined(CONSOLE_CHECK_SUPPORT)
    if (!ruleMatch) {
        if (atConsole) {
            ruleMatch = CheckConnect(allow, connectRS.atConsoleRules, uid, gid);
        } else {
            ruleMatch = CheckConnect(allow, connectRS.notAtConsoleRules, uid, gid);
        }
    }
#endif
    if (!ruleMatch) {
        it = connectRS.userRules.find(uid);
        if (it != connectRS.userRules.end()) {
            ruleMatch = CheckConnect(allow, it->second, uid, gid);
        }
    }

    if (!ruleMatch) {
        it = connectRS.groupRules.find(gid);
        if (it != connectRS.groupRules.end()) {
            ruleMatch = CheckConnect(allow, it->second, uid, gid);
        }
    }

    if (!ruleMatch) {
        ruleMatch = CheckConnect(allow, connectRS.defaultRules, uid, gid);
    }

    return allow;
}


bool _PolicyDB::OKToOwn(uint32_t busNameID,
                        uint32_t uid,
                        uint32_t gid) const
{
    bool allow(false);
    bool ruleMatch(false);
    std::unordered_map<uint32_t, PolicyRuleList>::const_iterator it;

    if (!ownRS.mandatoryRules.empty()) {
        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking mandatory rules\n"));
        ruleMatch = CheckOwn(allow, ownRS.mandatoryRules, busNameID);
    }

#if defined(CONSOLE_CHECK_SUPPORT)
    if (atConsole) {
        if (!ruleMatch && !ownRS.atConsoleRules.empty()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking atconsole=true rules\n"));
            ruleMatch = CheckOwn(allow, ownRS.atConsoleRules, busNameID);
        }
    } else {
        if (!ruleMatch && !ownRS.notAtConsoleRules.empty()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking atconsole=false rules\n"));
            ruleMatch = CheckOwn(allow, ownRS.notAtConsoleRules, busNameID);
        }
    }
#endif
    if (!ruleMatch && !ownRS.userRules.empty()) {
        it = ownRS.userRules.find(uid);
        if (it != ownRS.userRules.end()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking user=%u rules\n", uid));
            ruleMatch = CheckOwn(allow, it->second, busNameID);
        }
    }

    if (!ruleMatch && !ownRS.groupRules.empty()) {
        it = ownRS.groupRules.find(gid);
        if (it != ownRS.groupRules.end()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking group=%u rules\n", gid));
            ruleMatch = CheckOwn(allow, it->second, busNameID);
        }
    }

    if (!ruleMatch) {
        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking default rules\n"));
        ruleMatch = CheckOwn(allow, ownRS.defaultRules, busNameID);
    }

    return allow;
}


bool _PolicyDB::OKToReceive(const NormalizedMsgHdr& nmh,
                            uint32_t uid,
                            uint32_t gid) const
{
    bool allow(false);
    bool ruleMatch(false);
    std::unordered_map<uint32_t, PolicyRuleList>::const_iterator it;

    if (!receiveRS.mandatoryRules.empty()) {
        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking mandatory rules\n"));
        ruleMatch = CheckMessage(allow, receiveRS.mandatoryRules, nmh, nmh.senderIDList, false);
    }

#if defined(CONSOLE_CHECK_SUPPORT)
    if (atConsole) {
        if (!ruleMatch && !receiveRS.atConsoleRules.empty()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking atconsole=true rules\n"));
            ruleMatch = CheckMessage(allow, receiveRS.atConsoleRules, nmh, nmh.senderIDList, false);
        }
    } else {
        if (!ruleMatch && !receiveRS.notAtConsoleRules.empty()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking atconsole=false rules\n"));
            ruleMatch = CheckMessage(allow, receiveRS.notAtConsoleRules, nmh, nmh.senderIDList, false);
        }
    }
#endif

    if (!ruleMatch && !receiveRS.userRules.empty()) {
        it = receiveRS.userRules.find(uid);
        if (it != receiveRS.userRules.end()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking user=%u rules\n", uid));
            ruleMatch = CheckMessage(allow, it->second, nmh, nmh.senderIDList, false);
        }
    }

    if (!ruleMatch && !receiveRS.groupRules.empty()) {
        it = receiveRS.groupRules.find(gid);
        if (it != receiveRS.groupRules.end()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking group=%u rules\n", gid));
            ruleMatch = CheckMessage(allow, it->second, nmh, nmh.senderIDList, false);
        }
    }

    if (!ruleMatch) {
        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking default rules\n"));
        ruleMatch = CheckMessage(allow, receiveRS.defaultRules, nmh, nmh.senderIDList, false);
    }

    return allow;
}


bool _PolicyDB::OKToSend(const NormalizedMsgHdr& nmh,
                         uint32_t uid,
                         uint32_t gid) const
{
    bool allow(((nmh.type != ajn::MESSAGE_INVALID) &&
                (nmh.type != ajn::MESSAGE_METHOD_CALL)));
    bool ruleMatch(false);
    std::unordered_map<uint32_t, PolicyRuleList>::const_iterator it;

    if (!sendRS.mandatoryRules.empty()) {
        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking mandatory rules\n"));
        ruleMatch = CheckMessage(allow, sendRS.mandatoryRules, nmh, nmh.destIDList, false);
    }

#if defined(CONSOLE_CHECK_SUPPORT)
    if (atConsole) {
        if (!ruleMatch && !sendRS.atConsoleRules.empty()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking atconsole=true rules\n"));
            ruleMatch = CheckMessage(allow, sendRS.atConsoleRules, nmh, nmh.destIDList, false);
        }
    } else {
        if (!ruleMatch && !sendRS.notAtConsoleRules.empty()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking atconsole=false rules\n"));
            ruleMatch = CheckMessage(allow, sendRS.notAtConsoleRules, nmh, nmh.destIDList, false);
        }
    }
#endif

    if (!ruleMatch && !sendRS.userRules.empty()) {
        it = sendRS.userRules.find(uid);
        if (it != sendRS.userRules.end()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking user=%u rules\n", uid));
            ruleMatch = CheckMessage(allow, it->second, nmh, nmh.destIDList, false);
        }
    }

    if (!ruleMatch && !sendRS.groupRules.empty()) {
        it = sendRS.groupRules.find(gid);
        if (it != sendRS.groupRules.end()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking group=%u rules\n", gid));
            ruleMatch = CheckMessage(allow, it->second, nmh, nmh.destIDList, false);
        }
    }

    if (!ruleMatch) {
        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking default rules\n"));
        ruleMatch = CheckMessage(allow, sendRS.defaultRules, nmh, nmh.destIDList, false);
    }

    return allow;
}


bool _PolicyDB::OKToEavesdrop(const NormalizedMsgHdr& nmh,
                              uint32_t suid,
                              uint32_t sgid,
                              uint32_t duid,
                              uint32_t dgid) const
{
    bool allow(false);
    bool ruleMatch(false);
    std::unordered_map<uint32_t, PolicyRuleList>::const_iterator it;

    if (!sendRS.mandatoryRules.empty()) {
        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking mandatory eavesdrop send rules\n"));
        ruleMatch = CheckMessage(allow, sendRS.mandatoryRules, nmh, nmh.destIDList, true);
    }
    if (!receiveRS.mandatoryRules.empty()) {
        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking mandatory eavesdrop receive rules\n"));
        ruleMatch = CheckMessage(allow, receiveRS.mandatoryRules, nmh, nmh.senderIDList, true);
    }

#if defined(CONSOLE_CHECK_SUPPORT)
    if (atConsole) {
        if (!ruleMatch && !sendRS.atConsoleRules.empty()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking atconsole=true eavesdrop send rules\n"));
            ruleMatch = CheckMessage(allow, sendRS.atConsoleRules, nmh, nmh.destIDList, true);
        }
        if (!ruleMatch && !receiveRS.atConsoleRules.empty()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking atconsole=true eavesdrop receive rules\n"));
            ruleMatch = CheckMessage(allow, receiveRS.atConsoleRules, nmh, nmh.senderIDList, true);
        }
    } else {
        if (!ruleMatch && !sendRS.notAtConsoleRules.empty()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking atconsole=false eavesdrop send rules\n"));
            ruleMatch = CheckMessage(allow, sendRS.notAtConsoleRules, nmh, nmh.destIDList, true);
        }
        if (!ruleMatch && !receiveRS.notAtConsoleRules.empty()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking atconsole=false eavesdrop receive rules\n"));
            ruleMatch = CheckMessage(allow, receiveRS.notAtConsoleRules, nmh, nmh.senderIDList, true);
        }
    }
#endif

    if (!ruleMatch && !sendRS.userRules.empty()) {
        it = sendRS.userRules.find(suid);
        if (it != sendRS.userRules.end()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking user=%u eavesdrop send rules\n", suid));
            ruleMatch = CheckMessage(allow, it->second, nmh, nmh.destIDList, true);
        }
    }
    if (!ruleMatch && !receiveRS.userRules.empty()) {
        it = receiveRS.userRules.find(duid);
        if (it != receiveRS.userRules.end()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking user=%u eavesdrop receive rules\n", duid));
            ruleMatch = CheckMessage(allow, it->second, nmh, nmh.senderIDList, true);
        }
    }

    if (!ruleMatch && !sendRS.groupRules.empty()) {
        it = sendRS.groupRules.find(sgid);
        if (it != sendRS.groupRules.end()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking group=%u eavesdrop send rules\n", sgid));
            ruleMatch = CheckMessage(allow, it->second, nmh, nmh.destIDList, true);
        }
    }
    if (!ruleMatch && !receiveRS.groupRules.empty()) {
        it = receiveRS.groupRules.find(dgid);
        if (it != receiveRS.groupRules.end()) {
            ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking group=%u eavesdrop receive rules\n", dgid));
            ruleMatch = CheckMessage(allow, it->second, nmh, nmh.senderIDList, true);
        }
    }

    if (!ruleMatch) {
        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking default eavesdrop send rules\n"));
        ruleMatch = CheckMessage(allow, sendRS.defaultRules, nmh, nmh.destIDList, true);
    }
    if (!ruleMatch) {
        ALLJOYN_POLICY_DEBUG(Log(LOG_DEBUG, "    checking default eavesdrop receive rules\n"));
        ruleMatch = CheckMessage(allow, receiveRS.defaultRules, nmh, nmh.senderIDList, true);
    }

    return allow;
}
