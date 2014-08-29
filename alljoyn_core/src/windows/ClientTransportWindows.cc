/**
 * @file
 * ClientTransportBase is a partial specialization of Transport that listens
 * on a TCP socket.
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

#include <qcc/platform.h>

#include <list>

#include <qcc/IPAddress.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <alljoyn/BusAttachment.h>

#include "BusInternal.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "ClientTransport.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

const char* ClientTransport::TransportName = "tcp";

class _ClientEndpoint;

typedef ManagedObj<_ClientEndpoint> ClientEndpoint;

class _ClientEndpoint : public _RemoteEndpoint {
  public:
    _ClientEndpoint(ClientTransport* transport, BusAttachment& bus, const qcc::String connectSpec,
                    qcc::SocketFd sock, const qcc::IPAddress& ipAddr, uint16_t port) :
        _RemoteEndpoint(bus, false, connectSpec, &m_stream, ClientTransport::TransportName),
        m_transport(transport),
        m_stream(sock),
        m_ipAddr(ipAddr),
        m_port(port)
    { }

    ~_ClientEndpoint() { }

    const qcc::IPAddress& GetIPAddress() { return m_ipAddr; }
    uint16_t GetPort() { return m_port; }

  protected:
    ClientTransport* m_transport;
    qcc::SocketStream m_stream;
    qcc::IPAddress m_ipAddr;
    uint16_t m_port;
};

QStatus ClientTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    /*
     * Take the string in inSpec, which must start with "tcp:" and parse it,
     * looking for comma-separated "key=value" pairs and initialize the
     * argMap with those pairs.
     */
    QStatus status = ParseArguments("tcp", inSpec, argMap);
    if (status != ER_OK) {
        return status;
    }

    /*
     * We need to return a map with all of the configuration items set to
     * valid values and a normalized string with the same.  For a client or
     * service TCP, we need a valid "addr" key.
     */
    map<qcc::String, qcc::String>::iterator i = argMap.find("addr");
    if (i == argMap.end()) {
        return ER_FAIL;
    } else {
        /*
         * We have a value associated with the "addr" key.  Run it through
         * a conversion function to make sure it's a valid value.
         */
        IPAddress addr;
        status = addr.SetAddress(i->second);
        if (status == ER_OK) {
            i->second = addr.ToString();
            outSpec = "tcp:addr=" + i->second;
        } else {
            return ER_BUS_BAD_TRANSPORT_ARGS;
        }
    }

    /*
     * For a client or service TCP, we need a valid "port" key.
     */
    i = argMap.find("port");
    if (i == argMap.end()) {
        return ER_FAIL;
    } else {
        /*
         * We have a value associated with the "port" key.  Run it through
         * a conversion function to make sure it's a valid value.
         */
        uint32_t port = StringToU32(i->second);
        if (port > 0 && port <= 0xffff) {
            i->second = U32ToString(port);
            outSpec += ",port=" + i->second;
        } else {
            return ER_BUS_BAD_TRANSPORT_ARGS;
        }
    }

    return ER_OK;
}

QStatus ClientTransport::Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep)
{
    QCC_DbgHLPrintf(("ClientTransport::Connect(): %s", connectSpec));

    if (!m_running) {
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }
    if (m_endpoint->IsValid()) {
        return ER_BUS_ALREADY_CONNECTED;
    }
    /*
     * Parse and normalize the connectArgs.  For a client or service, there are
     * no reasonable defaults and so the addr and port keys MUST be present or
     * an error is returned.
     */
    QStatus status;
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    status = NormalizeTransportSpec(connectSpec, normSpec, argMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("ClientTransport::Connect(): Invalid TCP connect spec \"%s\"", connectSpec));
        return status;
    }

    IPAddress ipAddr(argMap["addr"]);            // Guaranteed to be there.
    uint16_t port = StringToU32(argMap["port"]); // Guaranteed to be there.

    /*
     * Attempt to connect to the remote TCP address and port specified in the connectSpec.
     */
    SocketFd sockFd = qcc::INVALID_SOCKET_FD;
    status = Socket(QCC_AF_INET, QCC_SOCK_STREAM, sockFd);
    if (status != ER_OK) {
        QCC_LogError(status, ("ClientTransport(): socket Create() failed"));
        return status;
    }
    /*
     * Got a socket, now Connect() to the remote address and port.
     */
    status = qcc::Connect(sockFd, ipAddr, port);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("ClientTransport(): socket Connect() failed %s", QCC_StatusText(status)));
        qcc::Close(sockFd);
        return status;
    }
    /*
     * We have a connection established, but DBus wire protocol requires that every connection,
     * irrespective of transport, start with a single zero byte. This is so that the Unix-domain
     * socket transport used by DBus can pass SCM_RIGHTS out-of-band when that byte is sent.
     */
    uint8_t nul = 0;
    size_t sent;

    status = Send(sockFd, &nul, 1, sent);
    if (status != ER_OK) {
        QCC_LogError(status, ("ClientTransport::Connect(): Failed to send initial NUL byte"));
        qcc::Close(sockFd);
        return status;
    }
    /*
     * The underlying transport mechanism is started, but we need to create a
     * ClientEndpoint object that will orchestrate the movement of data across the
     * transport.
     */
    ClientEndpoint ep(this, m_bus, normSpec, sockFd, ipAddr, port);

    /* Initialized the features for this endpoint */
    ep->GetFeatures().isBusToBus = false;
    ep->GetFeatures().allowRemote = m_bus.GetInternal().AllowRemoteMessages();
    ep->GetFeatures().handlePassing = true;

    qcc::String authName;
    qcc::String redirection;
    status = ep->Establish("ANONYMOUS", authName, redirection);
    if (status == ER_OK) {
        ep->SetListener(this);
        status = ep->Start();
        if (status != ER_OK) {
            QCC_LogError(status, ("ClientTransport::Connect(): Start ClientEndpoint failed"));
        }
    }
    /*
     * If we got an error, we need to cleanup the socket and zero out the
     * returned endpoint.  If we got this done without a problem, we return
     * a pointer to the new endpoint. We do not close the socket since the
     * endpoint that was created is responsible for doing so.
     */
    if (status != ER_OK) {
        ep->Invalidate();
    } else {
        newep = BusEndpoint::cast(ep);
        m_endpoint = RemoteEndpoint::cast(ep);
    }
    return status;
}

} // namespace ajn
