/**
 * @file
 * NamedPipeClientTransport is a specialization of Transport that connects to
 * Daemon on a named pipe for Windows
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Windows/NamedPipeStream.h>
#include <alljoyn/BusAttachment.h>

#include "BusInternal.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "ClientTransport.h"
#include "NamedPipeClientTransport.h"

#if (_WIN32_WINNT > 0x0603)
#include <MSAJTransport.h>
#endif

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

#if (_WIN32_WINNT > 0x0603)
const char* NamedPipeClientTransport::TransportName = "npipe";

class _NamedPipeClientEndpoint;

typedef ManagedObj<_NamedPipeClientEndpoint> ClientEndpoint;

class _NamedPipeClientEndpoint : public _RemoteEndpoint {
  public:
    _NamedPipeClientEndpoint(NamedPipeClientTransport* transport, BusAttachment& bus, const qcc::String connectSpec,
                             HANDLE clientHandle) :
        _RemoteEndpoint(bus, false, connectSpec, &m_stream, NamedPipeClientTransport::TransportName, false),
        m_transport(transport),
        m_stream(clientHandle)
    {
    }

    ~_NamedPipeClientEndpoint()
    {
    }

    qcc::NamedPipeStream m_stream;

  protected:
    NamedPipeClientTransport* m_transport;
};
#else
const char* NamedPipeClientTransport::TransportName = NULL;
#endif

QStatus NamedPipeClientTransport::IsConnectSpecValid(const char* connectSpec)
{
#if (_WIN32_WINNT > 0x0603)
    /*
     * The string in connectSpec, must start with "npipe:"
     */
    qcc::String tpNameStr("npipe:");
    qcc::String argStr(connectSpec);

    size_t pos = argStr.find(tpNameStr);
    if (pos != 0) {
        return ER_BUS_BAD_TRANSPORT_ARGS;
    } else {
        return ER_OK;
    }
#else
    return ER_FAIL;
#endif
}

QStatus NamedPipeClientTransport::Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep)
{
#if (_WIN32_WINNT > 0x0603)
    QCC_DbgHLPrintf(("NamedPipeClientTransport::Connect(): %s", connectSpec));

    if (!m_running) {
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }
    if (m_endpoint->IsValid()) {
        return ER_BUS_ALREADY_CONNECTED;
    }

    /*
     * Parse and check if the connectSpec is valid.
     */
    QStatus status;
    status = IsConnectSpecValid(connectSpec);

    if (status != ER_OK) {
        QCC_LogError(status, ("NamedPipeClientTransport::Connect(): Bad transport argument. It must be 'npipe:' (without quotes)"));
        return status;
    }

    HANDLE clientHandle = INVALID_HANDLE_VALUE;
    BOOL success = FALSE;
    DWORD bytesWritten;
    DWORD lastError;

    /*
     * Connect to the server via named pipe.
     */
    wchar_t* wideConnectSpec = MultibyteToWideString(connectSpec);
    if (NULL == wideConnectSpec) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("NamedPipeClientTransport::Connect(): could not create pipe connection. Invalid Handle Value \n"));
        return status;
    }
    clientHandle = AllJoynConnectToBus(wideConnectSpec);
    status = ER_OS_ERROR;
    lastError = ::GetLastError();

    delete[] wideConnectSpec;

    /*
     * Break if the client handle is invalid.
     */
    if (clientHandle == INVALID_HANDLE_VALUE) {
        QCC_LogError(status, ("NamedPipeClientTransport::Connect(): could not create pipe connection. Invalid Handle Value (0x%08X)", lastError));
        return status;
    }

    status = ER_OK;

    /*
     * We have a connection established, but the DBus wire protocol requires that every connection,
     * irrespective of transport, start with a single zero byte.
     */
    uint8_t nul = 0;
    size_t sent;

    success = AllJoynSendToBus(
        clientHandle,           // bus handle
        &nul,                   // message
        1,                      // message length
        &bytesWritten,          // bytes written
        nullptr);               // no overlapped

    if (!success) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("NamedPipeClientTransport::Connect(): WriteFile to pipe failed (0x%08X).", ::GetLastError()));
        AllJoynCloseBusHandle(clientHandle);
        return status;
    }

    /*
     * The underlying transport mechanism is started, but we need to create a
     * ClientEndpoint object that will orchestrate the movement of data across the
     * transport.
     */
    ClientEndpoint ep(this, m_bus, connectSpec, clientHandle);

    /*
     * Initialize the features for this endpoint.
     */
    ep->GetFeatures().isBusToBus = false;
    ep->GetFeatures().allowRemote = m_bus.GetInternal().AllowRemoteMessages();
    ep->GetFeatures().handlePassing = false;

    qcc::String authName;
    qcc::String redirection;
    status = ep->Establish("ANONYMOUS", authName, redirection);
    if (status == ER_OK) {
        ep->SetListener(this);
        status = ep->Start();
        if (status != ER_OK) {
            QCC_LogError(status, ("NamedPipeClientTransport::Connect(): Start ClientEndpoint failed"));
        }
    }

    /*
     * If we got an error, we need to clean up the pipe handle and zero out the
     * returned endpoint.  If we got this done without a problem, we return
     * a pointer to the new endpoint. We do not close the pipe handle since the
     * endpoint that was created is responsible for doing so.
     */
    if (status != ER_OK) {
        ep->Invalidate();
    } else {
        newep = BusEndpoint::cast(ep);
        m_endpoint = RemoteEndpoint::cast(ep);
    }
    return status;
#else
    return ER_FAIL;
#endif
}


NamedPipeClientTransport::NamedPipeClientTransport(BusAttachment& bus) : m_bus(bus), m_listener(0)
{
}

NamedPipeClientTransport::~NamedPipeClientTransport()
{
    Stop();
    Join();
}

QStatus NamedPipeClientTransport::Start()
{
#if (_WIN32_WINNT > 0x0603)
    m_running = true;
    return ER_OK;
#else
    return ER_FAIL;
#endif
}

QStatus NamedPipeClientTransport::Stop(void)
{
#if (_WIN32_WINNT > 0x0603)
    m_running = false;
    m_endpoint->Stop();
    return ER_OK;
#else
    return ER_FAIL;
#endif
}

QStatus NamedPipeClientTransport::Join(void)
{
#if (_WIN32_WINNT > 0x0603)
    /* Join the endpoint i.e. wait for the EndpointExit callback to complete */
    m_endpoint->Join();
    m_endpoint = RemoteEndpoint();
    return ER_OK;
#else
    return ER_FAIL;
#endif
}

void NamedPipeClientTransport::EndpointExit(RemoteEndpoint& ep)
{
#if (_WIN32_WINNT > 0x0603)
    assert(ep == m_endpoint);
    QCC_DbgTrace(("NamedPipeClientTransport::EndpointExit()"));
    m_endpoint->Invalidate();
#endif
}

QStatus NamedPipeClientTransport::Disconnect(const char* connectSpec)
{
#if (_WIN32_WINNT > 0x0603)
    QCC_DbgHLPrintf(("NamedPipeClientTransport::Disconnect(): %s", connectSpec));

    if (!m_endpoint->IsValid()) {
        return ER_BUS_NOT_CONNECTED;
    }

    /*
     * Higher level code tells us which connection is used by giving us the
     * same connect spec it used in the Connect() call.
     */
    QStatus status = NamedPipeClientTransport::IsConnectSpecValid(connectSpec);
    if (ER_OK != status) {
        QCC_LogError(status, ("NamedPipeClientTransport::Disconnect(): Invalid connect spec \"%s\"", connectSpec));
    } else {
        m_endpoint->Stop();
        m_endpoint->Join();
        m_endpoint = RemoteEndpoint();
    }
    return status;
#else
    return ER_FAIL;
#endif
}

} // namespace ajn