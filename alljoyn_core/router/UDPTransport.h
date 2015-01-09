/**
 * @file
 * UDPTransport is a specialization of class Transport for daemons talking over
 * UDP.
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

#ifndef _ALLJOYN_UDPTRANSPORT_H
#define _ALLJOYN_UDPTRANSPORT_H

#ifndef __cplusplus
#error Only include UDPTransport.h in C++ code.
#endif

#include <list>
#include <queue>
#include <alljoyn/Status.h>

#include <qcc/platform.h>
#include <qcc/atomic.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/time.h>

#include <alljoyn/TransportMask.h>

#include "Transport.h"
#include "RemoteEndpoint.h"

#include "ArdpProtocol.h"

#include "ns/IpNameService.h"

#define WORKAROUND_1298 1 /**< Implement workaround for ASACORE-1298 */

namespace ajn {

class ConnectEntry;
bool operator<(const ConnectEntry& lhs, const ConnectEntry& rhs);

class _UDPEndpoint; /**< Forward declaration for a class of endpoints implementing UDP Transport functionality */

class MessagePump; /**< Forward declaration for a class implementing an active message pump to move messages into the router */

#define N_PUMPS 8 /**<  The number of message pumps and possibly concurrent threads we use to move messages */

typedef qcc::ManagedObj<_UDPEndpoint> UDPEndpoint;

/**
 * @brief A class for UDP Transports used in daemons.
 */
class UDPTransport : public Transport, public _RemoteEndpoint::EndpointListener, public qcc::Thread {
    friend class MessagePump;
    friend class _UDPEndpoint;
    friend class ArdpStream;

  public:
    /**
     * Create a UDP based transport for use by daemons.
     *
     * @param bus The BusAttachment associated with this endpoint
     */
    UDPTransport(BusAttachment& bus);

    /**
     * Destructor
     */
    virtual ~UDPTransport();

    /**
     * Start the transport and associate it with a router.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus Start();

    /**
     * Stop the transport.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus Stop();

    /**
     * Pend the caller until the transport stops.
     *
     * @return
     *      - ER_OK if successful
     *      - an error status otherwise.
     */
    QStatus Join();

    /**
     * Determine if this transport is running. Running means Start() has been called.
     *
     * @return  Returns true if the transport is running.
     */
    bool IsRunning() { return Thread::IsRunning(); }

    /**
     * @internal
     * @brief Normalize a transport specification.
     *
     * Given a transport specification, convert it into a form which is guaranteed to
     * have a one-to-one relationship with a connection instance.
     *
     * @param inSpec    Input transport connect spec.
     * @param outSpec   Output transport connect spec.
     * @param argMap    Parsed parameter map.
     *
     * @return ER_OK if successful.
     */
    QStatus NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, std::map<qcc::String, qcc::String>& argMap) const;

    /**
     * Connect to a specified remote AllJoyn/DBus address.
     *
     * @param connectSpec    Transport specific key/value args used to configure the client-side endpoint.
     *                       The form of this string is @c "<transport>:<key1>=<val1>,<key2>=<val2>..."
     * @param opts           Requested sessions opts.
     * @param newep          [OUT] Endpoint created as a result of successful connect.
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep);

    /**
     * Disconnect from a specified AllJoyn/DBus address.
     *
     * @param connectSpec    The connectSpec used in Connect.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus Disconnect(const char* connectSpec);

    /**
     * Start listening for incomming connections on a specified bus address.
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *                    - Valid transport is @c "udp". All others ignored.
     *                    - Valid keys are:
     *                        - @c addr = IP address of server to connect to.
     *                        - @c port = Port number of server to connect to.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus StartListen(const char* listenSpec);

    /**
     * @brief Stop listening for incomming connections on a specified bus address.
     *
     * This method cancels a StartListen request. Therefore, the listenSpec must
     * match previous call to StartListen().
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *                    - Valid transport is @c "udp". All others ignored.
     *                    - Valid keys are:
     *                        - @c addr = IP address of server to connect to.
     *                        - @c port = Port number of server to connect to.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus StopListen(const char* listenSpec);

    /**
     * Set a listener for transport related events.  There can only be one
     * listener set at a time. Setting a listener implicitly removes any
     * previously set listener.
     *
     * @param listener  Listener for transport related events.
     */
    void SetListener(TransportListener* listener) { m_listener = listener; }

    /**
     * @internal
     * @brief Start discovering busses.
     */
    void EnableDiscovery(const char* namePrefix, TransportMask transports);

    /**
     * @internal
     * @brief Stop discovering busses.
     */
    void DisableDiscovery(const char* namePrefix, TransportMask transports);

    /**
     * Start advertising a well-known name with a given quality of service.
     *
     * @param advertiseName   Well-known name to add to list of advertised names.
     * @param quietly         Advertise the name quietly
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus EnableAdvertisement(const qcc::String& advertiseName, bool quietly, TransportMask transports);

    /**
     * Stop advertising a well-known name with a given quality of service.
     *
     * @param advertiseName   Well-known name to remove from list of advertised names.
     */
    void DisableAdvertisement(const qcc::String& advertiseName, TransportMask transports);

    /**
     * Returns the name of this transport
     */
    const char* GetTransportName() const { return TransportName; }

    /**
     * Get the transport mask for this transport
     *
     * @return the TransportMask for this transport.
     */
    TransportMask GetTransportMask() const { return TRANSPORT_UDP; }

    /**
     * Get a list of the possible listen specs of the current Transport for a
     * given set of session options.
     *
     * Session options specify high-level characteristics of session, such as
     * whether or not the underlying transport carries data encapsulated in
     * AllJoyn messages, and whether or not delivery is reliable.
     *
     * It is possible that there is more than one answer to the question: what
     * abstract address should I use when talking to another endpoint.  Each
     * Transports is equipped to understand how many answers there are and also
     * which answers are better than the others.  This method fills in the
     * provided vector with a list of currently available busAddresses ordered
     * according to which the transport thinks would be best.
     *
     * If there are no addresses appropriate to the given session options the
     * provided vector of String is left unchanged.  If there are addresses,
     * they are added at the end of the provided vector.
     *
     * @param opts Session options describing the desired characteristics of
     *             an underlying session
     * @param busAddrs A vector of String to which bus addresses corresponding
     *                 to IFF_UP interfaces matching the desired characteristics
     *                 are added.
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus GetListenAddresses(const SessionOpts& opts, std::vector<qcc::String>& busAddrs) const;

    /**
     * Does this transport support connections as described by the provided
     * session options.
     *
     * @param opts  Proposed session options.
     * @return
     *      - true if the SessionOpts specifies a supported option set.
     *      - false otherwise.
     */
    bool SupportsOptions(const SessionOpts& opts) const;

    /**
     * Indicates whether this transport is used for client-to-bus or bus-to-bus connections.
     *
     * @return  Always returns true, UDP is a bus-to-bus transport.
     */
    bool IsBusToBus() const { return true; }

    /**
     * Callback for UDPEndpoint exit.
     *
     * @param endpoint   UDPEndpoint instance that has exited.
     */
    void EndpointExit(RemoteEndpoint& endpoint);

    /**
     * Name of transport used in transport specs.
     */
    static const char* TransportName;

    class ConnectEntry;
    class ConnectEntry {
      public:
        ConnectEntry(qcc::Thread* thread, ArdpConnRecord* conn, uint32_t connId, qcc::Event* event) : m_thread(thread), m_conn(conn), m_connId(connId), m_event(event) { }
        friend bool operator<(const ConnectEntry& lhs, const ConnectEntry& rhs);
        qcc::Thread* m_thread;
        ArdpConnRecord* m_conn;
        uint32_t m_connId;
        qcc::Event* m_event;
    };

  private:
    /**
     * Empty private overloaded virtual function for Thread::Start
     * this avoids the overloaded-virtual warning. For the Thread::Start
     * function.
     */
    QStatus Start(void* arg, qcc::ThreadListener* listener) { return Thread::Start(arg, listener); }

    UDPTransport(const UDPTransport& other);
    UDPTransport& operator =(const UDPTransport& other);

    /**
     * This function will check the given endpoint to see if it is running on the same machine or not
     * by comparing the connecting IP address with the local machine's addresses. If there is a match
     * then this app is running on the local machine. Windows Universal Applications aren't allowed
     * to use the loopback interface so this must be a Desktop Application, set the group ID
     * accordingly. Since this code is only needed on Windows it is inside #ifdef
     * QCC_OS_GROUP_WINDOWS.
     *
     * @param endpoint The endpoint to check and set the group ID on
     */
    static void CheckEndpointLocalMachine(UDPEndpoint endpoint);

    BusAttachment& m_bus;                                          /**< The message bus for this transport */
    mutable volatile int32_t m_refCount;                           /**< Incremented if a thread is doing something somewhere */
    bool m_stopping;                                               /**< True if Stop() has been called but endpoints still exist */
    TransportListener* m_listener;                                 /**< Registered TransportListener */
    std::set<UDPEndpoint> m_preList;                               /**< "Pre" list of authenticating endpoints (see AcceptCb comments) */
    qcc::Mutex m_preListLock;                                      /**< Mutex that protects the endpoint and auth lists */
    std::set<UDPEndpoint> m_authList;                              /**< List of authenticating endpoints */
    std::set<UDPEndpoint> m_endpointList;                          /**< List of active endpoints */
    std::set<ConnectEntry> m_connectThreads;                       /**< List of threads starting up active endpoints */
    qcc::Mutex m_endpointListLock;                                 /**< Mutex that protects the endpoint and auth lists */

    MessagePump* m_messagePumps[N_PUMPS];                          /**< array of MessagePumps (with possiblly running pump threads) */

    std::list<std::pair<qcc::String, qcc::SocketFd> > m_listenFds; /**< File descriptors the transport is listening on */
    qcc::Mutex m_listenFdsLock;                                    /**< Mutex that protects m_listenFds */

    std::list<qcc::String> m_listenSpecs;                          /**< Listen specs clients have requested us to listen on */
    qcc::Mutex m_listenSpecsLock;                                  /**< Mutex that protects m_listenSpecs */

    std::list<qcc::String> m_discovering;                          /**< Name prefixes the transport is looking for */
    std::list<qcc::String> m_advertising;                          /**< Names the transport is advertising */
    std::list<qcc::String> m_listening;                            /**< ListenSpecs on which the transport is listening */
    qcc::Mutex m_discoLock;                                        /**< Mutex that protects discovery and advertisement lists */

    /**
     * @internal
     * @brief Commmand codes sent to the server accept loop thread.
     */
    enum RequestOp {
        START_LISTEN_INSTANCE,           /**< A StartListen() has happened */
        STOP_LISTEN_INSTANCE,            /**< A StopListen() has happened */
        ENABLE_ADVERTISEMENT_INSTANCE,   /**< An EnableAdvertisement() has happened */
        DISABLE_ADVERTISEMENT_INSTANCE,  /**< A DisableAdvertisement() has happened */
        ENABLE_DISCOVERY_INSTANCE,       /**< An EnableDiscovery() has happened */
        DISABLE_DISCOVERY_INSTANCE,      /**< A DisableDiscovery() has happened */
        HANDLE_NETWORK_EVENT             /**< A network event has happened */
    };

    /**
     * @internal
     * @brief Request code for communicating StartListen, StopListen requests
     * and started-advertising and stopped-advertising notifications to the
     * server accept loop thread.
     */
    class ListenRequest {
      public:
        RequestOp m_requestOp;
        qcc::String m_requestParam;
        bool m_requestParamOpt;
        TransportMask m_requestTransportMask;
        std::map<qcc::String, qcc::IPAddress> ifMap;
    };

    qcc::Mutex m_listenRequestsLock;  /**< Mutex that protects m_listenRequests */

    /**
     * @internal
     * @brief Log Warnings if endpoints are not progressing throughg states as expected
     */
    void EmitStallWarnings(UDPEndpoint& ep);

    /**
     * @internal
     * @brief Manage the list of endpoints for the transport.
     */
    void ManageEndpoints(qcc::Timespec authTimeout, qcc::Timespec sessionSetupTimeout);

    /**
     * @internal
     * @brief Thread entry point.
     *
     * @param arg  Unused thread entry arg.
     */
    qcc::ThreadReturn STDCALL Run(void* arg);

    /**
     * @internal
     * @brief Queue a StartListen request for the server accept loop
     *
     * The server accept loop (executing in UDPTransport::Run() uses the
     * socket FD resources that are used to listen on the endpoints specified
     * by the listenSpec parameters.  Creation and deletion of these resources
     * then happen with the involvement of two threads.
     *
     * In order to serialize the creation, deletion and use of these resources
     * we only change them in one place -- in the server accept loop.  This
     * means that we need to queue requests to start and stop listening on a
     * given listen spec to the accept loop thread.
     *
     * The sequence of events for StartListen() is then:
     *
     *   1.  The client calls StartListen().
     *   2.  StartListen() calls QueueStartListen() to send a request to the
     *       server accept loop thread.
     *   3.  The server accept loop thread picks up the request and calls
     *       DoStartListen() which allocates the resources.
     *
     * A similar sequence happens for StopListen().
     *
     * @param listenSpec A String containing the string specifying the address
     *                   and port to listen on.
     *
     * @see QueueStartListen
     * @see DoStartListen
     * @see StopListen
     */
    void QueueStartListen(qcc::String& listenSpec);

    /**
     * @internal @brief Perform the work required for a StartListen request
     * (doing it in the context of the server accept loop thread)
     *
     * @param listenSpec A String containing the string specifying the address
     *                   and port to listen on.
     *
     * @see StartListen
     * @see QueueStartListen
     */
    QStatus DoStartListen(qcc::String& listenSpec);

    /**
     * @internal
     * @brief Queue a StopListen request for the server accept loop
     *
     * The server accept loop (executing in UDPTransport::Run() uses the
     * socket FD resources that are used to listen on the endpoints specified
     * by the listenSpec parameters.  Creation and deletion of these resources
     * then happen with the involvement of two threads.
     *
     * In order to serialize the creation, deletion and use of these resources
     * we only change them in one place -- the server accept loop.  This means
     * that we need to queue requests to start and stop listening on a given
     * listen spec to the accept loop thread.
     *
     * The sequence of events for StopListen() is then:
     *
     *   1.  The client calls StopListen().
     *   2.  StopListen() calls QueueStopListen() to send a request to the
     *       server accept loop thread.
     *   3.  The server accept loop thread picks up the request and calls
     *       DoStopListen() which allocates the resources.
     *
     * A similar sequence happens for StartListen().
     *
     * @param listenSpec A String containing the string specifying the address
     *                   and port to stop listening on.
     *
     * @see QueueStopListen
     * @see DoStopListen
     * @see StartListen
     */
    void QueueStopListen(qcc::String& listenSpec);

    /**
     * @internal @brief Perform the work required for a StopListen request
     * (doing it in the context of the server accept loop thread)
     *
     * @param listenSpec A String containing the string specifying the address
     *                   and port to stop listening on.
     *
     * @see StopListen
     * @see QueueStopListen
     */
    void DoStopListen(qcc::String& listenSpec);

    void QueueHandleNetworkEvent(const std::map<qcc::String, qcc::IPAddress>&);

    /**
     * @internal
     * @brief Authentication complete notificiation.
     *
     * @param conn Reference to the UDPEndpoint that completed authentication.
     */
    void Authenticated(UDPEndpoint& conn);

    /**
     * @internal
     * @brief Normalize a listen specification.
     *
     * Given a listen specification (which is the same as a transport
     * specification but with relaxed semantics allowing defaults), convert
     * it into a form which is guaranteed to have a one-to-one relationship
     * with a listener instance.
     *
     * @param inSpec    Input transport connect spec.
     * @param outSpec   Output transport connect spec.
     * @param argMap    Parsed parameter map.
     *
     * @return ER_OK if successful.
     */
    QStatus NormalizeListenSpec(const char* inSpec, qcc::String& outSpec, std::map<qcc::String, qcc::String>& argMap) const;

    class FoundCallback {
      public:
        FoundCallback(TransportListener*& listener) : m_listener(listener) { }
        void Found(const qcc::String& busAddr, const qcc::String& guid, std::vector<qcc::String>& nameList, uint32_t timer);
      private:
        TransportListener*& m_listener;
    };

    FoundCallback m_foundCallback;  /**< Called by IpNameService when new busses are discovered */

    class NetworkEventCallback {
      public:
        NetworkEventCallback(UDPTransport& transport) : m_transport(transport) { }
        void Handler(const std::map<qcc::String, qcc::IPAddress>&);
      private:
        UDPTransport& m_transport;
    };

    NetworkEventCallback m_networkEventCallback;  /**< Called by IpNameService when new network interfaces come up */
    /**
     * @brief The default timeout for in-process authentications.
     *
     * The authentication process can be used as the basis of a denial of
     * service attack by simply stopping in mid-authentication.  If an
     * authentication takes longer than this number of milliseconds, it may be
     * summarily aborted if another connection comes in.  This value can be
     * overridden in the config file by setting "auth_timeout".
     */
    static const uint32_t ALLJOYN_AUTH_TIMEOUT_DEFAULT = 20000;

    /**
     * @brief The default timeout for session establishment following authentication.
     *
     * The session establishment process can be used as the basis of a denial of
     * service attack by simply not sending the AttachSession.  If the session
     * establishment takes longer than this number of milliseconds, it may be
     * summarily aborted if another connection comes in.  This value can be
     * overridden in the config file by setting "session_setup_timeout".  The 30 second
     * number comes from the the timeout used for AttachSession method calls which is
     * 30 seconds.
     */
    static const uint32_t ALLJOYN_SESSION_SETUP_TIMEOUT_DEFAULT = 30000;

    /**
     * @brief The default value for the maximum number of authenticating
     * connections.
     *
     * This corresponds to the configuration item "max_incomplete_connections"
     * in the DBus configuration, but it applies only to the UDP transport.  To
     * override this value, change the limit, "max_incomplete_connections_udp".
     * Typically, DBus sets this value to 10,000 which is essentially infinite
     * from the perspective of a phone.  Since this represents a transient state
     * in connection establishment, there should be few connections in this
     * state, so we default to a quite low number.
     */
    static const uint32_t ALLJOYN_MAX_INCOMPLETE_CONNECTIONS_UDP_DEFAULT = 10;

    /**
     * @brief The default value for the maximum number of UDP connections
     * (remote endpoints).
     *
     * This corresponds to the configuration item "max_completed_connections"
     * in the DBus configuration, but it applies only to the UDP transport.
     * To override this value, change the limit, "max_completed_connections_udp".
     * Typically, DBus sets this value to 100,000 which is essentially infinite
     * from the perspective of a phone.  Since we expect bus topologies to be
     * relatively small, we default to a quite low number.
     *
     * @warning This maximum is enforced on incoming connections only.  An
     * AllJoyn daemon is free to form as many outbound connections as it pleases
     * but if the total number of connections exceeds this value, no inbound
     * connections will be accepted.  This is because we are defending against
     * attacks from "abroad" and trust ourselves implicitly.
     */
    static const uint32_t ALLJOYN_MAX_COMPLETED_CONNECTIONS_UDP_DEFAULT = 50;

    /**
     * @brief The default value for the maximum number of untrusted clients
     *
     * This corresponds to the configuration item "max_untrusted_clients"
     * To override this value, change the limit, "max_untrusted_clients".
     *
     * @warning This maximum is enforced on incoming connections from untrusted clients only.
     * This is to limit the amount of resources being used by untrusted clients.
     */
    static const uint32_t ALLJOYN_MAX_UNTRUSTED_CLIENTS_DEFAULT = 0;

    /**
     * @brief The default value for the router advertisement prefix that untrusted thin clients
     * will use for the discovery of the daemon.
     *
     * This corresponds to the configuration item "router_advertisement_prefix"
     * To override this value, change the limit, "router_advertisement_prefix".
     *
     */
    static const char* const ALLJOYN_DEFAULT_ROUTER_ADVERTISEMENT_PREFIX;

    /*
     * The Android Compatibility Test Suite (CTS) is used by Google to enforce a
     * common idea of what it means to be Android.  One of their tests is to
     * make sure there are no UDP or UDP listeners in running processes when the
     * phone is idle.  Since we want to be able to run our daemon on idle phones
     * and manufacturers want their Android phones to pass the CTS, we have got
     * to be able to shut off our listeners unless they are actually required.
     *
     * To do this, we need to keep track of whether or not the daemon is
     * actively advertising or discovering.  To this end, we keep track of the
     * advertise and discover calls, and if there are any outstanding (not
     * canceled) we enable the Name Service to talk to the outside world.  If
     * there are no outstanding operations, we tell the Name Service to shut up.
     *
     * There is nothing preventing us from receiving multiple identical
     * discovery and advertisement requests, so we allow multiple instances of
     * an identical name on the list.  We could reference count the entries, but
     * this seems like a relatively rare condition, so we take the
     * straightforward approach.
     */
    enum DiscoveryOp {
        ENABLE_DISCOVERY,  /**< A request to start a discovery has been received */
        DISABLE_DISCOVERY  /**< A request to cancel a discovery has been received */
    };

    enum AdvertiseOp {
        ENABLE_ADVERTISEMENT,  /**< A request to start advertising has been received */
        DISABLE_ADVERTISEMENT  /**< A request to cancel advertising has been received */
    };

    enum ListenOp {
        START_LISTEN,  /**< A request to start listening has been received */
        STOP_LISTEN    /**< A request to stop listening has been received */
    };

    /**
     * @brief Add or remove a discover indication.
     *
     * The transport has received a new discovery operation.  This will either
     * be an EnableDiscovery() or DisbleDiscovery() discriminated by the
     * DiscoveryOp enum.
     *
     * We want to keep a list of name prefixes that are currently active for
     * well-known name discovery.  The presence of a non-zero size of this list
     * indicates discovery is in-process and the Name Service should be kept
     * alive (it can be listening for inbound packets in particular).
     *
     * @return true if the list of discoveries is empty as a result of the
     *              operation.
     */
    bool NewDiscoveryOp(DiscoveryOp op, qcc::String namePrefix, bool& isFirst);

    /**
     * @brief Add or remove an advertisement indication.
     *
     * Called when the transport has received a new advertisement operation.
     * This will either be an EnableAdvertisement() or DisbleAdvertisement()
     * discriminated by the AdvertiseOp enum.
     *
     * We want to keep a list of names that are currently being advertised.  The
     * presence of a non-zero size of this list indicates that at least one
     * advertisement is in-process and the Name Service should be kept alive to
     * respond to WHO_HAS queries.
     *
     * @return true if the list of advertisements is empty as a result of the
     *              operation.
     */
    bool NewAdvertiseOp(AdvertiseOp op, qcc::String name, bool& isFirst);

    /**
     * @brief Add or remove a listen operation.
     *
     * Called when the transport has received a new listen operation.
     * This will either be an StartListen() or StopListen()
     * discriminated by the ListenOp enum.
     *
     * We want to keep a list of listen specs that are currently being listened
     * on.  This lest is kept so we can tear down the listeners if there are no
     * advertisements and recreate it if an advertisement is started.
     *
     * This is keep UDP from having a listener so that the Android Compatibility
     * test suite can pass with when the daemon is in the quiescent state.
     *
     * @return true if the list of listeners is empty as a result of the
     *              operation.
     */
    bool NewListenOp(ListenOp op, qcc::String name);

    void QueueEnableDiscovery(const char* namePrefix, TransportMask transports);
    void QueueDisableDiscovery(const char* namePrefix, TransportMask transports);
    void QueueEnableAdvertisement(const qcc::String& advertiseName, bool quietly, TransportMask transports);
    void QueueDisableAdvertisement(const qcc::String& advertiseName, TransportMask transports);

    void RunListenMachine(ListenRequest& listenRequest);

    void StartListenInstance(ListenRequest& listenRequest);
    void StopListenInstance(ListenRequest& listenRequest);
    void EnableAdvertisementInstance(ListenRequest& listenRequest);
    void DisableAdvertisementInstance(ListenRequest& listenRequest);
    void EnableDiscoveryInstance(ListenRequest& listenRequest);
    void DisableDiscoveryInstance(ListenRequest& listenRequest);
    void HandleNetworkEventInstance(ListenRequest& listenRequest);
    void UntrustedClientExit();
    QStatus UntrustedClientStart();
    bool m_isAdvertising;
    bool m_isDiscovering;
    bool m_isListening;
    bool m_isNsEnabled;

    enum ReloadState {
        STATE_RELOADING = 0,    /**< The set of listen FDs has changed and needs to be reloaded by the main thread */
        STATE_RELOADED,         /**< The set of listen FDs has been reloaded by the main thread */
        STATE_EXITED            /**< The main UDPTransport thread has exited */
    };

    ReloadState m_reload;       /**< Flag used for synchronization of DoStopListen with the Run thread */

    enum ManageState {
        STATE_MANAGE = 0,       /**< The state of the endpoints has changed and needs to be managed by the main thread */
        STATE_MANAGED           /**< The state of the endpoints has been managed by the main thread */
    };

    ManageState m_manage;       /**< Flag used for synchronization of ManageEndpoints() */

    std::map<qcc::String, uint16_t> m_listenPortMap; /**< If m_isListening, a map of the ports on which we are listening on different interfaces/addresses */

    std::map<qcc::String, qcc::IPEndpoint> m_requestedInterfaces; /**< A map of requested interfaces and corresponding IP addresses/ports or defaults */

    std::map<qcc::String, qcc::String> m_requestedAddresses; /**< A map of requested IP addresses to interfaces or defaults */

    std::map<qcc::String, uint16_t> m_requestedAddressPortMap; /**< A map of requested IP addresses to ports */

    std::list<ListenRequest> m_pendingAdvertisements; /**< A list of advertisement requests that came in while no interfaces were yet IFF_UP */

    std::list<ListenRequest> m_pendingDiscoveries; /**< A list of discovery requests that came in while no interfaces were yet IFF_UP */

    int32_t m_nsReleaseCount; /**< the number of times we have released the name service singleton */

    bool m_wildcardIfaceProcessed; /**< flag to avoid processing network events if we have a wildcard interface request */

    bool m_wildcardAddressProcessed;

    /**< The router advertisement prefix set in the configuration file appended with the BusController's unique name */
    qcc::String m_routerName;

    int32_t m_maxUntrustedClients; /**< the maximum number of untrusted clients allowed at any point of time */

    int32_t m_numUntrustedClients; /**< Number of untrusted clients currently registered with the daemon */

    /**
     * authTimeout is the maximum amount of time we allow incoming connections to
     * mess about while they should be authenticating.  If they take longer
     * than this time, we feel free to disconnect them as deniers of service.
     */
    qcc::Timespec m_authTimeout;

    /**
     * sessionSetupTimeout is the maximum amount of time we allow incoming connections to
     * mess about while they should be sending messages to set up the session routes.
     * If they take longer than this time, we feel free to disconnect them as deniers of service.
     */
    qcc::Timespec m_sessionSetupTimeout;

    /**
     * m_maxAuth is the maximum number of incoming connections that can be in
     * the process of authenticating.  If starting to authenticate a new
     * connection would mean exceeding this number, we drop the new connection.
     */
    uint32_t m_maxAuth;

    /**
     * m_maxConn is the maximum number of active connections possible over the
     * UDP transport.  If starting to process a new connection would mean
     * exceeding this number, we reject the new connection.
     */
    uint32_t m_maxConn;

    /**
     * m_currAuth is the current number of incoming connections that are in the
     * process of authenticating.  If starting to authenticate a new connection
     * would mean this number exceeding m_maxAuth, we reject the new connection.
     */
    volatile int32_t m_currAuth;

    /**
     * m_currConn is current number of active connections running over the
     * UDP transport.  If starting to process a new connection would mean
     * this number exceeds m_maxConn, we reject the new connection.
     */
    volatile int32_t m_currConn;

    qcc::Mutex m_connLock;  /**< m_currAuth and m_currConn must be changed atomically, so need to be mutex protected */

    /**
     * arcpConfig are the limits from the daemon configuration database relating
     * to the various configurable settings for the ARDP protocol.
     */
    ArdpGlobalConfig m_ardpConfig;

    qcc::Mutex m_ardpLock;  /**< Since written for embedded as well as daemon environments, ARDP is not thread-safe */
    qcc::Mutex m_cbLock;    /**< Lock to synchronize interactions between callback contexts and other threads */

    ArdpHandle* m_handle;

    /**
     * MessageDispatcherThread handles AllJoyn messages that have been received
     * by the transport and need to be sent of into the daemon router and off
     * to a destination.
     */
    class DispatcherThread : public qcc::Thread {
      public:
        DispatcherThread(UDPTransport* transport) : qcc::Thread(qcc::String("UDP Dispatcher")), m_transport(transport) { }
        void ThreadExit(Thread* thread);

      protected:
        qcc::ThreadReturn STDCALL Run(void* arg);

      private:
        UDPTransport* m_transport;
    };

    DispatcherThread* m_dispatcher;  /** Pointer to the message dispatcher thread for the transport */

    /**
     * MessageDispatcherThread handles EndpointExit processing to avoid deadlock
     * in cases where DaemonRouter::UnregisterEndpoint() calls back out into
     * PushMessage() due to NameChanged.
     */
    class ExitDispatcherThread : public qcc::Thread {
      public:
        ExitDispatcherThread(UDPTransport* transport) : qcc::Thread(qcc::String("UDP Exit Dispatcher")), m_transport(transport) { }
        void ThreadExit(Thread* thread);

      protected:
        qcc::ThreadReturn STDCALL Run(void* arg);

      private:
        UDPTransport* m_transport;
    };

    ExitDispatcherThread* m_exitDispatcher;  /** Pointer to the exit dispatcher thread for the transport */

    class WorkerCommandQueueEntry {
      public:
        WorkerCommandQueueEntry()
            : m_command(NONE), m_handle(NULL), m_conn(NULL), m_connId(0), m_rcv(NULL), m_passive(false), m_buf(NULL), m_len(0), m_status(ER_OK) { }
        enum Command {
            NONE,
            EXIT,
            CONNECT_CB,
            DISCONNECT_CB,
            RECV_CB,
            SEND_CB
        };

        Command m_command;
        ArdpHandle* m_handle;
        ArdpConnRecord* m_conn;
        uint32_t m_connId;
        ArdpRcvBuf* m_rcv;
        bool m_passive;
        uint8_t* m_buf;
        uint32_t m_len;
        QStatus m_status;
    };

    std::queue<WorkerCommandQueueEntry> m_workerCommandQueue;  /** Queue of commands to dispatch to the router */
    qcc::Mutex m_workerCommandQueueLock; /**< Lock to synchronize access to the message dispatcher command queue */

    std::queue<WorkerCommandQueueEntry> m_exitWorkerCommandQueue;  /** Queue of exit commands to dispatch to the router */
    qcc::Mutex m_exitWorkerCommandQueueLock; /**< Lock to synchronize access to the exit dispatcher command queue */

#if WORKAROUND_1298
    bool m_done1298; /**< True if workaround for "socket closed during CancelAdvertise" issue (ASACORE-1298) has been done */
#endif

#ifndef NDEBUG
    void DebugPreListCheck(UDPEndpoint uep);
    void DebugAuthListCheck(UDPEndpoint uep);
    void DebugEndpointListCheck(UDPEndpoint uep);
#endif

#if ARDP_TESTHOOKS
    static void ArdpSendToSGHook(ArdpHandle* handle, ArdpConnRecord* conn, TesthookSource source, qcc::ScatterGatherList& msgSG);
    static void ArdpSendToHook(ArdpHandle* handle, ArdpConnRecord* conn, TesthookSource source, void* buf, uint32_t len);
    static void ArdpRecvFromHook(ArdpHandle* handle, ArdpConnRecord* conn, TesthookSource source, void* buf, uint32_t len);
#endif

    static bool ArdpAcceptCb(ArdpHandle* handle, qcc::IPAddress ipAddr, uint16_t ipPort, ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status);
    static void ArdpConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status);
    static void ArdpDisconnectCb(ArdpHandle* handle, ArdpConnRecord* conn, QStatus status);
    static void ArdpRecvCb(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* rcv, QStatus status);
    static void ArdpSendCb(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status);
    static void ArdpSendWindowCb(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t window, QStatus status);

    bool AcceptCb(ArdpHandle* handle, qcc::IPAddress ipAddr, uint16_t ipPort, ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status);
    void ConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status);
    void DoConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t connId, bool passive, uint8_t* buf, uint16_t len, QStatus status);
    void DisconnectCb(ArdpHandle* handle, ArdpConnRecord* conn, QStatus status);
    void RecvCb(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* rcv, QStatus status);
    void SendCb(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status);
    void SendWindowCb(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t window, QStatus status);
    void ExitEndpoint(uint32_t connId);
};

} // namespace ajn

#endif // _ALLJOYN_UDPTRANSPORT_H
