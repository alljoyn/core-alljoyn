/*
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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
 */
#ifndef _PLUGIN_H
#define _PLUGIN_H

#include "npn.h"
#include <alljoyn/Status.h>
#include <qcc/ManagedObj.h>
#include <qcc/String.h>
#include <list>
#include <map>
class NativeObject;
class ScriptableObject;

/**
 * Per-instance plugin data (controlled via NPP_New and NPP_Destroy).
 */
class _Plugin {
  public:
    /**
     * Plugin handle.  This will be 0 after the NPP_Destroy is called.
     */
    NPP npp;
    /**
     * HostObject constructor params (an impl of T).
     *
     * The runtime calls Allocate of the host object with two params, this plugin and an NPClass.
     * So the only way to pass params to the HostObject constructor is via this plugin or NPClass.
     * Adding the params to NPClass would make it global across all instances of the plugin which
     * could lead to race conditions.  Adding the params to this plugin makes it per-instance and
     * thread-safe.
     */
    void* params;
    /**
     * Cache of allocated HostObjects, keyed off the impl object.
     *
     * This means that as long as the runtime has not called Deallocate of a HostObject, then
     * HostObject::GetInstance() will just retain cached object and return it.
     */
    std::map<ScriptableObject*, NPObject*> hostObjects;
    /**
     * Cache of retained NPObjects, keyed off the NativeObject wrapper.
     *
     * This is necessary as Firefox will delete native *retained* objects after calling
     * NPP_Destroy.  This at least gives me a chance to null out the pointers when the plugin is
     * destroyed and not reference freed memory.
     */
    std::map<NativeObject*, NPObject*> nativeObjects;

    QStatus Initialize();

    /**
     * Compare two values for equality using the native '===' operator.
     *
     * This exists so that two native objects can be compared for equality.  The pointers cannot
     * be compared since that does not work in chrome, so add a native function to the plugin
     * element to do the comparison for us (which does work in all the browsers).
     */
    bool StrictEquals(const NPVariant& a, const NPVariant& b) const;

    /**
     * Return the security origin of this plugin instance.
     *
     * @param[out] origin a string of the form "<protocol>://<hostname>:<port>"
     */
    QStatus Origin(qcc::String& origin);
    /**
     * The characters (minus the quotes) "$-_.+!*'(),;/?:@=&" may appear unencoded in a URL.
     * Depending on the filesystem, these may not work for filenames, so encode all of them.
     *
     * For the curious, the intersection of unencoded characters and Windows is "/:*?".  On Linux,
     * it is "/".
     *
     * @param url the URL to encode as a filename
     *
     * @return the encoded URL
     */
    qcc::String ToFilename(const qcc::String& url);

    qcc::String KeyStoreFileName() { return qcc::String(); }

    bool RaiseBusError(QStatus code, const char* message = NULL);
    bool RaiseTypeError(const char* message);

    /*
     * Used only by HostObject and BusErrorInterface.  Real support for throwing Error objects is
     * missing from NPAPI.
     */
    class Error {
      public:
        /* Error fields */
        qcc::String name;
        qcc::String message;
        /* BusError fields */
        QStatus code;
        Error() : code(ER_NONE) { }
        void Clear() {
            name.clear();
            message.clear();
            code = ER_NONE;
        }
    };
    Error error;
    void CheckError(NPObject* npobj);

    _Plugin(NPP npp);
    _Plugin();
    ~_Plugin();

  private:
    Error _error;
};

typedef qcc::ManagedObj<_Plugin> Plugin;

#endif
