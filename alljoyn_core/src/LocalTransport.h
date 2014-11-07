/**
 * @file
 * LocalTransport is a special type of Transport that is responsible
 * for all communication of all endpoints that terminate at registered
 * BusObjects residing within this Bus instance.
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_LOCALTRANSPORT_H
#define _ALLJOYN_LOCALTRANSPORT_H

#include <qcc/platform.h>

#include <map>

#include <qcc/String.h>
#include <qcc/GUID.h>
#include <qcc/Event.h>
#include <qcc/Mutex.h>
#include <qcc/StringMapKey.h>
#include <qcc/Timer.h>
#include <qcc/Util.h>

#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/Message.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/ProxyBusObject.h>

#include <alljoyn/Status.h>

#include "BusEndpoint.h"
#include "CompressionRules.h"
#include "MethodTable.h"
#include "SignalTable.h"
#include "Transport.h"

#include <qcc/STLContainer.h>

namespace ajn {

class BusAttachment;
class AllJoynPeerObj;

class _LocalEndpoint;

/**
 * Managed object type that wraps a local endpoint
 */
typedef qcc::ManagedObj<_LocalEndpoint> LocalEndpoint;

/**
 * %LocalEndpoint represents an endpoint connection to DBus/AllJoyn server
 */
class _LocalEndpoint : public _BusEndpoint, public qcc::AlarmListener, public MessageReceiver {

    friend class LocalTransport;
    friend class BusObject;

  public:

    /**
     * Default constructor initializes an invalid endpoint. This allows for the declaration of uninitialized LocalEndpoint variables.
     */
    _LocalEndpoint() : dispatcher(NULL), deferredCallbacks(NULL), bus(NULL), replyTimer("replyTimer", true) { }

    /**
     * Constructor
     *
     * @param bus          Bus associated with endpoint.
     * @param concurrency  The maximum number of concurrent method and signal handlers locally executing.
     */
    _LocalEndpoint(BusAttachment& bus, uint32_t concurrency);

    /**
     * Destructor.
     */
    ~_LocalEndpoint();

    /**
     * Get the bus attachment for this endpoint
     */
    BusAttachment& GetBus() { return *bus; }

    /**
     * Start endpoint.
     *
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus Start();

    /**
     * Stop endpoint.
     *
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus Stop();

    /**
     * Although LocalEndpoint is not a thread it contains threads that need to be joined.
     *
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus Join();

    /**
     * Register a handler for method call reply
     *
     * @param receiver       The object that will receive the response
     * @param replyHandler   The reply callback function
     * @param method         Interface/member of method call awaiting this reply.
     * @param methodCallMsg  The method call message
     * @param context        Opaque context pointer passed from method call to it's reply handler.
     * @param timeout        Timeout specified in milliseconds to wait for a reply to a method call.
     *                       The value 0 means use the implementation dependent default timeout.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus RegisterReplyHandler(MessageReceiver* receiver,
                                 MessageReceiver::ReplyHandler replyHandler,
                                 const InterfaceDescription::Member& method,
                                 Message& methodCallMsg,
                                 void* context = NULL,
                                 uint32_t timeout = 0);

    /**
     * Un-register the handler for a specified method call.
     *
     * @param methodCallMsg  The specified method call message
     *
     * @return true if a reply handler for the method call is found and unregistered
     */
    bool UnregisterReplyHandler(Message& methodCallMsg);

    /**
     * Conditionally updates the serial number on a message. This is to ensure that the serial
     * number accurately reflects the order in which messages are queued for delivery. For message
     * that were delayed pending authentication the serial number may be updated by this call.
     *
     * @param msg  The message to update the serial number on.
     */
    void UpdateSerialNumber(Message& msg);

    /**
     * Pause the timeout handler for specified method call. If the reply handler is succesfully
     * paused it must be resumed by calling ResumeReplyHandler later.
     *
     * @param methodCallMsg  The method call message
     *
     * @return  Returns true if the timeout was paused or false if there is no reply
     *          handler registered for the specified message.
     */
    bool PauseReplyHandlerTimeout(Message& methodCallMsg);

    /**
     * Resume the timeout handler for specified method call.
     *
     * @param methodCallMsg  The method call message
     *
     * @return  Returns true if the timeout was resumed or false if there is no reply
     *          handler registered for the specified message.
     */
    bool ResumeReplyHandlerTimeout(Message& methodCallMsg);

    /**
     * Register a signal handler.
     * Signals are forwarded to the signalHandler if sender, interface, member and path
     * qualifiers are ALL met.
     *
     * @param receiver       The object receiving the signal.
     * @param signalHandler  The signal handler method.
     * @param member         Interface/member of signal.
     * @param matchRule      A filter rule.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus RegisterSignalHandler(MessageReceiver* receiver,
                                  MessageReceiver::SignalHandler signalHandler,
                                  const InterfaceDescription::Member* member,
                                  const char* matchRule);

    /**
     * Un-Register a signal handler.
     * Remove the signal handler that was registered with the given parameters.
     *
     * @param receiver       The object receiving the signal.
     * @param signalHandler  The signal handler method.
     * @param member         Interface/member of signal.
     * @param matchRule      A filter rule.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus UnregisterSignalHandler(MessageReceiver* receiver,
                                    MessageReceiver::SignalHandler signalHandler,
                                    const InterfaceDescription::Member* member,
                                    const char* matchRule);

    /**
     * Un-Register all signal and reply handlers registered to the specified MessageReceiver.
     *
     * @param receiver   The object receiving the signal or waiting for the reply.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus UnregisterAllHandlers(MessageReceiver* receiver);

    /**
     * Get the endpoint's unique name.
     *
     * @return   Unique name for endpoint.
     */
    const qcc::String& GetUniqueName() const { return uniqueName; }

    /**
     * Set the endpoint's unique name.
     *
     * @param uniqueName   Unique name for endpoint.
     */
    void SetUniqueName(const qcc::String& uniqueName) { this->uniqueName = uniqueName; }

    /**
     * Register a BusObject.
     *
     * @param obj      BusObject to be registered.
     * @param secure   true if peer authentication is required to access this object.
     * @return
     *      - ER_OK if successful.
     *      - ER_BUS_BAD_OBJ_PATH for a bad object path
     */
    QStatus RegisterBusObject(BusObject& obj, bool secure);

    /**
     * Unregisters an object and its method and signal handlers.
     *
     * @param obj  Object to be unregistered.
     */
    void UnregisterBusObject(BusObject& obj);

    /**
     * Get the Announced Object Description for the BusObjects registered on
     * the BusAttachment with interfaces marked as announced.
     *
     * This will clear any previous contents of the of the MsgArg provided. The
     * resulting MsgArg will have a signature a(oas) and will contain an array
     * of object paths. For each object path an array of announced interfaces found
     * at that object path will be listed.
     *
     * @param[out] aboutObjectDescriptionArg reference to a MsgArg that will
     *             be filled in.
     *
     * @return ER_OK on success
     */
    QStatus GetAnnouncedObjectDescription(MsgArg& objectDescriptionArg);

    /**
     * Find a local object.
     *
     * @param objectPath   Object path.
     * @return
     *      - Pointer to matching object
     *      - NULL if none is found.
     */
    BusObject* FindLocalObject(const char* objectPath);

    /**
     * Notify local endpoint that a bus connection has been made.
     */
    void OnBusConnected();

    /**
     * Notify local endpoint that a bus  had disconnected.
     */
    void OnBusDisconnected();

    /**
     * Get the org.freedesktop.DBus remote object.
     *
     * @return org.freedesktop.DBus remote object
     */
    const ProxyBusObject& GetDBusProxyObj() const { return *dbusObj; }

    /**
     * Get the org.alljoyn.Bus remote object.
     *
     * @return org.alljoyn.Bus remote object
     */
    const ProxyBusObject& GetAllJoynProxyObj() const { return *alljoynObj; }

    /**
     * Get the org.alljoyn.Debug remote object.
     *
     * @return org.alljoyn.Debug remote object
     */
    const ProxyBusObject& GetAllJoynDebugObj();

    /**
     * Get the org.alljoyn.Bus.Peer local object.
     */
    AllJoynPeerObj* GetPeerObj() const { return peerObj; }

    /**
     * Get the guid for the local endpoint
     *
     * @return  The guid for the local endpoint
     */
    const qcc::GUID128& GetGuid() { return guid; }

    /**
     * Return the user id of the endpoint.
     *
     * @return  User ID number.
     */
    uint32_t GetUserId() const { return qcc::GetUid(); }

    /**
     * Return the group id of the endpoint.
     *
     * @return  Group ID number.
     */
    uint32_t GetGroupId() const { return qcc::GetGid(); }

    /**
     * Return the process id of the endpoint.
     *
     * @return  Process ID number.
     */
    uint32_t GetProcessId() const { return qcc::GetPid(); }

    /**
     * Indicates if the endpoint supports reporting UNIX style user, group, and process IDs.
     *
     * @return  'true' if UNIX IDs supported, 'false' if not supported.
     */
    bool SupportsUnixIDs() const { return true; }

    /**
     * Send message to this endpoint.
     *
     * @param msg        Message to deliver to this endpoint.
     *
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus PushMessage(Message& msg);

    /**
     * Indicate whether this endpoint is allowed to receive messages from remote devices.
     * LocalEndpoints always allow remote messages.
     *
     * @return true
     */
    bool AllowRemoteMessages() { return true; }

    /**
     * Set reentrancy on the dispatcher
     */
    void EnableReentrancy();

    /**
     * Check the calling thread is making an illegal reentrant call.
     */
    bool IsReentrantCall();

  private:

    /**
     * Signal/Method dispatcher
     */
    class Dispatcher;
    Dispatcher* dispatcher;

    /**
     * Performs operations that were deferred until the bus is connected such
     * as object registration callbacks
     */
    class DeferredCallbacks;
    DeferredCallbacks* deferredCallbacks;

    /**
     * PushMessage worker.
     */
    QStatus DoPushMessage(Message& msg);

    /**
     * Assignment operator is private - LocalEndpoints cannot be assigned.
     */
    _LocalEndpoint& operator=(const _LocalEndpoint& other);

    /**
     * Copy constructor is private - LocalEndpoints cannot be copied.
     */
    _LocalEndpoint(const _LocalEndpoint& other);

    /**
     * Type definition for a method call reply context
     */
    class ReplyContext;

    /**
     * Equality function for matching object paths
     */
    struct PathEq { bool operator()(const char* p1, const char* p2) const { return (p1 == p2) || (strcmp(p1, p2) == 0); } };

    /**
     * Remove a reply handler from the reply handler list.
     *
     * @param serial       The serial number expected in the reply
     *
     * @return A pointer to the reply context if it was removed.
     */
    ReplyContext* RemoveReplyHandler(uint32_t serial);

    /**
     * Hash functor
     */
    struct Hash {
        inline size_t operator()(const char* s) const {
            return qcc::hash_string(s);
        }
    };

    /**
     * Registered LocalObjects
     */
    std::unordered_map<const char*, BusObject*, Hash, PathEq> localObjects;

    /**
     * List of contexts for method call replies.
     */
    std::map<uint32_t, ReplyContext*> replyMap;

    bool running;                      /**< Is the local endpoint up and running */
    bool isRegistered;                 /**< true iff endpoint has been registered with router */
    MethodTable methodTable;           /**< Hash table of BusObject methods */
    SignalTable signalTable;           /**< Hash table of BusObject signal handlers */
    BusAttachment* bus;                /**< Message bus */
    qcc::Mutex objectsLock;            /**< Mutex protecting Objects hash table */
    qcc::Mutex replyMapLock;           /**< Mutex protecting reply contexts */
    qcc::GUID128 guid;                 /**< GUID to uniquely identify a local endpoint */
    qcc::String uniqueName;            /**< Unique name for endpoint */
    qcc::Timer replyTimer;             /**< Timer used to timeout method calls */

    std::vector<BusObject*> defaultObjects;  /**< Auto-generated, heap allocated parent objects */

    /**
     * Remote object for the standard DBus object and its interfaces
     */
    ProxyBusObject* dbusObj;

    /**
     * Remote object for the AllJoyn object and its interfaces
     */
    ProxyBusObject* alljoynObj;

    /**
     * Remote object for the AllJoyn debug object and its interfaces
     */
    ProxyBusObject* alljoynDebugObj;

    /**
     * The local AllJoyn peer object that implements AllJoyn endpoint functionality
     */
    AllJoynPeerObj* peerObj;

    /** Helper to diagnose misses in the methodTable */
    QStatus Diagnose(Message& msg);

    /** Special-cased message handler for the Peer interface */
    QStatus PeerInterface(Message& msg);

    /**
     * Process an incoming SIGNAL message
     */
    QStatus HandleSignal(Message& msg);

    /**
     * Process an incoming METHOD_CALL message
     */
    QStatus HandleMethodCall(Message& msg);

    /**
     * Process an incoming METHOD_REPLY or ERROR message
     */
    QStatus HandleMethodReply(Message& msg);

    /**
     *   Process a timeout on a METHOD_REPLY message
     */
    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

    /**
     * Inner utility method used bo RegisterBusObject.
     * Do not call this method externally.
     */
    QStatus DoRegisterBusObject(BusObject& object, BusObject* parent, bool isPlaceholder);

};

/**
 * %LocalTransport is a special type of Transport that is responsible
 * for all communication of all endpoints that terminate at registered
 * AllJoynObjects residing within this Bus instance.
 */
class LocalTransport : public Transport {
    friend class BusAttachment;

  public:

    /**
     *  Constructor
     *
     * @param bus               The bus
     * @param concurrency       The maximum number of concurrent method and signal handlers locally executing.
     *
     */
    LocalTransport(BusAttachment& bus, uint32_t concurrency) : localEndpoint(bus, concurrency), isStoppedEvent() { isStoppedEvent.SetEvent(); }

    /**
     * Destructor
     */
    ~LocalTransport();

    /**
     * Normalize a transport specification.
     * Given a transport specification, convert it into a form which is guaranteed to have a one-to-one
     * relationship with a transport.
     *
     * @param inSpec    Input transport connect spec.
     * @param outSpec   Output transport connect spec.
     * @param argMap    Parsed parameter map.
     * @return ER_OK if successful.
     */
    QStatus NormalizeTransportSpec(const char* inSpec,
                                   qcc::String& outSpec,
                                   std::map<qcc::String, qcc::String>& argMap) const { return ER_NOT_IMPLEMENTED; }

    /**
     * Start the transport and associate it with a router.
     *
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus Start();

    /**
     * Stop the transport.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus Stop(void);

    /**
     * Pend caller until transport is stopped.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus Join(void);

    /**
     * Determine if this transport is running. Running means Start() has been called.
     *
     * @return  Returns true if the transport is running.
     */
    bool IsRunning();

    /**
     * Connect a local endpoint. (Not used for local transports)
     *
     * @param connectSpec     Unused parameter.
     * @param opts            Requested sessions opts.
     * @param newep           [OUT] Endpoint created as a result of successful connect.
     * @return  ER_NOT_IMPLEMENTED.
     */
    QStatus Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep) { return ER_NOT_IMPLEMENTED; }

    /**
     * Disconnect a local endpoint. (Not used for local transports)
     *
     * @param args   Connect args used to create connection that is now being disconnected.
     * @return  ER_NOT_IMPLEMENTED.
     */
    QStatus Disconnect(const char* args) { return ER_NOT_IMPLEMENTED; }

    /**
     * Start listening for incomming connections  (Not used for local transports)
     *
     * @param listenSpec      Unused parameter.
     * @return  ER_NOT_IMPLEMENTED.
     */
    QStatus StartListen(const char* listenSpec) { return ER_NOT_IMPLEMENTED; }

    /**
     * Stop listening for incoming connections.  (Not used for local transports)
     *
     * @param listenSpec      Unused parameter.
     * @return  ER_NOT_IMPLEMENTED.
     */
    QStatus StopListen(const char* listenSpec) { return ER_NOT_IMPLEMENTED; }

    /**
     * Register a BusLocalObject.
     *
     * @param obj       BusLocalObject to be registered.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus RegisterBusObject(BusObject& obj)
    {
        /*
         * Note: bus local objects do not require peer authentication
         */
        return localEndpoint->RegisterBusObject(obj, false);
    }

    /**
     * Unregisters an object and its method and signal handlers.
     *
     * @param object   Object to be deregistred.
     */
    void UnregisterBusObject(BusObject& object)
    {
        return localEndpoint->UnregisterBusObject(object);
    }

    /**
     * Return the singleton local endpoint
     *
     * @return  Local endpoint
     */
    LocalEndpoint GetLocalEndpoint() { return localEndpoint; }

    /**
     * Set a listener for transport related events.
     * There can only be one listener set at a time. Setting a listener
     * implicitly removes any previously set listener.
     *
     * @param listener  Listener for transport related events.
     */
    void SetListener(TransportListener* listener) { };

    /**
     * Start discovering busses.
     */
    void EnableDiscovery(const char* namePrefix, TransportMask transports) { }

    /**
     * Stop discovering busses to connect to
     */
    void DisableDiscovery(const char* namePrefix, TransportMask transports) { }

    /**
     * Start advertising a well-known name with a given quality of service.
     *
     * @param advertiseName   Well-known name to add to list of advertised names.
     */
    QStatus EnableAdvertisement(const qcc::String& advertiseName, bool quietly, TransportMask transports) { return ER_FAIL; }

    /**
     * Stop advertising a well-known name with a given quality of service.
     *
     * @param advertiseName   Well-known name to remove from list of advertised names.
     * @param nameListEmpty   Indicates whether advertise name list is completely empty (safe to disable OTA advertising).
     */
    void DisableAdvertisement(const qcc::String& advertiseName, TransportMask transports) { }

    /**
     * Returns the name of this transport
     */
    const char* GetTransportName() const { return "local"; }

    /**
     * Get the transport mask for this transport
     *
     * @return the TransportMask for this transport.
     */
    TransportMask GetTransportMask() const { return TRANSPORT_LOCAL; }

    /**
     * Get a list of the possible listen specs for a given set of session options.
     * @param[IN]    opts      Session options.
     * @param[OUT]   busAddrs  Set of listen addresses. Always empty for this transport.
     * @return ER_OK if successful.
     */
    QStatus GetListenAddresses(const SessionOpts& opts, std::vector<qcc::String>& busAddrs) const { return ER_OK; }

    /**
     * Does this transport support connections as described by the provided
     * session options.
     *
     * @param opts  Proposed session options.
     * @return
     *      - true if the SessionOpts specifies a supported option set.
     *      - false otherwise.
     */
    bool SupportsOptions(const SessionOpts& opts) const
    {
        bool rc = true;
        /* Supports any traffic so long as it is reliable */
        if (opts.traffic != SessionOpts::TRAFFIC_MESSAGES && opts.traffic != SessionOpts::TRAFFIC_RAW_RELIABLE) {
            rc = false;
        }
        return rc;
    }

    /**
     * Indicates whether this transport is used for client-to-bus or bus-to-bus connections.
     *
     * @return  Always returns false, the LocalTransport belongs to the local application.
     */
    bool IsBusToBus() const { return false; }

  private:

    /**
     * Singleton endpoint for LocalTransport
     */
    LocalEndpoint localEndpoint;

    /**
     * Set when transport is stopped
     */
    qcc::Event isStoppedEvent;
};

}

#endif
