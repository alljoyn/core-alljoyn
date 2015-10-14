/**
 * @file
 * MQTTTransport is an implementation of TransportBase for daemons
 * to connect to an MQTT broker.
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

#ifndef _ALLJOYN_MQTTTRANSPORT_H
#define _ALLJOYN_MQTTTRANSPORT_H

#ifndef __cplusplus
#error Only include MQTTTransport.h in C++ code.
#endif

#if defined(QCC_OS_GROUP_WINDOWS)
/* Disabling warning C 4100. Function doesnt use all passed in parameters */
#pragma warning(push)
#pragma warning(disable: 4100)
#endif
#include <mosquittopp.h>
#if defined(QCC_OS_GROUP_WINDOWS)
#pragma warning(pop)
#endif

#include <qcc/SocketStream.h>
#include <qcc/Thread.h>
#include "Transport.h"
#include "RemoteEndpoint.h"

namespace ajn {

class MQTTTransport;
/*
 * An endpoint class to communicate with a MQTT broker.
 */
class _MQTTEndpoint : public _RemoteEndpoint, public mosqpp::mosquittopp {
    friend class MQTTTransport;
  public:
    _MQTTEndpoint(MQTTTransport* transport,
                  BusAttachment& bus, qcc::String uqn, qcc::IPEndpoint ipaddr);

    _MQTTEndpoint() { }
    virtual ~_MQTTEndpoint();
    QStatus Stop();
    QStatus Start();
    void on_connect(int rc);
    void on_message(const struct mosquitto_message*message);
    void on_disconnect(int rc);
    QStatus PushMessage(Message& msg);
    int Publish(int*mid, const char*topic, int payloadlen = 0, const void*payload = NULL, int qos = 0, bool retain = false);
    int Subscribe(int*mid, const char*sub, int qos = 0);
    int Unsubscribe(int*mid, const char*sub);
    void SubscribeToSessionless(qcc::String iface, qcc::String member);
    void SubscribeToPresence(qcc::String name);
    void UnsubscribeToPresence(qcc::String name);
    void PublishPresence(qcc::String name, bool isPresent);
    void SubscribeForDestination(qcc::String name);
    void SubscribeToSession(qcc::String sessionHost, SessionId id, bool isMultipoint);
    void PublishToDestination(qcc::String name, Message& msg);
    void CancelMessage(qcc::String sender, qcc::String iface, qcc::String member);
    struct SessionHostEntry {
        qcc::String m_sessionHost;
        bool m_isMultipoint;
        SessionHostEntry(qcc::String sessionHost = "", bool isMultipoint = false) : m_sessionHost(sessionHost), m_isMultipoint(isMultipoint) { }

    };
  private:
    MQTTTransport* m_transport;
    bool m_started;
    qcc::String m_clientId;
    std::map<SessionId, SessionHostEntry> m_sessionHostMap;
    bool m_connected;
    bool m_reconnected;
};

typedef qcc::ManagedObj<_MQTTEndpoint> MQTTEndpoint;

class MQTTTransport : public Transport, public _RemoteEndpoint::EndpointListener, public qcc::Thread {
    friend class _MQTTEndpoint;

  public:
    /**
     * Create a MQTT based transport for use by daemons.
     *
     * @param bus The BusAttachment associated with this endpoint
     */
    MQTTTransport(BusAttachment& bus) : Thread("MQTT"), m_bus(bus) { };

    /**
     * Destructor
     */
    virtual ~MQTTTransport();

    /**
     * Start the transport and associate it with a router.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus Start();

    /**
     * Stop the transport.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus Stop();

    /**
     * Pend the caller until the transport stops.
     *
     * @return
     *      - ER_OK if successful
     *      - an error status otherwise.
     */
    QStatus Join();

    /**
     * Determine if this transport is running. Running means Start() has been called.
     *
     * @return  Returns true if the transport is running.
     */
    bool IsRunning() { return true; }

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
     * Connect to a specified remote AllJoyn/DBus address.
     *
     * @param connectSpec    Transport specific key/value args used to configure the client-side endpoint.
     *                       The form of this string is @c "<transport>:<key1>=<val1>,<key2>=<val2>..."
     * @param opts           Requested sessions opts.
     * @param[out] newep     Endpoint created as a result of successful connect.
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep);

    /**
     * Start listening for incomming connections on a specified bus address.
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *                    - Valid transport is @c "tcp". All others ignored.
     *                    - Valid keys are:
     *                        - @c addr = IP address of server to connect to.
     *                        - @c port = Port number of server to connect to.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus StartListen(const char* listenSpec);

    /**
     * @brief Stop listening for incomming connections on a specified bus address.
     *
     * This method cancels a StartListen request. Therefore, the listenSpec must
     * match previous call to StartListen().
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *                    - Valid transport is @c "tcp". All others ignored.
     *                    - Valid keys are:
     *                        - @c addr = IP address of server to connect to.
     *                        - @c port = Port number of server to connect to.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus StopListen(const char* listenSpec) { QCC_UNUSED(listenSpec); return ER_OK; };

    /**
     * Returns the name of this transport
     */
    const char* GetTransportName() const { return TransportName; }

    /**
     * Get the transport mask for this transport
     *
     * @return the TransportMask for this transport.
     */
    TransportMask GetTransportMask() const { return TRANSPORT_MQTT; }

    /**
     * Does this transport support connections as described by the provided
     * Session options.
     *
     * @param opts  Proposed session options.
     * @return
     *      - true if the SessionOpts specifies a supported option set.
     *      - false otherwise.
     */
    bool SupportsOptions(const SessionOpts& opts) const { QCC_UNUSED(opts); return true; };

    /**
     * Indicates whether this transport is used for client-to-bus or bus-to-bus connections.
     *
     * @return  Always returns true, TCP is a bus-to-bus transport.
     */
    bool IsBusToBus() const { return true; }
/**
 * Name of transport used in transport specs.
 */
    static const char* TransportName;

    /**
     * Callback for TCPEndpoint exit.
     *
     * @param endpoint   TCPEndpoint instance that has exited.
     */
    void EndpointExit(RemoteEndpoint& endpoint) { QCC_UNUSED(endpoint); };

    void* STDCALL Run(void* arg);

  private:
    /**
     * Empty private overloaded virtual function for Thread::Start
     * this avoids the overloaded-virtual warning. For the Thread::Start
     * function.
     */
    QStatus Start(void* arg, qcc::ThreadListener* listener) { return Thread::Start(arg, listener); }

    MQTTTransport(const MQTTTransport& other);
    MQTTTransport& operator =(const MQTTTransport& other);
    MQTTEndpoint m_ep;
    BusAttachment& m_bus;                                          /**< The message bus for this transport */

};

} // namespace ajn

#endif // _ALLJOYN_MQTTTRANSPORT_H
