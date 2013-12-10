/******************************************************************************
 *
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
 *
 *****************************************************************************/

#pragma once

#include <alljoyn/Session.h>
#include <qcc/Mutex.h>
#include <map>

namespace AllJoyn {

inline void AddObjectReference(Platform::Object ^ obj)
{
    __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(obj);
    pUnk->__abi_AddRef();
}

inline void RemoveObjectReference(Platform::Object ^ obj)
{
    __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(obj);
    pUnk->__abi_Release();
}

void AddObjectReference(qcc::Mutex * mtx, Platform::Object ^ key, std::map<void*, void*>* map);

void RemoveObjectReference(qcc::Mutex * mtx, Platform::Object ^ key, std::map<void*, void*>* map);

void AddObjectReference2(qcc::Mutex * mtx, void* key, Platform::Object ^ val, std::map<void*, void*>* map);

void RemoveObjectReference2(qcc::Mutex* mtx, void* key, std::map<void*, void*>* map);

void ClearObjectMap(qcc::Mutex* mtx, std::map<void*, void*>* m);

void AddIdReference(qcc::Mutex * mtx, ajn::SessionPort key, Platform::Object ^ val, std::map<ajn::SessionId, std::map<void*, void*>*>* m);

void RemoveIdReference(qcc::Mutex* mtx, ajn::SessionPort key, std::map<ajn::SessionId, std::map<void*, void*>*>* m);

void ClearIdMap(qcc::Mutex* mtx, std::map<ajn::SessionId, std::map<void*, void*>*>* m);

void AddPortReference(qcc::Mutex * mtx, ajn::SessionPort key, Platform::Object ^ val, std::map<ajn::SessionPort, std::map<void*, void*>*>* m);

void RemovePortReference(qcc::Mutex* mtx, ajn::SessionPort key, std::map<ajn::SessionPort, std::map<void*, void*>*>* m);

void ClearPortMap(qcc::Mutex* mtx, std::map<ajn::SessionPort, std::map<void*, void*>*>* m);

uint32_t QueryReferenceCount(Platform::Object ^ obj);

}
// ObjectReference.h
