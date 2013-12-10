#ifndef _ALLJOYN_METHODTABLE_H
#define _ALLJOYN_METHODTABLE_H
/**
 * @file
 * This file defines the method hash table class
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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

#include <vector>

#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/atomic.h>

#include <alljoyn/BusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/MessageReceiver.h>

#include <alljoyn/Status.h>

#include <qcc/STLContainer.h>

namespace ajn {

/**
 * %MethodTable is a hash table that maps object paths to BusObject instances.
 */
class MethodTable {

  public:

    /**
     * Type definition for a method hash table entry
     */
    struct Entry {
        /**
         * Construct an Entry
         */
        Entry(BusObject* object,
              MessageReceiver::MethodHandler handler,
              const InterfaceDescription::Member* member,
              void* context)
            : object(object), handler(handler), member(member), context(context), ifaceStr(member->iface->GetName()), methodStr(member->name),
            refCount(0) { }

        ~Entry()
        {
            while (0 != refCount) {
                qcc::Sleep(1);
            }
        }

        /**
         * Construct an empty Entry.
         */
        Entry(void) : object(NULL), handler(), ifaceStr(), methodStr() { }

        BusObject* object;                             /**<  BusObject instance*/
        MessageReceiver::MethodHandler handler;        /**<  Handler for method */
        const InterfaceDescription::Member* member;    /**<  Member that handler implements  */
        void* context;                                 /**<  Optional context provided when handler was registered */
        qcc::String ifaceStr;                          /**<  Interface string */
        qcc::String methodStr;                         /**<  Method string */
        mutable volatile int32_t refCount;
    };

    struct SafeEntry {
        SafeEntry() : entry(NULL) {   }

        ~SafeEntry()
        {
            if (NULL != entry) {
                qcc::DecrementAndFetch(&(entry->refCount));
            }
        }

        void Set(Entry* entry)
        {
            qcc::IncrementAndFetch(&(entry->refCount));
            this->entry = entry;
        }

        const Entry* entry;
    };

    /**
     * Destructor
     */
    ~MethodTable();

    /**
     * Add an entry to the method hash table.
     *
     * @param object     Object instance.
     * @param func       Handler for method.
     * @param member     Member that func implements.
     */
    void Add(BusObject* object,
             MessageReceiver::MethodHandler func,
             const InterfaceDescription::Member* member,
             void* context = NULL);

    /**
     * Find an Entry based on set of criteria.
     *
     * @param objectPath   The object path.
     * @param iface        The interface.
     * @param methodName   The method name.
     * @return
     *      - Entry that matches objectPath, interface and method
     *      - NULL if not found
     */
    SafeEntry* Find(const char* objectPath, const char* iface, const char* methodName);

    /**
     * Remove all hash entries related to the specified object.
     *
     * @param object   Object whose method table entries are to be removed.
     */
    void RemoveAll(BusObject* object);

    /**
     * Register handlers for an object's methods.
     *
     * @param object  Object whose methods are to be registered.
     */
    void AddAll(BusObject* object);

  private:

    qcc::Mutex lock; /**< Lock protecting the method table */

    /**
     * Type definition for method hash table key
     */
    class Key {
      public:
        const char* objPath;
        const char* iface;
        const char* methodName;
        Key(const char* obj, const char* ifc, const char* method) : objPath(obj), iface((ifc && *ifc) ? ifc : NULL), methodName(method) { }
    };

    /**
     * Hash functor
     */
    struct Hash {
        /** Calculate hash for Key k  */
        size_t operator()(const Key& k) const {
            size_t hash = 37;
            for (const char* p = k.methodName; *p; ++p) {
                hash = *p + hash * 11;
            }
            for (const char* p = k.objPath; *p; ++p) {
                hash = *p + hash * 5;
            }
            if (k.iface) {
                for (const char* p = k.iface; *p; ++p) {
                    hash += *p * 7;
                }
            }
            return hash;
        }
    };

    /**
     * Functor for testing 2 keys for equality
     */
    struct Equal {
        /**
         * Return true two keys are equal
         */
        bool operator()(const Key& k1, const Key& k2) const {
            if ((k1.iface == NULL) || (k2.iface == NULL)) {
                return (k1.iface == k2.iface) && (strcmp(k1.methodName, k2.methodName) == 0) && (strcmp(k1.objPath, k2.objPath) == 0);
            } else {
                return (strcmp(k1.methodName, k2.methodName) == 0) && (strcmp(k1.iface, k2.iface) == 0) && (strcmp(k1.objPath, k2.objPath) == 0);
            }
        }
    };

    /** The hash table */
    typedef std::unordered_map<Key, Entry*, Hash, Equal> MapType;
    MapType hashTable;
};

}

#endif
