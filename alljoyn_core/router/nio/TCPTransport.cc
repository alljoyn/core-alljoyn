/*
 * TCPTransport.cc
 *
 *  Created on: May 29, 2015
 *      Author: erongo
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
#include "TCPTransport.h"
#include "Proactor.h"
#include "SocketWriteableEvent.h"
#include "SocketReadableEvent.h"
#include "TimerEvent.h"
#include "Buffer.h"
#include "Endpoint.h"

#include <qcc/Util.h>
#include <qcc/Debug.h>

#include <algorithm>

#define QCC_MODULE "NIO_TCP"

using namespace nio;

class TCPTransport::TCPEndpoint : public Endpoint {

    friend class TCPTransport;

  public:
    TCPEndpoint(TransportBase& transport, Handle handle, const std::string& spec, qcc::SocketFd fd, const qcc::IPAddress& ip, uint16_t port, bool incoming) :
        Endpoint(transport, handle, spec), fd(fd), ip(ip), port(port), incoming(incoming) { }

    TCPEndpoint(const TCPEndpoint&) = delete;
    TCPEndpoint& operator=(const TCPEndpoint&) = delete;

    ~TCPEndpoint() { }

    qcc::SocketFd fd;
    const qcc::IPAddress ip;
    const uint16_t port;

    // notification handlers for when fd becomes readable/writeable
    std::shared_ptr<SocketReadableEvent> read_event;
    std::shared_ptr<SocketWriteableEvent> write_event;

    const bool incoming;

    std::recursive_mutex lock;
    bool connected = true;
};

#define DEFAULT_MAX_TCP 8
#define DEFAULT_TCP_CONN_TIMEOUT 10
#define DEFAULT_TCP_LISTEN_BACKLOG 10

const std::string TCPTransport::TransportName("tcp");

TCPTransport::TCPTransport(Proactor& proactor) : IPTransport(proactor, TransportName),
    num_incoming(0),
    max_tcp_connections(DEFAULT_MAX_TCP), tcp_connect_timeout(DEFAULT_TCP_CONN_TIMEOUT), tcp_listen_backlog(DEFAULT_TCP_LISTEN_BACKLOG)
{
    // TODO: get params from storage
}

TCPTransport::~TCPTransport()
{
    {
        std::lock_guard<std::mutex> guard(endpoints_lock);
        for (auto it : endpoints) {
            TCPEndpointPtr ep = it.second;
            std::lock_guard<std::recursive_mutex> guard(ep->lock);
            CloseEndpoint(ep);
        }

        endpoints.clear();
    }


    {
        std::lock_guard<std::mutex> guard(listeners_lock);
        for (auto it : listeners) {
            TCPEndpointPtr ep = it.second;
            std::lock_guard<std::recursive_mutex> guard(ep->lock);
            CloseEndpoint(ep);
        }

        listeners.clear();
    }

}



TCPTransport::TCPEndpointPtr TCPTransport::GetEndpoint(Endpoint::Handle handle) const
{
    TCPEndpointPtr ep;

    std::lock_guard<std::mutex> guard(endpoints_lock);
    auto it = endpoints.find(handle);
    if (it != endpoints.end()) {
        ep = it->second;
    }

    return ep;
}

TCPTransport::TCPEndpointPtr TCPTransport::CreateEndpoint(qcc::SocketFd sock, const qcc::IPAddress& addr, uint16_t port, bool incoming, const std::string& spec)
{
    Endpoint::Handle handle;
    std::lock_guard<std::mutex> guard(endpoints_lock);
    do {
        handle = qcc::Rand64();
    } while (handle != 0 && endpoints.find(handle) != endpoints.end());

    TCPEndpointPtr ep = std::make_shared<TCPEndpoint>(*this, handle, spec, sock, addr, port, incoming);

    if (incoming) {
        ++num_incoming;
    }

    endpoints[handle] = ep;
    return ep;
}

void TCPTransport::RemoveEndpoint(Endpoint::Handle handle)
{
    std::lock_guard<std::mutex> guard(endpoints_lock);
    auto it = endpoints.find(handle);
    if (it != endpoints.end()) {
        it->second->handle = Endpoint::INVALID_HANDLE;
        if (it->second->incoming) {
            --num_incoming;
        }

        endpoints.erase(it);
    }
}

// NOTE: ep->lock MUST BE HELD by the current thread when this is called!
void TCPTransport::CloseEndpoint(TCPEndpointPtr ep)
{
    ep->connected = false;

    if (ep->write_event.get()) {
        proactor.Cancel(ep->write_event);
    }

    if (ep->read_event.get()) {
        proactor.Cancel(ep->read_event);
    }

    if (ep->fd != qcc::INVALID_SOCKET_FD) {
        qcc::Shutdown(ep->fd);
        qcc::Close(ep->fd);
        ep->fd = qcc::INVALID_SOCKET_FD;
    }
}

// NOTE: ep->lock MUST BE HELD by the current thread when this is called!
void TCPTransport::EndpointDisconnected(TCPEndpointPtr ep)
{
    CloseEndpoint(ep);
    RemoveEndpoint(ep->handle);
}

void TCPTransport::WriteCallback(TCPEndpointPtr ep, MessageType msg, Endpoint::SendCompleteCB cb)
{
    QCC_DbgTrace(("TCPTransport::WriteCallback()"));
    size_t sent = 0;
    uint8_t* buf = msg->GetBuffer();
    size_t len = msg->GetLength();

    std::lock_guard<std::recursive_mutex>(ep->lock);
    if (!ep->connected) {
        return;
    }

    QStatus status = qcc::Send(ep->fd, buf, len, sent);
    // ER_WOULDBLOCK shouldn't happen since all TCP sockets in AJ are nonblocking
    if (status != ER_OK && status != ER_WOULDBLOCK) {
        proactor.Cancel(ep->write_event);
        ep->write_event.reset();
        cb(ep, msg, status);
        EndpointDisconnected(ep);
        return;
    } else if (status == ER_WOULDBLOCK) {
        return;
    }

    if (sent == 0) {
        proactor.Cancel(ep->write_event);
        ep->write_event.reset();
        cb(ep, msg, ER_SOCK_OTHER_END_CLOSED);
        // disconnect!
        EndpointDisconnected(ep);
        return;
    }

    if (sent == len) {
        proactor.Cancel(ep->write_event);
        ep->write_event.reset();
        cb(ep, msg, ER_OK);
    } else {
        // advance past the next 'sent' bytes
        ::memmove(buf, buf + sent, len - sent);
        msg->m_length -= sent;
    }
}

QStatus TCPTransport::Send(Endpoint::Handle handle, MessageType msg, Endpoint::SendCompleteCB cb)
{
    QCC_DbgTrace(("TCPTransport::Send(handle=%lu, msg=%p, cb=<>)", handle, &msg));

    uint8_t* buf = msg->GetBuffer();
    size_t len = msg->GetLength();

    TCPEndpointPtr ep = GetEndpoint(handle);
    if (!ep.get()) {
        return ER_BUS_NO_ENDPOINT;
    }

    std::lock_guard<std::recursive_mutex>(ep->lock);
    if (!ep->connected) {
        return ER_SOCK_OTHER_END_CLOSED;
    }

    if (ep->write_event.get()) {
        // we are already writing a message!
        // TODO: better error
        return ER_WRITE_ERROR;
    }

    size_t sent = 0;
    QStatus status = qcc::Send(ep->fd, buf, len, sent);
    if (status != ER_OK && status != ER_WOULDBLOCK) {
        EndpointDisconnected(ep);
        return status;
    }

    if (sent == 0) {
        ep->connected = false;
        //auto fcn = [cb, ep, msg] () { cb(ep, msg, ER_SOCK_OTHER_END_CLOSED); };
        //proactor.Dispatch(fcn);
        EndpointDisconnected(ep);
        return ER_SOCK_OTHER_END_CLOSED;
    }

    if (sent < len) {
        // need to advance past the data we have sent!
        ::memmove(buf, buf + sent, len - sent);
        msg->m_length -= sent;

        auto func = [this, ep, msg, cb] (qcc::SocketFd, QStatus) {
                        WriteCallback(ep, msg, cb);
                    };
        ep->write_event = std::make_shared<SocketWriteableEvent>(ep->fd, func);
        proactor.Register(ep->write_event);
    } else {
        auto fcn = [cb, ep, msg] () {
                       cb(ep, msg, ER_OK);
                   };
        proactor.Dispatch(fcn);
    }

    return ER_OK;
}

void TCPTransport::ReadCallback(TCPEndpointPtr ep, MessageType msg, Endpoint::ReadMessageCB cb)
{
    QCC_DbgTrace(("TCPTransport::ReadCallback()"));

    size_t recved = 0;
    uint8_t* buf = msg->GetBuffer();
    size_t len = msg->GetLength();
    size_t capacity = msg->GetCapacity();

    // lock!
    std::lock_guard<std::recursive_mutex>(ep->lock);
    QStatus status = qcc::Recv(ep->fd, buf + len, capacity - len, recved);

    if (status != ER_OK && status != ER_WOULDBLOCK) {
        proactor.Cancel(ep->read_event);
        ep->read_event.reset();

        cb(ep, msg, status);
        EndpointDisconnected(ep);
    } else if (status == ER_WOULDBLOCK) {
        // this should never happen; the reactor has informed us that this socket is readable
        return;
    }

    msg->m_length += recved;

    if (recved == 0) {
        // disconnected from the other end!
        proactor.Cancel(ep->read_event);
        ep->read_event.reset();
        cb(ep, msg, ER_SOCK_OTHER_END_CLOSED);
        EndpointDisconnected(ep);
    } else if (msg->m_length == capacity) {
        // we're finished with the message!
        proactor.Cancel(ep->read_event);
        ep->read_event.reset();
        cb(ep, msg, ER_OK);
    }
}

QStatus TCPTransport::Recv(Endpoint::Handle handle, MessageType msg, Endpoint::ReadMessageCB cb)
{
    QCC_DbgTrace(("TCPTransport::Recv(handle=%lu, msg=%p, cb=<>)", handle, &msg));
    TCPEndpointPtr ep = GetEndpoint(handle);
    if (!ep.get()) {
        return ER_BUS_NO_ENDPOINT;
    }

    std::lock_guard<std::recursive_mutex>(ep->lock);
    if (!ep->connected) {
        return ER_SOCK_OTHER_END_CLOSED;
    }

    if (ep->write_event.get()) {
        // we are already writing a message!
        // TODO: do we want to queue this?  or make the caller queue?
        return ER_READ_ERROR;
    }

    // TODO: attempt to recv now, only register for CB if can't get enough bytes.  Maybe?
    auto func = [this, ep, msg, cb] (qcc::SocketFd, QStatus) {
                    ReadCallback(ep, msg, cb);
                };
    ep->read_event = std::make_shared<SocketReadableEvent>(ep->fd, func);
    proactor.Register(ep->read_event);
    return ER_OK;
}




struct PendingConnection {
    std::mutex lock;
    std::shared_ptr<SocketWriteableEvent> sock_event;
    std::shared_ptr<TimerEvent> timeout_event;

    bool timed_out = false;
    bool connected = false;
};

QStatus TCPTransport::Connect(const std::string& spec, ConnectedCB cb)
{
    QCC_DbgTrace(("TCPTransport::Connect(spec=%s, cb=<>)", spec.c_str()));
    qcc::IPAddress addr;
    uint16_t port;
    std::string normSpec;

    if (!ParseSpec(spec, addr, port, normSpec)) {
        return ER_BUS_BAD_TRANSPORT_ARGS;
    }

    qcc::SocketFd sock;
    QStatus status = qcc::Socket(addr.GetAddressFamily(), qcc::QCC_SOCK_STREAM, sock);
    if (status != ER_OK) {
        QCC_LogError(status, ("qcc::Socket failed"));
        return status;
    }

    status = qcc::SetBlocking(sock, false);
    if (status != ER_OK) {
        QCC_LogError(status, ("qcc::SetBlocking failed"));
        return status;
    }

    status = qcc::Connect(sock, addr, port);
    if (status != ER_OK && status != ER_WOULDBLOCK) {
        QCC_LogError(status, ("qcc::Connect failed"));
        return status;
    }

    TCPEndpointPtr ep = CreateEndpoint(sock, addr, port, false, normSpec);

    // track the timeout!
    std::shared_ptr<PendingConnection> connection = std::make_shared<PendingConnection>();

    std::lock_guard<std::mutex>(connection->lock);
    auto timer_func = [this, connection, ep, cb] () {
                          std::lock_guard<std::mutex>(connection->lock);

                          QCC_DbgTrace(("Socket Connect timeout callback"));

                          if (connection->sock_event.get()) {
                              proactor.Cancel(connection->sock_event);
                              connection->sock_event.reset();
                          }

                          if (connection->timeout_event.get()) {
                              proactor.Cancel(connection->timeout_event);
                              connection->timeout_event.reset();
                          }

                          connection->timed_out = true;
                          if (!connection->connected) {
                              cb(ep, ER_TIMEOUT);
                              EndpointDisconnected(ep);
                          }
                      };

    auto func = [this, connection, cb, ep] (qcc::SocketFd, QStatus status) {
                    // cancel the events
                    std::lock_guard<std::mutex>(connection->lock);
                    QCC_DbgTrace(("Socket Connect connect callback"));

                    if (connection->sock_event.get()) {
                        proactor.Cancel(connection->sock_event);
                        connection->sock_event.reset();
                    }

                    if (connection->timeout_event.get()) {
                        proactor.Cancel(connection->timeout_event);
                        connection->timeout_event.reset();
                    }

                    connection->connected = true;
                    // only make the callback ONCE on connect OR timeout
                    if (!connection->timed_out) {
                        cb(ep, status);

                        if (status != ER_OK) {
                            EndpointDisconnected(ep);
                        }
                    }
                };

    connection->timeout_event = std::make_shared<TimerEvent>(std::chrono::milliseconds(tcp_connect_timeout * 1000), timer_func);
    connection->sock_event = std::make_shared<SocketWriteableEvent>(sock, func);

    proactor.Register(connection->timeout_event);
    proactor.Register(connection->sock_event);
    return ER_OK;
}


QStatus TCPTransport::Disconnect(Endpoint::Handle handle, bool force)
{
    QCC_UNUSED(force);
    TCPEndpointPtr ep = GetEndpoint(handle);
    if (!ep.get()) {
        return ER_BUS_NO_ENDPOINT;
    }

    std::lock_guard<std::recursive_mutex>(ep->lock);
    EndpointDisconnected(ep);
    return ER_OK;
}

uint32_t TCPTransport::GetNumIncoming() const
{
    std::lock_guard<std::mutex> guard(endpoints_lock);
    return num_incoming;
}

void TCPTransport::AcceptCallback(qcc::SocketFd sock, AcceptedCB cb)
{
    qcc::SocketFd client;
    qcc::IPAddress ip;
    uint16_t port;
    QStatus status;

    // loop and accept all incoming connections.
    // if we don't get them all, we might miss some because our Reactor is edge-triggered
    do {
        status = qcc::Accept(sock, ip, port, client);

        if (status == ER_OK) {
            status = qcc::SetBlocking(sock, false);
            if (status != ER_OK) {
                QCC_LogError(status, ("qcc::SetBlocking failed"));
                qcc::Shutdown(client);
                qcc::Close(client);
                continue;
            }

            if (GetNumIncoming() >= max_tcp_connections) {
                QCC_DbgTrace(("TCPTransport::AcceptCallback: too many incoming connections"));
                qcc::Shutdown(client);
                qcc::Close(client);
                continue;
            }

            std::string normSpec = NormalizeConnectionData(ip, port);
            QCC_DbgTrace(("TCPTransport::AcceptCallback: accepted from spec = %s", normSpec.c_str()));

            TCPEndpointPtr ep = CreateEndpoint(client, ip, port, true, normSpec);
            std::lock_guard<std::recursive_mutex>(ep->lock);

            // TODO: do all protocol handshake stuff *before* making this call!!
            const bool b = cb(ep);
            if (!b) {
                EndpointDisconnected(ep);
            }
        } else {
            QCC_LogError(status, ("qcc::Accept failed"));
        }
    } while (status == ER_OK);
}

QStatus TCPTransport::StopListen(const std::string& spec)
{
    QCC_DbgTrace(("TCPTransport::StopListen(spec = %s)", spec.c_str()));
    ListenEndpoints eps;

    if (!ParseSpec(spec, eps)) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS, ("Invalid spec: %s", spec.c_str()));
        return ER_BUS_BAD_TRANSPORT_ARGS;
    }

    bool found = false;
    for (const qcc::IPEndpoint& ipep : eps) {
        std::lock_guard<std::mutex> guard(listeners_lock);
        const std::string normSpec = NormalizeConnectionData(ipep.addr, ipep.port);
        auto it = listeners.find(normSpec);
        if (it != listeners.end()) {
            TCPEndpointPtr ep = it->second;
            proactor.Cancel(ep->read_event);
            ep->read_event.reset();

            proactor.Cancel(ep->write_event);
            ep->write_event.reset();

            qcc::Close(ep->fd);
            listeners.erase(it);
            found = true;

            // TODO: kill all incoming connections that came from this listener?
        }
    }

    return (found ? ER_OK : ER_BUS_NO_LISTENER);
}

QStatus TCPTransport::Listen(const std::string& spec, AcceptedCB cb)
{
    QCC_DbgTrace(("TCPTransport::Listen(spec = %s, cb = <>)", spec.c_str()));

    ListenEndpoints eps;
    bool found = false;

    if (!ParseSpec(spec, eps)) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS, ("Invalid spec: %s", spec.c_str()));
        return ER_BUS_BAD_TRANSPORT_ARGS;
    }

    std::lock_guard<std::mutex> guard(listeners_lock);

    for (const qcc::IPEndpoint& ipep : eps) {
        const std::string normSpec = NormalizeConnectionData(ipep.addr, ipep.port);
        if (listeners.find(normSpec) != listeners.end()) {
            QCC_LogError(ER_BUS_ALREADY_LISTENING, ("spec %s already found", normSpec.c_str()));
            return ER_BUS_ALREADY_LISTENING;
        }

        qcc::SocketFd sock;
        QStatus status = qcc::Socket(ipep.addr.GetAddressFamily(), qcc::QCC_SOCK_STREAM, sock);
        if (status != ER_OK) {
            QCC_LogError(status, ("qcc::Socket failed"));
            continue;
        }

        status = qcc::SetReusePort(sock, true);
        if (status != ER_OK) {
            QCC_LogError(status, ("qcc::SetReusePort failed"));
            qcc::Close(sock);
            continue;
        }

        status = qcc::SetBlocking(sock, false);
        if (status != ER_OK) {
            QCC_LogError(status, ("qcc::SetBlocking failed"));
            qcc::Close(sock);
            continue;
        }

        status = qcc::Bind(sock, ipep.addr, ipep.port);
        if (status != ER_OK) {
            QCC_LogError(status, ("qcc::Bind failed"));
            qcc::Close(sock);
            continue;
        }

        status = qcc::Listen(sock, tcp_listen_backlog);
        if (status != ER_OK) {
            QCC_LogError(status, ("qcc::Listen failed"));
            qcc::Close(sock);
            continue;
        }

        found = true;
        // don't need a handle for listeners!
        TCPEndpointPtr ep = std::make_shared<TCPEndpoint>(*this, Endpoint::INVALID_HANDLE, normSpec, sock, ipep.addr, ipep.port, false);

        auto func = [this, cb] (qcc::SocketFd fd, QStatus) {
                        AcceptCallback(fd, cb);
                    };
        ep->read_event = std::make_shared<SocketReadableEvent>(sock, func);
        proactor.Register(ep->read_event);

        // track this so we can cancel later!
        listeners[normSpec] = ep;
    }

    return (found ? ER_OK : ER_BUS_NO_LISTENER);
}
