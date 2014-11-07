/**
 * @file
 * PacketEngineStream is an implemenation of qcc::Stream used by PacketEngine.
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
#ifndef _ALLJOYN_PACKETENGINESTREAM_H
#define _ALLJOYN_PACKETENGINESTREAM_H

#include <qcc/platform.h>
#include <qcc/Stream.h>
#include <alljoyn/Status.h>

namespace ajn {

/* Forward Declaration */
class PacketEngine;

/**
 * Stream is a virtual class that defines a standard interface for a streaming source and sink.
 */
class PacketEngineStream : public qcc::Stream {
    friend class PacketEngine;

  public:
    /** Default Constructor */
    PacketEngineStream();

    /** Destructor */
    ~PacketEngineStream();

    /** Copy constructor */
    PacketEngineStream(const PacketEngineStream& other);

    /** Assignment operator */
    PacketEngineStream& operator=(const PacketEngineStream& other);

    /** Equality */
    bool operator==(const PacketEngineStream& other) const;

    uint32_t GetChannelId() const { return chanId; }

    /**
     * Pull bytes from the source.
     * The source is exhausted when ER_EOF is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param timeout      Time to wait to pull the requested bytes.
     * @return   ER_OK if successful. ER_EOF if source is exhausted. Otherwise an error.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = qcc::Event::WAIT_FOREVER);

    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    qcc::Event& GetSourceEvent() { return *sourceEvent; }

    /**
     * Push zero or more bytes into the sink with per-msg time-to-live.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      Number of bytes actually consumed by sink.
     * @param ttl          Time-to-live in ms or 0 for infinite ttl.
     * @return   ER_OK if successful.
     */
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent, uint32_t ttl);

    /**
     * Push zero or more bytes into the sink with infinite ttl.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      Number of bytes actually consumed by sink.
     * @return   ER_OK if successful.
     */
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent) {
        return PushBytes(buf, numBytes, numSent, 0);
    }

    /**
     * Get the Event that indicates when data can be pushed to sink.
     *
     * @return Event that is signaled when sink can accept more bytes.
     */
    qcc::Event& GetSinkEvent() { return *sinkEvent; }

    /**
     * Set the send timeout for this sink.
     *
     * @param sendTimeout   Send timeout in ms.
     */
    void SetSendTimeout(uint32_t sendTimeout) { this->sendTimeout = sendTimeout; }

  private:
    PacketEngine* engine;
    uint32_t chanId;
    qcc::Event* sourceEvent;
    qcc::Event* sinkEvent;
    uint32_t sendTimeout;

    PacketEngineStream(PacketEngine& engine, uint32_t chanId, qcc::Event& sourceEvent, qcc::Event& sinkEvent);
};

}  /* namespace */

#endif
