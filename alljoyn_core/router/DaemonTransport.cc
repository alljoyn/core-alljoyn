/**
 * @file
 * Platform independent methods for DaemonTransport.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014 AllSeen Alliance. All rights reserved.
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

#include <errno.h>

#include <qcc/platform.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <alljoyn/BusAttachment.h>

#include "BusInternal.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "DaemonTransport.h"
#include "ConfigDB.h"

#define QCC_MODULE "DAEMON_TRANSPORT"

using namespace std;
using namespace qcc;

namespace ajn {


DaemonTransport::DaemonTransport(BusAttachment& bus)
    : Thread("DaemonTransport"), bus(bus), stopping(false)
{
    /*
     * We know we are daemon code, so we'd better be running with a daemon
     * router.  This is assumed elsewhere.
     */
    assert(bus.GetInternal().GetRouter().IsDaemon());
}

DaemonTransport::~DaemonTransport()
{
    Stop();
    Join();
}

QStatus DaemonTransport::Start()
{
    stopping = false;

    ConfigDB* config = ConfigDB::GetConfigDB();
    m_minHbeatIdleTimeout = config->GetLimit("dt_min_idle_timeout", MIN_HEARTBEAT_IDLE_TIMEOUT_DEFAULT);
    m_maxHbeatIdleTimeout = config->GetLimit("dt_max_idle_timeout", MAX_HEARTBEAT_IDLE_TIMEOUT_DEFAULT);
    m_defaultHbeatIdleTimeout = config->GetLimit("dt_default_idle_timeout", DEFAULT_HEARTBEAT_IDLE_TIMEOUT_DEFAULT);

    m_numHbeatProbes = HEARTBEAT_NUM_PROBES;
    m_maxHbeatProbeTimeout = config->GetLimit("dt_max_probe_timeout", MAX_HEARTBEAT_PROBE_TIMEOUT_DEFAULT);
    m_defaultHbeatProbeTimeout = config->GetLimit("dt_default_probe_timeout", DEFAULT_HEARTBEAT_PROBE_TIMEOUT_DEFAULT);

    QCC_DbgPrintf(("DaemonTransport: Using m_minHbeatIdleTimeout=%u, m_maxHbeatIdleTimeout=%u, m_numHbeatProbes=%u, m_defaultHbeatProbeTimeout=%u m_maxHbeatProbeTimeout=%u", m_minHbeatIdleTimeout, m_maxHbeatIdleTimeout, m_numHbeatProbes, m_defaultHbeatProbeTimeout, m_maxHbeatProbeTimeout));

    return ER_OK;
}

QStatus DaemonTransport::Stop(void)
{
    stopping = true;

    /*
     * Tell the server accept loop thread to shut down through the thread
     * base class.
     */
    QStatus status = Thread::Stop();
    if (status != ER_OK) {
        QCC_LogError(status, ("DaemonDaemonTransport::Stop(): Failed to Stop() server thread"));
        return status;
    }

    endpointListLock.Lock(MUTEX_CONTEXT);

    /*
     * Ask any running endpoints to shut down and exit their threads.
     */
    for (list<RemoteEndpoint>::iterator i = endpointList.begin(); i != endpointList.end(); ++i) {
        (*i)->Stop();
    }

    endpointListLock.Unlock(MUTEX_CONTEXT);

    return ER_OK;
}

QStatus DaemonTransport::Join(void)
{
    /*
     * Wait for the server accept loop thread to exit.
     */
    QStatus status = Thread::Join();
    if (status != ER_OK) {
        QCC_LogError(status, ("DaemonTransport::Join(): Failed to Join() server thread"));
        return status;
    }

    /*
     * A call to Stop() above will ask all of the endpoints to stop.  We still
     * need to wait here until all of the threads running in those endpoints
     * actually stop running.  When a remote endpoint thead exits the endpoint
     * will call back into our EndpointExit() and have itself removed from the
     * list.  We poll for the all-exited condition, yielding the CPU to let
     * the endpoint therad wake and exit.
     */
    endpointListLock.Lock(MUTEX_CONTEXT);
    while (endpointList.size() > 0) {
        endpointListLock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(50);
        endpointListLock.Lock(MUTEX_CONTEXT);
    }
    endpointListLock.Unlock(MUTEX_CONTEXT);

    stopping = false;

    return ER_OK;
}

void DaemonTransport::EndpointExit(RemoteEndpoint& ep)
{
    /*
     * This is a callback driven from the remote endpoint thread exit function.
     * Our DaemonEndpoint inherits from class RemoteEndpoint and so when
     * either of the threads (transmit or receive) of one of our endpoints exits
     * for some reason, we get called back here.
     */
    QCC_DbgTrace(("DaemonTransport::EndpointExit()"));

    /* Remove the dead endpoint from the live endpoint list */
    endpointListLock.Lock(MUTEX_CONTEXT);
    list<RemoteEndpoint>::iterator i = find(endpointList.begin(), endpointList.end(), ep);
    if (i != endpointList.end()) {
        endpointList.erase(i);
    } else {
        QCC_LogError(ER_FAIL, ("DaemonTransport::EndpointExit() endpoint missing from endpointList"));
    }
    endpointListLock.Unlock(MUTEX_CONTEXT);
    ep->Invalidate();
}

} // namespace ajn
