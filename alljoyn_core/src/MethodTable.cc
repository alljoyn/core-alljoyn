/**
 * @file
 *
 * This file defines the method hash table class
 *
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014, AllSeen Alliance. All rights reserved.
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

#include "MethodTable.h"

/** @internal */
#define QCC_MODULE "ALLJOYN"

using namespace qcc;
using namespace std;

namespace ajn {

MethodTable::~MethodTable()
{
    lock.Lock(MUTEX_CONTEXT);
    MapType::iterator iter = hashTable.begin();
    while (iter != hashTable.end()) {
        delete iter->second;
        ++iter;
    }

    hashTable.clear();
    lock.Unlock(MUTEX_CONTEXT);
}

void MethodTable::Add(BusObject* object,
                      MessageReceiver::MethodHandler func,
                      const InterfaceDescription::Member* member,
                      void* context)
{
    Entry* entry = new Entry(object, func, member, context);
    lock.Lock(MUTEX_CONTEXT);
    hashTable[Key(object->GetPath(), entry->ifaceStr.empty() ? NULL : entry->ifaceStr.c_str(), member->name.c_str())] = entry;

    /* Method calls don't require an interface so we need to add an entry with a NULL interface */
    if (!entry->ifaceStr.empty()) {
        // specification states "if there are multiple properties on an object
        // with the same name, the results are undefined." We choose to only
        // use the first member that was added.
        if (hashTable.find(Key(object->GetPath(), NULL, member->name.c_str())) == hashTable.end()) {
            hashTable[Key(object->GetPath(), NULL, member->name.c_str())] = new Entry(*entry);
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
}

MethodTable::SafeEntry* MethodTable::Find(const char* objectPath,
                                          const char* iface,
                                          const char* methodName)
{
    SafeEntry* entry = NULL;
    Key key(objectPath, iface, methodName);
    lock.Lock(MUTEX_CONTEXT);
    MapType::iterator iter = hashTable.find(key);
    if (iter != hashTable.end()) {
        entry = new SafeEntry();
        entry->Set(iter->second);
    }
    lock.Unlock(MUTEX_CONTEXT);
    return entry;
}

void MethodTable::RemoveAll(BusObject* object)
{
    MapType::iterator iter;
    /*
     * Iterate over all entries deleting all entries that reference the object
     */
    lock.Lock(MUTEX_CONTEXT);
    iter = hashTable.begin();
    while (iter != hashTable.end()) {
        if (iter->second->object == object) {
            Entry* deleteMe = iter->second;
            hashTable.erase(iter);
            delete deleteMe;
            iter = hashTable.begin();
        } else {
            ++iter;
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
}

void MethodTable::AddAll(BusObject* object)
{
    object->InstallMethods(*this);
}

}

