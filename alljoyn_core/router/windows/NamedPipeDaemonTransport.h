/**
 * @file
 * NamedPipeDaemonTransport is a specialization of class Transport for communication between an AllJoyn
 * client application and the daemon over named pipe for Windows. This is the daemon's counterpart
 * to ClientTransport.
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
#ifndef _ALLJOYN_NAMEDPIPE_DAEMON_TRANSPORT_H
#define _ALLJOYN_NAMEDPIPE_DAEMON_TRANSPORT_H

#ifndef __cplusplus
#error Only include NamedPipeDaemonTransport.h in C++ code.
#endif

#include <alljoyn/Status.h>

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

#include "Transport.h"
#include "RemoteEndpoint.h"
#include "DaemonTransport.h"

namespace ajn {

class NamedPipeDaemonTransport;
class _NamedPipeDaemonEndpoint;
typedef qcc::ManagedObj<_NamedPipeDaemonEndpoint> NamedPipeDaemonEndpoint;

class NamedPipeAcceptThread : public qcc::Thread {

  public:

    NamedPipeAcceptThread(_In_ NamedPipeDaemonTransport& transport) : m_transport(transport) { };

    /**
     * @brief Thread entry point.
     *
     * @param arg  Thread entry arg.
     */
    qcc::ThreadReturn STDCALL Run(_In_ void* arg);

  private:

    NamedPipeDaemonTransport& m_transport;
};

/**
 * @brief A class for the daemon end of the client transport
 *
 * The NamedPipeDaemonTransport class has different incarnations depending on the platform.
 */
class NamedPipeDaemonTransport : public DaemonTransport {
    friend class _NamedPipeDaemonEndpoint;
    friend class NamedPipeAcceptThread;

  public:

    /**
     * Create a transport to receive incoming connections from AllJoyn application.
     *
     * @param bus  The bus associated with this transport.
     */
    NamedPipeDaemonTransport(_In_ BusAttachment& bus);

    /**
     * Destructor
     */
    virtual ~NamedPipeDaemonTransport();

    /**
     * @internal
     * @brief Normalize a transport specification.
     *
     * Given a transport specification, convert it into a form which is guaranteed to
     * have a one-to-one relationship with a connection instance.
     *
     * @param inSpec    Input transport connect spec.
     * @param outSpec   Output transport connect spec.
     * @param argMap    Parsed parameter map.
     *
     * @return ER_OK if successful.
     */
    QStatus NormalizeTransportSpec(_In_z_ const char* inSpec, _Out_ qcc::String& outSpec, _Inout_ std::map<qcc::String, qcc::String>& argMap) const;

    /**
     * Start listening for incoming connections on a specified bus address.
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus StartListen(_In_z_ const char* listenSpec);

    /**
     * @brief Stop listening for incoming connections on a specified bus address.
     *
     * This method cancels a StartListen request. Therefore, the listenSpec must
     * match previous call to StartListen().
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *
     * @return ER_OK if successful.
     */
    QStatus StopListen(_In_z_ const char* listenSpec);

    /**
     * Pend the caller until the transport stops.
     * @return ER_OK if successful.
     */
    QStatus Join();

    /**
     * Return the name of this transport.
     */
    const char* GetTransportName() const { return NamedPipeTransportName; }

    /**
     * Name of transport used in transport specs.
     */
    static const char* NamedPipeTransportName;

  private:
    /**
     * @internal
     * @brief Thread entry point.
     *
     * @param arg  Thread entry arg.
     */
    qcc::ThreadReturn STDCALL Run(_In_ void* arg);

    /**
     * @internal
     * @brief Called by the NamedPipeAcceptThread immediately after accepting a connection.
     */
    void AcceptedConnection(_In_ HANDLE pipeHandle);

    /**
     * @internal
     * @brief Called by the main transport thread after an endpoint finished authentication.
     */
    void ManageAuthenticatingEndpoints();

    /**
     * @internal
     * @brief Return the client authentication timeout.
     */
    uint32_t GetAuthTimeout() { return m_authTimeout; }

    /**
     * @internal
     * @brief The default timeout for client authentication.
     *
     * The authentication process can be used as the basis of a denial of
     * service attack by simply stopping in mid-authentication.  If an endpoint
     * I/O takes longer than this number of milliseconds, the transport
     * disconnects and removes that endpoind. This value can be overridden in
     * the daemon config file by setting "auth_timeout".
     */
    static const uint32_t ALLJOYN_AUTH_TIMEOUT_DEFAULT = 20000;

    /*
     * @internal
     * @brief Timeout for each pipe I/O during client authentication.
     *
     * If a client doesn't respond during this time, the daemon treats it as
     * denier of service and closes the connection.
     */
    uint32_t m_authTimeout;

    /*
     * @internal
     * @brief Thread responsible for accepting pipe connections.
     *
     * This has to be separate from the main transport thread because it has to perform
     * endpoint list maintenance every once in a while. Main thread is not appropriate
     * for calling AllJoynAcceptBusConnection, because that call that would block the
     * aformentioned maintenance.
     */
    NamedPipeAcceptThread m_acceptThread;

    /*
     * @internal
     * @brief List of endpoints that are being authenticated.
     *
     * When authentication ends successfully, the endpoint is moved from this list to the
     * separate endpointList. Both authenticatingEndpointList and endpointList are protected
     * by endpointListLock.
     */
    std::list<NamedPipeDaemonEndpoint> authenticatingEndpointList;

    /*
     * @internal
     * @brief Event signaled when an endpoint authentication is completed.
     *
     * The main transport thread waits for this event, then performs maintenance for
     * endpointListLock and endpointList.
     */
    qcc::Event m_authFinishedEvent;
};

} // namespace ajn

#endif // _ALLJOYN_NAMEDPIPE_DAEMON_TRANSPORT_H
