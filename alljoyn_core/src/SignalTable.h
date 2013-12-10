#ifndef _ALLJOYN_SIGNALTABLE_H
#define _ALLJOYN_SIGNALTABLE_H
/**
 * @file
 * This file defines the signal hash table class
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#ifndef __cplusplus
#error Only include SignalTable.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/StringMapKey.h>

#include <vector>

#include <qcc/String.h>
#include <qcc/StringMapKey.h>
#include <qcc/Mutex.h>

#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/MessageReceiver.h>

#include <alljoyn/Status.h>

#include <qcc/STLContainer.h>

namespace ajn {

/**
 * %SignalTable is a multimap that maps interface/signalname and/or source path to SignalHandler instances.
 */
class SignalTable {

  public:

    /**
     * Type definition for signal hash table key
     */
    struct Key {
        qcc::StringMapKey sourcePath;           /**< The object path of the signal sender */
        qcc::StringMapKey iface;                /**< The Interface name */
        qcc::StringMapKey signalName;           /**< The signal name */

        /**
         * Constructor used for lookups only (no storage)
         */
        Key(const char* src, const char* ifc, const char* sig)
            : sourcePath(src), iface(ifc), signalName(sig) { }

        /**
         * Constructor used for storage into hash table (no dangling char*)
         */
        Key(const qcc::String& src, const qcc::String& ifc, const qcc::String& sig)
            : sourcePath(src), iface(ifc), signalName(sig) { }
    };

    /**
     * Type definition for a signal hash table entry
     */
    struct Entry {
        MessageReceiver::SignalHandler handler;      /**< SignalHandler instance */
        MessageReceiver* object;                     /**< Object that received the signal */
        const InterfaceDescription::Member* member;  /**< Signal member */

        /**
         * Construct an Entry
         */
        Entry(const MessageReceiver::SignalHandler& handler, MessageReceiver* object, const InterfaceDescription::Member* member)
            : handler(handler),
            object(object),
            member(member) { }

        /**
         * Construct an empty Entry.
         */
        Entry(void) : handler(), object(NULL), member(NULL) { }
    };

    /** %Hash functor */
    struct Hash {
        /** Calculate hash for Key k */
        size_t operator()(const Key& k) const {
            /* source path cannot factor into hash because a key with no sourcepath is considered equal to one that does */
            size_t hash = 0;
            for (const char* p = k.signalName.c_str(); *p; ++p) {
                hash = *p + hash * 11;
            }
            for (const char* p = k.iface.c_str(); *p; ++p) {
                hash += *p * 7;
            }
            return hash;
        }
    };

    /** Functor for testing 2 keys for equality */
    struct Equal {
        /** Return true two keys are equal */
        bool operator()(const Key& k1, const Key& k2) const {
            /* If either source path is null, then this field should be treated as don't care */
            if ((k1.sourcePath.empty()) || (k2.sourcePath.empty())) {
                return (0 == strcmp(k1.iface.c_str(), k2.iface.c_str())) && (0 == strcmp(k1.signalName.c_str(), k2.signalName.c_str()));
            } else {
                return (0 == strcmp(k1.iface.c_str(), k2.iface.c_str())) && \
                       (0 == strcmp(k1.signalName.c_str(), k2.signalName.c_str())) && \
                       (0 == strcmp(k1.sourcePath.c_str(), k2.sourcePath.c_str()));
            }
        }
    };

    /**
     * Table iterator
     */
    typedef std::unordered_multimap<Key, Entry, Hash, Equal>::iterator iterator;

    /**
     * Const table iterator
     */
    typedef std::unordered_multimap<Key, Entry, Hash, Equal>::const_iterator const_iterator;

    /**
     * Add an entry to the signal hash table.
     *
     * @param receiver    Object receiving the message.
     * @param func        Handler for signal.
     * @param member      Signal member.
     * @param sourcePath  Signal originator or empty for all signal originators.
     */
    void Add(MessageReceiver* receiver,
             MessageReceiver::SignalHandler func,
             const InterfaceDescription::Member* member,
             const qcc::String& sourcePath);

    /**
     * Remove an entry from the signal hash table.
     *
     * @param receiver    Object receiving the message.
     * @param func        Signal handler to remove
     * @param member      Signal member.
     * @param sourcePath  Signal originator or empty for all signal originators.
     */
    void Remove(MessageReceiver* receiver,
                MessageReceiver::SignalHandler func,
                const InterfaceDescription::Member* member,
                const char* sourcePath);

    /**
     * Remove all entries from the signal hash table for the specified receiver.
     *
     * @param receiver    Object receiving the message.
     */
    void RemoveAll(MessageReceiver* receiver);

    /**
     * Find Entries based on set of criteria.
     * Signal table lock should be held until iterators are no longer in use.
     *
     * @param sourcePath   The object path of the signal sender.
     * @param iface    The interface.
     * @param signalName   The signal name.
     *
     * @return   Iterator range of entries with matching criteria.
     */
    std::pair<const_iterator, const_iterator> Find(const char* sourcePath, const char* iface, const char* signalName);

    /**
     * Get the lock that protects the signal table.
     */
    void Lock(void) { lock.Lock(MUTEX_CONTEXT); }

    /**
     * Release the lock that protects the signal table.
     */
    void Unlock(void) { lock.Unlock(MUTEX_CONTEXT); }

  private:

    qcc::Mutex lock; /**< Lock protecting the signal table */

    /**  The hash table */
    std::unordered_multimap<Key, Entry, Hash, Equal> hashTable;
};

}

#endif
