/**
 * @file SocketStream.h
 *
 * Sink/Source wrapper for Socket.
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

#ifndef _QCC_SOCKETSTREAM_H
#define _QCC_SOCKETSTREAM_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Event.h>
#include <qcc/Socket.h>
#include <qcc/SocketTypes.h>
#include <qcc/Stream.h>

#include <Status.h>


namespace qcc {

/**
 * SocketStream is an implementation of Source and Sink for use with Sockets.
 */
class SocketStream : public Stream {
  public:

    /**
     * Create a SocketStream from an existing socket.
     *
     * @param sock      socket handle.
     */
    SocketStream(SocketFd sock);

    /**
     * Create a SocketStream.
     *
     * @param family    Socket family.
     * @param type      Socket type.
     */
    SocketStream(AddressFamily family, SocketType type);

    /**
     * Copy-constructor
     *
     * @param other  SocketStream to copy from.
     */
    SocketStream(const SocketStream& other);

    /**
     * Assignment operator.
     *
     * @param other  SocketStream to assign from.
     */
    SocketStream operator=(const SocketStream& other);

    /** Destructor */
    virtual ~SocketStream();

    /**
     * Connect the socket to a destination.
     *
     * @param host   Hostname or IP address of destination.
     * @param port   Destination port.
     * @return  ER_OK if successful.
     */
    QStatus Connect(qcc::String& host, uint16_t port);

    /**
     * Connect the socket to a path destination.
     *
     * @param path   Path name of destination.
     * @return  ER_OK if successful.
     */
    QStatus Connect(qcc::String& path);

    /** Close and shutdown the socket.  */
    void Close();

    /**
     * Pull bytes from the socket.
     * The source is exhausted when ER_EOF is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  [OUT] Actual number of bytes retrieved from source.
     * @param timeout      Timeout in milliseconds.
     * @return   ER_OK if successful. ER_EOF if source is exhausted. Otherwise an error.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);

    /**
     * Pull bytes and any accompanying file/socket descriptors from the stream.
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
    QStatus PullBytesAndFds(void* buf, size_t reqBytes, size_t& actualBytes, SocketFd* fdList, size_t& numFds, uint32_t timeout = Event::WAIT_FOREVER);

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
     * @return  ER_OK or an error.
     */
    QStatus PushBytesAndFds(const void* buf, size_t numBytes, size_t& numSent, SocketFd* fdList, size_t numFds, uint32_t pid = -1);

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
     * Return the socketFd for this SocketStream.
     *
     * @return socketFd
     */
    SocketFd GetSocketFd() { return sock; }

    /**
     * Detach this SocketStream from the underlying socket.
     * Calling this method will cause the underlying socket descriptor to not be shutdown when
     * the SocketStream is closed or destroyed. The socket descriptor will, however, be closed.
     */
    void DetachSocketFd() { isDetached = true; }

    /**
     * Set send timeout.
     *
     * @param sendTimeout    Send timeout in ms or WAIT_FOREVER for infinite
     */
    void SetSendTimeout(uint32_t sendTimeout) { this->sendTimeout = sendTimeout; }

    /**
     * Set TCP based socket to use or not use Nagle algorithm (TCP_NODELAY)
     *
     * @param useNagle  Set to true to Nagle algorithm. Set to false to disable Nagle.
     */
    QStatus SetNagle(bool reuse);

  private:

    /** Private default constructor */
    SocketStream();

    bool isConnected;                /**< true iff socket is connected */
  protected:

    SocketFd sock;                   /**< Socket file descriptor */
    Event* sourceEvent;              /**< Event signaled when data is available */
    Event* sinkEvent;                /**< Event signaled when sink can accept data */
    bool isDetached;                 /**< Detached socket streams do not shutdown the underlying socket when closing */
    uint32_t sendTimeout;            /**< Send timeout */
};

}

#endif
