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

#include "ObjectReference.h"
#include <AllJoynException.h>

namespace AllJoyn {

void AddObjectReference(qcc::Mutex* mtx, Platform::Object ^ key, std::map<void*, void*>* map)
{
    // Grab the mutex if specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    void* handle = (void*)key;
    // Check for object in map
    if (map->find(handle) == map->end()) {
        Platform::Object ^ oHandle = key;
        __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(oHandle);
        // Incremenet ref count
        pUnk->__abi_AddRef();
        // Store object in map
        (*map)[handle] = handle;
    }
    // Release the mutex if specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

void RemoveObjectReference(qcc::Mutex* mtx, Platform::Object ^ key, std::map<void*, void*>* map)
{
    // Grab the mutex if specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    void* handle = (void*)key;
    // Check for object in map
    if (map->find(handle) != map->end()) {
        Platform::Object ^ oHandle = key;
        __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(oHandle);
        // Decrement the ref count
        pUnk->__abi_Release();
        // Remove object in map
        map->erase(handle);
    }
    // Release the mutex if specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

void AddObjectReference2(qcc::Mutex* mtx, void* key, Platform::Object ^ val, std::map<void*, void*>* map)
{
    // Grab the mutex if specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    void* handle = (void*)key;
    // Check for object in map
    if (map->find(handle) == map->end()) {
        Platform::Object ^ oHandle = val;
        __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(oHandle);
        // Incremenet the ref count
        pUnk->__abi_AddRef();
        // Store object in map
        (*map)[handle] = (void*)oHandle;
    }
    // Release the mutex if specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

void RemoveObjectReference2(qcc::Mutex* mtx, void* key, std::map<void*, void*>* map)
{
    // Grab the mutex if specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    void* handle = (void*)key;
    // Check for object in map
    if (map->find(handle) != map->end()) {
        __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>((*map)[handle]);
        // Decrement the ref count
        pUnk->__abi_Release();
        // Remove object in map
        map->erase(handle);
    }
    // Release the mutex if specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

void ClearObjectMap(qcc::Mutex* mtx, std::map<void*, void*>* m)
{
    // Grab the lock if specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    // Iterate the map
    for (std::map<void*, void*, std::less<void*> >::const_iterator iter = m->begin();
         iter != m->end();
         ++iter) {
        __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(iter->second);
        // Decrement the ref count
        pUnk->__abi_Release();
    }
    // Clear the map
    m->clear();
    // Grab the lock if specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

void AddIdReference(qcc::Mutex* mtx, ajn::SessionPort key, Platform::Object ^ val, std::map<ajn::SessionId, std::map<void*, void*>*>* m)
{
    // Check val for invalid values
    if (val == nullptr) {
        return;
    }

    // Grab lock if mutex is specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    // Look up key in map
    if (m->find(key) == m->end()) {
        // Create a new map
        std::map<void*, void*>* lMap = new std::map<void*, void*>();
        if (NULL == lMap) {
            QCC_THROW_EXCEPTION(ER_OUT_OF_MEMORY);
        }
        Platform::Object ^ oHandle = val;
        // Store object in new map
        (*lMap)[(void*)oHandle] = (void*)oHandle;
        __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(oHandle);
        // Increment reference count
        pUnk->__abi_AddRef();
        // Store new map in map
        (*m)[key] = lMap;
    } else {
        // Key already exists
        Platform::Object ^ oHandle = val;
        // Get the map
        std::map<void*, void*>* lMap = (*m)[key];
        // Check the map for existing handle value
        if (lMap->find((void*)oHandle) == lMap->end()) {
            (*lMap)[(void*)oHandle] = (void*)oHandle;
            __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(oHandle);
            // Increment reference count
            pUnk->__abi_AddRef();
            // Store object in new map
            (*m)[key] = lMap;
        }
    }
    // Release the lock if mutex specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

void RemoveIdReference(qcc::Mutex* mtx, ajn::SessionPort key, std::map<ajn::SessionId, std::map<void*, void*>*>* m)
{
    // Grab the lock if mutex is specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    // Check if key exists in map
    if (m->find(key) != m->end()) {
        // Get the map
        std::map<void*, void*>* lMap = (*m)[key];
        // Iterate values in the map
        for (std::map<void*, void*, std::less<void*> >::const_iterator iter = lMap->begin();
             iter != lMap->end();
             ++iter) {
            __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(iter->second);
            // Decrement the ref count
            pUnk->__abi_Release();
        }
        // Delete the map by key
        m->erase(key);
        // Cleanup the released map
        lMap->clear();
        delete lMap;
        lMap = NULL;
    }
    // Release the lock if mutex is specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

void ClearIdMap(qcc::Mutex* mtx, std::map<ajn::SessionId, std::map<void*, void*>*>* m)
{
    // Grab the lock if the mutex is specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    // Iterate over the session id maps
    for (std::map<ajn::SessionId, std::map<void*, void*>*, std::less<ajn::SessionId> >::const_iterator iter = m->begin();
         iter != m->end();
         ++iter) {
        // Get the map value
        std::map<void*, void*>* lMap = iter->second;
        // Iterate over this map
        for (std::map<void*, void*, std::less<void*> >::const_iterator iter = lMap->begin();
             iter != lMap->end();
             ++iter) {
            __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(iter->second);
            // Decrement the ref count
            pUnk->__abi_Release();
        }
        // Cleanup the map
        lMap->clear();
        delete lMap;
        lMap = NULL;
    }
    // Clear the map
    m->clear();
    // Release the lock if the mutex is specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

void AddPortReference(qcc::Mutex* mtx, ajn::SessionPort key, Platform::Object ^ val, std::map<ajn::SessionPort, std::map<void*, void*>*>* m)
{
    // Grab the lock if mutex is specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    // Check if key exists in map
    if (m->find(key) == m->end()) {
        // Create a new map
        std::map<void*, void*>* lMap = new std::map<void*, void*>();
        // Check for allocation error
        if (NULL == lMap) {
            QCC_THROW_EXCEPTION(ER_OUT_OF_MEMORY);
        }
        Platform::Object ^ oHandle = val;
        // Store object in map
        (*lMap)[(void*)oHandle] = (void*)oHandle;
        __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(oHandle);
        // Increment the ref count
        pUnk->__abi_AddRef();
        // Store map
        (*m)[key] = lMap;
    } else {
        Platform::Object ^ oHandle = val;
        std::map<void*, void*>* lMap = (*m)[key];
        // See if object already exists in map
        if (lMap->find((void*)oHandle) == lMap->end()) {
            // Store object in map
            (*lMap)[(void*)oHandle] = (void*)oHandle;
            __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(oHandle);
            // Increment the ref count
            pUnk->__abi_AddRef();
            // Store the map
            (*m)[key] = lMap;
        }
    }
    // Release the lock if the mutex is specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

void RemovePortReference(qcc::Mutex* mtx, ajn::SessionPort key, std::map<ajn::SessionPort, std::map<void*, void*>*>* m)
{
    // Grab the lock if mutex is specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    // Check if key exists in map
    if (m->find(key) != m->end()) {
        // Get the map
        std::map<void*, void*>* lMap = (*m)[key];
        // Iterate over the map values
        for (std::map<void*, void*, std::less<void*> >::const_iterator iter = lMap->begin();
             iter != lMap->end();
             ++iter) {
            __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(iter->second);
            // Decrement the ref count
            pUnk->__abi_Release();
        }
        // Erase key from map
        m->erase(key);
        // Delete object map
        lMap->clear();
        delete lMap;
        lMap = NULL;
    }
    // Release the lock if the mutex is specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

void ClearPortMap(qcc::Mutex* mtx, std::map<ajn::SessionPort, std::map<void*, void*>*>* m)
{
    // Grab the lock if mutex is specified
    if (NULL != mtx) {
        mtx->Lock();
    }
    // Iterate over the port map
    for (std::map<ajn::SessionPort, std::map<void*, void*>*, std::less<ajn::SessionPort> >::const_iterator iter = m->begin();
         iter != m->end();
         ++iter) {
        // Get the object map
        std::map<void*, void*>* lMap = iter->second;
        // Iterate over object map
        for (std::map<void*, void*, std::less<void*> >::const_iterator iter = lMap->begin();
             iter != lMap->end();
             ++iter) {
            __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(iter->second);
            // Decrement the ref count
            pUnk->__abi_Release();
        }
        // Clear values from object map and cleanup
        lMap->clear();
        delete lMap;
        lMap = NULL;
    }
    // Clear the map
    m->clear();
    // Release the lock if mutex is specified
    if (NULL != mtx) {
        mtx->Unlock();
    }
}

uint32_t QueryReferenceCount(Platform::Object ^ obj)
{
    __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(obj);
    // Up/Down count the object to get ref count
    pUnk->__abi_AddRef();
    return pUnk->__abi_Release();
}

}
