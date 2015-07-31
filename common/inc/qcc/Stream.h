/**
 * @file
 *
 * This file defines a base class used for streaming data sources.
 */

/******************************************************************************
 *
 *
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

#ifndef _QCC_STREAM_H
#define _QCC_STREAM_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Event.h>
#include <qcc/Socket.h>

#include <Status.h>

namespace qcc {

/**
 * Source is pure virtual class that defines a standard interface for
 * a streaming data source.
 */
class Source {
  public:

    /** Singleton null source */
    static Source nullSource;

    /** Construct a NULL source */
    Source() { }

    /** Destructor */
    virtual ~Source() { }

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
    virtual QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER) {
        QCC_UNUSED(buf);
        QCC_UNUSED(reqBytes);
        QCC_UNUSED(actualBytes);
        QCC_UNUSED(timeout);
        return ER_EOF;
    }

    /**
     * Pull bytes and any accompanying file/socket descriptors from the source.
     * The source is exhausted when ER_EOF is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  [OUT] Actual number of bytes retrieved from source.
     * @param fdList       Array to receive file descriptors.
     * @param numFds       [IN,OUT] On IN the size of fdList on OUT number of files descriptors pulled.
     * @param timeout      Timeout in milliseconds.
     * @return   ER_OK if successful. ER_EOF if source is exhausted. Otherwise an error.
     */
    virtual QStatus PullBytesAndFds(void* buf, size_t reqBytes, size_t& actualBytes, SocketFd* fdList, size_t& numFds, uint32_t timeout = Event::WAIT_FOREVER) {
        QCC_UNUSED(buf);
        QCC_UNUSED(reqBytes);
        QCC_UNUSED(actualBytes);
        QCC_UNUSED(fdList);
        QCC_UNUSED(numFds);
        QCC_UNUSED(timeout);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    virtual Event& GetSourceEvent() { return Event::neverSet; }

    /**
     * Read source up to end of line or end of file.
     *
     * @param outStr   Line output.
     * @return  ER_OK if successful. ER_EOF if source is exhausted. Otherwise an error.
     */
    virtual QStatus GetLine(qcc::String& outStr, uint32_t timeout = Event::WAIT_FOREVER);
};

/**
 * Sink is pure virtual class that defines a standard interface for
 * a streaming data sink.
 */
class Sink {
  public:

    /** Construct a NULL sink */
    Sink() { }

    /** Destructor */
    virtual ~Sink() { }

    /**
     * Push zero or more bytes into the sink with infinite ttl.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      Number of bytes actually consumed by sink.
     * @return   ER_OK if successful.
     */
    virtual QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent) {
        QCC_UNUSED(buf);
        QCC_UNUSED(numBytes);
        QCC_UNUSED(numSent);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Push zero or more bytes into the sink.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      Number of bytes actually consumed by sink.
     * @param ttl          Time-to-live for message or 0 for infinite.
     * @return   ER_OK if successful.
     */
    virtual QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent, uint32_t ttl) {
        QCC_UNUSED(ttl);
        return PushBytes(buf, numBytes, numSent);
    }

    /**
     * Push one or more byte accompanied by one or more file/socket descriptors to a sink.
     *
     * @param buf       Buffer containing bytes to push
     * @param numBytes  Number of bytes from buf to send to sink, must be at least 1.
     * @param numSent   [OUT] Number of bytes actually consumed by sink.
     * @param fdList    Array of file descriptors to push.
     * @param numFds    Number of files descriptors, must be at least 1.
     * @param pid       Process id required on some platforms.
     *
     * @return  ER_OK or an error.
     */
    virtual QStatus PushBytesAndFds(const void* buf, size_t numBytes, size_t& numSent, SocketFd* fdList, size_t numFds, uint32_t pid = (uint32_t)-1) {
        QCC_UNUSED(buf);
        QCC_UNUSED(numBytes);
        QCC_UNUSED(numSent);
        QCC_UNUSED(fdList);
        QCC_UNUSED(numFds);
        QCC_UNUSED(pid);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Get the Event that indicates when data can be pushed to sink.
     *
     * @return Event that is signaled when sink can accept more bytes.
     */
    virtual Event& GetSinkEvent() { return Event::alwaysSet; }

    /**
     * Set the send timeout for this sink.
     *
     * @param sendTimeout   Send timeout in ms.
     */
    virtual void SetSendTimeout(uint32_t sendTimeout) {
        QCC_UNUSED(sendTimeout);
    }
};

/**
 * Stream is a virtual class that defines a standard interface for a streaming source and sink.
 */
class Stream : public Source, public Sink {
  public:
    /** Destructor */
    virtual ~Stream() { }

    /**
     * Used to perform an 'orderly' release of the stream.  The orderly release
     * is as follows:
     * -# PushBytes() or PushBytesAndFds() to transmit all bytes.
     * -# Shutdown()
     * -# PullBytes() or PullBytesAndFds() until the receive side is drained.
     * -# Close()
     *
     * @return #ER_OK if successful
     */
    virtual QStatus Shutdown() { return ER_OK; }

    /**
     * Sets the stream to discard any queued data and immediately tear down
     * the resource on Close().
     *
     * This is used to perform an 'abortive' release of the stream.  The
     * abortive release is as follows:
     * -# Abort()
     * -# Close()
     *
     * @return #ER_OK if successful
     */
    virtual QStatus Abort() { return ER_OK; }

    /** Close the stream */
    virtual void Close() { }
};

/**
 * NonBlockingStream is a type of stream that is not blocking i.e. Reads/Writes will return immediately.
 */
class NonBlockingStream : public Stream {
  public:
    /** Destructor */
    virtual ~NonBlockingStream() { }

    /** Close the stream */
    virtual void Close() { }
};

}  /* namespace */

#endif
