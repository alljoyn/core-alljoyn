#ifndef _ALLJOYN_INTERNAL_H
#define _ALLJOYN_INTERNAL_H
/**
 * @file
 *
 * This file defines internal state for a BusAttachment
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#ifndef __cplusplus
#error Only include BusInternal.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Event.h>
#include <qcc/atomic.h>
#include <qcc/ManagedObj.h>
#include <qcc/IODispatch.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/PermissionConfigurator.h>

#include "AuthManager.h"
#include "ObserverManager.h"
#include "ClientRouter.h"
#include "KeyStore.h"
#include "PeerState.h"
#include "Transport.h"
#include "TransportList.h"
#include "PermissionManager.h"

#include <alljoyn/Status.h>
#include <set>

namespace ajn {

class BusAttachment::Internal : public MessageReceiver, public JoinSessionAsyncCB {
    friend class BusAttachment;
    friend class BusObject;

    struct Session;

  public:
    /**
     * Get a reference to the internal store object.
     *
     * @return A reference to the bus's key store.
     */
    KeyStore& GetKeyStore() { return keyStore; }

    /**
     * Return the next available serial number. Note 0 is an invalid serial number.
     *
     * @return   The next available serial number.
     */
    uint32_t NextSerial() {
        uint32_t sn = (uint32_t) qcc::IncrementAndFetch(&msgSerial);
        return sn ? sn : NextSerial();
    }

    /**
     * Return most recently allocated serial number
     */
    uint32_t PrevSerial() { return msgSerial ? msgSerial : -1; }

    /**
     * Get a reference to the authentication manager object.
     *
     * @return A pointer to the bus's authentication manager
     */
    AuthManager& GetAuthManager() { return authManager; }

    /**
     * Get a reference to the observer manager object.
     *
     * @return A pointer to the bus's observer manager
     */
    ObserverManager& GetObserverManager() {
        if (!observerManager) {
            observerManager = new ObserverManager(bus);
            observerManager->Start();
        }
        return *observerManager;
    }

    /**
     * Get a reference to the internal transport list.
     *
     * @return A reference to the bus's transport list.
     */
    TransportList& GetTransportList() { return transportList; }

    /**
     * Get a pointer to the internal peer state table.
     *
     * @return  The peer state table.
     */
    PeerStateTable* GetPeerStateTable() { return &peerStateTable; }

    /**
     * Get the global GUID for this bus.
     *
     * @return Global GUID
     */
    const qcc::GUID128& GetGlobalGUID(void) const { return globalGuid; }

    /**
     * Return the local endpoint for this bus.
     *
     * @return  Returns the local endpoint.
     */
    LocalEndpoint& GetLocalEndpoint() { return localEndpoint; }

    /**
     * Get the router.
     *
     * @return  The router
     */
    Router& GetRouter(void) { return *router; }

    /**
     * Get the router.
     *
     * @return  The router
     */
    const Router& GetRouter(void) const { return *router; }

    /**
     * Get the iodispatch.
     *
     * @return  The iodispatch
     */
    qcc::IODispatch& GetIODispatch(void) { return m_ioDispatch; }

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
     * @return ER_OK on success
     */
    QStatus GetAnnouncedObjectDescription(MsgArg& objectDescriptionArg);

    /**
     * Constructor called by BusAttachment.
     */
    Internal(const char* appName,
             BusAttachment& bus,
             TransportFactoryContainer& factories,
             Router* router,
             bool allowRemoteMessages,
             const char* listenAddresses,
             uint32_t concurrency);

    /*
     * Destructor also called by BusAttachment
     */
    virtual ~Internal();

    /**
     * Filter out authentication mechanisms not present in the list.
     *
     * @param list  The list of authentication mechanisms to filter on.
     */
    size_t FilterAuthMechanisms(const qcc::String& list) { return authManager.FilterMechanisms(list); }

    /**
     * A generic signal handler for AllJoyn signals
     */
    void AllJoynSignalHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& message);

    /**
     * Indicate whether endpoints of this attachment are allowed to receive messages
     * from remote devices.
     */
    bool AllowRemoteMessages() const { return allowRemoteMessages; }

    /**
     * Get the bus addresses that this daemon uses to listen on.
     * For clients, this list is empty since clients dont listen.
     *
     * @return The bus addresses that this bus instance uses to listen on.
     */
    const qcc::String& GetListenAddresses() const { return listenAddresses; }

    /**
     * Inform PermissionConfigurationListener of incoming request to reset
     * application state.
     *
     * @return  ER_OK if successful.
     */
    QStatus CallFactoryResetCallback();

    /**
     * Inform PermissionConfigurationListener of a security policy change.
     */
    void CallPolicyChangedCallback();

    /**
     * Set the PermissionConfigurationListener.
     *
     * @param listener   PermissionConfigurationListener to use.
     * @return  ER_OK if successful.
     */
    QStatus SetPermissionConfigurationListener(PermissionConfigurationListener* listener);

    /**
     * Inform BusListeners of incoming JoinSession attempt.
     *
     * @param sessionPort    SessionPort specified in join request.
     * @param joiner         Unique name of potential joiner.
     * @param opts           Session options requested by joiner.
     * @return   Return true if JoinSession request is accepted. false if rejected.
     */
    bool CallAcceptListeners(SessionPort sessionPort, const char* joiner, const SessionOpts& opts);

    /**
     * Inform BusListeners of a successful JoinSession.
     *
     * @param sessionPort    SessionPort specified by joiner.
     * @param id             Session id.
     * @param joiner         Unique name of sucessful joiner.
     */
    void CallJoinedListeners(SessionPort sessionPort, SessionId id, const char* joiner);

    /**
     * Set the SessionListener for an existing session id.
     *
     * @param sessionId  Existing session Id.
     * @param listener   SessionListener to associate with sessionId.
     * @return  ER_OK if successful.
     */
    QStatus SetSessionListener(SessionId id, SessionListener* listener, SessionSideMask bitset);

    /**
     * Check whether a session with a particular sessionId exists for host/joiner side
     *
     * @param id  Existing session Id.
     * @param index      host/joiner
     * @return  true if session exists
     */
    bool SessionExists(SessionId id, size_t index) const;

    /**
     * Check whether a session with a particular sessionId exists has been selfjoined
     *
     * @param sessionId  Existing session Id.
     *
     * @return  true if session exists and session was selfjoined on this leaf
     */
    bool IsSelfJoin(SessionId id) const;

    /**
     * Called if the bus attachment become disconnected from the bus.
     */
    void NonLocalEndpointDisconnected();

    /**
     * JoinSession implementation.
     */
    QStatus JoinSession(const char* sessionHost, SessionPort sessionPort, SessionListener* listener,
                        SessionId& sessionId, SessionOpts& opts);

    /**
     * JoinSessionAsync method_reply handler.
     */
    void JoinSessionAsyncCB(Message& message, void* context);

    /**
     * LeaveSessionAsync method_reply handler.
     */
    void LeaveSessionAsyncCB(Message& message, void* context);

    /**
     * SetLinkTimeoutAsync method_reply handler.
     */
    void SetLinkTimeoutAsyncCB(Message& message, void* context);

    /**
     * AddMatchAsync method_reply handler
     */
    void AddMatchAsyncCB(Message& message, void* context);

    /**
     * RemoveMatchAsync method_reply handler
     */
    void RemoveMatchAsyncCB(Message& message, void* context);

    /**
     * PingAsync method_reply handler
     */
    void PingAsyncCB(Message& message, void* context);

    /**
     * Get a reference to the internal permission manager
     *
     * @return A reference to the bus's permmision manager
     */
    PermissionManager& GetPermissionManager() { return permissionManager; }

    /**
     * Get a reference to the permission configurator
     *
     * @return A reference to the bus's permmision configurator
     */
    PermissionConfigurator& GetPermissionConfigurator() { return permissionConfigurator; }

    /**
     * GetNameOwnerAsync method_reply handler
     */
    void GetNameOwnerAsyncCB(Message& message, void* context);

    /**
     * Push a message into the local endpoint
     *
     * @param msg  The message to push
     * @return  ER_OK if successful.
     */
    QStatus PushToLocalEndpoint(Message& msg) {
        BusEndpoint busEndpoint = BusEndpoint::cast(localEndpoint);
        return router->PushMessage(msg, busEndpoint);
    }

    /**
     * Find out if the BusAttachment has bound the specified SessionPort
     *
     * @param sessionPort port number being checked
     *
     * @return true if the sessionPort is bound
     */
    bool IsSessionPortBound(SessionPort sessionPort);

    /**
     * Return all hosted sessions
     *
     * @return set with all hosted session ids
     */
    std::vector<Session> GetHostedSessions() const {
        sessionsLock[SESSION_SIDE_HOST].Lock(MUTEX_CONTEXT);
        std::vector<Session> copy;
        for (SessionMap::const_iterator it = sessions[SESSION_SIDE_HOST].begin(); it != sessions[SESSION_SIDE_HOST].end(); ++it) {
            copy.push_back(it->second);
        }
        sessionsLock[SESSION_SIDE_HOST].Unlock(MUTEX_CONTEXT);
        return copy;
    }

    /**
     * Get a particular session description
     *
     * @param[in]  sessionId the session id
     * @param[out] session_out   a reference to a Section struct to be filled in
     *
     * @return ER_OK if the session was found and the Session struct is filled in
     *         ER_FAIL otherwise
     */
    QStatus GetSession(SessionId sessionId, Session& session_out) const {
        bool found = false;
        for (size_t side = 0; !found && side < ArraySize(sessions); ++side) {
            sessionsLock[side].Lock(MUTEX_CONTEXT);
            SessionMap::const_iterator it = sessions[side].find(sessionId);
            if (it != sessions[side].end()) {
                found = true;
                session_out = it->second;
            }
            sessionsLock[side].Unlock(MUTEX_CONTEXT);
        }
        return found ? ER_OK : ER_FAIL;
    }

    /**
     * Retrieve bus names for all peers that are in session (either hosted
     * or joined) with this BusAttachment.
     *
     * @param[out] busNames a reference to a set<string> to be filled in
     */
    void GetConnectedPeers(std::set<std::string>& busNames);

    /**
     * Indicate whether bus is currently connected.
     *
     * Messages can only be sent or received when the bus is connected.
     *
     * @return true if the bus is connected.
     */
    bool IsConnected() const;

    /**
     * Start all the transports.
     *
     * @return
     *         - ER_OK if successful.
     */
    virtual QStatus TransportsStart() { return transportList.Start(GetListenAddresses()); }

    /**
     * Stop all the transports.
     *
     * @return
     *         - ER_OK if successful.
     *         - an error status otherwise.
     */
    virtual QStatus TransportsStop() { return transportList.Stop(); }

    /**
     * Wait for all transports to stop.
     *
     * @return
     *         - ER_OK if successful.
     *         - an error status otherwise.
     */
    virtual QStatus TransportsJoin() { return transportList.Join(); }

    /**
     * Connect to an AllJoyn router at a specific connectSpec destination.
     *
     * If there is no router present at the given connectSpec or if the router
     * at the connectSpec has an incompatible AllJoyn version, this method will
     * attempt to use a bundled router if one exists.
     *
     * @param[in] requestedConnectSpec The transport connection spec to try.
     * @param[out] actualConnectSpec The connected transport spec if successful.
     *
     * @return
     *     - #ER_OK if successful.
     *     - An error status otherwise
     */
    virtual QStatus TransportConnect(const char* requestedConnectSpec, qcc::String& actualConnectSpec);

    /**
     * Disconnect a remote bus address connection.
     *
     * @param[in] disconnectConnectSpec The transport connection spec used to connect.
     *
     * @return
     *     - #ER_OK if successful
     *     - #ER_BUS_BUS_NOT_STARTED if the bus is not started
     *     - #ER_BUS_NOT_CONNECTED if the %BusAttachment is not connected to the bus
     *     - Other error status codes indicating a failure
     */
    virtual QStatus TransportDisconnect(const char* disconnectConnectSpec);

    /** @copydoc _LocalEndpoint::RegisterSignalHandler() */
    virtual QStatus RegisterSignalHandler(MessageReceiver* receiver,
                                          MessageReceiver::SignalHandler signalHandler,
                                          const InterfaceDescription::Member* member,
                                          const char* matchRule) {
        return localEndpoint->RegisterSignalHandler(receiver, signalHandler, member, matchRule);
    }

    /** @copydoc _LocalEndpoint::UnregisterSignalHandler() */
    virtual QStatus UnregisterSignalHandler(MessageReceiver* receiver,
                                            MessageReceiver::SignalHandler signalHandler,
                                            const InterfaceDescription::Member* member,
                                            const char* matchRule) {
        return localEndpoint->UnregisterSignalHandler(receiver, signalHandler, member, matchRule);
    }

    /** @copydoc _LocalEndpoint::GetDBusProxyObj() */
    virtual const ProxyBusObject& GetDBusProxyObj() const { return localEndpoint->GetDBusProxyObj(); }

  private:

    static void Init();
    static void Shutdown();
    friend class StaticGlobals;

    /**
     * Copy constructor.
     * Internal may not be copy constructed.
     *
     * @param other   sink being copied.
     */
    Internal(const BusAttachment::Internal& other);

    /**
     * Assignment operator.
     * Internal may not be assigned.
     *
     * @param other   RHS of assignment.
     */
    Internal& operator=(const BusAttachment::Internal& other);

    /**
     * @internal
     * JoinSessionAsync callback used by JoinSession
     */
    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context);

    /**
     * @internal
     * Connect to an AllJoyn router at a specific connectSpec destination.
     *
     * @param[in] requestedConnectSpec The transport connection spec to try.
     *
     * @return
     *     - #ER_OK if successful.
     *     - An error status otherwise
     */
    QStatus TransportConnect(const char* requestedConnectSpec);

    qcc::String application;              /* Name of the that owns the BusAttachment application */
    BusAttachment& bus;                   /* Reference back to the bus attachment that owns this state */


    qcc::Mutex listenersLock;             /* Mutex that protects BusListeners container (set) */
    typedef qcc::ManagedObj<BusListener*> ProtectedBusListener;
    typedef std::set<ProtectedBusListener> ListenerSet;
    ListenerSet listeners;               /* List of registered BusListeners */
    qcc::IODispatch m_ioDispatch;         /* iodispatch for this bus */
    std::map<qcc::StringMapKey, InterfaceDescription> ifaceDescriptions;
    TransportList transportList;          /* List of active transports */
    KeyStore keyStore;                    /* The key store for the bus attachment */
    AuthManager authManager;              /* The authentication manager for the bus attachment */
    qcc::GUID128 globalGuid;              /* Global GUID for this BusAttachment */
    volatile int32_t msgSerial;           /* Serial number is updated for every message sent by this bus */
    Router* router;                       /* Message bus router */
    PeerStateTable peerStateTable;        /* Table that maintains state information about remote peers */
    LocalEndpoint localEndpoint;          /* The local endpoint */

    bool allowRemoteMessages;             /* true iff endpoints of this attachment can receive messages from remote devices */
    qcc::String listenAddresses;          /* The set of bus addresses that this bus can listen on. (empty for clients) */
    qcc::Mutex stopLock;                  /* Protects BusAttachement::Stop from being reentered */
    volatile int32_t stopCount;           /* Number of caller's blocked in BusAttachment::Stop() */

    typedef qcc::ManagedObj<SessionPortListener*> ProtectedSessionPortListener;
    typedef std::map<SessionPort, ProtectedSessionPortListener> SessionPortListenerMap;
    SessionPortListenerMap sessionPortListeners;  /* Lookup SessionPortListener by session port */
    qcc::Mutex sessionPortListenersLock;       /* Lock protecting sessionPortListeners map */

    typedef qcc::ManagedObj<SessionListener*> ProtectedSessionListener;
    struct Session {
        bool host;
        bool multipoint;
        SessionId id;
        ProtectedSessionListener listener;
        std::set<std::string> otherParticipants;
    };

    typedef std::map<SessionId, Session> SessionMap;
    SessionMap sessions[SESSION_SIDE_NUM]; /* look up session description by session id (index 0 for hosted sessions, index 1 for joined sessions) */
    mutable qcc::Mutex sessionsLock[SESSION_SIDE_NUM];

    typedef qcc::ManagedObj<AboutListener*> ProtectedAboutListener;
    typedef std::set<ProtectedAboutListener> AboutListenerSet;
    AboutListenerSet aboutListeners; /* About Signals are received out of Sessions so a set is all that is needed */

    qcc::Mutex aboutListenersLock;   /* Lock protecting the aboutListeners set */

    struct JoinContext {
        QStatus status;
        SessionId sessionId;
        SessionOpts opts;
    };

    std::map<qcc::Thread*, JoinContext> joinThreads;  /* List of threads waiting to join */
    qcc::Mutex joinLock;                              /* Mutex that protects joinThreads */
    KeyStoreKeyEventListener ksKeyEventListener;
    PermissionManager permissionManager;
    PermissionConfigurator permissionConfigurator;

    typedef qcc::ManagedObj<ApplicationStateListener*> ProtectedApplicationStateListener;
    typedef std::set<ProtectedApplicationStateListener> ApplicationStateListenerSet;
    ApplicationStateListenerSet applicationStateListeners; /* State Signals are received outside Sessions so a set container is all that is needed */

    qcc::Mutex applicationStateListenersLock;   /* Lock protecting the applicationStateListeners set */
    ObserverManager* observerManager;      /* The observer manager for the bus attachment */

    typedef qcc::ManagedObj<PermissionConfigurationListener*> ProtectedPermissionConfigurationListener;
    ProtectedPermissionConfigurationListener* permissionConfigurationListener;
    qcc::Mutex permissionConfigurationListenerLock;   /* Lock protecting permissionConfigurationListener */
};

}

#endif
