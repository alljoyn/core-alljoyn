/**
 * @file
 * BusObject responsible for controlling/handling Bluetooth delegations.
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

#include <alljoyn/BusObject.h>
#include <alljoyn/Message.h>
#include <alljoyn/MessageReceiver.h>

#include "BDAddress.h"
#include "BTNodeDB.h"
#include "Bus.h"
#include "NameTable.h"

#ifndef NDEBUG
#include "BTDebug.h"
#endif


namespace ajn {

typedef qcc::ManagedObj<std::set<BDAddress> > BDAddressSet;

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
     * Start the advertise operation for the given list of names.  A duration
     * may be specified that will result in the advertise operation to
     * automatically stop after the specified number of seconds.  This
     * includes setting the SDP record to contain the information specified in
     * the parameters.
     *
     * @param uuidRev   AllJoyn Bluetooth service UUID revision
     * @param bdAddr    BD address of the connectable node
     * @param psm       The L2CAP PSM number for the AllJoyn service
     * @param adInfo    The complete list of names to advertise and their associated GUIDs
     * @param duration  Find duration in seconds (0 = forever)
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StartAdvertise(uint32_t uuidRev,
                                   const BDAddress& bdAddr,
                                   uint16_t psm,
                                   const BTNodeDB& adInfo,
                                   uint32_t duration = 0) = 0;


    /**
     * Stop the advertise operation.
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StopAdvertise() = 0;

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
                                  const BDAddress& bdAddr,
                                  uint16_t psm,
                                  bool lost) = 0;

    /**
     * Tells the Bluetooth transport to start listening for incoming connections.
     *
     * @param addr[out] BD Address of the adapter listening for connections
     * @param psm[out]  L2CAP PSM allocated
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StartListen(BDAddress& addr,
                                uint16_t& psm) = 0;

    /**
     * Tells the Bluetooth transport to stop listening for incoming connections.
     */
    virtual void StopListen() = 0;

    /**
     * Retrieves the information from the specified device necessary to
     * establish a connection and get the list of advertised names.
     *
     * @param addr          BD address of the device of interest.
     * @param uuidRev[out]  UUID revision number.
     * @param connAddr[out] Bluetooth bus address of the connectable device.
     * @param adInfo[out]   Advertisement information.
     *
     * @return  ER_OK if successful
     */
    virtual QStatus GetDeviceInfo(const BDAddress& addr,
                                  uint32_t& uuidRev,
                                  BTBusAddress& connAddr,
                                  BTNodeDB& adInfo) = 0;

    virtual QStatus Disconnect(const qcc::String& busName) = 0;
    virtual void ReturnEndpoint(RemoteEndpoint& ep) = 0;
    virtual RemoteEndpoint LookupEndpoint(const qcc::String& busName) = 0;

    virtual QStatus IsMaster(const BDAddress& addr, bool& master) const = 0;
    virtual void RequestBTRole(const BDAddress& addr, bt::BluetoothRole role) = 0;

    virtual bool IsEIRCapable() const = 0;
};


/**
 * BusObject responsible for Bluetooth topology management.  This class is
 * used by the Bluetooth transport for the purposes of maintaining a sane
 * topology.
 */
class BTController :
#ifndef NDEBUG
    public debug::BTDebugObjAccess,
#endif
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
     * Send the AdvertiseName signal to the node we believe is the Master node
     * (may actually be a drone node).
     *
     * @param name          Name to add to the list of advertise names.
     *
     * @return ER_OK if successful.
     */
    QStatus AddAdvertiseName(const qcc::String& name);

    /**
     * Send the CancelAdvertiseName signal to the node we believe is the
     * Master node (may actually be a drone node).
     *
     * @param name          Name to remove from the list of advertise names.
     *
     * @return ER_OK if successful.
     */
    QStatus RemoveAdvertiseName(const qcc::String& name);

    /**
     * Send the FindName signal to the node we believe is the Master node (may
     * actually be a drone node).
     *
     * @param name          Name to add to the list of find names.
     *
     * @return ER_OK if successful.
     */
    QStatus AddFindName(const qcc::String& name)
    {
        return DoNameOp(name, *org.alljoyn.Bus.BTController.FindName, true, find);
    }

    /**
     * Send the CancelFindName signal to the node we believe is the Master
     * node (may actually be a drone node).
     *
     * @param findName      Name to remove from the list of find names.
     *
     * @return ER_OK if successful.
     */
    QStatus RemoveFindName(const qcc::String& name);

    /**
     * Process the found or lost device or pass it up the chain to the master node if
     * we are not the master.
     *
     * @param bdAddr        BD Address from the SDP record.
     * @param uuidRev       UUID revsision of the found bus.
     * @param eirCapable    - true if found device was confirmed AllJoyn capable via EIR
     *                      - false if found device is potential AllJoyn capable
     *
     * @return ER_OK if successful.
     */
    void ProcessDeviceChange(const BDAddress& adBdAddr,
                             uint32_t uuidRev,
                             bool eirCapable);

    /**
     * Test whether it is ok to make outgoing connections or accept incoming
     * connections.
     *
     * @return  true if OK to make or accept a connection; false otherwise.
     */
    bool OKToConnect() const { return IsMaster() && (directMinions < maxConnections); }

    /**
     * Perform preparations for an outgoing connection.  This turns off
     * discovery and discoverability when there are no other Bluetooth AllJoyn
     * connections.  It also looks up the real connect address for a device
     * given the device's address.
     *
     * @param addr          Connect address for the device.
     * @param redirection   Bus Address spec if we were told to redirect.
     *
     * @return  The actual address to use to create the connection.
     */
    BTNodeInfo PrepConnect(const BTBusAddress& addr, const qcc::String& redirection);

    /**
     * Perform operations necessary based on the result of connect operation.
     * For now, this just restores the local discovery and discoverability
     * when the connect operation failed and there are no other Bluetooth
     * AllJoyn connections.
     *
     * @param status        Status result of creating the connection.
     * @param addr          Bus address of device connected to.
     * @param remoteName    Unique bus name of the AllJoyn daemon on the other side (only if status == ER_OK)
     */
    void PostConnect(QStatus status, BTNodeInfo& node, const qcc::String& remoteName);

    void LostLastConnection(const BTNodeInfo& node);

    /**
     * Function for the BT Transport to inform a change in the
     * power/availablity of the Bluetooth device.
     *
     * @param on    true if BT device is powered on and available, false otherwise.
     */
    void BTDeviceAvailable(bool on);

    /**
     * Check if it is OK to accept the incoming connection from the specified
     * address.
     *
     * @param addr          BT device address to check
     * @param redirectAddr  [OUT] BT bus address to redirect the connection to if the return value is true.
     *
     * @return  true if OK to accept the connection, false otherwise.
     */
    bool CheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const;

    /**
     * Get the "best" listen spec for a given set of session options.
     *
     * @return listenSpec (busAddr) to use for given session options (empty string if
     *         session opts are incompatible with this transport.
     */
    qcc::String GetListenAddress()
    {
        return self->IsValid() ? self->GetBusAddress().ToSpec() : "";
    }

    void NameOwnerChanged(const qcc::String& alias,
                          const qcc::String* oldOwner,
                          const qcc::String* newOwner);


    /**
     * Accept or reject an incoming JoinSession request. The session does not exist until this
     * after this function returns.
     *
     * This callback is only used by session creators. Therefore it is only called on listeners
     * passed to BusAttachment::BindSessionPort.
     *
     * @param sessionPort    Session port that was joined.
     * @param joiner         Unique name of potential joiner.
     * @param opts           Session options requested by the joiner.
     * @return   Return true if JoinSession request is accepted. false if rejected.
     */
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts);

    /**
     * Called by the bus when a session has been successfully joined. The session is now fully up.
     *
     * This callback is only used by session creators. Therefore it is only called on listeners
     * passed to BusAttachment::BindSessionPort.
     *
     * @param sessionPort    Session port that was joined.
     * @param id             Id of session.
     * @param joiner         Unique name of the joiner.
     */
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner);

    /**
     * Called by the bus when an existing session becomes disconnected.
     *
     * @param sessionId     Id of session that was lost.
     */
    void SessionLost(SessionId id, SessionLostReason reason);

    /**
     * Called when JoinSessionAsync() completes.
     *
     * @param status       ER_OK if successful
     * @param sessionId    Unique identifier for session.
     * @param opts         Session options.
     * @param context      User defined context which will be passed as-is to callback.
     */
    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context);


  private:
    static const uint32_t DELEGATE_TIME = 30;   /**< Delegate ad/find operations to minion for 30 seconds. */

    struct NameArgInfo : public AlarmListener {
        class _NameArgs {
          public:
            MsgArg* args;
            const size_t argsSize;
            _NameArgs(size_t size) : argsSize(size) { args = new MsgArg[size]; }
            ~_NameArgs() { delete[] args; }
          private:
            _NameArgs() : argsSize(0) { }
            _NameArgs(const _NameArgs& other) : argsSize(0) { }
            _NameArgs& operator=(const _NameArgs& other) { return *this; }
        };
        typedef qcc::ManagedObj<_NameArgs> NameArgs;

        BTController& bto;
        BTNodeInfo minion;
        NameArgs args;
        const size_t argsSize;
        const InterfaceDescription::Member* delegateSignal;
        qcc::Alarm alarm;
        bool active;
        bool dirty;
        size_t count;
        NameArgInfo(BTController& bto, size_t size) :
            bto(bto),
            args(size),
            argsSize(size),
            active(false),
            dirty(false),
            count(0)
        {
            minion = bto.self;
        }
        virtual ~NameArgInfo() { }
        virtual void SetArgs() = 0;
        virtual void ClearArgs() = 0;
        virtual void AddName(const qcc::String& name, BTNodeInfo& node) = 0;
        virtual void RemoveName(const qcc::String& name, BTNodeInfo& node) = 0;
        size_t Empty() const { return count == 0; }
        bool Changed() const { return dirty; }
        void AddName(const qcc::String& name)
        {
            AddName(name, bto.self);
        }
        void RemoveName(const qcc::String& name)
        {
            RemoveName(name, bto.self);
        }
        void StartAlarm()
        {
            assert(!bto.dispatcher.HasAlarm(alarm));
            uint32_t delay = BTController::DELEGATE_TIME * 1000;
            alarm = qcc::Alarm(delay, this, this);
            bto.dispatcher.AddAlarm(alarm);
        }
        void StopAlarm() { bto.dispatcher.RemoveAlarm(alarm); }
        virtual bool UseLocal() = 0;
        void StartOp();
        void RestartOp() { assert(active); StopOp(true); StartOp(); }
        void StopOp(bool immediate);
        virtual QStatus StartLocal() = 0;
        virtual QStatus StopLocal(bool immediate) = 0;
        QStatus SendDelegateSignal();

      private:
        void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);
    };

    struct AdvertiseNameArgInfo : public NameArgInfo {
        std::vector<MsgArg> adInfoArgs;
        AdvertiseNameArgInfo(BTController& bto);
        void AddName(const qcc::String& name, BTNodeInfo& node);
        void RemoveName(const qcc::String& name, BTNodeInfo& node);
        void SetArgs();
        void ClearArgs();
        bool UseLocal() { return bto.UseLocalAdvertise(); }
        QStatus StartLocal();
        QStatus StopLocal(bool immediate = true);
    };

    struct FindNameArgInfo : public NameArgInfo {
        qcc::String resultDest;
        BDAddressSet ignoreAddrs;
        std::vector<uint64_t> ignoreAddrsCache;
        FindNameArgInfo(BTController& bto);
        void AddName(const qcc::String& name, BTNodeInfo& node);
        void RemoveName(const qcc::String& name, BTNodeInfo& node);
        void SetArgs();
        void ClearArgs();
        bool UseLocal() { return bto.UseLocalFind(); }
        QStatus StartLocal();
        QStatus StopLocal(bool immediate = true);
    };


    struct DispatchInfo {
        typedef enum {
            UPDATE_DELEGATIONS,
            EXPIRE_CACHED_NODES,
            NAME_LOST,
            BT_DEVICE_AVAILABLE,
            SEND_SET_STATE,
            PROCESS_SET_STATE_REPLY,
            HANDLE_DELEGATE_FIND,
            HANDLE_DELEGATE_ADVERTISE,
            EXPIRE_BLACKLISTED_DEVICE
        } DispatchTypes;
        DispatchTypes operation;

        DispatchInfo(DispatchTypes operation) : operation(operation) { }
        virtual ~DispatchInfo() { }
    };

    struct UpdateDelegationsDispatchInfo : public DispatchInfo {
        bool resetMinions;
        UpdateDelegationsDispatchInfo(bool resetMinions = false) :
            DispatchInfo(UPDATE_DELEGATIONS),
            resetMinions(resetMinions)
        { }
    };

    struct ExpireCachedNodesDispatchInfo : public DispatchInfo {
        ExpireCachedNodesDispatchInfo() : DispatchInfo(EXPIRE_CACHED_NODES) { }
    };

    struct NameLostDispatchInfo : public DispatchInfo {
        qcc::String name;
        NameLostDispatchInfo(const qcc::String& name) :
            DispatchInfo(NAME_LOST),
            name(name)
        { }
    };

    struct BTDevAvailDispatchInfo : public DispatchInfo {
        bool on;
        BTDevAvailDispatchInfo(bool on) : DispatchInfo(BT_DEVICE_AVAILABLE), on(on) { }
    };

    struct SendSetStateDispatchInfo : public DispatchInfo {
        SendSetStateDispatchInfo() : DispatchInfo(SEND_SET_STATE) { }
    };

    struct DeferredMessageHandlerDispatchInfo : public DispatchInfo {
        Message msg;
        DeferredMessageHandlerDispatchInfo(DispatchTypes operation, const Message& msg) :
            DispatchInfo(operation),
            msg(msg)
        { }
    };

    struct ProcessSetStateReplyDispatchInfo : public DeferredMessageHandlerDispatchInfo {
        ProxyBusObject* newMaster;
        ProcessSetStateReplyDispatchInfo(const Message& msg,
                                         ProxyBusObject* newMaster) :
            DeferredMessageHandlerDispatchInfo(PROCESS_SET_STATE_REPLY, msg),
            newMaster(newMaster)
        { }
    };

    struct HandleDelegateOpDispatchInfo : public DeferredMessageHandlerDispatchInfo {
        HandleDelegateOpDispatchInfo(const Message& msg, bool findOp) :
            DeferredMessageHandlerDispatchInfo(findOp ? HANDLE_DELEGATE_FIND : HANDLE_DELEGATE_ADVERTISE, msg)
        { }
    };

    struct ExpireBlacklistedDevDispatchInfo : public DispatchInfo {
        BDAddress addr;
        ExpireBlacklistedDevDispatchInfo(BDAddress addr) : DispatchInfo(EXPIRE_BLACKLISTED_DEVICE), addr(addr) { }
    };


    /**
     * Send the one of the following specified signals to the node we believe
     * is the Master node (may actually be a drone node): FindName,
     * CancelFindName, AdvertiseName, CancelAdvertiseName.
     *
     * @param name          Name to remove from the list of advertise names.
     * @param signal        Reference to the signal to be sent.
     * @param add           Flag indicating if adding or removing a name.
     * @param nameArgInfo   Advertise of Find name arg info structure to inform of name change.
     *
     * @return ER_OK if successful.
     */
    QStatus DoNameOp(const qcc::String& findName,
                     const InterfaceDescription::Member& signal,
                     bool add,
                     NameArgInfo& nameArgInfo);

    /**
     * Handle one of the following incoming signals: FindName, CancelFindName,
     * AdvertiseName, CancelAdvertiseName.
     *
     * @param member        Member.
     * @param sourcePath    Object path of signal sender.
     * @param msg           The incoming message.
     */
    void HandleNameSignal(const InterfaceDescription::Member* member,
                          const char* sourcePath,
                          Message& msg);

    /**
     * Handle the incoming SetState method call.
     *
     * @param member    Member.
     * @param msg       The incoming message
     */
    void HandleSetState(const InterfaceDescription::Member* member,
                        Message& msg);

    /**
     * Handle the incoming SetState method reply.
     *
     * @param msg       The incoming message
     * @param context   User-defined context passed to MethodCall and returned upon reply.
     */
    void HandleSetStateReply(Message& msg,
                             void* context);

    /**
     * Handle the incoming DelegateFind and DelegateAdvertise signals.
     *
     * @param member        Member.
     * @param sourcePath    Object path of signal sender.
     * @param msg           The incoming message
     */
    void HandleDelegateOp(const InterfaceDescription::Member* member,
                          const char* sourcePath,
                          Message& msg);

    /**
     * Handle the incoming FoundNames signal.
     *
     * @param member        Member.
     * @param sourcePath    Object path of signal sender.
     * @param msg           The incoming message - "assq":
     *                        - List of advertised names
     *                        - BD Address
     *                        - L2CAP PSM
     */
    void HandleFoundNamesChange(const InterfaceDescription::Member* member,
                                const char* sourcePath,
                                Message& msg);

    /**
     * Handle the incoming FoundDevice signal.
     *
     * @param member        Member.
     * @param sourcePath    Object path of signal sender.
     * @param msg           The incoming message - "su":
     *                        - BD Address
     *                        - UUID Revision number
     */
    void HandleFoundDeviceChange(const InterfaceDescription::Member* member,
                                 const char* sourcePath,
                                 Message& msg);

    /**
     * Handle the incoming ConnectAddrChanged signal.
     *
     * @param member        Member.
     * @param sourcePath    Object path of signal sender.
     * @param msg           The incoming message - "tqtq":
     *                        - Old BD Address
     *                        - Old PSM
     *                        - New BD Address
     *                        - new PSM
     */
    void HandleConnectAddrChanged(const InterfaceDescription::Member* member,
                                  const char* sourcePath,
                                  Message& msg);

    /**
     * Deals with the power/availability change of the Bluetooth device on the
     * BTController dispatch thread.
     *
     * @param on    true if BT device is powered on and available, false otherwise.
     */
    void DeferredBTDeviceAvailable(bool on);

    /**
     * Send the SetState method call to the Master node we are connecting to.
     */
    void DeferredSendSetState();

    void DeferredProcessSetStateReply(Message& reply,
                                      ProxyBusObject* newMaster);

    /**
     * Handle the incoming DelegateFind signal on the BTController dispatch
     * thread.
     *
     * @param msg           The incoming message - "sas":
     *                        - Master node's bus name
     *                        - List of names to find
     *                        - Bluetooth UUID revision to ignore
     */
    void DeferredHandleDelegateFind(Message& msg);

    /**
     * Handle the incoming DelegateAdvertise signal on the BTController
     * dispatch thread.
     *
     * @param msg           The incoming message - "ssqas":
     *                        - Bluetooth UUID
     *                        - BD Address
     *                        - L2CAP PSM
     *                        - List of names to advertise
     */
    void DeferredHandleDelegateAdvertise(Message& msg);

    void DeferredNameLostHander(const qcc::String& name);

    /**
     * Distribute the advertised name changes to all connected nodes.
     *
     * @param newAdInfo     Added advertised names
     * @param oldAdInfo     Removed advertiesd names
     */
    void DistributeAdvertisedNameChanges(const BTNodeDB* newAdInfo,
                                         const BTNodeDB* oldAdInfo);

    /**
     * Send the FoundNames signal to the node interested in one or more of the
     * names on that bus.
     *
     * @param destNode      The minion that should receive the message.
     * @param adInfo        Advertise information to send.
     * @param lost          Set to true if names are lost, false otherwise.
     */
    void SendFoundNamesChange(const BTNodeInfo& destNode,
                              const BTNodeDB& adInfo,
                              bool lost);

    /**
     * Update the internal state information for other nodes based on incoming
     * message args.
     *
     * @param addr              BT bus address of the newly connected minion
     *                          node.
     * @param nodeStateArgs     List of message args containing the remote
     *                          node's state information.
     * @param numNodeStates     Number of node state entries in the message arg
     *                          list.
     * @param foundNodeArgs     List of message args containing the found node
     *                          known by the remote node.
     * @param numFoundNodes     Number of found nodes in the message arg list.
     *
     * @return
     *         - #ER_OK success
     *         - #ER_FAIL failed to import state information
     */
    QStatus ImportState(BTNodeInfo& connectingNode,
                        MsgArg* nodeStateEntries,
                        size_t numNodeStates,
                        MsgArg* foundNodeArgs,
                        size_t numFoundNodes);

    /**
     * Updates the find/advertise name information on the minion assigned to
     * perform the specified name discovery operation.  It will update the
     * delegation for find or advertise based on current activity, whether we
     * are the Master or not, if the name list changed, and if we can
     * participate in more connections.
     *
     * @param nameInfo  Reference to the advertise or find name info struct
     */
    void UpdateDelegations(NameArgInfo& nameInfo);

    /**
     * Extract advertisement information from an array message args into the
     * internal representation.
     *
     * @param entries       Array of MsgArgs all with type struct:
     *                      - Bus GUID associated with advertise names
     *                      - BT device address (part of bus address)
     *                      - L2CAP PSM (part of bus address)
     *                      - Array of bus names advertised by device with
     *                        associated Bus GUID/Address
     * @param size          Number of entries in the array
     * @param adInfo[out]   Advertisement information to be filled
     *
     * @return  ER_OK if advertisment information successfully extracted.
     */
    static QStatus ExtractAdInfo(const MsgArg* entries,
                                 size_t size,
                                 BTNodeDB& adInfo);

    /**
     * Extract node information from an array of message args into the
     * internal representation.
     *
     * @param entries   Array of MsgArgs all with type struct:
     *                  - BT device address of the connect device
     *                  - L2CAP PSM of the connect device
     *                  - UUIDRev of advertised names
     *                  - Array of advertisement information.
     * @param size      Number of entries in the array
     * @param db[out]   BTNodeDB to be updated with information from entries
     *
     * @return  ER_OK if advertisment information successfully extracted.
     */
    QStatus ExtractNodeInfo(const MsgArg* entries,
                            size_t size,
                            BTNodeDB& db);

    /**
     * Convenience function for filling a vector of MsgArgs with the node
     * state information.
     *
     * @param args[out] vector of MsgArgs to fill.
     */
    void FillNodeStateMsgArgs(std::vector<MsgArg>& args) const;

    /**
     * Convenience function for filling a vector of MsgArgs with the set of
     * found nodes.
     *
     * @param args[out] vector of MsgArgs to fill.
     * @param adInfo    source advertisement information
     */
    void FillFoundNodesMsgArgs(std::vector<MsgArg>& args,
                               const BTNodeDB& adInfo);

    void SetSelfAddress(const BTBusAddress& newAddr);

    uint8_t ComputeSlaveFactor() const;

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

    void ResetExpireNameAlarm();
    void RemoveExpireNameAlarm() { dispatcher.RemoveAlarm(expireAlarm); }
    void JoinSessionNodeComplete();


    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

    bool IsMaster() const { return !master; }
    bool IsDrone() const { return (master && (NumMinions() > 0)); }
    bool IsMinion() const { return (master && (NumMinions() == 0)); }

    size_t NumMinions() const { return nodeDB.Size() - 1; }
    size_t NumEIRMinions() const;

    void PickNextDelegate(NameArgInfo& nameOp);

    bool UseLocalFind()
    {
        return (IsMinion() ||
                (!bt.IsEIRCapable() && (NumMinions() == 0)) ||
                (bt.IsEIRCapable() && (NumEIRMinions() == 0)));
    }
    bool UseLocalAdvertise()
    {
        return (IsMinion() ||
                (!bt.IsEIRCapable() && (NumEIRMinions() == 0) && (NumMinions() <= 1)) ||
                (bt.IsEIRCapable() && (NumEIRMinions() <= 1)));
    }
    bool RotateMinions()
    {
        return (IsMaster() &&
                ((NumEIRMinions() > 2) ||
                 ((NumEIRMinions() == 0) && (NumMinions() > 2))));
    }

#ifndef NDEBUG
    void DumpNodeStateTable() const;
    void FlushCachedNames();

    debug::BTDebugObj dbgIface;

    debug::BTDebugObj::BTDebugTimingProperty& discoverTimer;
    debug::BTDebugObj::BTDebugTimingProperty& sdpQueryTimer;
    debug::BTDebugObj::BTDebugTimingProperty& connectTimer;

    uint64_t discoverStartTime;
    uint64_t sdpQueryStartTime;
    std::map<BDAddress, uint64_t> connectStartTimes;
#endif

    BusAttachment& bus;
    BluetoothDeviceInterface& bt;

    ProxyBusObject* master;        // Bus Object we believe is our master
    BTNodeInfo masterNode;
    BTNodeInfo joinSessionNode;

    uint8_t maxConnects;           // Maximum number of direct connections
    uint32_t masterUUIDRev;        // Revision number for AllJoyn Bluetooth UUID
    uint8_t directMinions;         // Number of directly connected minions
    const uint8_t maxConnections;
    bool listening;
    bool devAvailable;

    BTNodeDB foundNodeDB;
    BTNodeDB nodeDB;
    BTNodeInfo self;

    mutable qcc::Mutex lock;

    AdvertiseNameArgInfo advertise;
    FindNameArgInfo find;

    qcc::Timer dispatcher;
    qcc::Alarm expireAlarm;

    BDAddressSet blacklist;

    volatile int32_t incompleteConnections; // Number of outgoing connections that are being setup
    qcc::Event connectCompleted;

    struct {
        struct {
            struct {
                struct {
                    const InterfaceDescription* interface;
                    // Methods
                    const InterfaceDescription::Member* SetState;
                    // Signals
                    const InterfaceDescription::Member* FindName;
                    const InterfaceDescription::Member* CancelFindName;
                    const InterfaceDescription::Member* AdvertiseName;
                    const InterfaceDescription::Member* CancelAdvertiseName;
                    const InterfaceDescription::Member* DelegateAdvertise;
                    const InterfaceDescription::Member* DelegateFind;
                    const InterfaceDescription::Member* FoundNames;
                    const InterfaceDescription::Member* LostNames;
                    const InterfaceDescription::Member* FoundDevice;
                    const InterfaceDescription::Member* ConnectAddrChanged;
                } BTController;
            } Bus;
        } alljoyn;
    } org;
};

}

#endif
