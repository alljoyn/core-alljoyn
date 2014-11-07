/**
 * @file
 * MsgRouter is responsible for taking inbound messages and routing them
 * to an appropriate set of endpoints.
 */

/******************************************************************************
 * Copyright (c) 2009-2011,2014 AllSeen Alliance. All rights reserved.
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
    Lock();
    rules.insert(std::pair<BusEndpoint, Rule>(endpoint, rule));
    Unlock();
    return ER_OK;
}

QStatus RuleTable::RemoveRule(BusEndpoint& endpoint, Rule& rule)
{
    QStatus status = ER_BUS_MATCH_RULE_NOT_FOUND;
    Lock();
    std::pair<RuleIterator, RuleIterator> range = rules.equal_range(endpoint);
    while (range.first != range.second) {
        if (range.first->second == rule) {
            rules.erase(range.first);
            status = ER_OK;
            break;
        }
        ++range.first;
    }
    Unlock();
    return status;
}

QStatus RuleTable::RemoveAllRules(BusEndpoint& endpoint)
{
    Lock();
    std::pair<RuleIterator, RuleIterator> range = rules.equal_range(endpoint);
    if (range.first != rules.end()) {
        rules.erase(range.first, range.second);
    }
    Unlock();
    return ER_OK;
}

}
