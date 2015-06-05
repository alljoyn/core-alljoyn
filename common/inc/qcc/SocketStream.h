/**
 * @file SocketStream.h
 *
 * Sink/Source wrapper for Socket.
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
     * Ownership of the underlying socket descriptor passes to this
     * SocketStream: the SocketStream will shutdown the socket when
     * Close() is called and close the socket when this SocketStream
     * is destroyed.
     *
     * @see DetachSocketFd()
     *
     * @param sock      A connected socket handle.
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
     * Assignment operator
     *
     * @param other  SocketStream to assign from.
     */
    SocketStream operator=(const SocketStream& other);

    /**
     * Destructor
     *
     * The destructor will close the underlying socket descriptor.
     */
    virtual ~SocketStream();

    /**
     * Connect the socket to a destination.
     *
     * @param host   Hostname or IP address of destination.
     * @param port   Destination port.
     *
     * @return  ER_OK if successful.
     */
    QStatus Connect(qcc::String& host, uint16_t port);

    /**
     * Connect the socket to a path destination.
     *
     * @param path   Path name of destination.
     *
     * @return  ER_OK if successful.
     */
    QStatus Connect(qcc::String& path);

    /**
     * Shuts down the transmit side of the socket descriptor.
     *
     * This is used to perform an 'orderly' release of the socket.  The orderly
     * release is as follows:
     * -# PushBytes() or PushBytesAndFds() to transmit all bytes.
     * -# Shutdown()
     * -# PullBytes() or PullBytesAndFds() until the receive side is drained
     *    (until PullBytes() returns #ER_SOCK_OTHER_END_CLOSED or #ER_OS_ERROR).
     * -# Close()
     *
     * @see DetachSocketFd()
     *
     * @return
     * - #ER_OK if the underlying socket request succeeds.
     * - #ER_OS_ERROR if the socket has been closed
     * - #ER_FAIL if this SocketStream is detached from the underlying socket.
     */
    QStatus Shutdown();

    /**
     * Sets the socket to discard any queued data and immediately tear down
     * the connection on Close().
     *
     * This is used to perform an 'abortive' release of the socket.  The
     * abortive release is as follows:
     * -# Abort()
     * -# Close()
     *
     * @see DetachSocketFd()
     *
     * @return
     * - #ER_OK if the underlying socket request succeeds.
     * - #ER_OS_ERROR if the socket has been closed
     * - #ER_FAIL if this SocketStream is detached from the underlying socket.
     */
    QStatus Abort();

    /**
     * Closes the socket descriptor.
     */
    void Close();

    /**
     * Pull bytes from the socket.
     *
     * @param buf              Buffer to store pulled bytes
     * @param reqBytes         Number of bytes requested to be pulled from source.
     * @param[out] actualBytes Actual number of bytes retrieved from source.
     * @param timeout          Timeout in milliseconds.
     *
     * @return
     * - #ER_OK if the pull succeeds.
     * - #ER_OS_ERROR if the underlying socket request fails.
     * - #ER_READ_ERROR if the socket is not connected.
     * - #ER_SOCK_OTHER_END_CLOSED if the number of bytes requested is not zero
     *   and the pull returns zero bytes.
     * - #ER_TIMEOUT if the timeout is reached before pulling any bytes.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);

    /**
     * Pull bytes and any accompanying file/socket descriptors from the stream.
     *
     * @param buf              Buffer to store pulled bytes
     * @param reqBytes         Number of bytes requested to be pulled from source.
     * @param[out] actualBytes Actual number of bytes retrieved from source.
     * @param fdList           Array to receive file descriptors.
     * @param[in,out] numFds   On in the size of fdList, on out number of files descriptors pulled.
     * @param timeout          Timeout in milliseconds.
     *
     * @return
     * - #ER_OK if the pull succeeds.
     * - #ER_BAD_ARG_4 if fdList is NULL.
     * - #ER_BAD_ARG_5 if numFds is 0.
     * - #ER_OS_ERROR if the underlying socket request fails or numFds is not
     *   large enough to receive all the descriptors.
     * - #ER_READ_ERROR if the socket is not connected.
     * - #ER_SOCK_OTHER_END_CLOSED if the pull returns zero bytes.
     * - #ER_TIMEOUT if the timeout is reached before pulling any bytes.
     */
    QStatus PullBytesAndFds(void* buf, size_t reqBytes, size_t& actualBytes, SocketFd* fdList, size_t& numFds, uint32_t timeout = Event::WAIT_FOREVER);

    /**
     * Push bytes into the sink.
     *
     * @param buf          Buffer containing bytes to push
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param[out] numSent Number of bytes actually consumed by sink.
     *
     * @return
     * - #ER_OK if numBytes is 0 or the push succeeds.
     * - #ER_WRITE_ERROR if the socket is not connected.
     * - #ER_TIMEOUT if timeout is reached before pushing any bytes.
     * - #ER_OS_ERROR if the underlying socket request fails.
     */
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent);

    /**
     * Push bytes accompanied by one or more file/socket descriptors to a sink.
     *
     * @param buf           Buffer containing bytes to push
     * @param numBytes      Number of bytes from buf to send to sink, must be at least 1.
     * @param[out] numSent  Number of bytes actually consumed by sink.
     * @param fdList        Array of file descriptors to push.
     * @param numFds        Number of files descriptors, must be at least 1.
     * @param pid           Process id required on some platforms.
     *
     * @return
     * - #ER_OK if the push succeeds.
     * - #ER_OS_ERROR if the underlying socket request fails.
     * - #ER_BAD_ARG_2 if numBytes is 0.
     * - #ER_BAD_ARG_4 if fdList is NULL.
     * - #ER_BAD_ARG_5 if numFds is 0 or numFds is greater than #SOCKET_MAX_FILE_DESCRIPTORS.
     * - #ER_WRITE_ERROR if the socket is not connected.
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
     *
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
     *
     * Calling this method will cause the underlying socket descriptor
     * to not be shutdown by this SocketStream.
     */
    void DetachSocketFd() { isDetached = true; }

    /**
     * Set send timeout.
     *
     * @param sendTimeoutMs    Send timeout in ms or WAIT_FOREVER for infinite
     */
    void SetSendTimeout(uint32_t sendTimeoutMs) { this->sendTimeout = sendTimeoutMs; }

    /**
     * Set TCP based socket to use or not use Nagle algorithm (TCP_NODELAY)
     *
     * @param useNagle  Set to true to Nagle algorithm. Set to false to disable Nagle.
     */
    QStatus SetNagle(bool reuse);

  private:

    /*
     * Private constructors and assignment operator that do nothing
     */
    SocketStream();

    bool isConnected;                /**< true iff socket is connected */

  protected:

    SocketFd sock;                   /**< Socket file descriptor */
    Event* sourceEvent;              /**< Event signaled when data is available */
    Event* sinkEvent;                /**< Event signaled when sink can accept data */
    bool isDetached;                 /**< Detached socket streams do not shutdown the underlying socket */
    uint32_t sendTimeout;            /**< Send timeout */
};

}

#endif
