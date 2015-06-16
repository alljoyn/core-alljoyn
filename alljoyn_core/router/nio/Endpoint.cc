/*
 * Endpoint.cc
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
#include "Endpoint.h"

#include "TransportBase.h"

namespace nio {

const Endpoint::Handle Endpoint::INVALID_HANDLE = 0;

Endpoint::Endpoint(TransportBase& transport, Handle handle, const std::string& spec) :
    transport(transport), handle(handle), spec(spec)
{
}

Endpoint::~Endpoint()
{
}

QStatus Endpoint::Send(MessageType msg, SendCompleteCB cb)
{
    return transport.Send(handle, msg, cb);
}

QStatus Endpoint::Recv(MessageType msg, ReadMessageCB cb)
{
    return transport.Recv(handle, msg, cb);
}
/*
   QStatus Endpoint::UnregisterMessageRecvCB()
   {
    return transport.UnregisterMessageRecvCB(handle);
   }*/

void Endpoint::Disconnect(bool force)
{
    transport.Disconnect(handle, force);
}

std::string Endpoint::ToString() const
{
    return spec;
}

} /* namespace nio */
