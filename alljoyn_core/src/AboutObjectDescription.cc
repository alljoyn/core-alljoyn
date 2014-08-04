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

namespace ajn {

//AboutObjectDescription::AboutObjectDescription()
//{
//    //TODO
//}
//
//AboutObjectDescription::AboutObjectDescription(const MsgArg& arg)
//{
//    //TODO
//}
//
//AboutObjectDescription::~AboutObjectDescription()
//{
//    //TODO
//}

QStatus AboutObjectDescription::Initialize(const MsgArg& arg)
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
            //interfaceNames.insert(intfName);
            status = Add(objectPath, intfName);
            if (status != ER_OK) {
                return status;
            }
        }
//        status = Add(objectPath, interfaceNames);
//        if (status != ER_OK) {
//            return status;
//        }
    }

    return status;
}

QStatus AboutObjectDescription::Add(qcc::String const& path, qcc::String const& interfaceName)
{
    QStatus status = ER_OK;
    m_AnnounceObjectsMapLock.Lock();
    if (m_AnnounceObjectsMap.find(path) == m_AnnounceObjectsMap.end()) {
        std::set<qcc::String> newInterfaceName;
        newInterfaceName.insert(interfaceName);
        m_AnnounceObjectsMap[path] = newInterfaceName;
    } else {
        m_AnnounceObjectsMap[path].insert(interfaceName);
    }
    m_AnnounceObjectsMapLock.Unlock();
    return status;
}

QStatus AboutObjectDescription::Add(qcc::String const& path, const char** interfaceNames, size_t numInterfaces)
{
    QStatus status = ER_OK;
    m_AnnounceObjectsMapLock.Lock();
    if (m_AnnounceObjectsMap.find(path) == m_AnnounceObjectsMap.end()) {
        std::set<qcc::String> newInterfaceNames;
        for (size_t i = 0; i < numInterfaces; ++i) {
            newInterfaceNames.insert(interfaceNames[i]);
        }
        m_AnnounceObjectsMap[path] = newInterfaceNames;
    } else {
        for (size_t i = 0; i < numInterfaces; ++i) {
            m_AnnounceObjectsMap[path].insert(interfaceNames[i]);
        }
    }
    m_AnnounceObjectsMapLock.Unlock();
    return status;
}

QStatus AboutObjectDescription::Remove(qcc::String const& path, qcc::String const& interfaceName)
{
    QStatus status = ER_OK;
    m_AnnounceObjectsMapLock.Lock();
    std::map<qcc::String, std::set<qcc::String> >::iterator it = m_AnnounceObjectsMap.find(path);
    if (it != m_AnnounceObjectsMap.end()) {
        it->second.erase(interfaceName);
    }
    if (it->second.empty()) {
        m_AnnounceObjectsMap.erase(it);
    }
    m_AnnounceObjectsMapLock.Unlock();
    return status;
}

QStatus AboutObjectDescription::Remove(qcc::String const& path, const char** interfaceNames, size_t numInterfaces)
{
    QStatus status = ER_OK;
    m_AnnounceObjectsMapLock.Lock();
    std::map<qcc::String, std::set<qcc::String> >::iterator it = m_AnnounceObjectsMap.find(path);
    if (it != m_AnnounceObjectsMap.end()) {
        for (size_t i = 0; i < numInterfaces; ++i) {
            it->second.erase(interfaceNames[i]);
        }
    }
    if (it->second.empty()) {
        m_AnnounceObjectsMap.erase(it);
    }
    m_AnnounceObjectsMapLock.Unlock();
    return status;
}

bool AboutObjectDescription::HasPath(qcc::String const& path)
{
    return (m_AnnounceObjectsMap.find(path) != m_AnnounceObjectsMap.end());
}

bool AboutObjectDescription::HasInterface(qcc::String const& interfaceName)
{
    std::map<qcc::String, std::set<qcc::String> >::iterator it;
    for (it = m_AnnounceObjectsMap.begin(); it != m_AnnounceObjectsMap.end(); ++it) {
        if (HasInterface(it->first, interfaceName)) {
            return true;
        }
    }

    return false;
}

bool AboutObjectDescription::HasInterface(qcc::String const& path, qcc::String const& interfaceName)
{
    std::map<qcc::String, std::set<qcc::String> >::iterator it = m_AnnounceObjectsMap.find(path);
    if (it == m_AnnounceObjectsMap.end()) {
        return false;
    }

    size_t n = interfaceName.find_first_of('*');
    std::set<qcc::String>::iterator ifac_it = it->second.begin();
    for (ifac_it = it->second.begin(); ifac_it != it->second.end(); ++ifac_it) {
        if (n == qcc::String::npos && interfaceName == *ifac_it) {
            return true;
        } else if (n != qcc::String::npos && interfaceName.compare(0, n, ifac_it->substr(0, n)) == 0) {
            return true;
        }
    }
    return false;
//    if (it->second.find(interfaceName) == it->second.end()) {
//        //return false;
//
//    }
//    return true;
}

QStatus AboutObjectDescription::GetMsgArg(MsgArg* msgArg)
{
    QStatus status = ER_OK;
    MsgArg* announceObjectsArg = new MsgArg[m_AnnounceObjectsMap.size()];
    int objIndex = 0;
    for (std::map<qcc::String, std::set<qcc::String> >::const_iterator it = m_AnnounceObjectsMap.begin();
         it != m_AnnounceObjectsMap.end(); ++it) {

        qcc::String objectPath = it->first;
        const char** interfaces = new const char*[it->second.size()];
        std::set<qcc::String>::const_iterator interfaceIt;
        int interfaceIndex = 0;

        for (interfaceIt = it->second.begin(); interfaceIt != it->second.end(); ++interfaceIt) {
            interfaces[interfaceIndex++] = interfaceIt->c_str();
        }

        status = announceObjectsArg[objIndex].Set("(oas)", objectPath.c_str(), it->second.size(), interfaces);
        // TODO check there is no memory leak here.
        //announceObjectsArg[objIndex].SetOwnershipFlags(MsgArg::OwnsData);
        if (ER_OK != status) {
            return status;
        }
        objIndex++;
    }
    status = msgArg->Set("a(oas)", objIndex, announceObjectsArg);
    msgArg->SetOwnershipFlags(MsgArg::OwnsArgs);
    return status;
}
}
