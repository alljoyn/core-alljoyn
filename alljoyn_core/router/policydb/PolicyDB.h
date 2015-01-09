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
#ifndef _POLICYDB_H
#define _POLICYDB_H

#include <qcc/platform.h>
#include <qcc/Logger.h>
#include <qcc/ManagedObj.h>
#include <qcc/RWLock.h>
#include <qcc/String.h>
#include <qcc/StringMapKey.h>
#include <qcc/STLContainer.h>

#include <alljoyn/Message.h>

#include "Bus.h"
#include "BusEndpoint.h"
#include "NameTable.h"


namespace ajn {

namespace policydb {
/** Enumeration of different types of policy groups */
enum PolicyCategory {
    POLICY_CONTEXT,     /**< Policy group for either default or mandatory policies */
    POLICY_USER,        /**< Policy group that applies to specific users */
    POLICY_GROUP        /**< Policy group that applies to specific groups */
};

/** Enumeration of allow or deny policy rules */
enum PolicyPermission {
    POLICY_DENY,    /**< Deny policy rule */
    POLICY_ALLOW    /**< Allow policy rule */
};
}


class _PolicyDB;
class NormalizedMsgHdr;

/**
 * Managed object wrapper for policy database class.
 */
typedef qcc::ManagedObj<_PolicyDB> PolicyDB;

/**
 * Normalized string ID type.
 */
typedef uint32_t StringID;


/**
 * Real policy database class.
 */
class _PolicyDB {
  public:
    typedef qcc::ManagedObj<std::unordered_set<StringID> > IDSet;

    /**
     * Constructor.
     */
    _PolicyDB();

    /**
     * Determine if the remote application running as the specified user and
     * group is allowed connect to the bus.
     *
     * @param uid   Numeric user id
     * @param gid   Numeric group id
     *
     * @return true = connection allowed, false = connection denied.
     */
    bool OKToConnect(uint32_t uid, uint32_t gid) const;

    /**
     * Determine if the remote application is allowed to own the specified bus
     * name.
     *
     * @param busName       Bus name requested
     * @param ep            BusEndpoint requesting to own the busName
     *
     * @return true = ownership allowed, false = ownership denied.
     */
    bool OKToOwn(const char* busName, BusEndpoint& ep) const;

    /**
     * Determine if the destination is allowed to receive the specified
     * message.
     *
     * @param nmh       Normalized message header
     * @param dest      BusEndpoint where the router intends to send the message
     *
     * @return true = receive allowed, false = receive denied.
     */
    bool OKToReceive(const NormalizedMsgHdr& nmh, BusEndpoint& dest) const;

    /**
     * Determine if the sender is allowed to send the specified message.
     *
     * @param nmh       Normalized message header
     * @param dest      BusEndpoint where the router intends to send the message
     * @param destIDSet Alternate destination ID set (internal use only)
     *
     * @return true = send allowed, false = send denied.
     */
    bool OKToSend(const NormalizedMsgHdr& nmh, BusEndpoint& dest, const IDSet* destIDSet = NULL) const;

    /**
     * Convert a string to a normalized form.
     *
     * @param key   The string to be converted to a normalized form.
     *
     * @return Normalized ID of string.
     */
    StringID LookupStringID(const char* key) const;

    /**
     * Generate a set of normalized prefixes for the given string.
     *
     * @param idStr     The string to be converted to a set of normalized prefixes.
     * @param sep       Separator character between prefix segments
     *
     * @return Set of normalized prefix IDs of the string.
     */
    const IDSet LookupStringIDPrefix(const char* idStr, char sep) const;

    /**
     * Convert a bus name to a normalized form.
     *
     * @param busName   The bus name to be converted to a normalized form.
     *
     * @return Set of equivalent bus name IDs
     */
    const IDSet LookupBusNameID(const char* busName) const;

    void NameOwnerChanged(const qcc::String& alias,
                          const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                          const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer);

    /**
     * Adds a rule to the Policy database.
     *
     * @param cat           Policy category
     * @param catValue      Policy category differentiator (possible values depend on category)
     * @param permstr       Either "allow" or "deny"
     * @param ruleAttrs     Set of Key-Value pairs defining rule match criteria
     *
     * @return  true = rule added successfully, false = failed to add rule
     */
    bool AddRule(const qcc::String& cat,
                 const qcc::String& catValue,
                 const qcc::String& permStr,
                 const std::map<qcc::String, qcc::String>& ruleAttrs);

    /**
     * Helper function to Finalize() for repopulating busNameIDMap.
     *
     * @param alias     Bus name alias
     * @param name      Bus name
     */
    void AddAlias(const qcc::String& alias, const qcc::String& name);

    /**
     * This performs final policy setup after all of the rules have been
     * added.  It ensures that the Bus Name ID Map gets pre-populated based on
     * the contents of the NameTable in the event that the configuration was
     * reloaded some time after startup.
     */
    void Finalize(Bus* bus);


  private:

    static const StringID ID_NOT_FOUND = 0xffffffff;    /**< constant for string not found */
    static const StringID NIL_MATCH = 0xfffffffe;       /**< unmatchable string id */
    static const StringID WILDCARD = 0x0;               /**< match everything string id */

    /**
     * Container for all the matching criteria for a rule.
     */
    class PolicyRule {
      public:
        policydb::PolicyPermission permission;    /**< allow or deny rule */

        StringID interface;             /**< normalized interface name */
        StringID member;                /**< normalized member name */
        StringID error;                 /**< normalized error name */
        StringID busName;               /**< normalized well known bus name */
        AllJoynMessageType type;        /**< message type */
        StringID path;                  /**< normalized object path */
        StringID pathPrefix;            /**< normalized object path prefix */

        StringID own;                   /**< normalized well known bus name for ownership purposes */
        StringID ownPrefix;             /**< normalized well known bus name prefix for ownership purposes */

        bool userSet;                   /**< indicates if user has been set */
        bool userAny;                   /**< indicates if user has been set to "*" */
        uint32_t user;                  /**< numeric user id */
        bool groupSet;                  /**< indicates if group has been set */
        bool groupAny;                  /**< indicates if group has been set to "*" */
        uint32_t group;                 /**< numeric group id */

#ifndef NDEBUG
        qcc::String ruleString;         /**< regenerated xml rule string for debugging purposes */
#endif

        /**
         * Constructor.
         *
         * @param permission    Allow or deny permission for the rule being created
         */
        PolicyRule(policydb::PolicyPermission permission) :
            permission(permission),
            interface(WILDCARD),
            member(WILDCARD),
            error(WILDCARD),
            busName(WILDCARD),
            type(MESSAGE_INVALID),
            path(WILDCARD),
            pathPrefix(WILDCARD),
            own(WILDCARD),
            ownPrefix(WILDCARD),
            userSet(false),
            userAny(false),
            user(-1),
            groupSet(false),
            groupAny(false),
            group(-1)
        { }

        /**
         * Check if interface name matches.
         *
         * @param other     Normalized interface name for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckInterface(StringID other) const
        {
            /* Interface names are optional in messages and the documentation
             * of dbus-daemon suggests that such messages always match where
             * the interface is concerned; thus '(other == WILDCARD)' returns
             * 'true'.
             */
            return ((interface == WILDCARD) || (other == WILDCARD) || (interface == other));
        }

        /**
         * Check if member name matches.
         *
         * @param other     Normalized member name for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckMember(StringID other) const
        {
            return ((member == WILDCARD) || (member == other));
        }

        /**
         * Check if error name matches.
         *
         * @param other     Normalized error name for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckError(StringID other) const
        {
            return ((error == WILDCARD) || (error == other));
        }

        /**
         * Check if bus name matches.
         *
         * @param other     Normalized bus name for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckBusName(const IDSet& bnIDSet) const
        {
            return ((busName == WILDCARD) || (bnIDSet->find(busName) != bnIDSet->end()));
        }

        /**
         * Check if type matches.
         *
         * @param other     Normalized type for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckType(AllJoynMessageType other) const
        {
            return ((type == MESSAGE_INVALID) || (type == other));
        }

        /**
         * Check if object path matches.
         *
         * @param other     Normalized object path for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckPath(StringID other, const _PolicyDB::IDSet& prefixes) const
        {
            return (((path == WILDCARD) && (pathPrefix == WILDCARD)) ||
                    (path == other) || (prefixes->find(pathPrefix) != prefixes->end()));
        }

        /**
         * Check if bus name matches for ownership rule.
         *
         * @param other     Normalized bus name for comparision
         * @param prefixes  Normalized bus name prefixes for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckOwn(StringID other, const _PolicyDB::IDSet& prefixes) const
        {
            return (((own == WILDCARD) && (ownPrefix == WILDCARD)) ||
                    (own == other) || (prefixes->find(ownPrefix) != prefixes->end()));
        }

        /**
         * Check if user ID matches.
         *
         * @param other     Numerical user id for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckUser(StringID other) const
        {
            return (!userSet || userAny || (user == other));
        }

        /**
         * Check if group ID matches.
         *
         * @param other     Numerical group id for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckGroup(StringID other) const
        {
            return (!groupSet || groupAny || (group == other));
        }
    };

    typedef std::list<PolicyRule> PolicyRuleList;   /**< Policy rule list typedef */
    typedef std::unordered_map<uint32_t, PolicyRuleList> IDRuleMap; /**< UID/GID Rule map typedef */

    /**
     * Collection of policy rules for each category.
     */
    struct PolicyRuleListSet {
        PolicyRuleList defaultRules;        /**< default rules */
        IDRuleMap groupRules;               /**< group rules on a per group id basis */
        IDRuleMap userRules;                /**< user rules on a per user id basis */
        PolicyRuleList mandatoryRules;      /**< mandatory rules */
    };

    /** typedef for mapping a string to a numerical value for normalization */
    typedef std::unordered_map<qcc::StringMapKey, StringID> StringIDMap;

    /** typedef for mapping a string to a numerical value for normalization */
    typedef std::unordered_map<qcc::StringMapKey, IDSet> BusNameIDMap;

    /**
     * Adds rules to specific rule sets.  Called by public AddRule to add
     * rules to rulesets from the appropriate policy category.
     *
     * @param ownList       [OUT] ownership rule set to be filled
     * @param sendList      [OUT] sender rule set to be filled
     * @param receiveList   [OUT] receiver rule set to be filled
     * @param connectList   [OUT] connection rule set to be filled
     * @param permission    allow or deny rule
     * @param ruleAttrs     Set of Key-Value pairs defining rule match criteria
     */
    bool AddRule(PolicyRuleList& ownList,
                 PolicyRuleList& connectList,
                 PolicyRuleList& sendList,
                 PolicyRuleList& receiveList,
                 policydb::PolicyPermission permission,
                 const std::map<qcc::String, qcc::String>& ruleAttrs);

    /**
     * Get a normalized string ID for the specified string.  If string is
     * empty, the returned ID will be NIL_MATCH.  If the string is not empty
     * and has not been seen before a new ID number will be assigned to that
     * string.  If the string is not empty and has been seen before, the ID
     * previously assigned to that string will be returned.
     *
     * @param key   string to be normalized into an ID number
     *
     * @return  The strings normalization ID number.
     */
    StringID UpdateDictionary(const qcc::String& key);

    /**
     * Check rule list for rule about connecting.
     *
     * @param allow     [OUT] true for "allow" rule, false for "deny" rule
     * @param ruleList  rule list to search for match
     * @param uid       numeric user ID
     * @param gid       numeric group ID
     *
     * @return  true if match found, false if match not found
     */
    static bool CheckConnect(bool& allow, const PolicyRuleList& ruleList, uint32_t uid, uint32_t gid);

    /**
     * Check rule list for rule about owning a bus name.
     *
     * @param allow     [OUT] true for "allow" rule, false for "deny" rule
     * @param ruleList  rule list to search for match
     * @param bnid      normalized bus name ID
     * @param prefixes  normalized set of bus name prefix IDs
     *
     * @return  true if match found, false if match not found
     */
    static bool CheckOwn(bool& allow, const PolicyRuleList& ruleList, StringID bnid, const IDSet& prefixes);

    /**
     * Check rule list for rule about message.
     *
     * @param allow     [OUT] true for "allow" rule, false for "deny" rule
     * @param ruleList  rule list to search for match
     * @param nmh       normalized message header
     * @param bnIDSet   set of normalized bus names
     *
     * @return  true if match found, false if match not found
     */
    static bool CheckMessage(bool& allow, const PolicyRuleList& ruleList,
                             const NormalizedMsgHdr& nmh, const IDSet& bnIDSet,
                             uint32_t userId, uint32_t groupId);

    PolicyRuleListSet ownRS;        /**< bus name ownership policy rule sets */
    PolicyRuleListSet sendRS;       /**< sender message policy rule sets */
    PolicyRuleListSet receiveRS;    /**< receiver message policy rule sets */
    PolicyRuleListSet connectRS;    /**< bus connect policy rule sets */

    StringIDMap dictionary;         /**< mapping of strings to normalized IDs */
    BusNameIDMap busNameIDMap;      /**< mapping of bus names to a set of equivalent IDs */
    mutable qcc::RWLock lock;       /**< rwlock to protect R/W contention */

    friend class NormalizedMsgHdr;
};



/**
 * This class converts and stores a message's header information in a form
 * that allows for very fast lookup in unordered_map<>'s.
 */
class NormalizedMsgHdr {
  public:
    /**
     * This constructor converts a message header into a normalized form
     * based on information in the policy rules for very fast lookup.
     *
     * @param msg       Reference to the message to be normalized
     * @param policy    Pointer to the PolicyDB
     */
    NormalizedMsgHdr(const Message& msg, const PolicyDB& policy, BusEndpoint& sender) :
#ifndef NDEBUG
        msg(msg),
#endif
        ifcID(policy->LookupStringID(msg->GetInterface())),
        memberID(policy->LookupStringID(msg->GetMemberName())),
        errorID(policy->LookupStringID(msg->GetErrorName())),
        pathID(policy->LookupStringID(msg->GetObjectPath())),
        pathIDSet(policy->LookupStringIDPrefix(msg->GetObjectPath(), '/')),
        destIDSet(policy->LookupBusNameID(msg->GetDestination())),
        senderIDSet(policy->LookupBusNameID(msg->GetSender())),
        type(msg->GetType()),
        sender(sender)
    {
        if (destIDSet->empty()) {
            const char* destStr = msg->GetDestination();
            // GetDestination() will return an empty string if there is no destination
            if (destStr[0] != '\0') {
                StringID nid = policy->LookupStringID(destStr);
                if (nid != _PolicyDB::ID_NOT_FOUND) {
                    destIDSet->insert(nid);
                }
            }
        }
        if (senderIDSet->empty()) {
            const char* senderStr = msg->GetSender();
            // GetSender() will return an empty string if there is no sender
            if (senderStr[0] != '\0') {
                StringID nid = policy->LookupStringID(senderStr);
                if (nid != _PolicyDB::ID_NOT_FOUND) {
                    senderIDSet->insert(nid);
                }
            }
        }
    }

  private:
    friend class _PolicyDB;  /**< Give PolicyDB access to the internals */

#ifndef NDEBUG
    const Message msg;                      /**< Reference to original message for debug purposes */
#endif
    StringID ifcID;                         /**< normalized interface name */
    StringID memberID;                      /**< normalized member name */
    StringID errorID;                       /**< normalized error name */
    StringID pathID;                        /**< normalized object path */
    const _PolicyDB::IDSet pathIDSet;       /**< set of normalized object path prefixes */
    _PolicyDB::IDSet destIDSet;             /**< set of normalized well known bus name destinations */
    _PolicyDB::IDSet senderIDSet;           /**< set of normalized well known bus name senders */
    AllJoynMessageType type;                /**< message type */
    BusEndpoint& sender;                    /**< sender bus endpoint */
};


}

#endif
