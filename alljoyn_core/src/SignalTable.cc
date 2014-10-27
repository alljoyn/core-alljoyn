/**
 * @file
 *
 * This file defines the signal hash table class
 *
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
#include <qcc/Debug.h>
#include <qcc/String.h>

#include <list>

#include "SignalTable.h"

/** @internal */
#define QCC_MODULE "ALLJOYN"

using namespace std;

namespace ajn {

void SignalTable::Add(MessageReceiver* receiver,
                      MessageReceiver::SignalHandler handler,
                      const InterfaceDescription::Member* member,
                      const qcc::String& rule)
{
    QCC_DbgTrace(("SignalTable::Add(iface = {%s}, member = {%s}, rule = \"%s\")",
                  member->iface->GetName(),
                  member->name.c_str(),
                  rule.c_str()));
    Entry entry(handler, receiver, member, rule);
    Key key(member->iface->GetName(), member->name);
    lock.Lock(MUTEX_CONTEXT);
    hashTable.insert(pair<const Key, Entry>(key, entry));
    lock.Unlock(MUTEX_CONTEXT);
}

QStatus SignalTable::Remove(MessageReceiver* receiver,
                            MessageReceiver::SignalHandler handler,
                            const InterfaceDescription::Member* member,
                            const char* rule)
{
    QStatus status = ER_FAIL;
    Key key(member->iface->GetName(), member->name.c_str());
    iterator iter;
    pair<iterator, iterator> range;
    Rule matchRule(rule);

    lock.Lock(MUTEX_CONTEXT);
    range = hashTable.equal_range(key);
    iter = range.first;
    while (iter != range.second) {
        if ((iter->second.object == receiver) &&
            (iter->second.handler == handler) &&
            (iter->second.rule == matchRule)) {
            hashTable.erase(iter);
            status = ER_OK;
            break;
        } else {
            ++iter;
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

void SignalTable::RemoveAll(MessageReceiver* receiver)
{
    bool removed;
    lock.Lock(MUTEX_CONTEXT);
    do {
        removed = false;
        for (iterator iter = hashTable.begin(); iter != hashTable.end(); ++iter) {
            if (iter->second.object == receiver) {
                hashTable.erase(iter);
                removed = true;
                break;
            }
        }
    } while (removed);
    lock.Unlock(MUTEX_CONTEXT);
}

pair<SignalTable::const_iterator, SignalTable::const_iterator> SignalTable::Find(const char* iface,
                                                                                 const char* signalName)
{
    Key key(iface, signalName);
    return hashTable.equal_range(key);
}

}

