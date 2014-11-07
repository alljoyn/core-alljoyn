/**
 * @file
 * RemoteEndpoint provides a remote endpoint rx and tx threading.
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
#ifndef _ALLJOYN_REMOTEENDPOINT_H
#define _ALLJOYN_REMOTEENDPOINT_H

#include <qcc/platform.h>

#include <deque>

#include <qcc/atomic.h>
#include <qcc/String.h>
#include <qcc/GUID.h>
#include <qcc/Mutex.h>
#include <qcc/Stream.h>
#include <qcc/Thread.h>

#include "BusEndpoint.h"
#include "EndpointAuth.h"

#include <alljoyn/Status.h>

namespace ajn {

class _RemoteEndpoint;

/**
 * Managed object type that wraps a remote endpoint
 */
typedef qcc::ManagedObj<_RemoteEndpoint> RemoteEndpoint;

/**
 * %RemoteEndpoint handles incoming and outgoing messages
 * over a stream interface
 */
class _RemoteEndpoint : public _BusEndpoint, public qcc::ThreadListener, public qcc::IOReadListener, public qcc::IOWriteListener, public qcc::IOExitListener {

  public:
    const static uint32_t MAX_CONTROL_MSGS_PER_SECOND = 10;
    /**
     * RemoteEndpoint::Features type. Features are values that are negotiated during session
     * establishment.
     */
    class Features {

      public:

        Features() : isBusToBus(false), allowRemote(false), handlePassing(false), ajVersion(0), protocolVersion(0), processId(0), trusted(false)
        { }

        bool isBusToBus;       /**< When initiating connection this is an input value indicating if this is a bus-to-bus connection.
                                    When accepting a connection this is an output value indicating if this is bus-to-bus connection. */

        bool allowRemote;      /**< When initiating a connection this input value tells the local daemon whether it wants to receive
                                    messages from remote busses. When accepting a connection, this output indicates whether the connected
                                    endpoint is willing to receive messages from remote busses. */

        bool handlePassing;    /**< Indicates if support for handle passing is enabled for this the endpoint. This is only
                                    enabled for endpoints that connect applications on the same device. */

        uint32_t ajVersion;        /**< The AllJoyn version negotiated with the remote peer */

        uint32_t protocolVersion;  /**< The AllJoyn version negotiated with the remote peer */

        uint32_t processId;        /**< Process id optionally obtained from the remote peer */

        bool trusted;              /**< Indicated if the remote client was trusted */
        SessionOpts::NameTransferType nameTransfer;

    };

    /**
     * Listener called when endpoint changes state.
     */
    class EndpointListener {
      public:
        /**
         * Virtual destructor for derivable class.
         */
        virtual ~EndpointListener() { }

        /**
         * Called when a new untrusted client has connected to the daemon.
         * @return
         *       - ER_OK if client is to be accepted
         *       - ER_BUS_NOT_ALLOWED if client NOT to be accepted
         *       - ER_NOT_IMPLEMENTED for transports that do not allow untrusted clients.
         */
        virtual QStatus UntrustedClientStart() { return ER_NOT_IMPLEMENTED; };

        /**
         * Called when an untrusted client exits.
         */
        virtual void UntrustedClientExit() { /*Default do-nothing*/ };

        /**
         * Called when endpoint is about to exit.
         *
         * @param[in] ep   Endpoint that is exiting.
         */
        virtual void EndpointExit(RemoteEndpoint& ep) = 0;
    };

    /**
     * Called when a new untrusted client has connected to the daemon.
     * This calls into the transport's UntrustedClientStart function
     * and returns the value received to the caller.
     * @return
     *       - ER_OK if client is to be accepted
     *       - ER_BUS_NOT_ALLOWED if client NOT to be accepted
     *       - ER_NOT_IMPLEMENTED for transports that do not allow untrusted clients.
     */
    QStatus UntrustedClientStart();
    /**
     * Default constructor initializes an invalid endpoint. This allows for the declaration of uninitialized RemoteEndpoint variables.
     */
    _RemoteEndpoint() : internal(NULL), minimalEndpoint(false) { }

    /**
     * Constructor
     *
     * @param[in] bus            Message bus associated with transport.
     * @param[in] incoming       true iff this is an incoming connection.
     * @param[in] connectSpec    AllJoyn connection specification for this endpoint.
     * @param[in] stream         Socket Stream used to communicate with media.
     * @param[in] type           Base name for thread.
     * @param[in] isSocket       true iff stream is actually a socketStream.
     */
    _RemoteEndpoint(BusAttachment& bus,
                    bool incoming,
                    const qcc::String& connectSpec,
                    qcc::Stream* stream,
                    const char* type = "endpoint",
                    bool isSocket = true, bool minimal = false);

    /**
     * Destructor
     */
    virtual ~_RemoteEndpoint();

    /**
     * Send an outgoing message.
     *
     * @param[in] msg   Message to be sent.
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    virtual QStatus PushMessage(Message& msg);

    /**
     * Start the endpoint.
     *
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    virtual QStatus Start();

    /**
     * Start the endpoint.
     *
     * @param[in] idleTimeout   Idle Timeout for the link. i.e. time after which the Routing node must
     *                send the DBus ping in case of inactivity.
     * @param[in] probeTimeout  Probe timeout. The time from the Routing node sending the DBus
     *                          ping to the expected response.
     * @param[in] numProbes     Number of probes sent before the leaf node is declared dead.
     * @param[in] sendTimeout   Send Timeout for the link. i.e. time after which the Routing node must
     *                          disconnect the remote node if it has not read a message from the link
     *                          in the situation that the send buffer on this end and receive buffer on
     *                          the remote end are full.
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    virtual QStatus Start(uint32_t idleTimeout, uint32_t probeTimeout, uint32_t numProbes, uint32_t sendTimeout);

    /**
     * Request the endpoint to stop executing.
     *
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    virtual QStatus Stop();

    /**
     * Request endpoint to stop AFTER the endpoint's txqueue empties out.
     *
     * @param[in] maxWaitMS    Max number of ms to wait before stopping or 0 for wait indefinitely.
     *
     * @return
     *     - ER_OK if successful.
     */
    QStatus StopAfterTxEmpty(uint32_t maxWaitMs = 0);

    /**
     * Request endpoint to pause receiving (wihtout stopping) AFTER next METHOD_REPLY is received.
     *
     * @return
     *     - ER_OK if successful.
     */
    QStatus PauseAfterRxReply();

    /**
     * Set the underlying stream for this RemoteEndpoint.
     * This call can be used to override the Stream set in RemoteEndpoint's constructor
     * @param[in] stream    Stream to use.
     */
    void SetStream(qcc::Stream* stream);

    /**
     * Join the endpoint.
     * Block the caller until the endpoint is stopped.
     *
     * @return ER_OK if successful.
     */
    virtual QStatus Join(void);

    /**
     * Set the listener for this endpoint.
     *
     * @param[in] listener   Endpoint listener.
     */
    void SetListener(EndpointListener* listener);

    /**
     * Get the unique bus name assigned by the bus for this endpoint.
     *
     * @return
     *      - The unique bus name for this endpoint
     *      - An empty string if called before the endpoint has been established.
     */
    const qcc::String& GetUniqueName() const;

    /**
     * Set the unique bus name assigned by the bus for this endpoint.
     *
     * @param[in] uniqueName The unique bus name for this endpoint
     */
    void SetUniqueName(const qcc::String& uniqueName);

    /**
     * Get the bus name for the peer at the remote end of this endpoint.
     *
     * @return  - The bus name of the remote side.
     *          - An empty string if called before the endpoint has been established.
     */
    const qcc::String& GetRemoteName() const;

    /**
     * Set the bus name for the peer at the remote end of this endpoint.
     *
     * @param[in] remoteName The bus name of the remote side.
     */
    void SetRemoteName(const qcc::String& remoteName);

    /**
     * Get the protocol version used by the remote end of this endpoint.
     *
     * @return  - The protocol version use by the remote side
     *          - 0 if called before the endpoint has been established
     */
    uint32_t GetRemoteProtocolVersion() const { return GetFeatures().protocolVersion; }

    /**
     * Get the AllJoyn version of the remote end of this endpoint
     *
     * @return  - The AllJoyn version use by the remote side, as returned by the function ajn::GetNumericVersion
     *          - 0 if the remote EP predates release 2.5
     */
    uint32_t GetRemoteAllJoynVersion() const { return GetFeatures().ajVersion; }

    /**
     * Establish a connection.
     *
     * @param[in] authMechanisms  The authentication mechanism(s) to use.
     * @param[out] authUsed    Returns the name of the authentication method
     *                         that was used to establish the connection.
     * @param[out] redirection Returns a redirection address for the endpoint. This value
     *                         is only meaninful if the return status is ER_BUS_ENDPOINT_REDIRECT.
     * @param[in] listener     Optional authentication listener
     *
     * @return
     *      - ER_OK if successful.
     *      = ER_BUS_ENDPOINT_REDIRECT if the endpoint is being redirected.
     *      - An error status otherwise
     */
    QStatus Establish(const qcc::String& authMechanisms, qcc::String& authUsed, qcc::String& redirection, AuthListener* listener = NULL);

    /**
     * Get the GUID of the remote side of a bus-to-bus endpoint.
     *
     * @return GUID of the remote side of a bus-to-bus endpoint.
     */
    const qcc::GUID128& GetRemoteGUID() const;

    /**
     * Set the GUID of the remote side of a bus-to-bus endpoint.
     *
     * @param[in] remoteGUID GUID of the remote side of a bus-to-bus endpoint.
     */
    void SetRemoteGUID(const qcc::GUID128& remoteGUID);

    /**
     * Get the connect spec for this endpoint.
     *
     * @return The connect spec string (may be empty).
     */
    const qcc::String& GetConnectSpec() const;

    /**
     * Set the connect spec for this endpoint.
     *
     * @param[in] connSpec The connect spec string.
     */
    void SetConnectSpec(const qcc::String& connSpec);

    /**
     * Indicate whether this endpoint can receive messages from other devices.
     *
     * @return true iff this endpoint can receive messages from other devices.
     */
    bool AllowRemoteMessages() { return GetFeatures().allowRemote; }

    /**
     * Indicate if this endpoint is for an incoming connection or an outgoing connection.
     *
     * @return 'true' if incoming, 'false' if outgoing.
     */
    bool IsIncomingConnection() const;

    bool IsTrusted() { return GetFeatures().trusted; }
    /**
     * Get the data source for this endpoint
     *
     * @return  The data source for this endpoint.
     */
    qcc::Source& GetSource() { return GetStream(); }

    /**
     * Get the data sink for this endpoint
     *
     * @return  The data sink for this endpoint.
     */
    qcc::Sink& GetSink() { return GetStream(); }

    /**
     * Get the Stream from this endpoint
     *
     * @return The stream for this endpoint.
     */
    qcc::Stream& GetStream();

    /**
     * Set link timeout
     *
     * @param[in,out] idleTimeout Seconds of unresponsive link time (including any transport
     *                            specific idle probes and retries) before link will be shutdown.
     * @return ER_OK if successful.
     */
    virtual QStatus SetLinkTimeout(uint32_t& idleTimeout);

    /**
     * Set Idle timeouts for Router to leaf node connection
     *
     * @param[in,out] reqIdleTimeout    Idle Timeout for the link. i.e. time after which the
     *                                  Routing node must send a DBus ping to the leaf node
     *                                  in case of inactivity.
     * @param[in,out] reqProbeTimeout   Probe timeout. The time from the Routing node sending the DBus
     *                                  ping to the expected response.
     * @return ER_OK if successful.
     */
    virtual QStatus SetIdleTimeouts(uint32_t& reqIdleTimeout, uint32_t& reqProbeTimeout);

    /**
     * Set the endpoint started state variable.  Provided for endpoints that do
     * not use the standard sockets and IODispatch mechanism and therefore
     * override Start() and Stop().
     *
     * @param[in] value Logical value of endpoint started (or not).
     */
    void SetStarted(bool value);

    /**
     * Set the endpoint stopping state variable.  Provided for endpoints that do
     * not use the standard sockets and IODispatch mechanism and therefore
     * override Start() and Stop().
     *
     * @param[in] value Logical value of whether or not the endpoint is stopping.
     */
    void SetStopping(bool value);

    /**
     * Return the features for this BusEndpoint
     *
     * @return   Returns the features for this BusEndpoint.
     */
    Features& GetFeatures();

    /**
     * Return the features for this BusEndpoint
     *
     * @return   Returns the features for this BusEndpoint.
     */
    const Features& GetFeatures() const;

    /**
     * Increment the reference count for this remote endpoint.
     * RemoteEndpoints are stopped when the number of references reaches zero.
     */
    void IncrementRef();

    /**
     * Decrement the reference count for this remote endpoint.
     * RemoteEndpoints are stopped when the number of refereneces reaches zero.
     */
    void DecrementRef();

    /**
     * Called during endpoint establishment to to check if connections are being accepted or
     * redirected to a different address.
     *
     * @return  An empty string or a connect spec for the address to redirect the connection to.
     */
    virtual qcc::String RedirectionAddress() { return ""; }

    /**
     * Get SessionId for endpoint.
     * This is used for BusToBus endpoints only.
     */
    uint32_t GetSessionId();

    /**
     * Set SessionId for endpoint.
     * This is used for BusToBus endpoints only.
     */
    void SetSessionId(uint32_t sessionId);

    /**
     * Return true iff a session route has been set up for this b2b ep.
     */
    bool IsSessionRouteSetUp();

    /**
     * Get the IP address of the remote end.
     * @param[out] ipAddr The IP address of the remote end.
     * @return status ER_OK if the IP address is valid, or error.
     */
    virtual QStatus GetRemoteIp(qcc::String& ipAddr) { return ER_NOT_IMPLEMENTED; };

    /**
     * Get the IP address of the local end.
     * @param[out] ipAddr The IP address of the local end.
     * @return status ER_OK if the IP address is valid, or error.
     */
    virtual QStatus GetLocalIp(qcc::String& ipAddr) { return ER_NOT_IMPLEMENTED; };

  protected:

    /**
     * Set link timeout params (with knowledge of the underlying transport characteristics)
     *
     * @param[in] idleTimeout    Seconds of RX idle time before a ProbeReq will be sent (0 means infinite).
     * @param[in] probeTimeout   Seconds to wait for ProbeAck.
     * @param[in] maxIdleProbes  Number of ProbeReqs to send before delclaring the link dead.
     * @return ER_OK if successful.
     */
    QStatus SetLinkTimeout(uint32_t idleTimeout, uint32_t probeTimeout, uint32_t maxIdleProbes);

    /**
     * Set Idle timeouts for Router to leaf node connection
     *
     * @param[in] reqIdleTimeout   Idle Timeout for the link. i.e. time after which the
     *                             Routing node must send the DBus ping in case of inactivity.
     * @param[in] reqProbeTimeout  Probe timeout. The time from the Routing node sending the
     *                             DBus ping to the expected response.
     * @param[in] numProbes        Number of probes sent before the leaf node is declared dead.
     * @return ER_OK if successful.
     */
    QStatus SetIdleTimeouts(uint32_t reqIdleTimeout, uint32_t reqProbeTimeout, uint32_t numProbes);

    /**
     * Internal callback used to indicate that one of the internal threads (rx or tx) has exited.
     * RemoteEndpoint users should not call this method.
     *
     * @param[in] thread   Thread that exited.
     */
    virtual void ThreadExit(qcc::Thread* thread);

    /**
     * Internal method used to indicate that non-stream-based endpoints have shut down.
     * Like ExitCallback but without the IODispatch bells and whistles.
     */
    void Exit();

    /**
     * Internal method used to indicate that non-stream-based endpoints have no threads
     * running.
     */
    void Exited();

    /**
     * Internal method used to return the value of Probe timeout.
     */
    uint32_t GetProbeTimeout();

    /**
     * Internal method used to return the value of Idle timeout.
     */
    uint32_t GetIdleTimeout();
  private:

    class Internal;
    Internal* internal;    /** All the internal state for a remote endpoint */
    bool minimalEndpoint;  /* Whether or not the remote endpoint manages IODispatch and queues (true) or not (false) */

    /**
     * Copy constructor is undefined.
     */
    _RemoteEndpoint(const _RemoteEndpoint& other);

    /**
     * Assignment operator is undefined - _RemoteEndpoints cannot be assigned.
     */
    _RemoteEndpoint& operator=(const _RemoteEndpoint& other);

    /**
     * Utility function used to generate an idle probe (req or ack)
     *
     * @param[in] isAck   If true, message is a ProbeAck. If false, message is ProbeReq.
     * @param[out] msg     Message.
     * @return   ER_OK if successful
     */
    QStatus GenProbeMsg(bool isAck, Message msg);

    /**
     * Determine if message is a ProbeReq or ProbeAck message.
     *
     * @param[in] msg    Message to examine.
     * @param[out] isAck True if msg ProbeAck.
     * @return  true if message is ProbeReq or ProbeAck.
     */
    bool IsProbeMsg(const Message& msg, bool& isAck);

    /**
     * Internal callback used to indicate that data is available on the File descriptor.
     * RemoteEndpoint users should not call this method.
     *
     * @param[in] source   Source that data is available on.
     * @param[in] isTimedOut         false - if the source event has fired.
     *                           true - if no source event has fired in the specified timeout.
     */
    QStatus ReadCallback(qcc::Source& source, bool isTimedOut);

    /**
     * Internal callback used to indicate that data can be written to File descriptor.
     * RemoteEndpoint users should not call this method.
     *
     * @param[in] source   Source that data is available on.
     * @param[in] isTimedOut         false - if the sink event has fired.
     *                           true - if no sink event has fired in the specified timeout.
     * @return   ER_OK if successful
     */
    QStatus WriteCallback(qcc::Sink& sink, bool isTimedOut);

    /**
     * Internal callback used to indicate that the Stream for this endpoint has been removed
     * from the IODispatch.
     * RemoteEndpoint users should not call this method.
     *
     */
    void ExitCallback();
    /**
     * Send an outgoing message.
     *
     * @param[in] msg   Message to be sent.
     * @param[out] count   Number of messages in txQueue
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    virtual QStatus PushMessageRouter(Message& msg, size_t& count);
    /**
     * Send an outgoing message.
     *
     * @param[in] msg   Message to be sent.
     * @param[out] count   Number of messages in txQueue
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    virtual QStatus PushMessageLeaf(Message& msg, size_t& count);
};

}

#endif

