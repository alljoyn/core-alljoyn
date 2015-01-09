/**
 * @file
 * AllJoyn-Daemon Policy database class
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014, AllSeen Alliance. All rights reserved.
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

#ifndef NDEBUG
#include <qcc/StringUtil.h>
#endif

#include "PolicyDB.h"

#define QCC_MODULE "POLICYDB"

using namespace ajn;
using namespace qcc;
using namespace std;

/*
 * The whole design of the PolicyDB is based around the idea that integers are
 * more efficient to compare than strings.  Just about everything related to
 * applying policy rules involves comparing strings.  With a lot of rules,
 * this can get to be quite expensive computationally.
 *
 * To make these comparisons more efficient, a dictionary of all the strings
 * found in all the rules is created where each string is assigned a unique ID
 * number.  Strings that appear more than once in the rules will use the same
 * ID since they are the same.
 *
 * Now, when messages are to be routed to endpoints, the strings in the header
 * fields are converted to their unique IDs using the dictionary that was
 * setup while parsing the policy rule table.  Not all strings seen in message
 * headers will appear in the dictionary.  In such a case, a special value
 * will be used that indicates the string is not in the dictionary.
 *
 * A small complicating factor is that an endpoint may have more than one bus
 * name.  In such a case, an endpoint would have its unique name plus one or
 * more aliases (aka well-known-names).  It is very unlikely for a rule to
 * specify a unique name, but highly likely to specifiy an alias.  Such rules
 * apply to the endpoint that sent/received the message and not only the name
 * of the endpoint that appears in the message.  Since the bus name that
 * appears in the message could be either the unique name or any of that
 * endpoint's aliases, all of those names need to be treated as equals.  To
 * accomplish this the PolicyDB code maintains its own name table.  This name
 * table maps all names to the set of all their aliases.  The set of aliases
 * is kept as a table of string IDs for efficiency purposes.
 */


/* Policy Groups */
#define RULE_UNKNOWN    (0x0)
#define RULE_OWN        (0x1 << 0)
#define RULE_SEND       (0x1 << 1)
#define RULE_RECEIVE    (0x1 << 2)
#define RULE_CONNECT    (0x1 << 3)


#ifndef NDEBUG
static String IDSet2String(const _PolicyDB::IDSet& idset)
{
    String ids;
    std::unordered_set<StringID>::const_iterator it = idset->begin();
    while (it != idset->end()) {
        ids += I32ToString(*it);
        ++it;
        if (it != idset->end()) {
            ids += ", ";
        }
    }
    return ids;
}
#endif

static bool MsgTypeStrToEnum(const String& str, AllJoynMessageType& type)
{
    bool success = true;
    if (str == "*") {
        type = MESSAGE_INVALID;
    } else if (str == "method_call") {
        type = MESSAGE_METHOD_CALL;
    } else if (str == "method_return") {
        type = MESSAGE_METHOD_RET;
    } else if (str == "signal") {
        type = MESSAGE_SIGNAL;
    } else if (str == "error") {
        type = MESSAGE_ERROR;
    } else {
        Log(LOG_ERR, "Invalid type for policy rule: \"%s\"\n", str.c_str());
        success = false;
    }
    return success;
}

StringID _PolicyDB::UpdateDictionary(const String& key)
{
    StringID id;

    if (key.empty()) {
        /* A rule that specifies an empty string will never match anything. */
        id = NIL_MATCH;
    } else {
        lock.WRLock();
        StringIDMap::const_iterator it = dictionary.find(key);

        if (it == dictionary.end()) {
            /* New string found; assign it an ID. */
            id = dictionary.size();
            pair<StringMapKey, StringID> p(key, id);
            dictionary.insert(p);
        } else {
            /* The string already has an ID. */
            id = it->second;
        }
        lock.Unlock();
    }
    return id;
}


StringID _PolicyDB::LookupStringID(const char* key) const
{
    StringID id = ID_NOT_FOUND;

    if (key) {
        lock.RDLock();
        StringIDMap::const_iterator it = dictionary.find(key);

        if (it != dictionary.end()) {
            id = it->second;
        }
        lock.Unlock();
    }
    return id;
}


const _PolicyDB::IDSet _PolicyDB::LookupStringIDPrefix(const char* idStr, char sep) const
{
    assert(idStr);
    assert(sep != '\0');
    IDSet ret;
    char* prefix = strdup(idStr); // duplicate idStr since we are modifying it

    /*
     * If prefix is NULL, then the program is out of memory.  While it may be
     * considered reasonable to rework the design of this function to return
     * an error, in this case an out-of-memory condition, we do not for the
     * following reasons.  One, it should never happen in real-life, and two,
     * it is more likely just a small symptom of a much bigger problem that
     * will quickly manifest itself in other ways.  To keep this function
     * simple and its usage simple, an empty IDSet will be returned in such an
     * unlikely condition.
     */
    if (prefix) {
        while (*prefix) {
            StringID id = LookupStringID(prefix);
            if ((id != ID_NOT_FOUND) && (id != WILDCARD)) {
                /*
                 * We only add ID's that are found in the string ID table.  By
                 * not keeping prefixes that are known to not be specifed by
                 * the policy rules, we keep the size of the possible matches
                 * to a minimum and save time by not adding useless
                 * information to an unordered_set<>.
                 */
                ret->insert(id);
            }
            char* p = strrchr(prefix, sep);
            if (p) {
                *p = '\0';      // Shorten the string to the next separator.
            } else {
                *prefix = '\0'; // All done.
            }
        }

        free(prefix);
    }

    return ret;
}


const _PolicyDB::IDSet _PolicyDB::LookupBusNameID(const char* busName) const
{
    IDSet ret;

    if (busName && (busName[0] != '\0')) {
        lock.RDLock();
        BusNameIDMap::const_iterator it = busNameIDMap.find(busName);
        if (it != busNameIDMap.end()) {
            /*
             * We need to make a local copy of the set of aliases for the
             * given bus name so that we can safely release the lock and allow
             * NameOwnerChanged() to possibly modify the set without causing
             * interference.  While it is very unlikely that
             * NameOwnerChanged() would be called while another message is
             * begin processed, this measure follows the priciple of better
             * safe than sorry.
             */
            ret = IDSet(it->second, true);  // deep copy
        }
        lock.Unlock();
    }
    return ret;
}


_PolicyDB::_PolicyDB()
{
    // Prefill the string ID table with the wildcard character - used when applying rules.
    dictionary[""] = WILDCARD;
    dictionary["*"] = WILDCARD;
}


bool _PolicyDB::AddRule(PolicyRuleList& ownList,
                        PolicyRuleList& connectList,
                        PolicyRuleList& sendList,
                        PolicyRuleList& receiveList,
                        policydb::PolicyPermission permission,
                        const map<String, String>& ruleAttrs)
{
    std::map<String, String>::const_iterator attr;
    bool success = true;
    bool skip = false; // Get's set to true if any component of a rule is guaranteed to not match.
    PolicyRule rule(permission);

    uint32_t prevPolicyGroup = RULE_UNKNOWN;
    uint32_t policyGroup = RULE_UNKNOWN;

    QCC_DEBUG_ONLY(rule.ruleString = (permission == policydb::POLICY_ALLOW) ? "<allow" : "<deny");

    for (attr = ruleAttrs.begin(); attr != ruleAttrs.end(); ++attr) {
        String attrStr = attr->first;
        const String& attrVal = attr->second;

        QCC_DEBUG_ONLY(rule.ruleString += " " + attrStr + "=\"" + attrVal);

        if (attrStr.compare(0, sizeof("send_") - 1, "send_") == 0) {
            policyGroup = RULE_SEND;
        } else if (attrStr.compare(0, sizeof("receive_") - 1, "receive_") == 0) {
            policyGroup = RULE_RECEIVE;
        }

        if (policyGroup & (RULE_SEND | RULE_RECEIVE)) {
            // Trim off "send_"/"receive_" for a shorter/simpler comparisons below.
            attrStr = attrStr.substr(attrStr.find_first_of('_') + 1);

            if (attrStr == "type") {
                success = MsgTypeStrToEnum(attrVal, rule.type);
            } else {
                StringID strID = UpdateDictionary(attrVal);
                skip |= (strID == NIL_MATCH);

                QCC_DEBUG_ONLY(rule.ruleString += "{" + I32ToString(strID) + "}");

                if (attrStr == "interface") {
                    rule.interface = strID;
                } else if (attrStr == "member") {
                    rule.member = strID;
                } else if (attrStr == "error") {
                    rule.error = strID;
                } else if (attrStr == "path") {
                    rule.path = strID;
                } else if (attrStr == "path_prefix") {
                    rule.pathPrefix = strID;
                } else if (attr->first == "send_destination") {
                    rule.busName = strID;
                } else if (attr->first == "receive_sender") {
                    rule.busName = strID;
                } else if (attrStr == "group") {
                    if (attrVal == "*") {
                        rule.groupAny = true;
                    } else {
                        rule.group = GetUsersGid(attrVal.c_str());
                        skip |= (rule.group == static_cast<uint32_t>(-1));
                    }
                    rule.groupSet = true;
                } else if (attrStr == "user") {
                    if (attrVal == "*") {
                        rule.userAny = true;
                    } else {
                        rule.user = GetUsersUid(attrVal.c_str());
                        skip |= (rule.user == static_cast<uint32_t>(-1));
                    }
                    rule.userSet = true;
                } else {
                    Log(LOG_ERR, "Unknown policy attribute: \"%s\"\n", attr->first.c_str());
                    success = false;
                }
            }
        } else {
            if (attrStr == "own") {
                policyGroup = RULE_OWN;
                rule.own = UpdateDictionary(attrVal);
                skip |= (rule.own == NIL_MATCH);
            } else if (attrStr == "own_prefix") {
                policyGroup = RULE_OWN;
                rule.ownPrefix = UpdateDictionary(attrVal);
                skip |= (rule.ownPrefix == NIL_MATCH);
            } else if (attrStr == "user") {
                policyGroup = RULE_CONNECT;
                if (attrVal == "*") {
                    rule.userAny = true;
                } else {
                    rule.user = GetUsersUid(attrVal.c_str());
                    skip |= (rule.user == static_cast<uint32_t>(-1));
                }
                rule.userSet = true;
            } else if (attrStr == "group") {
                policyGroup = RULE_CONNECT;
                if (attrVal == "*") {
                    rule.groupAny = true;
                } else {
                    rule.group = GetUsersGid(attrVal.c_str());
                    skip |= (rule.group == static_cast<uint32_t>(-1));
                }
                rule.groupSet = true;
            } else {
                Log(LOG_ERR, "Unknown policy attribute: \"%s\"\n", attr->first.c_str());
                success = false;
            }
        }

        QCC_DEBUG_ONLY(rule.ruleString += "\"");

        if ((prevPolicyGroup != RULE_UNKNOWN) && (policyGroup != prevPolicyGroup)) {
            // Invalid rule spec mixed different policy group attributes.
            success = false;
        }
        prevPolicyGroup = policyGroup;

        if (!success) {
            break;
        }
    }

    QCC_DEBUG_ONLY(rule.ruleString += "/>");

    if (success && !skip) {
        assert(policyGroup != RULE_UNKNOWN);

        if (policyGroup & RULE_SEND) {
            sendList.push_back(rule);
        }

        if (policyGroup & RULE_RECEIVE) {
            receiveList.push_back(rule);
        }

        if (policyGroup & RULE_OWN) {
            ownList.push_back(rule);
        }

        if (policyGroup & RULE_CONNECT) {
            connectList.push_back(rule);
        }
    }

#ifndef NDEBUG
    if (!success && (policyGroup != RULE_UNKNOWN)) {
        Log(LOG_ERR, "Invalid attribute \"%s\" in \"%s\".\n", attr->first.c_str(), rule.ruleString.c_str());
    }
#endif

    return success;
}


bool _PolicyDB::AddRule(const String& cat, const String& catValue, const String& permStr,
                        const map<String, String>& ruleAttrs)
{
    bool success = false;
    policydb::PolicyPermission permission;

    if (permStr == "allow") {
        permission = policydb::POLICY_ALLOW;
    } else if (permStr == "deny") {
        permission = policydb::POLICY_DENY;
    } else {
        // Invalid policy.
        return false;
    }

    if (cat == "context") {
        if (catValue == "default") {
            // <policy context="default">
            success = AddRule(ownRS.defaultRules, connectRS.defaultRules,
                              sendRS.defaultRules, receiveRS.defaultRules,
                              permission, ruleAttrs);
        } else if (catValue == "mandatory") {
            // <policy context="mandatory">
            success = AddRule(ownRS.mandatoryRules, connectRS.mandatoryRules,
                              sendRS.mandatoryRules, receiveRS.mandatoryRules,
                              permission, ruleAttrs);
        }

    } else if (cat == "user") {
        // <policy user="userid">
        uint32_t uid = GetUsersUid(catValue.c_str());
        if (uid != static_cast<uint32_t>(-1)) {
            success = AddRule(ownRS.userRules[uid], connectRS.userRules[uid],
                              sendRS.userRules[uid], receiveRS.userRules[uid],
                              permission, ruleAttrs);
        } else {
            Log(LOG_WARNING, "Ignoring policy rules for invalid user: %s", catValue.c_str());
            success = true;
        }

    } else if (cat == "group") {
        // <policy group="groupid">
        uint32_t gid = GetUsersGid(catValue.c_str());
        if (gid != static_cast<uint32_t>(-1)) {
            success = AddRule(ownRS.groupRules[gid], connectRS.groupRules[gid],
                              sendRS.groupRules[gid], receiveRS.groupRules[gid],
                              permission, ruleAttrs);
        } else {
            Log(LOG_WARNING, "Ignoring policy rules for invalid group: %s", catValue.c_str());
            success = true;
        }
    }
    return success;
}


void _PolicyDB::AddAlias(const String& alias, const String& name)
{
    // We assume the write lock is already held.

    StringID nameID = ID_NOT_FOUND;
    /*
     * Can't call LookupStringID() since it would try to get a read lock while
     * we already have a write lock.  That would cause a nice assert fail.
     * Since the write lock trumps read locks, we'll just lookup the string ID
     * directly.
     */
    StringIDMap::const_iterator sit = dictionary.find(alias);

    if (sit != dictionary.end()) {
        nameID = sit->second;
    }

    IDSet bnids;
    BusNameIDMap::iterator it = busNameIDMap.find(name);
    if (it != busNameIDMap.end()) {
        bnids = it->second;
    }
    if (nameID != ID_NOT_FOUND) {
        QCC_DbgPrintf(("Add %s{%d} to table for %s", alias.c_str(), nameID, name.c_str()));
        bnids->insert(nameID);
    }
    pair<StringMapKey, IDSet> p(alias, bnids);
    busNameIDMap.insert(p);
}


void _PolicyDB::Finalize(Bus* bus)
{
    if (bus) {
        /*
         * If the config was reloaded while the bus is operating, then the
         * internal map of bus names and aliases has been wiped out.  We need
         * to regenerate that map from the information in the NameTable.
         * Since the NameTable only provides vectors of Strings, the only
         * thing we can do is iterate over those vectors and convert them to
         * StringIDs.
         */
        vector<String> nameList;
        vector<String>::const_iterator nlit;
        vector<pair<String, vector<String> > > aliasMap;
        vector<pair<String, vector<String> > >::const_iterator amit;
        DaemonRouter& router(reinterpret_cast<DaemonRouter&>(bus->GetInternal().GetRouter()));

        /*
         * Need to hold the name table lock for the entire duration of
         * processing the bus names even though we get a separate copy of
         * those names.  This is to prevent a race condition where a
         * NameOwnerChanged event happens while we are processing the bus
         * names from the name table.  We can't acquire the write lock before
         * the name table lock due to a dead lock risk.  This forces use to
         * hold onto the name table lock for the entire duration of processing
         * bus names.
         */
        router.LockNameTable();
        lock.WRLock();

        router.GetBusNames(nameList);
        router.GetUniqueNamesAndAliases(aliasMap);

        for (nlit = nameList.begin(); nlit != nameList.end(); ++nlit) {
            const String& name = *nlit;
            if (name[0] == ':') {
                // Only handle unique names right now, aliases will be handled below.
                AddAlias(name, name);
            }
        }

        for (amit = aliasMap.begin(); amit != aliasMap.end(); ++amit) {
            const String& unique = amit->first;
            const vector<String>& aliases = amit->second;
            vector<String>::const_iterator ait;

            for (ait = aliases.begin(); ait != aliases.end(); ++ait) {
                AddAlias(*ait, unique);
            }
        }
        lock.Unlock();
        router.UnlockNameTable();
    }

#ifndef NDEBUG
    QCC_DbgPrintf(("Dictionary:"));
    for (StringIDMap::const_iterator it = dictionary.begin(); it != dictionary.end(); ++it) {
        QCC_DbgPrintf(("    \"%s\" = %u", it->first.c_str(), it->second));
    }
    QCC_DbgPrintf(("Name Table:"));
    for (unordered_map<qcc::StringMapKey, IDSet>::const_iterator it = busNameIDMap.begin(); it != busNameIDMap.end(); ++it) {
        QCC_DbgPrintf(("    \"%s\" = {%s}", it->first.c_str(), IDSet2String(it->second).c_str()));
    }
#endif
}


void _PolicyDB::NameOwnerChanged(const String& alias,
                                 const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                                 const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer)
{
    /* When newOwner and oldOwner are the same, only the name transfer changed. */
    if (newOwner == oldOwner) {
        return;
    }

    /*
     * Bus name matching rules must treat all aliases (well known names) they
     * resolve to as thesame, otherwise it would be relatively trivial to
     * bypass a <deny/> rule specified with one alias by sending to either the
     * unique name or a different alias owned by the same owner with a
     * matching <allow/> rule.  Thus, we must keep track of who owns what
     * aliases.
     *
     * Because messages coming though will include either well known names or
     * unique names in the source or destination fields, we need to map each
     * unique name and alias to the set of equivalent aliases (plus unique
     * name).
     *
     * Here we take advantage of the fact that IDSet is a MangedObj and that
     * there is just one underlying instance.  When a new node joins the bus
     * (alias == *newOwner), a new IDSet is created with the unique name of
     * the new node and that unique name is mapped to that new IDSet.  When a
     * node gains ownership of an alias, the IDSet for the owner is updated
     * with the new alias and a mapping is created from the alias to that
     * IDSet.  When a node loses ownership of an alias, that alias is removed
     * from the owner's IDSet and the mapping from that alias to the IDSet is
     * removed.  When a node leaves (alias == *oldOwner), the unique name is
     * removed from the assiciated IDSet and the mapping from the unique name
     * to the IDSet is removed.  Due to the nature of ManagedObj and that all
     * names are map keys to shared IDSets, it does not matter in what order
     * aliases/nodes are added or removed.
     */

    StringID aliasID = LookupStringID(alias.c_str());

    lock.WRLock();

    if (oldOwner) {
        BusNameIDMap::iterator it = busNameIDMap.find(alias);
        if (it != busNameIDMap.end()) {
            QCC_DbgPrintf(("Remove %s{%d} from table for %s", alias.c_str(), aliasID, oldOwner->c_str()));
            it->second->erase(aliasID);
            busNameIDMap.erase(it);
        } else {
            QCC_LogError(ER_FAIL, ("Alias '%s' not in busNameIDMap", alias.c_str()));
        }
    }

    if (newOwner) {
        IDSet bnids;
        BusNameIDMap::iterator it = busNameIDMap.find(*newOwner);
        if (it != busNameIDMap.end()) {
            assert(alias != *newOwner);
            bnids = it->second;
        }
        if (aliasID != ID_NOT_FOUND) {
            QCC_DbgPrintf(("Add %s{%d} to table for %s", alias.c_str(), aliasID, newOwner->c_str()));
            bnids->insert(aliasID);
        }
        pair<StringMapKey, IDSet> p(alias, bnids);
        busNameIDMap.insert(p);
    }

    lock.Unlock();
}


/**
 * Rule check macros.  The rule check functions all operate in the same way
 * with the only difference being the arguments they take and what they check.
 * This macro is the simplest way to put all the boilerplate code into one
 * place.  This is a case where macros work better than C++ templates and
 * varargs while avoiding a cumbersome callback mechanism.  This is truely a
 * macro and not a macro that has function-like semantics.
 *
 * @param[out] _allow   Whether the rule is an ALLOW or DENY rule
 * @param _rl           The ruleList to iterate over
 * @param _it           The name of the iterator variable to use (e.g. "it")
 * @param _checks       The checks to perform
 *                      (e.g. "(it->CheckUser(uid) && it->CheckGroup(gid))")
 */
#ifndef NDEBUG
#define RULE_CHECKS(_allow, _rl, _it, _checks)                          \
    PolicyRuleList::const_reverse_iterator _it;                         \
    bool ruleMatch = false;                                             \
    policydb::PolicyPermission permission;                              \
    size_t rc = 0;                                                      \
    for (_it = _rl.rbegin(); !ruleMatch && (_it != _rl.rend()); ++_it) { \
        ruleMatch = _checks;                                            \
        permission = _it->permission;                                   \
        QCC_DbgPrintf(("        checking rule (%u/%u): %s - %s",        \
                       _rl.size() - rc++,                               \
                       _rl.size(),                                      \
                       _it->ruleString.c_str(),                         \
                       ruleMatch ? "MATCH" : "no match"));              \
    }                                                                   \
    if (ruleMatch) {                                                    \
        _allow = (permission == policydb::POLICY_ALLOW);                \
    }                                                                   \
    return ruleMatch
#else
#define RULE_CHECKS(_allow, _rl, _it, _checks)                          \
    PolicyRuleList::const_reverse_iterator _it;                         \
    bool ruleMatch = false;                                             \
    policydb::PolicyPermission permission;                              \
    for (_it = _rl.rbegin(); !ruleMatch && (_it != _rl.rend()); ++_it) { \
        ruleMatch = _checks;                                            \
        permission = _it->permission;                                   \
    }                                                                   \
    if (ruleMatch) {                                                    \
        _allow = (permission == policydb::POLICY_ALLOW);                \
    }                                                                   \
    return ruleMatch
#endif

bool _PolicyDB::CheckConnect(bool& allow, const PolicyRuleList& ruleList, uint32_t uid, uint32_t gid)
{
    RULE_CHECKS(allow, ruleList, it,
                (it->CheckUser(uid) && it->CheckGroup(gid)));
}


bool _PolicyDB::CheckOwn(bool& allow, const PolicyRuleList& ruleList, StringID bnid, const IDSet& prefixes)
{
    RULE_CHECKS(allow, ruleList, it,
                it->CheckOwn(bnid, prefixes));
}


bool _PolicyDB::CheckMessage(bool& allow, const PolicyRuleList& ruleList,
                             const NormalizedMsgHdr& nmh,
                             const IDSet& bnIDSet,
                             uint32_t userId,
                             uint32_t groupId)
{
    RULE_CHECKS(allow, ruleList, it,
                (it->CheckType(nmh.type) &&
                 it->CheckInterface(nmh.ifcID) &&
                 it->CheckMember(nmh.memberID) &&
                 it->CheckPath(nmh.pathID, nmh.pathIDSet) &&
                 it->CheckError(nmh.errorID) &&
                 it->CheckBusName(bnIDSet) &&
                 it->CheckUser(userId) &&
                 it->CheckGroup(groupId)));
}



bool _PolicyDB::OKToConnect(uint32_t uid, uint32_t gid) const
{
    /* Implicitly default to allow any endpoint to connect. */
    bool allow = true;
    bool ruleMatch = false;

    QCC_DbgPrintf(("Check if OK for endpoint with UserID %u and GroupID %u to connect", uid, gid));

    if (!connectRS.mandatoryRules.empty()) {
        QCC_DbgPrintf(("    checking mandatory connect rules"));
        ruleMatch = CheckConnect(allow, connectRS.mandatoryRules, uid, gid);
    }

    if (!ruleMatch) {
        IDRuleMap::const_iterator it = connectRS.userRules.find(uid);
        if (it != connectRS.userRules.end()) {
            QCC_DbgPrintf(("    checking user=%u connect rules", uid));
            ruleMatch = CheckConnect(allow, it->second, uid, gid);
        }
    }

    if (!ruleMatch) {
        IDRuleMap::const_iterator it = connectRS.groupRules.find(gid);
        if (it != connectRS.groupRules.end()) {
            QCC_DbgPrintf(("    checking group=%u connect rules", gid));
            ruleMatch = CheckConnect(allow, it->second, uid, gid);
        }
    }

    if (!ruleMatch) {
        QCC_DbgPrintf(("    checking default connect rules"));
        ruleMatch = CheckConnect(allow, connectRS.defaultRules, uid, gid);
    }

    return allow;
}


bool _PolicyDB::OKToOwn(const char* busName, BusEndpoint& ep) const
{
    if (!busName || busName[0] == '\0' || busName[0] == ':') {
        /* Can't claim ownership of a unique name, empty name, or a NULL pointer */
        return false;
    }

    QCC_DbgPrintf(("Check if OK for endpoint %s to own %s{%d}",
                   ep->GetUniqueName().c_str(), busName, LookupStringID(busName)));

    /* Implicitly default to allow any endpoint to own any name. */
    bool allow = true;
    bool ruleMatch = false;
    StringID busNameID = LookupStringID(busName);
    IDSet prefixes = LookupStringIDPrefix(busName, '.');

    if (!ownRS.mandatoryRules.empty()) {
        QCC_DbgPrintf(("    checking mandatory own rules"));
        ruleMatch = CheckOwn(allow, ownRS.mandatoryRules, busNameID, prefixes);
    }

    if (!ruleMatch && !ownRS.userRules.empty()) {
        uint32_t uid = ep->GetUserId();
        IDRuleMap::const_iterator it = ownRS.userRules.find(uid);
        if (it != ownRS.userRules.end()) {
            QCC_DbgPrintf(("    checking user=%u own rules", uid));
            ruleMatch = CheckOwn(allow, it->second, busNameID, prefixes);
        }
    }

    if (!ruleMatch && !ownRS.groupRules.empty()) {
        uint32_t gid = ep->GetGroupId();
        IDRuleMap::const_iterator it = ownRS.groupRules.find(gid);
        if (it != ownRS.groupRules.end()) {
            QCC_DbgPrintf(("    checking group=%u own rules", gid));
            ruleMatch = CheckOwn(allow, it->second, busNameID, prefixes);
        }
    }

    if (!ruleMatch) {
        QCC_DbgPrintf(("    checking default own rules"));
        ruleMatch = CheckOwn(allow, ownRS.defaultRules, busNameID, prefixes);
    }

    return allow;
}


bool _PolicyDB::OKToReceive(const NormalizedMsgHdr& nmh, BusEndpoint& dest) const
{
    /* Implicitly default to allow all messages to be received. */
    bool allow = true;
    bool ruleMatch = false;

    if (nmh.destIDSet->empty()) {
        /*
         * Broadcast/multicast signal - need to re-check send rules for each
         * destination.
         */
        const IDSet destIDSet = LookupBusNameID(dest->GetUniqueName().c_str());
        allow = OKToSend(nmh, dest, &destIDSet);
        if (!allow) {
            return false;
        }
    }

    QCC_DbgPrintf(("Check if OK for endpoint %s to receive %s (%s{%s} --> %s{%s})",
                   dest->GetUniqueName().c_str(), nmh.msg->Description().c_str(),
                   nmh.msg->GetSender(), IDSet2String(nmh.senderIDSet).c_str(),
                   nmh.msg->GetDestination(), IDSet2String(nmh.destIDSet).c_str()));

    uint32_t senderUid = nmh.sender->GetUserId();
    uint32_t senderGid = nmh.sender->GetGroupId();
    if (!receiveRS.mandatoryRules.empty()) {
        QCC_DbgPrintf(("    checking mandatory receive rules"));
        ruleMatch = CheckMessage(allow, receiveRS.mandatoryRules, nmh, nmh.senderIDSet, senderUid, senderGid);
    }

    uint32_t uid = dest->GetUserId();
    if (!ruleMatch && !receiveRS.userRules.empty()) {
        IDRuleMap::const_iterator it = receiveRS.userRules.find(uid);
        if (it != receiveRS.userRules.end()) {
            QCC_DbgPrintf(("    checking user=%u receive rules", uid));
            ruleMatch = CheckMessage(allow, it->second, nmh, nmh.senderIDSet, senderUid, senderGid);
        }
    }

    uint32_t gid = dest->GetGroupId();
    if (!ruleMatch && !receiveRS.groupRules.empty()) {
        IDRuleMap::const_iterator it = receiveRS.groupRules.find(gid);
        if (it != receiveRS.groupRules.end()) {
            QCC_DbgPrintf(("    checking group=%u receive rules", gid));
            ruleMatch = CheckMessage(allow, it->second, nmh, nmh.senderIDSet, senderUid, senderGid);
        }
    }

    if (!ruleMatch) {
        QCC_DbgPrintf(("    checking default receive rules"));
        ruleMatch = CheckMessage(allow, receiveRS.defaultRules, nmh, nmh.senderIDSet, senderUid, senderGid);
    }

    return allow;
}


bool _PolicyDB::OKToSend(const NormalizedMsgHdr& nmh, BusEndpoint& dest, const IDSet* destIDSet) const
{
    /* Implicitly default to allow messages to be sent. */
    bool allow = true;
    bool ruleMatch = false;

    if (!destIDSet) {
        destIDSet = &nmh.destIDSet;
    }

    QCC_DbgPrintf(("Check if OK for endpoint %s to send %s to destination %s (%s{%s} --> %s{%s})",
                   nmh.sender->GetUniqueName().c_str(), nmh.msg->Description().c_str(),
                   (dest->IsValid() ? dest->GetUniqueName().c_str() : ""),
                   nmh.msg->GetSender(), IDSet2String(nmh.senderIDSet).c_str(),
                   nmh.msg->GetDestination(), IDSet2String(*destIDSet).c_str()));


    uint32_t destUid = -1;
    uint32_t destGid = -1;
    if (dest->IsValid()) {
        destUid = dest->GetUserId();
        destGid = dest->GetGroupId();
    }

    if (!sendRS.mandatoryRules.empty()) {
        QCC_DbgPrintf(("    checking mandatory send rules"));
        ruleMatch = CheckMessage(allow, sendRS.mandatoryRules, nmh, *destIDSet, destUid, destGid);
    }

    if (!ruleMatch && !sendRS.userRules.empty()) {
        uint32_t uid = nmh.sender->GetUserId();
        IDRuleMap::const_iterator it = sendRS.userRules.find(uid);
        if (it != sendRS.userRules.end()) {
            QCC_DbgPrintf(("    checking user=%u send rules", uid));
            ruleMatch = CheckMessage(allow, it->second, nmh, *destIDSet, destUid, destGid);
        }
    }

    if (!ruleMatch && !sendRS.groupRules.empty()) {
        uint32_t gid = nmh.sender->GetGroupId();
        IDRuleMap::const_iterator it = sendRS.groupRules.find(gid);
        if (it != sendRS.groupRules.end()) {
            QCC_DbgPrintf(("    checking group=%u send rules", gid));
            ruleMatch = CheckMessage(allow, it->second, nmh, *destIDSet, destUid, destGid);
        }
    }

    if (!ruleMatch) {
        QCC_DbgPrintf(("    checking default send rules"));
        ruleMatch = CheckMessage(allow, sendRS.defaultRules, nmh, *destIDSet, destUid, destGid);
    }

    return allow;
}
