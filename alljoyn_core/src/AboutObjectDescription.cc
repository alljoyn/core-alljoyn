/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/AboutObjectDescription.h>

#include <qcc/Mutex.h>

#include <map>
#include <algorithm>

#include "AboutObjectDescriptionInternal.h"

namespace ajn {

AboutObjectDescription::AboutObjectDescription() : aodInternal(new AboutObjectDescription::Internal)
{
    //Empty constructor
}

AboutObjectDescription::AboutObjectDescription(const MsgArg& arg) : aodInternal(new AboutObjectDescription::Internal)
{
    CreateFromMsgArg(arg);
}

AboutObjectDescription::AboutObjectDescription(const AboutObjectDescription& src)
{
    aodInternal = new AboutObjectDescription::Internal();
    *aodInternal = *(src.aodInternal);
}

AboutObjectDescription& AboutObjectDescription::operator=(const AboutObjectDescription& src) {
    if (&src == this) {
        return *this;
    }
    delete aodInternal;
    aodInternal = NULL;
    aodInternal = new AboutObjectDescription::Internal();
    *aodInternal = *(src.aodInternal);
    return *this;
}

AboutObjectDescription::~AboutObjectDescription()
{
    delete aodInternal;
    aodInternal = NULL;
}

QStatus AboutObjectDescription::CreateFromMsgArg(const MsgArg& arg)
{
    QStatus status = ER_OK;
    MsgArg* structarg;
    size_t struct_size;
    status = arg.Get("a(oas)", &struct_size, &structarg);
    if (status != ER_OK) {
        return status;
    }

    for (size_t i = 0; i < struct_size; ++i) {
        char* objectPath;
        size_t numberItfs;
        MsgArg* interfacesArg;
        status = structarg[i].Get("(oas)", &objectPath, &numberItfs, &interfacesArg);
        if (status != ER_OK) {
            return status;
        }
        std::set<qcc::String> interfaceNames;
        for (size_t j = 0; j < numberItfs; ++j) {
            char* intfName;
            status = interfacesArg[j].Get("s", &intfName);
            if (status != ER_OK) {
                return status;
            }
            status = Add(objectPath, intfName);
            if (status != ER_OK) {
                return status;
            }
        }
    }

    return status;
}

QStatus AboutObjectDescription::Add(const char* path, const char* interfaceName)
{
    QStatus status = ER_OK;
    aodInternal->announceObjectsMapLock.Lock(MUTEX_CONTEXT);
    if (aodInternal->announceObjectsMap.find(path) == aodInternal->announceObjectsMap.end()) {
        std::set<qcc::String> newInterfaceName;
        newInterfaceName.insert(interfaceName);
        aodInternal->announceObjectsMap[path] = newInterfaceName;
    } else {
        aodInternal->announceObjectsMap[path].insert(interfaceName);
    }
    aodInternal->announceObjectsMapLock.Unlock(MUTEX_CONTEXT);
    return status;
}

size_t AboutObjectDescription::GetPaths(const char** paths, size_t numPaths) const
{
    if (paths == NULL) {
        return aodInternal->announceObjectsMap.size();
    }
    aodInternal->announceObjectsMapLock.Lock(MUTEX_CONTEXT);
    size_t count = 0;
    for (std::map<qcc::String, std::set<qcc::String> >::const_iterator it = aodInternal->announceObjectsMap.begin();
         it != aodInternal->announceObjectsMap.end() && count < numPaths; ++it, ++count) {
        paths[count] = it->first.c_str();
    }
    aodInternal->announceObjectsMapLock.Unlock(MUTEX_CONTEXT);
    return aodInternal->announceObjectsMap.size();
}

size_t AboutObjectDescription::GetInterfaces(const char* path, const char** interfaces, size_t numInterfaces) const
{
    std::map<qcc::String, std::set<qcc::String> >::const_iterator aom_it = aodInternal->announceObjectsMap.find(path);
    if (aom_it == aodInternal->announceObjectsMap.end()) {
        return static_cast<size_t>(0);
    }

    if (interfaces == NULL) {
        return aom_it->second.size();
    }
    aodInternal->announceObjectsMapLock.Lock(MUTEX_CONTEXT);
    size_t count = 0;
    for (std::set<qcc::String>::const_iterator it = aom_it->second.begin();
         it != aom_it->second.end() && count < numInterfaces; ++it, ++count) {
        interfaces[count] = it->c_str();
    }
    aodInternal->announceObjectsMapLock.Unlock(MUTEX_CONTEXT);
    return aom_it->second.size();
}

size_t AboutObjectDescription::GetInterfacePaths(const char* iface, const char** paths, size_t numPaths) const
{
    std::map<qcc::String, std::set<qcc::String> >::const_iterator it;
    size_t count = 0;
    aodInternal->announceObjectsMapLock.Lock(MUTEX_CONTEXT);
    for (it = aodInternal->announceObjectsMap.begin(); it != aodInternal->announceObjectsMap.end(); ++it) {
        std::set<qcc::String>::const_iterator it2 = it->second.find(iface);
        if (it2 != it->second.end()) {
            if (count < numPaths) {
                paths[count] = it->first.c_str();
            }
            ++count;
        }
    }
    aodInternal->announceObjectsMapLock.Unlock(MUTEX_CONTEXT);
    return count;
}

void AboutObjectDescription::Clear()
{
    aodInternal->announceObjectsMapLock.Lock(MUTEX_CONTEXT);
    aodInternal->announceObjectsMap.clear();
    aodInternal->announceObjectsMapLock.Unlock(MUTEX_CONTEXT);
}
bool AboutObjectDescription::HasPath(const char* path)  const
{
    std::map<qcc::String, std::set<qcc::String> >::const_iterator find_it = aodInternal->announceObjectsMap.find(path);
    std::map<qcc::String, std::set<qcc::String> >::const_iterator end_it = aodInternal->announceObjectsMap.end();
    return (find_it != end_it);
}

bool AboutObjectDescription::HasInterface(const char* interfaceName) const
{
    std::map<qcc::String, std::set<qcc::String> >::const_iterator it;
    for (it = aodInternal->announceObjectsMap.begin(); it != aodInternal->announceObjectsMap.end(); ++it) {
        if (HasInterface(it->first.c_str(), interfaceName)) {
            return true;
        }
    }

    return false;
}

bool AboutObjectDescription::HasInterface(const char* path, const char* interfaceName) const
{
    std::map<qcc::String, std::set<qcc::String> >::const_iterator it = aodInternal->announceObjectsMap.find(path);
    if (it == aodInternal->announceObjectsMap.end()) {
        return false;
    }
    qcc::String interfaceNameStr(interfaceName);
    size_t n = interfaceNameStr.find_first_of('*');
    std::set<qcc::String>::const_iterator ifac_it = it->second.begin();
    for (ifac_it = it->second.begin(); ifac_it != it->second.end(); ++ifac_it) {
        if (n == qcc::String::npos && interfaceNameStr == *ifac_it) {
            return true;
        } else if (n != qcc::String::npos && interfaceNameStr.compare(0, n, ifac_it->substr(0, n)) == 0) {
            return true;
        }
    }
    return false;
}

QStatus AboutObjectDescription::GetMsgArg(MsgArg* msgArg)
{
    QStatus status = ER_OK;
    MsgArg* announceObjectsArg = new MsgArg[aodInternal->announceObjectsMap.size()];
    int objIndex = 0;
    for (std::map<qcc::String, std::set<qcc::String> >::const_iterator it = aodInternal->announceObjectsMap.begin();
         it != aodInternal->announceObjectsMap.end(); ++it) {

        qcc::String objectPath = it->first;
        const char** interfaces = new const char*[it->second.size()];
        std::set<qcc::String>::const_iterator interfaceIt;
        int interfaceIndex = 0;

        for (interfaceIt = it->second.begin(); interfaceIt != it->second.end(); ++interfaceIt) {
            interfaces[interfaceIndex++] = interfaceIt->c_str();
        }

        status = announceObjectsArg[objIndex].Set("(oas)", objectPath.c_str(), it->second.size(), interfaces);
        announceObjectsArg[objIndex].Stabilize();
        delete [] interfaces;
        if (ER_OK != status) {
            return status;
        }
        objIndex++;
    }
    status = msgArg->Set("a(oas)", objIndex, announceObjectsArg);
    msgArg->SetOwnershipFlags(MsgArg::OwnsArgs | MsgArg::OwnsData, true);
    return status;
}
}
