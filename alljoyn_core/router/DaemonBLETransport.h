/**
 * @file
 * DaemonBLETransport is a specialization of class Transport for communication between an AllJoyn
 * client application and the daemon over BLE.
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
#ifndef _ALLJOYN_DAEMON_BLE_TRANSPORT_H
#define _ALLJOYN_DAEMON_BLE_TRANSPORT_H

#ifndef __cplusplus
#error Only include DaemonBLETransport.h in C++ code.
#endif

#include <alljoyn/Status.h>

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/BDAddress.h>
#include <qcc/BLEStream.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <map>
#include <qcc/Timer.h>

#include "Transport.h"
#include "RemoteEndpoint.h"
#include "BTController.h"

namespace ajn {

#define ALLJOYN_BT_VERSION_NUM_ATTR    0x400
#define ALLJOYN_BT_CONN_ADDR_ATTR      0x401
#define ALLJOYN_BT_L2CAP_PSM_ATTR      0x402

#define ALLJOYN_BT_UUID_BASE "-1c25-481f-9dfb-59193d238280"


class _DaemonBLEEndpoint;
typedef qcc::ManagedObj<_DaemonBLEEndpoint> DaemonBLEEndpoint;
/**
 * @brief A class for the daemon end of the transport communicating over BLE using the SLAP protocol.
 *
 */
class DaemonBLETransport :
    public Transport,
    public _RemoteEndpoint::EndpointListener,
    public qcc::Thread,
    public BluetoothDeviceInterface     {
    friend class _DaemonBLEEndpoint;
    friend class BTController;
  public:
    /* Default packet size */
    static const uint32_t SLAP_DEFAULT_PACKET_SIZE = 4000;

    /**
     * @internal
     * @brief Authentication complete notificiation.
     *
     * @param conn Reference to the DaemonBLEEndpoint that completed authentication.
     */
    void Authenticated(DaemonBLEEndpoint& conn);

    /**
     * Create a transport to receive incoming connections from AllJoyn application.
     *
     * @param bus  The bus associated with this transport.
     */
    DaemonBLETransport(BusAttachment& bus);

    /**
     * Destructor
     */
    ~DaemonBLETransport();

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
    void BLEDeviceAvailable(bool avail);
    void DisconnectAll();

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
     * @return  Always returns false, DaemonBLETransports are only used to accept connections from a local client.
     */
    bool IsBusToBus() const { return false; }

    /**
     * Callback for Daemon RemoteEndpoint exit.
     *
     * @param endpoint   Daemon RemoteEndpoint instance that has exited.
     */
    void EndpointExit(RemoteEndpoint& endpoint);

    /**
     * Callback for Daemon NewDeviceFound.
     *
     * @param remoteDevice   New remote device that has appeared.
     */
    qcc::BLEController* NewDeviceFound(const char* remoteDevice);

    bool IsConnValid(qcc::BLEController*conn);

    /**
     * Callback indicating that an untrusted client is trying to connect to this daemon.
     */
    QStatus UntrustedClientStart();

    /**
     * Callback indicating that an untrusted client has disconnected from this daemon.
     */
    void UntrustedClientExit() { };

    /**
     * Incomplete class for BT stack specific state.
     */
    class BLEAccessor;

  private:
    /**
     * Empty private overloaded virtual function for Thread::Start
     * this avoids the overloaded-virtual warning. For the Thread::Start
     * function.
     */
    QStatus Start(void* arg, qcc::ThreadListener* listener) { return Thread::Start(arg, listener); }

    void EndpointExit(DaemonBLEEndpoint& dEp);

    BusAttachment& m_bus;                          /**< The message bus for this transport */
    bool stopping;                                 /**< True if Stop() has been called but endpoints still exist */
    bool btmActive;                                /**< Indicates if the Bluetooth Topology Manager is registered */
    BTController* btController;                    /**< Bus Object that manages the BT topology */
    BLEAccessor* bleAccessor;                        /**< Object for accessing the Bluetooth device */
    TransportListener* listener;

    struct ListenEntry {
        qcc::String normSpec;
        std::map<qcc::String, qcc::String> args;
        qcc::BDAddress listenRemObj;
        bool endpointStarted;

        ListenEntry(qcc::String normSpec, std::map<qcc::String, qcc::String> args) : normSpec(normSpec), args(args), listenRemObj(), endpointStarted(false) { }
        bool operator<(const ListenEntry& other) const {
            return (normSpec < other.normSpec) || ((normSpec == other.normSpec) && ((args < other.args) || ((args == other.args) && ((listenRemObj < other.listenRemObj) || ((listenRemObj == other.listenRemObj) && (endpointStarted < other.endpointStarted))))));

        }

        bool operator==(const ListenEntry& other) const {
            return (normSpec == other.normSpec)  && (args == other.args) && (listenRemObj == other.listenRemObj) && (endpointStarted == other.endpointStarted);
        }

    };
    std::set<DaemonBLEEndpoint> m_EPset;                /**< File descriptors the transport is listening on */
    std::list<ListenEntry> m_listenList;           /**< File descriptors the transport is listening on */
    std::set<DaemonBLEEndpoint> m_endpointList;   /**< List of active endpoints */
    std::set<DaemonBLEEndpoint> m_authList;       /**< List of active endpoints */
    qcc::Mutex m_lock;                             /**< Mutex that protects the endpoint and auth list, and listen list */

    /**
     * @internal
     * @brief Thread entry point.
     *
     * @param arg  Thread entry arg.
     */
    qcc::ThreadReturn STDCALL Run(void* arg);


    RemoteEndpoint LookupEndpoint(const qcc::String& busName);
    void ReturnEndpoint(RemoteEndpoint& ep);

    /**
     * Tells the Bluetooth transport to start listening for incoming connections.
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StartListen();

    /**
     * Tells the Bluetooth transport to stop listening for incoming connections.
     */
    virtual void StopListen();

    QStatus Disconnect(const qcc::String& busName);


    /**
     * Start listening for incomming connections on a specified bus address.
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *                      The form of this string is @c "<transport>:<key1>=<val1>,<key2>=<val2>..."
     *                      - Valid transport is "bluetooth". All others ignored.
     *                      - Valid keys are:
     *                          - @c addr = Bluetooth device address
     *                          - @c name = Bluetooth Bus name
     *
     * @return ER_OK
     */

};

} // namespace ajn

#endif // _ALLJOYN_DAEMON_BLE_TRANSPORT_H
