/**
 * @file
 *
 * Define the abstracted socket interface for Windows.
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
#ifndef _OS_QCC_SOCKETTYPES_H
#define _OS_QCC_SOCKETTYPES_H

#include <qcc/platform.h>

#include <Winsock2.h>
#include <Mswsock.h>
#include <ws2tcpip.h>

namespace qcc {

/**
 * CL definition of IOVec matches the Visual Studio definition of
 * WSABUF for direct casting.
 */
struct IOVec {
    u_long len;         ///< Length of the buffer.
    char FAR* buf;      ///< Pointer to a buffer to be included in a
                        //   scatter-gather list.
};

#define ER_MAX_SG_ENTRIES (IOV_MAX)  /**< Maximum number of scatter-gather list entries. */

typedef int SockAddrSize;  /**< Abstraction of the socket address length type. */

/**
 * Enumeration of address families.
 */
typedef enum {
    QCC_AF_UNSPEC = AF_UNSPEC,  /**< unspecified address family */
    QCC_AF_INET  = AF_INET,     /**< IPv4 address family */
    QCC_AF_INET6 = AF_INET6,    /**< IPv6 address family */
    QCC_AF_UNIX  = -1           /**< Not implemented on windows */
} AddressFamily;

/**
 * Enumeration of socket types.
 */
typedef enum {
    QCC_SOCK_STREAM =    SOCK_STREAM,    /**< TCP */
    QCC_SOCK_DGRAM =     SOCK_DGRAM,     /**< UDP */
    QCC_SOCK_SEQPACKET = SOCK_SEQPACKET, /**< Sequenced data transmission */
    QCC_SOCK_RAW =       SOCK_RAW,       /**< Raw IP packet */
    QCC_SOCK_RDM =       SOCK_RDM        /**< Reliable datagram */
} SocketType;


/**
 * The abstract message header structure defined to match the Linux definition of struct msghdr.
 */
struct MsgHdr {
    SOCKADDR* name;         /**< IP Address. */
    int nameLen;            /**< IP Address length. */
    struct IOVec* iov;      /**< Array of scatter-gather entries. */
    DWORD iovLen;           /**< Number of elements in iov. */
    WSABUF control;         /**< Ancillary data buffer. */
    DWORD flags;            /**< Flags on received message. */
};

/**
 * Many of the flags used in SendTo are supported in Posix, but not in Windows
 */
#define MSG_FLAG_UNSUPPORTED 0

/**
 * Flag bit definitions for the flags passed to sendmsg-related functions
 * See platform sockets API for detailed descriptions.
 */
typedef enum {
    QCC_MSG_NONE =      0,                     /**< No flag bits set */
    QCC_MSG_CONFIRM =   MSG_FLAG_UNSUPPORTED,  /**< Progress happened, don't rerpbe using ARP. */
    QCC_MSG_DONTROUTE = MSG_DONTROUTE,         /**< Don't send to gageway, only send on directly connected networks. */
    QCC_MSG_DONTWAIT =  MSG_FLAG_UNSUPPORTED,  /**< Enable nonblocking operation (like O_NONBLOCK with fnctl. */
    QCC_MSG_EOR =       MSG_FLAG_UNSUPPORTED,  /**< End of record (SOCK_SEQPACKET sockets). */
    QCC_MSG_MORE =      MSG_FLAG_UNSUPPORTED,  /**< More data coming.  See TCP_CORK. */
    QCC_MSG_NOSIGNAL =  MSG_FLAG_UNSUPPORTED,  /**< Request to not send SIGPIPE on stream sockets. */
    QCC_MSG_OOB =       MSG_OOB                /**< Out of band data (SOCK_STREAM sockets). */
} SendMsgFlags;

/**
 * How to shutdown parts of a full-duplex connection.
 */
typedef enum {
    QCC_SHUTDOWN_RD = SD_RECEIVE, /**< Further receptions will be disallowed */
    QCC_SHUTDOWN_WR = SD_SEND, /**< Further transmissions will be disallowed */
    QCC_SHUTDOWN_RDWR = SD_BOTH /**< Further receptions and transmissions will be disallowed */
} ShutdownHow;

}

#endif
