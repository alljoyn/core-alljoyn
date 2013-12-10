/**
 * @file
 * AllJoynBTTransport is an implementation of AllJoynTransport that uses Bluetooth.
 *
 * This implementation uses the message bus to talk to the Bluetooth subsystem.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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

#include <limits>
#include <map>
#include <vector>

#include <qcc/Event.h>
#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>

#include "BDAddress.h"
#include "BTController.h"
#include "BTEndpoint.h"
#include "BTTransport.h"

#if defined QCC_OS_GROUP_POSIX
#if defined(QCC_OS_DARWIN)
#warning Darwin support for bluetooth to be implemented
#else
#include "bt_bluez/BTAccessor.h"
#endif
#elif defined QCC_OS_GROUP_WINDOWS
#include "bt_windows/BTAccessor.h"
#endif

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_BT"


using namespace std;
using namespace qcc;
using namespace ajn;

#if !defined QCC_OS_GROUP_WINDOWS
using namespace ajn::bluez;
#endif

namespace ajn {

/**
 * Name of transport used in transport specs.
 */
const char* BTTransport::TransportName = "bluetooth";

#define BUS_NAME_TTL numeric_limits<uint8_t>::max()

/*****************************************************************************/


BTTransport::BTTransport(BusAttachment& bus) :
    Thread("BTTransport"),
    bus(bus),
    transportIsStopping(false),
    btmActive(false)
{
    btController = new BTController(bus, *this);
    QStatus status = btController->Init();
    if (status == ER_OK) {
        btAccessor = new BTAccessor(this, bus.GetGlobalGUIDString());
        btmActive = true;
    }
}


BTTransport::~BTTransport()
{
    /* Stop the thread */
    Stop();
    Join();

    delete btController;
    btController = NULL;
    if (btmActive) {
        delete btAccessor;
    }
}

QStatus BTTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    if (!btmActive) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    QStatus status = ParseArguments("bluetooth", inSpec, argMap);
    if (status == ER_OK) {
        map<qcc::String, qcc::String>::iterator it;
        outSpec = "bluetooth:";

        it = argMap.find("addr");
        if (it == argMap.end()) {
            status = ER_FAIL;
            QCC_LogError(status, ("'addr=' must be specified for 'bluetooth:'"));
        } else {
            outSpec.append("addr=");
            outSpec += it->second;

            it = argMap.find("psm");
            outSpec.append(",psm=");
            if (it == argMap.end()) {
                status = ER_FAIL;
                QCC_LogError(status, ("'psm=' must be specified for 'bluetooth:'"));
            } else {
                outSpec += it->second;
            }
        }
    }
    return status;
}

void* BTTransport::Run(void* arg)
{
    if (!btmActive) {
        return (void*)ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    QStatus status = ER_OK;
    vector<Event*> signaledEvents;
    vector<Event*> checkEvents;

    while (!IsStopping()) {
        Event* l2capEvent = btAccessor->GetL2CAPConnectEvent();
        if (l2capEvent) {
            checkEvents.push_back(l2capEvent);
        }

        checkEvents.push_back(&stopEvent);

        /* wait for something to happen */
        QCC_DbgTrace(("waiting for incoming connection ..."));
        status = Event::Wait(checkEvents, signaledEvents);
        if (ER_OK != status) {
            QCC_LogError(status, ("Event::Wait failed"));
            break;
        }

        /* Iterate over signaled events */
        for (vector<Event*>::iterator it = signaledEvents.begin(); it != signaledEvents.end(); ++it) {
            if (*it != &stopEvent) {
                qcc::String unused;
                /* Accept a new connection */
                qcc::String authName;
                RemoteEndpoint conn(btAccessor->Accept(bus, *it));
                if (!conn->IsValid()) {
                    continue;
                }

                /* Initialized the features for this endpoint */
                conn->GetFeatures().isBusToBus = false;
                conn->GetFeatures().allowRemote = false;
                conn->GetFeatures().handlePassing = false;

                threadListLock.Lock(MUTEX_CONTEXT);
                threadList.insert(conn);
                threadListLock.Unlock(MUTEX_CONTEXT);
                QCC_DbgPrintf(("BTTransport::Run: Calling conn->Establish() [for accepted connection]"));
                status = conn->Establish("ANONYMOUS", authName, unused);
                if (ER_OK == status) {
                    QCC_DbgPrintf(("Starting endpoint [for accepted connection]"));
                    conn->SetListener(this);
                    status = conn->Start();
                }

                if (ER_OK == status) {
                    connNodeDB.Lock(MUTEX_CONTEXT);
                    const BTNodeInfo& connNode = (BTEndpoint::cast(conn))->GetNode();
                    BTNodeInfo node = connNodeDB.FindNode(connNode->GetBusAddress().addr);
                    if (!node->IsValid()) {
                        node = connNode;
                        connNodeDB.AddNode(node);
                    }

                    node->IncConnCount();
                    QCC_DbgPrintf(("Increment connection count for %s to %u: ACCEPT", node->ToString().c_str(), node->GetConnectionCount()));
                    connNodeDB.Unlock(MUTEX_CONTEXT);

                } else {
                    QCC_LogError(status, ("Error starting RemoteEndpoint"));
                    EndpointExit(conn);
                    conn->Invalidate();
                }
            } else {
                (*it)->ResetEvent();
            }
        }
        signaledEvents.clear();
        checkEvents.clear();
    }
    return (void*) status;
}


QStatus BTTransport::Start()
{
    QCC_DbgTrace(("BTTransport::Start()"));
    if (!btmActive) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    return btAccessor->Start();
}


QStatus BTTransport::Stop(void)
{
    QCC_DbgTrace(("BTTransport::Stop()"));
    if (!btmActive) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    transportIsStopping = true;

    set<RemoteEndpoint>::iterator eit;

    bool isStopping = IsStopping();

    if (!isStopping) {
        btAccessor->Stop();
    }

    /* Stop any endpoints that are running */
    threadListLock.Lock(MUTEX_CONTEXT);
    for (eit = threadList.begin(); eit != threadList.end(); ++eit) {
        RemoteEndpoint r = (*eit);
        r->Stop();
    }
    threadListLock.Unlock(MUTEX_CONTEXT);

    return ER_OK;
}


QStatus BTTransport::Join(void)
{
    if (!btmActive) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    /* Wait for the thread list to empty out */
    threadListLock.Lock(MUTEX_CONTEXT);
    while (threadList.size() > 0) {
        threadListLock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(50);
        threadListLock.Lock(MUTEX_CONTEXT);
    }
    threadListLock.Unlock(MUTEX_CONTEXT);
    return Thread::Join();
}


void BTTransport::EnableDiscovery(const char* namePrefix)
{
    QCC_DbgTrace(("BTTransport::EnableDiscovery(namePrefix = \"%s\")", namePrefix));
    if (!btmActive) {
        return;
    }

    qcc::String name(namePrefix);
    QStatus status;

    status = btController->AddFindName(name);

    if (status != ER_OK) {
        QCC_LogError(status, ("BTTransport::EnableDiscovery"));
    }
}


void BTTransport::DisableDiscovery(const char* namePrefix)
{
    QCC_DbgTrace(("BTTransport::DisableDiscovery(namePrefix = \"%s\")", namePrefix));
    if (!btmActive) {
        return;
    }

    qcc::String name(namePrefix);
    QStatus status;

    status = btController->RemoveFindName(name);

    if (status != ER_OK) {
        QCC_LogError(status, ("BTTransport::DisableDiscovery"));
    }
}


QStatus BTTransport::EnableAdvertisement(const qcc::String& advertiseName, bool quietly)
{
    QCC_DbgTrace(("BTTransport::EnableAdvertisement(%s)", advertiseName.c_str()));
    if (!btmActive) {
        return ER_FAIL;
    }

    QStatus status = btController->AddAdvertiseName(advertiseName);
    if (status != ER_OK) {
        QCC_LogError(status, ("BTTransport::EnableAdvertisement"));
    }
    return status;
}


void BTTransport::DisableAdvertisement(const qcc::String& advertiseName)
{
    QCC_DbgTrace(("BTTransport::DisableAdvertisement(advertiseName = %s)", advertiseName.c_str()));
    if (!btmActive) {
        return;
    }

    QStatus status = btController->RemoveAdvertiseName(advertiseName);
    if (status != ER_OK) {
        QCC_LogError(status, ("BTTransport::DisableAdvertisement"));
    }
}


QStatus BTTransport::Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep)
{
    QCC_DbgTrace(("BTTransport::Connect(connectSpec = \"%s\")", connectSpec));
    if (!btmActive) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    qcc::String spec = connectSpec;
    BTBusAddress addr = spec;
    RemoteEndpoint ep;

    QStatus status = Connect(addr, ep);
    newep = BusEndpoint::cast(ep);
    if (status != ER_OK) {
        ep->Invalidate();
    }
    return status;
}

QStatus BTTransport::Disconnect(const char* connectSpec)
{
    return ER_OK;
}

// @@ TODO
QStatus BTTransport::StartListen(const char* listenSpec)
{
    QCC_DbgTrace(("BTTransport::StartListen(listenSpec = \"%s\")", listenSpec));
    if (!btmActive) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    /*
     * Bluetooth listens are managed by the Master node in a piconet
     */
    return ER_OK;
}

// @@ TODO
QStatus BTTransport::StopListen(const char* listenSpec)
{
    QCC_DbgTrace(("BTTransport::StopListen(listenSpec = \"%s\")", listenSpec));
    if (!btmActive) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    /*
     * Bluetooth listens are managed by the Master node in a piconet
     */
    return ER_OK;
}

void BTTransport::BTDeviceAvailable(bool avail)
{
    if (btController) {
        btController->BTDeviceAvailable(avail);
    }
}

bool BTTransport::CheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const
{
    if (btController) {
        return btController->CheckIncomingAddress(addr, redirectAddr);
    } else {
        return false;
    }
}

void BTTransport::DisconnectAll()
{
    set<RemoteEndpoint>::iterator eit;
    threadListLock.Lock(MUTEX_CONTEXT);
    for (eit = threadList.begin(); eit != threadList.end(); ++eit) {
        RemoteEndpoint r = *eit;
        r->Stop();
    }
    threadListLock.Unlock(MUTEX_CONTEXT);
}

void BTTransport::EndpointExit(RemoteEndpoint& endpoint)
{
    if (!btmActive) {
        return;
    }

    QCC_DbgTrace(("BTTransport::EndpointExit(endpoint => \"%s\" - \"%s\")",
                  endpoint->GetRemoteGUID().ToShortString().c_str(),
                  endpoint->GetConnectSpec().c_str()));

    BTNodeInfo node;

    connNodeDB.Lock(MUTEX_CONTEXT);

    /* Remove thread from thread list */
    threadListLock.Lock(MUTEX_CONTEXT);
    set<RemoteEndpoint>::iterator eit = threadList.find(endpoint);
    if (eit != threadList.end()) {
        BTEndpoint btEp = BTEndpoint::cast(endpoint);
        if (btEp->GetNode()->GetBusAddress().psm == bt::INCOMING_PSM) {
            node = connNodeDB.FindNode(btEp->GetNode()->GetBusAddress().addr);
        } else {
            node = connNodeDB.FindNode(btEp->GetNode()->GetBusAddress());
        }
        threadList.erase(eit);
    }
    threadListLock.Unlock(MUTEX_CONTEXT);

    if (node->IsValid()) {
        uint32_t connCount = node->DecConnCount();
        QCC_DbgPrintf(("Decrement connection count for %s to %u: ENDPOINT_EXIT", node->ToString().c_str(), connCount));
        if (connCount == 0) {
            connNodeDB.RemoveNode(node);

            // There should only ever have been one.
            assert(!connNodeDB.FindNode(node->GetBusAddress().addr)->IsValid());
        }

        if (connCount == 1) {
            btController->LostLastConnection(node);
        }
    }

    connNodeDB.Unlock(MUTEX_CONTEXT);

}


void BTTransport::DeviceChange(const BDAddress& bdAddr,
                               uint32_t uuidRev,
                               bool eirCapable)
{
    if (btController) {
        btController->ProcessDeviceChange(bdAddr, uuidRev, eirCapable);
    }
}


/********************************************************/

QStatus BTTransport::StartFind(const BDAddressSet& ignoreAddrs, uint32_t duration)
{
    return btAccessor->StartDiscovery(ignoreAddrs, duration);
}


QStatus BTTransport::StopFind()
{
    return btAccessor->StopDiscovery();
}


QStatus BTTransport::StartAdvertise(uint32_t uuidRev,
                                    const BDAddress& bdAddr,
                                    uint16_t psm,
                                    const BTNodeDB& adInfo,
                                    uint32_t duration)
{
    QStatus status = btAccessor->SetSDPInfo(uuidRev, bdAddr, psm, adInfo);
    if (status == ER_OK) {
        status = btAccessor->StartDiscoverability(duration);
    }
    return status;
}


QStatus BTTransport::StopAdvertise()
{
    BDAddress bdAddr;
    BTNodeDB adInfo;
    btAccessor->SetSDPInfo(bt::INVALID_UUIDREV, bdAddr, bt::INVALID_PSM, adInfo);
    btAccessor->StopDiscoverability();
    return ER_OK;  // This will ensure that the topology manager stays in the right state.
}


void BTTransport::FoundNamesChange(const qcc::String& guid,
                                   const vector<String>& names,
                                   const BDAddress& bdAddr,
                                   uint16_t psm,
                                   bool lost)
{
    if (listener) {
        qcc::String busAddr("bluetooth:addr=" + bdAddr.ToString() +
                            ",psm=0x" + U32ToString(psm, 16));

        listener->FoundNames(busAddr, guid, TRANSPORT_BLUETOOTH, &names, lost ? 0 : BUS_NAME_TTL);
    }
}


QStatus BTTransport::StartListen(BDAddress& addr,
                                 uint16_t& psm)
{
    if (!btmActive) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    QStatus status;
    status = btAccessor->StartConnectable(addr, psm);
    if (status == ER_OK) {
        QCC_DbgHLPrintf(("Listening on addr: %s  psm = %04x", addr.ToString().c_str(), psm));
        Thread::Start();
    }
    return status;
}


void BTTransport::StopListen()
{
    Thread::Stop();
    Thread::Join();
    btAccessor->StopConnectable();
    QCC_DbgHLPrintf(("Stopped listening"));
}

QStatus BTTransport::GetDeviceInfo(const BDAddress& addr,
                                   uint32_t& uuidRev,
                                   BTBusAddress& connAddr,
                                   BTNodeDB& adInfo)
{
    if (!btmActive) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    return btAccessor->GetDeviceInfo(addr, &uuidRev, &connAddr, &adInfo);
}



QStatus BTTransport::Connect(const BTBusAddress& addr,
                             RemoteEndpoint& newep)
{
    QStatus status;
    RemoteEndpoint conn;

    qcc::String authName;
    qcc::String redirection;
    BTNodeInfo connNode;

redirect:
    connNode = btController->PrepConnect(addr, redirection);
    if (!connNode->IsValid()) {
        status = ER_FAIL;
        QCC_LogError(status, ("No connect route to device with address %s", addr.ToString().c_str()));
        goto exit;
    }

    conn = btAccessor->Connect(bus, connNode);
    if (!conn->IsValid()) {
        status = ER_FAIL;
        goto exit;
    }

    /* Initialized the features for this endpoint */
    conn->GetFeatures().isBusToBus = true;
    conn->GetFeatures().allowRemote = bus.GetInternal().AllowRemoteMessages();
    conn->GetFeatures().handlePassing = false;

    threadListLock.Lock(MUTEX_CONTEXT);
    threadList.insert(conn);
    threadListLock.Unlock(MUTEX_CONTEXT);
    QCC_DbgPrintf(("BTTransport::Connect: Calling conn->Establish() [addr = %s via %s]",
                   addr.ToString().c_str(), connNode->ToString().c_str()));
    redirection.clear();
    status = conn->Establish("ANONYMOUS", authName, redirection);
    if (status != ER_OK) {
        QCC_LogError(status, ("BTEndpoint::Establish failed"));
        EndpointExit(conn);
        conn->Invalidate();
        goto exit;
    }

    QCC_DbgPrintf(("Starting endpoint [addr = %s via %s]", addr.ToString().c_str(), connNode->ToString().c_str()));
    /* Start the endpoint */
    conn->SetListener(this);
    status = conn->Start();
    if (status != ER_OK) {
        QCC_LogError(status, ("BTEndpoint::Start failed"));
        EndpointExit(conn);
        conn->Invalidate();
        goto exit;
    }

    /* If transport is closing, then don't allow any new endpoints */
    if (transportIsStopping) {
        status = ER_BUS_TRANSPORT_NOT_STARTED;
    }

exit:
    if (status == ER_OK) {

        newep = conn;

        connNodeDB.Lock(MUTEX_CONTEXT);
        BTEndpoint btc = BTEndpoint::cast(conn);
        BTNodeInfo connNode = btc->GetNode();
        BTNodeInfo node = connNodeDB.FindNode(connNode->GetBusAddress().addr);
        if (!node->IsValid() || (node->GetBusAddress().psm == bt::INCOMING_PSM)) {
            if (node->GetBusAddress().psm == bt::INCOMING_PSM) {
                connNode->SetConnectionCount(node->GetConnectionCount());
                if ((connNode->GetSessionState() != _BTNodeInfo::SESSION_UP) &&
                    (node->GetSessionState() != _BTNodeInfo::NO_SESSION)) {
                    connNode->SetSessionState(node->GetSessionState());
                }
                connNodeDB.RemoveNode(node);

                QCC_DbgPrintf(("Set connection count for %s to %u: CONNECT", connNode->ToString().c_str(), connNode->GetConnectionCount()));
            }
            node = connNode;
            connNodeDB.AddNode(node);
        }

        node->IncConnCount();
        QCC_DbgPrintf(("Increment connection count for %s to %u: CONNECT", node->ToString().c_str(), node->GetConnectionCount()));
        connNodeDB.Unlock(MUTEX_CONTEXT);

    } else {
        conn->Invalidate();
    }

    String emptyName;
    btController->PostConnect(status, connNode, conn->IsValid() ? conn->GetRemoteName() : emptyName);

    if ((status == ER_BUS_ENDPOINT_REDIRECTED) && !redirection.empty()) {
        QCC_DbgPrintf(("Redirecting connection to %s.", redirection.c_str()));
        goto redirect;
    }

    return status;
}


QStatus BTTransport::Disconnect(const String& busName)
{
    QCC_DbgTrace(("BTTransport::Disconnect(busName = %s)", busName.c_str()));
    QStatus status(ER_BUS_BAD_TRANSPORT_ARGS);

    set<RemoteEndpoint>::iterator eit;

    threadListLock.Lock(MUTEX_CONTEXT);
    for (eit = threadList.begin(); eit != threadList.end(); ++eit) {
        RemoteEndpoint r = (*eit);
        if (busName == r->GetUniqueName()) {
            status = r->Stop();
        }
    }
    threadListLock.Unlock(MUTEX_CONTEXT);
    return status;
}


RemoteEndpoint BTTransport::LookupEndpoint(const qcc::String& busName)
{
    RemoteEndpoint ep;
    set<RemoteEndpoint>::iterator eit;
    threadListLock.Lock(MUTEX_CONTEXT);
    for (eit = threadList.begin(); eit != threadList.end(); ++eit) {
        RemoteEndpoint r = (*eit);
        if (r->GetRemoteName() == busName) {
            ep = r;
            break;
        }
    }
    if (!ep->IsValid()) {
        threadListLock.Unlock(MUTEX_CONTEXT);
    }
    return ep;
}


void BTTransport::ReturnEndpoint(RemoteEndpoint& ep) {
    if (threadList.find(ep) != threadList.end()) {
        threadListLock.Unlock(MUTEX_CONTEXT);
    }
}


QStatus BTTransport::IsMaster(const BDAddress& addr, bool& master) const
{
    return btAccessor->IsMaster(addr, master);
}


void BTTransport::RequestBTRole(const BDAddress& addr, bt::BluetoothRole role)
{
    btAccessor->RequestBTRole(addr, role);
}

bool BTTransport::IsEIRCapable() const
{
    return btAccessor->IsEIRCapable();
}

}
