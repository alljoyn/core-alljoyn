/**
 * @file
 * BTAccessor declaration for BlueZ
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
#ifndef _ALLJOYN_BTACCESSOR_H
#define _ALLJOYN_BTACCESSOR_H

#include <qcc/platform.h>

#include <qcc/Event.h>
#include <qcc/ManagedObj.h>
#include <qcc/String.h>
#include <qcc/Timer.h>
#include <qcc/XmlElement.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/ProxyBusObject.h>

#include "AdapterObject.h"
#include "BDAddress.h"
#include "BlueZUtils.h"
#include "BTController.h"
#include "BTNodeDB.h"
#include "BTNodeInfo.h"
#include "BTTransport.h"
#include "RemoteEndpoint.h"

#include <alljoyn/Status.h>


namespace ajn {

class BTTransport::BTAccessor : public MessageReceiver, public qcc::AlarmListener, public BusListener {
  public:
    /**
     * Constructor
     *
     * @param transport
     * @param busGuid
     */
    BTAccessor(BTTransport* transport, const qcc::String& busGuid);

    /**
     * Destructor
     */
    ~BTAccessor();

    /**
     * Start the underlying Bluetooth subsystem.
     *
     * @return ER_OK if successful.
     */
    QStatus Start();

    /**
     * Start the underlying Bluetooth subsystem.
     */
    void Stop();

    /**
     * Start discovery (inquiry)
     *
     * @param ignoreAddrs   Set of BD Addresses to ignore
     * @param duration      Number of seconds to discover (0 = forever, default is 0)
     */
    QStatus StartDiscovery(const BDAddressSet& ignoreAddrs, uint32_t duration = 0);

    /**
     * Stop discovery (inquiry)
     */
    QStatus StopDiscovery();

    /**
     * Start discoverability (inquiry scan)
     */
    QStatus StartDiscoverability(uint32_t duration = 0);

    /**
     * Stop discoverability (inquiry scan)
     */
    QStatus StopDiscoverability();

    /**
     * Set SDP information
     *
     * @param uuidRev   Bus UUID revision to advertise in the SDP record
     * @param bdAddr    Bluetooth device address that of the node that is connectable
     * @param psm       L2CAP PSM number accepting connections
     * @param adInfo    Map of bus node GUIDs and bus names to advertise
     */
    QStatus SetSDPInfo(uint32_t uuidRev,
                       const BDAddress& bdAddr,
                       uint16_t psm,
                       const BTNodeDB& adInfo);

    /**
     * Make the Bluetooth device connectable.
     *
     * @param addr[out] Bluetooth device address that is connectable
     * @param psm[out]  L2CAP PSM that is connectable (0 if not connectable)
     *
     * @return  ER_OK if device is now connectable
     */
    QStatus StartConnectable(BDAddress& addr,
                             uint16_t& psm);

    /**
     * Make the Bluetooth device not connectable.
     */
    void StopConnectable();

    /**
     * Accepts an incoming connection from a remote Bluetooth device.
     *
     * @param alljoyn       BusAttachment that will be connected to the resulting endpoint
     * @param connectEvent  The event signalling the incoming connection
     *
     * @return  A newly instatiated remote endpoint for the Bluetooth connection (NULL indicates a failure)
     */
    RemoteEndpoint Accept(BusAttachment& alljoyn,
                          qcc::Event* connectEvent);

    /**
     * Create an outgoing connection to a remote Bluetooth device.  If the
     * L2CAP PSM are not specified, then an SDP query will be performed to get
     * that information.
     *
     * @param alljoyn   BusAttachment that will be connected to the resulting endpoint
     *
     * @return  A newly instatiated remote endpoint for the Bluetooth connection (NULL indicates a failure)
     */
    RemoteEndpoint Connect(BusAttachment& alljoyn,
                           const BTNodeInfo& node);

    /**
     * Perform an SDP queary on the specified device to get the bus information.
     *
     * @param addr          Bluetooth device address to retrieve the SDP record from
     * @param uuidRev[out]  Bus UUID revision to found in the SDP record
     * @param connAddr[out] Address of the Bluetooth device accepting connections.
     * @param adInfo[out]   Map of bus node GUIDs and bus names being advertised
     */
    QStatus GetDeviceInfo(const BDAddress& addr,
                          uint32_t* uuidRev,
                          BTBusAddress* connAddr,
                          BTNodeDB* adInfo);

    /**
     * Accessor to get the L2CAP connect event object.
     *
     * @return pointer to the L2CAP connect event object.
     */
    qcc::Event* GetL2CAPConnectEvent() { return l2capEvent; }

    /**
     * This looks up the low level Bluetooth connection information for the
     * connection with the specified device and indicates if our role for that
     * connection is master or slave.
     *
     * @param addr          Bluetooth device address for the connection of interest.
     * @param master[out]   - 'true' if we are master of the connection with addr
     *                      - 'false' if we are slave of the connection with addr
     *
     * @return  ER_OK if successful; an error will be returned if there is no
     *          connection with the specified device
     */
    QStatus IsMaster(const BDAddress& addr, bool& master) const;

    /**
     * This function forces a role switch in the HCI device so that we become
     * master of the connection with the specified device.
     *
     * @param addr  Bluetooth device address for the connection of interest
     * @param role  Requested Bluetooth connection role
     */
    void RequestBTRole(const BDAddress& addr, bt::BluetoothRole role);

    bool IsEIRCapable() const;


  private:

    struct DispatchInfo;

    void ConnectBlueZ();
    void DisconnectBlueZ();

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner);

    /* Adapter management functions */
    QStatus EnumerateAdapters();
    void AdapterAdded(const char* adapterObjPath);
    void AdapterRemoved(const char* adapterObjPath);
    void DefaultAdapterChanged(const char* adapterObjPath);

    /* Adapter management signal handlers */
    void AdapterAddedSignalHandler(const InterfaceDescription::Member* member,
                                   const char* sourcePath,
                                   Message& msg);
    void AdapterRemovedSignalHandler(const InterfaceDescription::Member* member,
                                     const char* sourcePath,
                                     Message& msg);
    void DefaultAdapterChangedSignalHandler(const InterfaceDescription::Member* member,
                                            const char* sourcePath,
                                            Message& msg);
    void AdapterPropertyChangedSignalHandler(const InterfaceDescription::Member* member,
                                             const char* sourcePath,
                                             Message& msg);


    /* Device presence management signal handlers */
    void DeviceFoundSignalHandler(const InterfaceDescription::Member* member,
                                  const char* sourcePath,
                                  Message& msg);
    void DeviceCreatedSignalHandler(const InterfaceDescription::Member* member,
                                    const char* sourcePath,
                                    Message& msg);
    void DeviceRemovedSignalHandler(const InterfaceDescription::Member* member,
                                    const char* sourcePath,
                                    Message& msg);
    void DevicePropertyChangedSignalHandler(const InterfaceDescription::Member* member,
                                            const char* sourcePath,
                                            Message& msg);

    /* support */
    QStatus InitializeAdapterInformation(bluez::AdapterObject& adapter);
    QStatus AddRecord(const char* recordXml,
                      uint32_t& newHandle);
    void RemoveRecord();
    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);
    static bool FindAllJoynUUID(const MsgArg* uuids,
                                size_t listSize,
                                uint32_t& uuidRev);
    void ExpireFoundDevices(bool all);
    static QStatus ProcessSDPXML(qcc::XmlParseContext& xmlctx,
                                 uint32_t* uuidRev,
                                 BDAddress* connAddr,
                                 uint16_t* connPSM,
                                 BTNodeDB* adInfo);
    static QStatus ProcessXMLAdvertisementsAttr(const qcc::XmlElement* elem,
                                                BTNodeDB& adInfo,
                                                uint32_t remoteVersion);
    QStatus GetDeviceObjPath(const BDAddress& bdAddr,
                             qcc::String& devObjPath);
    QStatus DiscoveryControl(bool start);
    QStatus DiscoveryControl(const InterfaceDescription::Member* method);
    QStatus SetDiscoverabilityProperty();

    bluez::AdapterObject GetAdapterObject(const qcc::String& adapterObjPath) const
    {
        bluez::AdapterObject adapter;
        assert(!adapter->IsValid());
        adapterLock.Lock(MUTEX_CONTEXT);
        AdapterMap::const_iterator it(adapterMap.find(adapterObjPath));
        if (it != adapterMap.end()) {
            adapter = it->second;
        }
        adapterLock.Unlock(MUTEX_CONTEXT);
        return adapter;
    }

    bluez::AdapterObject GetDefaultAdapterObject() const
    {
        adapterLock.Lock(MUTEX_CONTEXT);
        bluez::AdapterObject adapter(defaultAdapterObj);
        adapterLock.Unlock(MUTEX_CONTEXT);
        return adapter;
    }

    bluez::AdapterObject GetAnyAdapterObject() const
    {
        adapterLock.Lock(MUTEX_CONTEXT);
        bluez::AdapterObject adapter(anyAdapterObj);
        adapterLock.Unlock(MUTEX_CONTEXT);
        return adapter;
    }

    qcc::Alarm DispatchOperation(DispatchInfo* op, uint32_t delay = 0)
    {
        void* context = (void*)op;
        qcc::Alarm alarm(delay, this, context);
        timer.AddAlarm(alarm);
        return alarm;
    }

    qcc::Alarm DispatchOperation(DispatchInfo* op, uint64_t triggerTime)
    {
        void* context = (void*)op;
        qcc::Timespec ts(triggerTime);
        qcc::Alarm alarm(ts, this, context);
        timer.AddAlarm(alarm);
        return alarm;
    }



/******************************************************************************/


    typedef std::map<qcc::StringMapKey, bluez::AdapterObject> AdapterMap;

    class FoundInfo {
      public:
        FoundInfo() :
            uuidRev(bt::INVALID_UUIDREV),
            timeout(0)
        { }
        uint32_t uuidRev;
        uint64_t timeout;
    };
    typedef std::map<BDAddress, FoundInfo> FoundInfoMap;
    typedef std::multimap<uint64_t, BDAddress> FoundInfoExpireMap;

    struct DispatchInfo {
        typedef enum {
            STOP_DISCOVERY,
            STOP_DISCOVERABILITY,
            ADAPTER_ADDED,
            ADAPTER_REMOVED,
            DEFAULT_ADAPTER_CHANGED,
            DEVICE_FOUND,
            EXPIRE_DEVICE_FOUND,
            FLUSH_FOUND_EXPIRATIONS
        } DispatchTypes;
        DispatchTypes operation;

        DispatchInfo(DispatchTypes operation) : operation(operation) { }
        virtual ~DispatchInfo() { }
    };

    struct AdapterDispatchInfo : public DispatchInfo {
        qcc::String adapterPath;

        AdapterDispatchInfo(DispatchTypes operation, const char* adapterPath) :
            DispatchInfo(operation), adapterPath(adapterPath) { }
    };

    struct DeviceDispatchInfo : public DispatchInfo {
        BDAddress addr;
        uint32_t uuidRev;
        bool eirCapable;

        DeviceDispatchInfo(DispatchTypes operation, const BDAddress& addr, uint32_t uuidRev, bool eirCapable) :
            DispatchInfo(operation), addr(addr), uuidRev(uuidRev), eirCapable(eirCapable) { }
    };

    struct MsgDispatchInfo : public DispatchInfo {
        MsgArg* args;
        size_t argCnt;

        MsgDispatchInfo(DispatchTypes operation, MsgArg* args, size_t argCnt) :
            DispatchInfo(operation), args(args), argCnt(argCnt) { }
    };


    BusAttachment bzBus;
    const qcc::String busGuid;
    qcc::String connectArgs;

    ProxyBusObject bzManagerObj;
    bluez::AdapterObject defaultAdapterObj;
    bluez::AdapterObject anyAdapterObj;
    AdapterMap adapterMap;
    mutable qcc::Mutex adapterLock; // Generic lock for adapter related objects, maps, etc.

    BTTransport* transport;

    uint32_t recordHandle;

    mutable qcc::Mutex deviceLock; // Generic lock for device related objects, maps, etc.
    FoundInfoMap foundDevices;  // Map of found AllJoyn devices w/ UUID-Rev and expire time.
    FoundInfoExpireMap foundExpirations;
    qcc::Timer timer;
    qcc::Alarm expireAlarm;
    qcc::Alarm stopAdAlarm;
    BDAddressSet ignoreAddrs;

    std::set<qcc::StringMapKey> createdDevices;  // Set of devices we created

    bool bluetoothAvailable;
    bool discoverable;
    volatile int32_t discoveryCtrl;

    qcc::SocketFd l2capLFd;
    qcc::Event* l2capEvent;

    uint32_t cod;

    struct {
        struct {
            struct {
                const InterfaceDescription* interface;
                // Methods (not all; only those needed)
                const InterfaceDescription::Member* DefaultAdapter;
                const InterfaceDescription::Member* ListAdapters;
                // Signals
                const InterfaceDescription::Member* AdapterAdded;
                const InterfaceDescription::Member* AdapterRemoved;
                const InterfaceDescription::Member* DefaultAdapterChanged;
            } Manager;

            struct {
                const InterfaceDescription* interface;
                // Methods (not all; only those needed)
                const InterfaceDescription::Member* AddRecord;
                const InterfaceDescription::Member* RemoveRecord;
            } Service;

            struct {
                const InterfaceDescription* interface;
                // Methods (not all; only those needed)
                const InterfaceDescription::Member* CreateDevice;
                const InterfaceDescription::Member* FindDevice;
                const InterfaceDescription::Member* GetProperties;
                const InterfaceDescription::Member* ListDevices;
                const InterfaceDescription::Member* RemoveDevice;
                const InterfaceDescription::Member* SetProperty;
                const InterfaceDescription::Member* StartDiscovery;
                const InterfaceDescription::Member* StopDiscovery;
                // Signals
                const InterfaceDescription::Member* DeviceCreated;
                const InterfaceDescription::Member* DeviceDisappeared;
                const InterfaceDescription::Member* DeviceFound;
                const InterfaceDescription::Member* DeviceRemoved;
                const InterfaceDescription::Member* PropertyChanged;
            } Adapter;

            struct {
                const InterfaceDescription* interface;
                // Methods (not all; only those needed)
                const InterfaceDescription::Member* DiscoverServices;
                const InterfaceDescription::Member* GetProperties;
                // Signals
                const InterfaceDescription::Member* DisconnectRequested;
                const InterfaceDescription::Member* PropertyChanged;
            } Device;
        } bluez;
    } org;

};


} // namespace ajn


#endif
