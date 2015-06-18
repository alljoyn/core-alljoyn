/**
 * @file
 * MsgRouter is responsible for taking inbound messages and routing them
 * to an appropriate set of endpoints.
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
#include <qcc/platform.h>

#include <algorithm>
#include <cstring>

#include "RuleTable.h"

#include <qcc/Debug.h>
#include <qcc/String.h>

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

QStatus RuleTable::AddRule(BusEndpoint& endpoint, const Rule& rule)
{
    QCC_DbgPrintf(("AddRule for endpoint %s\n  %s", endpoint->GetUniqueName().c_str(), rule.ToString().c_str()));
    lock.Lock(MUTEX_CONTEXT);
    rules.insert(std::pair<BusEndpoint, Rule>(endpoint, rule));
    lock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

QStatus RuleTable::RemoveRule(BusEndpoint& endpoint, Rule& rule)
{
    QStatus status = ER_BUS_MATCH_RULE_NOT_FOUND;
    lock.Lock(MUTEX_CONTEXT);
    std::pair<RuleIterator, RuleIterator> range = rules.equal_range(endpoint);
    while (range.first != range.second) {
        if (range.first->second == rule) {
            const RuleIterator begin = range.first;
            const RuleIterator end = ++range.first;
            rules.erase(begin, end);
            status = ER_OK;
            break;
        }
        ++range.first;
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus RuleTable::RemoveAllRules(BusEndpoint& endpoint)
{
    lock.Lock(MUTEX_CONTEXT);
    std::pair<RuleIterator, RuleIterator> range = rules.equal_range(endpoint);
    if (range.first != rules.end()) {
        rules.erase(range.first, range.second);
    }
    lock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

bool RuleTable::OkToSend(const Message& msg, BusEndpoint& endpoint) const
{
    bool match = false;
    lock.Lock(MUTEX_CONTEXT);
    pair<RuleConstIterator, RuleConstIterator> range = rules.equal_range(endpoint);
    for (RuleConstIterator it = range.first; !match && (it != range.second); ++it) {
        match = it->second.IsMatch(msg);

        /*
         * This little hack is to make DaemonRouter::PushMessage() work with the
         * same behavior as it did previously which is that sessionless messages
         * that match rules for the endpoint that include sessionless='t' as
         * part of the rule will not be delivered by DaemonRouter::PushMessage()
         * directly, but rather the message will be delivered via
         * SessionlessObj.  It exists in this function so that DaemonRouter does
         * not need to know the internal gory details of Rules or the RuleTable.
         *
         * This hack imposes a hidden direct coupling with
         * DaemonRouter::PushMessage() that needs to be cleaned up at some point
         * in the future.  This will likely require that the interaction between
         * SessionlessObj and DaemonRouter change.
         */
        if (match && (it->second.sessionless == Rule::SESSIONLESS_TRUE)) {
            match = false;
            break;
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
    return match;
}


}
