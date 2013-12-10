/**
 * @file
 *
 * Defines the abstracted socket interface types for WinRT.
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

#pragma once

#include <qcc/platform.h>
#include <qcc/SocketTypes.h>

namespace qcc {

namespace winrt {

#ifdef COMMON_WINRT_PUBLIC
public
#else
private
#endif
enum class AddressFamily {
    QCC_AF_UNSPEC = qcc::AddressFamily::QCC_AF_UNSPEC,
    QCC_AF_INET  = qcc::AddressFamily::QCC_AF_INET,
    QCC_AF_INET6 = qcc::AddressFamily::QCC_AF_INET6,
    QCC_AF_UNIX  = qcc::AddressFamily::QCC_AF_UNIX,
};

#ifdef COMMON_WINRT_PUBLIC
public
#else
private
#endif
enum class SocketType {
    QCC_SOCK_STREAM =    qcc::SocketType::QCC_SOCK_STREAM,
    QCC_SOCK_DGRAM =     qcc::SocketType::QCC_SOCK_DGRAM,
    QCC_SOCK_SEQPACKET = qcc::SocketType::QCC_SOCK_SEQPACKET,
    QCC_SOCK_RAW =       qcc::SocketType::QCC_SOCK_RAW,
    QCC_SOCK_RDM =       qcc::SocketType::QCC_SOCK_RDM,
};

}
}

