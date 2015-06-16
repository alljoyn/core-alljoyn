/*
 * TCPTransport.h
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
#ifndef TCPTRANSPORT_H_
#define TCPTRANSPORT_H_

#include "TransportBase.h"
#include "IPTransport.h"

#include <mutex>
#include <map>
#include <list>

#include <qcc/IfConfig.h>
#include <qcc/Socket.h>

namespace nio {

class Proactor;

class TCPTransport : public IPTransport {

    class TCPEndpoint;

  public:

    static const std::string TransportName;

    TCPTransport(Proactor& proacator);

    TCPTransport(const TCPTransport&) = delete;
    TCPTransport& operator=(const TCPTransport&) = delete;

    ~TCPTransport();


    QStatus Send(Endpoint::Handle handle, MessageType msg, Endpoint::SendCompleteCB cb) override;
    QStatus Recv(Endpoint::Handle handle, MessageType msg, Endpoint::ReadMessageCB cb) override;

    QStatus Connect(const std::string& spec, ConnectedCB cb) override;
    QStatus Disconnect(Endpoint::Handle handle, bool force = false) override;


    QStatus Listen(const std::string& spec, AcceptedCB cb) override;
    QStatus StopListen(const std::string& spec) override;

  private:

    typedef std::shared_ptr<TCPEndpoint> TCPEndpointPtr;

    void EndpointDisconnected(TCPEndpointPtr ep);
    void CloseEndpoint(TCPEndpointPtr ep);

    TCPEndpointPtr CreateEndpoint(qcc::SocketFd sock, const qcc::IPAddress& addr, uint16_t port, bool incoming, const std::string& spec);
    TCPEndpointPtr GetEndpoint(Endpoint::Handle handle) const;
    void RemoveEndpoint(Endpoint::Handle handle);

    void WriteCallback(TCPEndpointPtr ep, MessageType msg, Endpoint::SendCompleteCB cb);
    void ReadCallback(TCPEndpointPtr ep, MessageType msg, Endpoint::ReadMessageCB cb);

    void AcceptCallback(qcc::SocketFd sock, AcceptedCB cb);

    uint32_t GetNumIncoming() const;


    typedef std::map<Endpoint::Handle, TCPEndpointPtr> EndpointMap;
    EndpointMap endpoints;
    mutable std::mutex endpoints_lock;
    uint32_t num_incoming;


    typedef std::map<std::string, TCPEndpointPtr> ListenMap;
    ListenMap listeners;
    std::mutex listeners_lock;

    // configuration
    uint32_t max_tcp_connections;
    uint32_t tcp_connect_timeout;
    uint32_t tcp_listen_backlog;
};

} /* namespace nio */

#endif /* TCPTRANSPORT_H_ */
