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
#include <qcc/Debug.h>

#include "ProxyObjectManager.h"
#include "SecLibDef.h"

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
ProxyObjectManager::ProxyObjectManager(ajn::BusAttachment* ba,
                                       const SecurityManagerConfig& config) :
    bus(ba), objectPath(config.pmObjectPath), interfaceName(config.pmIfn)
{
}

ProxyObjectManager::~ProxyObjectManager()
{
}

QStatus ProxyObjectManager::GetProxyObject(const ApplicationInfo appInfo,
                                           SessionType type,
                                           ajn::ProxyBusObject** remoteObject)
{
    ajn::SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                     SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = bus->JoinSession(appInfo.busName.c_str(), MNGT_SERVICE_PORT,
                                      this, sessionId, opts);
    if (status != ER_OK) {
        return status;
    }
    /* B. Setup ProxyBusObject:
     *     - Create a ProxyBusObject from remote application info and session
     *     - Get the interface description from the bus based on the remote interface name
     *     - Extend the ProxyBusObject with the interface description
     */

    do {
        const InterfaceDescription* remoteIntf = bus->GetInterface(interfaceName.c_str());
        if (NULL == remoteIntf) {
            QCC_DbgRemoteError(("No remote interface found of app to claim"));
            status = ER_FAIL;
            break;
        }
        ajn::ProxyBusObject* remoteObj = new ajn::ProxyBusObject(*bus, appInfo.busName.c_str(),
                                                                 objectPath.c_str(), sessionId);
        if (remoteObj == NULL) {
            break;
        }
        status = remoteObj->AddInterface(*remoteIntf);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to add interface to proxy object."));
            delete remoteObj;
            break;
        }
        *remoteObject = remoteObj;
    } while (0);
    return status;
}

QStatus ProxyObjectManager::ReleaseProxyObject(ajn::ProxyBusObject* remoteObject)
{
    SessionId sessionId = remoteObject->GetSessionId();
    delete remoteObject;
    return bus->LeaveSession(sessionId);
}

void ProxyObjectManager::SessionLost(ajn::SessionId sessionId,
                                     SessionLostReason reason)
{
    QCC_DbgPrintf(("Lost session %lu", (unsigned long)sessionId));
}
}
}
#undef QCC_MODULE
