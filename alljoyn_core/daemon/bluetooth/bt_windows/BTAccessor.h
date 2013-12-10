/**
 * @file
 * BTAccessor declaration for Windows
 */

/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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

#ifndef _ALLJOYN_BTACCESSOR_H
#define _ALLJOYN_BTACCESSOR_H

#include <qcc/platform.h>

#include <qcc/ManagedObj.h>
#include <qcc/String.h>
#include <qcc/Timer.h>
#include <qcc/Thread.h>
#include <qcc/Event.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/ProxyBusObject.h>

#include <alljoyn/Status.h>

#include "BDAddress.h"
#include "BTController.h"
#include "BTNodeDB.h"
#include "BTTransport.h"

#include "userKernelComm.h"

namespace ajn {

class WindowsBTEndpoint;

class BTTransport::BTAccessor : public MessageReceiver, public qcc::AlarmListener {
  public:
    /**
     * Constructor
     *
     * @param transport
     * @param busGuid
     */
    BTAccessor(BTTransport* transport, const qcc::String& busGuid);

    /**
     * Destructor
     */
    ~BTAccessor();

    /**
     * Start the underlying Bluetooth subsystem.
     *
     * @return ER_OK if successful.
     */
    QStatus Start()
    {
        isStarted = true;

        // All start and stop tasks are handled by adapterChangeThread.
        adapterChangeThread.Alert();

        return ER_OK;
    }

    /**
     * Start the underlying Bluetooth subsystem.
     */
    void Stop()
    {
        isStarted = false;

        // All start and stop tasks are handled by adapterChangeThread.
        adapterChangeThread.Alert();
    }

    /**
     * Start discovery (inquiry)
     *
     * @param ignoreAddrs   Set of BD Addresses to ignore
     * @param duration      Number of seconds to discover (0 = forever, default is 0)
     */
    QStatus StartDiscovery(const BDAddressSet& ignoreAddrs, uint32_t duration = 0);

    /**
     * Stop discovery (inquiry)
     */
    QStatus StopDiscovery();

    /**
     * Start discoverability (inquiry scan)
     */
    QStatus StartDiscoverability(uint32_t duration = 0);

    /**
     * Stop discoverability (inquiry scan)
     */
    QStatus StopDiscoverability();

    /**
     * Set SDP information
     *
     * @param uuidRev   Bus UUID revision to advertise in the SDP record
     * @param bdAddr    Bluetooth device address that of the node that is connectable
     * @param psm       L2CAP PSM number accepting connections
     * @param adInfo    Map of bus node GUIDs and bus names to advertise
     */
    QStatus SetSDPInfo(uint32_t uuidRev,
                       const BDAddress& bdAddr,
                       uint16_t psm,
                       const BTNodeDB& adInfo);

    /**
     * Make the Bluetooth device connectable.
     *
     * @param addr[out] Bluetooth device address that is connectable
     * @param psm[out]  L2CAP PSM that is connectable (0 if not connectable)
     *
     * @return  ER_OK if device is now connectable
     */
    QStatus StartConnectable(BDAddress& addr,
                             uint16_t& psm);

    /**
     * Make the Bluetooth device not connectable.
     */
    void StopConnectable();

    /**
     * Accepts an incoming connection from a remote Bluetooth device.
     *
     * @param alljoyn       BusAttachment that will be connected to the resulting endpoint
     * @param connectEvent  The event signalling the incoming connection
     *
     * @return  A newly instantiated remote endpoint for the Bluetooth connection (NULL indicates a failure)
     */
    RemoteEndpoint* Accept(BusAttachment& alljoyn,
                           qcc::Event* connectEvent);

    /**
     * Create an outgoing connection to a remote Bluetooth device.  If the
     * L2CAP PSM are not specified, then an SDP query will be performed to get
     * that information.
     *
     * @param alljoyn   BusAttachment that will be connected to the resulting endpoint
     *
     * @return  A newly instantiated remote endpoint for the Bluetooth connection (NULL indicates a failure)
     */
    RemoteEndpoint* Connect(BusAttachment& alljoyn,
                            const BTNodeInfo& node);

    /**
     * Perform an SDP query on the specified device to get the bus information.
     *
     * @param addr          Bluetooth device address to retrieve the SDP record from
     * @param uuidRev[out]  Bus UUID revision found in the SDP record
     * @param connAddr[out] Address of the Bluetooth device accepting connections.
     * @param adInfo[out]   Map of bus node GUIDs and bus names being advertised
     *
     * @return  ER_OK if successful
     */
    QStatus GetDeviceInfo(const BDAddress& addr,
                          uint32_t* uuidRev,
                          BTBusAddress* connAddr,
                          BTNodeDB* adInfo);

    /**
     * Accessor to get the L2CAP connect event object.
     *
     * @return pointer to the L2CAP connect event object.
     */
    qcc::Event* GetL2CAPConnectEvent() { return &this->l2capEvent; }

    /**
     * This looks up the low level Bluetooth connection information for the
     * connection with the specified device and indicates if our role for that
     * connection is master or slave.
     *
     * @param addr          Bluetooth device address for the connection of interest.
     * @param master[out]   - 'true' if we are master of the connection with addr
     *                      - 'false' if we are slave of the connection with addr
     *
     * @return  ER_OK if successful; an error will be returned if there is no
     *          connection with the specified device
     */
    QStatus IsMaster(const BDAddress& addr, bool& master) const
    {
        return ER_NOT_IMPLEMENTED;    // Windows doesn't support this.
    }

    /**
     * This method forces a role switch in the HCI device so that we become
     * master of the connection with the specified device.
     *
     * @param addr  Bluetooth device address for the connection of interest
     * @param role  Requested Bluetooth connection role
     */
    void RequestBTRole(const BDAddress& addr, ajn::bt::BluetoothRole role)
    {
        // Windows doesn't support this.
    }

    /**
     * Always returns false. This device is not EIR capable.
     *
     * @return false.
     */
    bool IsEIRCapable() const { return false; }

    /**
     * This method removes the endpoint from the collection of active WindowsBTEndpoints
     * and sends a message to the kernel to disconnect this channel.
     *
     * @param endpoint  The WindowsBTEndpoint to remove.
     */
    void EndPointsRemove(WindowsBTEndpoint* endpoint);

    /**
     * Communicate with the AllJoyn kernel Bluetooth device driver.
     *
     * @param messageIn     The message to send to the kernel.
     * @param sizeMessageIn The size of the message to send to the kernel.
     * @param messageOut    The message from the kernel, may be NULL if outSize is 0.
     * @param sizeMessageOut The size of the message expected to be received from the kernel.
     * @param returnedSize  Pointer to the actual size of the message received from the kernel.
                            May be NULL.
     *
     * @return true if successful. false if not and GetLastError() has the error code.
     */
    bool DeviceIo(void* messageIn, size_t sizeMessageIn,
                  void* messageOut, size_t sizeMessageOut, size_t* returnedSize) const;

    /**
     * Get the current state of the kernel and do a debug spew (QCC_DbgPrintf()) of the result.
     */
    void DebugDumpKernelState(void) const;

  private:
    /**
     * Generic lock for device related objects, maps, etc. This is the first declaration in the private
     * section so it gets constructed first and destructed last.
     */
    mutable qcc::Mutex deviceLock;

    volatile HANDLE radioHandle;    // Handle of the BT radio on this system.
    BDAddress address;              // Address of the BT radio on this system.
    HANDLE recordHandle;            // Handle of the SDP record.
    HANDLE deviceHandle;            // The handle used for communication to the driver.
    bool isStarted;                 // Set to true if Start() has been called. Set to false when Stop() is called.

    /**
     * Has Start() been called more recently than Stop()?
     *
     * @return true Start() has been called more recently than Stop().
     */
    bool IsStarted() const
    {
        return isStarted;
    }

    /**
     * Is there a bluetooth radio available?
     *
     * @return true if a bluetooth radio is available.
     */
    bool BluetoothIsAvailable() const
    {
        return radioHandle != NULL;
    }

    /**
     * Class for handling Bluetooth enable/disable.
     */
    class AdapterChangeThread : public qcc::Thread {
      public:
        AdapterChangeThread(BTAccessor& btAccessor) : qcc::Thread("AdapterChangeThread"), btAccessor(btAccessor) { }

      private:
        qcc::ThreadReturn STDCALL Run(void* args);
        BTAccessor& btAccessor;
    };

    /**
     * Class for handling Bluetooth discovery
     */
    class DiscoveryThread : public qcc::Thread {
      public:
        DiscoveryThread(BTAccessor& btAccessor) : qcc::Thread("DiscoveryThread"), btAccessor(btAccessor), duration(0) { }

        void StartDiscovery(uint32_t duration) {
            this->duration = duration;
            Alert();
        }

        void StopDiscovery() {
            this->duration = 0;
        }

      private:
        qcc::ThreadReturn STDCALL Run(void* args);
        BTAccessor& btAccessor;
        uint32_t duration;
    };

    /**
     * Class for handling communication with the kernel mode driver
     */
    class MessageThread : public qcc::Thread {
      public:
        MessageThread(BTAccessor& btAccessor) : qcc::Thread("MessageThread"), btAccessor(btAccessor) { }
      private:
        qcc::ThreadReturn STDCALL Run(void* args);
        BTAccessor& btAccessor;
    };

    DiscoveryThread discoveryThread; // The discovery thread.
    MessageThread getMessageThread;  // Thread for receiving messages from the kernel mode driver
    AdapterChangeThread adapterChangeThread;    // Thread to detect Bluetooth adapter removal/disable/enable
    qcc::Event getMessageEvent;      // Set if there is a message waiting in the kernel.

    bool wsaInitialized;                // Set to true if WSAStartup() was called successfully.

    BDAddressSet discoveryIgnoreAddrs;  // BT addresses to ignore during discovery.
    qcc::Event l2capEvent;              // Signaled when a connection request is made.
    BusAttachment winBus;
    BTTransport* transport;
    const qcc::String busGuid;

    WindowsBTEndpoint* activeEndPoints[MAX_OPEN_L2CAP_CHANNELS];

    /**
     * Incoming connection requests are stored in this circular queue until an accept occurs.
     * At the time of the accept the oldest connect request is used to complete
     * the connection. The connectRequestTail is the index of where the new request
     * is put and connectRequestHead is the request about to be accepted.
     */
    struct _KRNUSRCMD_L2CAP_EVENT connectRequests[MAX_OPEN_L2CAP_CHANNELS];

    /**
     * The index of where the incoming connection requests are place and removed from
     * connectRequests[].
     */
    int connectRequestsTail, connectRequestsHead;

    struct DispatchInfo {
        typedef enum {
            STOP_DISCOVERABILITY,
        } DispatchTypes;
        DispatchTypes operation;

        DispatchInfo(DispatchTypes operation) : operation(operation) { }
        virtual ~DispatchInfo() { }
    };

    /**
     * Set the Bluetooth radio handle to this new value. The new value may be 0.
     */
    void SetRadioHandle(HANDLE newHandle);

    /**
     * This connects to the driver in the kernel and does other initialization when a bluetooth
     * device becomes available.
     *
     * @param newRadioHandle The radio handle to use for this connection.
     */
    QStatus KernelConnect(HANDLE newRadioHandle);

    /**
     * This disconnects from the driver in the kernel and does other cleanup when a bluetooth
     * device becomes unavailable.
     *
     * @param radioIsOn True if the bluetooth radio is on and driver should be up and running.
     */
    void KernelDisconnect(bool radioIsOn);

    /**
     * This initializes the array of pointers for the WindowsBTEndpoints to be saved.
     */
    void EndPointsInit(void);

    /**
     * This adds the WindowsBTEndpoint to the collection.
     *
     * @param endpoint A pointer to the endpoint to be saved.
     *
     * @return  true if successful.
     */
    bool EndPointsAdd(WindowsBTEndpoint* endpoint);

    /**
     * This method removes all the endpoints from the collection of active WindowsBTEndpoints
     * and sends a message to the kernel to disconnect any of those endpoint found.
     */
    void EndPointsRemoveAll(void);

    /**
     * This finds the WindowsBTEndpoint associated with the given address and channel handle.
     *
     * @param address Bluetooth device address for the connection of interest.
     * @param handle Bluetooth device channel handle for the connection of interest.
     *
     * @return  The endpoint connection to the remote device.
     */
    WindowsBTEndpoint* EndPointsFind(BTH_ADDR address, L2CAP_CHANNEL_HANDLE handle = 0) const;

    /**
     * Initializes the circular queue for incoming connection requests.
     * Takes the lock on deviceLock.
     */
    void ConnectRequestsInit(void)
    {
        deviceLock.Lock(MUTEX_CONTEXT);
        memset(connectRequests, 0, sizeof(connectRequests));
        connectRequestsTail = connectRequestsHead = 0;
        deviceLock.Unlock(MUTEX_CONTEXT);
    }

    /**
     * Put the next incoming connection request from the circular queue in *request.
     * Takes the lock on deviceLock.
     *
     * @return  Returns ER_OK if successful, ER_FAIL if there are none available,
     *          ER_BAD_ARG_1 if request is NULL.
     */
    QStatus ConnectRequestsGet(struct _KRNUSRCMD_L2CAP_EVENT* request);

    /**
     * Put the next incoming connection request in the circular queue. If the queue is
     * full then it silently overrights the oldest entry.
     * Takes the lock on deviceLock.
     *
     * @return  Returns ER_OK if successful ER_BAD_ARG_1 if request is NULL.
     */
    QStatus ConnectRequestsPut(const struct _KRNUSRCMD_L2CAP_EVENT* request);

    /**
     * Determine whether the queue of connect requests is empty.
     *
     * @return  Returns true if there are no more requests waiting.
     */
    bool ConnectRequestsIsEmpty()
    {
        return connectRequestsHead == connectRequestsTail;
    }

    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

    qcc::Alarm DispatchOperation(DispatchInfo* op, uint32_t delay = 0)
    {
        void* context = (void*)op;
        qcc::Alarm alarm(delay, this, context);
        winBus.GetInternal().GetDispatcher().AddAlarm(alarm);
        return alarm;
    }

    /**
     * Get a handle to the Bluetooth radio.
     * @return the handle if successful or NULL if not.
     */
    HANDLE GetRadioHandle(void);

    /**
     * Initialize this->address.
     * @return true if successful.
     */
    bool GetRadioAddress(void);

    void DeviceFound(const BDAddress& adBdAddr);

    /**
     * Remove the SDP records we used to advertised our service.
     */
    void RemoveRecord();

    /**
     * Send a message to the device and get a response back in messageOut.
     * This is a wrapper for DeviceIo() for sending/receiving USER_KERNEL_MESSAGEs.
     *
     * @param messageIn[in] The message sent to the device.
     * @param messageOut[out] The response message from the device.
     *
     * @return ER_OK if the communication to the device was successful.
     * The command status must be retrieved from messageOut.commandStatus.status.
     */
    QStatus DeviceSendMessage(USER_KERNEL_MESSAGE* messageIn,
                              USER_KERNEL_MESSAGE* messageOut) const;

    /**
     * Handle a L2CAP connection request from a remote device.
     * @param message[in] The message which contains the address and channel handle.
     */
    void HandleL2CapEvent(const USER_KERNEL_MESSAGE* message);

    /**
     * Handle a message from the kernel there is data ready to be read.
     * @param message[in] The message which contains the channel handle
     * and the number of bytes waiting.
     */
    void HandleReadReady(const USER_KERNEL_MESSAGE* message);

    /**
     * Handle a message from the kernel there the accept attempt has been completed.
     * @param message[in] The message which contains the channel handle and status.
     */
    void HandleAcceptComplete(const USER_KERNEL_MESSAGE* message);

    /**
     * Handle a message from the kernel there the connection attempt has been completed.
     * @param message[in] The message which contains the channel handle and status.
     */
    void HandleConnectComplete(const USER_KERNEL_MESSAGE* message);

    void HandleMessageFromKernel(const USER_KERNEL_MESSAGE* message);
};


} // namespace ajn


#endif
