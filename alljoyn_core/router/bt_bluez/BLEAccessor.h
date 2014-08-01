/**
 * @file
 * BLEAccessor declaration for BlueZ
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/BDAddress.h>
#include <qcc/BLEStream.h>
#include <qcc/BLEStreamAccessor.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/ProxyBusObject.h>

#include "AdapterObject.h"
#include "DeviceObject.h"
#include "BTController.h"
#include "DaemonBLETransport.h"
#include "RemoteEndpoint.h"

#include <alljoyn/Status.h>


namespace ajn {

class DaemonBLETransport::
BLEAccessor :
    public qcc::BLEStreamAccessor,
    public MessageReceiver,
    public qcc::AlarmListener,
    public BusListener,
    private ProxyBusObject::PropertiesChangedListener {
  public:
    /**
     * Constructor
     *
     * @param transport
     * @param busGuid
     */
    BLEAccessor(DaemonBLETransport* transport, const qcc::String& busGuid);

    /**
     * Destructor
     */
    ~BLEAccessor();

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
     * Push Bytes
     */
    QStatus PushBytes(qcc::String remObj, const void* buf, size_t numBytes, size_t& actualBytes);

    /**
     * Make the Bluetooth device connectable.
     *
     * @return  ER_OK if device is now connectable
     */
    QStatus StartConnectable();

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
    RemoteEndpoint Connect(BusAttachment& alljoyn, qcc::String objPath);

    /**
     * Accessor to get the L2CAP connect event object.
     *
     * @return pointer to the L2CAP connect event object.
     */
    qcc::Event* GetL2CAPConnectEvent() { return l2capEvent; }

  private:

    struct DispatchInfo;

    void ConnectBlueZ();
    void DisconnectBlueZ();

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner);

    /* Adapter management functions */
    QStatus EnumerateAdapters();
    void AdapterAdded(const char* adapterObjPath);
    void AdapterAdded(const char* adapterObjPath,
                      const MsgArg* props);
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
    void RxDataRecvSignalHandler(const InterfaceDescription::Member* member,
                                 const char* sourcePath,
                                 Message& msg);
    void InterfacesAddedSignalHandler(const InterfaceDescription::Member* member,
                                      const char* sourcePath,
                                      Message& msg);
    void InterfacesRemovedSignalHandler(const InterfaceDescription::Member* member,
                                        const char* sourcePath,
                                        Message& msg);

    /* support */
    QStatus InitializeAdapterInformation(bluez::AdapterObject& adapter,
                                         const MsgArg* props);
    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);
    static bool FindAllJoynUUID(const MsgArg* uuids,
                                size_t listSize,
                                uint32_t& uuidRev);
    void ExpireFoundDevices(bool all);
    QStatus GetDeviceObjPath(const qcc::BDAddress& bdAddr,
                             qcc::String& devObjPath);
    QStatus DiscoveryControl(bool start);
    QStatus DiscoveryControl(const InterfaceDescription::Member* method);

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

    void PropertiesChanged(ProxyBusObject& obj,
                           const char* ifaceName,
                           const MsgArg& changed,
                           const MsgArg& invalidated,
                           void* context);



/******************************************************************************/


    typedef std::map<qcc::String, qcc::BLEController*> DeviceMap;
    typedef std::map<qcc::String, bluez::DeviceObject*> DeviceProxyMap;
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
    typedef std::map<qcc::BDAddress, FoundInfo> FoundInfoMap;
    typedef std::multimap<uint64_t, qcc::BDAddress> FoundInfoExpireMap;

    struct DispatchInfo {
        typedef enum {
            STOP_DISCOVERY,
            ADAPTER_ADDED,
            ADAPTER_REMOVED,
            DEFAULT_ADAPTER_CHANGED,
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
        qcc::BDAddress addr;
        uint32_t uuidRev;
        bool eirCapable;

        DeviceDispatchInfo(DispatchTypes operation, const qcc::BDAddress& addr, uint32_t uuidRev, bool eirCapable) :
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
    DeviceMap deviceMap;
    DeviceProxyMap deviceProxyMap;
    mutable qcc::Mutex adapterLock; // Generic lock for adapter related objects, maps, etc.

    DaemonBLETransport* transport;

    uint32_t recordHandle;

    mutable qcc::Mutex deviceLock; // Generic lock for device related objects, maps, etc.
    FoundInfoMap foundDevices;  // Map of found AllJoyn devices w/ UUID-Rev and expire time.
    FoundInfoExpireMap foundExpirations;
    qcc::Timer timer;
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
                const InterfaceDescription::Member* GetManagedObjects;
                // Signals
                const InterfaceDescription::Member* InterfacesAdded;
                const InterfaceDescription::Member* InterfacesRemoved;
            } ObjMgr;

            struct {
                const InterfaceDescription* interface;
                // Methods (not all; only those needed)
                const InterfaceDescription::Member* RemoveDevice;
                const InterfaceDescription::Member* StartDiscovery;
                const InterfaceDescription::Member* StopDiscovery;
            } Adapter1;

            struct {
                const InterfaceDescription* interface;
                // Methods (not all; only those needed)
                const InterfaceDescription::Member* SetUuid;
            } AllJoynMgr;

            struct {
                const InterfaceDescription* interface;
                // Methods (not all; only those needed)
                const InterfaceDescription::Member* TxDataSend;
                // Signals
                const InterfaceDescription::Member* RxDataRecv;
            } AllJoyn;

            struct {
                const InterfaceDescription* interface;
                // Methods (not all; only those needed)
                const InterfaceDescription::Member* Connect;
                const InterfaceDescription::Member* Disconnect;
                const InterfaceDescription::Member* ConnectProfile;
                const InterfaceDescription::Member* DisconnectProfile;
                const InterfaceDescription::Member* Pair;
                const InterfaceDescription::Member* CancelPairing;
            } Device1;
        } bluez;
    } org;

};


} // namespace ajn


#endif
