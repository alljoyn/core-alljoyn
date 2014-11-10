/**
 * @file
 * ClientTransport methods that are common to all implementations
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/BusAttachment.h>

#include "BusInternal.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "ClientTransport.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

ClientTransport::ClientTransport(BusAttachment& bus) : m_bus(bus), m_listener(0)
{
}

ClientTransport::~ClientTransport()
{
    Stop();
    Join();
}

QStatus ClientTransport::Start()
{
    m_running = true;
    return ER_OK;
}

QStatus ClientTransport::Stop(void)
{
    m_running = false;
    /* Stop the endpoint */
    m_endpoint->Stop();
    return ER_OK;
}

QStatus ClientTransport::Join(void)
{
    /* Join the endpoint i.e. wait for the EndpointExit callback to complete */
    m_endpoint->Join();
    m_endpoint = RemoteEndpoint();
    return ER_OK;
}

void ClientTransport::EndpointExit(RemoteEndpoint& ep)
{
    assert(ep == m_endpoint);
    QCC_DbgTrace(("ClientTransport::EndpointExit()"));
    m_endpoint->Invalidate();
}

QStatus ClientTransport::Disconnect(const char* connectSpec)
{
    QCC_DbgHLPrintf(("ClientTransport::Disconnect(): %s", connectSpec));

    if (!m_endpoint->IsValid()) {
        return ER_BUS_NOT_CONNECTED;
    }
    /*
     * Higher level code tells us which connection is refers to by giving us the
     * same connect spec it used in the Connect() call.  We have to determine the
     * address and port in exactly the same way
     */
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    QStatus status = NormalizeTransportSpec(connectSpec, normSpec, argMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("ClientTransport::Disconnect(): Invalid connect spec \"%s\"", connectSpec));
    } else {
        m_endpoint->Stop();
        m_endpoint->Join();
        m_endpoint = RemoteEndpoint();
    }
    return status;
}

} // namespace ajn
