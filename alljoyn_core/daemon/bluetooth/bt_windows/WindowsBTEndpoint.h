/**
 * @file
 * BTEndpoint declaration for Windows
 * BTEndpoint implementation for Windows.
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
#ifndef _ALLJOYN_WINDOWSBTENDPOINT_H
#define _ALLJOYN_WINDOWSBTENDPOINT_H

#include <qcc/platform.h>

#include "WindowsBTStream.h"
#include <qcc/SocketStream.h>

#include "BTEndpoint.h"

#include <alljoyn/BusAttachment.h>

namespace ajn {

class WindowsBTEndpoint : public BTEndpoint {
  public:

    /**
     * Windows Bluetooth endpoint constructor
     *
     * @param bus
     * @param incoming
     * @param node
     * @param accessor
     * @param address
     */
    WindowsBTEndpoint(BusAttachment& bus,
                      bool incoming,
                      const BTNodeInfo& node,
                      ajn::BTTransport::BTAccessor* accessor,
                      BTH_ADDR address,
                      const BTBusAddress& redirect) :
        BTEndpoint(bus, incoming, btStream, node, redirect),
        connectionStatus(ER_FAIL),
        btStream(address, accessor)
    {
        connectionCompleteEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    /**
     * Windows Bluetooth endpoint destructor
     */
    ~WindowsBTEndpoint();

    /**
     * Get the channel handle associated with this endpoint.
     * @return The channel handle.
     */
    L2CAP_CHANNEL_HANDLE GetChannelHandle() const
    {
        return btStream.GetChannelHandle();
    }

    /**
     * Set the channel handle for this endpoint.
     *
     * @param channel The channel handle associated with this endpoint.
     */
    void SetChannelHandle(L2CAP_CHANNEL_HANDLE channel)
    {
        btStream.SetChannelHandle(channel);
    }

    /**
     * Get the bluetooth address for this endpoint.
     *
     * @return The address.
     */
    BTH_ADDR GetRemoteDeviceAddress() const
    {
        return btStream.GetRemoteDeviceAddress();
    }

    /**
     * Set number of bytes waiting in the kernel buffer.
     *
     * @param bytesWaiting The number of bytes.
     */
    void SetSourceBytesWaiting(size_t bytesWaiting, QStatus status)
    {
        connectionStatus = status;
        btStream.SetSourceBytesWaiting(bytesWaiting, status);
    }

    /**
     * Wait for the kernel to indicate the connection attempt has been completed.
     *
     * @return ER_OK if successful, ER_TIMEOUT the wait was not successful, or ER_FAIL for
     * other failures while waiting for the completion event to fire.
     * To determine the connection status after the completion event has fired call
     * GetConnectionStatus().
     */
    QStatus WaitForConnectionComplete(bool incoming);

    /**
     * Called via a message from the kernel to indicate the connection attempt has been
     * completed.
     *
     * @param status The status of the connection attempt.
     */
    void SetConnectionComplete(QStatus status);

    /**
     * Get the connection status for this endpoint.
     *
     * @return The status.
     */
    QStatus GetConnectionStatus(void) const { return connectionStatus; }

    /**
     * Set the pointer in the stream to the BTAccessor which created this endpoint to NULL.
     * This is needed when the endpoint has not yet been deleted but the BTAccessor is
     * in the process of being deleted.
     */
    void OrphanEndpoint(void) { btStream.OrphanStream(); }

  private:
    ajn::WindowsBTStream btStream;
    HANDLE connectionCompleteEvent;    // Used to signal the channel connection is completed.
    QStatus connectionStatus;
};

} // namespace ajn

#endif
