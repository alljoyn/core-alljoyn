/**
 * @file
 * NameTable is a thread-safe mapping between unique/well-known
 * bus names and the BusEndpoint that these names exist on.
 * This mapping is many (names) to one (endpoint). Every endpoint has
 * exactly one unique name and zero or more well-known names.
 */

/******************************************************************************
 * Copyright (c) 2009-2011,2014 AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>

#include <assert.h>

#include <qcc/Debug.h>
#include <qcc/Logger.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include "NameTable.h"
#include "VirtualEndpoint.h"
#include "EndpointHelper.h"

#include <alljoyn/DBusStd.h>

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

SessionOpts::NameTransferType NameTable::GetNameTransfer(const VirtualEndpoint& vep)
{
    multimap<SessionId, RemoteEndpoint> b2bEps = vep->GetBusToBusEndpoints();
    if (b2bEps.empty()) {
        return SessionOpts::ALL_NAMES;
    } else {
        SessionOpts::NameTransferType nameTransfer = SessionOpts::DAEMON_NAMES;
        for (multimap<SessionId, RemoteEndpoint>::const_iterator it = b2bEps.begin();
             (nameTransfer != SessionOpts::ALL_NAMES) && (it != b2bEps.end());
             ++it) {
            nameTransfer = min(nameTransfer, it->second->GetFeatures().nameTransfer);
        }
        return nameTransfer;
    }
}

SessionOpts::NameTransferType NameTable::GetNameTransfer(BusEndpoint& ep)
{
    if (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
        VirtualEndpoint vep = VirtualEndpoint::cast(ep);
        return GetNameTransfer(vep);
    } else {
        return SessionOpts::ALL_NAMES;
    }
}

qcc::String NameTable::GenerateUniqueName(void)
{
    return uniquePrefix + U32ToString(IncrementAndFetch((int32_t*)&uniqueId));
}

void NameTable::SetGUID(const qcc::GUID128& guid)
{
    QCC_DbgPrintf(("AllJoyn Daemon GUID = %s (%s)\n", guid.ToString().c_str(), guid.ToShortString().c_str()));
    uniquePrefix = ":";
    uniquePrefix.append(guid.ToShortString());
    uniquePrefix.append(".");
}

void NameTable::AddUniqueName(BusEndpoint& endpoint)
{
    QCC_DbgTrace(("NameTable::AddUniqueName(%s)", endpoint->GetUniqueName().c_str()));

    SessionOpts::NameTransferType nameTransfer = GetNameTransfer(endpoint);

    const qcc::String& uniqueName = endpoint->GetUniqueName();
    QCC_DbgPrintf(("Add unique name %s", uniqueName.c_str()));
    lock.Lock(MUTEX_CONTEXT);
    UniqueNameEntry entry = { endpoint, nameTransfer };
    uniqueNames[uniqueName] = entry;
    lock.Unlock(MUTEX_CONTEXT);

    /* Notify listeners */
    CallListeners(uniqueName,
                  NULL, SessionOpts::ALL_NAMES,
                  &uniqueName, nameTransfer);
}

void NameTable::RemoveUniqueName(const qcc::String& uniqueName)
{
    QCC_DbgTrace(("RemoveUniqueName %s", uniqueName.c_str()));

    /* Erase the unique bus name and any well-known names that use the same endpoint */
    lock.Lock(MUTEX_CONTEXT);
    unordered_map<qcc::String, UniqueNameEntry, Hash, Equal>::iterator it = uniqueNames.find(uniqueName);
    if (it != uniqueNames.end()) {
        BusEndpoint endpoint = it->second.endpoint;
        SessionOpts::NameTransferType nameTransfer = it->second.nameTransfer;

        /* Remove well-known names asssociated with uniqueName */
        unordered_map<qcc::String, deque<NameQueueEntry>, Hash, Equal>::iterator ait = aliasNames.begin();
        while (ait != aliasNames.end()) {
            deque<NameQueueEntry>::iterator lit = ait->second.begin();
            bool startOver = false;
            while (lit != ait->second.end()) {
                if (lit->endpointName == endpoint->GetUniqueName()) {
                    if (lit == ait->second.begin()) {
                        uint32_t disposition;
                        String alias = ait->first;
                        String epName = endpoint->GetUniqueName();
                        /* Must unlock before calling RemoveAlias because it can call out (and cannot be locked at the time) */
                        lock.Unlock(MUTEX_CONTEXT);
                        RemoveAlias(alias, epName, disposition, NULL, NULL);
                        lock.Lock(MUTEX_CONTEXT);
                        /* Make sure iterator is still valid */
                        it = uniqueNames.find(uniqueName);
                        if (it == uniqueNames.end()) {
                            break;
                        }
                        if (DBUS_RELEASE_NAME_REPLY_RELEASED == disposition) {
                            ait = aliasNames.begin();
                            startOver = true;
                            break;
                        } else {
                            QCC_LogError(ER_FAIL, ("Failed to release %s from %s", alias.c_str(), epName.c_str()));
                            break;
                        }
                    } else {
                        ait->second.erase(lit);
                        break;
                    }
                } else {
                    ++lit;
                }
            }
            if (!startOver) {
                ++ait;
            }
        }

        if (it != uniqueNames.end()) {
            uniqueNames.erase(it);
            QCC_DbgPrintf(("Removed ep=%s from name table", uniqueName.c_str()));
        }

        lock.Unlock(MUTEX_CONTEXT);
        /* Notify listeners */
        CallListeners(uniqueName,
                      &uniqueName, nameTransfer,
                      NULL, SessionOpts::ALL_NAMES);
    } else {
        lock.Unlock(MUTEX_CONTEXT);
    }
}

QStatus NameTable::AddAlias(const qcc::String& aliasName,
                            const qcc::String& uniqueName,
                            uint32_t flags,
                            uint32_t& disposition,
                            NameListener* listener,
                            void* context)
{
    QStatus status;

    QCC_DbgTrace(("NameTable: AddAlias(%s, %s)", aliasName.c_str(), uniqueName.c_str()));

    lock.Lock(MUTEX_CONTEXT);
    unordered_map<qcc::String, UniqueNameEntry, Hash, Equal>::const_iterator it = uniqueNames.find(uniqueName);
    if (it != uniqueNames.end()) {
        unordered_map<qcc::String, deque<NameQueueEntry>, Hash, Equal>::iterator wasIt = aliasNames.find(aliasName);
        NameQueueEntry entry = { uniqueName, flags };
        /*
         * The value of origOwner comes from data that may be freed after the lock is released, so we can't
         * just use a pointer here, must make a copy.  newOwner does not have the same problem.
         */
        qcc::String origOwner;
        SessionOpts::NameTransferType origOwnerNameTransfer = SessionOpts::ALL_NAMES;
        const qcc::String* newOwner = NULL;

        if (wasIt != aliasNames.end()) {
            assert(!wasIt->second.empty());
            const NameQueueEntry& primary = wasIt->second[0];
            if (primary.endpointName == uniqueName) {
                /* Enpoint already owns this alias */
                disposition = DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER;
            } else if ((primary.flags & DBUS_NAME_FLAG_ALLOW_REPLACEMENT) && (flags & DBUS_NAME_FLAG_REPLACE_EXISTING)) {
                /* Make endpoint the current owner */
                wasIt->second.push_front(entry);
                disposition = DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
                origOwner = primary.endpointName;
                newOwner = &uniqueName;
            } else {
                if (flags & DBUS_NAME_FLAG_DO_NOT_QUEUE) {
                    /* Cannot replace current owner */
                    disposition = DBUS_REQUEST_NAME_REPLY_EXISTS;
                } else {
                    /* Add this new potential owner to the end of the list */
                    wasIt->second.push_back(entry);
                    disposition = DBUS_REQUEST_NAME_REPLY_IN_QUEUE;
                }
            }
        } else {
            /* No pre-existing queue for this name */
            aliasNames[aliasName] = deque<NameQueueEntry>(1, entry);
            disposition = DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
            newOwner = &uniqueName;

            /* Check to see if we are overriding a virtual (remote) name */
            map<qcc::StringMapKey, VirtualAliasEntry>::const_iterator vit = virtualAliasNames.find(aliasName);
            if (vit != virtualAliasNames.end()) {
                origOwner = vit->second.endpoint->GetUniqueName();
                origOwnerNameTransfer = vit->second.nameTransfer;
            }
        }
        lock.Unlock(MUTEX_CONTEXT);

        if (listener) {
            listener->AddAliasComplete(aliasName, disposition, context);
        }
        if (newOwner) {
            CallListeners(aliasName,
                          origOwner.empty() ? NULL : &origOwner, origOwnerNameTransfer,
                          newOwner, SessionOpts::ALL_NAMES);
        }
        status = ER_OK;
    } else {
        status = ER_BUS_NO_ENDPOINT;
        lock.Unlock(MUTEX_CONTEXT);
    }
    return status;
}

void NameTable::RemoveAlias(const qcc::String& aliasName,
                            const qcc::String& ownerName,
                            uint32_t& disposition,
                            NameListener* listener,
                            void* context)
{
    qcc::String oldOwner;
    qcc::String newOwner;
    SessionOpts::NameTransferType newOwnerNameTransfer = SessionOpts::ALL_NAMES;
    qcc::String aliasNameCopy(aliasName);

    QCC_DbgTrace(("NameTable: RemoveAlias(%s, %s)", aliasName.c_str(), ownerName.c_str()));

    lock.Lock(MUTEX_CONTEXT);

    /* Find endpoint for aliasName */
    unordered_map<qcc::String, deque<NameQueueEntry>, Hash, Equal>::iterator it = aliasNames.find(aliasName);
    if (it != aliasNames.end()) {
        deque<NameQueueEntry>& queue = it->second;

        assert(!queue.empty());
        if (queue[0].endpointName == ownerName) {
            /* Remove primary */
            if (queue.size() > 1) {
                queue.pop_front();
                BusEndpoint ep = FindEndpoint(queue[0].endpointName);
                if (ep->IsValid()) {
                    newOwner = queue[0].endpointName;
                }
            }
            if (newOwner.empty()) {
                /* Check to see if there is a (now unmasked) remote owner for the alias */
                map<qcc::StringMapKey, VirtualAliasEntry>::const_iterator vit = virtualAliasNames.find(aliasName);
                if (vit != virtualAliasNames.end()) {
                    newOwner = vit->second.endpoint->GetUniqueName();
                    newOwnerNameTransfer = vit->second.nameTransfer;
                }
                aliasNames.erase(it);
            }
            oldOwner = ownerName;
            disposition = DBUS_RELEASE_NAME_REPLY_RELEASED;
        } else {
            /* Alias is not owned by ownerName */
            disposition = DBUS_RELEASE_NAME_REPLY_NOT_OWNER;
        }
    } else {
        disposition = DBUS_RELEASE_NAME_REPLY_NON_EXISTENT;
    }

    lock.Unlock(MUTEX_CONTEXT);

    if (listener) {
        listener->RemoveAliasComplete(aliasNameCopy, disposition, context);
    }
    if (!oldOwner.empty()) {
        CallListeners(aliasNameCopy,
                      &oldOwner, SessionOpts::ALL_NAMES,
                      newOwner.empty() ? NULL : &newOwner, newOwnerNameTransfer);
    }
}
bool NameTable::IsValidLocalUniqueName(const qcc::String& uniqueName) const
{
    bool ret = false;
    size_t period_pos = uniqueName.find(".");
    if (period_pos != String::npos) {
        size_t period_pos1 = uniqueName.find(".", period_pos + 1);
        if (period_pos1 == String::npos) {
            // Contains exactly one "."
            String guid = uniqueName.substr(0, GUID128::SHORT_SIZE + 2);
            if (guid == uniquePrefix) {
                //guid matches uniquePrefix
                String idStr = uniqueName.substr(GUID128::SHORT_SIZE + 2);
                uint32_t id = StringToU32(idStr);
                if ((id != 0) && (id <= uniqueId)) {
                    //valid id
                    ret = true;
                }
            }
        }
    }
    return ret;
}

BusEndpoint NameTable::FindEndpoint(const qcc::String& busName) const
{
    BusEndpoint ep;

    lock.Lock(MUTEX_CONTEXT);
    if (busName[0] == ':') {
        unordered_map<qcc::String, UniqueNameEntry, Hash, Equal>::const_iterator it = uniqueNames.find(busName);
        if (it != uniqueNames.end()) {
            ep = it->second.endpoint;
        }
    } else {
        unordered_map<qcc::String, deque<NameQueueEntry>, Hash, Equal>::const_iterator it = aliasNames.find(busName);
        if (it != aliasNames.end()) {
            assert(!it->second.empty());
            ep = FindEndpoint(it->second[0].endpointName);
        }
        /* Fallback to virtual (remote) aliases if a suitable local one cannot be found */
        if (!ep->IsValid()) {
            map<qcc::StringMapKey, VirtualAliasEntry>::const_iterator vit = virtualAliasNames.find(busName);
            if (vit != virtualAliasNames.end()) {
                VirtualEndpoint vep = vit->second.endpoint;
                ep = BusEndpoint::cast(vep);
            }
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
    return ep;
}

void NameTable::GetBusNames(vector<qcc::String>& names) const
{
    lock.Lock(MUTEX_CONTEXT);

    unordered_map<qcc::String, deque<NameQueueEntry>, Hash, Equal>::const_iterator it = aliasNames.begin();
    while (it != aliasNames.end()) {
        names.push_back(it->first);
        ++it;
    }
    unordered_map<qcc::String, UniqueNameEntry, Hash, Equal>::const_iterator uit = uniqueNames.begin();
    while (uit != uniqueNames.end()) {
        names.push_back(uit->first);
        ++uit;
    }
    lock.Unlock(MUTEX_CONTEXT);
}

void NameTable::GetUniqueNamesAndAliases(vector<pair<qcc::String, vector<qcc::String> > >& names) const
{

    /* Create a intermediate map to avoid N^2 perf */
    multimap<BusEndpoint, qcc::String> epMap;
    lock.Lock(MUTEX_CONTEXT);
    unordered_map<qcc::String, UniqueNameEntry, Hash, Equal>::const_iterator uit = uniqueNames.begin();
    while (uit != uniqueNames.end()) {
        epMap.insert(pair<const BusEndpoint, qcc::String>(uit->second.endpoint, uit->first));
        ++uit;
    }
    unordered_map<qcc::String, deque<NameQueueEntry>, Hash, Equal>::const_iterator ait = aliasNames.begin();
    while (ait != aliasNames.end()) {
        if (!ait->second.empty()) {
            BusEndpoint ep = FindEndpoint(ait->second.front().endpointName);
            if (ep->IsValid()) {
                epMap.insert(pair<BusEndpoint, qcc::String>(ep, ait->first));
            }
        }
        ++ait;
    }
    map<StringMapKey, VirtualAliasEntry>::const_iterator vit = virtualAliasNames.begin();
    while (vit != virtualAliasNames.end()) {
        VirtualEndpoint vep = vit->second.endpoint;
        epMap.insert(pair<BusEndpoint, qcc::String>(BusEndpoint::cast(vep), vit->first.c_str()));
        ++vit;
    }
    lock.Unlock(MUTEX_CONTEXT);

    /* Fill in the caller's vector */
    qcc::String uniqueName;
    vector<qcc::String> aliasVec;
    BusEndpoint lastEp;
    multimap<BusEndpoint, qcc::String>::iterator it = epMap.begin();
    names.reserve(uniqueNames.size());  // prevent dynamic resizing in loop
    while (true) {
        if ((it == epMap.end()) || (lastEp != it->first)) {
            if (!uniqueName.empty()) {
                names.push_back(pair<qcc::String, vector<qcc::String> >(uniqueName, aliasVec));
            }
            uniqueName.clear();
            aliasVec.clear();
            if (it == epMap.end()) {
                break;
            }
        }
        const String& name = it->second;
        if (name[0] == ':') {
            uniqueName = name;
        } else {
            aliasVec.push_back(name);
        }
        lastEp = it->first;
        ++it;
    }
}

void NameTable::GetQueuedNames(const qcc::String& busName, std::vector<qcc::String>& names)
{
    unordered_map<qcc::String, deque<NameQueueEntry>, Hash, Equal>::iterator ait = aliasNames.find(busName.c_str());
    if (ait != aliasNames.end()) {

        names.reserve(ait->second.size()); //prevent dynamic resizing in loop
        for (deque<NameQueueEntry>::iterator lit = ait->second.begin(); lit != ait->second.end(); ++lit) {
            names.push_back(lit->endpointName);
        }
    } else {
        names.clear();
    }
}

void NameTable::UpdateVirtualAliases(const qcc::String& epName)
{
    lock.Lock(MUTEX_CONTEXT);
    BusEndpoint tempEp = FindEndpoint(epName);
    VirtualEndpoint ep = VirtualEndpoint::cast(tempEp);

    QCC_DbgTrace(("NameTable::UpdateVirtualAliases(%s)", ep->IsValid() ? ep->GetUniqueName().c_str() : "<none>"));

    if (ep->IsValid()) {
        map<qcc::StringMapKey, VirtualAliasEntry>::iterator vit = virtualAliasNames.begin();
        while (vit != virtualAliasNames.end()) {
            SessionOpts::NameTransferType oldNameTransfer = SessionOpts::ALL_NAMES;
            SessionOpts::NameTransferType newNameTransfer = SessionOpts::ALL_NAMES;
            bool madeChange = false;
            if (vit->second.endpoint == ep) {
                oldNameTransfer = vit->second.nameTransfer;
                newNameTransfer = GetNameTransfer(vit->second.endpoint);
                madeChange = (oldNameTransfer != newNameTransfer);
                vit->second.nameTransfer = newNameTransfer;
            }
            String alias = vit->first.c_str();
            if (madeChange && (aliasNames.find(alias) == aliasNames.end())) {
                lock.Unlock(MUTEX_CONTEXT);
                CallListeners(alias,
                              &epName, oldNameTransfer,
                              &epName, newNameTransfer);
                lock.Lock(MUTEX_CONTEXT);
                vit = virtualAliasNames.upper_bound(alias);
            } else {
                ++vit;
            }
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
}

void NameTable::RemoveVirtualAliases(const qcc::String& epName)
{
    lock.Lock(MUTEX_CONTEXT);
    BusEndpoint tempEp = FindEndpoint(epName);
    VirtualEndpoint ep = VirtualEndpoint::cast(tempEp);

    QCC_DbgTrace(("NameTable::RemoveVirtualAliases(%s)", ep->IsValid() ? ep->GetUniqueName().c_str() : "<none>"));

    if (ep->IsValid()) {
        map<qcc::StringMapKey, VirtualAliasEntry>::iterator vit = virtualAliasNames.begin();
        while (vit != virtualAliasNames.end()) {
            if (vit->second.endpoint == ep) {
                String alias = vit->first.c_str();
                SessionOpts::NameTransferType nameTransfer = vit->second.nameTransfer;
                virtualAliasNames.erase(vit++);
                if (aliasNames.find(alias) == aliasNames.end()) {
                    lock.Unlock(MUTEX_CONTEXT);
                    CallListeners(alias,
                                  &epName, nameTransfer,
                                  NULL, SessionOpts::ALL_NAMES);
                    lock.Lock(MUTEX_CONTEXT);
                    vit = virtualAliasNames.upper_bound(alias);
                }
            } else {
                ++vit;
            }
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
}

bool NameTable::SetVirtualAlias(const qcc::String& alias,
                                VirtualEndpoint* newOwner,
                                VirtualEndpoint& requestingEndpoint)
{
    QCC_DbgTrace(("NameTable::SetVirtualAlias(%s, %p/%s, %p/%s)",
                  alias.c_str(),
                  newOwner ? (*newOwner).unwrap() : NULL, newOwner ? (*newOwner)->GetUniqueName().c_str() : "<none>",
                  requestingEndpoint.unwrap(), requestingEndpoint->GetUniqueName().c_str()));

    lock.Lock(MUTEX_CONTEXT);

    VirtualEndpoint oldOwner;
    String oldName;
    SessionOpts::NameTransferType oldOwnerNameTransfer = SessionOpts::ALL_NAMES;
    map<qcc::StringMapKey, VirtualAliasEntry>::iterator vit = virtualAliasNames.find(alias);
    if (vit != virtualAliasNames.end()) {
        oldOwner = vit->second.endpoint;
    }
    if (oldOwner->IsValid()) {
        oldName = oldOwner->GetUniqueName();
        oldOwnerNameTransfer = vit->second.nameTransfer;
        /*
         * Virtual aliases cannot directly change ownership from one remote daemon to another.
         * Allowing this would allow a daemon to "take" an existing name from another daemon.
         * Name changes are allowed within the same remote daemon or when the name is not already
         * owned.
         */
        const String& reqOwnerName = requestingEndpoint->GetUniqueName();
        size_t oldPeriodOff = oldName.find_first_of('.');
        size_t reqPeriodOff = reqOwnerName.find_first_of('.');
        if ((oldPeriodOff == String::npos) || (0 != oldName.compare(0, oldPeriodOff, reqOwnerName, 0, reqPeriodOff))) {
            lock.Unlock(MUTEX_CONTEXT);
            return false;
        }
    }

    bool maskingLocalName = (aliasNames.find(alias) != aliasNames.end());

    String newName;
    SessionOpts::NameTransferType newOwnerNameTransfer = SessionOpts::ALL_NAMES;
    bool madeChange = false;
    if (newOwner && (*newOwner)->IsValid()) {
        newOwnerNameTransfer = GetNameTransfer(*newOwner);
        VirtualAliasEntry entry = { *newOwner, newOwnerNameTransfer };
        virtualAliasNames[alias] = entry;
        madeChange = !newOwner->iden(oldOwner) || (oldOwnerNameTransfer != newOwnerNameTransfer);
    } else {
        virtualAliasNames.erase(StringMapKey(alias));
        madeChange = true;
    }
    if (newOwner && (*newOwner)->IsValid()) {
        newName = (*newOwner)->GetUniqueName();
    }

    lock.Unlock(MUTEX_CONTEXT);

    /* Virtual aliases cannot override locally requested aliases */
    if (madeChange && !maskingLocalName) {
        CallListeners(alias,
                      oldName.empty() ? NULL : &oldName, oldOwnerNameTransfer,
                      newName.empty() ? NULL : &newName, newOwnerNameTransfer);
    }
    return madeChange;
}

void NameTable::AddListener(NameListener* listener)
{
    lock.Lock(MUTEX_CONTEXT);
    listeners.insert(ProtectedNameListener(listener));
    lock.Unlock(MUTEX_CONTEXT);
}

void NameTable::RemoveListener(NameListener* listener)
{
    lock.Lock(MUTEX_CONTEXT);
    ProtectedNameListener pl(listener);
    set<ProtectedNameListener>::iterator it = listeners.find(pl);
    if (it != listeners.end()) {
        /* Remove listener from set */
        listeners.erase(it);

        /* Wait until references to pl reach q (pl is only remaining ref) */
        while (pl.GetRefCount() > 1) {
            lock.Unlock(MUTEX_CONTEXT);
            qcc::Sleep(4);
            lock.Lock(MUTEX_CONTEXT);
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
}

void NameTable::CallListeners(const qcc::String& aliasName,
                              const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                              const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer)
{
    lock.Lock(MUTEX_CONTEXT);
    set<ProtectedNameListener>::iterator it = listeners.begin();
    while (it != listeners.end()) {
        ProtectedNameListener nl = *it;
        lock.Unlock(MUTEX_CONTEXT);
        (*nl)->NameOwnerChanged(aliasName,
                                oldOwner, oldOwnerNameTransfer,
                                newOwner, newOwnerNameTransfer);
        lock.Lock(MUTEX_CONTEXT);
        it = listeners.upper_bound(nl);
    }
    lock.Unlock(MUTEX_CONTEXT);
}

}
