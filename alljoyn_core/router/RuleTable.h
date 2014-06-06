/**
 * @file
 * RuleTable is a thread-safe store used for storing
 * and retrieving message bus routing rules.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2012,2014 AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_RULETABLE_H
#define _ALLJOYN_RULETABLE_H

#include <qcc/platform.h>

#include <map>
#include <set>

#include <qcc/String.h>
#include <qcc/Mutex.h>

#include <alljoyn/Message.h>

#include "BusEndpoint.h"

#include <alljoyn/Status.h>

namespace ajn {

/**
 * Rule defines a message bus routing rule.
 */
struct Rule {

    /** Rule type specifier */
    AllJoynMessageType type;

    /** Busname of sender or empty for all senders */
    qcc::String sender;

    /** Interface or empty for all interfaces */
    qcc::String iface;

    /** Member or empty for all methods */
    qcc::String member;

    /** Object path or empty for all object paths */
    qcc::String path;

    /** Destination bus name or empty for all destinations */
    qcc::String destination;

    /** true iff Rule specifies a filter for sessionless signals */
    enum {SESSIONLESS_NOT_SPECIFIED, SESSIONLESS_FALSE, SESSIONLESS_TRUE} sessionless;

    /** Interfaces implemented in org.alljoyn.About.Announce sessionless signal */
    std::set<qcc::String> implements;

    /** Map of argument matches */
    // @@ TODO

    /** Equality comparison */
    bool operator==(const Rule& o) const {
        return (type == o.type) && (sender == o.sender) && (iface == o.iface) &&
               (member == o.member) && (path == o.path) && (destination == o.destination) &&
               (implements == o.implements);
    }

    /** Constructor */
    Rule() : type(MESSAGE_INVALID) { }

    /**
     * Construct a rule from a rule string.
     *
     * @param ruleStr   String describing the rule.
     *                  This format of this string is specified in the DBUS spec.
     *                  AllJoyn has added the following additional parameters:
     *                     sessionless  - Valid values are "true" and "false"
     *
     * @param status    ER_OK if ruleStr was successfully parsed.
     */
    Rule(const char* ruleStr, QStatus* status = NULL);

    /**
     * Return true if messages matches rule.
     *
     * @param msg   Message to compare with rule.
     * @return  true if this rule matches the message.
     */
    bool IsMatch(Message& msg) const;

    /**
     * String representation of a rule
     */
    qcc::String ToString() const;

};


/** Rule iterator */
typedef std::multimap<BusEndpoint, Rule>::iterator RuleIterator;

/**
 * RuleTable is a thread-safe store used for storing
 * and retrieving message bus routing rules.
 */
class RuleTable {
  public:

    /**
     * Add a rule for an endpoint.
     *
     * @param endpoint   The endpoint that this rule applies to.
     * @param rule       Rule for endpoint
     * @return ER_OK if successful;
     */
    QStatus AddRule(BusEndpoint& endpoint, const Rule& rule);

    /**
     * Remove a rule for an endpoint.
     *
     * @param endpoint   The endpoint that rule applies to.
     * @param rule       Rule to remove.
     * @return ER_OK if successful;
     */
    QStatus RemoveRule(BusEndpoint& endpoint, Rule& rule);

    /**
     * Remove all rules for a given endpoint.
     *
     * @param endpoint    Endpoint whose rules will be removed.
     * @return ER_OK if successful;
     */
    QStatus RemoveAllRules(BusEndpoint& endpoint);

    /**
     * Obtain exclusive access to rule table.
     * This method only needs to be called before using methods that return or use
     * AllJoynRuleIterators. Atomic rule table operations will obtain the lock internally.
     *
     * @return ER_OK if successful.
     */
    QStatus Lock() { return lock.Lock(MUTEX_CONTEXT); }

    /**
     * Release exclusive access to rule table.
     *
     * @return ER_OK if successful.
     */
    QStatus Unlock() { return lock.Unlock(MUTEX_CONTEXT); }

    /**
     * Return an iterator to the start of the rules.
     * Caller should obtain lock before calling this method.
     * @return Iterator to first rule.
     */
    RuleIterator Begin() { return rules.begin(); }

    /**
     * Return an iterator to the end of the rules.
     * @return Iterator to end of rules.
     */
    RuleIterator End() { return rules.end(); }

    /**
     * Find all rules for a given endpoint.
     * Caller should obtain lock before calling this method.
     *
     * @param endpoint  Endpoint whose rules are needed.
     * @return  Iterator to first rule for given endpoint.
     */
    RuleIterator FindRulesForEndpoint(BusEndpoint& endpoint) {
        return rules.find(endpoint);
    }

    /**
     * lower_bound algorithm for rule table.
     *
     * @param ep     BusEndpoint that rule applies to.
     * @param rule   Rule
     * @return       lower_bound itertor for ep, rule.
     */
    RuleIterator LowerBound(BusEndpoint endpoint, const Rule& rule);

    /**
     * Advance iterator to next endpoint.
     *
     * @param endpoint   Endpoint before advance.
     * @return   Iterator to next endpoint in ruleTable or end.
     *
     */
    RuleIterator AdvanceToNextEndpoint(BusEndpoint endpoint) {
        std::multimap<BusEndpoint, Rule>::iterator ret = rules.upper_bound(endpoint);
        return ret;
    }

  private:
    qcc::Mutex lock;                            /**< Lock protecting rule table */
    std::multimap<BusEndpoint, Rule> rules;    /**< Rule table */
};

}

#endif
