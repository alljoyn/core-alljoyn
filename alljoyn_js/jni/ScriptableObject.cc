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
#include "ScriptableObject.h"

#include "NativeObject.h"
#include "npn.h"
#include <assert.h>
#include <qcc/Debug.h>
#include <string.h>

#define QCC_MODULE "ALLJOYN_JS"

#define CALL_MEMBER(o, m) ((*o).*(m))

std::map<qcc::String, int32_t> ScriptableObject::noConstants;

ScriptableObject::ScriptableObject(Plugin& plugin) :
    plugin(plugin),
    getter(0),
    setter(0),
    deleter(0),
    enumerator(0),
    caller(0),
    constants(noConstants)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

ScriptableObject::ScriptableObject(Plugin& plugin, std::map<qcc::String, int32_t>& constants) :
    plugin(plugin),
    getter(0),
    setter(0),
    deleter(0),
    enumerator(0),
    caller(0),
    constants(constants)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

ScriptableObject::~ScriptableObject()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void ScriptableObject::Invalidate()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool ScriptableObject::HasMethod(const qcc::String& name)
{
    QCC_DbgTrace(("%s(name=%s)", __FUNCTION__, name.c_str()));
    std::map<qcc::String, Operation>::iterator it = operations.find(name);
    return (it != operations.end());
}

bool ScriptableObject::Invoke(const qcc::String& name, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s(name=%s)", __FUNCTION__, name.c_str()));
    std::map<qcc::String, Operation>::iterator it = operations.find(name);
    if (it != operations.end()) {
        assert(it->second.call);
        return CALL_MEMBER(this, it->second.call) (args, argCount, result);
    }
    return false;
}

bool ScriptableObject::InvokeDefault(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (caller) {
        return CALL_MEMBER(this, caller) (args, argCount, result);
    }
    return false;
}

bool ScriptableObject::HasProperty(const qcc::String& name)
{
    QCC_DbgTrace(("%s(name=%s)", __FUNCTION__, name.c_str()));
    std::map<qcc::String, int32_t>::iterator cit = constants.find(name);
    if (cit != constants.end()) {
        return true;
    }
    std::map<qcc::String, Attribute>::iterator ait = attributes.find(name);
    return (ait != attributes.end());
}

bool ScriptableObject::GetProperty(const qcc::String& name, NPVariant* result)
{
    QCC_DbgTrace(("%s(name=%s)", __FUNCTION__, name.c_str()));
    std::map<qcc::String, int32_t>::iterator cit = constants.find(name);
    if (cit != constants.end()) {
        INT32_TO_NPVARIANT(cit->second, *result);
        return true;
    }
    std::map<qcc::String, Attribute>::iterator ait = attributes.find(name);
    if (ait != attributes.end()) {
        assert(ait->second.get);
        return CALL_MEMBER(this, ait->second.get) (result);
    }
    if (getter) {
        return CALL_MEMBER(this, getter) (name, result);
    }
    return false;
}

bool ScriptableObject::SetProperty(const qcc::String& name, const NPVariant* value)
{
    QCC_DbgTrace(("%s(name=%s)", __FUNCTION__, name.c_str()));
    /*
     * Workaround for WebKit browsers.  "delete obj.property" doesn't call RemoveProperty, so allow
     * "obj.property = undefined" to do the same thing.
     */
    if (NPVARIANT_IS_VOID(*value)) {
        return RemoveProperty(name);
    }

    std::map<qcc::String, Attribute>::iterator it = attributes.find(name);
    if ((it != attributes.end()) && it->second.set) {
        return CALL_MEMBER(this, it->second.set) (value);
    }
    if (setter) {
        return CALL_MEMBER(this, setter) (name, value);
    }
    return false;
}

bool ScriptableObject::RemoveProperty(const qcc::String& name)
{
    QCC_DbgTrace(("%s(name=%s)", __FUNCTION__, name.c_str()));
    if (deleter) {
        return CALL_MEMBER(this, deleter) (name);
    }
    return false;
}

bool ScriptableObject::Enumerate(NPIdentifier** value, uint32_t* count)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    *value = NULL;
    *count = 0;

    NPIdentifier* enumeratorValue = NULL;
    uint32_t enumeratorCount = 0;
    if (enumerator) {
        CALL_MEMBER(this, enumerator) (&enumeratorValue, &enumeratorCount);
    }
    *count = enumeratorCount + constants.size() + attributes.size() + operations.size();
    if (*count) {
        *value = reinterpret_cast<NPIdentifier*>(NPN_MemAlloc(*count * sizeof(NPIdentifier)));
        NPIdentifier* v = *value;
        for (uint32_t i = 0; i < enumeratorCount; ++i) {
            *v++ = enumeratorValue[i];
        }
        if (enumeratorValue) {
            NPN_MemFree(enumeratorValue);
        }
        for (std::map<qcc::String, int32_t>::iterator it = constants.begin(); it != constants.end(); ++it) {
            *v++ = NPN_GetStringIdentifier(it->first.c_str());
        }
        for (std::map<qcc::String, Attribute>::iterator it = attributes.begin(); it != attributes.end(); ++it) {
            *v++ = NPN_GetStringIdentifier(it->first.c_str());
        }
        for (std::map<qcc::String, Operation>::iterator it = operations.begin(); it != operations.end(); ++it) {
            *v++ = NPN_GetStringIdentifier(it->first.c_str());
        }
    }
    return true;
}

bool ScriptableObject::Construct(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return false;
}
