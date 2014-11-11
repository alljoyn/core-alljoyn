/**
 * @file
 * DaemonSLAPTransport is a specialization of class Transport for communication between an AllJoyn
 * client application and the daemon over UART.
 */

/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_DAEMONSLAPTRANSPORT_H
#define _ALLJOYN_DAEMONSLAPTRANSPORT_H

#ifndef __cplusplus
#error Only include DaemonSLAPTransport.h in C++ code.
#endif

#include <alljoyn/Status.h>

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <map>
#include <qcc/Timer.h>

#include "Transport.h"
#include "RemoteEndpoint.h"

namespace ajn {
class _DaemonSLAPEndpoint;
typedef qcc::ManagedObj<_DaemonSLAPEndpoint> DaemonSLAPEndpoint;
/**
 * @brief A class for the daemon end of the transport communicating over UART using the SLAP protocol.
 *
 */
class DaemonSLAPTransport : public Transport, public _RemoteEndpoint::EndpointListener, public qcc::Thread {
    friend class _DaemonSLAPEndpoint;
  public:
    /* Default packet size */
    static const uint32_t SLAP_DEFAULT_PACKET_SIZE = 4000;

    /**
     * @internal
     * @brief Authentication complete notificiation.
     *
     * @param conn Reference to the DaemonSLAPEndpoint that completed authentication.
     */
    void Authenticated(DaemonSLAPEndpoint& conn);

    /**
     * Create a transport to receive incoming connections from AllJoyn application.
     *
     * @param bus  The bus associated with this transport.
     */
    DaemonSLAPTransport(BusAttachment& bus);

    /**
     * Destructor
     */
    ~DaemonSLAPTransport();

    /**
     * Start the transport and associate it with the router.
     *
     * @return ER_OK if successful.
     */
    QStatus Start();

    /**
     * Stop the transport.
     *
     * @return ER_OK if successful.
     */
    QStatus Stop();

    /**
     * Pend the caller until the transport stops.
     * @return ER_OK if successful.
     */
    QStatus Join();

    /**
     * Determine if this transport is running. Running means Start() has been called.
     *
     * @return  Returns true if the transport is running.
     */
    bool IsRunning() { return Thread::IsRunning(); }

    /**
     * Get the transport mask for this transport
     *
     * @return the TransportMask for this transport.
     */
    TransportMask GetTransportMask() const { return TRANSPORT_LOCAL; }

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
    QStatus NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, std::map<qcc::String, qcc::String>& argMap) const;

    /**
     * Start listening for incoming connections on a specified bus address.
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus StartListen(const char* listenSpec);

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
    QStatus StopListen(const char* listenSpec);

    /**
     * Returns the name of this transport
     */
    const char* GetTransportName() const { return TransportName; }

    /**
     * Name of transport used in transport specs.
     */
    static const char* TransportName;

    /**
     * Indicates whether this transport is used for client-to-bus or bus-to-bus connections.
     *
     * @return  Always returns false, DaemonSLAPTransports are only used to accept connections from a local client.
     */
    bool IsBusToBus() const { return false; }

    /**
     * Callback for Daemon RemoteEndpoint exit.
     *
     * @param endpoint   Daemon RemoteEndpoint instance that has exited.
     */
    void EndpointExit(RemoteEndpoint& endpoint);

    /**
     * Callback indicating that an untrusted client is trying to connect to this daemon.
     */
    QStatus UntrustedClientStart();

    /**
     * Callback indicating that an untrusted client has disconnected from this daemon.
     */
    void UntrustedClientExit() { };
  private:
    /**
     * @brief The default values for range and default idle timeout for
     * the DaemonSLAPTransport in seconds.
     *
     * This corresponds to the configuration items "slap_min_idle_timeout",
     * "slap_max_idle_timeout" and "slap_default_idle_timeout"
     * To override this value, change the limit, "slap_min_idle_timeout",
     * "slap_max_idle_timeout" and "slap_default_idle_timeout"
     */
    static const uint32_t MIN_HEARTBEAT_IDLE_TIMEOUT_DEFAULT = 3;
    static const uint32_t MAX_HEARTBEAT_IDLE_TIMEOUT_DEFAULT = 30;
    static const uint32_t DEFAULT_HEARTBEAT_IDLE_TIMEOUT_DEFAULT = 20;

    /**
     * @brief The default probe timeout for DaemonSLAPTransport in seconds.
     *
     * This corresponds to the configuration item "slap_default_probe_timeout"
     * and "slap_max_probe_timeout"
     * To override this value, change the limit, "slap_default_probe_timeout"
     * and "slap_max_probe_timeout"
     */
    static const uint32_t MAX_HEARTBEAT_PROBE_TIMEOUT_DEFAULT = 30;
    static const uint32_t DEFAULT_HEARTBEAT_PROBE_TIMEOUT_DEFAULT = 3;

    /**
     * @brief The number of DBus pings sent from Routing node to leaf node.
     *
     */
    static const uint32_t HEARTBEAT_NUM_PROBES = 1;
    /**
     * Empty private overloaded virtual function for Thread::Start
     * this avoids the overloaded-virtual warning. For the Thread::Start
     * function.
     */
    QStatus Start(void* arg, qcc::ThreadListener* listener) { return Thread::Start(arg, listener); }

    BusAttachment& m_bus;                          /**< The message bus for this transport */
    bool stopping;                                 /**< True if Stop() has been called but endpoints still exist */

    struct ListenEntry {
        qcc::String normSpec;
        std::map<qcc::String, qcc::String> args;
        qcc::UARTFd listenFd;
        bool endpointStarted;

        ListenEntry(qcc::String normSpec, std::map<qcc::String, qcc::String> args) : normSpec(normSpec), args(args), listenFd(-1), endpointStarted(false) { }
        bool operator<(const ListenEntry& other) const {
            return (normSpec < other.normSpec) || ((normSpec == other.normSpec) && ((args < other.args) || ((args == other.args) && ((listenFd < other.listenFd) || ((listenFd == other.listenFd) && (endpointStarted < other.endpointStarted))))));

        }

        bool operator==(const ListenEntry& other) const {
            return (normSpec == other.normSpec)  && (args == other.args) && (listenFd == other.listenFd) && (endpointStarted == other.endpointStarted);
        }

    };
    std::list<ListenEntry> m_listenList;           /**< File descriptors the transport is listening on */
    std::set<DaemonSLAPEndpoint> m_endpointList;   /**< List of active endpoints */
    std::set<DaemonSLAPEndpoint> m_authList;       /**< List of active endpoints */
    qcc::Mutex m_lock;                             /**< Mutex that protects the endpoint and auth list, and listen list */

    /**
     * @internal
     * @brief Thread entry point.
     *
     * @param arg  Thread entry arg.
     */
    qcc::ThreadReturn STDCALL Run(void* arg);

    uint32_t m_minHbeatIdleTimeout; /**< The minimum allowed idle timeout for the Heartbeat between Routing node
                                         and Leaf node - configurable in router config */

    uint32_t m_defaultHbeatIdleTimeout; /**< The default idle timeout for the Heartbeat between Routing node
                                           and Leaf node - configurable in router config */

    uint32_t m_maxHbeatIdleTimeout; /**< The maximum allowed idle timeout for the Heartbeat between Routing node
                                         and Leaf node - configurable in router config */

    uint32_t m_defaultHbeatProbeTimeout;   /**< The time the Routing node should wait for Heartbeat response to be
                                                  recieved from the Leaf node - configurable in router config */

    uint32_t m_maxHbeatProbeTimeout;       /**< The max time the Routing node should wait for Heartbeat response to be
                                                  recieved from the Leaf node - configurable in router config */

    uint32_t m_numHbeatProbes;             /**< Number of probes Routing node should wait for Heartbeat response to be
                                              recieved from the Leaf node before declaring it dead - Transport specific */

};

} // namespace ajn

#endif // _ALLJOYN_DAEMON_SLAP_TRANSPORT_H
