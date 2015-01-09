/**
 * @file
 * WFDTransport is a specialization of class Transport for daemons talking over
 * Wi-Fi Direct links and doing Wi_Fi Direct pre-association service discovery.
 */

/******************************************************************************
 * Copyright (c) 2012, 2014 AllSeen Alliance. All rights reserved.
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

#ifndef _ALLJOYN_WFDTRANSPORT_H
#define _ALLJOYN_WFDTRANSPORT_H

#ifndef __cplusplus
#error Only include WFDTransport.h in C++ code.
#endif

#include <list>
#include <queue>
#include <alljoyn/Status.h>

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/time.h>


#include <alljoyn/TransportMask.h>

#include "Transport.h"
#include "RemoteEndpoint.h"

#include "P2PNameService.h"
#include "P2PConMan.h"

namespace ajn {

class _WFDEndpoint;
typedef qcc::ManagedObj<_WFDEndpoint> WFDEndpoint;
/**
 * @brief A class for WFD Transports used in daemons running on Android
 */
class WFDTransport : public Transport, public _RemoteEndpoint::EndpointListener, public qcc::Thread {
    friend class _WFDEndpoint;

  public:

    /**
     * Define a quantity to be added to the JoinSession timeout in a
     * BusAttachment due to the added group creation, group configuration and IP
     * name service distovery times that will happen due to the Wi-Fi Direct
     * infrastructure requirements.  This is a clear layering violation, but we
     * don't want to have a default timeout of around two and a half minutes up
     * in BusAttachment unless we can absolutely avoid it.
     */
    static const uint32_t AddedCallTimeout = P2PConMan::TEMPORARY_NETWORK_ESTABLISH_TIMEOUT + P2PConMan::CREATE_CONNECT_SPEC_TIMEOUT;

    /**
     * Create a WFD based transport for use by daemons running on Android.
     *
     * @param bus The BusAttachment associated with this endpoint
     */
    WFDTransport(BusAttachment& bus);

    /**
     * Destructor
     */
    virtual ~WFDTransport();

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
     *                    - Valid transport is @c "wfd". All others ignored.
     *                    - Valid keys are:
     *                        - @c addr = IP address to listen for inbound connections on.
     *                        - @c port = Port number to listen for inbound connections on.
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
     *                    - Valid transport is @c "wfd". All others ignored.
     *                    - Valid keys are:
     *                        - @c addr = IP address to stop listening for inbound connections on.
     *                        - @c port = Port number to stop listening for inbound connections on.
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
    void EnableDiscovery(const char* namePrefix);

    /**
     * @internal
     * @brief Stop discovering busses.
     */
    void DisableDiscovery(const char* namePrefix);

    /**
     * Start advertising a well-known name with the given quality of service.
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
    TransportMask GetTransportMask() const { return TRANSPORT_WFD; }

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
     * Indicates whether this transport is used for client-to-bus or bus-to-bus connections.
     *
     * @return  Always returns true, WFD implies a bus-to-bus transport.
     */
    bool IsBusToBus() const { return true; }

    /**
     * Callback for WFDEndpoint exit.
     *
     * @param endpoint   WFDEndpoint instance that has exited.
     */
    void EndpointExit(RemoteEndpoint& endpoint);

    /**
     * Name of transport used in transport specs.
     */
    static const char* TransportName;

  private:
    WFDTransport(const WFDTransport& other);
    WFDTransport& operator =(const WFDTransport& other);

    BusAttachment& m_bus;                                          /**< The message bus for this transport */
    bool m_stopping;                                               /**< True if Stop() has been called but endpoints still exist */
    TransportListener* m_listener;                                 /**< Registered TransportListener */
    std::set<WFDEndpoint> m_authList;                             /**< List of authenticating endpoints */
    std::set<WFDEndpoint> m_endpointList;                         /**< List of active endpoints */
    std::set<Thread*> m_activeEndpointsThreadList;                 /**< List of threads starting up active endpoints */
    qcc::Mutex m_endpointListLock;                                 /**< Mutex that protects the endpoint and auth lists */

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
    };

    std::queue<ListenRequest> m_listenRequests;                    /**< Queue of StartListen and StopListen requests */
    qcc::Mutex m_listenRequestsLock;                               /**< Mutex that protects m_listenRequests */

    /**
     * @internal
     * @brief Manage the list of endpoints for the transport.
     */
    void ManageEndpoints(qcc::Timespec tTimeout);

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
     * The server accept loop (executing in WFDTransport::Run() uses the
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
    void DoStartListen(qcc::String& listenSpec);

    /**
     * @internal
     * @brief Queue a StopListen request for the server accept loop
     *
     * The server accept loop (executing in WFDTransport::Run() uses the
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

    /**
     * @internal
     * @brief Authentication complete notificiation.
     *
     * @param conn Pointer to the WFDEndpoint that completed authentication.
     */
    void Authenticated(WFDEndpoint& conn);

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
     * @brief The default value for the maximum number of authenticating
     * connections.
     *
     * This corresponds to the configuration item "max_incomplete_connections"
     * in the DBus configuration, but it applies only to the WFD transport.  To
     * override this value, change the limit, "max_incomplete_connections_wfd".
     * Typically, DBus sets this value to 10,000 which is essentially infinite
     * from the perspective of a phone.  Since this represents a transient state
     * in connection establishment, there should be few connections in this
     * state, so we default to a quite low number.
     */
    static const uint32_t ALLJOYN_MAX_INCOMPLETE_CONNECTIONS_WFD_DEFAULT = 10;

    /**
     * @brief The default value for the maximum number of WFD connections
     * (remote endpoints).
     *
     * This corresponds to the configuration item "max_completed_connections"
     * in the DBus configuration, but it applies only to the WFD transport.
     * To override this value, change the limit, "max_completed_connections_wfd".
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
    static const uint32_t ALLJOYN_MAX_COMPLETED_CONNECTIONS_WFD_DEFAULT = 50;

    /*
     * The Android Compatibility Test Suite (CTS) is used by Google to enforce a
     * common idea of what it means to be Android.  One of their tests is to
     * make sure there are no TCP or UDP listeners in running processes when the
     * phone is idle.  Since we want to be able to run our daemon on idle phones
     * and manufacturers want their Android phones to pass the CTS, we have got
     * to be able to shut off our listeners unless they are actually required.
     *
     * To do this, we need to keep track of whether or not the transport is
     * actively advertising or discovering.  To this end, we keep track of the
     * advertise and discover calls, and if there are any outstanding (not
     * canceled) we enable the IP and P2P name services to talk to the outside
     * world.  If there are no outstanding operations, we tell the name services
     * to shut up.  It may be the case that another transport has active
     * advertise or discover operations, in which case the name services may
     * continue doing their thing.
     *
     * There is nothing preventing us from receiving multiple identical
     * discovery and advertisement requests, so we allow multiple instances of
     * an identical name on the list.  We could reference count the entries, but
     * this seems like a relatively rare condition, so we take the
     * straightforward approach and just (over)write many, delete once.
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
     * This is keep TCP from having a listener running so that the Android
     * Compatibility test suite can pass with when the daemon is in the
     * quiescent state.
     *
     * @return true if the list of listeners is empty as a result of the
     *              operation.
     */
    bool NewListenOp(ListenOp op, qcc::String name);

    void QueueEnableDiscovery(const char* namePrefix);
    void QueueDisableDiscovery(const char* namePrefix);
    void QueueEnableAdvertisement(const qcc::String& advertiseName);
    void QueueDisableAdvertisement(const qcc::String& advertiseName);

    void RunListenMachine(void);

    void StartListenInstance(ListenRequest& listenRequest);
    void StopListenInstance(ListenRequest& listenRequest);
    void EnableAdvertisementInstance(ListenRequest& listenRequest);
    void DisableAdvertisementInstance(ListenRequest& listenRequest);
    void EnableDiscoveryInstance(ListenRequest& listenRequest);
    void DisableDiscoveryInstance(ListenRequest& listenRequest);

    bool m_isAdvertising;
    bool m_isDiscovering;
    bool m_isListening;
    bool m_isNsEnabled;

    uint16_t m_listenPort;     /**< If m_isListening, is the port on which we are listening */

    /**
     * Callback from the P2P name service indicating that it has found or lost
     * and advertised name.
     */
    void P2PNameServiceCallback(const qcc::String& guid, qcc::String& name, uint8_t timer);

    /**
     * Callback from the P2P connection manager indicating that a link and
     * associated interface has come up or gone down.
     */
    void P2PConManStateCallback(P2PConMan::LinkState state, const qcc::String& interface);

    /**
     * Callback from the P2P connection manager indicating that it got a
     * FoundAdvertisedName from the IP name service.
     */
    void P2PConManNameCallback(const qcc::String& busAddr, const qcc::String& guid, std::vector<qcc::String>& nameList, uint8_t timer);

    bool m_p2pNsAcquired;  /**< True if we've done an Acquire() on the P2P name service singleton */
    bool m_p2pCmAcquired;  /**< True if we've done an Acquire() on the P2P connection manager singleton */
    bool m_ipNsAcquired;   /**< True if we've done an Acquire() on the IP name service singleton */
};

} // namespace ajn

#endif // _ALLJOYN_WFDTRANSPORT_H
