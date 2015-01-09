/**
 * @file
 *
 * Sink/Source Named Pipe data stream operations.
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#ifndef _OS_QCC_NAMEDPIPE_STREAM_H
#define _OS_QCC_NAMEDPIPE_STREAM_H

#include <qcc/platform.h>

#include <winsock2.h>
#include <windows.h>
#include <string>

#include <qcc/String.h>
#include <qcc/Stream.h>

#include <Status.h>

namespace qcc {

/**
 * NamedPipeStream is an implementation of Source and Sink for use with Named Pipe (NP).
 */

class NamedPipeStream : public Stream {
  public:

    /**
     * Create an NamedPipeStream
     *
     * @param busHandle   pipe handle
     */
    NamedPipeStream(HANDLE busHandle);

    /**
     * Copy constructor.
     *
     * @param other   NamedPipeStream to copy from.
     */
    NamedPipeStream(const NamedPipeStream& other);

    /** Destructor */
    virtual ~NamedPipeStream();

    /**
     * Pull bytes from the source.
     * The source is exhausted when ER_NONE is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param timeout      Timeout in milliseconds.
     * @return   OI_OK if successful. ER_NONE if source is exhausted. Otherwise an error.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);


    /**
     * Pull bytes and any accompanying file/socket descriptors from the stream.
     * The source is exhausted when ER_NONE is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  [OUT] Actual number of bytes retrieved from source.
     * @param fdList       Array to receive file descriptors.
     * @param numFds       [IN,OUT] On IN the size of fdList on OUT number of files descriptors pulled.
     * @param timeout      Timeout in milliseconds.
     * @return   OI_OK if successful. ER_NONE if source is exhausted. Otherwise an error.
     */
    QStatus PullBytesAndFds(void* buf, size_t reqBytes, size_t& actualBytes, SocketFd* fdList, size_t& numFds, uint32_t timeout = Event::WAIT_FOREVER);


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
     * Push bytes accompanied by one or more file/socket descriptors to a sink.
     *
     * @param buf       Buffer containing bytes to push
     * @param numBytes  Number of bytes from buf to send to sink, must be at least 1.
     * @param numSent   [OUT] Number of bytes actually consumed by sink.
     * @param fdList    Array of file descriptors to push.
     * @param numFds    Number of files descriptors, must be at least 1.
     * @param pid       Process id required on some platforms.
     *
     * @return  OI_OK or an error.
     */
    QStatus PushBytesAndFds(const void* buf, size_t numBytes, size_t& numSent, SocketFd* fdList, size_t numFds, uint32_t pid = -1);


    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    Event& GetSourceEvent() { return *sourceEvent; }


    /**
     * Get the Event that indicates when data can be pushed to sink.
     *
     * @return Event that is signaled when sink can accept more bytes.
     */
    Event& GetSinkEvent() { return *sinkEvent; }

    /**
     * Check validity of Stream.
     *
     * @return  true if stream was successfully initialized.
     */
    bool IsValid() { return INVALID_HANDLE_VALUE != busHandle; }

    /** Close and shutdown the Named Pipe.  */
    void Close();

  private:

    /** Private default constructor */
    NamedPipeStream();

    bool isConnected;     /**< true if there is a pipe connection */

    /**
     * Assignment. Currently, this is not implemented.
     *
     * @param other NamedPipeStream to copy from.
     * @return This NamedPipeStream.
     */
    NamedPipeStream operator=(const NamedPipeStream& other);

  protected:

    HANDLE busHandle;     /**< Bus handle */
    Event* sourceEvent;   /**< Source event */
    Event* sinkEvent;     /**< Sink event */
    bool isDetached;      /**< true if pipe is detached */
    uint32_t sendTimeout; /**< Send timeout */
};

}  /* namespace */

#endif // _OS_QCC_NAMEDPIPE_STREAM_H
