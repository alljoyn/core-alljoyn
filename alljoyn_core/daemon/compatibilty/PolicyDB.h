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
#ifndef _POLICYDB_H
#define _POLICYDB_H

#include <qcc/platform.h>
#include <qcc/ManagedObj.h>
#include <qcc/String.h>
#include <qcc/StringMapKey.h>

#include <alljoyn/Message.h>

#include "NameTable.h"

#include <qcc/STLContainer.h>

#if !defined(ALLJOYN_BUILD_POLICY_DEBUG)
#define ALLJOYN_BUILD_POLICY_DEBUG 0
#endif

#if (ALLJOYN_BUILD_POLICY_DEBUG == 0) || defined(NDEBUG)
#define ALLJOYN_POLICY_DEBUG(x) do { } while (false)
#else
#define ALLJOYN_POLICY_DEBUG(x) x
#endif


namespace ajn {

namespace policydb {
/** Enumeration of different types of policy groups */
enum PolicyCategory {
    POLICY_CONTEXT,     /**< Policy group for either default or mandatory policies */
    POLICY_USER,        /**< Policy group that applies to specific users */
    POLICY_GROUP,       /**< Policy group that applies to specific groups */
    POLICY_AT_CONSOLE   /**< Policy group for when app is started from console or not */
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
 * Real policy database class.
 */
class _PolicyDB {
  public:
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
     * @param busNameID     Normalized bus name
     * @param uid           Numeric user id
     * @param gid           Numeric group id
     *
     * @return true = ownership allowed, false = ownership denied.
     */
    bool OKToOwn(uint32_t busNameID,
                 uint32_t uid,
                 uint32_t gid) const;

    /**
     * Determine if the destination is allowed to receive the specified
     * message.
     *
     * @param nmh   Normalized message header
     * @param uid   Numeric user id
     * @param gid   Numeric group id
     *
     * @return true = receive allowed, false = receive denied.
     */
    bool OKToReceive(const ajn::NormalizedMsgHdr& nmh,
                     uint32_t uid,
                     uint32_t gid) const;

    /**
     * Determine if the sender is allowed to send the specified message.
     *
     * @param nmh   Normalized message header
     * @param uid   Numeric user id
     * @param gid   Numeric group id
     *
     * @return true = send allowed, false = send denied.
     */
    bool OKToSend(const ajn::NormalizedMsgHdr& nmh,
                  uint32_t uid,
                  uint32_t gid) const;

    /**
     * Determine if the destination is allowed to eavesdrop the specified
     * message.
     *
     * @param nmh   Normalized message header
     * @param suid  Numeric user id of the sender
     * @param sgid  Numeric group id of the sender
     * @param duid  Numeric user id of the destination
     * @param dgid  Numeric group id of the destination
     *
     * @return true = receive allowed, false = receive denied.
     */
    bool OKToEavesdrop(const ajn::NormalizedMsgHdr& nmh,
                       uint32_t suid,
                       uint32_t sgid,
                       uint32_t duid,
                       uint32_t dgid) const;

    /**
     * Convert a string to a normalized form.
     *
     * @param str   The string to be converted to a normalized form.
     *
     * @return Normalized form of string.
     */
    uint32_t LookupStringID(const qcc::String& str) const;

    /**
     * @overloaded Convert a string to a normalized form.
     *
     * @param str   The string to be converted to a normalized form.
     *
     * @return Normalized form of string.
     */
    uint32_t LookupStringID(const char* str) const;

    /**
     * Name owner changed listener implementation for tracking when well known
     * names change onwer ship.  The ownership information is used when
     * normalizing bus names.
     *
     * @param alias     The well known name
     * @param oldOwner  Unique name of previous owner or NULL if no previous owner
     * @param newOwner  Unique name of new owner or NULL of no new owner
     */
    void NameOwnerChanged(const qcc::String& alias,
                          const qcc::String* oldOwner,
                          const qcc::String* newOwner);

    /**
     * Adds a rule to the Policy database.
     *
     * @param cat           Policy category
     * @param catValue      Policy category differentiator (possible values depend on category)
     * @param permission    Either allow or deny
     * @param ruleAttrs     Set of Key-Value pairs defining rule match criteria
     *
     * @return  true = rule added successfully, false = failed to add rule
     */
    bool AddRule(policydb::PolicyCategory cat,
                 qcc::String& catValue,
                 policydb::PolicyPermission permission,
                 const std::map<qcc::String, qcc::String>& ruleAttrs);

    /**
     * Indicates if eavesdropping was enabled on any rule.
     *
     * @return  true = eavesdropping enable, false = eavesdropping disabled
     */
    bool EavesdropEnabled() const { return eavesdrop; }

  private:

    static const uint32_t ID_NOT_FOUND = 0xffffffff;    /**< constant for string not found */
    static const uint32_t NIL_MATCH = 0xfffffffe;       /**< unmatchable string id */
    static const uint32_t WILDCARD = 0x0;               /**< match everything string id */


    typedef std::unordered_set<uint32_t> BusNameIDSet;

    /**
     * Container for all the matching criteria for a rule.
     */
    struct PolicyRule {
        policydb::PolicyPermission permission;    /**< allow or deny rule */
        uint32_t interface;             /**< normalized interface name */
        uint32_t member;                /**< normalized member name */
        uint32_t error;                 /**< normalized error name */
        uint32_t busName;               /**< normalized well known bus name */
        ajn::AllJoynMessageType type;     /**< message type */
        uint32_t path;                  /**< normalized object path */

        bool requested_reply;           /**< requested_reply flag */

        bool eavesdrop;                 /**< eavesdrop enable flag */

        uint32_t own;                   /**< normalized well known bus name for ownership purposes */

        bool userSet;                   /**< indicates if user has been set */
        uint32_t user;                  /**< numeric user id */
        bool groupSet;                  /**< indicates if group has been set */
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
            type(ajn::MESSAGE_INVALID),
            path(WILDCARD),
            requested_reply(permission == policydb::POLICY_ALLOW),
            eavesdrop(false),
            own(WILDCARD),
            userSet(false),
            user(-1),
            groupSet(false),
            group(-1)
        { }

        /**
         * Check if interface name matches.
         *
         * @param other     Normalized interface name for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckInterface(uint32_t other) const
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
        inline bool CheckMember(uint32_t other) const
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
        inline bool CheckError(uint32_t other) const
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
        inline bool CheckBusName(const BusNameIDSet& bnIDSet) const
        {
            return ((busName == WILDCARD) || (bnIDSet.find(busName) != bnIDSet.end()));
        }

        /**
         * Check if type matches.
         *
         * @param other     Normalized type for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckType(ajn::AllJoynMessageType other) const
        {
            return ((type == ajn::MESSAGE_INVALID) || (type == other));
        }

        /**
         * Check if object path matches.
         *
         * @param other     Normalized object path for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckPath(uint32_t other) const
        {
            return ((path == WILDCARD) || (path == other));
        }

        /**
         * Check if bus name matches for ownership rule.
         *
         * @param other     Normalized bus name for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckOwn(uint32_t other) const
        {
            return ((own == WILDCARD) || (own == other));
        }

        /**
         * Check if user ID matches.
         *
         * @param other     Numerical user id for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckUser(uint32_t other) const
        {
            return (!userSet || (user == other));
        }

        /**
         * Check if group ID matches.
         *
         * @param other     Numerical group id for comparision
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckGroup(uint32_t other) const
        {
            return (!groupSet || (group == other));
        }

        /**
         * Check if rule matches in eavesdrop context.
         *
         * @param edCtx     true indicates to check for match in eavesdrop context,
         *                  false indicates to check for match in non-eavesdrop context
         *
         * @return true = matches, false = does not match
         */
        inline bool CheckEavesdrop(bool edCtx) const
        {
            /* The documentation about eavesdropping in dbus-daemon is a bit
             * convoluted.  Basically, it boils down to that the rule always
             * matches (where eavesdropping is concerned) except for allow
             * rules where the rule's eavesdrop is set 'false' and we are
             * checking to send the message to an eavesdropper, and for deny
             * rules where the rule's eavesdrop is set to 'true' and we are
             * checking to send the message to an ordinary recipient.
             */
            return permission ? (!edCtx || eavesdrop) : (edCtx || !eavesdrop);
        }
    };

    typedef std::list<PolicyRule> PolicyRuleList;   /**< Policy rule list typedef */

    /**
     * Collection of policy rules for each category.
     */
    struct PolicyRuleListSet {
        PolicyRuleList defaultRules;        /**< default rules */
        std::unordered_map<uint32_t, PolicyRuleList> groupRules; /**< group rules on a per group id basis */
        std::unordered_map<uint32_t, PolicyRuleList> userRules;  /**< user rules on a per user id basis */
        PolicyRuleList atConsoleRules;      /**< at console rules */
        PolicyRuleList notAtConsoleRules;   /**< not at console rules */
        PolicyRuleList mandatoryRules;      /**< mandatory rules */
    };

    /** typedef for mapping a string to a numerical value for normalization */
    typedef std::unordered_map<qcc::StringMapKey, uint32_t> StringIDMap;

    /** typedef for mapping a unique bus name to a set of normalized well known bus names */
    typedef std::unordered_map<qcc::StringMapKey, BusNameIDSet> UniqueNameIDMap;

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
                 PolicyRuleList& sendList,
                 PolicyRuleList& receiveList,
                 PolicyRuleList& connectList,
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
    uint32_t GetStringIDMapUpdate(const qcc::String& key);

    /**
     * Convert a message type string the ajn::AllJoynMessageType enum value.
     *
     * @param str   string to be converted
     * @param type  [OUT] the message type
     *
     * @return  true if conversion succeeded, false if conversion failed
     */
    static bool MsgTypeStrToEnum(const qcc::String& str, ajn::AllJoynMessageType& type);

    /**
     * Convert "true" or "false" string to a boolean.
     *
     * @param str   string to be converted
     * @param b     [OUT] true or false depending on the string
     *
     * @return  true if conversion succeeded, false if conversion failed
     */
    static bool TrueFalseStrToBool(const qcc::String& str, bool& b);

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
    bool CheckConnect(bool& allow, const PolicyRuleList& ruleList,
                      uint32_t uid, uint32_t gid) const;

    /**
     * Check rule list for rule about owning a bus name.
     *
     * @param allow     [OUT] true for "allow" rule, false for "deny" rule
     * @param ruleList  rule list to search for match
     * @param bnid      normalized bus name id
     *
     * @return  true if match found, false if match not found
     */
    bool CheckOwn(bool& allow, const PolicyRuleList& ruleList, uint32_t bnid) const;

    /**
     * Check rule list for rule about message.
     *
     * @param allow     [OUT] true for "allow" rule, false for "deny" rule
     * @param ruleList  rule list to search for match
     * @param nmh       normalized message header
     * @param bnIDSet   set of normalized bus names
     * @param eavesdrop flag indicating if the check is for eavesdropping or not.
     *
     * @return  true if match found, false if match not found
     */
    bool CheckMessage(bool& allow, const PolicyRuleList& ruleList,
                      const ajn::NormalizedMsgHdr& nmh,
                      const BusNameIDSet& bnIDSet,
                      bool eavesdrop) const;

    bool eavesdrop;     /**< indicated if there is a rule specifying eavesdropping */

    PolicyRuleListSet ownRS;        /**< bus name ownership policy rule sets */
    PolicyRuleListSet sendRS;       /**< sender message policy rule sets */
    PolicyRuleListSet receiveRS;    /**< receiver message policy rule sets */
    PolicyRuleListSet connectRS;    /**< bus connect policy rule sets */

    StringIDMap stringIDs;          /**< mapping of strings to normalization IDs */
    UniqueNameIDMap uniqueNameMap;  /**< mapping of unique bus names to normalized well known bus names */
    StringIDMap busNameMap;         /**< mapping of well known bus names to normalization IDs */
    mutable qcc::Mutex bnLock;      /**< mutex protecting access to uniqueNameMap and busNameMap when normalizing unique names to list of normalized well known bus names. */

    friend class ajn::NormalizedMsgHdr;
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
    NormalizedMsgHdr(const ajn::Message& msg, const PolicyDB& policy) :
        ifcID(policy->LookupStringID(msg->GetInterface())),
        memberID(policy->LookupStringID(msg->GetMemberName())),
        errorID(policy->LookupStringID(msg->GetErrorName())),
        pathID(policy->LookupStringID(msg->GetObjectPath())),
        type(msg->GetType())
    {
        policy->bnLock.Lock(MUTEX_CONTEXT);
        InitBusNameID(policy, msg->GetSender(), senderIDList);
        InitBusNameID(policy, msg->GetDestination(), destIDList);
        policy->bnLock.Unlock(MUTEX_CONTEXT);
    }

  private:
    friend class _PolicyDB;  /**< Give PolicyDB access to the internals */

    /**
     * Helper function generate a set of normalized well known bus names
     * for given bus name string.  If the given bus name is already a well
     * known name, then the set will just be one entry.  If the given bus
     * name is a unique name, the the list will be for all well known
     * names assocated with that unique bus name.
     *
     * @param policy    Pointer to the PolicyDB
     * @param bnStr     String with either the well known or unique bus name
     * @param bnIDSet   The normalized bus name set being filled.
     */
    static inline void InitBusNameID(const PolicyDB& policy,
                                     const char* bnStr,
                                     _PolicyDB::BusNameIDSet& bnIDSet)
    {
        if (bnStr && (bnStr[0] == ':')) {
            _PolicyDB::UniqueNameIDMap::const_iterator unit(policy->uniqueNameMap.find(bnStr));
            if (unit != policy->uniqueNameMap.end()) {
                bnIDSet.insert(unit->second.begin(), unit->second.end());
            }
        } else {
            bnIDSet.insert(policy->LookupStringID(bnStr));
        }
    }

    uint32_t ifcID;             /**< normalized interface name */
    uint32_t memberID;          /**< normalized member name */
    uint32_t errorID;           /**< normalized error name */
    uint32_t pathID;            /**< normalized object path */
    ajn::AllJoynMessageType type; /**< message type */
    _PolicyDB::BusNameIDSet destIDList;    /**< set of normalized well known bus name destinations */
    _PolicyDB::BusNameIDSet senderIDList;  /**< set of normalized well known bus name senders */
};


}

#endif
