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
#ifndef _SCRIPTABLEOBJECT_H
#define _SCRIPTABLEOBJECT_H

#include "Plugin.h"
#include <map>

class ScriptableObject {
  public:
    ScriptableObject(Plugin& plugin);
    ScriptableObject(Plugin& plugin, std::map<qcc::String, int32_t>& constants);
    virtual ~ScriptableObject();
    virtual void Invalidate();
    virtual bool HasMethod(const qcc::String& name);
    virtual bool Invoke(const qcc::String& name, const NPVariant* args, uint32_t argCount, NPVariant* result);
    virtual bool InvokeDefault(const NPVariant* args, uint32_t argCount, NPVariant* result);
    virtual bool HasProperty(const qcc::String& name);
    virtual bool GetProperty(const qcc::String& name, NPVariant* result);
    virtual bool SetProperty(const qcc::String& name, const NPVariant* value);
    virtual bool RemoveProperty(const qcc::String& name);
    virtual bool Enumerate(NPIdentifier** value, uint32_t* count);
    virtual bool Construct(const NPVariant* args, uint32_t argCount, NPVariant* result);

  protected:
    typedef bool (ScriptableObject::*Get)(NPVariant* result);
    typedef bool (ScriptableObject::*Set)(const NPVariant* value);
    class Attribute {
      public:
        Get get;
        Set set;
        Attribute(Get get, Set set = 0) : get(get), set(set) { }
        Attribute() : get(0), set(0) { }
    };
    typedef bool (ScriptableObject::*Call)(const NPVariant* args, uint32_t argCount, NPVariant* result);
    class Operation {
      public:
        Call call;
        Operation(Call call) : call(call) { }
        Operation() : call(0) { }
    };
    typedef bool (ScriptableObject::*Getter)(const qcc::String& name, NPVariant* result);
    typedef bool (ScriptableObject::*Setter)(const qcc::String& name, const NPVariant* value);
    typedef bool (ScriptableObject::*Deleter)(const qcc::String& name);
    typedef bool (ScriptableObject::*Enumerator)(NPIdentifier** value, uint32_t* count);
    typedef bool (ScriptableObject::*Caller)(const NPVariant* args, uint32_t argCount, NPVariant* result);
    Plugin plugin;
    std::map<qcc::String, Attribute> attributes;
    std::map<qcc::String, Operation> operations;
    Getter getter;
    Setter setter;
    Deleter deleter;
    Enumerator enumerator;
    Caller caller;

  private:
    static std::map<qcc::String, int32_t> noConstants;
    std::map<qcc::String, int32_t>& constants; /* Constants are shared between interface and host objects */
};

#define CONSTANT(name, value) constants[name] = (value)
#define ATTRIBUTE(name, get, set) attributes[name] = Attribute(static_cast<Get>(get), static_cast<Set>(set))
#define OPERATION(name, call) operations[name] = Operation(static_cast<Call>(call))
#define GETTER(customGetter) getter = static_cast<Getter>(customGetter)
#define SETTER(customSetter) setter = static_cast<Setter>(customSetter)
#define DELETER(customDeleter) deleter = static_cast<Deleter>(customDeleter)
#define ENUMERATOR(customEnumerator) enumerator = static_cast<Enumerator>(customEnumerator)
#define CALLER(customCaller) caller = static_cast<Caller>(customCaller)

#define REMOVE_CONSTANT(name) constants.erase(name)
#define REMOVE_ATTRIBUTE(name) attributes.erase(name)
#define REMOVE_OPERATION(name) operations.erase(name)

#endif // _SCRIPTABLEOBJECT_H
