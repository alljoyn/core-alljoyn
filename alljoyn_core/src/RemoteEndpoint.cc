/**
 * @file
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

/*
 * SetState is defined as a macro so that the line number in the debug logs
 * corresponds to the location the state was changed at.
 */
#ifndef NDEBUG
#define SetState(to)                                                    \
    do {                                                                \
        Internal::State from = internal->state;                         \
        QCC_DbgHLPrintf(("%s: %s -> %s", internal->uniqueName.c_str(), Internal::StateText[from], Internal::StateText[to])); \
        internal->state = to;                                           \
    } while (0)
#else
#define SetState(to)                            \
    do {                                        \
        internal->state = to;                   \
    } while (0)
#endif

class _RemoteEndpoint::Internal {
    friend class _RemoteEndpoint;
  public:

    /** The states of the stream */
    typedef enum {
        STOPPED = 0,         /**< Not started. */
        STARTED,             /**< StartStream() has been called. */
        STOP_WAIT,           /**< Other end is closed and Stop() has not been called. */
        OTHER_END_STOP_WAIT, /**< Stop() has been called, waiting for other end to close. */
        STOPPING,            /**< Other end is closed and txQueue is not empty. */
        EXIT_WAIT            /**< StopStream() has been called. */
    } State;
    static const char* StateText[];

    Internal(BusAttachment& bus, bool incoming, const qcc::String& connectSpec, Stream* stream, const char* threadName, bool isSocket) :
        bus(bus),
        stream(stream),
        txQueue(),
        txWaitQueue(),
        lock(),
        listener(NULL),
        connSpec(connectSpec),
        incoming(incoming),
        processId((uint32_t)-1),
        alljoynVersion(0),
        refCount(0),
        isSocket(isSocket),
        armRxPause(false),
        idleTimeoutCount(0),
        maxIdleProbes(0),
        idleTimeout(0),
        probeTimeout(0),
        threadName(threadName),
        currentReadMsg(bus),
        validateSender(incoming),
        hasRxSessionMsg(false),
        getNextMsg(true),
        currentWriteMsg(bus),
        state(STOPPED),
        stopAfterTxEmpty(false),
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

    EndpointListener* listener;              /**< Listener for thread exit and untrusted client start and exit notifications. */

    qcc::String connSpec;                    /**< Connection specification for out-going connections */
    bool incoming;                           /**< Indicates if connection is incoming (true) or outgoing (false) */

    Features features;                       /**< Requested and negotiated features of this endpoint */
    uint32_t processId;                      /**< Process id of the process at the remote end of this endpoint */
    uint32_t alljoynVersion;                 /**< AllJoyn version of the process at the remote end of this endpoint */
    volatile int32_t refCount;               /**< Number of active users of this remote endpoint */
    bool isSocket;                           /**< True iff this endpoint contains a SockStream as its 'stream' member */
    volatile bool armRxPause;                /**< Pause Rx after receiving next METHOD_REPLY message */

    uint32_t idleTimeoutCount;               /**< Number of consecutive idle timeouts */
    uint32_t maxIdleProbes;                  /**< Maximum number of missed idle probes before shutdown */
    uint32_t idleTimeout;                    /**< RX idle seconds before sending probe */
    uint32_t probeTimeout;                   /**< Probe timeout in seconds */

    String uniqueName;                       /**< Obtained from EndpointAuth */
    String remoteName;                       /**< Obtained from EndpointAuth */
    GUID128 remoteGUID;                      /**< Obtained from EndpointAuth */
    const char* threadName;                  /**< Transport Name for the Endpoint */

    Message currentReadMsg;                  /**< The message currently being read for this endpoint */
    const bool validateSender;               /**< If true, the sender field on incomming messages will be overwritten with actual endpoint name */
    bool hasRxSessionMsg;                    /**< true iff this endpoint has previously processed a non-control message */
    bool getNextMsg;                         /**< If true, read the next message from the txQueue */
    Message currentWriteMsg;                 /**< The message currently being read for this endpoint */
    volatile State state;                    /**< The state of the stream */
    bool stopAfterTxEmpty;                   /**< True to StopStream() when txQueue is empty */
    set<SessionId> sessionIdSet;                    /**< Set of session Ids that this endpoint is a part of */
    uint32_t pingCallSerial;                 /**< Serial number of last Heartbeat DBus ping sent */
    uint32_t sendTimeout;                    /**< Send timeout for this endpoint i.e. time after which the Routing node must
                                                  disconnect the remote node if the remote node has not read a message from the link
                                                  in the situation that the send buffer on this end and receive buffer on
                                                  the remote end are full. */
    size_t maxControlMessages;               /**< Number of control messages that can be queued up before disconnecting this endpoint.
                                                  - used on Routing nodes only */
    volatile size_t numControlMessages;      /**< Number of control messages in txQueue - used on Routing nodes only */
    volatile size_t numDataMessages;         /**< Number of data messages in txQueue - used on Routing nodes only */
  private:
    Internal& operator=(const Internal&);
};

const char* _RemoteEndpoint::Internal::StateText[] = { "STOPPED", "STARTED", "STOP_WAIT", "OTHER_END_STOP_WAIT", "STOPPING", "EXIT_WAIT" };

void _RemoteEndpoint::SetStream(qcc::Stream* s)
{
    if (internal) {
        internal->stream = s;
    }
}

void _RemoteEndpoint::SetStarted()
{
    if (internal) {
        internal->lock.Lock(MUTEX_CONTEXT);
        SetState(Internal::STARTED);
        internal->lock.Unlock(MUTEX_CONTEXT);
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

const qcc::String& _RemoteEndpoint::GetConnectSpec() const
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

QStatus _RemoteEndpoint::Establish(const qcc::String& authMechanisms, qcc::String& authUsed, qcc::String& redirection, AuthListener* listener, uint32_t timeout)
{
    QStatus status = ER_OK;

    if (!internal || minimalEndpoint) {
        status = ER_BUS_NO_ENDPOINT;
    } else {
        RemoteEndpoint rep = RemoteEndpoint::wrap(this);
        EndpointAuth auth(internal->bus, rep, internal->incoming);

        status = auth.Establish(authMechanisms, authUsed, redirection, listener, timeout);
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
    QCC_UNUSED(idleTimeout);

    if (internal) {
        internal->lock.Lock(MUTEX_CONTEXT);
        internal->idleTimeout = 0;
        internal->lock.Unlock(MUTEX_CONTEXT);
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
        Join(0);
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
        uint32_t timeout = (internal->idleTimeoutCount == 0) ? internal->idleTimeout : internal->probeTimeout;
        QStatus status = internal->bus.GetInternal().GetIODispatch().EnableTimeoutCallback(internal->stream, timeout);
        internal->lock.Unlock(MUTEX_CONTEXT);
        return status;
    } else {
        return ER_ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT;
    }
}

QStatus _RemoteEndpoint::SetIdleTimeouts(uint32_t& idleTimeout, uint32_t& probeTimeout)
{
    QCC_UNUSED(idleTimeout);
    QCC_UNUSED(probeTimeout);

    if (internal) {
        internal->lock.Lock(MUTEX_CONTEXT);
        internal->idleTimeout = 0;
        internal->probeTimeout = 0;
        internal->lock.Unlock(MUTEX_CONTEXT);
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
    internal->idleTimeoutCount = 0;
    QStatus status = internal->bus.GetInternal().GetIODispatch().EnableTimeoutCallback(internal->stream, internal->idleTimeout);
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
    assert(minimalEndpoint || internal->stream);

    QCC_DbgPrintf(("_RemoteEndpoint::Start(%s) isBusToBus=%s,allowRemote=%s", GetUniqueName().c_str(),
                   internal->features.isBusToBus ? "true" : "false",
                   internal->features.allowRemote ? "true" : "false"));

    if (internal->features.isBusToBus) {
        endpointType = ENDPOINT_TYPE_BUS2BUS;
    }

    internal->lock.Lock(MUTEX_CONTEXT);
    SetState(Internal::STARTED);
    internal->lock.Unlock(MUTEX_CONTEXT);

    if (minimalEndpoint) {
        return ER_OK;
    }

    /* Endpoint needs to be wrapped before we can use it */
    RemoteEndpoint rep = RemoteEndpoint::wrap(this);
    BusEndpoint bep = BusEndpoint::cast(rep);

    Router& router = internal->bus.GetInternal().GetRouter();
    QStatus status = router.RegisterEndpoint(bep);
    if (status == ER_OK) {
        internal->lock.Lock(MUTEX_CONTEXT);
        /*
         * Set the send timeout for this endpoint
         * Note that this is set to zero even though the actual send timeout is different.
         * This is because we want non-blocking functionality from the underlying stream.
         * Send timeout is implemented using a timedout WriteCallback from IODispatch.
         */
        internal->stream->SetSendTimeout(0);

        status = internal->bus.GetInternal().GetIODispatch().StartStream(internal->stream, this, this, this, false, true);
        if (status == ER_OK) {
            status = internal->bus.GetInternal().GetIODispatch().EnableReadCallback(internal->stream);
        }
        if (status != ER_OK) {
            internal->stream->Abort();
            internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
            SetState(Internal::STOPPED);
            Invalidate();
        }
        internal->lock.Unlock(MUTEX_CONTEXT);
    }
    if (status != ER_OK) {
        router.UnregisterEndpoint(this->GetUniqueName(), this->GetEndpointType());
    }

    return status;
}

QStatus _RemoteEndpoint::Start(uint32_t idleTimeout, uint32_t probeTimeout, uint32_t numProbes, uint32_t sendTimeout)
{
    QCC_DbgTrace(("_RemoteEndpoint::Start(%s,idleTimeout=%u,probeTimeout=%u,numProbes=%u,sendTimeout=%u)",
                  GetUniqueName().c_str(), idleTimeout, probeTimeout, numProbes, sendTimeout));

    QStatus status = Start();
    if (status == ER_OK && endpointType == ENDPOINT_TYPE_REMOTE) {
        /* Set idle timeouts for leaf nodes only */
        status = SetIdleTimeouts(idleTimeout, probeTimeout, numProbes);
    }
    internal->lock.Lock(MUTEX_CONTEXT);
    internal->sendTimeout = sendTimeout;
    internal->maxControlMessages = sendTimeout * MAX_CONTROL_MSGS_PER_SECOND;
    if (status != ER_OK) {
        SetState(Internal::STOPPED);
        Invalidate();
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
    return status;
}

void _RemoteEndpoint::SetListener(EndpointListener* listener)
{
    if (internal) {
        internal->listener = listener;
    }
}

QStatus _RemoteEndpoint::Stop(bool stopAfterTxEmpty)
{
    if (!internal) {
        return ER_BUS_NO_ENDPOINT;
    }
    QCC_DbgPrintf(("_RemoteEndpoint::Stop(%s,stopAfterTxEmpty=%d) called", GetUniqueName().c_str(), stopAfterTxEmpty));

    if (minimalEndpoint) {
        internal->lock.Lock(MUTEX_CONTEXT);
        SetState(Internal::STOPPED);
        Invalidate();
        internal->lock.Unlock(MUTEX_CONTEXT);
        return ER_OK;
    }

    QStatus status = ER_OK;
    internal->lock.Lock(MUTEX_CONTEXT);
    switch (internal->state) {
    case Internal::STARTED:
        SetState(Internal::OTHER_END_STOP_WAIT);
        internal->stopAfterTxEmpty = stopAfterTxEmpty;
        if (internal->txQueue.empty() && internal->txWaitQueue.empty()) {
            status = internal->stream->Shutdown();
            if ((ER_OK == status) && internal->stopAfterTxEmpty) {
                internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
                SetState(Internal::EXIT_WAIT);
                Invalidate();
            }
        }
        break;

    case Internal::STOP_WAIT:
        SetState(Internal::STOPPING);
        if (internal->txQueue.empty() && internal->txWaitQueue.empty()) {
            status = internal->stream->Shutdown();
            if (ER_OK == status) {
                internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
                SetState(Internal::EXIT_WAIT);
                Invalidate();
            }
        }
        break;

    case Internal::OTHER_END_STOP_WAIT:
    case Internal::STOPPING:
    case Internal::STOPPED:
    case Internal::EXIT_WAIT:
        /* Nothing to do */
        break;
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus _RemoteEndpoint::Stop()
{
    return Stop(false);
}

QStatus _RemoteEndpoint::StopAfterTxEmpty()
{
    return Stop(true);
}

QStatus _RemoteEndpoint::PauseAfterRxReply()
{
    if (internal) {
        internal->lock.Lock(MUTEX_CONTEXT);
        internal->armRxPause = true;
        internal->lock.Unlock(MUTEX_CONTEXT);
        return ER_OK;
    } else {
        return ER_BUS_NO_ENDPOINT;
    }
}

QStatus _RemoteEndpoint::Join()
{
    /*
     * How long to wait for an orderly release of the connection?  The TCP and
     * UDP transports have a 30 second timer that will kick-in for a
     * non-responsive connection, so re-use that value here.
     */
    return Join(30000);
}

QStatus _RemoteEndpoint::Join(uint32_t maxWaitMs)
{
    QCC_DbgPrintf(("_RemoteEndpoint::Join(%s) called", GetUniqueName().c_str()));

    if (!internal) {
        return ER_BUS_NO_ENDPOINT;
    }

    internal->lock.Lock(MUTEX_CONTEXT);
    /* First wait for an orderly release */
    uint32_t startTime = GetTimestamp();
    while ((internal->state != Internal::STOPPED) && (qcc::GetTimestamp() < (startTime + maxWaitMs))) {
        internal->lock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(5);
        internal->lock.Lock(MUTEX_CONTEXT);
    }
    /* Abortively release if we didn't exit before the timeout */
    if ((internal->state != Internal::STOPPED) && ((startTime + maxWaitMs) <= qcc::GetTimestamp())) {
        internal->stream->Abort();
        internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
        SetState(Internal::EXIT_WAIT);
        Invalidate();
        while (internal->state != Internal::STOPPED) {
            internal->lock.Unlock(MUTEX_CONTEXT);
            qcc::Sleep(5);
            internal->lock.Lock(MUTEX_CONTEXT);
        }
    }
    assert(internal->state == Internal::STOPPED);
    internal->lock.Unlock(MUTEX_CONTEXT);
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

    if (!internal) {
        return;
    }

    Invalidate();

    /* Un-register this remote endpoint from the router */
    internal->bus.GetInternal().GetRouter().UnregisterEndpoint(this->GetUniqueName(), this->GetEndpointType());

    if (internal->listener) {
        RemoteEndpoint rep = RemoteEndpoint::wrap(this);
        internal->listener->EndpointExit(rep);
        internal->listener = NULL;
    }

    internal->lock.Lock(MUTEX_CONTEXT);
    SetState(Internal::STOPPED);
    internal->lock.Unlock(MUTEX_CONTEXT);
}

void _RemoteEndpoint::Exited()
{
    QCC_DbgTrace(("_RemoteEndpoint::Exited(%s)", GetUniqueName().c_str()));
    if (internal) {
        internal->lock.Lock(MUTEX_CONTEXT);
        SetState(Internal::STOPPED);
        internal->lock.Unlock(MUTEX_CONTEXT);
    }
}

void _RemoteEndpoint::ExitCallback()
{
    assert(minimalEndpoint == false && "_RemoteEndpoint::ExitCallback(): Where did a callback come from if no thread?");

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
    internal->lock.Lock(MUTEX_CONTEXT);
    switch (internal->state) {
    case Internal::STOPPED:
        /* Invalid */
        QCC_LogError(ER_WARNING, ("%s: unexpected state in ExitCallback: %s", GetUniqueName().c_str(),
                                  Internal::StateText[internal->state]));
        break;

    case Internal::STARTED:
    case Internal::STOP_WAIT:
    case Internal::OTHER_END_STOP_WAIT:
    case Internal::STOPPING:
    case Internal::EXIT_WAIT:
        if (!(internal->txQueue.empty() && internal->txWaitQueue.empty())) {
            internal->stream->Abort();
        }
        SetState(Internal::STOPPED);
        break;
    }
    internal->stream->Close();
    internal->lock.Unlock(MUTEX_CONTEXT);
}

QStatus _RemoteEndpoint::ReadCallback(qcc::Source& source, bool isTimedOut)
{
    QCC_UNUSED(source);
    assert(minimalEndpoint == false && "_RemoteEndpoint::ReadCallback(): Where did a callback come from if no thread?");

    if (!internal) {
        return ER_BUS_NO_ENDPOINT;
    }

    QStatus status = ER_OK;
    if (!isTimedOut) {
        Router& router = internal->bus.GetInternal().GetRouter();
        const bool bus2bus = (ENDPOINT_TYPE_BUS2BUS == GetEndpointType());
        RemoteEndpoint rep = RemoteEndpoint::wrap(this);
        while (status == ER_OK) {
            status = internal->currentReadMsg->ReadNonBlocking(rep, (internal->validateSender && !bus2bus));
            if (status == ER_OK) {
                /* Message read complete.  Proceed to unmarshal it. */
                internal->lock.Lock(MUTEX_CONTEXT);
                Message msg = internal->currentReadMsg;
                status = msg->Unmarshal(rep, (internal->validateSender && !bus2bus));
                switch (status) {
                case ER_OK:
                    internal->idleTimeoutCount = 0;
                    bool isAck;
                    if ((internal->pingCallSerial != 0) &&
                        (msg->GetType() == MESSAGE_METHOD_RET) &&
                        (msg->GetReplySerial() == internal->pingCallSerial)) {
                        /* This is a response to the DBus ping sent from RN to LN. Consume the reply quietly. */
                        internal->pingCallSerial = 0;

                    } else if (IsProbeMsg(msg, isAck)) {
                        QCC_DbgPrintf(("%s: Received %s", GetUniqueName().c_str(), isAck ? "ProbeAck" : "ProbeReq"));
                        if (!isAck) {
                            /* Respond to probe request */
                            Message probeMsg(internal->bus);
                            status = GenProbeMsg(true, probeMsg);
                            if (status == ER_OK) {
                                internal->lock.Unlock(MUTEX_CONTEXT);
                                status = PushMessage(probeMsg);
                                internal->lock.Lock(MUTEX_CONTEXT);
                            }
                            QCC_DbgPrintf(("%s: Sent ProbeAck (%s)", GetUniqueName().c_str(), QCC_StatusText(status)));
                        }

                    } else {
                        internal->lock.Unlock(MUTEX_CONTEXT);
                        BusEndpoint bep = BusEndpoint::cast(rep);
                        status = router.PushMessage(msg, bep);
                        internal->lock.Lock(MUTEX_CONTEXT);
                        if (status != ER_OK) {
                            /*
                             * There are five cases where a failure to push a message to the router is ok:
                             *
                             * 1) The message received did not match the expected signature.
                             * 2) The message was a method reply that did not match up to a method call.
                             * 3) A daemon is pushing the message to a connected client or service.
                             * 4) Pushing a message to an endpoint that has closed.
                             * 5) Pushing the first non-control message of a new session (must wait for route to be fully setup)
                             */
                            if (status == ER_BUS_NO_ROUTE) {
                                int retries = 20;
                                while ((internal->state != Internal::STOPPED) &&
                                       (status == ER_BUS_NO_ROUTE) && !internal->hasRxSessionMsg && retries--) {
                                    internal->lock.Unlock(MUTEX_CONTEXT);
                                    qcc::Sleep(10);
                                    status = router.PushMessage(msg, bep);
                                    internal->lock.Lock();
                                }
                            }
                            if ((router.IsDaemon() && !bus2bus) ||
                                (status == ER_BUS_SIGNATURE_MISMATCH) ||
                                (status == ER_BUS_UNMATCHED_REPLY_SERIAL) ||
                                (status == ER_BUS_ENDPOINT_CLOSING)) {
                                QCC_DbgHLPrintf(("%s: Discarding %s: %s", GetUniqueName().c_str(), msg->Description().c_str(),
                                                 QCC_StatusText(status)));
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
                    if (router.IsDaemon()) {
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

                /* Check pause condition. */
                if (internal->armRxPause && (internal->state != Internal::STOPPED) && (msg->GetType() == MESSAGE_METHOD_RET)) {
                    status = ER_BUS_ENDPOINT_CLOSING;
                    internal->bus.GetInternal().GetIODispatch().DisableReadCallback(internal->stream);
                    internal->lock.Unlock(MUTEX_CONTEXT);
                    return ER_OK;
                }
                if (status == ER_OK) {
                    internal->currentReadMsg = Message(internal->bus);
                }
                internal->lock.Unlock(MUTEX_CONTEXT);
            }
        }

        internal->lock.Lock(MUTEX_CONTEXT);
        if (status == ER_TIMEOUT) {
            internal->bus.GetInternal().GetIODispatch().EnableReadCallback(internal->stream, internal->idleTimeout);
        } else {
            if ((status != ER_STOPPING_THREAD) && (status != ER_SOCK_OTHER_END_CLOSED) && (status != ER_BUS_STOPPING)) {
                QCC_DbgPrintf(("Endpoint Rx failed (%s): %s", GetUniqueName().c_str(), QCC_StatusText(status)));
            }
            /* On an unexpected disconnect save the status that cause the thread exit */
            if (disconnectStatus == ER_OK) {
                disconnectStatus = (status == ER_STOPPING_THREAD) ? ER_OK : status;
            }
            switch (internal->state) {
            case Internal::STARTED:
                /*
                 * The other end has stopped writing to the stream.  Now we need
                 * to stop writing to the connection before we close it.  How do
                 * we know we are done writing to the connection?  We don't
                 * unless the upper layer calls Stop() which may or may not
                 * happen depending on the messages sent by the other end.  So
                 * instead, we rely on the state of pending sends to tell us
                 * when we can close the connection.
                 */
                if (internal->txQueue.empty() && internal->txWaitQueue.empty()) {
                    internal->stream->Shutdown();
                    internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
                    SetState(Internal::EXIT_WAIT);
                    Invalidate();
                } else {
                    SetState(Internal::STOP_WAIT);
                }
                break;

            case Internal::OTHER_END_STOP_WAIT:
                if (internal->txQueue.empty() && internal->txWaitQueue.empty()) {
                    internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
                    SetState(Internal::EXIT_WAIT);
                    Invalidate();
                } else {
                    SetState(Internal::STOPPING);
                }
                break;

            case Internal::STOP_WAIT:
            case Internal::STOPPING:
            case Internal::STOPPED:
            case Internal::EXIT_WAIT:
                /* Invalid */
                QCC_LogError(ER_WARNING, ("%s: unexpected state in ReadCallback: %s", GetUniqueName().c_str(),
                                          Internal::StateText[internal->state]));
                break;
            }
        }
        internal->lock.Unlock(MUTEX_CONTEXT);
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
                QCC_DbgPrintf(("%s: Sent ProbeReq (%s)", GetUniqueName().c_str(), QCC_StatusText(status)));

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
                QCC_DbgPrintf(("%s: Sent DBus ping (%s)", GetUniqueName().c_str(), QCC_StatusText(status)));

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
            internal->stream->Abort();
            internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
            SetState(Internal::EXIT_WAIT);
            Invalidate();
        }
    }
    return status;
}

/* Note: isTimedOut indicates that this is a timeout alarm. This is used to implement
 * the SendTimeout functionality.
 */
QStatus _RemoteEndpoint::WriteCallback(qcc::Sink& sink, bool isTimedOut)
{
    QCC_UNUSED(sink);
    assert(minimalEndpoint == false && "_RemoteEndpoint::WriteCallback(): Where did a callback come from if no thread?");

    if (!internal) {
        return ER_BUS_NO_ENDPOINT;
    }

    if (isTimedOut) {
        /* On an unexpected disconnect save the status that cause the thread exit */
        if (disconnectStatus == ER_OK) {
            disconnectStatus = ER_TIMEOUT;
        }
        QCC_LogError(ER_TIMEOUT, ("Endpoint Tx timed out (%s)", GetUniqueName().c_str()));
        internal->lock.Lock(MUTEX_CONTEXT);
        internal->stream->Abort();
        internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
        SetState(Internal::EXIT_WAIT);
        Invalidate();
        internal->lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_ENDPOINT_CLOSING;
    }

    internal->lock.Lock(MUTEX_CONTEXT);
    QStatus status = ER_OK;
    while (status == ER_OK) {
        if (!IsValid()) {
            internal->lock.Unlock(MUTEX_CONTEXT);
            return ER_BUS_NO_ENDPOINT;
        }

        /* Get the message */
        if (internal->getNextMsg) {
            if (!internal->txQueue.empty()) {
                /*
                 * Make a deep copy of the message since there is state
                 * information inside the message.  Each copy of the message
                 * could be in different write state.
                 */
                internal->currentWriteMsg = Message(internal->txQueue.back(), true);
                internal->getNextMsg = false;
            } else {
                internal->bus.GetInternal().GetIODispatch().DisableWriteCallback(internal->stream);
                if (internal->txWaitQueue.empty()) {
                    switch (internal->state) {
                    case Internal::STARTED:
                    case Internal::STOP_WAIT:
                        /* Nothing to do */
                        break;

                    case Internal::OTHER_END_STOP_WAIT:
                        internal->stream->Shutdown();
                        if (internal->stopAfterTxEmpty) {
                            internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
                            SetState(Internal::EXIT_WAIT);
                            Invalidate();
                        }
                        break;

                    case Internal::STOPPING:
                        internal->stream->Shutdown();
                        internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
                        SetState(Internal::EXIT_WAIT);
                        Invalidate();
                        break;

                    case Internal::STOPPED:
                    case Internal::EXIT_WAIT:
                        /* Invalid */
                        QCC_LogError(ER_WARNING, ("%s: unexpected state in WriteCallback: %s", GetUniqueName().c_str(),
                                                  Internal::StateText[internal->state]));
                        break;
                    }
                }
                internal->lock.Unlock(MUTEX_CONTEXT);
                return ER_OK;
            }
        }

        /* Deliver the message */
        internal->lock.Unlock(MUTEX_CONTEXT);
        RemoteEndpoint rep = RemoteEndpoint::wrap(this);
        status = internal->currentWriteMsg->DeliverNonBlocking(rep);
        if (status == ER_BUS_NOT_AUTHORIZED) {
            /*
             * Report authorization failure as a security violation and clear
             * the status, otherwise we will exit this thread which will shut
             * down the endpoint.
             */
            internal->bus.GetInternal().GetLocalEndpoint()->GetPeerObj()->HandleSecurityViolation(internal->currentWriteMsg, status);
            status = ER_OK;
        }
        internal->lock.Lock(MUTEX_CONTEXT);
        if (status == ER_OK) {
            /* Message has been successfully delivered. i.e. PushBytes is complete */
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
        }
    }

    if (status == ER_TIMEOUT) {
        /*
         * Timed-out in the middle of a message write.  Re-enable the write
         * callback after a send timeout.
         */
        internal->bus.GetInternal().GetIODispatch().EnableWriteCallback(internal->stream, internal->sendTimeout);
    } else if (status != ER_OK) {
        /* On an unexpected disconnect save the status that cause the thread exit */
        if (disconnectStatus == ER_OK) {
            disconnectStatus = (status == ER_STOPPING_THREAD) ? ER_OK : status;
        }
        if ((status != ER_OK) && (status != ER_STOPPING_THREAD) && (status != ER_SOCK_OTHER_END_CLOSED) && (status != ER_BUS_STOPPING)) {
            QCC_LogError(status, ("Endpoint Tx failed (%s)", GetUniqueName().c_str()));
        }
        internal->stream->Abort();
        internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
        SetState(Internal::EXIT_WAIT);
        Invalidate();
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
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
        } else {
            QCC_LogError(ER_BUS_ENDPOINT_CLOSING, ("Endpoint Tx failed (%s)", GetUniqueName().c_str()));
            internal->stream->Abort();
            internal->bus.GetInternal().GetIODispatch().StopStream(internal->stream);
            SetState(Internal::EXIT_WAIT);
            Invalidate();
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

            for (;;) {
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
                    thread->ResetAlertCode();
                    thread->GetStopEvent().ResetEvent();
                }

                switch (internal->state) {
                case Internal::STARTED:
                case Internal::STOP_WAIT:
                case Internal::OTHER_END_STOP_WAIT:
                case Internal::STOPPING:
                case Internal::EXIT_WAIT:
                    /* Waiting for txQueue and txWaitQueue to drain. */
                    break;

                case Internal::STOPPED:
                    status = ER_BUS_ENDPOINT_CLOSING;
                    break;
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
    }
    internal->lock.Unlock(MUTEX_CONTEXT);

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

        for (;;) {
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
                thread->ResetAlertCode();
                thread->GetStopEvent().ResetEvent();
            }

            switch (internal->state) {
            case Internal::STARTED:
            case Internal::STOP_WAIT:
            case Internal::OTHER_END_STOP_WAIT:
            case Internal::STOPPING:
            case Internal::EXIT_WAIT:
                /* Waiting for txQueue and txWaitQueue to drain. */
                break;

            case Internal::STOPPED:
                status = ER_BUS_ENDPOINT_CLOSING;
                break;
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
    QCC_DbgTrace(("_RemoteEndpoint::PushMessage %s (serial=%d)", GetUniqueName().c_str(), msg->GetCallSerial()));

    QStatus status = ER_OK;
    size_t count;

    if (!internal) {
        return ER_BUS_NO_ENDPOINT;
    }
    /*
     * Don't continue if this endpoint is in the process of being closed
     * Otherwise we risk deadlock when sending NameOwnerChanged signal to
     * this dying endpoint
     */
    internal->lock.Lock(MUTEX_CONTEXT);
    if (internal->state != Internal::STARTED) {
        internal->lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_ENDPOINT_CLOSING;
    }
    internal->lock.Unlock(MUTEX_CONTEXT);

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
    int32_t refs = IncrementAndFetch(&internal->refCount);
    QCC_DbgPrintf(("_RemoteEndpoint::IncrementRef(%s) refs=%d", GetUniqueName().c_str(), refs));
    QCC_UNUSED(refs); /* avoid unused variable warning in release build */
}

void _RemoteEndpoint::DecrementRef()
{
    int32_t refs = DecrementAndFetch(&internal->refCount);
    QCC_DbgPrintf(("_RemoteEndpoint::DecrementRef(%s) refs=%d", GetUniqueName().c_str(), refs));
    if (refs <= 0) {
        if (!minimalEndpoint || (refs == 0)) {
            Stop();
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

void _RemoteEndpoint::RegisterSessionId(uint32_t sessionId)
{
    if (internal) {
        QCC_DbgPrintf(("_RemoteEndpoint::RegisterSessionId (%s,%u)", GetUniqueName().c_str(), sessionId));
        internal->sessionIdSet.insert(sessionId);
        assert(!internal->features.isBusToBus || (internal->sessionIdSet.size() == 1));
    }
}

void _RemoteEndpoint::UnregisterSessionId(uint32_t sessionId)
{
    if (internal) {
        QCC_DbgPrintf(("_RemoteEndpoint::UnregisterSessionId (%s,%u)", GetUniqueName().c_str(), sessionId));
        internal->sessionIdSet.erase(sessionId);
    }
}

bool _RemoteEndpoint::IsInSession(SessionId sessionId)
{
    if (internal) {
        return internal->sessionIdSet.find(sessionId) != internal->sessionIdSet.end();
    }
    return false;
}

uint32_t _RemoteEndpoint::GetSessionId() const
{
    if (internal) {
        assert(internal->sessionIdSet.size() < 2);
        if (internal->sessionIdSet.size() == 1) {
            return *internal->sessionIdSet.begin();
        }
    }
    return 0;

}

bool _RemoteEndpoint::IsSessionRouteSetUp()
{
    if (internal) {
        return (internal->sessionIdSet.size() != 0);
    } else {
        return false;
    }
}

}
