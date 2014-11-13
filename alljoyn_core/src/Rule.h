/**
 * @file
 * The Rule class encapsulates match rules.
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
#ifndef _ALLJOYN_RULE_H
#define _ALLJOYN_RULE_H

#include <map>
#include <set>

#include <qcc/String.h>
#include <alljoyn/Message.h>
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
    std::map<uint32_t, qcc::String> args;

    /** Equality comparison */
    bool operator==(const Rule& o) const {
        return (type == o.type) && (sender == o.sender) && (iface == o.iface) &&
               (member == o.member) && (path == o.path) && (destination == o.destination) &&
               (implements == o.implements) && (args == o.args);
    }

    /** Constructor */
    Rule() : type(MESSAGE_INVALID), sessionless(SESSIONLESS_NOT_SPECIFIED) { }

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

}

#endif
