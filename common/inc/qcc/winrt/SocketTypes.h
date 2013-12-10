/**
 * @file
 *
 * Define the abstracted socket interface for WinRT.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

namespace qcc {

/** Windows uses SOCKET_ERROR to signify errors */
#define SOCKET_ERROR -1

/**
 * CL definition of IOVec matches the Visual Studio definition of
 * WSABUF for direct casting.
 */
struct IOVec {
    ULONG len;          ///< Length of the buffer.
    char FAR* buf;      ///< Pointer to a buffer to be included in a
                        //   scatter-gather list.
};

/**
 * Enumeration of address families.
 */
typedef enum {
    QCC_AF_UNSPEC = -1, /**< unspecified address family */
    QCC_AF_INET  = 1,  /**< IPv4 address family */
    QCC_AF_INET6 = 2, /**< IPv6 address family */
    QCC_AF_UNIX  = -1        /**< Not implemented on windows */
} AddressFamily;

/**
 * Enumeration of socket types.
 */
typedef enum {
    QCC_SOCK_STREAM =    1,    /**< TCP */
    QCC_SOCK_DGRAM =     2,     /**< UDP */
    QCC_SOCK_SEQPACKET = -1, /**< Sequenced data transmission */
    QCC_SOCK_RAW =       -1,       /**< Raw IP packet */
    QCC_SOCK_RDM =       -1        /**< Reliable datagram */
} SocketType;
}

#endif
