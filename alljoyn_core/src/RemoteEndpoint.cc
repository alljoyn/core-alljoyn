/**
 * @file
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
#include <qcc/platform.h>

#include <assert.h>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/atomic.h>
#include <qcc/Thread.h>
#include <qcc/SocketStream.h>
#include <qcc/atomic.h>
#include <qcc/IODispatch.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/AllJoynStd.h>

#include "Router.h"
#include "RemoteEndpoint.h"
#include "LocalTransport.h"
#include "AllJoynPeerObj.h"
#include "BusInternal.h"

#ifndef NDEBUG
#include <qcc/time.h>
#endif

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

#define ENDPOINT_IS_DEAD_ALERTCODE  1

class _RemoteEndpoint::Internal {
    friend class _RemoteEndpoint;
  public:

    Internal(BusAttachment& bus, bool incoming, const qcc::String& connectSpec, Stream* stream, const char* threadName, bool isSocket) :
        bus(bus),
        stream(stream),
        txQueue(),
        txWaitQueue(),
        lock(),
        exitCount(0),
        listener(NULL),
        connSpec(connectSpec),
        incoming(incoming),
        processId(-1),
        alljoynVersion(0),
        refCount(0),
        isSocket(isSocket),
        armRxPause(false),
        idleTimeoutCount(0),
        maxIdleProbes(0),
        idleTimeout(0),
        probeTimeout(0),
        threadName(threadName),
        started(false),
        currentReadMsg(bus),
        validateSender(incoming),
        hasRxSessionMsg(false),
        getNextMsg(true),
        currentWriteMsg(bus),
        stopping(false),
        sessionId(0),
        pingCallSerial(0),
        sendTimeout(0),
        maxControlMessages(30),
        numControlMessages(0),
        numDataMessages(0)
    {
    }

    ~Internal() {
    }

    BusAttachment& bus;                      /**< Message bus associated with this endpoint */
    qcc::Stream* stream;                     /**< Stream for this endpoint or NULL if uninitialized */

    std::deque<Message> txQueue;             /**< Transmit message queue */
    std::deque<qcc::Thread*> txWaitQueue;    /**< Threads waiting for txQueue to become not-full */
    qcc::Mutex lock;                         /**< Mutex that protects the txQueue and timeout values */
    int32_t exitCount;                       /**< Number of sub-threads (rx and tx) that have exited (atomically incremented) */

    EndpointListener* listener;              /**< Listener for thread exit and untrusted client start and exit notifications. */

    qcc::String connSpec;                    /**< Connection specification for out-going connections */
    bool incoming;                           /**< Indicates if connection is incoming (true) or outgoing (false) */

    Features features;                       /**< Requested and negotiated features of this endpoint */
    uint32_t processId;                      /**< Process id of the process at the remote end of this endpoint */
    uint32_t alljoynVersion;                 /**< AllJoyn version of the process at the remote end of this endpoint */
    int32_t refCount;                        /**< Number of active users of this remote endpoint */
    bool isSocket;                           /**< True iff this endpoint contains a SockStream as its 'stream' member */
    bool armRxPause;                         /**< Pause Rx after receiving next METHOD_REPLY message */

    uint32_t idleTimeoutCount;               /**< Number of consecutive idle timeouts */
    uint32_t maxIdleProbes;                  /**< Maximum number of missed idle probes before shutdown */
    uint32_t idleTimeout;                    /**< RX idle seconds before sending probe */
    uint32_t probeTimeout;                   /**< Probe timeout in seconds */

    String uniqueName;                       /**< Obtained from EndpointAuth */
    String remoteName;                       /**< Obtained from EndpointAuth */
    GUID128 remoteGUID;                      /**< Obtained from EndpointAuth */
    const char* threadName;                  /**< Transport Name for the Endpoint */
    bool started;                            /**< Is this EP started? */

    Message currentReadMsg;                  /**< The message currently being read for this endpoint */
    bool validateSender;                     /**< If true, the sender field on incomming messages will be overwritten with actual endpoint name */
    bool hasRxSessionMsg;                    /**< true iff this endpoint has previously processed a non-control message */
    bool getNextMsg;                         /**< If true, read the next message from the txQueue */
    Message currentWriteMsg;                 /**< The message currently being read for this endpoint */
    bool stopping;                           /**< Is this EP stopping? */
    uint32_t sessionId;                      /**< SessionId for BusToBus endpoint. (not used for non-B2B endpoints) */
    uint32_t pingCallSerial;                 /**< Serial number of last Heartbeat DBus ping sent */
    uint32_t sendTimeout;                    /**< Send timeout for this endpoint i.e. time after which the Routing node must
                                                  disconnect the remote node if it has not read a message from the link
                                                  in the situation that the send buffer on this end and receive buffer on
                                                  the remote end are full. */
    size_t maxControlMessages;               /**< Number of control messages that can be queued up before disconnecting this endpoint.
                                                  - used on Routing nodes only */
    size_t numControlMessages;               /**< Number of control messages in txQueue - used on Routing nodes only */
    size_t numDataMessages;                  /**< Number of data messages in txQueue - used on Routing nodes only */
};


void _RemoteEndpoint::SetStream(qcc::Stream* s)
{

    if (internal) {
        internal->stream = s;
    }
}


void _RemoteEndpoint::SetStarted(bool value)
{
    if (internal) {
        internal->started = value;
    }
}

void _RemoteEndpoint::SetStopping(bool value)
{
    if (internal) {
        internal->stopping = value;
    }
}

const qcc::String& _RemoteEndpoint::GetUniqueName() const
{
    if (internal) {
        return internal->uniqueName;
    } else {
        return String::Empty;
    }
}

void _RemoteEndpoint::SetUniqueName(const qcc::String& uniqueName)
{
    if (internal) {
        internal->uniqueName = uniqueName;
    }
}

const qcc::String& _RemoteEndpoint::GetRemoteName() const
{
    if (internal) {
        return internal->remoteName;
    } else {
        return String::Empty;
    }
}

void _RemoteEndpoint::SetRemoteName(const qcc::String& remoteName)
{
    if (internal) {
        internal->remoteName = remoteName;
    }
}

const qcc::GUID128& _RemoteEndpoint::GetRemoteGUID() const
{
    if (internal) {
        return internal->remoteGUID;
    } else {
        static const qcc::GUID128 g;
        return g;
    }
}

void _RemoteEndpoint::SetRemoteGUID(const qcc::GUID128& remoteGUID)
{
    if (internal) {
        internal->remoteGUID = remoteGUID;
    }
}


qcc::Stream& _RemoteEndpoint::GetStream()
{
    if (internal) {
        return *(internal->stream);
    } else {
        static Stream stream;
        return stream;
    }
}

const qcc::String&  _RemoteEndpoint::GetConnectSpec() const
{
    if (internal) {
        return internal->connSpec;
    } else {
        return String::Empty;
    }
}

void _RemoteEndpoint::SetConnectSpec(const qcc::String& connSpec)
{
    if (internal) {
        internal->connSpec = connSpec;
    }
}

bool _RemoteEndpoint::IsIncomingConnection() const
{
    if (internal) {
        return internal->incoming;
    } else {
        return false;
    }
}

_RemoteEndpoint::Features&  _RemoteEndpoint::GetFeatures()
{
    if (internal) {
        return internal->features;
    } else {
        static Features f;
        return f;
    }
}

const _RemoteEndpoint::Features&  _RemoteEndpoint::GetFeatures() const
{
    if (internal) {
        return internal->features;
    } else {
        static Features f;
        return f;
    }
}

QStatus _RemoteEndpoint::Establish(const qcc::String& authMechanisms, qcc::String& authUsed, qcc::String& redirection, AuthListener* listener)
{
    QStatus status = ER_OK;

    if (!internal || minimalEndpoint) {
        status = ER_BUS_NO_ENDPOINT;
    } else {
        RemoteEndpoint rep = RemoteEndpoint::wrap(this);
        EndpointAuth auth(internal->bus, rep, internal->incoming);

        status = auth.Establish(authMechanisms, authUsed, redirection, listener);
        if (status == ER_OK) {
            internal->uniqueName = auth.GetUniqueName();
            internal->remoteName = auth.GetRemoteName();
            internal->remoteGUID = auth.GetRemoteGUID();
            internal->features.protocolVersion = auth.GetRemoteProtocolVersion();
            internal->features.trusted = (authUsed != "ANONYMOUS");
            internal->features.nameTransfer = (SessionOpts::NameTransferType)auth.GetNameTransfer();

        }
    }
    return status;
}

QStatus _RemoteEndpoint::SetLinkTimeout(uint32_t& idleTimeout)
{
    if (internal) {
        internal->idleTimeout = 0;
    }
    return ER_OK;
}

/* Endpoint constructor */
_RemoteEndpoint::_RemoteEndpoint(BusAttachment& bus,
                                 bool incoming,
                                 const qcc::String& connectSpec,
                                 Stream* stream,
                                 const char* threadName,
                                 bool isSocket,
                                 bool minimal) :
    _BusEndpoint(ENDPOINT_TYPE_REMOTE), minimalEndpoint(minimal)
{
    internal = new Internal(bus, incoming, connectSpec, stream, threadName, isSocket);
}

_RemoteEndpoint::~_RemoteEndpoint()
{
    if (internal) {
        Stop();
        Join();
        delete internal;
        internal = NULL;
    }
}
QStatus _RemoteEndpoint::UntrustedClientStart() {

    /* If a transport expects to accept untrusted clients, it MUST implement the
     * UntrustedClientStart and UntrustedClientExit methods and call SetListener
     * before making a call to _RemoteEndpoint::Establish(). So assert if the
     * internal->listener is NULL.
     * Note: It is required to set the listener only on the accepting end
     * i.e. for incoming endpoints.
     */
    assert(internal);
    assert(internal->listener);
    return internal->listener->UntrustedClientStart();
}
QStatus _RemoteEndpoint::SetLinkTimeout(uint32_t idleTimeout, uint32_t probeTimeout, uint32_t maxIdleProbes)
{
    QCC_DbgTrace(("_RemoteEndpoint::SetLinkTimeout(%u, %u, %u) for %s", idleTimeout, probeTimeout, maxIdleProbes, GetUniqueName().c_str()));

    if (!internal || minimalEndpoint) {
        return ER_BUS_NO_ENDPOINT;
    }

    if (GetRemoteProtocolVersion() >= 3) {
        internal->lock.Lock(MUTEX_CONTEXT);
        internal->idleTimeout = idleTimeout;
        internal->probeTimeout = probeTimeout;
        internal->maxIdleProbes = maxIdleProbes;
        IODispatch& iodispatch = internal->bus.GetInternal().GetIODispatch();
        uint32_t timeout = (internal->idleTimeoutCount == 0) ? internal->idleTimeout : internal->probeTimeout;

        QStatus status = iodispatch.EnableTimeoutCallback(internal->stream, timeout);
        internal->lock.Unlock(MUTEX_CONTEXT);
        return status;
    } else {
        return ER_ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT;
    }
}
QStatus _RemoteEndpoint::SetIdleTimeouts(uint32_t& idleTimeout, uint32_t& probeTimeout)
{
    if (internal) {
        internal->idleTimeout = 0;
        internal->probeTimeout = 0;
    }
    return ER_OK;
}
QStatus _RemoteEndpoint::SetIdleTimeouts(uint32_t idleTimeout, uint32_t probeTimeout, uint32_t maxIdleProbes)
{
    QCC_DbgPrintf(("_RemoteEndpoint::SetIdleTimeouts(%u, %u, %u) for %s", idleTimeout, probeTimeout, maxIdleProbes, GetUniqueName().c_str()));

    if (!internal || minimalEndpoint) {
        return ER_BUS_NO_ENDPOINT;
    }

    internal->lock.Lock(MUTEX_CONTEXT);
    internal->idleTimeout = idleTimeout;
    internal->probeTimeout = probeTimeout;
    internal->maxIdleProbes = maxIdleProbes;
    IODispatch& iodispatch = internal->bus.GetInternal().GetIODispatch();
    internal->idleTimeoutCount = 0;

    QStatus status = iodispatch.EnableTimeoutCallback(internal->stream, internal->idleTimeout);
    internal->lock.Unlock(MUTEX_CONTEXT);
    return status;

}
uint32_t _RemoteEndpoint::GetProbeTimeout()
{
    if (internal) {
        return internal->probeTimeout;
    } else {
        return 0;
    }
}

uint32_t _RemoteEndpoint::GetIdleTimeout()
{
    if (internal) {
        return internal->idleTimeout;
    } else {
        return 0;
    }
}

QStatus _RemoteEndpoint::Start()
{
    assert(internal);

    if (minimalEndpoint) {
        if (internal->features.isBusToBus) {
            endpointType = ENDPOINT_TYPE_BUS2BUS;
        }
        return ER_OK;
    }

    assert(internal->stream);
    QCC_DbgPrintf(("_RemoteEndpoint::Start(%s, isBusToBus = %s, allowRemote = %s)", GetUniqueName().c_str(),
                   internal->features.isBusToBus ? "true" : "false",
                   internal->features.allowRemote ? "true" : "false"));
    QStatus status;
    internal->started = true;
    Router& router = internal->bus.GetInternal().GetRouter();
    IODispatch& iodispatch = internal->bus.GetInternal().GetIODispatch();

    if (internal->features.isBusToBus) {
        endpointType = ENDPOINT_TYPE_BUS2BUS;
    }

    /* Set the send timeout for this endpoint
     * Note that this is set to zero even though the actual send timeout is different.
     * This is because we want non-blocking functionality from the underlying stream.
     * Send timeout is implemented using a timedout WriteCallback from IODispatch.
     */
    internal->stream->SetSendTimeout(0);

    /* Endpoint needs to be wrapped before we can use it */
    RemoteEndpoint me = RemoteEndpoint::wrap(this);

    BusEndpoint bep = BusEndpoint::cast(me);

    /* Register endpoint with IODispatch - enable write, disable read */
    status = iodispatch.StartStream(internal->stream, this, this, this, false, true);
    if (status == ER_OK) {
        /* Register endpoint with router */
        status = router.RegisterEndpoint(bep);

        if (status != ER_OK) {
            /* Failed to register with iodispatch */
            internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
            router.UnregisterEndpoint(this->GetUniqueName(), this->GetEndpointType());
        }
    }

    if (status == ER_OK) {
        /* Enable read for this endpoint */
        status = internal->bus.GetInternal().GetIODispatch().EnableReadCallback(internal->stream);
        if (status != ER_OK) {
            /* Failed to start read with iodispatch */
            internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
            router.UnregisterEndpoint(this->GetUniqueName(), this->GetEndpointType());
        }
    }
    if (status != ER_OK) {
        Invalidate();
        internal->started = false;
    }

    return status;
}
QStatus _RemoteEndpoint::Start(uint32_t idleTimeout, uint32_t probeTimeout, uint32_t numProbes, uint32_t sendTimeout)
{
    QStatus status = Start();
    if (status == ER_OK && endpointType == ENDPOINT_TYPE_REMOTE) {
        /* Set idle timeouts for leaf nodes only */
        status = SetIdleTimeouts(idleTimeout, probeTimeout, numProbes);
    }
    internal->sendTimeout = sendTimeout;
    internal->maxControlMessages = sendTimeout * MAX_CONTROL_MSGS_PER_SECOND;
    if (status != ER_OK) {
        Invalidate();
        internal->started = false;
    }
    return status;
}
void _RemoteEndpoint::SetListener(EndpointListener* listener)
{
    if (internal) {
        internal->listener = listener;
    }

}

QStatus _RemoteEndpoint::Stop(void)
{
    QStatus ret = ER_OK;
    /* Ensure the endpoint is valid */
    if (!internal) {
        return ER_BUS_NO_ENDPOINT;
    }
    QCC_DbgPrintf(("_RemoteEndpoint::Stop(%s) called", GetUniqueName().c_str()));

    if (minimalEndpoint == false) {
        /*
         * Make the endpoint invalid - this prevents any further use of the endpoint that might delay
         * its ultimate demise.
         */
        if (internal->started) {
            ret = internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
        }
    }
    internal->stopping = true;
    Invalidate();
    return ret;

}

QStatus _RemoteEndpoint::StopAfterTxEmpty(uint32_t maxWaitMs)
{
    QStatus status;
    /* Init wait time */
    uint32_t startTime = maxWaitMs ? GetTimestamp() : 0;

    /* Ensure the endpoint is valid */
    if (!internal || minimalEndpoint) {
        return ER_BUS_NO_ENDPOINT;
    }

    /* Wait for txqueue to empty before triggering stop */
    internal->lock.Lock(MUTEX_CONTEXT);
    while (true) {
        if (internal->txQueue.empty() || (maxWaitMs && (qcc::GetTimestamp() > (startTime + maxWaitMs)))) {
            status = Stop();
            break;
        } else {
            internal->lock.Unlock(MUTEX_CONTEXT);
            qcc::Sleep(5);
            internal->lock.Lock(MUTEX_CONTEXT);
        }
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus _RemoteEndpoint::PauseAfterRxReply()
{
    if (internal) {
        internal->armRxPause = true;
        return ER_OK;
    } else {
        return ER_BUS_NO_ENDPOINT;
    }
}

QStatus _RemoteEndpoint::Join(void)
{
/* Ensure the endpoint is valid */
    QCC_DbgPrintf(("_RemoteEndpoint::Join(%s) called", GetUniqueName().c_str()));

    if (!internal) {
        return ER_BUS_NO_ENDPOINT;
    }
    if (internal->started) {
        while (internal->exitCount < 1) {
            qcc::Sleep(5);
        }
        internal->started = false;
    }
    return ER_OK;
}

void _RemoteEndpoint::ThreadExit(Thread* thread)
{
    if (minimalEndpoint) {
        return;
    }
    /* This is notification of a txQueue waiter has died. Remove him */
    internal->lock.Lock(MUTEX_CONTEXT);
    deque<Thread*>::iterator it = find(internal->txWaitQueue.begin(), internal->txWaitQueue.end(), thread);
    if (it != internal->txWaitQueue.end()) {
        (*it)->RemoveAuxListener(this);
        internal->txWaitQueue.erase(it);
    }
    internal->lock.Unlock(MUTEX_CONTEXT);

    return;
}

static inline bool IsControlMessage(Message& msg)
{
    const char* sender = msg->GetSender();
    size_t offset = ::strlen(sender);
    if (offset >= 2) {
        offset -= 2;
    }
    return (::strcmp(sender + offset, ".1") == 0) ? true : false;
}

void _RemoteEndpoint::Exit()
{
    QCC_DbgTrace(("_RemoteEndpoint::Exit(%s)", GetUniqueName().c_str()));

    assert(minimalEndpoint == true && "_RemoteEndpoint::Exit(): You should have had ExitCallback() called for you!");
    /* Ensure the endpoint is valid */
    if (!internal) {
        return;
    }

    Invalidate();

    RemoteEndpoint rep = RemoteEndpoint::wrap(this);
    /* Un-register this remote endpoint from the router */
    internal->bus.GetInternal().GetRouter().UnregisterEndpoint(this->GetUniqueName(), this->GetEndpointType());

    if (internal->listener) {
        internal->listener->EndpointExit(rep);
        internal->listener = NULL;
    }

    internal->exitCount = 1;
}

void _RemoteEndpoint::Exited()
{
    QCC_DbgTrace(("_RemoteEndpoint::Exited(%s)", GetUniqueName().c_str()));
    if (internal) {
        internal->exitCount = 1;
    }
}

void _RemoteEndpoint::ExitCallback()
{
    assert(minimalEndpoint == false && "_RemoteEndpoint::ExitCallback(): Where did a callback come from if no thread?");
    /* Ensure the endpoint is valid */
    if (!internal) {
        return;
    }
    /* Alert any threads that are on the wait queue */
    internal->lock.Lock(MUTEX_CONTEXT);
    deque<Thread*>::iterator it = internal->txWaitQueue.begin();
    while (it != internal->txWaitQueue.end()) {
        (*it++)->Alert(ENDPOINT_IS_DEAD_ALERTCODE);
    }

    internal->lock.Unlock(MUTEX_CONTEXT);
    RemoteEndpoint rep = RemoteEndpoint::wrap(this);
    /* Un-register this remote endpoint from the router */
    internal->bus.GetInternal().GetRouter().UnregisterEndpoint(this->GetUniqueName(), this->GetEndpointType());
    if (internal->incoming && !internal->features.trusted && !internal->features.isBusToBus) {
        /* If a transport expects to accept untrusted clients, it MUST implement the
         * UntrustedClientStart and UntrustedClientExit methods and call SetListener
         * before making a call to _RemoteEndpoint::Establish(). Since the ExitCallback
         * can occur only after _RemoteEndpoint::Establish() is successful, we assert
         * if the internal->listener is NULL.
         */
        assert(internal->listener);
        internal->listener->UntrustedClientExit();
    }

    if (internal->listener) {
        internal->listener->EndpointExit(rep);
        internal->listener = NULL;
    }
    /* Since endpoints are managed, the endpoint destructor will not be called until
     * all the references to the endpoint are released. This means that the SocketStream
     * destructor will also not be called until then.
     * Explicitly close the socketstream i.e. destroy the source and sink events and
     * close the associated socket here.
     */

    internal->stream->Close();
    internal->exitCount = 1;
}

QStatus _RemoteEndpoint::ReadCallback(qcc::Source& source, bool isTimedOut)
{
    assert(minimalEndpoint == false && "_RemoteEndpoint::ReadCallback(): Where did a callback come from if no thread?");
    /* Remote endpoints can be invalid if they were created with the default
     * constructor or being torn down. Return ER_BUS_NO_ENDPOINT only if the
     * endpoint was created with the default constructor. i.e. internal=NULL
     */
    if (!internal) {
        return ER_BUS_NO_ENDPOINT;
    }

    QStatus status;

    const bool bus2bus = ENDPOINT_TYPE_BUS2BUS == GetEndpointType();
    Router& router = internal->bus.GetInternal().GetRouter();
    RemoteEndpoint rep = RemoteEndpoint::wrap(this);
    if (!isTimedOut) {
        status = ER_OK;
        while (status == ER_OK) {

            status = internal->currentReadMsg->ReadNonBlocking(rep, (internal->validateSender && !bus2bus));
            if (status == ER_OK) {
                /* Message read complete.Proceed to unmarshal it. */
                Message msg = internal->currentReadMsg;
                status = msg->Unmarshal(rep, (internal->validateSender && !bus2bus));

                switch (status) {
                case ER_OK:
                    internal->idleTimeoutCount = 0;
                    bool isAck;
                    if ((internal->pingCallSerial != 0) && (msg->GetType() == MESSAGE_METHOD_RET) && (internal->pingCallSerial == msg->GetReplySerial())) {
                        /* This is a response to the DBus ping sent from RN to LN. Consume the reply quietly. */
                        internal->pingCallSerial = 0;
                    } else if (IsProbeMsg(msg, isAck)) {
                        QCC_DbgPrintf(("%s: Received %s\n", GetUniqueName().c_str(), isAck ? "ProbeAck" : "ProbeReq"));
                        if (!isAck) {
                            /* Respond to probe request */
                            Message probeMsg(internal->bus);
                            status = GenProbeMsg(true, probeMsg);
                            if (status == ER_OK) {
                                status = PushMessage(probeMsg);
                            }
                            QCC_DbgPrintf(("%s: Sent ProbeAck (%s)\n", GetUniqueName().c_str(), QCC_StatusText(status)));
                        }
                    } else {
                        BusEndpoint bep  = BusEndpoint::cast(rep);

                        status = router.PushMessage(msg, bep);
                        if (status != ER_OK) {
                            /*
                             * There are five cases where a failure to push a message to the router is ok:
                             *
                             * 1) The message received did not match the expected signature.
                             * 2) The message was a method reply that did not match up to a method call.
                             * 3) A daemon is pushing the message to a connected client or service.
                             * 4) Pushing a message to an endpoint that has closed.
                             * 5) Pushing the first non-control message of a new session (must wait for route to be fully setup)
                             *
                             */

                            if (status == ER_BUS_NO_ROUTE) {

                                int retries = 20;
                                while (!internal->stopping && (status == ER_BUS_NO_ROUTE) && !internal->hasRxSessionMsg && retries--) {
                                    qcc::Sleep(10);
                                    status = router.PushMessage(msg, bep);
                                }
                            }
                            if ((router.IsDaemon() && !bus2bus) || (status == ER_BUS_SIGNATURE_MISMATCH) || (status == ER_BUS_UNMATCHED_REPLY_SERIAL) || (status == ER_BUS_ENDPOINT_CLOSING)) {
                                QCC_DbgHLPrintf(("%s: Discarding %s: %s", GetUniqueName().c_str(), msg->Description().c_str(), QCC_StatusText(status)));
                                status = ER_OK;
                            }
                        }
                        /* Update haxRxSessionMessage */
                        if ((status == ER_OK) && !internal->hasRxSessionMsg && !IsControlMessage(msg)) {
                            internal->hasRxSessionMsg = true;
                        }
                    }
                    break;

                case ER_BUS_CANNOT_EXPAND_MESSAGE:
                    internal->idleTimeoutCount = 0;
                    /*
                     * The message could not be expanded so pass it the peer object to request the expansion
                     * rule from the endpoint that sent it.
                     */
                    status = internal->bus.GetInternal().GetLocalEndpoint()->GetPeerObj()->RequestHeaderExpansion(msg, rep);
                    if ((status != ER_OK) && router.IsDaemon()) {
                        QCC_LogError(status, ("%s: Discarding %s", GetUniqueName().c_str(), msg->Description().c_str()));
                        status = ER_OK;
                    }
                    break;

                case ER_BUS_TIME_TO_LIVE_EXPIRED:
                    internal->idleTimeoutCount = 0;
                    QCC_DbgHLPrintf(("%s: TTL expired discarding %s", GetUniqueName().c_str(), msg->Description().c_str()));
                    status = ER_OK;
                    break;

                case ER_BUS_INVALID_HEADER_SERIAL:
                    internal->idleTimeoutCount = 0;
                    /*
                     * Ignore invalid serial numbers for unreliable messages or broadcast messages that come from
                     * bus2bus endpoints as these can be delivered out-of-order or repeated.
                     *
                     * Ignore control messages (i.e. messages targeted at the bus controller)
                     * TODO - need explanation why this is neccessary.
                     *
                     * In all other cases an invalid serial number cause the connection to be dropped.
                     */
                    if (msg->IsUnreliable() || msg->IsBroadcastSignal() || IsControlMessage(msg)) {
                        QCC_DbgHLPrintf(("%s: Invalid serial discarding %s", GetUniqueName().c_str(), msg->Description().c_str()));
                        status = ER_OK;
                    } else {
                        QCC_LogError(status, ("%s: Invalid serial %s", GetUniqueName().c_str(), msg->Description().c_str()));
                    }
                    break;

                case ER_ALERTED_THREAD:
                    status = ER_OK;
                    break;

                default:
                    break;
                }

                /* Check pause condition. Block until stopped */
                if (internal->armRxPause && internal->started && (msg->GetType() == MESSAGE_METHOD_RET)) {
                    status = ER_BUS_ENDPOINT_CLOSING;
                    internal->bus.GetInternal().GetIODispatch().DisableReadCallback(internal->stream);
                    return ER_OK;
                }
                if (status == ER_OK) {
                    internal->currentReadMsg = Message(internal->bus);
                }
            }
        }
        if (status == ER_TIMEOUT) {
            internal->lock.Lock(MUTEX_CONTEXT);
            internal->bus.GetInternal().GetIODispatch().EnableReadCallback(internal->stream, internal->idleTimeout);
            internal->lock.Unlock(MUTEX_CONTEXT);
        } else {

            if ((status != ER_STOPPING_THREAD) && (status != ER_SOCK_OTHER_END_CLOSED) && (status != ER_BUS_STOPPING)) {
                QCC_DbgPrintf(("Endpoint Rx failed (%s): %s", GetUniqueName().c_str(), QCC_StatusText(status)));
            }
            /* On an unexpected disconnect save the status that cause the thread exit */
            if (disconnectStatus == ER_OK) {
                disconnectStatus = (status == ER_STOPPING_THREAD) ? ER_OK : status;
            }
            Invalidate();
            internal->stopping = true;
            internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
        }
    } else {

        /* This is a timeout alarm, try to send a probe message if maximum idle
         * probe attempts has not been reached.
         */
        if (internal->idleTimeoutCount++ < internal->maxIdleProbes) {
            if (endpointType == ENDPOINT_TYPE_BUS2BUS) {

                Message probeMsg(internal->bus);
                status = GenProbeMsg(false, probeMsg);
                if (status == ER_OK) {
                    PushMessage(probeMsg);
                }
                QCC_DbgPrintf(("%s: Sent ProbeReq (%s)\n", GetUniqueName().c_str(), QCC_StatusText(status)));

            } else {
                Message msg(internal->bus);
                status = msg->CallMsg("",
                                      GetUniqueName().c_str(),
                                      0,
                                      "/",
                                      org::freedesktop::DBus::Peer::InterfaceName,
                                      "Ping",
                                      NULL,
                                      0, 0);
                internal->pingCallSerial = msg->GetCallSerial();

                if (status == ER_OK) {
                    PushMessage(msg);
                }
                QCC_DbgPrintf(("%s: Sent DBus ping (%s)\n", GetUniqueName().c_str(), QCC_StatusText(status)));

            }
            internal->lock.Lock(MUTEX_CONTEXT);
            uint32_t timeout = (internal->idleTimeoutCount == 0) ? internal->idleTimeout : internal->probeTimeout;
            internal->bus.GetInternal().GetIODispatch().EnableReadCallback(internal->stream, timeout);
            internal->lock.Unlock(MUTEX_CONTEXT);

        } else {
            QCC_DbgPrintf(("%s: Maximum number of idle probe (%d) attempts reached", GetUniqueName().c_str(), internal->maxIdleProbes));
            /* On an unexpected disconnect save the status that cause the thread exit */
            if (disconnectStatus == ER_OK) {
                disconnectStatus = ER_TIMEOUT;
            }

            QCC_LogError(ER_TIMEOUT, ("Endpoint Rx timed out (%s)", GetUniqueName().c_str()));
            status = ER_BUS_ENDPOINT_CLOSING;
            Invalidate();
            internal->stopping = true;
            internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
        }
    }
    return status;
}
/* Note: isTimedOut indicates that this is a timeout alarm. This is used to implement
 * the SendTimeout functionality.
 */
QStatus _RemoteEndpoint::WriteCallback(qcc::Sink& sink, bool isTimedOut)
{
    assert(minimalEndpoint == false && "_RemoteEndpoint::WriteCallback(): Where did a callback come from if no thread?");

    /* Remote endpoints can be invalid if they were created with the default
     * constructor or being torn down. Return ER_BUS_NO_ENDPOINT only if the
     * endpoint was created with the default constructor. i.e. internal=NULL
     */
    if (!internal) {
        return ER_BUS_NO_ENDPOINT;
    }
    if (isTimedOut) {
        /* On an unexpected disconnect save the status that cause the thread exit */
        if (disconnectStatus == ER_OK) {
            disconnectStatus = ER_TIMEOUT;
        }

        QCC_LogError(ER_TIMEOUT, ("Endpoint Tx timed out (%s)", GetUniqueName().c_str()));
        Invalidate();
        internal->stopping = true;
        internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
        return ER_BUS_ENDPOINT_CLOSING;
    }
    QStatus status = ER_OK;
    while (status == ER_OK) {
        if (internal->getNextMsg) {
            internal->lock.Lock(MUTEX_CONTEXT);
            if (!internal->txQueue.empty()) {
                /* Make a deep copy of the message since there is state information inside the message.
                 * Each copy of the message could be in different write state.
                 */
                internal->currentWriteMsg = Message(internal->txQueue.back(), true);
                internal->getNextMsg = false;
                internal->lock.Unlock(MUTEX_CONTEXT);
            } else {

                internal->bus.GetInternal().GetIODispatch().DisableWriteCallback(internal->stream);
                internal->lock.Unlock(MUTEX_CONTEXT);
                return ER_OK;
            }
        }
        /* Deliver message */
        RemoteEndpoint rep = RemoteEndpoint::wrap(this);
        status = internal->currentWriteMsg->DeliverNonBlocking(rep);
        /* Report authorization failure as a security violation */
        if (status == ER_BUS_NOT_AUTHORIZED) {
            internal->bus.GetInternal().GetLocalEndpoint()->GetPeerObj()->HandleSecurityViolation(internal->currentWriteMsg, status);
            /*
             * Clear the error after reporting the security violation otherwise we will exit
             * this thread which will shut down the endpoint.
             */
            status = ER_OK;
        }
        if (status == ER_OK) {

            /* Message has been successfully delivered. i.e. PushBytes is complete
             */
            internal->lock.Lock(MUTEX_CONTEXT);
            internal->txQueue.pop_back();
            internal->getNextMsg = true;
            if (internal->bus.GetInternal().GetRouter().IsDaemon()) {
                if (IsControlMessage(internal->currentWriteMsg)) {
                    internal->numControlMessages--;
                } else {
                    internal->numDataMessages--;
                }
            }
            /* Alert the first one in the txWaitQueue */
            if (0 < internal->txWaitQueue.size()) {
                Thread* wakeMe = internal->txWaitQueue.back();
                status = wakeMe->Alert();
                if (ER_OK != status) {
                    QCC_LogError(status, ("Failed to alert thread blocked on full tx queue"));
                }
            }
            internal->lock.Unlock(MUTEX_CONTEXT);
        }
    }

    if (status == ER_TIMEOUT) {
        /* Timed-out in the middle of a message write. */
        internal->lock.Lock(MUTEX_CONTEXT);

        /* Set writecallback after sendtimeout. */
        internal->bus.GetInternal().GetIODispatch().EnableWriteCallback(internal->stream, internal->sendTimeout);
        internal->lock.Unlock(MUTEX_CONTEXT);
    } else if (status != ER_OK) {
        /* On an unexpected disconnect save the status that cause the thread exit */
        if (disconnectStatus == ER_OK) {
            disconnectStatus = (status == ER_STOPPING_THREAD) ? ER_OK : status;
        }
        if ((status != ER_OK) && (status != ER_STOPPING_THREAD) && (status != ER_SOCK_OTHER_END_CLOSED) && (status != ER_BUS_STOPPING)) {
            QCC_LogError(status, ("Endpoint Tx failed (%s)", GetUniqueName().c_str()));
        }

        Invalidate();
        internal->stopping = true;
        internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
    }
    return status;
}
QStatus _RemoteEndpoint::PushMessageRouter(Message& msg, size_t& count)
{
    QStatus status = ER_OK;
    static const size_t MAX_DATA_MESSAGES = 1;

    internal->lock.Lock(MUTEX_CONTEXT);
    count = internal->txQueue.size();
    bool wasEmpty = (count == 0);

    if (IsControlMessage(msg)) {

        if (internal->numControlMessages < internal->maxControlMessages) {
            internal->txQueue.push_front(msg);
            internal->numControlMessages++;
            if (wasEmpty) {
                internal->bus.GetInternal().GetIODispatch().EnableWriteCallbackNow(internal->stream);
            }
            internal->lock.Unlock(MUTEX_CONTEXT);
        } else {
            internal->lock.Unlock(MUTEX_CONTEXT);
            Invalidate();
            internal->stopping = true;
            internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
            QCC_LogError(ER_BUS_ENDPOINT_CLOSING, ("Endpoint Tx failed (%s)", GetUniqueName().c_str()));
            status = ER_BUS_ENDPOINT_CLOSING;
        }

    } else {
        /* If the txWaitQueue is not empty, dont queue the message.
         * There are other threads that are blocked trying to send a message to
         * this RemoteEndpoint
         */
        if ((internal->numDataMessages < MAX_DATA_MESSAGES) && (internal->txWaitQueue.empty())) {
            internal->txQueue.push_front(msg);
            internal->numDataMessages++;
        } else {
            /* This thread will have to wait for room in the queue */
            Thread* thread = Thread::GetThread();
            assert(thread);

            thread->AddAuxListener(this);
            internal->txWaitQueue.push_front(thread);

            while (true) {
                /* Remove a queue entry whose TTLs is expired.
                 * Only threads that are the head of the txWaitqueue will purge this deque
                 * and enqueue new messages to the txQueue.
                 * This is to ensure that the original order of calling of PushMessage
                 * is preserved.
                 */
                uint32_t maxWait = Event::WAIT_FOREVER;
                if (internal->txWaitQueue.back() == thread) {
                    deque<Message>::iterator it = internal->txQueue.begin();
                    while (it != internal->txQueue.end()) {
                        uint32_t expMs;
                        if ((*it)->IsExpired(&expMs)) {
                            if (IsControlMessage(internal->currentWriteMsg)) {
                                internal->numControlMessages--;
                            } else {
                                internal->numDataMessages--;
                            }

                            internal->txQueue.erase(it);
                            break;
                        } else {
                            ++it;
                            if (maxWait == Event::WAIT_FOREVER) {
                                maxWait = expMs;
                            } else {
                                maxWait = (std::min)(maxWait, expMs);
                            }
                        }
                    }

                    if (internal->numDataMessages < MAX_DATA_MESSAGES) {
                        count = internal->txQueue.size();
                        /* Check queue wasn't drained while we were waiting */
                        if (internal->txQueue.size() == 0) {
                            wasEmpty = true;
                        }
                        internal->txQueue.push_front(msg);
                        internal->numDataMessages++;

                        status = ER_OK;
                        break;
                    }
                }
                internal->lock.Unlock(MUTEX_CONTEXT);
                status = Event::Wait(Event::neverSet, maxWait);
                internal->lock.Lock(MUTEX_CONTEXT);
                /* Reset alert status */
                if (ER_ALERTED_THREAD == status) {
                    if (thread->GetAlertCode() == ENDPOINT_IS_DEAD_ALERTCODE) {
                        status = ER_BUS_ENDPOINT_CLOSING;
                    }
                    thread->GetStopEvent().ResetEvent();
                }

                if (internal->stopping) {
                    status = ER_BUS_ENDPOINT_CLOSING;
                }
                if ((ER_OK != status) && (ER_ALERTED_THREAD != status) && (ER_TIMEOUT != status)) {
                    break;
                }

            }
            /* Remove thread from wait queue. */
            thread->RemoveAuxListener(this);
            deque<Thread*>::iterator eit = find(internal->txWaitQueue.begin(), internal->txWaitQueue.end(), thread);
            if (eit != internal->txWaitQueue.end()) {
                internal->txWaitQueue.erase(eit);
            }

            /* Alert the first one in the txWaitQueue */
            if (0 < internal->txWaitQueue.size()) {
                Thread* wakeMe = internal->txWaitQueue.back();
                status = wakeMe->Alert();
                if (ER_OK != status) {
                    QCC_LogError(status, ("Failed to alert thread blocked on full tx queue"));
                }
            }

        }


        if (wasEmpty && (status == ER_OK)) {
            internal->bus.GetInternal().GetIODispatch().EnableWriteCallbackNow(internal->stream);
        }
        internal->lock.Unlock(MUTEX_CONTEXT);

    }

    return status;
}
QStatus _RemoteEndpoint::PushMessageLeaf(Message& msg, size_t& count)
{
    static const size_t MAX_TX_QUEUE_SIZE = 1;

    QStatus status = ER_OK;
    internal->lock.Lock(MUTEX_CONTEXT);
    count = internal->txQueue.size();
    bool wasEmpty = (count == 0);
    /* If the txWaitQueue is not empty, dont queue the message.
     * There are other threads that are blocked trying to send a message to
     * this RemoteEndpoint
     */
    if ((count < MAX_TX_QUEUE_SIZE) && (internal->txWaitQueue.empty())) {
        internal->txQueue.push_front(msg);
    } else {
        /* This thread will have to wait for room in the queue */
        Thread* thread = Thread::GetThread();
        assert(thread);

        thread->AddAuxListener(this);
        internal->txWaitQueue.push_front(thread);

        while (true) {
            /* Remove a queue entry whose TTLs is expired.
             * Only threads that are the head of the txWaitqueue will purge this deque
             * and enqueue new messages to the txQueue.
             * This is to ensure that the original order of calling of PushMessage
             * is preserved.
             */
            uint32_t maxWait = Event::WAIT_FOREVER;
            if (internal->txWaitQueue.back() == thread) {
                deque<Message>::iterator it = internal->txQueue.begin();
                while (it != internal->txQueue.end()) {
                    uint32_t expMs;
                    if ((*it)->IsExpired(&expMs)) {
                        internal->txQueue.erase(it);
                        break;
                    } else {
                        ++it;
                        if (maxWait == Event::WAIT_FOREVER) {
                            maxWait = expMs;
                        } else {
                            maxWait = (std::min)(maxWait, expMs);
                        }
                    }
                }

                if (internal->txQueue.size() < MAX_TX_QUEUE_SIZE) {
                    count = internal->txQueue.size();
                    /* Check queue wasn't drained while we were waiting */
                    if (internal->txQueue.size() == 0) {
                        wasEmpty = true;
                    }
                    internal->txQueue.push_front(msg);
                    status = ER_OK;
                    break;
                }
            }
            internal->lock.Unlock(MUTEX_CONTEXT);
            status = Event::Wait(Event::neverSet, maxWait);
            internal->lock.Lock(MUTEX_CONTEXT);
            /* Reset alert status */
            if (ER_ALERTED_THREAD == status) {
                if (thread->GetAlertCode() == ENDPOINT_IS_DEAD_ALERTCODE) {
                    status = ER_BUS_ENDPOINT_CLOSING;
                }
                thread->GetStopEvent().ResetEvent();
            }

            if (internal->stopping) {
                status = ER_BUS_ENDPOINT_CLOSING;
            }
            if ((ER_OK != status) && (ER_ALERTED_THREAD != status) && (ER_TIMEOUT != status)) {
                break;
            }

        }
        /* Remove thread from wait queue. */
        thread->RemoveAuxListener(this);
        deque<Thread*>::iterator eit = find(internal->txWaitQueue.begin(), internal->txWaitQueue.end(), thread);
        if (eit != internal->txWaitQueue.end()) {
            internal->txWaitQueue.erase(eit);
        }

        /* Alert the first one in the txWaitQueue */
        if (0 < internal->txWaitQueue.size()) {
            Thread* wakeMe = internal->txWaitQueue.back();
            status = wakeMe->Alert();
            if (ER_OK != status) {
                QCC_LogError(status, ("Failed to alert thread blocked on full tx queue"));
            }
        }


    }


    if (wasEmpty && (status == ER_OK)) {
        internal->bus.GetInternal().GetIODispatch().EnableWriteCallbackNow(internal->stream);
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
    return status;
}
QStatus _RemoteEndpoint::PushMessage(Message& msg)
{
    assert(minimalEndpoint == false && "_RemoteEndpoint::PushMessage(): Unexpected PushMessage with no queues");
    QCC_DbgTrace(("RemoteEndpoint::PushMessage %s (serial=%d)", GetUniqueName().c_str(), msg->GetCallSerial()));

    QStatus status = ER_OK;
    size_t count;
    /* Remote endpoints can be invalid if they were created with the default
     * constructor or being torn down. Return ER_BUS_NO_ENDPOINT only if the
     * endpoint was created with the default constructor. i.e. internal=NULL
     */
    if (!internal) {
        return ER_BUS_NO_ENDPOINT;
    }
    /*
     * Don't continue if this endpoint is in the process of being closed
     * Otherwise we risk deadlock when sending NameOwnerChanged signal to
     * this dying endpoint
     */
    if (internal->stopping) {
        return ER_BUS_ENDPOINT_CLOSING;
    }
    if (internal->bus.GetInternal().GetRouter().IsDaemon()) {
        status = PushMessageRouter(msg, count);
    } else {
        status = PushMessageLeaf(msg, count);
    }
#ifndef NDEBUG
#undef QCC_MODULE
#define QCC_MODULE "TXSTATS"
    static uint32_t lastTime = 0;
    uint32_t now = GetTimestamp();
    if ((now - lastTime) > 1000) {
        QCC_DbgPrintf(("Tx queue size (%s) = %d", GetUniqueName().c_str(), count));
        lastTime = now;
    }
#undef QCC_MODULE
#define QCC_MODULE "ALLJOYN"
#endif
    return status;
}

void _RemoteEndpoint::IncrementRef()
{
    int refs = IncrementAndFetch(&internal->refCount);
    QCC_DbgPrintf(("_RemoteEndpoint::IncrementRef(%s) refs=%d\n", GetUniqueName().c_str(), refs));
    QCC_UNUSED(refs); /* avoid unused variable warning in release build */
}

void _RemoteEndpoint::DecrementRef()
{
    int refs = DecrementAndFetch(&internal->refCount);
    QCC_DbgPrintf(("_RemoteEndpoint::DecrementRef(%s) refs=%d\n", GetUniqueName().c_str(), refs));
    if (refs <= 0) {
        if (minimalEndpoint && refs == 0) {
            Stop();
            return;
        }

        Thread* curThread = Thread::GetThread();
        if (strcmp(curThread->GetThreadName(), "iodisp") == 0) {
            Stop();
        } else {
            StopAfterTxEmpty(500);
        }
    }
}

bool _RemoteEndpoint::IsProbeMsg(const Message& msg, bool& isAck)
{
    bool ret = false;
    if (0 == ::strcmp(org::alljoyn::Daemon::InterfaceName, msg->GetInterface())) {
        if (0 == ::strcmp("ProbeReq", msg->GetMemberName())) {
            ret = true;
            isAck = false;
        } else if (0 == ::strcmp("ProbeAck", msg->GetMemberName())) {
            ret = true;
            isAck = true;
        }
    }
    return ret;
}

QStatus _RemoteEndpoint::GenProbeMsg(bool isAck, Message msg)
{
    return msg->SignalMsg("", NULL, 0, "/", org::alljoyn::Daemon::InterfaceName, isAck ? "ProbeAck" : "ProbeReq", NULL, 0, 0, 0);
}

void _RemoteEndpoint::SetSessionId(uint32_t sessionId) {
    if (internal) {
        internal->sessionId = sessionId;
    }
}

uint32_t _RemoteEndpoint::GetSessionId() {
    if (internal) {
        return internal->sessionId;
    } else {
        return 0;
    }
}

bool _RemoteEndpoint::IsSessionRouteSetUp()
{
    if (internal) {
        return (internal->sessionId != 0);
    } else {
        return false;
    }
}

}
