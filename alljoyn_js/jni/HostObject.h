/*
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
#ifndef _HOSTOBJECT_H
#define _HOSTOBJECT_H

#include "Plugin.h"
#include "PluginData.h"
#include <qcc/Debug.h>
#include <assert.h>

template <class T>
class HostObject : public NPObject {
  public:
    static HostObject<T>* GetInstance(Plugin& plugin, T& impl);
    static T* GetImpl(Plugin& plugin, NPObject* npobj);
    static NPClass Class;

  private:
    static NPObject* Allocate(NPP npp, NPClass* aClass);
    static void Deallocate(NPObject* npobj);
    static void Invalidate(NPObject* npobj);
    static bool HasMethod(NPObject* npobj, NPIdentifier name);
    static bool Invoke(NPObject* npobj, NPIdentifier name, const NPVariant* args, uint32_t argCount, NPVariant* result);
    static bool InvokeDefault(NPObject* npobj, const NPVariant* args, uint32_t argCount, NPVariant* result);
    static bool HasProperty(NPObject* npobj, NPIdentifier name);
    static bool GetProperty(NPObject* npobj, NPIdentifier name, NPVariant* result);
    static bool SetProperty(NPObject* npobj, NPIdentifier name, const NPVariant* value);
    static bool RemoveProperty(NPObject* npobj, NPIdentifier name);
    static bool Enumerate(NPObject* npobj, NPIdentifier** value, uint32_t* count);
    static bool Construct(NPObject* npobj, const NPVariant* args, uint32_t argCount, NPVariant* result);

    HostObject(Plugin& plugin, T& impl);
    ~HostObject();
    Plugin plugin;
    T impl;
};

#define QCC_MODULE "ALLJOYN_JS"

template <class T>
NPClass HostObject<T>::Class = {
    NP_CLASS_STRUCT_VERSION_CTOR,
    HostObject<T>::Allocate,
    HostObject<T>::Deallocate,
    HostObject<T>::Invalidate,
    HostObject<T>::HasMethod,
    HostObject<T>::Invoke,
    HostObject<T>::InvokeDefault,
    HostObject<T>::HasProperty,
    HostObject<T>::GetProperty,
    HostObject<T>::SetProperty,
    HostObject<T>::RemoveProperty,
    HostObject<T>::Enumerate,
    HostObject<T>::Construct,
};

template <class T>
HostObject<T>* HostObject<T>::GetInstance(Plugin& plugin, T& impl)
{
    ScriptableObject* obj = static_cast<ScriptableObject*>(&(*impl));
    std::map<ScriptableObject*, NPObject*>::iterator it = plugin->hostObjects.find(obj);
    NPObject* npobj;
    if (it != plugin->hostObjects.end()) {
        npobj = NPN_RetainObject(it->second);
        QCC_DbgTrace(("%s returning cached object %p", __FUNCTION__, npobj));
    } else {
        plugin->params = &impl;
        npobj = NPN_CreateObject(plugin->npp, &HostObject<T>::Class);
        QCC_DbgTrace(("%s returning new object %p", __FUNCTION__, npobj));
    }
    return static_cast<HostObject<T>*>(npobj);
}

template <class T>
T* HostObject<T>::GetImpl(Plugin& plugin, NPObject* npobj)
{
    assert(&HostObject<T>::Class == npobj->_class);
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    return &obj->impl;
}

template <class T>
NPObject* HostObject<T>::Allocate(NPP npp, NPClass* aClass)
{
    QCC_DbgTrace(("%s(npp=%p,aClass=%p)", __FUNCTION__, npp, aClass));
    PluginData* pluginData = reinterpret_cast<PluginData*>(npp->pdata);
    Plugin& plugin = pluginData->GetPlugin();
    T* impl = reinterpret_cast<T*>(plugin->params);
    NPObject* npobj = static_cast<NPObject*>(new HostObject<T>(plugin, *impl));
    plugin->hostObjects[static_cast<ScriptableObject*>(&(**impl))] = npobj;
    QCC_DbgTrace(("%s npobj=%p", __FUNCTION__, npobj));
    PluginData::InsertNPObject(npobj);
    return npobj;
}

template <class T>
void HostObject<T>::Deallocate(NPObject* npobj)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    obj->plugin->hostObjects.erase(&(*obj->impl));
    PluginData::RemoveNPObject(npobj);
    delete obj;
}

template <class T>
void HostObject<T>::Invalidate(NPObject* npobj)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    obj->impl->Invalidate();
}

template <class T>
bool HostObject<T>::HasMethod(NPObject* npobj, NPIdentifier name)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    if (!NPN_IdentifierIsString(name)) {
        QCC_LogError(ER_FAIL, ("HasMethod called with int identifier"));
        return false;
    }
    NPUTF8* nm = NPN_UTF8FromIdentifier(name);
    if (!nm) {
        QCC_LogError(ER_OUT_OF_MEMORY, ("NPN_UTF8FromIdentifier failed"));
        return false;
    }
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    bool ret = obj->impl->HasMethod(nm);
    NPN_MemFree(nm);
    obj->plugin->CheckError(obj);
    return ret;
}

template <class T>
bool HostObject<T>::Invoke(NPObject* npobj, NPIdentifier name, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    if (!NPN_IdentifierIsString(name)) {
        QCC_LogError(ER_FAIL, ("HasMethod called with int identifier"));
        return false;
    }
    NPUTF8* nm = NPN_UTF8FromIdentifier(name);
    if (!nm) {
        QCC_LogError(ER_OUT_OF_MEMORY, ("NPN_UTF8FromIdentifier failed"));
        return false;
    }
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    bool ret = obj->impl->Invoke(nm, args, argCount, result);
    NPN_MemFree(nm);
    obj->plugin->CheckError(obj);
    return ret;
}

template <class T>
bool HostObject<T>::InvokeDefault(NPObject* npobj, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    bool ret = obj->impl->InvokeDefault(args, argCount, result);
    obj->plugin->CheckError(obj);
    return ret;
}

template <class T>
bool HostObject<T>::HasProperty(NPObject* npobj, NPIdentifier name)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    if (!NPN_IdentifierIsString(name)) {
        QCC_LogError(ER_FAIL, ("HasMethod called with int identifier"));
        return false;
    }
    NPUTF8* nm = NPN_UTF8FromIdentifier(name);
    if (!nm) {
        QCC_LogError(ER_OUT_OF_MEMORY, ("NPN_UTF8FromIdentifier failed"));
        return false;
    }
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    bool ret = obj->impl->HasProperty(nm);
    NPN_MemFree(nm);
    obj->plugin->CheckError(obj);
    return ret;
}

template <class T>
bool HostObject<T>::GetProperty(NPObject* npobj, NPIdentifier name, NPVariant* result)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    if (!NPN_IdentifierIsString(name)) {
        QCC_LogError(ER_FAIL, ("HasMethod called with int identifier"));
        return false;
    }
    NPUTF8* nm = NPN_UTF8FromIdentifier(name);
    if (!nm) {
        QCC_LogError(ER_OUT_OF_MEMORY, ("NPN_UTF8FromIdentifier failed"));
        return false;
    }
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    bool ret = obj->impl->GetProperty(nm, result);
    NPN_MemFree(nm);
    obj->plugin->CheckError(obj);
    return ret;
}

template <class T>
bool HostObject<T>::SetProperty(NPObject* npobj, NPIdentifier name, const NPVariant* value)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    if (!NPN_IdentifierIsString(name)) {
        QCC_LogError(ER_FAIL, ("HasMethod called with int identifier"));
        return false;
    }
    NPUTF8* nm = NPN_UTF8FromIdentifier(name);
    if (!nm) {
        QCC_LogError(ER_OUT_OF_MEMORY, ("NPN_UTF8FromIdentifier failed"));
        return false;
    }
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    bool ret = obj->impl->SetProperty(nm, value);
    NPN_MemFree(nm);
    obj->plugin->CheckError(obj);
    return ret;
}

template <class T>
bool HostObject<T>::RemoveProperty(NPObject* npobj, NPIdentifier name)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    if (!NPN_IdentifierIsString(name)) {
        QCC_LogError(ER_FAIL, ("HasMethod called with int identifier"));
        return false;
    }
    NPUTF8* nm = NPN_UTF8FromIdentifier(name);
    if (!nm) {
        QCC_LogError(ER_OUT_OF_MEMORY, ("NPN_UTF8FromIdentifier failed"));
        return false;
    }
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    bool ret = obj->impl->RemoveProperty(nm);
    NPN_MemFree(nm);
    obj->plugin->CheckError(obj);
    return ret;
}

template <class T>
bool HostObject<T>::Enumerate(NPObject* npobj, NPIdentifier** value, uint32_t* count)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    bool ret = obj->impl->Enumerate(value, count);
    obj->plugin->CheckError(obj);
    return ret;
}

template <class T>
bool HostObject<T>::Construct(NPObject* npobj, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s(npobj=%p)", __FUNCTION__, npobj));
    HostObject<T>* obj = static_cast<HostObject<T>*>(npobj);
    bool ret = obj->impl->Construct(args, argCount, result);
    obj->plugin->CheckError(obj);
    return ret;
}

template <class T> HostObject<T>::HostObject(Plugin& plugin, T& impl) :
    plugin(plugin),
    impl(impl)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

template <class T> HostObject<T>::~HostObject()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

#undef QCC_MODULE

#endif // _HOSTOBJECT_H
