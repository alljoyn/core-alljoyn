/**
 * @file
 * BTTransport is an implementation of Transport for Bluetooth.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYNBTTRANSPORT_H
#define _ALLJOYNBTTRANSPORT_H

#ifndef __cplusplus
#error Only include BTTransport.h in C++ code.
#endif

#include <qcc/platform.h>

#include <set>
#include <vector>

#include <qcc/GUID.h>
#include <qcc/Mutex.h>
#include <qcc/Stream.h>
#include <qcc/String.h>
#include <qcc/Thread.h>

#include "BDAddress.h"
#include "BTController.h"
#include "BTNodeDB.h"
#include "RemoteEndpoint.h"
#include "Transport.h"

#include <alljoyn/Status.h>

namespace ajn {

#define ALLJOYN_BT_VERSION_NUM_ATTR    0x400
#define ALLJOYN_BT_CONN_ADDR_ATTR      0x401
#define ALLJOYN_BT_L2CAP_PSM_ATTR      0x402
#define ALLJOYN_BT_ADVERTISEMENTS_ATTR 0x404

#define ALLJOYN_BT_UUID_BASE "-1c25-481f-9dfb-59193d238280"

/**
 * Class for a Bluetooth endpoint
 */
//class BTEndpoint;

/**
 * %BTTransport is an implementation of Transport for Bluetooth.
 */
class BTTransport :
    public Transport,
    public _RemoteEndpoint::EndpointListener,
    public qcc::Thread,
    public BluetoothDeviceInterface {

    friend class BTController;

  public:
    /**
     * Returns the name of this transport
     * @return The name of this transport.
     */
    const char* GetTransportName() const { return TransportName; };

    /**
     * Get the transport mask for this transport
     *
     * @return the TransportMask for this transport.
     */
    TransportMask GetTransportMask() const { return TRANSPORT_BLUETOOTH; }

    /**
     * Get a list of the possible listen specs of the current Transport for a
     * given set of session options.
     *
     * Session options specify high-level characteristics of session, such as
     * whether or not the underlying transport carries data encapsulated in
     * AllJoyn messages, and whether or not delivery is reliable.
     *
     * It is possible that there is more than one answer to the question: what
     * abstract address should I use when talking to another endpoint.  Each
     * Transports is equipped to understand how many answers there are and also
     * which answers are better than the others.  This method fills in the
     * provided vector with a list of currently available busAddresses ordered
     * according to which the transport thinks would be best.
     *
     * If there are no addresses appropriate to the given session options the
     * provided vector of String is left unchanged.  If there are addresses,
     * they are added at the end of the provided vector.
     *
     * @param opts Session options describing the desired characteristics of
     *             an underlying session
     * @param busAddrs A vector of String to which bus addresses corresponding
     *                 to IFF_UP interfaces matching the desired characteristics
     *                 are added.
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus GetListenAddresses(const SessionOpts& opts, std::vector<qcc::String>& busAddrs) const
    {
        if (opts.transports & GetTransportMask()) {
            qcc::String listenAddr = btController->GetListenAddress();
            if (!listenAddr.empty()) {
                busAddrs.push_back(listenAddr);
            }
        }
        return ER_OK;
    }


    /**
     * Normalize a bluetooth transport specification.
     *
     * @param inSpec    Input transport connect spec.
     * @param[out] outSpec   Output transport connect spec.
     * @param[out] argMap    Map of connect parameters.
     * @return
     *      - ER_OK if successful.
     *      - ER_BUS_BAD_TRANSPORT_ARGS  is unable to parse the Input transport connect specification
     */
    QStatus NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, std::map<qcc::String, qcc::String>& argMap) const;

    /**
     * Create a Bluetooth connection based Transport.
     *
     * @param bus  The bus associated with this transport.
     */
    BTTransport(BusAttachment& bus);

    /**
     * Destructor
     */
    ~BTTransport();

    /**
     * Start the transport and associate it with a router.
     *
     * @return
     *      - ER_OK if successful.
     *      - ER_BUS_TRANSPORT_NOT_STARTED if unsuccessful
     */
    QStatus Start();

    /**
     * Stop the transport.
     * @return
     *      - ER_OK if successful.
     *      - ER_EXTERNAL_THREAD if unable to stop because operation not supported on external thread wrapper
     */
    QStatus Stop(void);

    /**
     * Pend the caller until the transport stops.
     * @return
     *      - ER_OK if successful.
     *      - ER_OS_ERROR if unable to join (Underlying OS has indicated an error)
     */
    QStatus Join(void);

    /**
     * Determine if this transport is running. Running means Start() has been called.
     *
     * @return  Returns true if the transport is running.
     */
    bool IsRunning() { return Thread::IsRunning(); }

    /**
     * Connect to a remote bluetooth devices
     *
     * @param connectSpec key/value arguments used to configure the client-side endpoint.
     *                    The form of this string is @c "<transport>:<key1>=<val1>,<key2>=<val2>..."
     *                    - Valid transport is "bluetooth". All others ignored.
     *                    - Valid keys are:
     *                        - @c addr = Bluetooth device address
     *                        - @c name = Bluetooth Bus name
     * @param opts           Requested sessions opts.
     * @param newep          [OUT] Endpoint created as a result of successful connect.
     *
     * @return
     *      - ER_OK if successful.
     *      - ER_BUS_BAD_TRANSPORT_ARGS if unable to parse the @c connectSpec param
     *      - An error status otherwise
     */
    QStatus Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep);

    /**
     * Disconnect a bluetooth endpoint
     *
     * @param connectSpec   The connect spec to be disconnected.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus Disconnect(const char* connectSpec);

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
    QStatus StartListen(const char* listenSpec);

    /**
     * Stop listening for incomming connections on a specified bus address.
     * This method cancels a StartListen request. Therefore, the listenSpec must match previous call to StartListen()
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *                      The form of this string is @c "<transport>:<key1>=<val1>,<key2>=<val2>..."
     *                       - Valid transport is "bluetooth". All others ignored.
     *                       - Valid keys are:
     *                          - @c addr = Bluetooth device address
     *                          - @c name = Bluetooth Bus name
     *
     * @return ER_OK
     *
     * @see BTTransport::StartListen
     */
    QStatus StopListen(const char* listenSpec);

    /**
     * Function for the BT Accessor to inform a change in the
     * power/availablity of the Bluetooth device.
     *
     * @param avail    true if BT device is powered on and available, false otherwise.
     */
    void BTDeviceAvailable(bool avail);

    /**
     * Check if it is OK to accept the incoming connection from the specified
     * address.
     *
     * @param addr          BT device address to check
     * @param redirectAddr  [OUT] BT bus address to redirect the connection to if the return value is true.
     *
     * @return  true if OK to accept the connection, false otherwise.
     */
    bool CheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const;

    /**
     * Disconnect all endpoints.
     */
    void DisconnectAll();

    /**
     * Callback for BTEndpoint thead exit.
     *
     * @param endpoint   BTEndpoint instance that has exited.
     */
    void EndpointExit(RemoteEndpoint& endpoint);

    /**
     * Name of transport used in transport specs.
     */
    static const char* TransportName;

    /**
     * Register a listener for transport related events.
     *
     * @param listener The listener to register. If this parameter is NULL the current listener is removed
     *
     * @return ER_OK
     */
    void SetListener(TransportListener* listener) { this->listener = listener; }

    /**
     * Start discovering busses to connect to
     *
     * @param namePrefix    First part of a well known name to search for.
     */
    void EnableDiscovery(const char* namePrefix);

    /**
     * Stop discovering busses to connect to
     *
     * @param namePrefix    First part of a well known name to stop searching for.
     */
    void DisableDiscovery(const char* namePrefix);

    /**
     * Start advertising a well-known name with the given quality of service.
     *
     * @param advertiseName   Well-known name to add to list of advertised names.
     * @param quietly         Advertise the name quietly
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus EnableAdvertisement(const qcc::String& advertiseName, bool quietly);

    /**
     * Stop advertising a well-known name with a given quality of service.
     *
     * @param advertiseName   Well-known name to remove from list of advertised names.
     */
    void DisableAdvertisement(const qcc::String& advertiseName);

    /**
     * Indicates whether this transport is used for client-to-bus or bus-to-bus connections.
     *
     * @return  Always returns true, Bluetooth is a bus-to-bus transport.
     */
    bool IsBusToBus() const { return true; }

    /**
     * Incomplete class for BT stack specific state.
     */
    class BTAccessor;

  protected:
    /**
     * Thread entry point.
     *
     * @param arg  Unused thread entry arg.
     */
    qcc::ThreadReturn STDCALL Run(void* arg);

  private:

    BTTransport(const BTTransport& other) : bus(other.bus) { }
    BTTransport& operator=(const BTTransport& other) { return *this; }

    /**
     * Internal connect method to establish a bus connection to a given BD Address.
     *
     * @param addr  BT bus address of the device to connect to.
     * @param newep Pointer to newly created endpoint (if non-NULL).
     *
     */
    QStatus Connect(const BTBusAddress& addr, RemoteEndpoint& newep);

    QStatus Connect(const BTBusAddress& addr) {
        RemoteEndpoint ret;
        return Connect(addr, ret);
    }

    /**
     * Internal disconnect method to remove a bus connection from a given BD Address.
     *
     * @param bdAddr    BD Address of the device to connect to.
     *
     * @return  ER_OK if successful.
     */
    QStatus Disconnect(const qcc::String& busName);

    /**
     * Called by BTAccessor to inform transport of an AllJoyn capable device.
     *
     * @param adBdAddr      BD Address of the device advertising names.
     * @param uuidRev       UUID revision number of the device that was found.
     * @param eirCapable    - true if found device was confirmed AllJoyn capable via EIR
     *                      - false if found device is potential AllJoyn capable
     */
    void DeviceChange(const BDAddress& bdAddr,
                      uint32_t uuidRev,
                      bool eirCapable);

    /**
     * Start the find operation for AllJoyn capable devices.  A duration may
     * be specified that will result in the find operation to automatically
     * stop after the specified number of seconds.  Exclude any results from
     * any device in the list of ignoreAddrs.
     *
     * @param ignoreAddrs   Set of BD addresses to ignore
     * @param duration      Find duration in seconds (0 = forever)
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StartFind(const BDAddressSet& ignoreAddrs, uint32_t duration = 0);

    /**
     * Stop the find operation.
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StopFind();

    /**
     * Start the advertise operation for the given list of names.  A duration
     * may be specified that will result in the advertise operation to
     * automatically stop after the specified number of seconds.  This
     * includes setting the SDP record to contain the information specified in
     * the parameters.
     *
     * @param uuidRev   AllJoyn Bluetooth service UUID revision
     * @param bdAddr    BD address of the connectable node
     * @param psm       The L2CAP PSM number for the AllJoyn service
     * @param adInfo    The complete list of names to advertise and their associated GUIDs
     * @param duration  Find duration in seconds (0 = forever)
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StartAdvertise(uint32_t uuidRev,
                                   const BDAddress& bdAddr,
                                   uint16_t psm,
                                   const BTNodeDB& adInfo,
                                   uint32_t duration = 0);

    /**
     * Stop the advertise operation.
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StopAdvertise();

    /**
     * This provides the Bluetooth transport with the information needed to
     * call AllJoynObj::FoundNames and to generate the connect spec.
     *
     * @param bdAddr    BD address of the connectable node
     * @param guid      Bus GUID of the discovered bus
     * @param names     The advertised names
     * @param psm       L2CAP PSM accepting connections
     * @param lost      Set to true if names are lost, false otherwise
     */
    virtual void FoundNamesChange(const qcc::String& guid,
                                  const std::vector<qcc::String>& names,
                                  const BDAddress& bdAddr,
                                  uint16_t psm,
                                  bool lost);

    /**
     * Tells the Bluetooth transport to start listening for incoming connections.
     *
     * @param addr[out] BD Address of the adapter listening for connections
     * @param psm[out]  L2CAP PSM allocated
     *
     * @return  ER_OK if successful
     */
    virtual QStatus StartListen(BDAddress& addr,
                                uint16_t& psm);

    /**
     * Tells the Bluetooth transport to stop listening for incoming connections.
     */
    virtual void StopListen();

    /**
     * Retrieves the information from the specified device necessary to
     * establish a connection and get the list of advertised names.
     *
     * @param addr          BD address of the device of interest.
     * @param uuidRev[out]  UUID revision number.
     * @param connAddr[out] Bluetooth bus address of the connectable device.
     * @param adInfo[out]   Advertisement information.
     *
     * @return  ER_OK if successful
     */
    virtual QStatus GetDeviceInfo(const BDAddress& addr,
                                  uint32_t& uuidRev,
                                  BTBusAddress& connAddr,
                                  BTNodeDB& adInfo);


    RemoteEndpoint LookupEndpoint(const qcc::String& busName);
    void ReturnEndpoint(RemoteEndpoint& ep);

    QStatus IsMaster(const BDAddress& addr, bool& master) const;
    void RequestBTRole(const BDAddress& addr, bt::BluetoothRole role);

    bool IsEIRCapable() const;

    BusAttachment& bus;                            /**< The message bus for this transport */
    BTAccessor* btAccessor;                        /**< Object for accessing the Bluetooth device */
    BTController* btController;                    /**< Bus Object that manages the BT topology */
    std::set<RemoteEndpoint> threadList;          /**< List of active BT endpoints */
    qcc::Mutex threadListLock;                     /**< Mutex that protects threadList */
    BTNodeDB connNodeDB;
    TransportListener* listener;
    bool transportIsStopping;                      /**< The transport has recevied a stop request */
    bool btmActive;                                /**< Indicates if the Bluetooth Topology Manager is registered */
};

}

#endif
