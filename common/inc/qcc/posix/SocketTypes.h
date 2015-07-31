/**
 * @file
 *
 * Define the abstracted socket interface for Linux.
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

#include <arpa/inet.h>
#include <limits.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>

/** Windows uses SOCKET_ERROR to signify errors */
#define SOCKET_ERROR -1

namespace qcc {

/**
 * GCC definition of QCC_IOVec matches the POSIX definition of struct iovec for
 * direct casting.
 */
struct IOVec {
    void* buf;  /**< Pointer to a buffer to be included in a scatter-gather list. */
    size_t len; /**< Length of the buffer. */
};

#define QCC_MAX_SG_ENTRIES (IOV_MAX)  /**< Maximum number of scatter-gather list entries. */

typedef socklen_t SockAddrSize;  /**< Abstraction of the socket address length type. */

/**
 * Enumeration of address families.
 */
typedef enum {
    QCC_AF_UNSPEC = PF_UNSPEC,  /**< unspecified address family */
    QCC_AF_INET  = PF_INET,     /**< IPv4 address family */
    QCC_AF_INET6 = PF_INET6,    /**< IPv6 address family */
    QCC_AF_UNIX  = PF_UNIX      /**< UNIX file system sockets address family */
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
    void* name;             /**< IP Address. */
    socklen_t nameLen;      /**< IP Address length. */
    struct IOVec* iov;      /**< Array of scatter-gather entries. */
    size_t iovLen;          /**< Number of elements in iov. */
    void* control;          /**< Ancillary data buffer. */
    socklen_t controlLen;   /**< Ancillary data buffer length. */
    int flags;              /**< Flags on received message. */
};

/**
 * Some of the flags used in SendTo are supported in Linux, but not in Darwin
 */
#define MSG_FLAG_UNSUPPORTED 0

/**
 * Flag bit definitions for the flags passed to sendmsg-related functions.
 * See platform sockets API for detailed descriptions.
 */
typedef enum {
    QCC_MSG_NONE =      0,              /**< No flag bits set */

#if defined(QCC_OS_DARWIN)
    QCC_MSG_CONFIRM =   MSG_FLAG_UNSUPPORTED,
#else
    QCC_MSG_CONFIRM =   MSG_CONFIRM,    /**< Progress happened, don't reprobe using ARP. */
#endif

    QCC_MSG_DONTROUTE = MSG_DONTROUTE,  /**< Don't send to gageway, only send on directly connected networks. */
    QCC_MSG_DONTWAIT =  MSG_DONTWAIT,   /**< Enable nonblocking operation (like O_NONBLOCK with fnctl. */
    QCC_MSG_EOR =       MSG_EOR,        /**< End of record (SOCK_SEQPACKET sockets). */

#if defined(QCC_OS_DARWIN)
    QCC_MSG_MORE =      MSG_FLAG_UNSUPPORTED,
#else
    QCC_MSG_MORE =      MSG_MORE,       /**< More data coming.  See TCP_CORK. */
#endif

#if defined(QCC_OS_DARWIN)
    QCC_MSG_NOSIGNAL =  MSG_FLAG_UNSUPPORTED,
#else
    QCC_MSG_NOSIGNAL =  MSG_NOSIGNAL,   /**< Request to not send SIGPIPE on stream sockets. */
#endif

    QCC_MSG_OOB =       MSG_OOB         /**< Out of band data (SOCK_STREAM sockets). */
} SendMsgFlags;

/**
 * How to shutdown parts of a full-duplex connection.
 */
typedef enum {
    QCC_SHUTDOWN_RD = SHUT_RD, /**< Further receptions will be disallowed */
    QCC_SHUTDOWN_WR = SHUT_WR, /**< Further transmissions will be disallowed */
    QCC_SHUTDOWN_RDWR = SHUT_RDWR /**< Further receptions and transmissions will be disallowed */
} ShutdownHow;

}

#endif
