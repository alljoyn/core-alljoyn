/**
 * @file STUNSocketStream.h
 *
 * Sink/Source wrapper for STUN.
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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

#ifndef _QCC_STUNSocketStream_H
#define _QCC_STUNSocketStream_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Event.h>
#include <qcc/Socket.h>
#include <qcc/SocketTypes.h>
#include <qcc/Stream.h>

#include <alljoyn/Status.h>
#include <Stun.h>

using namespace qcc;

namespace ajn {

/**
 * STUNSocketStream is an implementation of Source and Sink for use with Sockets.
 */
class STUNSocketStream : public Stream {
  public:

    /**
     * Create a STUNSocketStream from an existing STUN pointer.
     *
     * @param sock      socket handle.
     */
    STUNSocketStream(Stun* stunPtr);

    /**
     * Copy-constructor
     *
     * @param other  STUNSocketStream to copy from.
     */
    STUNSocketStream(const STUNSocketStream& other);

    /**
     * Assignment operator.
     *
     * @param other  STUNSocketStream to assign from.
     */
    STUNSocketStream operator=(const STUNSocketStream& other);

    /** Destructor */
    virtual ~STUNSocketStream();

    /**
     * Connect the socket to a destination.
     *
     * @param host   Hostname or IP address of destination.
     * @param port   Destination port.
     * @return  ER_OK if successful.
     *
     * Not implemented for STUNSocketStream as the underlying STUN socket is already connected to
     * the destination by the time an object of STUNSocketStream is created.
     */
    QStatus Connect(String& host, uint16_t port) { return ER_NOT_IMPLEMENTED; };

    /**
     * Connect the socket to a path destination.
     *
     * @param path   Path name of destination.
     * @return  ER_OK if successful.
     *
     * Not implemented for STUNSocketStream as the underlying STUN socket is already connected to
     * the destination by the time an object of STUNSocketStream is created.
     */
    QStatus Connect(String& path) { return ER_NOT_IMPLEMENTED; };

    /** Close and shutdown the STUN connection.  */
    void Close();

    /**
     * Pull bytes from the socket.
     * The source is exhausted when ER_NONE is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  [OUT] Actual number of bytes retrieved from source.
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
     *
     * Not implemented as STUN sockets are always bus to bus and hence no question of passing
     * file descriptors.
     *
     */
    QStatus PullBytesAndFds(void* buf, size_t reqBytes, size_t& actualBytes,
                            SocketFd* fdList, size_t& numFds,
                            uint32_t timeout = Event::WAIT_FOREVER) { return ER_NOT_IMPLEMENTED; };

    /**
     * Push bytes into the sink.
     *
     * @param buf          Buffer containing bytes to push
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      [OUT] Number of bytes actually consumed by sink.
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
     *
     * Not implemented as STUN sockets are always bus to bus and hence no question of passing
     * file descriptors.
     *
     */
    QStatus PushBytesAndFds(const void* buf, size_t numBytes, size_t& numSent,
                            SocketFd* fdList, size_t numFds, uint32_t pid = -1) { return ER_NOT_IMPLEMENTED; };

    /**
     * Get the Event indicating that data is available.
     *
     * @return Event that is set when data is available.
     */
    Event& GetSourceEvent() { return *sourceEvent; }

    /**
     * Get the Event indicating that sink can accept data.
     *
     * @return Event set when socket can accept more data via PushBytes
     */
    Event& GetSinkEvent() { return *sinkEvent; }

    /**
     * Indicate whether socket is connected.
     * @return true iff underlying socket is connected.
     */
    bool IsConnected() { return isConnected; }

    /**
     * Return the socketFd for this STUNSocketStream.
     *
     * @return socketFd
     */
    SocketFd GetSocketFd() { return sock; }

    /**
     * Detach this STUNSocketStream from the underlying socket.
     * Calling this method will cause the underlying socket descriptor to not be shutdown when
     * the STUNSocketStream is closed or destroyed. The socket descriptor will, however, be closed.
     */
    void DetachSocketFd() { isDetached = true; }

  private:

    /** Private default constructor */
    STUNSocketStream();

    bool isConnected;                /**< true iff socket is connected */
  protected:

    Stun* stunPtr;                   /**< STUN pointer */
    SocketFd sock;                                       /**< Socket associated with the STUN pointer*/
    Event* sourceEvent;              /**< Event signaled when data is available */
    Event* sinkEvent;                /**< Event signaled when sink can accept data */
    bool isDetached;                 /**< Detached socket streams do not shutdown the underlying socket when closing */
};

} // namespace ajn

#endif
