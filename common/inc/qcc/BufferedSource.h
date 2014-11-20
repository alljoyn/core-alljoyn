/**
 * @file
 *
 * This file defines a Source wrapper/filter that buffers input I/O.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

#ifndef _QCC_BUFFEREDSOURCE_H
#define _QCC_BUFFEREDSOURCE_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Event.h>
#include <qcc/Stream.h>

namespace qcc {

/**
 * BufferedSource is an Source wrapper that reads the underlying (wrapped)
 * Source in chunks. It is typically used by Source consumers that want
 * to read one byte at a time efficiently.
 *
 * BufferedSource also provides push back functionallity for Source consumers
 * that need it.
 */
class BufferedSource : public Source {
  public:

    /**
     * Construct a BufferedSource
     *
     * @param source      Raw source to be buffered.
     * @param bufSize     Number of bytes of buffering.
     * @param usePushBack true iff PushBack method will be used. (Requires extra heap space)>
     */
    BufferedSource(Source& source = Source::nullSource, size_t bufSize = 1024, bool usePushBack = false);

    /** Destructor */
    virtual ~BufferedSource();

    /**
     * Pull bytes from the source.
     * The source is exhausted when ER_EOF is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param timeout      Timeout in milliseconds.
     * @return   ER_OK if successful.
     *           ER_WOULDBLOCK if source has no more data but is still connected.
     *           ER_EOF if source is at end. Otherwise an error.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);

    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    Event& GetSourceEvent() { return event; }

    /**
     * Push bytes back onto the stream.
     * It is illegal to push back more bytes than received on the last call to PullBytes.
     * It is illegal to push back more bytes than the buffer size declared in the constructor.
     * It is illegal to push back different bytes than were originally pulled.
     *
     * @param buf      Pointer to buffer containing bytes to be pushed back.
     * @param numPush  Number of bytes to push back
     * @return ER_OK if successful.
     */
    QStatus PushBack(const void* buf, size_t numPush);

    /**
     * Reset this BufferedSource.
     *
     * @param source   Raw source to be buffered.
     */
    void Reset(Source& source);

    /**
     * Get the buffer size.
     */
    size_t GetBufferSize() { return bufSize; }


  private:

    /**
     * Copy constructor is private and does nothing
     */
    BufferedSource(const BufferedSource& other) : source(NULL), buf(NULL), rdPtr(NULL), endPtr(NULL) { }

    /**
     * Assigment operator is private and does nothing
     */
    BufferedSource& operator=(const BufferedSource& other) { return *this; }

    Source* source;         /**< Underlying raw source */
    Event event;            /**< IO event for this buffered source */
    uint8_t* buf;           /**< Heap allocated buffer */
    size_t bufSize;         /**< Amount of buffering provided by this BufferedSource */
    uint8_t* rdPtr;         /**< Pointer to next read byte in buf */
    uint8_t* endPtr;        /**< Pointer to one past end of valid bytes in buf */
    bool usePushBack;       /**< True iff PushBack is enabled for this buffered source */
};

}

#endif
