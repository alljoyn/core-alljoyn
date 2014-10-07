/**
 * @file
 * NullTransport implementation
 */

/******************************************************************************
 * Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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

#include <list>

#include <errno.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>
#include <qcc/Debug.h>

#include <alljoyn/BusAttachment.h>

#include "BusInternal.h"
#include "RemoteEndpoint.h"
#include "NullTransport.h"
#include "AllJoynPeerObj.h"

#define QCC_MODULE "NULL_TRANSPORT"

/* NULLEP_REFS_AT_DELETION:
 * Only the following refs to NullEndpoint should be active,
 * at the time of deletion of DaemonRouter,
 * all held by the current thread:
 *     local variable ep
 *     NullTransport::endpoint
 * This is to ensure that there are no threads in
 * NullEndpoint::PushMessage.
 */
const uint8_t NULLEP_REFS_AT_DELETION = 2;

using namespace std;
using namespace qcc;

namespace ajn {

const char* NullTransport::TransportName = "null";

RouterLauncher* NullTransport::routerLauncher;

class _NullEndpoint;

typedef ManagedObj<_NullEndpoint> NullEndpoint;

/*
 * The null endpoint simply moves messages between the daemon router to the client router and lets
 * the routers handle it from there. The only wrinkle is that messages forwarded to the routing node may
 * need to be encrypted because in the non-bundled case encryption is done in _Message::Deliver()
 * and that method does not get called in this case.
 */
class _NullEndpoint : public _BusEndpoint {

    friend class NullTransport;

  public:

    _NullEndpoint(BusAttachment& clientBus, BusAttachment& routerBus);

    ~_NullEndpoint();

    QStatus PushMessage(Message& msg);

    const qcc::String& GetUniqueName() const { return uniqueName; }

    uint32_t GetUserId() const { return qcc::GetUid(); }

    uint32_t GetGroupId() const { return qcc::GetGid(); }

    uint32_t GetProcessId() const { return qcc::GetPid(); }

    bool SupportsUnixIDs() const {
#if defined(QCC_OS_GROUP_WINDOWS)
        return false;
#else
        return true;
#endif
    }

    bool AllowRemoteMessages() { return true; }

    int32_t clientReady;
    BusAttachment& clientBus;
    BusAttachment& routerBus;

    qcc::String uniqueName;

    /*
     * Register the endpoint with the client on receiving the first message from the router.
     */
    inline void CheckRegisterEndpoint()
    {
        if (!clientReady) {
            /* Use atomic increment to avoid race */
            if (IncrementAndFetch(&clientReady) == 1) {
                QCC_DbgHLPrintf(("Registering null endpoint with client"));
                BusEndpoint busEndpoint = BusEndpoint::wrap(this);
                clientBus.GetInternal().GetRouter().RegisterEndpoint(busEndpoint);
            } else {
                DecrementAndFetch(&clientReady);
            }
        }
    }
};

_NullEndpoint::_NullEndpoint(BusAttachment& clientBus, BusAttachment& routerBus) :
    _BusEndpoint(ENDPOINT_TYPE_NULL),
    clientReady(0),
    clientBus(clientBus),
    routerBus(routerBus)
{
    /*
     * We short-circuit all of the normal authentication and hello handshakes and
     * simply get a unique name for the null endpoint directly from the router.
     */
    uniqueName = routerBus.GetInternal().GetRouter().GenerateUniqueName();
    QCC_DbgHLPrintf(("Creating null endpoint %s", uniqueName.c_str()));
}

_NullEndpoint::~_NullEndpoint()
{
    QCC_DbgHLPrintf(("Destroying null endpoint %s", uniqueName.c_str()));
}



QStatus _NullEndpoint::PushMessage(Message& msg)
{
    /* Get an extra reference to this endpoint to ensure
     * that the daemon router is not deleted while NullEndpoint::PushMessage
     * is in progress.
     */
    BusEndpoint busEndpoint = BusEndpoint::wrap(this);
    if (!IsValid()) {
        return ER_BUS_ENDPOINT_CLOSING;
    }

    QStatus status = ER_OK;
    /*
     * In the un-bundled router case messages store the name of the endpoint they were received
     * on. As far as the client and daemon routers are concerned the message was received from
     * this endpoint so we must set the received name to the unique name of this endpoint.
     */
    msg->rcvEndpointName = uniqueName;
    /*
     * If the message came from the client forward it to the routing node and visa versa. Note that
     * if the message didn't come from the client it must be assumed that it came from the
     * routing node to handle to the (rare) case of a broadcast signal being sent to multiple bus
     * attachments in a single application.
     */
    if (msg->bus == &clientBus) {
        /*
         * In the non-bundled case messages are encrypted when they are delivered to the routing node
         * endpoint by a call to Message::Deliver. The null transport bypasses Message::Deliver
         * by pushing the messages directly to the daemon router. This means we need to encrypt
         * messages here before we do the push.
         */
        if (msg->encrypt) {
            status = msg->EncryptMessage();
            /* Report authorization failure as a security violation */
            if (status == ER_BUS_NOT_AUTHORIZED) {
                clientBus.GetInternal().GetLocalEndpoint()->GetPeerObj()->HandleSecurityViolation(msg, status);
            }
        }
        if (status == ER_OK) {
            msg->bus = &routerBus;
            status = routerBus.GetInternal().GetRouter().PushMessage(msg, busEndpoint);
            if (status != ER_STOPPING_THREAD) {
                /* The NullEndpoint is a special case where the message is pushed to the DaemonRouter.
                 * In case of the RemoteEndpoint, the return value from PushMessage only indicates whether
                 * the message made it through to the RemoteEndpoint's transmit queue.
                 * We convert the error into an ER_OK here, so that the error codes from returned
                 * from this function resemble the behavior of the RemoteEndpoint.
                 * We preserve ER_STOPPING_THREAD.
                 * Note: The error codes returned from RemoteEndpoint::PushMessage are ER_OK,
                 * ER_BUS_NO_ENDPOINT(only if the endpoint was created with a default constructor),
                 * ER_BUS_ENDPOINT_CLOSING and ER_STOPPING_THREAD.
                 */
                status = ER_OK;
            }
        } else if (status == ER_BUS_AUTHENTICATION_PENDING) {
            status = ER_OK;
        }
    } else {
        assert(msg->bus == &routerBus);
        /*
         * Register the endpoint with the client router if needed
         */
        CheckRegisterEndpoint();
        /*
         * We need to clone broadcast signals because each receiving bus attachment must be
         * able to unmarshal the arg list including decrypting and doing header expansion.
         */
        if (msg->IsBroadcastSignal()) {
            Message clone(msg, true /*deep copy*/);
            clone->bus = &clientBus;
            status = clientBus.GetInternal().GetRouter().PushMessage(clone, busEndpoint);
        } else {
            msg->bus = &clientBus;
            status = clientBus.GetInternal().GetRouter().PushMessage(msg, busEndpoint);
        }
    }
    return status;
}

NullTransport::NullTransport(BusAttachment& bus) : bus(bus), running(false)
{
}

NullTransport::~NullTransport()
{
    Stop();
    Join();
    /* Only one ref to the NullEndpoint must remain,
     * held by NullTransport::endpoint
     */
    assert(endpoint.GetRefCount() == 1);
}

QStatus NullTransport::Start()
{
    running = true;
    return ER_OK;
}

QStatus NullTransport::Stop(void)
{
    running = false;
    Disconnect("null:");
    return ER_OK;
}

QStatus NullTransport::Join(void)
{
    if (routerLauncher) {
        routerLauncher->Join();
    }
    return ER_OK;
}

QStatus NullTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, std::map<qcc::String, qcc::String>& argMap) const
{
    outSpec = inSpec;
    return ER_OK;
}

QStatus NullTransport::LinkBus(BusAttachment* otherBus)
{
    QCC_DbgHLPrintf(("Linking leaf node and routing node busses"));

    assert(otherBus);

    /*
     * Initialize the null endpoint
     */
    NullEndpoint ep(bus, *otherBus);
    /*
     * The compression rules are shared between the client bus and the routing node bus
     */
    bus.GetInternal().OverrideCompressionRules(otherBus->GetInternal().GetCompressionRules());
    /*
     * Register the null endpoint with the daemon router. The endpoint is registered with the client
     * router either below or in PushMessage if a message is received before the call to register
     * the endpoint with the daemon router returns.
     */
    QCC_DbgHLPrintf(("Registering null endpoint with routing node"));

    endpoint = BusEndpoint::cast(ep);
    QStatus status = otherBus->GetInternal().GetRouter().RegisterEndpoint(endpoint);
    if (status != ER_OK) {
        endpoint->Invalidate();
    } else {
        /*
         * Register the endpoint with the client router if needed
         */
        ep->CheckRegisterEndpoint();
    }
    return status;
}

QStatus NullTransport::Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep)
{
    QStatus status = ER_OK;

    if (!running) {
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }
    if (!routerLauncher) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    status = routerLauncher->Start(this);
    if (status == ER_OK) {
        newep = endpoint;
    }
    return status;
}

QStatus NullTransport::Disconnect(const char* connectSpec)
{
    if (endpoint->IsValid()) {
        NullEndpoint ep = NullEndpoint::cast(endpoint);
        assert(routerLauncher);
        ep->clientBus.GetInternal().GetRouter().UnregisterEndpoint(ep->GetUniqueName(), ep->GetEndpointType());
        ep->routerBus.GetInternal().GetRouter().UnregisterEndpoint(ep->GetUniqueName(), ep->GetEndpointType());
        ep->Invalidate();

        /* Stop the routerLauncher first, so that all the Routing node bus objects are stopped */
        routerLauncher->Stop(this);

        /* Wait for any threads that are in PushMessage to finish
         * before the router-side BusAttachment is deleted as
         * a part of the RouterLauncher Join.
         */
        while (endpoint.GetRefCount() > NULLEP_REFS_AT_DELETION) {
            qcc::Sleep(4);
        }

        /* Only the following refs to NullEndpoint should be active now,
         * all held by the current thread:
         *     local variable ep
         *     NullTransport::endpoint
         */

        routerLauncher->Join();
    }
    return ER_OK;
}

void NullTransport::RegisterRouterLauncher(RouterLauncher* launcher)
{
    routerLauncher = launcher;
}

} // namespace ajn
