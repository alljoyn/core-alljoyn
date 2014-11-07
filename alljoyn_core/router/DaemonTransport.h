/**
 * @file
 * DaemonTransport is a specialization of class Transport for communication between an AllJoyn
 * client application and the daemon. This is the daemon's counterpart to ClientTransport.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_DAEMONTRANSPORT_H
#define _ALLJOYN_DAEMONTRANSPORT_H

#ifndef __cplusplus
#error Only include DaemonTransport.h in C++ code.
#endif

#include <alljoyn/Status.h>

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

#include "Transport.h"
#include "RemoteEndpoint.h"

namespace ajn {

/**
 * @brief A class for the daemon end of the client transport
 *
 * The DaemonTransport class has different incarnations depending on the platform.
 */
class DaemonTransport : public Transport, public _RemoteEndpoint::EndpointListener, public qcc::Thread {
    friend class _DaemonEndpoint;
  public:
    /**
     * Create a transport to receive incoming connections from AllJoyn application.
     *
     * @param bus  The bus associated with this transport.
     */
    DaemonTransport(BusAttachment& bus);

    /**
     * Destructor
     */
    ~DaemonTransport();

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
     * Determine if this transport is stopping.
     * Stopping means Stop() has been called but endpoints still exist.
     *
     * @return  Returns true if the transport is running.
     */
    bool IsTransportStopping() { return stopping; }

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
    virtual QStatus NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, std::map<qcc::String, qcc::String>& argMap) const;

    /**
     * Start listening for incoming connections on a specified bus address.
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    virtual QStatus StartListen(const char* listenSpec);

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
    virtual QStatus StopListen(const char* listenSpec);

    /**
     * Returns the name of this transport
     */
    virtual const char* GetTransportName() const { return TransportName; }

    /**
     * Name of transport used in transport specs.
     */
    static const char* TransportName;

    /**
     * Indicates whether this transport is used for client-to-bus or bus-to-bus connections.
     *
     * @return  Always returns false, DaemonTransports are only used to accept connections from a local client.
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

  protected:
    std::list<RemoteEndpoint> endpointList;   /**< List of active endpoints */
    qcc::Mutex endpointListLock;              /**< Mutex that protects the endpoint list */

  private:

    /**
     * @brief The default values for range and default idle timeout for
     * the DaemonTransport in seconds.
     *
     * This corresponds to the configuration items "dt_min_idle_timeout",
     * "dt_max_idle_timeout" and "dt_default_idle_timeout"
     * To override this value, change the limit, "dt_min_idle_timeout",
     * "dt_max_idle_timeout" and "dt_default_idle_timeout"
     */
    static const uint32_t MIN_HEARTBEAT_IDLE_TIMEOUT_DEFAULT = 3;
    static const uint32_t MAX_HEARTBEAT_IDLE_TIMEOUT_DEFAULT = 30;
    static const uint32_t DEFAULT_HEARTBEAT_IDLE_TIMEOUT_DEFAULT = 20;

    /**
     * @brief The default probe timeout for DaemonTransport in seconds.
     *
     * This corresponds to the configuration item "dt_default_probe_timeout"
     * and "dt_max_probe_timeout"
     * To override this value, change the limit, "dt_default_probe_timeout"
     * and "dt_max_probe_timeout"
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

    BusAttachment& bus;                       /**< The message bus for this transport */
    bool stopping;                            /**< True if Stop() has been called but endpoints still exist */

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

#endif // _ALLJOYN_DAEMONTRANSPORT_H
