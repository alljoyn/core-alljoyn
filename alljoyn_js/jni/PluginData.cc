/*
 * Copyright (c) 2011-2014, AllSeen Alliance. All rights reserved.
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
#include "PluginData.h"

#include "BusNamespace.h"
#include "HostObject.h"
#include "NativeObject.h"
#include <assert.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

static void _DestroyOnMainThread(PluginData::CallbackContext*) {
}

PluginData::_Callback::_Callback(Plugin& plugin, void(*callback)(CallbackContext*)) :
    callback(callback),
    context(NULL),
    plugin(plugin),
    key(0)
{
}

PluginData::_Callback::_Callback() :
    callback(NULL),
    context(NULL),
    key(0)
{
}

PluginData::_Callback::~_Callback()
{
    if (gPluginThread == qcc::Thread::GetThread()) {
        delete context;
    } else {
        PluginData::DestroyOnMainThread(plugin, context);
    }
}

void PluginData::_Callback::SetEvent()
{
    if (!context->event.IsSet()) {
        QStatus setStatus = context->event.SetEvent();
        setStatus = setStatus; /* Fix compiler warning in release builds. */
        assert(ER_OK == setStatus);
        if (ER_OK != setStatus) {
            QCC_LogError(setStatus, ("SetEvent failed"));
        }
    }
}

std::list<PluginData::Callback> PluginData::pendingCallbacks;
uintptr_t PluginData::nextPendingCallbackKey = 1;
qcc::Mutex PluginData::lock;

void PluginData::DispatchCallback(PluginData::Callback& callback)
{
    if (callback->plugin->npp) {
        lock.Lock();
        callback->key = nextPendingCallbackKey;
        if (++nextPendingCallbackKey == 0) {
            ++nextPendingCallbackKey;
        }
        pendingCallbacks.push_back(callback);
        lock.Unlock();
        NPN_PluginThreadAsyncCall(callback->plugin->npp, PluginData::AsyncCall, (void*)callback->key);
    }
}

void PluginData::DestroyOnMainThread(Plugin& plugin, PluginData::CallbackContext* context)
{
    lock.Lock();
    if (plugin->npp) {
        PluginData::Callback callback(plugin, _DestroyOnMainThread);
        callback->context = context;
        callback->key = nextPendingCallbackKey;
        if (++nextPendingCallbackKey == 0) {
            ++nextPendingCallbackKey;
        }
        pendingCallbacks.push_back(callback);
        lock.Unlock();
        NPN_PluginThreadAsyncCall(callback->plugin->npp, PluginData::AsyncCall, (void*)callback->key);
    } else {
        lock.Unlock();
        /*
         * It is not safe to release NPAPI resources from outside the main thread.  So this
         * could lead to a crash if called, or a memory leak if not called.  Prefer the memory
         * leak.
         */
        QCC_LogError(ER_WARNING, ("Leaking callback context"));
    }
}

void PluginData::CancelCallback(PluginData::Callback& callback)
{
    lock.Lock();
    if (callback->plugin->npp) {
        for (std::list<Callback>::iterator it = pendingCallbacks.begin(); it != pendingCallbacks.end(); ++it) {
            if (((*it)->plugin->npp == callback->plugin->npp) && ((*it)->callback == callback->callback) && ((*it)->context == callback->context)) {
                pendingCallbacks.erase(it);
                break;
            }
        }
    }
    lock.Unlock();
}

void PluginData::AsyncCall(void* key)
{
    Callback callback;
    lock.Lock();
    for (std::list<Callback>::iterator it = pendingCallbacks.begin(); it != pendingCallbacks.end(); ++it) {
        if ((*it)->key == (uintptr_t)key) {
            callback = *it;
            pendingCallbacks.erase(it);
            break;
        }
    }
    lock.Unlock();
    if (callback->callback) {
        callback->callback(callback->context);
        callback->SetEvent();
    }
}

std::list<NPObject*> PluginData::npobjects;

void PluginData::InsertNPObject(NPObject* npobj)
{
#if defined(NDEBUG)
    lock.Lock();
    npobjects.push_back(npobj);
    lock.Unlock();
#endif
}

void PluginData::RemoveNPObject(NPObject* npobj)
{
#if defined(NDEBUG)
    lock.Lock();
    npobjects.remove(npobj);
    lock.Unlock();
#endif
}

void PluginData::DumpNPObjects()
{
#if defined(NDEBUG)
    lock.Lock();
    std::list<NPObject*>::iterator it = npobjects.begin();
    if (it != npobjects.end()) {
        QCC_DbgHLPrintf(("Orphaned NPObjects"));
    }
    for (; it != npobjects.end(); ++it) {
        QCC_DbgHLPrintf(("%p", *it));
    }
    lock.Unlock();
#endif
}

PluginData::PluginData(Plugin& plugin) :
    plugin(plugin),
    busNamespace(plugin)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

PluginData::~PluginData()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    lock.Lock();
    plugin->npp = 0;
    /*
     * Clear out native object cache as Firefox will delete these regardless of the reference count
     * when destroying the plugin.
     */
    for (std::map<NativeObject*, NPObject*>::iterator it = plugin->nativeObjects.begin(); it != plugin->nativeObjects.end(); ++it) {
        if (it->second) {
            it->second = 0;
            it->first->Invalidate();
        }
    }

    for (std::list<Callback>::iterator it = pendingCallbacks.begin(); it != pendingCallbacks.end();) {
        if (plugin.iden((*it)->plugin)) {
            Callback callback = *it;
            callback->SetEvent();
            pendingCallbacks.erase(it);
            /*
             * Can't make any assumptions about the contents of pendingCallbacks after the above
             * call, so reset the iterator.
             */
            it = pendingCallbacks.begin();
        } else {
            ++it;
        }
    }
    lock.Unlock();
}

NPObject* PluginData::GetScriptableObject()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return HostObject<BusNamespace>::GetInstance(plugin, busNamespace);
}

Plugin& PluginData::GetPlugin()
{
    return plugin;
}

void PluginData::InitializeStaticData()
{
    permissionLevels.clear();
}
