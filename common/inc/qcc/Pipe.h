/**
 * @file
 *
 * Sink/Source implementation for storing/retrieving bytes
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

#ifndef _QCC_PIPE_H
#define _QCC_PIPE_H

#include <qcc/platform.h>

#include <qcc/String.h>
#include <qcc/Event.h>
#include <qcc/Mutex.h>
#include <qcc/Stream.h>

namespace qcc {

/**
 * Pipe provides Sink/Source based storage for bytes.
 * Pushing bytes into the Pipe's Sink will cause the bytes to become
 * available at the Source.
 */
class Pipe : public Stream {
  public:

    /**
     * Construct a Pipe.
     */
    Pipe() : str(), outIdx(0), isWaiting(false) { }

    /**
     * Construct a Pipe from an existing string.
     * @param str   Input string.
     */
    Pipe(const qcc::String str) : str(str), outIdx(0), isWaiting(false) { }

    /** Destructor */
    virtual ~Pipe() { }

    /**
     * Pull bytes from the ByteStream
     * This call will block if there are no bytes available.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param timeout      Timeout in milliseconds.
     * @return   ER_OK if successful. ER_EOF if source is exhausted. Otherwise an error.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);

    /**
     * Push bytes into the sink.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      Number of bytes actually consumed by sink.
     * @return   ER_OK if successful.
     */
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent);

    /**
     * Number of bytes avaiable to Pull
     *
     * @return The number of bytes tha can be pulled.
     */
    size_t AvailBytes() { return str.size() - outIdx; }

  private:
    qcc::String str;    /**< storage for byte stream */
    size_t outIdx;      /**< Index into str for next output char */
    bool isWaiting;     /**< true iff a thread is pending in PullBytes */
    Mutex lock;         /**< Mutex that protects pipe */
    Event event;        /**< Event used to signal availability of more bytes */
};

}  /* namespace */

#endif
