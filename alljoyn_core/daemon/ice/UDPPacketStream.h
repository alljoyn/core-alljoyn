/**
 * @file
 * UDPPacketStream is a UDP based implementation of the PacketStream interface.
 */

/******************************************************************************
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_UDPPACKETSTREAM_H
#define _ALLJOYN_UDPPACKETSTREAM_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <alljoyn/Status.h>
#include <qcc/IPAddress.h>
#include "Packet.h"
#include "PacketStream.h"

namespace ajn {

/**
 * UDPPacketStream is a UDP based implemntation of the PacketStream interface.
 */
class UDPPacketStream : public PacketStream {
  public:

    /** Constructor */
    UDPPacketStream(const char* ifaceName, uint16_t port);

    /** Constructor */
    UDPPacketStream(const qcc::IPAddress& addr, uint16_t port);

    /** Constructor */
    UDPPacketStream(const qcc::IPAddress& addr, uint16_t port, size_t mtu);

    /** Destructor */
    ~UDPPacketStream();

    /**
     * Start the PacketStream.
     */
    QStatus Start();

    /**
     * Stop the PacketStream.
     */
    QStatus Stop();

    /**
     * Get UDP port.
     */
    uint16_t GetPort() const { return port; }

    /**
     * Set a new UDP port.
     */
    void SetPort(uint16_t new_port) { port = new_port; }

    /**
     * Get UDP IP addr.
     */
    qcc::String GetIPAddr() const;

    /**
     * Pull bytes from the source.
     * The source is exhausted when ER_NONE is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param sender       Source type specific representation of the sender of the packet.
     * @param timeout      Time to wait to pull the requested bytes.
     * @return   ER_OK if successful. ER_NONE if source is exhausted. Otherwise an error.
     */
    QStatus PullPacketBytes(void* buf, size_t reqBytes, size_t& actualBytes, PacketDest& sender, uint32_t timeout = qcc::Event::WAIT_FOREVER);

    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    qcc::Event& GetSourceEvent() { return *sourceEvent; }

    /**
     * Get the mtu size for this PacketSource.
     *
     * @return MTU of PacketSource
     */
    size_t GetSourceMTU() { return mtu; }

    /**
     * Push zero or more bytes into the sink.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink. (Must be less that or equal to MTU of PacketSink.)
     * @return   ER_OK if successful.
     */
    QStatus PushPacketBytes(const void* buf, size_t numBytes, PacketDest& dest);

    /**
     * Get the Event that indicates when data can be pushed to sink.
     *
     * @return Event that is signaled when sink can accept more bytes.
     */
    qcc::Event& GetSinkEvent() { return *sinkEvent; }

    /**
     * Get the mtu size for this PacketSink.
     *
     * @return MTU of PacketSink
     */
    size_t GetSinkMTU() { return mtu; }

    /**
     * Human readable form of UDPPacketDest.
     */
    qcc::String ToString(const PacketDest& dest) const;

  private:

    UDPPacketStream(const UDPPacketStream& other);
    UDPPacketStream& operator=(const UDPPacketStream& other);

    qcc::IPAddress ipAddr;
    uint16_t port;
    size_t mtu;

    qcc::SocketFd sock;
    qcc::Event* sourceEvent;
    qcc::Event* sinkEvent;
};

}  /* namespace */

#endif

