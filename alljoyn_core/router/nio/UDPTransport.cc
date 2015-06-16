/*
 * UDPTransport.cc
 *
 *  Created on: Jun 2, 2015
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
#include "UDPTransport.h"
#include "Endpoint.h"
#include "EventNotifier.h"
#include "Proactor.h"

#include <qcc/IfConfig.h>
#include <qcc/Util.h>

using namespace nio;

class UDPTransport::UDPEndpoint : public Endpoint {

    friend class UDPTransport;

  public:
    UDPEndpoint(
        TransportBase& transport, Handle handle, const std::string& spec, ajn::ArdpConnRecord* conn,
        const qcc::IPAddress& ip, uint16_t port, bool incoming, ListenerPtr listener) :
        Endpoint(transport, handle, spec), conn(conn), ip(ip), port(port), incoming(incoming), listener(listener) { }

    UDPEndpoint(const UDPEndpoint&) = delete;
    UDPEndpoint& operator=(const UDPEndpoint&) = delete;

    virtual ~UDPEndpoint() { }

    ajn::ArdpConnRecord* conn;
    const qcc::IPAddress ip;
    const uint16_t port;
    const bool incoming;

    ListenerPtr listener;

    std::mutex lock;

    typedef std::pair<MessageType, Endpoint::SendCompleteCB> SentMessage;
    std::map<uint8_t*, SentMessage> sentMessages;

    typedef std::pair<uint8_t*, uint16_t> Message;
    std::queue<Message> outgoing;

    std::shared_ptr<EventNotifier> read_event;
    std::shared_ptr<EventNotifier> write_event;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string UDPTransport::TransportName("udp");

UDPTransport::UDPTransport(Proactor& proactor) : IPTransport(proactor, TransportName), handle(nullptr)
{
    // TODO: set up config

    handle = ajn::ARDP_AllocHandle(&config);

    ajn::ARDP_SetHandleContext(handle, this);
    ajn::ARDP_SetAcceptCb(handle, UDPTransport::ArdpAcceptCb);
    ajn::ARDP_SetConnectCb(handle, UDPTransport::ArdpConnectCb);
    ajn::ARDP_SetDisconnectCb(handle, UDPTransport::ArdpDisconnectCb);
    ajn::ARDP_SetRecvCb(handle, UDPTransport::ArdpRecvCb);
    ajn::ARDP_SetSendCb(handle, UDPTransport::ArdpSendCb);
    ajn::ARDP_SetSendWindowCb(handle, UDPTransport::ArdpSendWindowCb);
}

UDPTransport::~UDPTransport()
{
    ARDP_FreeHandle(handle);
}

UDPTransport::UDPEndpointPtr UDPTransport::GetEndpoint(Endpoint::Handle handle) const
{
    UDPEndpointPtr ep;
    std::lock_guard<std::recursive_mutex> guard(lock);
    auto it = endpoints.find(handle);
    if (it != endpoints.end()) {
        ep = it->second;
    }
    return ep;
}

UDPTransport::UDPEndpointPtr UDPTransport::GetEndpoint(ajn::ArdpConnRecord* conn) const
{
    UDPEndpointPtr ep;
    std::lock_guard<std::recursive_mutex> guard(lock);
    auto it = record_to_endpoint.find(conn);
    if (it != record_to_endpoint.end()) {
        ep = it->second;
    }
    return ep;
}

UDPTransport::UDPEndpointPtr UDPTransport::CreateEndpoint(
    ajn::ArdpConnRecord* conn, const qcc::IPAddress& addr, uint16_t port,
    bool incoming, ListenerPtr listener, const std::string& normSpec)
{
    Endpoint::Handle handle;
    std::lock_guard<std::recursive_mutex> guard(lock);
    do {
        handle = qcc::Rand64();
    } while (handle != 0 && endpoints.find(handle) != endpoints.end());

    UDPEndpointPtr ep = std::make_shared<UDPEndpoint>(*this, handle, normSpec, conn, addr, port, incoming, listener);

    if (incoming) {
        ++num_incoming;
    }

    endpoints[handle] = ep;
    record_to_endpoint[conn] = ep;
    listener->endpoints.insert(handle);
    return ep;
}


void UDPTransport::RemoveEndpoint(ajn::ArdpConnRecord* conn)
{
    std::lock_guard<std::recursive_mutex> guard(lock);
    auto it = record_to_endpoint.find(conn);
    if (it != record_to_endpoint.end()) {
        UDPEndpointPtr ep = it->second;
        record_to_endpoint.erase(it);

        auto it2 = endpoints.find(ep->handle);
        if (it2 != endpoints.end()) {
            endpoints.erase(it2);
        }

        std::lock_guard<std::recursive_mutex>(ep->listener->lock);
        auto it3 = ep->listener->endpoints.find(ep->handle);
        if (it3 != ep->listener->endpoints.end()) {
            ep->listener->endpoints.erase(it3);
        }
    }
}

void UDPTransport::RemoveEndpoint(Endpoint::Handle handle)
{
    std::lock_guard<std::recursive_mutex> guard(lock);
    auto it = endpoints.find(handle);
    if (it != endpoints.end()) {
        UDPEndpointPtr ep = it->second;

        if (ep->incoming) {
            --num_incoming;
        }

        endpoints.erase(it);

        auto it2 = record_to_endpoint.find(ep->conn);
        if (it2 != record_to_endpoint.end()) {
            record_to_endpoint.erase(it2);
        }

        std::lock_guard<std::recursive_mutex>(ep->listener->lock);
        auto it3 = ep->listener->endpoints.find(ep->handle);
        if (it3 != ep->listener->endpoints.end()) {
            ep->listener->endpoints.erase(it3);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UDPTransport::SendWindowCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, uint16_t window, QStatus status)
{
    // not sure what to do here
    QCC_UNUSED(handle);
    QCC_UNUSED(conn);
    QCC_UNUSED(window);
    QCC_UNUSED(status);
}

void UDPTransport::SocketNotWriteable(ListenerPtr listener)
{
    std::lock_guard<std::recursive_mutex>(listener->lock);
    listener->writeable = false;
    proactor.Register(listener->write_event);
}

void UDPTransport::WriteTimeout(UDPEndpointPtr ep)
{
    std::lock_guard<std::mutex>(ep->lock);

    UDPEndpoint::Message m = ep->outgoing.front();
    auto it = ep->sentMessages.find(m.first);
    assert(it != ep->sentMessages.end());

    Endpoint::SendCompleteCB cb = it->second.second;
    MessageType msg = it->second.first;

    cb(ep, msg, ER_TIMEOUT);

    // the message has timed out, skip it!
    ep->outgoing.pop();
}

QStatus UDPTransport::DoWrite(UDPEndpointPtr ep)
{
    // TODO: pull the next tiem off the queue and send it!
    std::lock_guard<std::mutex>(ep->lock);

    // pop off the queue and send!
    UDPEndpoint::Message m = ep->outgoing.front();

    QStatus status = ajn::ARDP_Send(this->handle, ep->conn, m.first, m.second, 0);

    switch (status) {
    case ER_OK:
        {
            // SendCB will eventually happen; need to map buf -> msg, cb
            //ep->sentMessages.insert(std::make_pair(m.first, std::make_pair(msg, cb)));
            break;
        }


    case ER_WOULDBLOCK:
        // anything else to do here?
        SocketNotWriteable(ep->listener);
    // intentionally fall through!

    case ER_ARDP_BACKPRESSURE:
        // queue up the message
        break;

    default:
        break;
    }

    return status;
}

void UDPTransport::SendCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status)
{
    QCC_UNUSED(handle);
    //QCC_UNUSED(buf);
    QCC_UNUSED(len);

    UDPEndpointPtr ep = GetEndpoint(conn);
    if (!ep.get()) {
        return;
    }

    std::lock_guard<std::mutex>(ep->lock);


    auto it = ep->sentMessages.find(buf);
    assert(it != ep->sentMessages.end());

    Endpoint::SendCompleteCB cb = it->second.second;
    MessageType msg = it->second.first;

    cb(ep, msg, status);

    ep->sentMessages.erase(it);

    // it's now safe to send more data!
    ep->write_event->Signal();
}





QStatus UDPTransport::Send(Endpoint::Handle handle, MessageType msg, Endpoint::SendCompleteCB cb)
{
    QCC_UNUSED(handle);
    QCC_UNUSED(msg);
    QCC_UNUSED(cb);

    UDPEndpointPtr ep = GetEndpoint(handle);
    if (!ep.get()) {
        return ER_BUS_NO_ENDPOINT;
    }

    uint8_t* buf = nullptr;
    uint16_t len = 0;
    uint32_t ttl = 0;

    std::lock_guard<std::mutex>(ep->lock);

    //std::chrono::milliseconds timeout = 2 * ajn::ARDP_GetDataTimeout(this->handle, ep->conn);

    QStatus status = ajn::ARDP_Send(this->handle, ep->conn, buf, len, ttl);
    switch (status) {
    case ER_OK:
        {
            // SendCB will eventually happen!
            ep->sentMessages.insert(std::make_pair(buf, std::make_pair(msg, cb)));
            break;
        }

    case ER_ARDP_BACKPRESSURE:
        {
            // either queue or force caller to queue.  or drop?
            break;
        }


    case ER_WOULDBLOCK:
        {
            // TODO: anything to do here?
            // this isn't currently necesssary
            SocketNotWriteable(ep->listener);
            break;
        }

    case ER_ARDP_TTL_EXPIRED:
    // fall through
    default:
        break;
    }

    return status;
}






///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QStatus UDPTransport::Disconnect(Endpoint::Handle handle, bool force)
{
    QCC_UNUSED(handle);
    QCC_UNUSED(force);
    return ER_NOT_IMPLEMENTED;
}

// whoever is listening for ReadMessageCB must call NotifyMessageDoneCB when the app is finished with the message
// must call this again after each CB, when the listener is ready to recv again
QStatus UDPTransport::Recv(Endpoint::Handle handle, MessageType msg, Endpoint::ReadMessageCB cb)
{
    QCC_UNUSED(handle);
    QCC_UNUSED(msg);
    QCC_UNUSED(cb);
    return ER_NOT_IMPLEMENTED;
}


void UDPTransport::SocketReadable(ListenerPtr listener)
{
    uint32_t next = 0;
    std::lock_guard<std::recursive_mutex>(listener->lock);

    // we don't care about the timer anymore
    if (listener->timer_event.get()) {
        proactor.Cancel(listener->timer_event);
        listener->timer_event.reset();
    }

    if (listener->running) {
        QStatus status = ajn::ARDP_Run(handle, listener->sock, true, listener->writeable, &next);

        if (next != ajn::ARDP_NO_TIMEOUT) {
            // ARDP_Run will tell us when to call again.
            auto timeout = [this, listener] () {
                               Timeout(listener);
                           };
            listener->timer_event = std::make_shared<TimerEvent>(std::chrono::milliseconds(next), timeout);
            proactor.Register(listener->timer_event);
        }

        switch (status) {
        case ER_OK:
            break;

        case ER_ARDP_WRITE_BLOCKED:
            SocketNotWriteable(listener);
            break;

        default:
            proactor.Cancel(listener->read_event);
            listener->read_event.reset();
            break;
        }
    }
}

void UDPTransport::Timeout(ListenerPtr listener)
{
    std::lock_guard<std::recursive_mutex>(listener->lock);

    if (listener->running) {
        uint32_t next = 0;
        QStatus status = ajn::ARDP_Run(handle, listener->sock, false, false, &next);

        if (next != ajn::ARDP_NO_TIMEOUT) {
            auto timeout = [this, listener] () {
                               Timeout(listener);
                           };
            listener->timer_event = std::make_shared<TimerEvent>(std::chrono::milliseconds(next), timeout);
            proactor.Register(listener->timer_event);
        }

        switch (status) {
        case ER_OK:
            break;

        case ER_ARDP_WRITE_BLOCKED:
            SocketNotWriteable(listener);
            break;

        default:
            proactor.Cancel(listener->read_event);
            listener->read_event.reset();
            break;
        }
    }
}

void UDPTransport::SocketWriteable(ListenerPtr listener)
{
    std::lock_guard<std::recursive_mutex>(listener->lock);

    if (listener->timer_event.get()) {
        proactor.Cancel(listener->timer_event);
        listener->timer_event.reset();
    }

    listener->writeable = true;
    proactor.Cancel(listener->write_event);
    uint32_t next = 0;
    QStatus status = ajn::ARDP_Run(handle, listener->sock, false, true, &next);

    if (next != ajn::ARDP_NO_TIMEOUT) {
        auto timeout = [this, listener] () {
                           Timeout(listener);
                       };
        listener->timer_event = std::make_shared<TimerEvent>(std::chrono::milliseconds(next), timeout);
        proactor.Register(listener->timer_event);
    }

    switch (status) {
    case ER_OK:
        break;

    case ER_ARDP_WRITE_BLOCKED:
        SocketNotWriteable(listener);
        break;

    default:
        proactor.Cancel(listener->read_event);
        listener->read_event.reset();
        break;
    }
}

QStatus UDPTransport::Listen(const std::string& spec, AcceptedCB cb)
{
    ListenEndpoints eps;
    bool found = false;

    if (!ParseSpec(spec, eps)) {
        return ER_BUS_BAD_TRANSPORT_ARGS;
    }

    std::lock_guard<std::recursive_mutex> guard(lock);

    for (const qcc::IPEndpoint& ipep : eps) {
        const std::string normSpec = NormalizeConnectionData(ipep.addr, ipep.port);
        if (listeners.find(normSpec) != listeners.end()) {
            return ER_BUS_ALREADY_LISTENING;
        }

        ListenerPtr listener = std::make_shared<Listener>();
        listener->cb = cb;
        listener->addr = ipep.addr;
        listener->port = ipep.port;
        QStatus status = qcc::Socket(ipep.addr.GetAddressFamily(), qcc::QCC_SOCK_DGRAM, listener->sock);
        if (status != ER_OK) {
            return status;
        }

        status = qcc::SetReusePort(listener->sock, true);
        if (status != ER_OK) {
            return status;
        }

        status = qcc::SetBlocking(listener->sock, false);
        if (status != ER_OK) {
            return status;
        }

        status = qcc::Bind(listener->sock, ipep.addr, ipep.port);
        if (status != ER_OK) {
            return status;
        }

        std::lock_guard<std::recursive_mutex>(listener->lock);

        auto sock_readable = [this, listener] (qcc::SocketFd, QStatus) {
                                 SocketReadable(listener);
                             };
        listener->read_event = std::make_shared<SocketReadableEvent>(listener->sock, sock_readable);
        proactor.Register(listener->read_event);

        auto sock_writeable = [this, listener] (qcc::SocketFd, QStatus) {
                                  SocketWriteable(listener);
                              };
        listener->write_event = std::make_shared<SocketWriteableEvent>(listener->sock, sock_writeable);

        found = true;
        listeners[normSpec] = listener;
    }

    return (found ? ER_OK : ER_BUS_NO_LISTENER);
}

QStatus UDPTransport::StopListen(const std::string& spec)
{
    ListenEndpoints eps;

    if (!ParseSpec(spec, eps)) {
        return ER_BUS_BAD_TRANSPORT_ARGS;
    }

    for (const qcc::IPEndpoint& ipep : eps) {
        const std::string normSpec = NormalizeConnectionData(ipep.addr, ipep.port);

        std::lock_guard<std::recursive_mutex> guard(lock);
        auto it = listeners.find(normSpec);
        if (it != listeners.end()) {
            ListenerPtr listener = it->second;
            std::lock_guard<std::recursive_mutex>(listener->lock);
            listener->running = false;

            if (listener->read_event.get()) {
                proactor.Cancel(listener->read_event);
                listener->read_event.reset();
            }

            if (listener->write_event.get()) {
                proactor.Cancel(listener->write_event);
                listener->write_event.reset();
            }

            if (listener->timer_event.get()) {
                proactor.Cancel(listener->timer_event);
                listener->timer_event.reset();
            }

            qcc::Close(listener->sock);
            listeners.erase(it);
        }
    }
    return ER_OK;
}


UDPTransport::ListenerPtr UDPTransport::GetListenerForConnection(const qcc::IPAddress& addr)
{
    ListenerPtr listener;

    std::vector<qcc::IfConfigEntry> entries;
    QStatus status = qcc::IfConfig(entries);
    if (ER_OK != status) {
        return listener;
    }

    for (auto it : listeners) {
        ListenerPtr listener = it.second;

        if (listener->addr.IsIPv4()) {
            if (listener->addr.ToString() == "0.0.0.0") {
                return listener;
            } else if (addr.IsIPv6()) {
                continue;
            }
        } else if (listener->addr.IsIPv6()) {
            if (listener->addr.ToString() == "0::0") {
                return listener;
            } else if (addr.IsIPv4()) {
                continue;
            }
        }

        uint32_t prefixLen = 0;
        for (const qcc::IfConfigEntry& entry : entries) {
            if (entry.m_addr == listener->addr.ToString()) {
                prefixLen = entry.m_prefixlen;
            }
        }

        if (addr.IsIPv4()) {
            uint32_t mask = 0;
            for (uint32_t j = 0; j < prefixLen; ++j) {
                mask >>= 1;
                mask |= 0x80000000;
            }

            uint32_t network1 = listener->addr.GetIPv4AddressCPUOrder() & mask;
            uint32_t network2 = addr.GetIPv4AddressCPUOrder() & mask;
            if (network1 == network2) {
                return listener;
            }
        } else if (addr.IsIPv6()) {
            uint8_t mask[qcc::IPAddress::IPv6_SIZE];
            ::memset(mask, 0, sizeof(mask));
            assert(prefixLen < 128);

            for (uint32_t i = 0; i < prefixLen; ++i) {
                const uint32_t byte = i / 8;
                const uint32_t bits = i % 8;
                mask[byte] = (1 << bits);
            }

            uint8_t network1[qcc::IPAddress::IPv6_SIZE];
            listener->addr.RenderIPv6Binary(network1, sizeof(network1));

            uint8_t network2[qcc::IPAddress::IPv6_SIZE];
            addr.RenderIPv6Binary(network1, sizeof(network1));

            // apply the net mask to both!
            for (uint8_t i = 0; i < 16; ++i) {
                network1[i] = network1[i] & mask[i];
                network2[i] = network2[i] & mask[i];
            }

            if (::memcmp(network1, network2, qcc::IPAddress::IPv6_SIZE) == 0) {
                return listener;
            }
        }
    }

    return listener;
}

UDPTransport::ListenerPtr UDPTransport::GetListener(const qcc::IPAddress& addr, uint16_t port)
{
    std::shared_ptr<UDPTransport::Listener> listener;
    std::lock_guard<std::recursive_mutex> guard(lock);

    auto it = listeners.find(NormalizeConnectionData(addr, port));
    if (it != listeners.end()) {
        listener = it->second;
    }

    return listener;
}

QStatus UDPTransport::Connect(const std::string& spec, ConnectedCB cb)
{
    qcc::IPAddress addr;
    uint16_t port;
    std::string normSpec;
    if (!ParseSpec(spec, addr, port, normSpec)) {
        return ER_BUS_BAD_TRANSPORT_ARGS;
    }


    ajn::ArdpConnRecord* conn = nullptr;

    // TODO: get the initial data
    uint8_t* buf = nullptr;
    uint16_t len = 0;

    std::lock_guard<std::recursive_mutex> guard(lock);
    std::shared_ptr<UDPTransport::Listener> listener = GetListenerForConnection(addr);
    if (!listener.get()) {
        // we don't have a socket bound to the network we want!
        return ER_BUS_NOT_CONNECTED;
    }

    // final param: context ==> conn->context;
    QStatus status = ARDP_Connect(handle, listener->sock, addr, port, config.segmax, config.segbmax, &conn, buf, len, nullptr);
    if (status != ER_OK) {
        return status;
    }

    outgoing_connections[conn] = cb;

    UDPEndpointPtr ep = CreateEndpoint(conn, addr, port, false, listener, normSpec);
    listener->endpoints.insert(ep->handle);
    return ER_OK;
}

void UDPTransport::MakeConnectedCallback(ajn::ArdpConnRecord* conn, UDPEndpointPtr ep, QStatus status)
{
    auto it = outgoing_connections.find(conn);
    if (it != outgoing_connections.end()) {
        ConnectedCB cb = it->second;
        cb(ep, status);
        outgoing_connections.erase(it);
    }
}

void UDPTransport::ReadyToRead(UDPEndpointPtr ep)
{
    QCC_UNUSED(ep);
}



void UDPTransport::ConnectCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status)
{
    QCC_UNUSED(handle);
    QCC_UNUSED(buf);
    QCC_UNUSED(len);

    UDPEndpointPtr ep = GetEndpoint(conn);

    if (status != ER_OK) {
        if (!passive) {
            MakeConnectedCallback(conn, ep, status);
            RemoveEndpoint(conn);
        }
    } else {
        if (!passive) {
            // check the buffer for something valid!
            MakeConnectedCallback(conn, ep, ER_OK);
        }

        auto read_func = [this, ep] () {
                             ReadyToRead(ep);
                         };
        ep->read_event = std::make_shared<EventNotifier>(read_func);
        auto write_func = [this, ep] () {
                              DoWrite(ep);
                          };
        ep->write_event = std::make_shared<EventNotifier>(write_func);

        proactor.Register(ep->read_event);
        proactor.Register(ep->write_event);
    }
}




bool UDPTransport::AcceptCb(ajn::ArdpHandle* handle, qcc::IPAddress addr, uint16_t port, ajn::ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status)
{
    QCC_UNUSED(buf);
    QCC_UNUSED(len);

    std::lock_guard<std::recursive_mutex> guard(lock);
    ListenerPtr listener = GetListener(addr, port);
    std::lock_guard<std::recursive_mutex>(listener->lock);
    if (!listener.get()) {
        return false;
    }

    std::string normSpec = NormalizeConnectionData(addr, port);

    if (status == ER_OK) {
        UDPEndpointPtr ep = CreateEndpoint(conn, addr, port, true, listener, normSpec);

        if (listener->cb(ep)) {
            uint8_t* buf = nullptr;
            uint16_t len = 0;

            //QStatus ARDP_Accept(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t segmax, uint16_t segbmax, uint8_t* buf, uint16_t len);
            status = ARDP_Accept(handle, conn, config.segmax, config.segbmax, buf, len);
            return true;
        } else {
            RemoveEndpoint(ep->handle);
        }
    }


    return false;
}

void UDPTransport::DisconnectCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, QStatus status)
{
    QCC_UNUSED(handle);
    QCC_UNUSED(conn);
    QCC_UNUSED(status);
}

void UDPTransport::RecvCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, ajn::ArdpRcvBuf* rcv, QStatus status)
{
    QCC_UNUSED(handle);
    QCC_UNUSED(conn);
    QCC_UNUSED(rcv);
    QCC_UNUSED(status);
}








// the following functions are static callbacks.  They are called by ARDP and plumb the data back to the UDPTransport object.
void UDPTransport::ArdpConnectCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status)
{
    UDPTransport* transport = static_cast<UDPTransport*>(ajn::ARDP_GetHandleContext(handle));
    transport->ConnectCb(handle, conn, passive, buf, len, status);
}

bool UDPTransport::ArdpAcceptCb(ajn::ArdpHandle* handle, qcc::IPAddress ipAddr, uint16_t ipPort, ajn::ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status)
{
    UDPTransport* transport = static_cast<UDPTransport*>(ajn::ARDP_GetHandleContext(handle));
    return transport->AcceptCb(handle, ipAddr, ipPort, conn, buf, len, status);
}

void UDPTransport::ArdpDisconnectCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, QStatus status)
{
    UDPTransport* transport = static_cast<UDPTransport*>(ajn::ARDP_GetHandleContext(handle));
    transport->DisconnectCb(handle, conn, status);
}

void UDPTransport::ArdpRecvCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, ajn::ArdpRcvBuf* rcv, QStatus status)
{
    UDPTransport* transport = static_cast<UDPTransport*>(ajn::ARDP_GetHandleContext(handle));
    transport->RecvCb(handle, conn, rcv, status);
}

void UDPTransport::ArdpSendCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status)
{
    UDPTransport* transport = static_cast<UDPTransport*>(ajn::ARDP_GetHandleContext(handle));
    transport->SendCb(handle, conn, buf, len, status);
}

void UDPTransport::ArdpSendWindowCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, uint16_t window, QStatus status)
{
    UDPTransport* transport = static_cast<UDPTransport*>(ajn::ARDP_GetHandleContext(handle));
    transport->SendWindowCb(handle, conn, window, status);
}
