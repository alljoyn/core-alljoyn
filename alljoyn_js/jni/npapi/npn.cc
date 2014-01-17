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
#include "npn.h"

#include "PluginData.h"
#include <alljoyn/Status.h>
#include <qcc/Debug.h>
#include <qcc/Log.h>
#include <assert.h>

#define QCC_MODULE "ALLJOYN_JS"

NPNetscapeFuncs* npn;
#if defined(QCC_OS_GROUP_WINDOWS)
HINSTANCE gHinstance;
#endif

void NPN_RetainVariantValue(const NPVariant*variant, NPVariant* retained)
{
    switch (variant->type) {
    case NPVariantType_String: {
            uint32_t UTF8Length = NPVARIANT_TO_STRING(*variant).UTF8Length;
            const NPUTF8* UTF8Characters = NPVARIANT_TO_STRING(*variant).UTF8Characters;
            char* chars = reinterpret_cast<char*>(NPN_MemAlloc(UTF8Length + 1));
            strncpy(chars, UTF8Characters, UTF8Length);
            chars[UTF8Length] = 0;
            STRINGN_TO_NPVARIANT(chars, UTF8Length, *retained);
            break;
        }

    case NPVariantType_Object: {
            *retained = *variant;
            NPN_RetainObject(NPVARIANT_TO_OBJECT(*retained));
            break;
        }

    default:
        *retained = *variant;
        break;
    }
}

void NPN_PluginThreadAsyncCall(NPP npp, void (*func)(void*), void* userData)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    npn->pluginthreadasynccall(npp, func, userData);
}

/*
 * The other NPN_ functions are defined here to provide an entrypoint for debugging during
 * development.
 */
qcc::Thread* gPluginThread = NULL;
#define ASSERT_MAIN_THREAD()                                            \
    do {                                                                \
        assert(gPluginThread == qcc::Thread::GetThread());              \
        if (gPluginThread != qcc::Thread::GetThread()) {                \
            QCC_LogError(ER_FAIL, ("NPN function called from external thread!")); \
        }                                                               \
    } while (0)
#define ASSERT_NPP()                                    \
    do {                                                \
        assert(npp);                                    \
        if (!npp) {                                     \
            QCC_LogError(ER_FAIL, ("Null npp!"));       \
        }                                               \
    } while (0)

NPObject* NPN_CreateObject(NPP npp, NPClass* aClass)
{
    ASSERT_MAIN_THREAD();
    ASSERT_NPP();
    return npn->createobject(npp, aClass);
}

bool NPN_Enumerate(NPP npp, NPObject* obj, NPIdentifier** identifier, uint32_t* count)
{
    ASSERT_MAIN_THREAD();
    ASSERT_NPP();
    return npn->enumerate(npp, obj, identifier, count);
}

bool NPN_Evaluate(NPP npp, NPObject* obj, NPString* script, NPVariant* result)
{
    ASSERT_MAIN_THREAD();
    ASSERT_NPP();
    return npn->evaluate(npp, obj, script, result);
}

NPIdentifier NPN_GetIntIdentifier(int32_t intid)
{
    ASSERT_MAIN_THREAD();
    return npn->getintidentifier(intid);
}

bool NPN_GetProperty(NPP npp, NPObject* obj, NPIdentifier propertyName, NPVariant* result)
{
    ASSERT_MAIN_THREAD();
    ASSERT_NPP();
    return npn->getproperty(npp, obj, propertyName, result);
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8* name)
{
    ASSERT_MAIN_THREAD();
    return npn->getstringidentifier(name);
}

NPError NPN_GetValue(NPP npp, NPNVariable variable, void* ret_value)
{
    ASSERT_MAIN_THREAD();
    return npn->getvalue(npp, variable, ret_value);
}

bool NPN_HasMethod(NPP npp, NPObject* obj, NPIdentifier propertyName)
{
    ASSERT_MAIN_THREAD();
    ASSERT_NPP();
    return npn->hasmethod(npp, obj, propertyName);
}

bool NPN_IdentifierIsString(NPIdentifier identifier)
{
    ASSERT_MAIN_THREAD();
    return npn->identifierisstring(identifier);
}

int32_t NPN_IntFromIdentifier(NPIdentifier identifier)
{
    ASSERT_MAIN_THREAD();
    return npn->intfromidentifier(identifier);
}

bool NPN_Invoke(NPP npp, NPObject* obj, NPIdentifier methodName, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    ASSERT_MAIN_THREAD();
    ASSERT_NPP();
    return npn->invoke(npp, obj, methodName, args, argCount, result);
}

bool NPN_InvokeDefault(NPP npp, NPObject* obj, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    ASSERT_MAIN_THREAD();
    ASSERT_NPP();
    return npn->invokeDefault(npp, obj, args, argCount, result);
}

void* NPN_MemAlloc(uint32_t size)
{
    ASSERT_MAIN_THREAD();
    return npn->memalloc(size);
}

void NPN_MemFree(void* ptr)
{
    ASSERT_MAIN_THREAD();
    return npn->memfree(ptr);
}

void NPN_ReleaseObject(NPObject* obj)
{
    ASSERT_MAIN_THREAD();
    return npn->releaseobject(obj);
}

void NPN_ReleaseVariantValue(NPVariant* variant)
{
    ASSERT_MAIN_THREAD();
    return npn->releasevariantvalue(variant);
}

NPObject* NPN_RetainObject(NPObject* obj)
{
    ASSERT_MAIN_THREAD();
    return npn->retainobject(obj);
}

void NPN_SetException(NPObject* obj, const NPUTF8* message)
{
    ASSERT_MAIN_THREAD();
    return npn->setexception(obj, message);
}

bool NPN_SetProperty(NPP npp, NPObject* obj, NPIdentifier propertyName, const NPVariant* value)
{
    ASSERT_MAIN_THREAD();
    ASSERT_NPP();
    return npn->setproperty(npp, obj, propertyName, value);
}

NPUTF8* NPN_UTF8FromIdentifier(NPIdentifier identifier)
{
    ASSERT_MAIN_THREAD();
    return npn->utf8fromidentifier(identifier);
}

NPError NPN_GetURLNotify(NPP npp, const char* url, const char* target, void* notifyData)
{
    ASSERT_MAIN_THREAD();
    ASSERT_NPP();
    return npn->geturlnotify(npp, url, target, notifyData);
}

const char* NPN_UserAgent(NPP npp)
{
    ASSERT_MAIN_THREAD();
    ASSERT_NPP();
    return npn->uagent(npp);
}
