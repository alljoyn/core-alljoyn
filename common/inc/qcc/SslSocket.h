/**
 * @file
 *
 * SSL stream-based socket interface
 *
 */

/******************************************************************************
 * Copyright (c) 2009,2012-2013 AllSeen Alliance. All rights reserved.
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

#ifndef _SSLSOCKET_H
#define _SSLSOCKET_H

#include <qcc/platform.h>
#include <qcc/Stream.h>
#include <qcc/String.h>
#include <qcc/Event.h>

#include <Status.h>

namespace qcc {
/**
 * SSL Socket
 */
class SslSocket : public Stream {

  public:

    /** Construct an SSL socket. */
    SslSocket(String host, const char* rootCert, const char* caCert);

    /** Destroy SSL socket */
    ~SslSocket();

    /**
     * Connect an SSL socket to a remote host on a specified port
     *
     * @param hostname     Destination IP address or hostname.
     * @param port         IP Port on remote host.
     *
     * @return  ER_OK if successful.
     */
    QStatus Connect(const qcc::String hostname, uint16_t port);

    /**
     * Close an SSL socket.
     */
    void Close();

    /**
     * Pull bytes from the source.
     * The source is exhausted when ER_NONE is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @return   OI_OK if successful. ER_NONE if source is exhausted. Otherwise an error.
     */
    QStatus PullBytes(void*buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);

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
     * Get the Event indicating that data is available.
     *
     * @return Event that is set when data is available.
     */
    qcc::Event& GetSourceEvent() { return *sourceEvent; }

    /**
     * Get the Event indicating that sink can accept data.
     *
     * @return Event set when socket can accept more data via PushBytes
     */
    qcc::Event& GetSinkEvent() { return *sinkEvent; }

    /**
     * Import a PEM-encoded public key.
     */
    QStatus ImportPEM(const char* rootCert, const char* caCert);

    /**
     * Return the socketFd for this SslSocket.
     *
     * @return SocketFd
     */
    SocketFd GetSocketFd() { return sock; }

  private:

    /** SslSockets cannot be copied or assigned */
    SslSocket(const SslSocket& other);

    SslSocket operator=(const SslSocket& other);

    /** Forward declaration of OS specific (hidden) internal state for SslSocket */
    struct Internal;

    Internal* internal;        /**< Inernal (OS specific) state for SslSocket instance */
    qcc::Event*sourceEvent;    /**< Event signaled when data is available */
    qcc::Event*sinkEvent;      /**< Event signaled when sink can accept data */
    String Host;               /**< Host to connect to */
    SocketFd sock;             /**< Socket file descriptor */
};

}  /* namespace */

#endif
