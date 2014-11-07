/**
 * @file
 * PacketStream defines a sink/source interface for packet-based data.
 */

/******************************************************************************
 * Copyright (c) 2011-2012, 2014, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_PACKETSTREAM_H
#define _ALLJOYN_PACKETSTREAM_H

#include <qcc/platform.h>
#include <qcc/Event.h>
#include <alljoyn/Status.h>
#include "Packet.h"

namespace ajn {

/**
 * PacketSource defines a standard interface for packet providers.
 */
class PacketSource {
  public:

    /** Destructor */
    virtual ~PacketSource() { }

    /**
     * Start the PacketStream
     *
     * @return ER_OK if successful.
     */
    virtual QStatus Start() = 0;

    /**
     * Stop the PacketStream
     *
     * @return ER_OK if successful.
     */
    virtual QStatus Stop() = 0;

    /**
     * Pull bytes from the source.
     * The source is exhausted when ER_EOF is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param sender       Source type specific representation of the sender of the packet.
     * @param timeout      Time to wait to pull the requested bytes.
     * @return   ER_OK if successful. ER_EOF if source is exhausted. Otherwise an error.
     */
    virtual QStatus PullPacketBytes(void* buf, size_t reqBytes, size_t& actualBytes, PacketDest& sender, uint32_t timeout = qcc::Event::WAIT_FOREVER) = 0;

    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    virtual qcc::Event& GetSourceEvent() = 0;

    /**
     * Get the mtu size for this PacketSource.
     *
     * @return MTU of PacketSource
     */
    virtual size_t GetSourceMTU() = 0;
};

/**
 * PacketSink defines a standard interface for packet consumers.
 */
class PacketSink {
  public:

    /** Destructor */
    virtual ~PacketSink() { }

    /**
     * Push zero or more bytes into the sink.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink. (Must be less that or equal to MTU of PacketSink.)
     * @param dest         Destination for packet bytes.
     * @return   ER_OK if successful.
     */
    virtual QStatus PushPacketBytes(const void* buf, size_t numBytes, PacketDest& dest) = 0;

    /**
     * Get the Event that indicates when data can be pushed to sink.
     *
     * @return Event that is signaled when sink can accept more bytes.
     */
    virtual qcc::Event& GetSinkEvent() = 0;

    /**
     * Get the mtu size for this PacketSink.
     *
     * @return MTU of PacketSink
     */
    virtual size_t GetSinkMTU() = 0;
};

/**
 * PacketStream is an interface that defines a standard interface for an object that is both a PacketSource and a PacketSink.
 */
class PacketStream : public PacketSource, public PacketSink {
  public:
    /** Destructor */
    virtual ~PacketStream() { }

    /**
     * Convert a PacketDest to human readable form.
     */
    virtual qcc::String ToString(const PacketDest& dest) const = 0;
};

}  /* namespace */

#endif

