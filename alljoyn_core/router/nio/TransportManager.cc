/*
 * TransportManager.cc
 *
 *  Created on: Jun 3, 2015
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
#include "TransportManager.h"
#include "TransportBase.h"
#include "TCPTransport.h"
#include "UDPTransport.h"

using namespace nio;

TransportManager::TransportManager(Proactor& proactor) : proactor(proactor)
{

}

TransportManager::~TransportManager()
{

}

static std::string GetTransportName(const std::string& spec)
{
    size_t col = spec.find(':');
    assert(col != std::string::npos);
    return spec.substr(0, col);
}

TransportBase* TransportManager::CreateTransport(const std::string& name)
{
    if (name == TCPTransport::TransportName) {
        return new TCPTransport(proactor);
    } else if (name == UDPTransport::TransportName) {
        return new UDPTransport(proactor);
    } else {
        printf("Transport [%s] not found\n", name.c_str());
        return nullptr;
    }
}

TransportBase* TransportManager::GetTransport(const std::string& spec)
{
    TransportBase* transport = nullptr;

    std::string name = GetTransportName(spec);

    std::lock_guard<std::mutex> g(transports_lock);
    auto it = transports.find(name);
    if (it != transports.end()) {
        transport = it->second;
    } else {
        transport = CreateTransport(name);

        if (transport) {
            transports[name] = transport;
        }
    }

    return transport;
}

QStatus TransportManager::Listen(const std::string& spec, TransportBase::AcceptedCB cb)
{
    QStatus status = ER_BUS_TRANSPORT_NOT_AVAILABLE;
    TransportBase* transport = GetTransport(spec);
    if (transport) {
        status = transport->Listen(spec, cb);
    }

    return status;
}

QStatus TransportManager::StopListen(const std::string& spec)
{
    QStatus status = ER_BUS_TRANSPORT_NOT_AVAILABLE;
    TransportBase* transport = GetTransport(spec);
    if (transport) {
        status = transport->StopListen(spec);
    }

    return status;
}

QStatus TransportManager::Connect(const std::string& spec, TransportBase::ConnectedCB cb)
{
    QStatus status = ER_BUS_TRANSPORT_NOT_AVAILABLE;
    TransportBase* transport = GetTransport(spec);
    if (transport) {
        return transport->Connect(spec, cb);
    }

    return status;
}
