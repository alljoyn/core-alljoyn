/**
 * @file
 *
 * A collection of wrapper functions that abstract the underlying platform socket APIs.
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
#ifndef _QCC_SOCKET_WRAPPER_H
#define _QCC_SOCKET_WRAPPER_H

#include <qcc/platform.h>
#include <Status.h>

namespace qcc {

/**
 * Platform dependent value for an invalid SocketFd
 */
extern const SocketFd INVALID_SOCKET_FD;

/**
 * Platform dependent value for an max listen backlog
 */
extern const int MAX_LISTEN_CONNECTIONS;

/**
 * Close a socket descriptor.
 *
 * @param sockfd        Socket descriptor.
 */
void Close(SocketFd sockfd);

/**
 * Shutdown a connection.
 *
 * @param sockfd        Socket descriptor.
 *
 * @return  Indication of success of failure.
 */
QStatus Shutdown(SocketFd sockfd);

/**
 * Duplicate a socket descriptor.
 *
 * @param sockfd   The socket descriptor to duplicate
 * @param dupSock  [OUT] The duplicated socket descriptor.
 *
 * @return  #ER_OK if the socket was successfully duplicated.
 *          #Other errors indicating the operation failed.
 */
QStatus SocketDup(SocketFd sockfd, SocketFd& dupSock);

/**
 * Send a buffer of data over a socket.
 *
 * Note that there are some unescapable platform differences when the local side
 * calls Send() after the remote side has shutdown its receive side.  The Send()
 * may succeed or fail with ER_OS_ERROR.
 *
 * @param sockfd        Socket descriptor.
 * @param buf           Pointer to the buffer containing the data to send.  This must not be NULL.
 * @param len           Number of octets in the buffer to be sent.
 * @param[out] sent     Number of octets sent.
 *
 * @return
 * - #ER_OK the send succeeded.
 * - #ER_OS_ERROR the underlying send failed.
 * - #ER_WOULDBLOCK sockfd is non-blocking and the underlying send would block.
 */
QStatus Send(SocketFd sockfd, const void* buf, size_t len, size_t& sent);

/**
 * Receive a buffer of data over a socket.
 *
 * @param sockfd        Socket descriptor.
 * @param buf           Pointer to the buffer where received data will be stored.
 * @param len           Size of the buffer in octets.
 * @param[out] received Number of octets received.
 *
 * @return
 * - #ER_OK the receive succeeded.
 * - #ER_OS_ERROR the underlying receive failed.
 * - #ER_WOULDBLOCK sockfd is non-blocking and the underlying receive would block.
 */
QStatus Recv(SocketFd sockfd, void* buf, size_t len, size_t& received);

}

#endif
