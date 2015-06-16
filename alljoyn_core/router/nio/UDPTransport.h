/*
 * UDPTransport.h
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
#ifndef UDPTransport_H_
#define UDPTransport_H_

#include "IPTransport.h"
#include "SocketReadableEvent.h"
#include "SocketWriteableEvent.h"
#include "TimerEvent.h"

#include <ArdpProtocol.h>

#include <set>

namespace nio {

class Proactor;

class UDPTransport : public IPTransport {

    class UDPEndpoint;

  public:

    static const std::string TransportName;

    UDPTransport(Proactor& proactor);

    UDPTransport(const UDPTransport&) = delete;
    UDPTransport& operator=(const UDPTransport&) = delete;

    virtual ~UDPTransport();

    QStatus Send(Endpoint::Handle handle, MessageType msg, Endpoint::SendCompleteCB cb) override;

    // whoever is listening for ReadMessageCB must call NotifyMessageDoneCB when the app is finished with the message
    // must call this again after each CB, when the listener is ready to recv again
    QStatus Recv(Endpoint::Handle handle, MessageType msg, Endpoint::ReadMessageCB cb) override;
    //QStatus UnregisterMessageRecvCB(Endpoint::Handle handle) override;


    QStatus Connect(const std::string& spec, ConnectedCB cb) override;
    QStatus Disconnect(Endpoint::Handle handle, bool force = false) override;


    QStatus Listen(const std::string& spec, AcceptedCB cb) override;
    QStatus StopListen(const std::string& spec) override;

  private:

    struct Listener {
        std::recursive_mutex lock;
        qcc::SocketFd sock;
        qcc::IPAddress addr;
        uint16_t port;

        AcceptedCB cb;
        std::shared_ptr<SocketReadableEvent> read_event;
        std::shared_ptr<SocketWriteableEvent> write_event;
        std::shared_ptr<TimerEvent> timer_event;

        bool writeable = true; // assume we are writeable unless ARDP tells us otherwise
        bool running = true;

        std::set<Endpoint::Handle> endpoints;
    };

    typedef std::shared_ptr<Listener> ListenerPtr;
    typedef std::shared_ptr<UDPEndpoint> UDPEndpointPtr;

    void ReadyToRead(UDPEndpointPtr ep);
    QStatus DoWrite(UDPEndpointPtr ep);
    void WriteTimeout(UDPEndpointPtr ep);

    void SocketReadable(std::shared_ptr<Listener> listener);
    void SocketWriteable(std::shared_ptr<Listener> listener);
    void SocketNotWriteable(ListenerPtr listener);
    void Timeout(std::shared_ptr<Listener> listener);

    void ConnectCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status);
    bool AcceptCb(ajn::ArdpHandle* handle, qcc::IPAddress addr, uint16_t port, ajn::ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status);
    void DisconnectCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, QStatus status);
    void RecvCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, ajn::ArdpRcvBuf* rcv, QStatus status);
    void SendCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status);
    void SendWindowCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, uint16_t window, QStatus status);

    // ARDP calls these
    static bool ArdpAcceptCb(ajn::ArdpHandle* handle, qcc::IPAddress ipAddr, uint16_t ipPort, ajn::ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status);
    static void ArdpDisconnectCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, QStatus status);
    static void ArdpRecvCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, ajn::ArdpRcvBuf* rcv, QStatus status);
    static void ArdpConnectCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status);
    static void ArdpSendCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status);
    static void ArdpSendWindowCb(ajn::ArdpHandle* handle, ajn::ArdpConnRecord* conn, uint16_t window, QStatus status);



    UDPEndpointPtr CreateEndpoint(ajn::ArdpConnRecord* conn, const qcc::IPAddress& addr, uint16_t port, bool incoming, ListenerPtr listener, const std::string& normSpec);
    UDPEndpointPtr GetEndpoint(Endpoint::Handle handle) const;
    UDPEndpointPtr GetEndpoint(ajn::ArdpConnRecord* conn) const;
    void RemoveEndpoint(Endpoint::Handle handle);
    void RemoveEndpoint(ajn::ArdpConnRecord* conn);

    ListenerPtr GetListenerForConnection(const qcc::IPAddress& addr);
    ListenerPtr GetListener(const qcc::IPAddress& addr, uint16_t port);

    void MakeConnectedCallback(ajn::ArdpConnRecord* conn, UDPEndpointPtr ep, QStatus status);

    ajn::ArdpGlobalConfig config;
    ajn::ArdpHandle* handle;

    std::map<Endpoint::Handle, UDPEndpointPtr> endpoints;
    std::map<ajn::ArdpConnRecord*, UDPEndpointPtr> record_to_endpoint;
    std::map<std::string, ListenerPtr> listeners;
    std::map<ajn::ArdpConnRecord*, ConnectedCB> outgoing_connections;

    mutable std::recursive_mutex lock;



    uint32_t num_incoming;








};

} /* namespace nio */

#endif /* UDPTransport_H_ */
