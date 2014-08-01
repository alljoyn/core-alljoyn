/**
 * @file
 * BusObject responsible for controlling/handling Bluetooth delegations.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_BTCONTROLLER_H
#define _ALLJOYN_BTCONTROLLER_H

#include <qcc/platform.h>

#include <list>
#include <map>
#include <set>
#include <vector>

#include <qcc/ManagedObj.h>
#include <qcc/String.h>
#include <qcc/Timer.h>
#include <qcc/BDAddress.h>

#include <alljoyn/BusObject.h>
#include <alljoyn/Message.h>
#include <alljoyn/MessageReceiver.h>

#include "Bus.h"
#include "NameTable.h"

namespace ajn {

typedef qcc::ManagedObj<std::set<qcc::BDAddress> > BDAddressSet;

class BluetoothDeviceInterface {
    friend class BTController;

  public:

    virtual ~BluetoothDeviceInterface() { }

  private:

    /**
     * Start the find operation for AllJoyn capable devices.  A duration may
     * be specified that will result in the find operation to automatically
     * stop after the specified number of seconds.  Exclude any results from
     * any device in the list of ignoreAddrs.
     *
     * @param ignoreAddrs   Set of BD addresses to ignore
     * @param duration      Find duration in seconds (0 = forever)
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StartFind(const BDAddressSet& ignoreAddrs, uint32_t duration = 0) = 0;

    /**
     * Stop the find operation.
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StopFind() = 0;

    /**
     * This provides the Bluetooth transport with the information needed to
     * call AllJoynObj::FoundNames and to generate the connect spec.
     *
     * @param bdAddr    BD address of the connectable node
     * @param guid      Bus GUID of the discovered bus
     * @param names     The advertised names
     * @param psm       L2CAP PSM accepting connections
     * @param lost      Set to true if names are lost, false otherwise
     */
    virtual void FoundNamesChange(const qcc::String& guid,
                                  const std::vector<qcc::String>& names,
                                  const qcc::BDAddress& bdAddr,
                                  uint16_t psm,
                                  bool lost) = 0;

    /**
     * Tells the Bluetooth transport to start listening for incoming connections.
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StartListen() = 0;

    /**
     * Tells the Bluetooth transport to stop listening for incoming connections.
     */
    virtual void StopListen() = 0;

    virtual QStatus Disconnect(const qcc::String& busName) = 0;
    virtual void ReturnEndpoint(RemoteEndpoint& ep) = 0;
    virtual RemoteEndpoint LookupEndpoint(const qcc::String& busName) = 0;
};


/**
 * BusObject responsible for Bluetooth topology management.  This class is
 * used by the Bluetooth transport for the purposes of maintaining a sane
 * topology.
 */
class BTController :
    public BusObject,
    public NameListener,
    public BusAttachment::JoinSessionAsyncCB,
    public SessionPortListener,
    public SessionListener,
    public qcc::AlarmListener {
  public:
    /**
     * Constructor
     *
     * @param bus    Bus to associate with org.freedesktop.DBus message handler.
     * @param bt     Bluetooth transport to interact with.
     */
    BTController(BusAttachment& bus, BluetoothDeviceInterface& bt);

    /**
     * Destructor
     */
    ~BTController();

    /**
     * Called by the message bus when the object has been successfully
     * registered. The object can perform any initialization such as adding
     * match rules at this time.
     */
    void ObjectRegistered();

    /**
     * Initialize and register this DBusObj instance.
     *
     * @return ER_OK if successful.
     */
    QStatus Init();

    /**
     * Function for the BT Transport to inform a change in the
     * power/availablity of the Bluetooth device.
     *
     * @param on    true if BT device is powered on and available, false otherwise.
     */
    void BLEDeviceAvailable(bool on);

    void NameOwnerChanged(const qcc::String& alias,
                          const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                          const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer);


    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context);


  private:
    struct DispatchInfo {
        typedef enum {
            NAME_LOST,
            BT_DEVICE_AVAILABLE,
        } DispatchTypes;
        DispatchTypes operation;

        DispatchInfo(DispatchTypes operation) : operation(operation) { }
        virtual ~DispatchInfo() { }
    };

    struct NameLostDispatchInfo : public DispatchInfo {
        qcc::String name;
        NameLostDispatchInfo(const qcc::String& name) :
            DispatchInfo(NAME_LOST),
            name(name)
        { }
    };

    struct BLEDevAvailDispatchInfo : public DispatchInfo {
        bool on;
        BLEDevAvailDispatchInfo(bool on) : DispatchInfo(BT_DEVICE_AVAILABLE), on(on) { }
    };

    /**
     * Deals with the power/availability change of the Bluetooth device on the
     * BTController dispatch thread.
     *
     * @param on    true if BT device is powered on and available, false otherwise.
     */
    void DeferredBLEDeviceAvailable(bool on);

    void DeferredNameLostHander(const qcc::String& name);

    qcc::Alarm DispatchOperation(DispatchInfo* op, uint32_t delay = 0)
    {
        void* context = (void*)op;
        qcc::Alarm alarm(delay, this, context);
        dispatcher.AddAlarm(alarm);
        return alarm;
    }


    qcc::Alarm DispatchOperation(DispatchInfo* op, uint64_t dispatchTime)
    {
        void* context = (void*)op;
        qcc::Timespec ts(dispatchTime);
        qcc::Alarm alarm(ts, this, context);
        dispatcher.AddAlarm(alarm);
        return alarm;
    }

    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

    BusAttachment& bus;
    BluetoothDeviceInterface& bt;

    uint8_t maxConnects;           // Maximum number of direct connections
    const uint8_t maxConnections;
    bool listening;
    bool devAvailable;

    mutable qcc::Mutex lock;

    qcc::Timer dispatcher;

    BDAddressSet blacklist;

    qcc::Event connectCompleted;

};

}

#endif
