/**
 * @file
 *
 * BusObject responsible for controlling/handling Bluetooth delegations and
 * implements the org.alljoyn.Bus.BTController interface.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <set>
#include <vector>

#include <qcc/Environ.h>
#include <qcc/GUID.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <alljoyn/AllJoynStd.h>

#include "BTController.h"
#include "BTEndpoint.h"

#define QCC_MODULE "ALLJOYN_BTC"


using namespace std;
using namespace qcc;

#define MethodHander(_a) static_cast<MessageReceiver::MethodHandler>(_a)
#define SignalHander(_a) static_cast<MessageReceiver::SignalHandler>(_a)
#define ReplyHander(_a) static_cast<MessageReceiver::ReplyHandler>(_a)

static const uint32_t ABSOLUTE_MAX_CONNECTIONS = 7; /* BT can't have more than 7 direct connections */
static const uint32_t DEFAULT_MAX_CONNECTIONS =  3; /* Gotta allow 1 connection for car-kit/headset/headphones */

/*
 * Timeout for detecting lost devices.  The nominal timeout is 60 seconds.
 * Absolute timing isn't critical so an additional 5 seconds is acctually
 * applied to when the alarm triggers.  This will allow lost device
 * expirations that are close to each other to be processed at the same time.
 * It also reduces the number of alarm resets if we get 2 updates within 5
 * seconds from the lower layer.
 */
static const uint32_t LOST_DEVICE_TIMEOUT = 60000;    /* 60 seconds */
static const uint32_t LOST_DEVICE_TIMEOUT_EXT = 5000; /* 5 seconds */

static const uint32_t BLACKLIST_TIME = (60 * 60 * 1000); /* 1 hour */

namespace ajn {

struct InterfaceDesc {
    AllJoynMessageType type;
    const char* name;
    const char* inputSig;
    const char* outSig;
    const char* argNames;
};

struct SignalEntry {
    const InterfaceDescription::Member* member;  /**< Pointer to signal's member */
    MessageReceiver::SignalHandler handler;      /**< Signal handler implementation */
};

static const char* bluetoothObjPath = "/org/alljoyn/Bus/BluetoothController";
static const char* bluetoothTopoMgrIfcName = "org.alljoyn.Bus.BluetoothController";

static const SessionOpts BTSESSION_OPTS(SessionOpts::TRAFFIC_MESSAGES,
                                        false,
                                        SessionOpts::PROXIMITY_ANY,
                                        TRANSPORT_BLUETOOTH);

#define SIG_ARRAY              "a"
#define SIG_ARRAY_SIZE         1
#define SIG_BDADDR             "t"
#define SIG_BDADDR_SIZE        1
#define SIG_DURATION           "u"
#define SIG_DURATION_SIZE      1
#define SIG_EIR_CAPABLE        "b"
#define SIG_EIR_CAPABLE_SIZE   1
#define SIG_GUID               "s"
#define SIG_GUID_SIZE          1
#define SIG_MINION_CNT         "y"
#define SIG_MINION_CNT_SIZE    1
#define SIG_NAME               "s"
#define SIG_NAME_SIZE          1
#define SIG_PSM                "q"
#define SIG_PSM_SIZE           1
#define SIG_SLAVE_FACTOR       "y"
#define SIG_SLAVE_FACTOR_SIZE  1
#define SIG_STATUS             "u"
#define SIG_STATUS_SIZE        1
#define SIG_UUIDREV            "u"
#define SIG_UUIDREV_SIZE       1

#define SIG_NAME_LIST               SIG_ARRAY SIG_NAME
#define SIG_NAME_LIST_SIZE          SIG_ARRAY_SIZE
#define SIG_BUSADDR                 SIG_BDADDR SIG_PSM
#define SIG_BUSADDR_SIZE            (SIG_BDADDR_SIZE + SIG_PSM_SIZE)
#define SIG_FIND_FILTER_LIST        SIG_ARRAY SIG_BDADDR
#define SIG_FIND_FILTER_LIST_SIZE   SIG_ARRAY_SIZE
#define SIG_AD_NAME_MAP_ENTRY       "(" SIG_GUID SIG_BUSADDR SIG_NAME_LIST ")"
#define SIG_AD_NAME_MAP_ENTRY_SIZE  1
#define SIG_AD_NAME_MAP             SIG_ARRAY SIG_AD_NAME_MAP_ENTRY
#define SIG_AD_NAME_MAP_SIZE        SIG_ARRAY_SIZE
#define SIG_AD_NAMES                SIG_NAME_LIST
#define SIG_AD_NAMES_SIZE           SIG_NAME_LIST_SIZE
#define SIG_FIND_NAMES              SIG_NAME_LIST
#define SIG_FIND_NAMES_SIZE         SIG_NAME_LIST_SIZE
#define SIG_NODE_STATE_ENTRY        "(" SIG_GUID SIG_NAME SIG_BUSADDR SIG_AD_NAMES SIG_FIND_NAMES SIG_EIR_CAPABLE ")"
#define SIG_NODE_STATE_ENTRY_SIZE   1
#define SIG_NODE_STATES             SIG_ARRAY SIG_NODE_STATE_ENTRY
#define SIG_NODE_STATES_SIZE        SIG_ARRAY_SIZE
#define SIG_FOUND_NODE_ENTRY        "(" SIG_BUSADDR SIG_UUIDREV SIG_AD_NAME_MAP ")"
#define SIG_FOUND_NODE_ENTRY_SIZE   1
#define SIG_FOUND_NODES             SIG_ARRAY SIG_FOUND_NODE_ENTRY
#define SIG_FOUND_NODES_SIZE        SIG_ARRAY_SIZE

#define SIG_SET_STATE_IN            SIG_MINION_CNT SIG_SLAVE_FACTOR SIG_EIR_CAPABLE SIG_UUIDREV SIG_BUSADDR SIG_NODE_STATES SIG_FOUND_NODES
#define SIG_SET_STATE_IN_SIZE       (SIG_MINION_CNT_SIZE + SIG_SLAVE_FACTOR_SIZE + SIG_EIR_CAPABLE_SIZE + SIG_UUIDREV_SIZE + SIG_BUSADDR_SIZE + SIG_NODE_STATES_SIZE + SIG_FOUND_NODES_SIZE)
#define SIG_SET_STATE_OUT           SIG_EIR_CAPABLE SIG_UUIDREV SIG_BUSADDR SIG_NODE_STATES SIG_FOUND_NODES
#define SIG_SET_STATE_OUT_SIZE      (SIG_EIR_CAPABLE_SIZE + SIG_UUIDREV_SIZE + SIG_BUSADDR_SIZE + SIG_NODE_STATES_SIZE + SIG_FOUND_NODES_SIZE)
#define SIG_NAME_OP                 SIG_BUSADDR SIG_NAME
#define SIG_NAME_OP_SIZE            (SIG_BUSADDR_SIZE + SIG_NAME_SIZE)
#define SIG_DELEGATE_AD             SIG_UUIDREV SIG_BUSADDR SIG_AD_NAME_MAP SIG_DURATION
#define SIG_DELEGATE_AD_SIZE        (SIG_UUIDREV_SIZE + SIG_BUSADDR_SIZE + SIG_AD_NAME_MAP_SIZE + SIG_DURATION_SIZE)
#define SIG_DELEGATE_AD_DURATION_PARAM (SIG_UUIDREV_SIZE + SIG_BUSADDR_SIZE + SIG_AD_NAME_MAP_SIZE)
#define SIG_DELEGATE_FIND           SIG_FIND_FILTER_LIST SIG_DURATION
#define SIG_DELEGATE_FIND_SIZE      (SIG_FIND_FILTER_LIST_SIZE + SIG_DURATION_SIZE)
#define SIG_FOUND_NAMES             SIG_FOUND_NODES
#define SIG_FOUND_NAMES_SIZE        (SIG_FOUND_NODES_SIZE)
#define SIG_FOUND_DEV               SIG_BDADDR SIG_UUIDREV SIG_EIR_CAPABLE
#define SIG_FOUND_DEV_SIZE          (SIG_BDADDR_SIZE + SIG_UUIDREV_SIZE + SIG_EIR_CAPABLE_SIZE)
#define SIG_CONN_ADDR_CHANGED       SIG_BUSADDR SIG_BUSADDR
#define SIG_CONN_ADDR_CHANGED_SIZE  (SIG_BUSADDR_SIZE + SIG_BUSADDR_SIZE)

const InterfaceDesc btmIfcTable[] = {
    /* Methods */
    { MESSAGE_METHOD_CALL, "SetState", SIG_SET_STATE_IN, SIG_SET_STATE_OUT, "minionCnt,slaveFactor,eirCapable,uuidRev,busAddr,psm,nodeStates,foundNodes,eirCapable,uuidRev,busAddr,psm,nodeStates,foundNodes" },

    /* Signals */
    { MESSAGE_SIGNAL, "FindName",            SIG_NAME_OP,           NULL, "requestorAddr,requestorPSM,findName" },
    { MESSAGE_SIGNAL, "CancelFindName",      SIG_NAME_OP,           NULL, "requestorAddr,requestorPSM,findName" },
    { MESSAGE_SIGNAL, "AdvertiseName",       SIG_NAME_OP,           NULL, "requestorAddr,requestorPSM,adName" },
    { MESSAGE_SIGNAL, "CancelAdvertiseName", SIG_NAME_OP,           NULL, "requestorAddr,requestorPSM,adName" },
    { MESSAGE_SIGNAL, "DelegateAdvertise",   SIG_DELEGATE_AD,       NULL, "uuidRev,bdAddr,psm,adNames,duration" },
    { MESSAGE_SIGNAL, "DelegateFind",        SIG_DELEGATE_FIND,     NULL, "ignoreBDAddr,duration" },
    { MESSAGE_SIGNAL, "FoundNames",          SIG_FOUND_NAMES,       NULL, "adNamesTable" },
    { MESSAGE_SIGNAL, "LostNames",           SIG_FOUND_NAMES,       NULL, "adNamesTable" },
    { MESSAGE_SIGNAL, "FoundDevice",         SIG_FOUND_DEV,         NULL, "bdAddr,uuidRev,eirCapable" },
    { MESSAGE_SIGNAL, "ConnectAddrChanged",  SIG_CONN_ADDR_CHANGED, NULL, "oldBDAddr,oldPSM,newBDAddr,newPSM" },
};


BTController::BTController(BusAttachment& bus, BluetoothDeviceInterface& bt) :
    BusObject(bus, bluetoothObjPath),
#ifndef NDEBUG
    dbgIface(this),
    discoverTimer(dbgIface.LookupTimingProperty("DiscoverTimes")),
    sdpQueryTimer(dbgIface.LookupTimingProperty("SDPQueryTimes")),
    connectTimer(dbgIface.LookupTimingProperty("ConnectTimes")),
#endif
    bus(bus),
    bt(bt),
    master(NULL),
    masterUUIDRev(bt::INVALID_UUIDREV),
    directMinions(0),
    maxConnections(min(StringToU32(Environ::GetAppEnviron()->Find("ALLJOYN_MAX_BT_CONNECTIONS"), 0, DEFAULT_MAX_CONNECTIONS),
                       ABSOLUTE_MAX_CONNECTIONS)),
    listening(false),
    devAvailable(false),
    foundNodeDB(true),
    advertise(*this),
    find(*this),
    dispatcher("BTC-Dispatcher"),
    incompleteConnections(0)
{
    while (masterUUIDRev == bt::INVALID_UUIDREV) {
        masterUUIDRev = qcc::Rand32();
    }

    const InterfaceDescription* ifc = NULL;
    InterfaceDescription* newIfc;
    QStatus status = bus.CreateInterface(bluetoothTopoMgrIfcName, newIfc);
    if (status == ER_OK) {
        for (size_t i = 0; i < ArraySize(btmIfcTable); ++i) {
            newIfc->AddMember(btmIfcTable[i].type,
                              btmIfcTable[i].name,
                              btmIfcTable[i].inputSig,
                              btmIfcTable[i].outSig,
                              btmIfcTable[i].argNames,
                              0);
        }
        newIfc->Activate();
        ifc = newIfc;
    } else if (status == ER_BUS_IFACE_ALREADY_EXISTS) {
        ifc = bus.GetInterface(bluetoothTopoMgrIfcName);
    }

    if (ifc) {
        org.alljoyn.Bus.BTController.interface =           ifc;
        org.alljoyn.Bus.BTController.SetState =            ifc->GetMember("SetState");
        org.alljoyn.Bus.BTController.FindName =            ifc->GetMember("FindName");
        org.alljoyn.Bus.BTController.CancelFindName =      ifc->GetMember("CancelFindName");
        org.alljoyn.Bus.BTController.AdvertiseName =       ifc->GetMember("AdvertiseName");
        org.alljoyn.Bus.BTController.CancelAdvertiseName = ifc->GetMember("CancelAdvertiseName");
        org.alljoyn.Bus.BTController.DelegateAdvertise =   ifc->GetMember("DelegateAdvertise");
        org.alljoyn.Bus.BTController.DelegateFind =        ifc->GetMember("DelegateFind");
        org.alljoyn.Bus.BTController.FoundNames =          ifc->GetMember("FoundNames");
        org.alljoyn.Bus.BTController.LostNames =           ifc->GetMember("LostNames");
        org.alljoyn.Bus.BTController.FoundDevice =         ifc->GetMember("FoundDevice");
        org.alljoyn.Bus.BTController.ConnectAddrChanged =  ifc->GetMember("ConnectAddrChanged");

        advertise.delegateSignal = org.alljoyn.Bus.BTController.DelegateAdvertise;
        find.delegateSignal = org.alljoyn.Bus.BTController.DelegateFind;

        static_cast<DaemonRouter&>(bus.GetInternal().GetRouter()).AddBusNameListener(this);
    }

    // Setup the BT node info for ourself.
    self->SetGUID(bus.GetGlobalGUIDString());
    self->SetRelationship(_BTNodeInfo::SELF);
    advertise.minion = self;
    find.minion = self;

    dispatcher.Start();
}


BTController::~BTController()
{
    // Don't need to remove our bus name change listener from the router (name
    // table) since the router is already destroyed at this point in time.

    dispatcher.Stop();
    dispatcher.Join();

    if (advertise.active && (advertise.minion == self)) {
        QCC_DbgPrintf(("Stopping local advertise..."));
        advertise.StopLocal();
    }

    if (find.active && (find.minion == self)) {
        QCC_DbgPrintf(("Stopping local find..."));
        find.StopLocal();
    }

    bus.UnregisterBusObject(*this);
    if (master) {
        delete master;
    }
}


void BTController::ObjectRegistered() {
    // Set our unique name now that we know it.
    self->SetUniqueName(bus.GetUniqueName());
}


QStatus BTController::Init()
{
    if (org.alljoyn.Bus.BTController.interface == NULL) {
        QCC_LogError(ER_FAIL, ("Bluetooth topology manager interface not setup"));
        return ER_FAIL;
    }

    AddInterface(*org.alljoyn.Bus.BTController.interface);

    const MethodEntry methodEntries[] = {
        { org.alljoyn.Bus.BTController.SetState,        MethodHandler(&BTController::HandleSetState) },
    };

    const SignalEntry signalEntries[] = {
        { org.alljoyn.Bus.BTController.FindName,            SignalHandler(&BTController::HandleNameSignal) },
        { org.alljoyn.Bus.BTController.CancelFindName,      SignalHandler(&BTController::HandleNameSignal) },
        { org.alljoyn.Bus.BTController.AdvertiseName,       SignalHandler(&BTController::HandleNameSignal) },
        { org.alljoyn.Bus.BTController.CancelAdvertiseName, SignalHandler(&BTController::HandleNameSignal) },
        { org.alljoyn.Bus.BTController.DelegateAdvertise,   SignalHandler(&BTController::HandleDelegateOp) },
        { org.alljoyn.Bus.BTController.DelegateFind,        SignalHandler(&BTController::HandleDelegateOp) },
        { org.alljoyn.Bus.BTController.FoundNames,          SignalHandler(&BTController::HandleFoundNamesChange) },
        { org.alljoyn.Bus.BTController.LostNames,           SignalHandler(&BTController::HandleFoundNamesChange) },
        { org.alljoyn.Bus.BTController.FoundDevice,         SignalHandler(&BTController::HandleFoundDeviceChange) },
        { org.alljoyn.Bus.BTController.ConnectAddrChanged,  SignalHandler(&BTController::HandleConnectAddrChanged) },
    };

    QStatus status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));

    for (size_t i = 0; (status == ER_OK) && (i < ArraySize(signalEntries)); ++i) {
        status = bus.RegisterSignalHandler(this, signalEntries[i].handler, signalEntries[i].member, bluetoothObjPath);
    }

    if (status == ER_OK) {
        status = bus.RegisterBusObject(*this);
    }

    return status;
}


QStatus BTController::AddAdvertiseName(const qcc::String& name)
{
    QStatus status = DoNameOp(name, *org.alljoyn.Bus.BTController.AdvertiseName, true, advertise);

    lock.Lock(MUTEX_CONTEXT);
    bool isMaster = IsMaster();
    bool lDevAvailable = devAvailable;
    BTBusAddress addr = self->GetBusAddress();
    lock.Unlock(MUTEX_CONTEXT);

    if (isMaster && (status == ER_OK)) {
        if (lDevAvailable) {
            BTNodeDB newAdInfo;
            BTNodeInfo node = self->Clone();  // make an actual copy of self
            node->AddAdvertiseName(name);     // clone of self only gets the new names (not all existing names)
            newAdInfo.AddNode(node);
            DistributeAdvertisedNameChanges(&newAdInfo, NULL);
        }
    }

    return status;
}


QStatus BTController::RemoveAdvertiseName(const qcc::String& name)
{
    QStatus status = DoNameOp(name, *org.alljoyn.Bus.BTController.CancelAdvertiseName, false, advertise);

    lock.Lock(MUTEX_CONTEXT);
    bool isMaster = IsMaster();
    bool lDevAvailable = devAvailable;
    BTBusAddress addr = self->GetBusAddress();
    lock.Unlock(MUTEX_CONTEXT);

    if (isMaster && (status == ER_OK)) {
        if (lDevAvailable) {
            BTNodeDB oldAdInfo;
            BTNodeInfo node = self->Clone();  // make an actual copy of self
            node->AddAdvertiseName(name);     // clone of self only gets the new names (not all existing names)
            oldAdInfo.AddNode(node);
            DistributeAdvertisedNameChanges(NULL, &oldAdInfo);
        }
    }

    return status;
}


QStatus BTController::RemoveFindName(const qcc::String& name)
{
    return DoNameOp(name, *org.alljoyn.Bus.BTController.CancelFindName, false, find);
}


void BTController::ProcessDeviceChange(const BDAddress& adBdAddr,
                                       uint32_t uuidRev,
                                       bool eirCapable)
{
    QCC_DbgTrace(("BTController::ProcessDeviceChange(adBdAddr = %s, uuidRev = %08x)",
                  adBdAddr.ToString().c_str(), uuidRev));

    // This gets called when the BTAccessor layer detects either a new
    // advertising device and/or a new uuidRev for associated with that
    // advertising device.

    assert(!eirCapable || uuidRev != bt::INVALID_UUIDREV);
    assert(adBdAddr.GetRaw() != static_cast<uint64_t>(0));

    QStatus status;

    lock.Lock(MUTEX_CONTEXT);
    if (IsMaster()) {
        if (nodeDB.FindNode(adBdAddr)->IsValid()) {
            /*
             * Normally the minion will not send us advertisements from nodes
             * that are connected to us since they contain our advertisements.
             * That said however, there is a race condition when establishing
             * a connection with a remote device where our find minion
             * received a device found indication for the device that we are
             * in the process of connecting to.  That minion may even see the
             * new UUIDRev for that newly connected remote device in its EIR
             * packets.  This will of course cause our minion to notify us
             * since we haven't told our minion to ingore that device's BD
             * Address yet (the connection may fail).  The most expedient
             * thing is to just ignore found device notification for devices
             * that we know are connected to us.
             */
            lock.Unlock(MUTEX_CONTEXT);
            return;
        }

        BTNodeInfo adNode = foundNodeDB.FindNode(adBdAddr);
        BTNodeDB newAdInfo;
        BTNodeDB oldAdInfo;
        BTNodeDB added;
        BTNodeDB removed;
        bool distributeChanges = false;

        bool knownAdNode = adNode->IsValid();

        // Get SDP record information if both devices are EIR capable and the
        // advertising device is either unknown or its UUID Rev changed.  Also
        // get the SDP record information if either device is not EIR capable
        // and the advertising device is unknown.
        bool getInfo = ((bt.IsEIRCapable() && (eirCapable || (knownAdNode && adNode->IsEIRCapable())) &&
                         (!knownAdNode || (adNode->GetUUIDRev() != uuidRev))) ||
                        ((!bt.IsEIRCapable() || !eirCapable) && !knownAdNode));

        // Only refresh exipriation times for EIR capable devices.  This will
        // cause us to poll SDP information every 60 seconds for non-EIR
        // devices.
        bool refreshExpiration = (bt.IsEIRCapable() &&
                                  knownAdNode &&
                                  eirCapable &&
                                  (adNode->GetUUIDRev() == uuidRev));

        if (refreshExpiration) {

            if (!adNode->IsEIRCapable()) {
                adNode->SetEIRCapable(eirCapable);
            }

            // We've see this advertising node before and nothing has changed
            // so just refresh the expiration time of all the nodes.
            foundNodeDB.RefreshExpiration(adNode->GetConnectNode(), LOST_DEVICE_TIMEOUT);
            String msg = String("foundNodeDB: Refresh Expiration for nodes with connect address: ") + adNode->GetConnectNode()->ToString();
            foundNodeDB.DumpTable(msg.c_str());
            ResetExpireNameAlarm();

        } else if (getInfo) {
            uint32_t newUUIDRev;
            BTBusAddress connAddr;

            if (!knownAdNode && !eirCapable && (blacklist->find(adBdAddr) != blacklist->end())) {
                lock.Unlock(MUTEX_CONTEXT);
                return; // blacklisted - ignore it
            }

            QCC_DbgPrintf(("Getting device info from %s (adNode: %s in foundNodeDB, adNode %s EIR capable, received %s EIR capable, adNode UUIDRev: %08x, received UUIDRev: %08x)",
                           adBdAddr.ToString().c_str(),
                           knownAdNode ? "is" : "is not",
                           adNode->IsEIRCapable() ? "is" : "is not",
                           eirCapable ? "is" : "is not",
                           adNode->GetUUIDRev(),
                           uuidRev));

            QCC_DEBUG_ONLY(sdpQueryStartTime = sdpQueryTimer.StartTime());
            lock.Unlock(MUTEX_CONTEXT);
            status = bt.GetDeviceInfo(adBdAddr, newUUIDRev, connAddr, newAdInfo);
            lock.Lock(MUTEX_CONTEXT);
            QCC_DEBUG_ONLY(sdpQueryTimer.RecordTime(adBdAddr, sdpQueryStartTime));

            // Make sure we are still master
            if (IsMaster()) {
                if ((status != ER_OK) || !connAddr.IsValid()) {
                    if (!eirCapable) {
                        uint32_t blacklistTime = BLACKLIST_TIME + (Rand32() % BLACKLIST_TIME);
                        QCC_DbgPrintf(("Blacklisting %s for %d.%03ds",
                                       adBdAddr.ToString().c_str(),
                                       blacklistTime / 1000, blacklistTime % 1000));
                        blacklist->insert(adBdAddr);
                        DispatchOperation(new ExpireBlacklistedDevDispatchInfo(adBdAddr), blacklistTime);

                        // Gotta add the new blacklist entry to ignore addresses set.
                        find.dirty = true;
                        DispatchOperation(new UpdateDelegationsDispatchInfo());
                    }
                    lock.Unlock(MUTEX_CONTEXT);
                    return;
                }

                if (nodeDB.FindNode(connAddr)->IsValid()) {
                    // Already connected.
                    lock.Unlock(MUTEX_CONTEXT);
                    return;
                }

                if (newAdInfo.FindNode(self->GetBusAddress())->IsValid()) {
                    QCC_DbgPrintf(("Device %s is advertising a set of nodes that include our own BD Address, ignoring it for now.", adBdAddr.ToString().c_str()));
                    // Clear out the newAdInfo DB then re-add minimal
                    // information about the advertising node so that we'll
                    // ignore it until its UUID revision changes.
                    BTNodeInfo n(newAdInfo.FindNode(adBdAddr)->GetBusAddress());
                    n->SetEIRCapable(eirCapable || adNode->IsEIRCapable());
                    newAdInfo.Clear();
                    newAdInfo.AddNode(n);
                }

                BTNodeInfo newConnNode = newAdInfo.FindNode(connAddr);
                if (!newConnNode->IsValid()) {
                    QCC_LogError(ER_FAIL, ("No device with connect address %s in advertisement",
                                           connAddr.ToString().c_str()));
                    lock.Unlock(MUTEX_CONTEXT);
                    return;
                }

                foundNodeDB.Lock(MUTEX_CONTEXT);

                if (knownAdNode) {
                    foundNodeDB.GetNodesFromConnectNode(adNode->GetConnectNode(), oldAdInfo);
                } else {
                    QCC_DEBUG_ONLY(discoverTimer.RecordTime(adBdAddr, discoverStartTime));
                    adNode = newAdInfo.FindNode(adBdAddr);
                }

                // We actually want the nodes in newAdInfo to use the existing
                // node in foundNodeDB if it exists.  This will ensure a
                // consistent foundNodeDB and allow operations like
                // RefreshExpireTime() and GetNodesFromConnectAddr() to work
                // properly.
                BTNodeInfo connNode = foundNodeDB.FindNode(connAddr);
                if (!connNode->IsValid()) {
                    connNode = newConnNode;
                }

                BTNodeDB::const_iterator nodeit;
                for (nodeit = newAdInfo.Begin(); nodeit != newAdInfo.End(); ++nodeit) {
                    BTNodeInfo node = *nodeit;
                    BTNodeInfo fnode = foundNodeDB.FindNode(node->GetBusAddress());
                    assert(connNode->IsValid());
                    node->SetConnectNode(connNode);
                    if (node->GetBusAddress().addr == adBdAddr) {
                        node->SetEIRCapable(eirCapable);
                    }
                    if (fnode->IsValid()) {
                        // Node is already known to us, so update the connect
                        // node and EIR capability as appropriate.
                        foundNodeDB.RemoveNode(fnode);
                        fnode->SetConnectNode(connNode);
                        if (fnode->GetBusAddress().addr == adBdAddr) {
                            fnode->SetEIRCapable(eirCapable);
                        }
                        foundNodeDB.AddNode(fnode);
                    }
                }

                oldAdInfo.Diff(newAdInfo, &added, &removed);

                foundNodeDB.UpdateDB(&added, &removed, false);  // Remove names only.

                BTNodeDB removedNodes;  // Don't re-use removed since it is still used later.
                oldAdInfo.NodeDiff(newAdInfo, NULL, &removedNodes);  // Remove nodes that are actually gone.
                foundNodeDB.UpdateDB(NULL, &removedNodes);


                connNode->SetUUIDRev(newUUIDRev);
                if (!foundNodeDB.FindNode(connNode->GetBusAddress())->IsValid()) {
                    // connNode somehow got removed from foundNodeDB.  Need to add it back in.
                    foundNodeDB.AddNode(connNode);
                }
                foundNodeDB.RefreshExpiration(connNode, LOST_DEVICE_TIMEOUT);
                foundNodeDB.DumpTable("foundNodeDB - Updated set of found devices due to remote device advertisement change");

                foundNodeDB.Unlock(MUTEX_CONTEXT);

                distributeChanges = true;
                ResetExpireNameAlarm();
            }
        }

        lock.Unlock(MUTEX_CONTEXT);

        if (distributeChanges) {
            DistributeAdvertisedNameChanges(&added, &removed);
        }
    } else {
        MsgArg args[SIG_FOUND_DEV_SIZE];
        size_t numArgs = ArraySize(args);

        status = MsgArg::Set(args, numArgs, SIG_FOUND_DEV, adBdAddr.GetRaw(), uuidRev, eirCapable);

        lock.Unlock(MUTEX_CONTEXT);

        if (status != ER_OK) {
            QCC_LogError(status, ("MsgArg::Set(args = <>, numArgs = %u, %s, %s, %08x, <%s>) failed",
                                  numArgs, SIG_FOUND_DEV, adBdAddr.ToString().c_str(), uuidRev, eirCapable ? "true" : "false"));
            return;
        }

        status = Signal(masterNode->GetUniqueName().c_str(), masterNode->GetSessionID(), *org.alljoyn.Bus.BTController.FoundDevice, args, numArgs);
    }
}


BTNodeInfo BTController::PrepConnect(const BTBusAddress& addr, const String& redirection)
{
    BTNodeInfo node;

    bool repeat;
    bool newDevice;

    if (addr == self->GetBusAddress()) {
        // If a remote device has a stale advertisement, it may try to
        // establish a session based on that advertisement, but because the
        // advertised name is no longer valid, the session management code may
        // try to tell us to connect to ourself.  Returning an invalid node
        // here will cause the connection to fail which is the proper thing to
        // do in this case.
        QCC_LogError(ER_FAIL, ("Attempt to connect to ourself (%s)", addr.ToString().c_str()));
        return node;
    }

    do {
        repeat = false;
        newDevice = false;

        lock.Lock(MUTEX_CONTEXT);
        if (!IsMinion()) {
            node = nodeDB.FindNode(addr);
            if (IsMaster() && !node->IsValid() && ((nodeDB.Size() - 1) < maxConnections)) {
                if (redirection.empty()) {
                    node = foundNodeDB.FindNode(addr);
                    newDevice = node->IsValid() && (node != joinSessionNode);
                } else {
                    foundNodeDB.Lock();

                    /*
                     * We need a redirect node to be in one of the node DBs.  Since we may not have
                     * found it before we got redirected, put it in.
                     */
                    BTBusAddress redirAddr(redirection);
                    BTNodeInfo redirNode = nodeDB.FindNode(redirAddr);
                    if (!redirNode->IsValid()) {
                        redirNode = foundNodeDB.FindNode(redirAddr);
                    }
                    if (!redirNode->IsValid()) {
                        redirNode = BTNodeInfo(redirAddr);
                        Timespec now;
                        GetTimeNow(&now);
                        redirNode->SetExpireTime(now.GetAbsoluteMillis() + 5000);
                        /*
                         * The GUID of the redirection node is invalid.  However as soon as we do
                         * the SetState handshake we will get the correct GUID (or it is found via
                         * SDP query).
                         */
                        foundNodeDB.AddNode(redirNode);
                    }

                    node = foundNodeDB.FindNode(addr);
                    if (node->IsValid()) {
                        /*
                         * A redirection is essentially telling us that our connect node information
                         * is stale.  Update that info now.
                         */
                        BTNodeInfo connNode = node->GetConnectNode();
                        connNode->SetConnectNode(redirNode);
                    } else {
                        /*
                         * It's possible that our target node is gone due to the name expiring
                         * between the original connect attempt and the redirected connect attempt.
                         * So just go straight to the redirection node and the name will be found
                         * again during the SetState handshake if it's still being advertised.
                         */
                        node = redirNode;
                    }
                    newDevice = node->IsValid() && (node != joinSessionNode);
                    foundNodeDB.Unlock();
                }
            }
        }

        if (!IsMaster() && !node->IsValid()) {
            node = masterNode;
        }
        lock.Unlock(MUTEX_CONTEXT);

        if (newDevice) {
            int ic = IncrementAndFetch(&incompleteConnections);
            QCC_DbgPrintf(("incompleteConnections = %d", ic));
            assert(ic > 0);
            if (ic > 1) {
                // Serialize creating new ACLs.
                QStatus status = Event::Wait(connectCompleted);
                QCC_DbgPrintf(("received connect completed event"));
                connectCompleted.ResetEvent();
                node = BTNodeInfo();
                if (status != ER_OK) {
                    return node;  // Fail the connection (probably shutting down anyway).
                } else {
                    repeat = true;
                    ic = DecrementAndFetch(&incompleteConnections);
                    QCC_DbgPrintf(("incompleteConnections = %d", ic));
                    assert(ic >= 0);
                    if (ic > 0) {
                        connectCompleted.SetEvent();
                    }
                }
            } else {
                joinSessionNode = node->GetConnectNode();
                QCC_DbgPrintf(("joinSessionNode set to %s", joinSessionNode->ToString().c_str()));
            }
        }
    } while (repeat);


    if (newDevice) {
        if ((find.minion == self) && find.active) {
            /*
             * Gotta shut down the local find operation since the exchange of
             * the SetState method call and response which will result in one
             * side or the other taking control of who performs the find
             * operation.
             */
            QCC_DbgPrintf(("Stopping local find..."));
            find.StopLocal();
        }
        if ((advertise.minion == self) && advertise.active) {
            /*
             * Gotta shut down the local advertise operation since the
             * exchange for the SetState method call and response which will
             * result in one side or the other taking control of who performs
             * the advertise operation.
             */
            QCC_DbgPrintf(("Stopping local advertise..."));
            advertise.StopLocal();
        }
    }


    QCC_DEBUG_ONLY(connectStartTimes[node->GetBusAddress().addr] = connectTimer.StartTime());

    QCC_DbgPrintf(("Connect address %s for %s (addr = %s) is %s as %s  (nodeDB size = %d  maxConnections = %d)",
                   node->GetConnectNode()->ToString().c_str(),
                   node->ToString().c_str(),
                   addr.ToString().c_str(),
                   !node->IsValid() ? "<unknown>" :
                   (foundNodeDB.FindNode(addr) == node) ? "in foundNodeDB" :
                   ((nodeDB.FindNode(addr) == node) ? "in nodeDB" :
                    ((node == masterNode) ? "masterNode" : "<impossible>")),
                   IsMaster() ? "Master" : (IsDrone() ? "Drone" : (IsMinion() ? "Minion" : "<invalid>")),
                   nodeDB.Size(), maxConnections));

    return node->GetConnectNode();
}


void BTController::PostConnect(QStatus status, BTNodeInfo& node, const String& remoteName)
{
    QCC_DbgPrintf(("Connect to %s resulted in status %s, remoteName = \"%s\"", node->ToString().c_str(), QCC_StatusText(status), remoteName.c_str()));

    if (status == ER_OK) {
        QCC_DEBUG_ONLY(connectTimer.RecordTime(node->GetBusAddress().addr, connectStartTimes[node->GetBusAddress().addr]));
        assert(!remoteName.empty());
        if (node->GetUniqueName().empty() || (node->GetUniqueName() != remoteName)) {
            node->SetUniqueName(remoteName);
        }

        bool inNodeDB = nodeDB.FindNode(node->GetBusAddress())->IsValid();

        /* Only call JoinSessionAsync for new outgoing connections where we
         * didn't already start the join session process.
         */
        QCC_DbgPrintf(("inNodeDB = %d   (node == joinSessionNode) => %d   node->GetSessionState() = %d",
                       inNodeDB, (node == joinSessionNode), node->GetSessionState()));
        if ((node == joinSessionNode) && (node->GetSessionState() == _BTNodeInfo::NO_SESSION)) {
            assert(node.iden(joinSessionNode));
            if (IsMaster() && !inNodeDB) {
                QCC_DbgPrintf(("Joining BT topology manager session for %s", node->ToString().c_str()));
                node->SetSessionState(_BTNodeInfo::JOINING_SESSION);

                status = bus.JoinSessionAsync(remoteName.c_str(),
                                              ALLJOYN_BTCONTROLLER_SESSION_PORT,
                                              NULL,
                                              BTSESSION_OPTS,
                                              this);
                if (status != ER_OK) {
                    joinSessionNode->SetSessionState(_BTNodeInfo::NO_SESSION);
                    JoinSessionNodeComplete();
                }
            } else {
                JoinSessionNodeComplete();
            }
        }
    } else {
        if ((node == joinSessionNode) && (node->GetConnectionCount() == 0)) {
            JoinSessionNodeComplete();
        }

        foundNodeDB.Lock(MUTEX_CONTEXT);
        if (foundNodeDB.FindNode(node->GetBusAddress())->IsValid() && IsMaster()) {
            // Failed to connect to the device.  Send out a lost advertised
            // name for all names in all nodes connectable via this node so
            // that if we find the name of interest again, we will send out a
            // found advertised name and the client app can try to connect
            // again.
            BTNodeDB reapDB;
            foundNodeDB.GetNodesFromConnectNode(node, reapDB);
            foundNodeDB.UpdateDB(NULL, &reapDB);
            foundNodeDB.Unlock(MUTEX_CONTEXT);

            QCC_LogError(status, ("Connection failed to %s, removing found names", node->ToString().c_str()));
            DistributeAdvertisedNameChanges(NULL, &reapDB);
        } else {
            foundNodeDB.Unlock(MUTEX_CONTEXT);
            QCC_LogError(status, ("Connection failed to %s", node->ToString().c_str()));
        }
    }
}


void BTController::LostLastConnection(const BTNodeInfo& node)
{
    QCC_DbgTrace(("BTController::LostLastConnection(node = %s)",
                  node->ToString().c_str()));

    BTNodeInfo lostNode;

    if (node->GetBusAddress().psm == bt::INCOMING_PSM) {
        if (node->GetBusAddress().addr == masterNode->GetBusAddress().addr) {
            lostNode = masterNode;
        } else {
            BTNodeDB::const_iterator it;
            BTNodeDB::const_iterator end;
            nodeDB.Lock(MUTEX_CONTEXT);
            nodeDB.FindNodes(node->GetBusAddress().addr, it, end);
            for (; it != end; ++it) {
                if ((*it)->GetConnectionCount() == 1) {
                    lostNode = *it;
                    break;
                }
            }
            nodeDB.Unlock(MUTEX_CONTEXT);
        }
    } else {
        lostNode = node;
    }

    if (lostNode->IsValid()) {
        SessionId sessionID = lostNode->GetSessionID();
        nodeDB.NodeSessionLost(sessionID);
        bus.LeaveSession(sessionID);
    }
}


void BTController::BTDeviceAvailable(bool on)
{
    QCC_DbgTrace(("BTController::BTDeviceAvailable(<%s>)", on ? "on" : "off"));
    DispatchOperation(new BTDevAvailDispatchInfo(on));
}


bool BTController::CheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const
{
    QCC_DbgTrace(("BTController::CheckIncomingAddress(addr = %s)", addr.ToString().c_str()));
    if (IsMaster()) {
        redirectAddr = BTBusAddress(); // Master redirects to no one.

        const BTNodeInfo& node = nodeDB.FindNode(addr);
        if (node->IsValid()) {
            bool allow  = node->IsDirectMinion();
            QCC_DbgPrintf(("%s incoming connection from %s minion as master.",
                           allow ? "Accept" : "Reject",
                           allow ? "a direct" : "an indirect"));
            return allow;
        } else if (incompleteConnections > 0) {
            bool allow = joinSessionNode->GetBusAddress().addr == addr;
            QCC_DbgPrintf(("%s incoming connection from a new remote device while we are creating a new outgoing connection to %s device as master.",
                           allow ? "Accept" : "Reject",
                           allow ? "the same" : "a different"));
            return allow;
        } else if ((nodeDB.Size() - 1) >= maxConnections) {
            QCC_DbgPrintf(("Reject incomming connection from new device since we've reached our max connections."));
            return false;
        } else {
            QCC_DbgPrintf(("Accept incoming connection as master."));
            return true;
        }

    } else if (addr == masterNode->GetBusAddress().addr) {
        QCC_DbgPrintf(("Always accept incoming connection from our master."));
        return true;

    } else if (IsDrone()) {
        const BTNodeInfo& node = nodeDB.FindNode(addr);
        bool redirect = !node->IsValid();
        bool allow = redirect || node->IsDirectMinion();
        QCC_DbgPrintf(("% incoming connection from %s %s.",
                       redirect ? "Redirect" : (allow ? "Accept" : "Reject"),
                       node->IsValid() ?
                       (node->IsDirectMinion() ? "direct" : "indirect") : "unknown node:",
                       node->IsValid() ? "minion" : addr.ToString().c_str()));
        redirectAddr = redirect ? masterNode->GetBusAddress() : BTBusAddress();
        return allow;
    }

    QCC_DbgPrintf(("Redirect incoming connection from %s because we are a minion (our master is %s).",
                   addr.ToString().c_str(),
                   masterNode->GetBusAddress().addr.ToString().c_str()));
    redirectAddr = masterNode->GetBusAddress();
    return true;
}


void BTController::NameOwnerChanged(const qcc::String& alias,
                                    const qcc::String* oldOwner,
                                    const qcc::String* newOwner)
{
    QCC_DbgTrace(("BTController::NameOwnerChanged(alias = %s, oldOwner = %s, newOwner = %s)",
                  alias.c_str(),
                  oldOwner ? oldOwner->c_str() : "<null>",
                  newOwner ? newOwner->c_str() : "<null>"));
    if (oldOwner && (alias == *oldOwner) && (alias != bus.GetUniqueName())) {
        DispatchOperation(new NameLostDispatchInfo(alias));
    } else if (!oldOwner && newOwner && (alias == org::alljoyn::Daemon::WellKnownName)) {
        /*
         * Need to bind the session port here instead of in the
         * ObjectRegistered() function since there is a race condition because
         * there is a race condition between which object will get registered
         * first: AllJoynObj or BTController.  Since AllJoynObj must be
         * registered before we can bind the session port we wait for
         * AllJoynObj to acquire its well known name.
         */
        SessionPort port = ALLJOYN_BTCONTROLLER_SESSION_PORT;
        QStatus status = bus.BindSessionPort(port, BTSESSION_OPTS, *this);
        if (status != ER_OK) {
            QCC_LogError(status, ("BindSessionPort(port = %04x, opts = <%x, %x, %x>, listener = %p)",
                                  port,
                                  BTSESSION_OPTS.traffic, BTSESSION_OPTS.proximity, BTSESSION_OPTS.transports,
                                  this));
        }
    }
}


bool BTController::AcceptSessionJoiner(SessionPort sessionPort,
                                       const char* joiner,
                                       const SessionOpts& opts)
{
    bool accept = (sessionPort == ALLJOYN_BTCONTROLLER_SESSION_PORT) && BTSESSION_OPTS.IsCompatible(opts);
    String uniqueName(joiner);
    BTNodeInfo node = nodeDB.FindNode(uniqueName);

    QCC_DbgPrintf(("SJK: accept = %d", accept));

    if (accept) {
        RemoteEndpoint ep = bt.LookupEndpoint(uniqueName);

        /* We only accept sessions from joiners who meet the following criteria:
         * - The endpoint is a Bluetooth endpoint (endpoint lookup succeeds).
         * - Is not already connected to us (sessionID is 0).
         */
        accept = (ep->IsValid() &&
                  (!node->IsValid() || (node->GetSessionID() == 0)));

        QCC_DbgPrintf(("SJK: accept = %d  (ep=%p  node->IsValid()=%d  node->GetSessionID()=%08x)", accept, &(*ep), node->IsValid(), node->GetSessionID()));

        if (ep->IsValid()) {
            bt.ReturnEndpoint(ep);
        }
    }

    if (accept) {
        /* If we happen to be joining the joiner at the same time then we need
         * to figure out which session will be rejected.  The deciding factor
         * will be who's unique name is "less".  (The unique names should
         * never be equal, but we'll reject those just in case.)
         */
        if ((joinSessionNode->GetUniqueName() == uniqueName) && !(uniqueName < bus.GetUniqueName())) {
            accept = false;
            QCC_DbgPrintf(("SJK: accept = %d   uniqueName = '%s'   bus.GetUniqueName() = '%s'", accept, uniqueName.c_str(), bus.GetUniqueName().c_str()));
        }
    }

    QCC_DbgPrintf(("%s session join from %s",
                   accept ? "Accepting" : "Rejecting",
                   node->IsValid() ? node->ToString().c_str() : uniqueName.c_str()));

    return accept;
}


void BTController::SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
{
    String uniqueName(joiner);
    BTNodeInfo node = nodeDB.FindNode(uniqueName);

    if (node->IsValid()) {
        QCC_DbgPrintf(("Session joined by %s", node->ToString().c_str()));
        nodeDB.UpdateNodeSessionID(id, node);
    }
}


void BTController::SessionLost(SessionId id, SessionLostReason reason)
{
    QCC_DbgPrintf(("BTController::SessionLost(id = %x)", id));
    nodeDB.NodeSessionLost(id);
}


void BTController::JoinSessionCB(QStatus status, SessionId sessionID, const SessionOpts& opts, void* context)
{
    QCC_DbgTrace(("BTController::JoinSessionCB(status = %s, sessionID = %x, opts = <>, context = %p)",
                  QCC_StatusText(status), sessionID, context));
    if ((status == ER_OK) &&
        (joinSessionNode != masterNode) &&
        !nodeDB.FindNode(joinSessionNode->GetBusAddress())->IsValid()) {

        uint16_t connCnt = joinSessionNode->GetConnectionCount();

        if (connCnt == 1) {
            bus.LeaveSession(sessionID);
            joinSessionNode->SetSessionState(_BTNodeInfo::NO_SESSION);
            JoinSessionNodeComplete();
        } else {
            joinSessionNode->SetSessionID(sessionID);
            joinSessionNode->SetSessionState(_BTNodeInfo::SESSION_UP);
            DispatchOperation(new SendSetStateDispatchInfo());
        }
    } else {
        if (status == ER_OK) {
            // This is a duplicate session - shut it down.
            bus.LeaveSession(sessionID);
        }

        joinSessionNode->SetSessionState(_BTNodeInfo::NO_SESSION);
        JoinSessionNodeComplete();
    }
}


QStatus BTController::DoNameOp(const qcc::String& name,
                               const InterfaceDescription::Member& signal,
                               bool add,
                               NameArgInfo& nameArgInfo)
{
    QCC_DbgTrace(("BTController::DoNameOp(name = %s, signal = %s, add = %s, nameArgInfo = <%s>)",
                  name.c_str(), signal.name.c_str(), add ? "true" : "false",
                  (&nameArgInfo == static_cast<NameArgInfo*>(&find)) ? "find" : "advertise"));
    QStatus status = ER_OK;

    lock.Lock(MUTEX_CONTEXT);
    if (add) {
        nameArgInfo.AddName(name, self);
    } else {
        nameArgInfo.RemoveName(name, self);
    }

    nameArgInfo.dirty = true;

    bool devAvail = devAvailable;
    bool isMaster = IsMaster();
    bool isDrone = IsDrone();
    lock.Unlock(MUTEX_CONTEXT);

    if (devAvail) {
        if (isMaster) {
            QCC_DbgPrintf(("Handling %s locally (we're the master)", signal.name.c_str()));

#ifndef NDEBUG
            if (add && (&nameArgInfo == static_cast<NameArgInfo*>(&find))) {
                discoverStartTime = discoverTimer.StartTime();
            }
#endif

            DispatchOperation(new UpdateDelegationsDispatchInfo());

        } else {
            QCC_DbgPrintf(("Sending %s to our master: %s (%s)", signal.name.c_str(), master->GetServiceName().c_str(), masterNode->ToString().c_str()));
            MsgArg args[SIG_NAME_OP_SIZE];
            size_t argsSize = ArraySize(args);
            MsgArg::Set(args, argsSize, SIG_NAME_OP,
                        self->GetBusAddress().addr.GetRaw(),
                        self->GetBusAddress().psm,
                        name.c_str());
            status = Signal(masterNode->GetUniqueName().c_str(), masterNode->GetSessionID(), signal, args, argsSize);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to send %s signal to %s (%s)", signal.name.c_str(), masterNode->ToString().c_str(), masterNode->GetUniqueName().c_str()));
            }
            /*
             * Drone is responsible for telling its direct minions about changes in advertised
             * names.  The signal to the master will take care of the rest.
             */
            if (isDrone) {
                /*
                 * Make an empty node DB and put the name in it to tell
                 * DistributeAdvertisedNameChanges about the name change.
                 */
                BTNodeDB db;
                BTNodeInfo node = self->Clone();
                if (&nameArgInfo == static_cast<NameArgInfo*>(&advertise)) {
                    node->AddAdvertiseName(name);
                    db.AddNode(node);
                    if (add) {
                        DistributeAdvertisedNameChanges(&db, NULL);
                    } else {
                        DistributeAdvertisedNameChanges(NULL, &db);
                    }
                } else {
                    /*
                     * DoNameOp is also called for FindName and CancelFindName, but the only thing
                     * the drone needs to do for those ops is to inform the master (which is already
                     * done above).
                     */
                }
            }
        }
    }

    return status;
}


void BTController::HandleNameSignal(const InterfaceDescription::Member* member,
                                    const char* sourcePath,
                                    Message& msg)
{
    QCC_DbgTrace(("BTController::HandleNameSignal(member = %s, sourcePath = \"%s\", msg = <>)",
                  member->name.c_str(), sourcePath));
    if (IsMinion()) {
        // Minions should not be getting these signals.
        return;
    }

    bool fn = (*member == *org.alljoyn.Bus.BTController.FindName);
    bool cfn = (*member == *org.alljoyn.Bus.BTController.CancelFindName);
    bool an = (*member == *org.alljoyn.Bus.BTController.AdvertiseName);

    bool addName = (fn || an);
    bool findOp = (fn || cfn);
    NameArgInfo* nameCollection = (findOp ?
                                   static_cast<NameArgInfo*>(&find) :
                                   static_cast<NameArgInfo*>(&advertise));
    char* nameStr;
    uint64_t addrRaw;
    uint16_t psm;

    QStatus status = msg->GetArgs(SIG_NAME_OP, &addrRaw, &psm, &nameStr);

    if (status == ER_OK) {
        BTBusAddress addr(addrRaw, psm);
        BTNodeInfo node = nodeDB.FindNode(addr);

        if (node->IsValid()) {
            QCC_DbgPrintf(("%s %s %s the list of %s names for %s.",
                           addName ? "Adding" : "Removing",
                           nameStr,
                           addName ? "to" : "from",
                           findOp ? "find" : "advertise",
                           node->ToString().c_str()));

            lock.Lock(MUTEX_CONTEXT);

            // All nodes need to be registered via SetState
            qcc::String name(nameStr);
            if (addName) {
                nameCollection->AddName(name, node);
            } else {
                nameCollection->RemoveName(name, node);
            }

            bool isMaster = IsMaster();
            bool isDrone = IsDrone();
            lock.Unlock(MUTEX_CONTEXT);

            if (isMaster) {
                DispatchOperation(new UpdateDelegationsDispatchInfo());

                if (findOp) {
                    if (addName && (node->FindNamesSize() == 1)) {
                        // Prime the name cache for our minion
                        SendFoundNamesChange(node, nodeDB, false);
                        if (foundNodeDB.Size() > 0) {
                            SendFoundNamesChange(node, foundNodeDB, false);
                        }
                    }  // else do nothing
                } else {
                    BTNodeDB newAdInfo;
                    BTNodeDB oldAdInfo;
                    BTNodeInfo nodeChange = node->Clone();
                    nodeChange->AddAdvertiseName(name);
                    if (addName) {
                        newAdInfo.AddNode(nodeChange);
                    } else {
                        oldAdInfo.AddNode(nodeChange);
                    }
                    DistributeAdvertisedNameChanges(&newAdInfo, &oldAdInfo);
                }

            } else {
                // We are a drone so pass on the name
                const MsgArg* args;
                size_t numArgs;
                msg->GetArgs(numArgs, args);
                Signal(masterNode->GetUniqueName().c_str(), masterNode->GetSessionID(), *member, args, numArgs);

                if (isDrone && !findOp) {
                    BTNodeDB newAdInfo;
                    BTNodeDB oldAdInfo;
                    BTNodeInfo nodeChange = node->Clone();
                    nodeChange->AddAdvertiseName(name);
                    if (addName) {
                        newAdInfo.AddNode(nodeChange);
                    } else {
                        oldAdInfo.AddNode(nodeChange);
                    }
                    DistributeAdvertisedNameChanges(&newAdInfo, &oldAdInfo);
                }
            }
        } else {
            QCC_LogError(ER_FAIL, ("Did not find node %s in node DB", addr.ToString().c_str()));
        }
    } else {
        QCC_LogError(status, ("Processing msg args"));
    }
}


void BTController::HandleSetState(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_DbgTrace(("BTController::HandleSetState(member = \"%s\", msg = <>)", member->name.c_str()));
    qcc::String sender = msg->GetSender();
    RemoteEndpoint ep = bt.LookupEndpoint(sender);

    bus.EnableConcurrentCallbacks();

    if ((!ep->IsValid()) ||
        nodeDB.FindNode(ep->GetRemoteName())->IsValid()) {
        /* We don't acknowledge anyone calling the SetState method call who
         * fits into one of these categories:
         *
         * - Not a Bluetooth endpoint
         * - Has already called SetState
         * - We are not the Master
         *
         * Don't send a response as punishment >:)
         */
        if (ep->IsValid()) {
            bt.ReturnEndpoint(ep);
        }
        QCC_LogError(ER_FAIL, ("Received a SetState method call from %s.",
                               (!ep->IsValid()) ? "an invalid sender" : "a node we're already connected to"));
        return;
    }

    uint32_t remoteProtocolVersion = ep->GetRemoteProtocolVersion();
    bt.ReturnEndpoint(ep);

    QStatus status;

    uint8_t remoteDirectMinions;
    uint8_t remoteSlaveFactor;
    bool remoteEIRCapable;
    uint64_t rawBDAddr;
    uint16_t psm;
    uint32_t otherUUIDRev;
    size_t numNodeStateArgs;
    MsgArg* nodeStateArgs;
    size_t numFoundNodeArgs;
    MsgArg* foundNodeArgs;
    bool updateDelegations = false;

    lock.Lock(MUTEX_CONTEXT);
    if (!IsMaster()) {
        // We are not the master so we should not get a SetState method call.
        // Don't send a response as punishment >:)
        QCC_LogError(ER_FAIL, ("SetState method call received while not a master"));
        lock.Unlock(MUTEX_CONTEXT);
        return;
    }

    status = msg->GetArgs(SIG_SET_STATE_IN,
                          &remoteDirectMinions,
                          &remoteSlaveFactor,
                          &remoteEIRCapable,
                          &otherUUIDRev,
                          &rawBDAddr,
                          &psm,
                          &numNodeStateArgs, &nodeStateArgs,
                          &numFoundNodeArgs, &foundNodeArgs);

    if (status != ER_OK) {
        lock.Unlock(MUTEX_CONTEXT);
        MethodReply(msg, "org.alljoyn.Bus.BTController.InternalError", QCC_StatusText(status));
        bt.Disconnect(sender);
        return;
    }

    const BTBusAddress addr(rawBDAddr, psm);
    MsgArg args[SIG_SET_STATE_OUT_SIZE];
    size_t numArgs = ArraySize(args);
    vector<MsgArg> nodeStateArgsStorage;
    vector<MsgArg> foundNodeArgsStorage;

    foundNodeDB.Lock(MUTEX_CONTEXT);
    // Get the known node if we know it in order to keep things consistent.
    BTNodeInfo connectingNode = foundNodeDB.FindNode(addr);

    if (connectingNode->IsValid()) {
        connectingNode->SetUniqueName(sender);
        if (connectingNode != connectingNode->GetConnectNode()) {
            foundNodeDB.RemoveNode(connectingNode);
            connectingNode->SetConnectNode(connectingNode);
            foundNodeDB.AddNode(connectingNode);
        }
    } else {
        connectingNode = BTNodeInfo(addr, sender);
    }
    connectingNode->SetUUIDRev(otherUUIDRev);
    connectingNode->SetSessionID(msg->GetSessionId());
    connectingNode->SetEIRCapable(remoteEIRCapable);
    foundNodeDB.Unlock(MUTEX_CONTEXT);

    if (addr == self->GetBusAddress()) {
        // We should never get a connection from a device with the same address as our own.
        // Don't send a response as punishment >:)
        QCC_LogError(ER_FAIL, ("SetState method call received with remote bus address the same as ours (%s)",
                               addr.ToString().c_str()));
        lock.Unlock(MUTEX_CONTEXT);
        bt.Disconnect(sender);
        return;
    }


    FillFoundNodesMsgArgs(foundNodeArgsStorage, foundNodeDB);

    bool wantMaster = ((ALLJOYN_PROTOCOL_VERSION > remoteProtocolVersion) ||

                       ((ALLJOYN_PROTOCOL_VERSION == remoteProtocolVersion) &&
                        ((!bt.IsEIRCapable() && remoteEIRCapable) ||

                         ((bt.IsEIRCapable() == remoteEIRCapable) &&
                          (directMinions >= remoteDirectMinions)))));

    bool isMaster;
    status = bt.IsMaster(addr.addr, isMaster);
    if (status != ER_OK) {
        isMaster = false; // couldn't tell, so guess
    }

    if (wantMaster != isMaster) {
        bt.RequestBTRole(addr.addr, wantMaster ? bt::MASTER : bt::SLAVE);

        // Now see if ForceMaster() worked...
        status = bt.IsMaster(addr.addr, isMaster);
        if (status != ER_OK) {
            isMaster = false; // couldn't tell, so guess
        }
    }

    uint8_t slaveFactor = ComputeSlaveFactor();

    QCC_DbgPrintf(("Who becomes Master? proto ver: %u, %u   EIR support: %d, %d   minion cnt: %u, %u   slave factor: %u, %u   bt role: %s  wantMaster: %s",
                   ALLJOYN_PROTOCOL_VERSION, remoteProtocolVersion,
                   bt.IsEIRCapable(), remoteEIRCapable,
                   directMinions, remoteDirectMinions,
                   slaveFactor, remoteSlaveFactor,
                   isMaster ? "master" : "slave",
                   wantMaster ? "true" : "false"));

    if ((slaveFactor > remoteSlaveFactor) ||
        ((slaveFactor == remoteSlaveFactor) && !isMaster)) {
        // We are now a minion (or a drone if we have more than one direct connection)
        master = new ProxyBusObject(bus, sender.c_str(), bluetoothObjPath, 0);

        masterNode = connectingNode;
        masterNode->SetRelationship(_BTNodeInfo::MASTER);

        if (advertise.active) {
            advertise.StopOp(true);
            advertise.minion = self;
        }
        if (find.active) {
            find.StopOp(true);
            find.minion = self;
        }

        if (dispatcher.HasAlarm(expireAlarm)) {
            dispatcher.RemoveAlarm(expireAlarm);
        }

        FillNodeStateMsgArgs(nodeStateArgsStorage);

        status = ImportState(connectingNode, nodeStateArgs, numNodeStateArgs, foundNodeArgs, numFoundNodeArgs);
        if (status != ER_OK) {
            lock.Unlock(MUTEX_CONTEXT);
            MethodReply(msg, "org.alljoyn.Bus.BTController.InternalError", QCC_StatusText(status));
            bt.Disconnect(sender);
            return;
        }

        foundNodeDB.RemoveExpiration();

    } else {
        // We are still the master

        // Add information about the already connected nodes to the found
        // node data so that our new minions will have up-to-date
        // advertising information about our existing minions.
        FillFoundNodesMsgArgs(foundNodeArgsStorage, nodeDB);

        bool noRotateMinions = !RotateMinions();

        connectingNode->SetRelationship(_BTNodeInfo::DIRECT_MINION);

        status = ImportState(connectingNode, nodeStateArgs, numNodeStateArgs, foundNodeArgs, numFoundNodeArgs);
        if (status != ER_OK) {
            lock.Unlock(MUTEX_CONTEXT);
            QCC_LogError(status, ("Dropping %s due to import state error", sender.c_str()));
            bt.Disconnect(sender);
            return;
        }

        if ((find.minion == self) && !UseLocalFind()) {
            // Force updating the find delegation
            if (find.active) {
                QCC_DbgPrintf(("Stopping local find..."));
                find.StopLocal();
            }
            find.dirty = true;
        }

        if ((advertise.minion == self) && !UseLocalAdvertise()) {
            // Force updating the advertise delegation
            if (advertise.active) {
                QCC_DbgPrintf(("Stopping local advertise..."));
                advertise.StopLocal();
            }
            advertise.dirty = true;
        }

        if (noRotateMinions && RotateMinions()) {
            // Force changing from permanent delegations to durational delegations
            advertise.dirty = true;
            find.dirty = true;
        }
        updateDelegations = true;
    }

    QCC_DbgPrintf(("We are %s, %s is now our %s",
                   IsMaster() ? "still the master" : (IsDrone() ? "now a drone" : "just a minion"),
                   addr.ToString().c_str(), IsMaster() ? "minion" : "master"));

    if (IsMaster()) {
        // Can't let the to-be-updated masterUUIDRev have a value that is
        // the same as the UUIDRev value used by our new minion.
        const uint32_t lowerBound = ((otherUUIDRev > (bt::INVALID_UUIDREV + 10)) ?
                                     (otherUUIDRev - 10) :
                                     bt::INVALID_UUIDREV);
        const uint32_t upperBound = ((otherUUIDRev < (numeric_limits<uint32_t>::max() - 10)) ?
                                     (otherUUIDRev + 10) :
                                     numeric_limits<uint32_t>::max());
        while ((masterUUIDRev == bt::INVALID_UUIDREV) &&
               (masterUUIDRev > lowerBound) &&
               (masterUUIDRev < upperBound)) {
            masterUUIDRev = qcc::Rand32();
        }
        advertise.dirty = true;
    }

    status = MsgArg::Set(args, numArgs, SIG_SET_STATE_OUT,
                         bt.IsEIRCapable(),
                         masterUUIDRev,
                         self->GetBusAddress().addr.GetRaw(),
                         self->GetBusAddress().psm,
                         nodeStateArgsStorage.size(),
                         nodeStateArgsStorage.empty() ? NULL : &nodeStateArgsStorage.front(),
                         foundNodeArgsStorage.size(),
                         foundNodeArgsStorage.empty() ? NULL : &foundNodeArgsStorage.front());

    if (status != ER_OK) {
        QCC_LogError(status, ("MsgArg::Set(%s)", SIG_SET_STATE_OUT));
        bt.Disconnect(sender);
        lock.Unlock(MUTEX_CONTEXT);
        return;
    }

    status = MethodReply(msg, args, numArgs);
    if (status != ER_OK) {
        QCC_LogError(status, ("MethodReply"));
        bt.Disconnect(sender);
        lock.Unlock(MUTEX_CONTEXT);
        return;
    }

    connectingNode->SetSessionState(_BTNodeInfo::SESSION_UP);  // Just in case it wasn't set before


    if (connectingNode == joinSessionNode) {
        JoinSessionNodeComplete();  // Also triggers UpdateDelegations if we stay the master.
    } else if (updateDelegations) {
        DispatchOperation(new UpdateDelegationsDispatchInfo());
    }

    lock.Unlock(MUTEX_CONTEXT);
}


void BTController::HandleSetStateReply(Message& msg, void* context)
{
    QCC_DbgTrace(("BTController::HandleSetStateReply(reply = <>, context = %p)", context));
    DispatchOperation(new ProcessSetStateReplyDispatchInfo(msg, reinterpret_cast<ProxyBusObject*>(context)));
}


void BTController::HandleDelegateOp(const InterfaceDescription::Member* member,
                                    const char* sourcePath,
                                    Message& msg)
{
    QCC_DbgTrace(("BTController::HandleDelegateOp(member = \"%s\", sourcePath = %s, msg = <>)",
                  member->name.c_str(), sourcePath));
    bool findOp = member == org.alljoyn.Bus.BTController.DelegateFind;
    if (IsMaster() || (strcmp(sourcePath, bluetoothObjPath) != 0) ||
        (master->GetServiceName() != msg->GetSender())) {
        // We only accept delegation commands from our master!
        QCC_DbgHLPrintf(("%s tried to delegate %s to us; our master is %s",
                         msg->GetSender(),
                         findOp ? "find" : "advertise",
                         IsMaster() ? "ourself" : master->GetServiceName().c_str()));
        return;
    }

    DispatchOperation(new HandleDelegateOpDispatchInfo(msg, findOp));

    // It is possible for multiple delegate operation singals to be received
    // in the same millisecond.  Since the alarm/timer mechanism does not
    // guarantee that registered alarms will be called in the same order in
    // which they are registered if they are registered to trigger at the
    // exact same time, we do a 1 ms sleep to force each delegate opeation to
    // be scheduled to run in the same sequence in which they were received.
    qcc::Sleep(1);
}


void BTController::HandleFoundNamesChange(const InterfaceDescription::Member* member,
                                          const char* sourcePath,
                                          Message& msg)
{
    QCC_DbgTrace(("BTController::HandleFoundNamesChange(member = %s, sourcePath = \"%s\", msg = <>)",
                  member->name.c_str(), sourcePath));

    if (IsMaster() ||
        (strcmp(sourcePath, bluetoothObjPath) != 0) ||
        (master->GetServiceName() != msg->GetSender())) {
        // We only accept FoundNames signals from our direct master!
        QCC_LogError(ER_FAIL, ("Received %s from %s who is NOT our master",
                               msg->GetMemberName(), msg->GetSender()));
        return;
    }

    BTNodeDB adInfo;
    bool lost = (member == org.alljoyn.Bus.BTController.LostNames);
    MsgArg* entries;
    size_t size;

    QStatus status = msg->GetArgs(SIG_FOUND_NAMES, &size, &entries);

    if (status == ER_OK) {
        status = ExtractNodeInfo(entries, size, adInfo);
    }

    if ((status == ER_OK) && (adInfo.Size() > 0)) {

        // Figure out which name changes belong to which DB (nodeDB or foundNodeDB).
        BTNodeDB externalDB;
        nodeDB.NodeDiff(adInfo, &externalDB, NULL);

        const BTNodeDB* newExternalDB = lost ? NULL : &externalDB;
        const BTNodeDB* oldExternalDB = lost ? &externalDB : NULL;

        foundNodeDB.UpdateDB(newExternalDB, oldExternalDB, false);
        foundNodeDB.DumpTable("foundNodeDB - Updated set of found devices");

        DistributeAdvertisedNameChanges(newExternalDB, oldExternalDB);
    }
}


void BTController::HandleFoundDeviceChange(const InterfaceDescription::Member* member,
                                           const char* sourcePath,
                                           Message& msg)
{
    QCC_DbgTrace(("BTController::HandleFoundDeviceChange(member = %s, sourcePath = \"%s\", msg = <>)",
                  member->name.c_str(), sourcePath));

    if (!nodeDB.FindNode(msg->GetSender())->IsDirectMinion()) {
        // We only handle FoundDevice signals from our minions.
        QCC_LogError(ER_FAIL, ("Received %s from %s who is NOT a direct minion",
                               msg->GetMemberName(), msg->GetSender()));
        return;
    }

    uint32_t uuidRev;
    uint64_t adBdAddrRaw;
    bool eirCapable;

    QStatus status = msg->GetArgs(SIG_FOUND_DEV, &adBdAddrRaw, &uuidRev, &eirCapable);

    if (status == ER_OK) {
        bus.EnableConcurrentCallbacks();
        BDAddress adBdAddr(adBdAddrRaw);
        ProcessDeviceChange(adBdAddr, uuidRev, eirCapable);
    }
}


void BTController::HandleConnectAddrChanged(const InterfaceDescription::Member* member,
                                            const char* sourcePath,
                                            Message& msg)
{
    QCC_DbgTrace(("BTController::HandleConnectAddrChanged(member = %s, sourcePath = \"%s\", msg = <>)",
                  member->name.c_str(), sourcePath));

    if ((!IsMinion() && !nodeDB.FindNode(msg->GetSender())->IsDirectMinion()) ||
        (!IsMaster() && (master->GetServiceName() == msg->GetSender()))) {
        // We only handle FoundDevice signals from our direct minions or from our master.
        QCC_LogError(ER_FAIL, ("Received %s from %s who is NOT a direct minion NOR our master.",
                               msg->GetMemberName(), msg->GetSender()));
        return;
    }

    uint64_t oldRawAddr;
    uint16_t oldPSM;
    uint64_t newRawAddr;
    uint16_t newPSM;

    QStatus status = msg->GetArgs(SIG_CONN_ADDR_CHANGED, &oldRawAddr, &oldPSM, &newRawAddr, &newPSM);
    if (status == ER_OK) {
        BTBusAddress oldAddr(oldRawAddr, oldPSM);
        BTBusAddress newAddr(newRawAddr, newPSM);
        if (!IsMinion()) {
            nodeDB.Lock(MUTEX_CONTEXT);
            BTNodeInfo changedNode = nodeDB.FindNode(oldAddr);
            if (changedNode->IsValid()) {
                assert(newAddr.IsValid());

                nodeDB.RemoveNode(changedNode);
                changedNode->SetBusAddress(newAddr);
                nodeDB.AddNode(changedNode);
            }
            nodeDB.Unlock(MUTEX_CONTEXT);
        }
        if (!IsMaster()) {
            lock.Lock(MUTEX_CONTEXT);
            if (masterNode->GetBusAddress() == oldAddr) {
                assert(newAddr.IsValid());

                foundNodeDB.Lock(MUTEX_CONTEXT);
                bool updateFoundNodeDB = (foundNodeDB.FindNode(oldAddr) == masterNode);
                if (updateFoundNodeDB) {
                    foundNodeDB.RemoveNode(masterNode);
                    masterNode->SetBusAddress(newAddr);
                    foundNodeDB.AddNode(masterNode);
                } else {
                    masterNode->SetBusAddress(newAddr);
                }
                foundNodeDB.Unlock(MUTEX_CONTEXT);
            }
            lock.Unlock(MUTEX_CONTEXT);
        }
    }
}


void BTController::DeferredBTDeviceAvailable(bool on)
{
    QCC_DbgTrace(("BTController::DeferredBTDeviceAvailable(<%s>)", on ? "on" : "off"));
    lock.Lock(MUTEX_CONTEXT);

    /*
     * Update our EIR capability.  This can only be true once a device is available.  When not,
     * it will default to false, so this is safe to call in any case.
     */
    self->SetEIRCapable(bt.IsEIRCapable());

    if (on && !devAvailable) {
        BTBusAddress listenAddr;
        devAvailable = true;
        QStatus status = bt.StartListen(listenAddr.addr, listenAddr.psm);
        if (status == ER_OK) {
            assert(listenAddr.IsValid());
            listening = true;

            if (self->GetBusAddress() != listenAddr) {
                SetSelfAddress(listenAddr);
            }

            find.dirty = true;  // Update ignore addrs

            if (IsMaster()) {
                //DispatchOperation(new UpdateDelegationsDispatchInfo());
                UpdateDelegations(advertise);
                UpdateDelegations(find);
            }
        } else {
            QCC_LogError(status, ("Failed to start listening for incoming connections"));
        }
    } else if (!on && devAvailable) {
        if (listening) {
            bt.StopListen();
            BTBusAddress nullAddr;
            listening = false;
        }
        if (advertise.active) {
            if (advertise.minion == self) {
                QCC_DbgPrintf(("Stopping local advertise..."));
                advertise.StopLocal();
            }
            advertise.active = false;
            advertise.StopAlarm();
        }
        if (find.active) {
            if (find.minion == self) {
                QCC_DbgPrintf(("Stopping local find..."));
                find.StopLocal();
            }
            find.active = false;
            find.StopAlarm();
        }

        /*
         * Expire found names in 10 seconds.  If BT turning off is just a
         * small glitch (i.e., bluetoothd crashed and will be restarted), we
         * should get refreshes fairly soon so that no lost name gets sent
         * out.  If BT was explicitly turned off by the user, then it will
         * likely be off for a very long time.
         */
        foundNodeDB.RefreshExpiration(10000);
        ResetExpireNameAlarm();

        blacklist->clear();

        devAvailable = false;
    }

    lock.Unlock(MUTEX_CONTEXT);
}


void BTController::DeferredSendSetState()
{
    QCC_DbgTrace(("BTController::DeferredSendSetState()  [joinSessionNode = %s]", joinSessionNode->ToString().c_str()));
    assert(!master);

    QStatus status;
    vector<MsgArg> nodeStateArgsStorage;
    vector<MsgArg> foundNodeArgsStorage;
    MsgArg args[SIG_SET_STATE_IN_SIZE];
    size_t numArgs = ArraySize(args);
    Message reply(bus);
    ProxyBusObject* newMaster = new ProxyBusObject(bus, joinSessionNode->GetUniqueName().c_str(), bluetoothObjPath, joinSessionNode->GetSessionID());
    uint8_t slaveFactor;


    lock.Lock(MUTEX_CONTEXT);
    newMaster->AddInterface(*org.alljoyn.Bus.BTController.interface);

    slaveFactor = ComputeSlaveFactor();

    QCC_DbgPrintf(("SendSetState prep args"));
    FillNodeStateMsgArgs(nodeStateArgsStorage);
    FillFoundNodesMsgArgs(foundNodeArgsStorage, foundNodeDB);

    status = MsgArg::Set(args, numArgs, SIG_SET_STATE_IN,
                         directMinions,
                         slaveFactor,
                         bt.IsEIRCapable(),
                         masterUUIDRev,
                         self->GetBusAddress().addr.GetRaw(),
                         self->GetBusAddress().psm,
                         nodeStateArgsStorage.size(),
                         nodeStateArgsStorage.empty() ? NULL : &nodeStateArgsStorage.front(),
                         foundNodeArgsStorage.size(),
                         foundNodeArgsStorage.empty() ? NULL : &foundNodeArgsStorage.front());
    lock.Unlock(MUTEX_CONTEXT);

    if (status != ER_OK) {
        QCC_LogError(status, ("Dropping %s due to internal error", joinSessionNode->ToString().c_str()));
        goto exit;
    }

    /*
     * There is a small chance that 2 devices initiating a connection to each
     * other may each send the SetState method call simultaneously.  We
     * release the lock while making the synchronous method call to prevent a
     * possible deadlock in that case.  The SendSetState function must not run
     * in the same thread as that HandleSetState function.
     */
    QCC_DbgPrintf(("Sending SetState method call to %s (%s)",
                   joinSessionNode->GetUniqueName().c_str(), joinSessionNode->ToString().c_str()));
    status = newMaster->MethodCallAsync(*org.alljoyn.Bus.BTController.SetState,
                                        this, ReplyHandler(&BTController::HandleSetStateReply),
                                        args, ArraySize(args),
                                        newMaster);

    if (status != ER_OK) {
        QCC_LogError(status, ("Dropping %s due to internal error", joinSessionNode->ToString().c_str()));
    }

exit:

    if (status != ER_OK) {
        delete newMaster;
        bt.Disconnect(joinSessionNode->GetUniqueName());
        joinSessionNode->SetSessionState(_BTNodeInfo::NO_SESSION);
        JoinSessionNodeComplete();
    }
}


void BTController::DeferredProcessSetStateReply(Message& reply,
                                                ProxyBusObject* newMaster)
{
    QCC_DbgTrace(("BTController::DeferredProcessSetStateReply(reply = <>, newMaster = %p)  [joinSessionNode = %s]",
                  newMaster, joinSessionNode->ToString().c_str()));

    lock.Lock(MUTEX_CONTEXT);
    QStatus status = ER_FAIL;

    if (reply->GetType() == MESSAGE_METHOD_RET) {
        size_t numNodeStateArgs;
        MsgArg* nodeStateArgs;
        size_t numFoundNodeArgs;
        MsgArg* foundNodeArgs;
        uint64_t rawBDAddr;
        uint16_t psm;
        uint32_t otherUUIDRev;
        bool remoteEIRCapable;

        if (nodeDB.FindNode(joinSessionNode->GetBusAddress())->IsValid()) {
            QCC_DbgHLPrintf(("Already got node state information."));
            delete newMaster;
            status = ER_FAIL;
            goto exit;
        }

        status = reply->GetArgs(SIG_SET_STATE_OUT,
                                &remoteEIRCapable,
                                &otherUUIDRev,
                                &rawBDAddr,
                                &psm,
                                &numNodeStateArgs, &nodeStateArgs,
                                &numFoundNodeArgs, &foundNodeArgs);
        if ((status != ER_OK) || ((joinSessionNode->GetBusAddress().addr.GetRaw() != rawBDAddr) &&
                                  (joinSessionNode->GetBusAddress().psm != psm))) {
            delete newMaster;
            QCC_LogError(status, ("Dropping %s due to error parsing the args (sig: \"%s\")",
                                  joinSessionNode->ToString().c_str(), SIG_SET_STATE_OUT));
            bt.Disconnect(joinSessionNode->GetUniqueName());
            goto exit;
        }

        if (otherUUIDRev != bt::INVALID_UUIDREV) {
            if (bt.IsEIRCapable() && !joinSessionNode->IsEIRCapable() && remoteEIRCapable && (joinSessionNode->GetConnectionCount() == 1)) {
                joinSessionNode->SetEIRCapable(true);
                SessionId sessionID = joinSessionNode->GetSessionID();
                joinSessionNode->SetSessionID(0);
                bus.LeaveSession(sessionID);
                status = ER_FAIL;
                goto exit;
            }

            if (numNodeStateArgs == 0) {
                // We are now a minion (or a drone if we have more than one direct connection)
                master = newMaster;
                masterNode = joinSessionNode;
                masterNode->SetUUIDRev(otherUUIDRev);
                masterNode->SetRelationship(_BTNodeInfo::MASTER);
                masterNode->SetEIRCapable(remoteEIRCapable);

                if (dispatcher.HasAlarm(expireAlarm)) {
                    dispatcher.RemoveAlarm(expireAlarm);
                }

                status = ImportState(masterNode, nodeStateArgs, numNodeStateArgs, foundNodeArgs, numFoundNodeArgs);
                if (status != ER_OK) {
                    QCC_LogError(status, ("Dropping %s due to import state error", joinSessionNode->ToString().c_str()));
                    bt.Disconnect(joinSessionNode->GetUniqueName());
                    goto exit;
                }

            } else {
                // We are the still the master
                bool noRotateMinions = !RotateMinions();
                delete newMaster;
                joinSessionNode->SetRelationship(_BTNodeInfo::DIRECT_MINION);

                status = ImportState(joinSessionNode, nodeStateArgs, numNodeStateArgs, foundNodeArgs, numFoundNodeArgs);
                if (status != ER_OK) {
                    QCC_LogError(status, ("Dropping %s due to import state error", joinSessionNode->ToString().c_str()));
                    bt.Disconnect(joinSessionNode->GetUniqueName());
                    goto exit;
                }

                if (noRotateMinions && RotateMinions()) {
                    // Force changing from permanent delegations to durational delegations
                    advertise.dirty = true;
                    find.dirty = true;
                }
            }

            QCC_DbgPrintf(("We are %s, %s is now our %s",
                           IsMaster() ? "still the master" : (IsDrone() ? "now a drone" : "just a minion"),
                           joinSessionNode->ToString().c_str(), IsMaster() ? "minion" : "master"));

            if (IsMaster()) {
                // Can't let the to-be-updated masterUUIDRev have a value that is
                // the same as the UUIDRev value used by our new minion.
                const uint32_t lowerBound = ((otherUUIDRev > (bt::INVALID_UUIDREV + 10)) ?
                                             (otherUUIDRev - 10) :
                                             bt::INVALID_UUIDREV);
                const uint32_t upperBound = ((otherUUIDRev < (numeric_limits<uint32_t>::max() - 10)) ?
                                             (otherUUIDRev + 10) :
                                             numeric_limits<uint32_t>::max());
                while ((masterUUIDRev == bt::INVALID_UUIDREV) &&
                       (masterUUIDRev > lowerBound) &&
                       (masterUUIDRev < upperBound)) {
                    masterUUIDRev = qcc::Rand32();
                }
            }
        }
    } else {
        delete newMaster;
        qcc::String errMsg;
        const char* errName = reply->GetErrorName(&errMsg);
        QCC_LogError(ER_FAIL, ("Dropping %s due to internal error: %s - %s", joinSessionNode->ToString().c_str(), errName, errMsg.c_str()));
        bt.Disconnect(joinSessionNode->GetUniqueName());
    }

exit:

    if (status == ER_OK) {
        joinSessionNode->SetSessionState(_BTNodeInfo::SESSION_UP);  // Just in case
    } else {
        joinSessionNode->SetSessionState(_BTNodeInfo::NO_SESSION);  // Just in case
    }

    JoinSessionNodeComplete();
    lock.Unlock(MUTEX_CONTEXT);
}


void BTController::DeferredHandleDelegateFind(Message& msg)
{
    QCC_DbgTrace(("BTController::HandleDelegateFind(msg = <>)"));

    lock.Lock(MUTEX_CONTEXT);

    PickNextDelegate(find);

    if (find.minion == self) {
        uint64_t* ignoreAddrsArg;
        size_t numIgnoreAddrs;
        uint32_t duration;

        QStatus status = msg->GetArgs(SIG_DELEGATE_FIND, &numIgnoreAddrs, &ignoreAddrsArg, &duration);

        if (status == ER_OK) {
            if (numIgnoreAddrs > 0) {
                size_t i;
                BDAddressSet ignoreAddrs(*blacklist); // initialize ignore addresses with the blacklist
                for (i = 0; i < numIgnoreAddrs; ++i) {
                    ignoreAddrs->insert(ignoreAddrsArg[i]);
                }

                QCC_DbgPrintf(("Starting find for %u seconds...", duration));
                status = bt.StartFind(ignoreAddrs, duration);
                find.active = (status == ER_OK);
            } else {
                QCC_DbgPrintf(("Stopping local find..."));
                find.StopLocal();
            }
        }
    } else {
        const MsgArg* args;
        size_t numArgs;

        BTNodeInfo delegate = find.minion->GetConnectNode();

        msg->GetArgs(numArgs, args);

        // Pick a minion to do the work for us.
        assert(nodeDB.FindNode(find.minion->GetBusAddress())->IsValid());
        QCC_DbgPrintf(("Selected %s as our find minion.", find.minion->ToString().c_str()));

        Signal(delegate->GetUniqueName().c_str(), delegate->GetSessionID(), *find.delegateSignal, args, numArgs);
    }
    lock.Unlock(MUTEX_CONTEXT);
}


void BTController::DeferredHandleDelegateAdvertise(Message& msg)
{
    QCC_DbgTrace(("BTController::DeferredHandleDelegateAdvertise(msg = <>)"));

    lock.Lock(MUTEX_CONTEXT);

    PickNextDelegate(advertise);

    if (advertise.minion == self) {
        uint32_t uuidRev;
        uint64_t bdAddrRaw;
        uint16_t psm;
        BTNodeDB adInfo;
        MsgArg* entries;
        size_t size;
        uint32_t duration;

        QStatus status = msg->GetArgs(SIG_DELEGATE_AD, &uuidRev, &bdAddrRaw, &psm, &size, &entries, &duration);

        if (status == ER_OK) {
            status = ExtractAdInfo(entries, size, adInfo);
        }

        if (status == ER_OK) {
            if (adInfo.Size() > 0) {
                BDAddress bdAddr(bdAddrRaw);

                QCC_DbgPrintf(("Starting advertise for %u seconds...", duration));
                status = bt.StartAdvertise(uuidRev, bdAddr, psm, adInfo, duration);
                advertise.active = (status == ER_OK);
            } else {
                QCC_DbgPrintf(("Stopping local advertise..."));
                advertise.StopLocal();
            }
        }
    } else {
        const MsgArg* args;
        size_t numArgs;

        BTNodeInfo delegate = advertise.minion->GetConnectNode();

        msg->GetArgs(numArgs, args);

        // Pick a minion to do the work for us.
        assert(nodeDB.FindNode(advertise.minion->GetBusAddress())->IsValid());
        QCC_DbgPrintf(("Selected %s as our advertise minion.", advertise.minion->ToString().c_str()));

        Signal(delegate->GetUniqueName().c_str(), delegate->GetSessionID(), *advertise.delegateSignal, args, numArgs);
    }
    lock.Unlock(MUTEX_CONTEXT);
}


void BTController::DeferredNameLostHander(const String& name)
{
    // An endpoint left the bus.
    QCC_DbgPrintf(("%s has left the bus", name.c_str()));
    bool updateDelegations = false;

    lock.Lock(MUTEX_CONTEXT);
    if (master && (master->GetServiceName() == name)) {
        // We are a minion or a drone and our master has left us.

        QCC_DbgPrintf(("Our master left us: %s", masterNode->ToString().c_str()));
        // We are the master now.

        masterNode->SetSessionState(_BTNodeInfo::NO_SESSION);

        if (advertise.minion == self) {
            if (advertise.active) {
                QCC_DbgPrintf(("Stopping local advertise..."));
                advertise.StopLocal();
            }
        } else {
            MsgArg args[SIG_DELEGATE_AD_SIZE];
            size_t argsSize = ArraySize(args);

            /* Advertise an empty list for a while */
            MsgArg::Set(args, argsSize, SIG_DELEGATE_AD,
                        bt::INVALID_UUIDREV,
                        0ULL,
                        bt::INVALID_PSM,
                        static_cast<size_t>(0), NULL,
                        static_cast<uint32_t>(0));
            assert(argsSize == ArraySize(args));

            BTNodeInfo delegate = advertise.minion->GetConnectNode();

            Signal(delegate->GetUniqueName().c_str(), delegate->GetSessionID(), *advertise.delegateSignal, args, argsSize);
            advertise.active = false;
        }

        if (find.minion == self) {
            if (find.active) {
                QCC_DbgPrintf(("Stopping local find..."));
                find.StopLocal();
            }
        } else {
            MsgArg args[SIG_DELEGATE_FIND_SIZE];
            size_t argsSize = ArraySize(args);

            /* Advertise an empty list for a while */
            MsgArg::Set(args, argsSize, SIG_DELEGATE_FIND,
                        static_cast<size_t>(0), NULL,
                        static_cast<uint32_t>(0));
            assert(argsSize == ArraySize(args));

            BTNodeInfo delegate = find.minion->GetConnectNode();

            Signal(delegate->GetUniqueName().c_str(), delegate->GetSessionID(), *find.delegateSignal, args, argsSize);
            find.active = false;
        }


        if (!find.Empty()) {
            // We're going to be doing discovery so set the expiration all
            // found nodes to half the normal expiration.
            foundNodeDB.RefreshExpiration(LOST_DEVICE_TIMEOUT / 2);
        } else {
            // We're not going to do any discovery so set the expiration for a
            // short time.
            foundNodeDB.RefreshExpiration(5000);
        }
        // Our master and all of our master's minions excluding ourself and
        // our minions are in foundNodeDB so refreshing the expiration time on
        // all devices connectable via the masterNode to the default
        // expiration time.  (This overrides the expiration time set above for
        // all nodes.)
        foundNodeDB.RefreshExpiration(masterNode, LOST_DEVICE_TIMEOUT);
        ResetExpireNameAlarm();

        delete master;
        master = NULL;
        masterNode = BTNodeInfo();

        // We need to prepare for controlling discovery.
        find.dirty = true;  // Update ignore addrs

        updateDelegations = true;

    } else {
        // Someone else left.  If it was a minion node, remove their find/ad names.
        BTNodeInfo minion = nodeDB.FindNode(name);

        if (minion->IsValid()) {
            // We are a master or a drone and one of our minions has left.
            // (Note: all minions including indirect minions leaving cause
            // this code path to be executed.)

            QCC_DbgPrintf(("One of our minions left us: %s", minion->ToString().c_str()));

            bool wasAdvertiseMinion = minion == advertise.minion;
            bool wasFindMinion = minion == find.minion;
            bool wasDirect = minion->IsDirectMinion();
            bool wasRotateMinions = RotateMinions();

            nodeDB.RemoveNode(minion);
            assert(!devAvailable || (nodeDB.Size() > 0));

            if (!wasDirect && nodeDB.FindNode(minion->GetConnectNode()->GetBusAddress())->IsValid()) {
                // An indirect minion is leaving but its connect node is still
                // connected (maybe).  Set its connect node to itself.  If
                // we're wrong, we'll figure it out eventually.
                minion->SetConnectNode(minion);
            }

            minion->SetSessionState(_BTNodeInfo::NO_SESSION);
            minion->SetRelationship(_BTNodeInfo::UNAFFILIATED);

            find.dirty = true;  // Update ignore addrs

            // Indicate the name lists have changed.
            advertise.count -= minion->AdvertiseNamesSize();
            advertise.dirty = true;

            find.count -= minion->FindNamesSize();
            find.dirty = true;

            if (!RotateMinions() && wasRotateMinions) {
                advertise.StopAlarm();
                find.StopAlarm();
            }

            if (wasFindMinion) {
                find.minion = self;
                find.active = false;
                find.StopAlarm();
            }

            if (wasAdvertiseMinion) {
                advertise.minion = self;
                advertise.active = false;
                advertise.StopAlarm();
            }

            if (wasDirect) {
                --directMinions;
            }

            if (IsMaster()) {
                updateDelegations = true;

                if (!minion->AdvertiseNamesEmpty()) {
                    // The minion we lost was advertising one or more names.  We need
                    // to setup to expire those advertised names.
                    Timespec now;
                    GetTimeNow(&now);
                    uint64_t expireTime = now.GetAbsoluteMillis() + LOST_DEVICE_TIMEOUT;
                    minion->SetExpireTime(expireTime);
                    foundNodeDB.AddNode(minion);

                    ResetExpireNameAlarm();
                }
            }
        }
    }

    if (updateDelegations) {
        UpdateDelegations(advertise);
        UpdateDelegations(find);
        QCC_DbgPrintf(("NodeDB after processing lost node"));
        QCC_DEBUG_ONLY(DumpNodeStateTable());
    }
    lock.Unlock(MUTEX_CONTEXT);
}


void BTController::DistributeAdvertisedNameChanges(const BTNodeDB* newAdInfo,
                                                   const BTNodeDB* oldAdInfo)
{
    QCC_DbgTrace(("BTController::DistributeAdvertisedNameChanges(newAdInfo = <%lu nodes>, oldAdInfo = <%lu nodes>)",
                  newAdInfo ? newAdInfo->Size() : 0, oldAdInfo ? oldAdInfo->Size() : 0));

    /*
     * Lost names in oldAdInfo must be sent out before found names in
     * newAdInfo.  The same advertised names for a given device may appear in
     * both.  This happens when the underlying connect address changes.  This
     * can result in a device that previously failed to connect to become
     * successfully connectable.  AllJoyn client apps will not know this
     * happens unless they get a LostAdvertisedName signal followed by a
     * FoundAdvertisedName signal.
     */

    if (oldAdInfo) oldAdInfo->DumpTable("oldAdInfo - Old ad information");
    if (newAdInfo) newAdInfo->DumpTable("newAdInfo - New ad information");

    // Now inform everyone of the changes in advertised names.
    if (!IsMinion() && devAvailable) {
        set<BTNodeInfo> destNodesOld;
        set<BTNodeInfo> destNodesNew;
        nodeDB.Lock(MUTEX_CONTEXT);
        for (BTNodeDB::const_iterator it = nodeDB.Begin(); it != nodeDB.End(); ++it) {
            const BTNodeInfo& node = *it;
            if (node->IsDirectMinion()) {
                assert(node != self);  // We can't be a direct minion of ourself.
                QCC_DbgPrintf(("Notify %s of the name changes.", node->ToString().c_str()));
                if (oldAdInfo && oldAdInfo->Size() > 0) {
                    destNodesOld.insert(node);
                }
                if (newAdInfo && newAdInfo->Size() > 0) {
                    destNodesNew.insert(node);
                }
            }
        }
        nodeDB.Unlock(MUTEX_CONTEXT);

        for (set<BTNodeInfo>::const_iterator it = destNodesOld.begin(); it != destNodesOld.end(); ++it) {
            SendFoundNamesChange(*it, *oldAdInfo, true);
        }

        for (set<BTNodeInfo>::const_iterator it = destNodesNew.begin(); it != destNodesNew.end(); ++it) {
            SendFoundNamesChange(*it, *newAdInfo, false);
        }
    }

    BTNodeDB::const_iterator nodeit;
    // Tell ourself about the names (This is best done outside the Lock just in case).
    if (oldAdInfo) {
        for (nodeit = oldAdInfo->Begin(); nodeit != oldAdInfo->End(); ++nodeit) {
            const BTNodeInfo& node = *nodeit;
            if ((node->AdvertiseNamesSize() > 0) && (node != self)) {
                vector<String> vectorizedNames;
                vectorizedNames.reserve(node->AdvertiseNamesSize());
                vectorizedNames.assign(node->GetAdvertiseNamesBegin(), node->GetAdvertiseNamesEnd());
                bt.FoundNamesChange(node->GetGUID().ToString(), vectorizedNames, node->GetBusAddress().addr, node->GetBusAddress().psm, true);
            }
        }
    }
    if (newAdInfo) {
        for (nodeit = newAdInfo->Begin(); nodeit != newAdInfo->End(); ++nodeit) {
            const BTNodeInfo& node = *nodeit;
            if ((node->AdvertiseNamesSize() > 0) && (node != self)) {
                vector<String> vectorizedNames;
                vectorizedNames.reserve(node->AdvertiseNamesSize());
                vectorizedNames.assign(node->GetAdvertiseNamesBegin(), node->GetAdvertiseNamesEnd());
                bt.FoundNamesChange(node->GetGUID().ToString(), vectorizedNames, node->GetBusAddress().addr, node->GetBusAddress().psm, false);
            }
        }
    }
}


void BTController::SendFoundNamesChange(const BTNodeInfo& destNode,
                                        const BTNodeDB& adInfo,
                                        bool lost)
{
    QCC_DbgTrace(("BTController::SendFoundNamesChange(destNode = %s (\"%s\"), adInfo = <>, <%s>)",
                  destNode->ToString().c_str(), destNode->GetUniqueName().c_str(),
                  lost ? "lost" : "found/changed"));

    vector<MsgArg> nodeList;

    FillFoundNodesMsgArgs(nodeList, adInfo);

    MsgArg arg(SIG_FOUND_NAMES, nodeList.size(), nodeList.empty() ? NULL : &nodeList.front());
    QStatus status;
    if (lost) {
        status = Signal(destNode->GetUniqueName().c_str(), destNode->GetSessionID(),
                        *org.alljoyn.Bus.BTController.LostNames,
                        &arg, 1);
    } else {
        status = Signal(destNode->GetUniqueName().c_str(), destNode->GetSessionID(),
                        *org.alljoyn.Bus.BTController.FoundNames,
                        &arg, 1);
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to send org.alljoyn.Bus.BTController.%s signal to %s",
                              lost ? "LostNames" : "FoundNames",
                              destNode->ToString().c_str()));
    }
}


QStatus BTController::ImportState(BTNodeInfo& connectingNode,
                                  MsgArg* nodeStateArgs,
                                  size_t numNodeStates,
                                  MsgArg* foundNodeArgs,
                                  size_t numFoundNodes)
{
    QCC_DbgTrace(("BTController::ImportState(addr = %s, nodeStateArgs = <>, numNodeStates = %u, foundNodeArgs = <>, numFoundNodes = %u)  [role = %s]",
                  connectingNode->ToString().c_str(), numNodeStates, numFoundNodes,
                  IsMaster() ? "master" : "drone/minion"));
    assert(connectingNode->IsValid());

    /*
     * Here we need to bring in the state information from one or more nodes
     * that have just connected to the bus.  Typically, only one node will be
     * connecting, but it is possible for a piconet or even a scatternet of
     * nodes to connect.  Since we are processing the ImportState() we are by
     * definition the master and importing the state information of new
     * minions.
     *
     * In most cases, we actually already have all the information in the
     * foundNodeDB gathered via advertisements.  However, it is possible that
     * the information cached in foundNodeDB is stale and we will be getting
     * the latest and greatest information via the SetState method call.  We
     * need to determine if and what those changes are then tell our existing
     * connected minions.
     */

    QStatus status;
    size_t i;

    BTNodeDB incomingDB;
    BTNodeDB addedDB;
    BTNodeDB removedDB;
    BTNodeDB staleDB;
    BTNodeDB newFoundDB;
    BTNodeDB::const_iterator nodeit;

    lock.Lock(MUTEX_CONTEXT);  // Must be acquired before the foundNodeDB lock.
    foundNodeDB.Lock(MUTEX_CONTEXT);

    bool cnKnown = foundNodeDB.FindNode(connectingNode->GetBusAddress())->IsValid();

    for (i = 0; i < numNodeStates; ++i) {
        char* bn;
        char* guidStr;
        uint64_t rawBdAddr;
        uint16_t psm;
        size_t anSize, fnSize;
        MsgArg* anList;
        MsgArg* fnList;
        size_t j;
        bool eirCapable;

        status = nodeStateArgs[i].Get(SIG_NODE_STATE_ENTRY,
                                      &guidStr,
                                      &bn,
                                      &rawBdAddr, &psm,
                                      &anSize, &anList,
                                      &fnSize, &fnList,
                                      &eirCapable);
        if (status != ER_OK) {
            foundNodeDB.Unlock(MUTEX_CONTEXT);
            lock.Unlock(MUTEX_CONTEXT);  // Must be acquired before the foundNodeDB lock.

            return status;
        }

        String busName(bn);
        BTBusAddress nodeAddr(BDAddress(rawBdAddr), psm);
        GUID128 guid(guidStr);
        BTNodeInfo incomingNode;

        if (busName.empty()) {
            QCC_LogError(ER_NONE, ("Skipping node with address %s because it has no bus name.", nodeAddr.ToString().c_str()));
            assert(!busName.empty());
            continue;
        }

        if (nodeAddr == connectingNode->GetBusAddress()) {
            /*
             * We need the remote GUID of the connectingNode, but it's not included in the "header"
             * fields of SetState.  However, it should always be included in the list of node states
             * if we remain the topology master.  Putting the GUID in the header fields would be a
             * protocol change, so set it here instead of changing the protocol.
             */
            connectingNode->SetGUID(guid);

            // incomingNode needs to refer to the same instance as
            // connectingNode since other nodes already point to
            // connectingNode as their connect node and that instance is the
            // one that needs to make it into incomingDB.
            if (cnKnown) {
                incomingNode = connectingNode->Clone();
            } else {
                incomingNode = connectingNode;
            }
            if (IsMaster()) {
                incomingNode->SetRelationship(_BTNodeInfo::DIRECT_MINION);
            }
        } else {
            incomingNode = BTNodeInfo(nodeAddr, busName, guid);
            if (IsMaster()) {
                incomingNode->SetRelationship(_BTNodeInfo::INDIRECT_MINION);
            }
        }
        incomingNode->SetConnectNode(connectingNode);
        incomingNode->SetEIRCapable(eirCapable);

        QCC_DbgPrintf(("Processing names for newly connected node %s (GUID: %s  uniqueName: %s):",
                       incomingNode->ToString().c_str(),
                       guid.ToString().c_str(),
                       busName.c_str()));

        /*
         * NOTE: expiration time is explicitly NOT set for nodes that we are
         * connected to.  Their advertisements will go away when the node
         * disconnects.
         */

        if (IsMaster()) {
            advertise.dirty = advertise.dirty || (anSize > 0);
            find.dirty = find.dirty || (fnSize > 0);
        }

        for (j = 0; j < anSize; ++j) {
            char* n;
            status = anList[j].Get(SIG_NAME, &n);
            if (status != ER_OK) {
                QCC_LogError(status, ("Get advertise name failed"));
                foundNodeDB.Unlock(MUTEX_CONTEXT);
                lock.Unlock(MUTEX_CONTEXT);
                return status;
            }
            QCC_DbgPrintf(("    Ad Name: %s", n));
            qcc::String name(n);
            if (IsMaster()) {
                advertise.AddName(name, incomingNode);
            } else {
                incomingNode->AddAdvertiseName(name);
            }
        }

        for (j = 0; j < fnSize; ++j) {
            char* n;
            status = fnList[j].Get(SIG_NAME, &n);
            if (status != ER_OK) {
                QCC_LogError(status, ("Get find name failed"));
                foundNodeDB.Unlock(MUTEX_CONTEXT);
                lock.Unlock(MUTEX_CONTEXT);
                return status;
            }
            QCC_DbgPrintf(("    Find Name: %s", n));
            qcc::String name(n);
            if (IsMaster()) {
                find.AddName(name, incomingNode);
            }
        }

        incomingDB.AddNode(incomingNode);

        BTNodeInfo foundNode = foundNodeDB.FindNode(nodeAddr);
        if (foundNode->IsValid()) {
            if (!cnKnown) {
                foundNodeDB.RemoveNode(foundNode);
                foundNode->SetConnectNode(connectingNode);
                foundNodeDB.AddNode(foundNode);
            }

            // The incoming node is known to us so we need to find any
            // differences in advertise/find name sets from what we think we
            // know.
            BTNodeInfo added, removed;

            foundNode->Diff(incomingNode, &added, &removed);
            if (added->IsValid()) {
                // So that we can tell minions and client apps of the new names.
                addedDB.AddNode(added);
            }
            if (removed->IsValid()) {
                // So that we can tell minions and client apps of the lost names.
                removedDB.AddNode(removed);
            }

            // Update the advertise/find name sets with what we learned from
            // the incoming node.
            foundNode->Update(&added, &removed);
            if (IsMaster()) {
                // Move the node from foundNodeDB to nodeDB since it is now a minion.
                foundNodeDB.RemoveNode(foundNode);
                // Make sure node's unique name and EIR capability are up-to-date
                foundNode->SetUniqueName(incomingNode->GetUniqueName());
                foundNode->SetEIRCapable(incomingNode->IsEIRCapable());
                nodeDB.AddNode(foundNode);
            }
        } else {
            // The incoming node is completely new to us.
            addedDB.AddNode(incomingNode);
            if (IsMaster()) {
                nodeDB.AddNode(incomingNode);
            }
        }
    }

    status = ExtractNodeInfo(foundNodeArgs, numFoundNodes, newFoundDB);
    if (status != ER_OK) {
        foundNodeDB.Unlock(MUTEX_CONTEXT);
        lock.Unlock(MUTEX_CONTEXT);
        return status;
    }

    // At this point, the state of the various BTNodeDBs in use are as follows:
    // - nodeDB:      If we are the master, nodeDB now contains all the nodes
    //                that were added to the bus and is our minion.  If we are
    //                not the master then it has remained unchanged.
    // - foundNodeDB: If we are the master, then all the nodes that were added
    //                to the bus have been removed from foundNodeDB.  It could
    //                still contain stale information about nodes that were
    //                once connected to the incoming bus but are no longer part
    //                of that bus.  If we are not the master then the advertise
    //                names are now updated.  It could still have stale
    //                information about other nodes.
    // - incomingDB:  This contains all the information about all the nodes
    //                that just joined the bus regardless of our role.
    // - addedDB:     This contains any names we didn't know about for nodes we
    //                already know about.
    // - removedDB:   This contains names that are no longer valid for nodes we
    //                know about.
    // - newFoundDB:  This contains all the nodes the connecting node has found.
    //                If we are not the master then it also contains all the
    //                nodes connected added to the bus via the connecting node.

    // Trim all the devices in our nodeDB from newFoundDB since we are the
    // authoritative source for the nodes found in nodeDB.
    for (nodeit = nodeDB.Begin(); nodeit != nodeDB.End(); ++nodeit) {
        newFoundDB.RemoveNode(*nodeit);
    }

    if (IsMaster()) {
        // Since we are master, any nodes remaining in foundNodeDB that are
        // connectable via connectingNode must be from a stale advertisement.
        // Lets remove them.
        foundNodeDB.GetNodesFromConnectNode(connectingNode, staleDB);

    } else {
        // Extract the nodes connectable via connectingNode from newFoundDB
        // for use as authoritative information for updating foundNodeDB.
        BTNodeDB peerDB;
        if (incomingDB.Size() > 0) {
            incomingDB.GetNodesFromConnectNode(connectingNode, peerDB);
        } else {
            newFoundDB.GetNodesFromConnectNode(connectingNode, peerDB);
        }

        for (nodeit = peerDB.Begin(); nodeit != peerDB.End(); ++nodeit) {
            BTNodeInfo foundNode = foundNodeDB.FindNode((*nodeit)->GetBusAddress());
            if (foundNode->IsValid()) {
                /*
                 * The remote GUID of foundNode may not be correct if it is a node we were
                 * redirected too (see PrepConnect).  But nodeit does include the GUID, so update
                 * the GUID of foundNode with the correct information here.
                 *
                 * This is important to do before adding entries to addedDB or removedDB since
                 * the name found/lost machinery relies on the GUID being correct.
                 */
                if (foundNode->GetBusAddress() == connectingNode->GetBusAddress()) {
                    foundNode->SetGUID((*nodeit)->GetGUID());
                }
                BTNodeInfo added, removed;
                foundNode->Diff(*nodeit, &added, &removed);

                if (incomingDB.Size() == 0) {
                    if (added->IsValid()) {
                        // So that we can tell minions and client apps of the new names.
                        addedDB.AddNode(added);
                    }
                    if (removed->IsValid()) {
                        // So that we can tell minions and client apps of the lost names.
                        removedDB.AddNode(removed);
                    }
                }

                foundNode->Update(&added, &removed);
            } else {
                BTNodeInfo n = *nodeit;
                addedDB.AddNode(n);
                BTNodeInfo cn = n->GetConnectNode();
                BTNodeInfo fcn = foundNodeDB.FindNode(cn->GetBusAddress());
                if ((cn == fcn) && !cn.iden(fcn)) {
                    n->SetConnectNode(fcn);
                }
                foundNodeDB.AddNode(n);
            }
        }

        // Any nodes connectable via connectingNode that are in foundNodeDB
        // but not in peerDB must be from a stale advertisement.  Lets
        // remove them.
        BTNodeDB tmpDB;
        foundNodeDB.GetNodesFromConnectNode(connectingNode, tmpDB);
        tmpDB.NodeDiff(peerDB, NULL, &staleDB);
        tmpDB.Clear();
    }

    for (nodeit = staleDB.Begin(); nodeit != staleDB.End(); ++nodeit) {
        // We don't know what the connect address is for these nodes so we'll
        // forget about them and if they are still around we'll get the update
        // advertisements from them eventually anyway.
        foundNodeDB.RemoveNode(*nodeit);
        removedDB.AddNode(*nodeit);
    }

    // Now foundNodeDB is completely in sync with respect to the nodes that
    // joined the bus.  We can now remove the names/nodes from newFoundDB
    newFoundDB.UpdateDB(NULL, &foundNodeDB);

    // Now newFoundDB only contains names/nodes the connecting node knows
    // about that we do not know about.  Those names/nodes need to be added
    // foundNode as well as addedDB so that we can notify our existing minions
    // and local client apps.
    foundNodeDB.UpdateDB(&newFoundDB, NULL);
    addedDB.UpdateDB(&newFoundDB, NULL);

    foundNodeDB.DumpTable("foundNodeDB - Updated set of found devices from imported state information from new connection");

    if (IsMaster()) {
        ResetExpireNameAlarm();
        ++directMinions;

        QCC_DbgPrintf(("NodeDB after updating importing state information from connecting node"));
        QCC_DEBUG_ONLY(DumpNodeStateTable());

    } else {
        RemoveExpireNameAlarm();
    }

    foundNodeDB.Unlock(MUTEX_CONTEXT);
    lock.Unlock(MUTEX_CONTEXT);

    DistributeAdvertisedNameChanges(&addedDB, &removedDB);

    return ER_OK;
}


void BTController::UpdateDelegations(NameArgInfo& nameInfo)
{
    const bool advertiseOp = (&nameInfo == &advertise);

    QCC_DbgTrace(("BTController::UpdateDelegations(nameInfo = <%s>)",
                  advertiseOp ? "advertise" : "find"));

    const bool allowConn = (!advertiseOp | listening) && IsMaster() && ((nodeDB.Size() - 1) < maxConnections);
    const bool changed = nameInfo.Changed();
    const bool empty = nameInfo.Empty();
    const bool active = nameInfo.active;

    const bool start = !active && !empty && allowConn && devAvailable;
    const bool stop = active && (empty || !allowConn);
    const bool restart = active && changed && !empty && allowConn;

    QCC_DbgPrintf(("%s %s operation because device is %s, conn is %s, %s %s%s, and op is %s.",
                   start ? "Starting" : (restart ? "Updating" : (stop ? "Stopping" : "Skipping")),
                   advertiseOp ? "advertise" : "find",
                   devAvailable ? "available" : "not available",
                   allowConn ? "allowed" : "not allowed",
                   advertiseOp ? "name list" : "ignore addrs",
                   changed ? "changed" : "didn't change",
                   empty ? " to empty" : "",
                   active ? "active" : "not active"));

    assert(!(!active && stop));     // assert that we are not "stopping" an operation that is already stopped.
    assert(!(active && start));     // assert that we are not "starting" an operation that is already running.
    assert(!(!active && restart));  // assert that we are not "restarting" an operation that is stopped.
    assert(!(start && stop));
    assert(!(start && restart));
    assert(!(restart && stop));

    if (advertiseOp && changed) {
        ++masterUUIDRev;
        if (masterUUIDRev == bt::INVALID_UUIDREV) {
            ++masterUUIDRev;
        }
    }

    if (start) {
        // Set the advertise/find arguments.
        nameInfo.StartOp();

    } else if (restart) {
        // Update the advertise/find arguments.
        nameInfo.RestartOp();

    } else if (stop) {
        // Clear out the advertise/find arguments.
        nameInfo.StopOp(false);
    }
}


QStatus BTController::ExtractAdInfo(const MsgArg* entries, size_t size, BTNodeDB& adInfo)
{
    QCC_DbgTrace(("BTController::ExtractAdInfo()"));

    QStatus status = ER_OK;

    if (entries && (size > 0)) {
        for (size_t i = 0; i < size; ++i) {
            char* guidRaw;
            uint64_t rawAddr;
            uint16_t psm;
            MsgArg* names;
            size_t numNames;

            status = entries[i].Get(SIG_AD_NAME_MAP_ENTRY, &guidRaw, &rawAddr, &psm, &numNames, &names);

            if (status == ER_OK) {
                String guidStr(guidRaw);
                GUID128 guid(guidStr);
                String empty;
                BTBusAddress addr(rawAddr, psm);
                BTNodeInfo node(addr, empty, guid);

                QCC_DbgPrintf(("Extracting %u advertise names for %s:",
                               numNames, node->ToString().c_str()));
                for (size_t j = 0; j < numNames; ++j) {
                    char* name;
                    status = names[j].Get(SIG_NAME, &name);
                    if (status == ER_OK) {
                        QCC_DbgPrintf(("    %s", name));
                        node->AddAdvertiseName(String(name));
                    }
                }
                adInfo.AddNode(node);
            }
        }
    }
    return status;
}


QStatus BTController::ExtractNodeInfo(const MsgArg* entries, size_t size, BTNodeDB& db)
{
    QCC_DbgTrace(("BTController::ExtractNodeInfo()"));

    QStatus status = ER_OK;
    Timespec now;
    GetTimeNow(&now);
    uint64_t expireTime = now.GetAbsoluteMillis() + LOST_DEVICE_TIMEOUT;

    QCC_DbgPrintf(("Extracting node information from %lu connect nodes:", size));

    for (size_t i = 0; i < size; ++i) {
        uint64_t connAddrRaw;
        uint16_t connPSM;
        uint32_t uuidRev;
        size_t adMapSize;
        MsgArg* adMap;
        size_t j;

        status = entries[i].Get(SIG_FOUND_NODE_ENTRY, &connAddrRaw, &connPSM, &uuidRev, &adMapSize, &adMap);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed MsgArg::Get(\"%s\", ...)", SIG_FOUND_NODE_ENTRY));
            return status;
        }

        BTBusAddress connNodeAddr(BDAddress(connAddrRaw), connPSM);
        if ((self->GetBusAddress() == connNodeAddr) || nodeDB.FindNode(connNodeAddr)->IsValid()) {
            // Don't add ourself or any minions on our piconet/scatternet to foundNodeDB.
            QCC_DbgPrintf(("    Skipping nodes with connect address: %s", connNodeAddr.ToString().c_str()));
            continue;
        }

        BTNodeInfo connNode = BTNodeInfo(connNodeAddr);

        for (j = 0; j < adMapSize; ++j) {
            char* guidRaw;
            uint64_t rawBdAddr;
            uint16_t psm;
            size_t anSize;
            MsgArg* anList;
            size_t k;

            status = adMap[j].Get(SIG_AD_NAME_MAP_ENTRY, &guidRaw, &rawBdAddr, &psm, &anSize, &anList);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed MsgArg::Get(\"%s\", ...)", SIG_AD_NAME_MAP_ENTRY));
                return status;
            }

            BTBusAddress nodeAddr(BDAddress(rawBdAddr), psm);
            BTNodeInfo node = (nodeAddr == connNode->GetBusAddress()) ? connNode : BTNodeInfo(nodeAddr);

            // If the node is in our subnet, then use the real connect address.
            BTNodeInfo n = nodeDB.FindNode(nodeAddr);
            assert((n->IsValid() ? n->GetConnectNode() : connNode)->IsValid());
            node->SetConnectNode(n->IsValid() ? n->GetConnectNode() : connNode);

            String guidStr(guidRaw);
            GUID128 guid(guidStr);
            node->SetGUID(guid);
            node->SetUUIDRev(uuidRev);
            node->SetExpireTime(expireTime);
            QCC_DbgPrintf(("    Processing advertised names for device %lu-%lu %s (connectable via %s):",
                           i, j,
                           node->ToString().c_str(),
                           node->GetConnectNode()->ToString().c_str()));
            for (k = 0; k < anSize; ++k) {
                char* n;
                status = anList[k].Get(SIG_NAME, &n);
                if (status != ER_OK) {
                    QCC_LogError(status, ("Failed MsgArg::Get(\"%s\", ...)", SIG_NAME));
                    return status;
                }
                QCC_DbgPrintf(("        Name: %s", n));
                String name(n);
                node->AddAdvertiseName(name);
            }
            db.AddNode(node);
        }
    }
    return status;
}


void BTController::FillNodeStateMsgArgs(vector<MsgArg>& args) const
{
    BTNodeDB::const_iterator it;

    nodeDB.Lock(MUTEX_CONTEXT);
    args.reserve(nodeDB.Size());
    for (it = nodeDB.Begin(); it != nodeDB.End(); ++it) {
        const BTNodeInfo& node = *it;
        QCC_DbgPrintf(("    Node State node %s:", node->ToString().c_str()));
        NameSet::const_iterator nit;

        vector<const char*> nodeAdNames;
        nodeAdNames.reserve(node->AdvertiseNamesSize());
        for (nit = node->GetAdvertiseNamesBegin(); nit != node->GetAdvertiseNamesEnd(); ++nit) {
            QCC_DbgPrintf(("        Ad name: %s", nit->c_str()));
            nodeAdNames.push_back(nit->c_str());
        }

        vector<const char*> nodeFindNames;
        nodeFindNames.reserve(node->FindNamesSize());
        for (nit = node->GetFindNamesBegin(); nit != node->GetFindNamesEnd(); ++nit) {
            QCC_DbgPrintf(("        Find name: %s", nit->c_str()));
            nodeFindNames.push_back(nit->c_str());
        }
        QCC_DbgPrintf(("        EIR capable: %d", node->IsEIRCapable()));

        args.push_back(MsgArg(SIG_NODE_STATE_ENTRY,
                              node->GetGUID().ToString().c_str(),
                              node->GetUniqueName().c_str(),
                              node->GetBusAddress().addr.GetRaw(),
                              node->GetBusAddress().psm,
                              nodeAdNames.size(),
                              nodeAdNames.empty() ? NULL : &nodeAdNames.front(),
                              nodeFindNames.size(),
                              nodeFindNames.empty() ? NULL : &nodeFindNames.front(),
                              node->IsEIRCapable()));

        args.back().Stabilize();
    }
    nodeDB.Unlock(MUTEX_CONTEXT);
}


void BTController::FillFoundNodesMsgArgs(vector<MsgArg>& args, const BTNodeDB& adInfo)
{
    BTNodeDB::const_iterator it;
    map<BTBusAddress, BTNodeDB*>::iterator xmit;
    map<BTBusAddress, BTNodeDB*> xformMap;

    adInfo.Lock(MUTEX_CONTEXT);
    for (it = adInfo.Begin(); it != adInfo.End(); ++it) {
        BTBusAddress key = (&adInfo == &nodeDB) ? self->GetBusAddress() : (*it)->GetConnectNode()->GetBusAddress();
        BTNodeDB* xdb;
        xmit = xformMap.find(key);

        if (xmit == xformMap.end()) {
            xdb = new BTNodeDB();
            xformMap.insert(pair<BTBusAddress, BTNodeDB*>(key, xdb));
        } else {
            xdb = xmit->second;
        }
        xdb->AddNode(*it);
    }
    adInfo.Unlock(MUTEX_CONTEXT);

    args.reserve(args.size() + xformMap.size());
    xmit = xformMap.begin();
    while (xmit != xformMap.end()) {
        vector<MsgArg> adNamesArgs;

        const BTNodeDB& db = *(xmit->second);
        BTNodeInfo connNode = db.FindNode(xmit->first);

        if (!connNode->IsValid()) {
            connNode = foundNodeDB.FindNode(xmit->first);
        }

        if (!connNode->IsValid()) {
            connNode = nodeDB.FindNode(xmit->first);
        }

        if (connNode->IsValid()) {
            adNamesArgs.reserve(adInfo.Size());
            for (it = db.Begin(); it != db.End(); ++it) {
                const BTNodeInfo& node = *it;
                NameSet::const_iterator nit;

                vector<const char*> nodeAdNames;
                nodeAdNames.reserve(node->AdvertiseNamesSize());
                for (nit = node->GetAdvertiseNamesBegin(); nit != node->GetAdvertiseNamesEnd(); ++nit) {
                    nodeAdNames.push_back(nit->c_str());
                }

                adNamesArgs.push_back(MsgArg(SIG_AD_NAME_MAP_ENTRY,
                                             node->GetGUID().ToString().c_str(),
                                             node->GetBusAddress().addr.GetRaw(),
                                             node->GetBusAddress().psm,
                                             nodeAdNames.size(),
                                             nodeAdNames.empty() ? NULL : &nodeAdNames.front()));
                adNamesArgs.back().Stabilize();
            }

            BTBusAddress connAddr = nodeDB.FindNode(xmit->first)->IsValid() ? self->GetBusAddress() : xmit->first;

            args.push_back(MsgArg(SIG_FOUND_NODE_ENTRY,
                                  connAddr.addr.GetRaw(),
                                  connAddr.psm,
                                  connNode->GetUUIDRev(),
                                  adNamesArgs.size(),
                                  adNamesArgs.empty() ? NULL : &adNamesArgs.front()));
            args.back().Stabilize();

        } else {
            // Should never happen, since it is an internal bug (hence assert
            // check below), but gracefully handle it in case it does in
            // release mode.
            QCC_LogError(ER_NONE, ("Failed to find address %s in DB that should contain it!", xmit->first.ToString().c_str()));
            db.DumpTable("db: Corrupt DB?");
            assert(connNode->IsValid());
        }

        delete xmit->second;
        xformMap.erase(xmit);
        xmit = xformMap.begin();
    }
}


uint8_t BTController::ComputeSlaveFactor() const
{
    BTNodeDB::const_iterator nit;
    uint8_t cnt = 0;

    nodeDB.Lock(MUTEX_CONTEXT);
    for (nit = nodeDB.Begin(); nit != nodeDB.End(); ++nit) {
        const BTNodeInfo& minion = *nit;
        if (minion->IsDirectMinion()) {
            bool master = false;
            QStatus status = bt.IsMaster(minion->GetBusAddress().addr, master);
            if ((status == ER_OK) && !master) {
                ++cnt;
            } else if (status != ER_OK) {
                // failures count against us
                ++cnt;
            }
        }
    }
    nodeDB.Unlock(MUTEX_CONTEXT);

    return cnt;
}


void BTController::SetSelfAddress(const BTBusAddress& newAddr)
{
    BTNodeDB::const_iterator nit;
    vector<BTNodeInfo> dests;
    vector<BTNodeInfo>::const_iterator dit;

    MsgArg args[SIG_CONN_ADDR_CHANGED_SIZE];
    size_t argsSize = ArraySize(args);

    lock.Lock(MUTEX_CONTEXT);
    MsgArg::Set(args, argsSize, SIG_CONN_ADDR_CHANGED,
                self->GetBusAddress().addr.GetRaw(),
                self->GetBusAddress().psm,
                newAddr.addr.GetRaw(),
                newAddr.psm);

    dests.reserve(directMinions + (!IsMaster() ? 1 : 0));

    nodeDB.Lock(MUTEX_CONTEXT);
    nodeDB.RemoveNode(self);
    assert(newAddr.IsValid());
    self->SetBusAddress(newAddr);
    nodeDB.AddNode(self);
    for (nit = nodeDB.Begin(); nit != nodeDB.End(); ++nit) {
        const BTNodeInfo& minion = *nit;
        if (minion->IsDirectMinion()) {
            dests.push_back(minion);
        }
    }
    nodeDB.Unlock(MUTEX_CONTEXT);

    if (!IsMaster()) {
        dests.push_back(master->GetServiceName());
    }

    lock.Unlock(MUTEX_CONTEXT);

    for (dit = dests.begin(); dit != dests.end(); ++dit) {
        Signal((*dit)->GetUniqueName().c_str(), (*dit)->GetSessionID(), *org.alljoyn.Bus.BTController.ConnectAddrChanged, args, argsSize);
    }
}


void BTController::ResetExpireNameAlarm()
{
    RemoveExpireNameAlarm();
    uint64_t dispatchTime = foundNodeDB.NextNodeExpiration();
    if (dispatchTime < (numeric_limits<uint64_t>::max() - LOST_DEVICE_TIMEOUT_EXT)) {
        expireAlarm = DispatchOperation(new ExpireCachedNodesDispatchInfo(), dispatchTime + LOST_DEVICE_TIMEOUT_EXT);
    }
}


void BTController::JoinSessionNodeComplete()
{
    lock.Lock(MUTEX_CONTEXT);
    if (joinSessionNode->IsValid()) {
        joinSessionNode = BTNodeInfo();
        QCC_DbgPrintf(("joinSessionNode set to %s", joinSessionNode->ToString().c_str()));
        int ic = DecrementAndFetch(&incompleteConnections);
        QCC_DbgPrintf(("incompleteConnections = %d", ic));
        assert(ic >= 0);
        if (ic > 0) {
            connectCompleted.SetEvent();
        }
    }

    if (IsMaster()) {
        DispatchOperation(new UpdateDelegationsDispatchInfo());
    }

    lock.Unlock(MUTEX_CONTEXT);
}


void BTController::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    QCC_DbgTrace(("BTController::AlarmTriggered(alarm = <>, reasons = %s)", QCC_StatusText(reason)));
    DispatchInfo* op = static_cast<DispatchInfo*>(alarm->GetContext());
    assert(op);

    if (reason == ER_OK) {
        QCC_DbgPrintf(("Handling deferred operation:"));
        switch (op->operation) {
        case DispatchInfo::UPDATE_DELEGATIONS:
            lock.Lock(MUTEX_CONTEXT);
            if (incompleteConnections == 0) {
                QCC_DbgPrintf(("    Updating delegations"));
                UpdateDelegations(advertise);
                UpdateDelegations(find);
                QCC_DbgPrintf(("NodeDB after updating delegations"));
                QCC_DEBUG_ONLY(DumpNodeStateTable());
            }
            lock.Unlock(MUTEX_CONTEXT);
            break;

        case DispatchInfo::EXPIRE_CACHED_NODES: {
            QCC_DbgPrintf(("    Expire cached nodes"));
            BTNodeDB expiredDB;
            foundNodeDB.PopExpiredNodes(expiredDB);

            expiredDB.DumpTable("expiredDB - Expiring cached advertisements");
            foundNodeDB.DumpTable("foundNodeDB - Remaining cached advertisements after expiration");

            DistributeAdvertisedNameChanges(NULL, &expiredDB);
            uint64_t dispatchTime = foundNodeDB.NextNodeExpiration();
            if (dispatchTime < (numeric_limits<uint64_t>::max() - LOST_DEVICE_TIMEOUT_EXT)) {
                expireAlarm = DispatchOperation(new ExpireCachedNodesDispatchInfo(), dispatchTime + LOST_DEVICE_TIMEOUT_EXT);
            }
            break;
        }

        case DispatchInfo::NAME_LOST:
            QCC_DbgPrintf(("    Process local bus name lost"));
            DeferredNameLostHander(static_cast<NameLostDispatchInfo*>(op)->name);
            break;

        case DispatchInfo::BT_DEVICE_AVAILABLE:
            QCC_DbgPrintf(("    BT device available"));
            DeferredBTDeviceAvailable(static_cast<BTDevAvailDispatchInfo*>(op)->on);
            break;

        case DispatchInfo::SEND_SET_STATE: {
            QCC_DbgPrintf(("    Send set state"));
            DeferredSendSetState();
            break;
        }

        case DispatchInfo::PROCESS_SET_STATE_REPLY: {
            QCC_DbgPrintf(("    Process set state reply"));
            ProcessSetStateReplyDispatchInfo* di = static_cast<ProcessSetStateReplyDispatchInfo*>(op);
            DeferredProcessSetStateReply(di->msg, di->newMaster);
            break;
        }

        case DispatchInfo::HANDLE_DELEGATE_FIND:
            QCC_DbgPrintf(("    Handle delegate find"));
            DeferredHandleDelegateFind(static_cast<HandleDelegateOpDispatchInfo*>(op)->msg);
            break;

        case DispatchInfo::HANDLE_DELEGATE_ADVERTISE:
            QCC_DbgPrintf(("    Handle delegate advertise"));
            DeferredHandleDelegateAdvertise(static_cast<HandleDelegateOpDispatchInfo*>(op)->msg);
            break;

        case DispatchInfo::EXPIRE_BLACKLISTED_DEVICE:
            QCC_DbgPrintf(("    Expiring blacklisted device"));
            lock.Lock(MUTEX_CONTEXT);
            blacklist->erase(static_cast<ExpireBlacklistedDevDispatchInfo*>(op)->addr);
            find.dirty = true;
            UpdateDelegations(find);
            lock.Unlock(MUTEX_CONTEXT);
            break;
        }
    }

    delete op;
}


void BTController::PickNextDelegate(NameArgInfo& nameOp)
{
    if (nameOp.UseLocal()) {
        nameOp.minion = self;

    } else {
        BTNodeInfo skip;
        if (NumEIRMinions() > 1) {
            skip = (&nameOp == static_cast<NameArgInfo*>(&find)) ? advertise.minion : find.minion;
        }
        nameOp.minion = nodeDB.FindDelegateMinion(nameOp.minion, skip, (NumEIRMinions() > 0));
    }

    QCC_DbgPrintf(("Selected %s as %s delegate.  (UseLocal(): %s  EIR: %s  Num EIR Minions: %u  Num Minions: %u)",
                   (nameOp.minion == self) ? "ourself" : nameOp.minion->ToString().c_str(),
                   (&nameOp == static_cast<NameArgInfo*>(&find)) ? "find" : "advertise",
                   nameOp.UseLocal() ? "true" : "false",
                   bt.IsEIRCapable() ? "true" : "false",
                   NumEIRMinions(),
                   NumMinions()));
}


void BTController::NameArgInfo::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    QCC_DbgTrace(("BTController::NameArgInfo::AlarmTriggered(alarm = <%s>, reason = %s)",
                  alarm == bto.find.alarm ? "find" : "advertise", QCC_StatusText(reason)));

    if (reason == ER_OK) {
        bto.lock.Lock(MUTEX_CONTEXT);
        if (bto.RotateMinions() && !Empty()) {
            // Manually re-arm alarm since automatically recurring alarms cannot be stopped.
            StartAlarm();

            bto.PickNextDelegate(*this);
            SendDelegateSignal();
        } else if (Empty() && (alarm == bto.advertise.alarm)) {
            ClearArgs();
            SendDelegateSignal();
        }
        bto.lock.Unlock(MUTEX_CONTEXT);
    }
}


QStatus BTController::NameArgInfo::SendDelegateSignal()
{
    QCC_DbgPrintf(("Sending %s signal to %s (via session %x)", delegateSignal->name.c_str(),
                   minion->ToString().c_str(), minion->GetSessionID()));
    assert(minion != bto.self);

    NameArgs largs = args;
    bto.lock.Unlock(MUTEX_CONTEXT);  // SendDelegateSignal gets called with bto.lock held.
    QStatus status = bto.Signal(minion->GetUniqueName().c_str(), minion->GetSessionID(), *delegateSignal, largs->args, largs->argsSize);
    bto.lock.Lock(MUTEX_CONTEXT);

    return status;
}


void BTController::NameArgInfo::StartOp()
{
    QStatus status;
    size_t retry = ((bto.NumEIRMinions() > 0) ? bto.NumEIRMinions() :
                    (bto.directMinions > 0) ? bto.directMinions : 1);

    SetArgs();

    do {
        bto.PickNextDelegate(*this);

        if (minion == bto.self) {
            status = StartLocal();
        } else {
            status = SendDelegateSignal();
            if (bto.RotateMinions()) {
                assert(minion->IsValid());
                assert(minion != bto.self);
                if (status == ER_OK) {
                    StartAlarm();
                }
            }
        }
    } while ((status == ER_BUS_NO_ROUTE) && (--retry));

    if (status != ER_OK) {
        QCC_LogError(status, ("StartOp() failed"));
    }

    active = (status == ER_OK);
}


void BTController::NameArgInfo::StopOp(bool immediate)
{
    QStatus status;

    if ((this != &bto.advertise) || immediate) {
        ClearArgs();
    } else {
        SetArgs();  // Update advertise to inlcude all devices with no advertised names
    }

    if (this == &bto.advertise) {
        // Set the duration to the delegate time if this is not an immediate stop operation command.
        args->args[SIG_DELEGATE_AD_DURATION_PARAM].Set(SIG_DURATION, immediate ? static_cast<uint32_t>(0) : static_cast<uint32_t>(DELEGATE_TIME));
    }

    active = false;

    if (minion == bto.self) {
        status = StopLocal(immediate);
    } else {
        status = SendDelegateSignal();
        StopAlarm();
        active = !(status == ER_OK);
    }

    if ((this == &bto.advertise) && !immediate) {
        ClearArgs();
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("StopOp() failed"));
    }

}


BTController::AdvertiseNameArgInfo::AdvertiseNameArgInfo(BTController& bto) :
    NameArgInfo(bto, SIG_DELEGATE_AD_SIZE)
{
}

void BTController::AdvertiseNameArgInfo::AddName(const qcc::String& name, BTNodeInfo& node)
{
    node->AddAdvertiseName(name);
    ++count;
    dirty = true;
}


void BTController::AdvertiseNameArgInfo::RemoveName(const qcc::String& name, BTNodeInfo& node)
{
    NameSet::iterator nit = node->FindAdvertiseName(name);
    if (nit != node->GetAdvertiseNamesEnd()) {
        node->RemoveAdvertiseName(nit);
        --count;
        dirty = true;
    }
}


void BTController::AdvertiseNameArgInfo::SetArgs()
{
    QCC_DbgTrace(("BTController::AdvertiseNameArgInfo::SetArgs()"));
    NameArgs newArgs(argsSize);
    size_t localArgsSize = argsSize;

    bto.nodeDB.Lock(MUTEX_CONTEXT);
    adInfoArgs.clear();
    adInfoArgs.reserve(bto.nodeDB.Size());

    BTNodeDB::const_iterator nodeit;
    NameSet::const_iterator nameit;
    vector<const char*> names;
    for (nodeit = bto.nodeDB.Begin(); nodeit != bto.nodeDB.End(); ++nodeit) {
        const BTNodeInfo& node = *nodeit;
        names.clear();
        names.reserve(node->AdvertiseNamesSize());
        for (nameit = node->GetAdvertiseNamesBegin(); nameit != node->GetAdvertiseNamesEnd(); ++nameit) {
            names.push_back(nameit->c_str());
        }
        adInfoArgs.push_back(MsgArg(SIG_AD_NAME_MAP_ENTRY,
                                    node->GetGUID().ToString().c_str(),
                                    node->GetBusAddress().addr.GetRaw(),
                                    node->GetBusAddress().psm,
                                    names.size(),
                                    names.empty() ? NULL : &names.front()));
        adInfoArgs.back().Stabilize();
    }

    bto.nodeDB.Unlock(MUTEX_CONTEXT);

    MsgArg::Set(newArgs->args, localArgsSize, SIG_DELEGATE_AD,
                bto.masterUUIDRev,
                bto.self->GetBusAddress().addr.GetRaw(),
                bto.self->GetBusAddress().psm,
                adInfoArgs.size(), adInfoArgs.empty() ? NULL : &adInfoArgs.front(),
                bto.RotateMinions() ? DELEGATE_TIME : (uint32_t)0);
    assert(localArgsSize == argsSize);

    bto.lock.Lock(MUTEX_CONTEXT);
    args = newArgs;
    bto.lock.Unlock(MUTEX_CONTEXT);

    dirty = false;
}


void BTController::AdvertiseNameArgInfo::ClearArgs()
{
    QCC_DbgTrace(("BTController::AdvertiseNameArgInfo::ClearArgs()"));
    NameArgs newArgs(argsSize);
    size_t localArgsSize = argsSize;

    /* Advertise an empty list for a while */
    MsgArg::Set(newArgs->args, localArgsSize, SIG_DELEGATE_AD,
                bt::INVALID_UUIDREV,
                0ULL,
                bt::INVALID_PSM,
                static_cast<size_t>(0), NULL,
                static_cast<uint32_t>(0));
    assert(localArgsSize == argsSize);

    bto.lock.Lock(MUTEX_CONTEXT);
    args = newArgs;
    bto.lock.Unlock(MUTEX_CONTEXT);
}


QStatus BTController::AdvertiseNameArgInfo::StartLocal()
{
    QStatus status;
    BTNodeDB adInfo;

    status = ExtractAdInfo(adInfoArgs.empty() ? NULL : &adInfoArgs.front(), adInfoArgs.size(), adInfo);
    if (status == ER_OK) {
        status = bto.bt.StartAdvertise(bto.masterUUIDRev, bto.self->GetBusAddress().addr, bto.self->GetBusAddress().psm, adInfo);
    }
    return status;
}


QStatus BTController::AdvertiseNameArgInfo::StopLocal(bool immediate)
{
    QStatus status;
    StopAlarm();
    if (immediate) {
        status = bto.bt.StopAdvertise();
    } else {
        // Advertise the (presumably) empty set of advertise names for 30 seconds.
        status = bto.bt.StartAdvertise(bto.masterUUIDRev,
                                       bto.self->GetBusAddress().addr, bto.self->GetBusAddress().psm,
                                       bto.nodeDB, BTController::DELEGATE_TIME);
    }
    active = !(status == ER_OK);
    return status;
}


BTController::FindNameArgInfo::FindNameArgInfo(BTController& bto) :
    NameArgInfo(bto, SIG_DELEGATE_FIND_SIZE)
{
}


void BTController::FindNameArgInfo::AddName(const qcc::String& name, BTNodeInfo& node)
{
    node->AddFindName(name);
    ++count;
}


void BTController::FindNameArgInfo::RemoveName(const qcc::String& name, BTNodeInfo& node)
{
    NameSet::iterator nit = node->FindFindName(name);
    if (nit != node->GetFindNamesEnd()) {
        node->RemoveFindName(nit);
        --count;
    }
}


void BTController::FindNameArgInfo::SetArgs()
{
    QCC_DbgTrace(("BTController::FindNameArgInfo::SetArgs()"));
    NameArgs newArgs(argsSize);
    size_t localArgsSize = argsSize;

    bto.lock.Lock(MUTEX_CONTEXT);
    bto.nodeDB.Lock(MUTEX_CONTEXT);
    ignoreAddrsCache.clear();
    ignoreAddrsCache.reserve(bto.nodeDB.Size() + bto.blacklist->size());
    BTNodeDB::const_iterator it;
    for (it = bto.nodeDB.Begin(); it != bto.nodeDB.End(); ++it) {
        ignoreAddrsCache.push_back((*it)->GetBusAddress().addr.GetRaw());
    }
    bto.nodeDB.Unlock(MUTEX_CONTEXT);

    set<BDAddress>::const_iterator bit;
    for (bit = bto.blacklist->begin(); bit != bto.blacklist->end(); ++bit) {
        ignoreAddrsCache.push_back(bit->GetRaw());
    }

    MsgArg::Set(newArgs->args, localArgsSize, SIG_DELEGATE_FIND,
                ignoreAddrsCache.size(),
                ignoreAddrsCache.empty() ? NULL : &ignoreAddrsCache.front(),
                bto.RotateMinions() ? DELEGATE_TIME : (uint32_t)0);
    assert(localArgsSize == argsSize);

    args = newArgs;
    bto.lock.Unlock(MUTEX_CONTEXT);

    dirty = false;
}


void BTController::FindNameArgInfo::ClearArgs()
{
    QCC_DbgTrace(("BTController::FindNameArgInfo::ClearArgs()"));
    NameArgs newArgs(argsSize);
    size_t localArgsSize = argsSize;

    MsgArg::Set(newArgs->args, localArgsSize, SIG_DELEGATE_FIND,
                static_cast<size_t>(0), NULL,
                static_cast<uint32_t>(0));
    assert(localArgsSize == argsSize);

    bto.lock.Lock(MUTEX_CONTEXT);
    args = newArgs;
    bto.lock.Unlock(MUTEX_CONTEXT);
}


QStatus BTController::FindNameArgInfo::StartLocal()
{
    bto.nodeDB.Lock(MUTEX_CONTEXT);
    BDAddressSet ignoreAddrs(*bto.blacklist); // initialize ignore addresses with the blacklist
    BTNodeDB::const_iterator it;
    for (it = bto.nodeDB.Begin(); it != bto.nodeDB.End(); ++it) {
        ignoreAddrs->insert((*it)->GetBusAddress().addr);
    }
    bto.nodeDB.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("Starting local find..."));
    return bto.bt.StartFind(ignoreAddrs);
}


QStatus BTController::FindNameArgInfo::StopLocal(bool immediate)
{
    StopAlarm();
    QStatus status = bto.bt.StopFind();
    active = !(status == ER_OK);
    return status;
}


size_t BTController::NumEIRMinions() const
{
    if (!IsMaster()) {
        return 0;
    }

    size_t eirMinions = 0;
    nodeDB.Lock(MUTEX_CONTEXT);
    for (BTNodeDB::const_iterator it = nodeDB.Begin(); it != nodeDB.End(); ++it) {
        if (((*it) != self) && (*it)->IsEIRCapable()) {
            ++eirMinions;
        }
    }
    nodeDB.Unlock(MUTEX_CONTEXT);
    return eirMinions;
}


#ifndef NDEBUG
void BTController::DumpNodeStateTable() const
{
    BTNodeDB::const_iterator nodeit;
    QCC_DbgPrintf(("Node State Table (local = %s):", bus.GetUniqueName().c_str()));
    for (nodeit = nodeDB.Begin(); nodeit != nodeDB.End(); ++nodeit) {
        const BTNodeInfo& node = *nodeit;
        NameSet::const_iterator nameit;
        QCC_DbgPrintf(("    %s (conn: %s) %s (%s%s%s%s):",
                       node->ToString().c_str(),
                       node->GetConnectNode()->ToString().c_str(),
                       node->GetUniqueName().c_str(),
                       (node == self) ? "local" : (node->IsDirectMinion() ? "direct minion" : "indirect minion"),
                       ((node == find.minion) || (node == advertise.minion)) ? " -" : "",
                       (node == find.minion) ? " find" : "",
                       (node == advertise.minion) ? " advertise" : ""));
        QCC_DbgPrintf(("         Advertise names:"));
        for (nameit = node->GetAdvertiseNamesBegin(); nameit != node->GetAdvertiseNamesEnd(); ++nameit) {
            QCC_DbgPrintf(("            %s", nameit->c_str()));
        }
        QCC_DbgPrintf(("         Find names:"));
        for (nameit = node->GetFindNamesBegin(); nameit != node->GetFindNamesEnd(); ++nameit) {
            QCC_DbgPrintf(("            %s", nameit->c_str()));
        }
    }
}


void BTController::FlushCachedNames()
{
    if (IsMaster()) {
        DistributeAdvertisedNameChanges(NULL, &foundNodeDB);
        foundNodeDB.Clear();
    } else {
        const InterfaceDescription* ifc;
        ifc = master->GetInterface("org.alljoyn.Bus.Debug.BT");
        if (!ifc) {
            ifc = bus.GetInterface("org.alljoyn.Bus.Debug.BT");
            if (!ifc) {
                InterfaceDescription* newIfc;
                bus.CreateInterface("org.alljoyn.Bus.Debug.BT", newIfc);
                if (newIfc) {
                    newIfc->AddMethod("FlushDiscoverTimes", NULL, NULL, NULL, 0);
                    newIfc->AddMethod("FlushSDPQueryTimes", NULL, NULL, NULL, 0);
                    newIfc->AddMethod("FlushConnectTimes", NULL, NULL, NULL, 0);
                    newIfc->AddMethod("FlushCachedNames", NULL, NULL, NULL, 0);
                    newIfc->AddProperty("DiscoverTimes", "a(su)", PROP_ACCESS_READ);
                    newIfc->AddProperty("SDPQueryTimes", "a(su)", PROP_ACCESS_READ);
                    newIfc->AddProperty("ConnectTimes", "a(su)", PROP_ACCESS_READ);
                    newIfc->Activate();
                    ifc = newIfc;
                }
            }

            if (ifc) {
                master->AddInterface(*ifc);
            }
        }

        if (ifc) {
            master->MethodCall("org.alljoyn.Bus.Debug.BT", "FlushCachedNames", NULL, 0);
        }
    }
}
#endif


}
