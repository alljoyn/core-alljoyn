/**
 * @file
 *
 * Define the abstracted socket interface for Linux
 */

/******************************************************************************
 * Copyright (c) 2009-2014 AllSeen Alliance. All rights reserved.
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

#if defined(QCC_OS_DARWIN)
#define  __APPLE_USE_RFC_3542
#include <netinet/in.h>
#endif

#include <qcc/platform.h>

#include <algorithm>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <unistd.h>
#if defined(QCC_OS_DARWIN)
#include <sys/ucred.h>
#endif

#include <qcc/IPAddress.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <Status.h>

#define QCC_MODULE "NETWORK"

namespace qcc {

const SocketFd INVALID_SOCKET_FD = -1;
const int MAX_LISTEN_CONNECTIONS = SOMAXCONN;

#if defined(QCC_OS_DARWIN)
const int CONNECT_TIMEOUT = 5;
#define MSG_NOSIGNAL 0

static void DisableSigPipe(SocketFd socket)
{
    int disableSigPipe = 1;
    setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &disableSigPipe, sizeof(disableSigPipe));
}
#endif

QStatus MakeSockAddr(const char* path,
                     struct sockaddr_storage* addrBuf, socklen_t& addrSize)
{
    size_t pathLen = strlen(path);
    struct sockaddr_un sa;
    assert((size_t)addrSize >= sizeof(sa));
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    memcpy(sa.sun_path, path, (std::min)(pathLen, sizeof(sa.sun_path) - 1));
    /*
     * We use an @ in the first character position to indicate an abstract socket. Abstract sockets
     * start with a NUL character on Linux.
     */
    if (sa.sun_path[0] == '@') {
#if defined(QCC_OS_LINUX) || defined (QCC_OS_ANDROID)
        sa.sun_path[0] = 0;
        addrSize = offsetof(struct sockaddr_un, sun_path) + pathLen;
#else /* Non-linux platforms */
        QCC_LogError(ER_NOT_IMPLEMENTED, ("Abstract socket paths are not supported"));
        return ER_NOT_IMPLEMENTED;
#endif
    } else {
        addrSize = sizeof(sa);
    }
    memcpy(addrBuf, &sa, sizeof(sa));
    return ER_OK;
}


QStatus MakeSockAddr(const IPAddress& addr, uint16_t port, uint32_t scopeId,
                     struct sockaddr_storage* addrBuf, socklen_t& addrSize)
{
    if (addr.IsIPv4()) {
        struct sockaddr_in sa;
        assert((size_t)addrSize >= sizeof(sa));
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = addr.GetIPv4AddressNetOrder();
        addrSize = sizeof(sa);
        memcpy(addrBuf, &sa, sizeof(sa));
    } else {
        struct sockaddr_in6 sa;
        assert((size_t)addrSize >= sizeof(sa));
        memset(&sa, 0, sizeof(sa));
        sa.sin6_family = AF_INET6;
        sa.sin6_port = htons(port);
        sa.sin6_flowinfo = 0;  // TODO: What should go here???
        addr.RenderIPv6Binary(sa.sin6_addr.s6_addr, sizeof(sa.sin6_addr.s6_addr));
        sa.sin6_scope_id = scopeId;
        addrSize = sizeof(sa);
        memcpy(addrBuf, &sa, sizeof(sa));
    }
    return ER_OK;
}

QStatus MakeSockAddr(const IPAddress& addr, uint16_t port,
                     struct sockaddr_storage* addrBuf, socklen_t& addrSize)
{
    return MakeSockAddr(addr, port, 0, addrBuf, addrSize);
}

QStatus GetSockAddr(const sockaddr_storage* addrBuf, socklen_t addrSize,
                    IPAddress& addr, uint16_t& port)
{
    QStatus status = ER_OK;
    char hostname[NI_MAXHOST];
    char servInfo[NI_MAXSERV];

    int s = getnameinfo((struct sockaddr*) addrBuf,
                        addrSize,
                        hostname,
                        NI_MAXHOST, servInfo,
                        NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

    if (s != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("GetSockAddr: %d - %s", s, gai_strerror(s)));
    } else {
        /*
         * In the case of IPv6, the hostname will have the interface name
         * tacked on the end, as in "fe80::20c:29ff:fe7b:6f10%eth1".  We
         * need to chop that off since nobody expects either the Spanish
         * Inquisition or the interface.
         */
        char* p = strchr(hostname, '%');
        if (p) {
            *p = '\0';
        }
        addr = IPAddress(hostname);
        port = atoi(servInfo);
    }

    return status;
}

uint32_t GetLastError()
{
    return errno;
}

qcc::String GetLastErrorString()
{
    return strerror(errno);
}

QStatus Socket(AddressFamily addrFamily, SocketType type, SocketFd& sockfd)
{
    QStatus status = ER_OK;
    int ret;

    QCC_DbgTrace(("Socket(addrFamily = %d, type = %d, sockfd = <>)", addrFamily, type));

    ret = socket(static_cast<int>(addrFamily), static_cast<int>(type), 0);
    if (ret == -1) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Opening socket: %d - %s", errno, strerror(errno)));
    } else {
        sockfd = static_cast<SocketFd>(ret);
#if defined(QCC_OS_DARWIN)
        DisableSigPipe(sockfd);
#endif
    }
    return status;
}


QStatus Connect(SocketFd sockfd, const IPAddress& remoteAddr, uint16_t remotePort)
{
    QStatus status = ER_OK;
    int ret;
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);
    QCC_DbgTrace(("Connect(sockfd = %d, remoteAddr = %s, remotePort = %hu)",
                  sockfd, remoteAddr.ToString().c_str(), remotePort));

    status = MakeSockAddr(remoteAddr, remotePort, &addr, addrLen);
    if (status != ER_OK) {
        return status;
    }

#if defined(QCC_OS_DARWIN)
    int selectRet;
    fd_set wfdset;
    int so_error;
    socklen_t slen = sizeof(so_error);
    struct timeval tv;
    tv.tv_sec = CONNECT_TIMEOUT;
    tv.tv_usec = 0;

    /* This should always be called after a socket is created */
    FD_ZERO(&wfdset);
    FD_SET(sockfd, &wfdset);

    /* Set the socket to non-blocking since by default our socket is blocking */
    int flags = fcntl(sockfd, F_GETFL, 0);
    ret = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Connect fcntl (sockfd = %u) to O_NONBLOCK: %d - %s", sockfd, errno, strerror(errno)));
    }

    /* Async connect call */
    ret = connect(static_cast<int>(sockfd), reinterpret_cast<struct sockaddr*>(&addr), addrLen);
    if (ret == -1) {
        if ((errno == EINPROGRESS) || (errno == EALREADY)) {
            /* Call select to wait for the connect to take place */
            selectRet = select(sockfd + 1, NULL, &wfdset, NULL, &tv);
            /* select will return 1 when it indicates that the socket is writable */
            if (selectRet == 1) {
                getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &slen);
                if (so_error == 0) {
                    status = ER_OK;
                } else {
                    status = ER_OS_ERROR;
                    QCC_LogError(status, ("Select on socket indicates it is not writable"));
                }
            } else {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("Timeout on connect. The other end may have gone away or not reachable"));
            }
        } else if (errno == EISCONN) {
            status = ER_OK;
        } else if (errno == ECONNREFUSED) {
            status = ER_CONN_REFUSED;
        } else {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Connecting (sockfd = %u) to %s %d: %d - %s", sockfd,
                                  remoteAddr.ToString().c_str(), remotePort,
                                  errno, strerror(errno)));
        }
    }
#else
    ret = connect(static_cast<int>(sockfd), reinterpret_cast<struct sockaddr*>(&addr), addrLen);
    if (ret == -1) {
        if ((errno == EINPROGRESS) || (errno == EALREADY)) {
            status = ER_WOULDBLOCK;
        } else if (errno == EISCONN) {
            status = ER_OK;
        } else if (errno == ECONNREFUSED) {
            status = ER_CONN_REFUSED;
        } else {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Connecting (sockfd = %u) to %s %d: %d - %s", sockfd,
                                  remoteAddr.ToString().c_str(), remotePort,
                                  errno, strerror(errno)));
        }
    } else {
        int flags = fcntl(sockfd, F_GETFL, 0);
        ret = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        if (ret == -1) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Connect fcntl (sockfd = %u) to O_NONBLOCK: %d - %s", sockfd, errno, strerror(errno)));
            /* Higher level code is responsible for closing the socket */
        }
    }
#endif
    return status;
}

QStatus Connect(SocketFd sockfd, const char* pathName)
{
    QStatus status = ER_OK;
    int ret;
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);

    QCC_DbgTrace(("Connect(sockfd = %u, path = %s)", sockfd, pathName));

    status = MakeSockAddr(pathName, &addr, addrLen);
    if (status != ER_OK) {
        return status;
    }

    ret = connect(static_cast<int>(sockfd), reinterpret_cast<struct sockaddr*>(&addr), addrLen);
    if (ret == -1) {
        status = ER_OS_ERROR;
        QCC_DbgHLPrintf(("Connecting (sockfd = %u) to %s : %d - %s", sockfd,
                         pathName,
                         errno, strerror(errno)));
    } else {
        int flags = fcntl(sockfd, F_GETFL, 0);
        ret = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        if (ret == -1) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Connect fcntl (sockfd = %u) to O_NONBLOCK: %d - %s", sockfd, errno, strerror(errno)));
            /* Higher level code is responsible for closing the socket */
        }
    }
    return status;
}


QStatus Bind(SocketFd sockfd, const IPAddress& localAddr, uint16_t localPort)
{
    QStatus status = ER_OK;
    int ret;
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);

    QCC_DbgTrace(("Bind(sockfd = %d, localAddr = %s, localPort = %hu)",
                  sockfd, localAddr.ToString().c_str(), localPort));

    status = MakeSockAddr(localAddr, localPort, &addr, addrLen);
    if (status != ER_OK) {
        return status;
    }

    ret = bind(static_cast<int>(sockfd), reinterpret_cast<struct sockaddr*>(&addr), addrLen);
    if (ret != 0) {
        status = (errno == EADDRNOTAVAIL ? ER_SOCKET_BIND_ERROR : ER_OS_ERROR);
        QCC_LogError(status, ("Binding (sockfd = %u) to %s %d: %d - %s", sockfd,
                              localAddr.ToString().c_str(), localPort, errno, strerror(errno)));
    }
    return status;
}


QStatus Bind(SocketFd sockfd, const char* pathName)
{
    QStatus status = ER_OK;
    int ret;
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);

    QCC_DbgTrace(("Bind(sockfd = %d, pathName = %s)", sockfd, pathName));

    status = MakeSockAddr(pathName, &addr, addrLen);
    if (status != ER_OK) {
        return status;
    }

    ret = bind(static_cast<int>(sockfd), reinterpret_cast<struct sockaddr*>(&addr), addrLen);
    if (ret != 0) {
        status = (errno == EADDRNOTAVAIL ? ER_SOCKET_BIND_ERROR : ER_OS_ERROR);
        QCC_LogError(status, ("Binding (sockfd = %u) to %s: %d - %s", sockfd,
                              pathName, errno, strerror(errno)));
    }
    return status;
}


QStatus Listen(SocketFd sockfd, int backlog)
{
    QStatus status = ER_OK;
    int ret;

    QCC_DbgTrace(("Listen(sockfd = %d, backlog = %d)", sockfd, backlog));

    ret = listen(static_cast<int>(sockfd), backlog);
    if (ret != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Listening (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
    }
    return status;
}


QStatus Accept(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort, SocketFd& newSockfd)
{
    QStatus status = ER_OK;
    int ret;
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);

    QCC_DbgTrace(("Accept(sockfd = %d, remoteAddr = <>, remotePort = <>)", sockfd));

    ret = accept(static_cast<int>(sockfd), reinterpret_cast<struct sockaddr*>(&addr), &addrLen);
    if (ret == -1) {
        if (errno == EWOULDBLOCK) {
            status = ER_WOULDBLOCK;
        } else {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Accept (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
        }
    } else {
        if (addr.ss_family == AF_INET) {
            struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(&addr);
            uint8_t* portBuf = reinterpret_cast<uint8_t*>(&sa->sin_port);
            remoteAddr = IPAddress(reinterpret_cast<uint8_t*>(&sa->sin_addr.s_addr),
                                   IPAddress::IPv4_SIZE);
            remotePort = (static_cast<uint16_t>(portBuf[0]) << 8) | static_cast<uint16_t>(portBuf[1]);
        } else if (addr.ss_family == AF_INET6) {
            struct sockaddr_in6* sa = reinterpret_cast<struct sockaddr_in6*>(&addr);
            uint8_t* portBuf = reinterpret_cast<uint8_t*>(&sa->sin6_port);
            remoteAddr = IPAddress(reinterpret_cast<uint8_t*>(&sa->sin6_addr.s6_addr),
                                   IPAddress::IPv6_SIZE);
            remotePort = (static_cast<uint16_t>(portBuf[0]) << 8) | static_cast<uint16_t>(portBuf[1]);
        } else {
            remotePort = 0;
        }
        newSockfd = static_cast<SocketFd>(ret);
#if defined(QCC_OS_DARWIN)
        DisableSigPipe(newSockfd);
#endif
        QCC_DbgPrintf(("New socket FD: %d", newSockfd));

        int flags = fcntl(newSockfd, F_GETFL, 0);
        ret = fcntl(newSockfd, F_SETFL, flags | O_NONBLOCK);

        if (ret == -1) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Accept fcntl (newSockfd = %u) to O_NONBLOCK: %d - %s", newSockfd, errno, strerror(errno)));
            /* better to close and error out than to leave in unexpected state */
            qcc::Close(newSockfd);
        }
    }
    return status;
}


QStatus Accept(SocketFd sockfd, SocketFd& newSockfd)
{
    IPAddress addr;
    uint16_t port;
    return Accept(sockfd, addr, port, newSockfd);
}


QStatus Shutdown(SocketFd sockfd)
{
    QStatus status = ER_OK;
    int ret;

    QCC_DbgTrace(("Shutdown(sockfd = %d)", sockfd));

    ret = shutdown(static_cast<int>(sockfd), SHUT_RDWR);
    if (ret != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Shutdown socket (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
    }
    return status;
}


void Close(SocketFd sockfd)
{
    assert(sockfd >= 0);
    close(static_cast<int>(sockfd));
}

QStatus SocketDup(SocketFd sockfd, SocketFd& dupSock)
{
    QStatus status = ER_OK;

    dupSock = dup(sockfd);
    if (dupSock < 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("SocketDup of %d failed %d - %s", sockfd, errno, strerror(errno)));
    }
    return status;
}

QStatus GetLocalAddress(SocketFd sockfd, IPAddress& addr, uint16_t& port)
{
    QStatus status = ER_OK;
    struct sockaddr_storage addrBuf;
    socklen_t addrLen = sizeof(addrBuf);
    int ret;

    QCC_DbgTrace(("GetLocalAddress(sockfd = %d, addr = <>, port = <>)", sockfd));

    memset(&addrBuf, 0, addrLen);

    ret = getsockname(static_cast<int>(sockfd), reinterpret_cast<struct sockaddr*>(&addrBuf), &addrLen);

    if (ret == -1) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Geting Local Address (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
    } else {
        if (addrBuf.ss_family == AF_INET) {
            struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(&addrBuf);
            uint8_t* portBuf = reinterpret_cast<uint8_t*>(&sa->sin_port);
            QCC_DbgLocalData(&addrBuf, sizeof(*sa));
            addr = IPAddress(reinterpret_cast<uint8_t*>(&sa->sin_addr.s_addr), IPAddress::IPv4_SIZE);
            port = (static_cast<uint16_t>(portBuf[0]) << 8) | static_cast<uint16_t>(portBuf[1]);
        } else {
            struct sockaddr_in6* sa = reinterpret_cast<struct sockaddr_in6*>(&addrBuf);
            uint8_t* portBuf = reinterpret_cast<uint8_t*>(&sa->sin6_port);
            addr = IPAddress(reinterpret_cast<uint8_t*>(&sa->sin6_addr.s6_addr), IPAddress::IPv6_SIZE);
            port = (static_cast<uint16_t>(portBuf[0]) << 8) | static_cast<uint16_t>(portBuf[1]);
        }
        QCC_DbgPrintf(("Local Address (sockfd = %u): %s - %u", sockfd, addr.ToString().c_str(), port));
    }

    return status;
}


QStatus Send(SocketFd sockfd, const void* buf, size_t len, size_t& sent)
{
    QStatus status = ER_OK;
    ssize_t ret;

    QCC_DbgTrace(("Send(sockfd = %d, *buf = <>, len = %lu, sent = <>)",
                  sockfd, len));
    assert(buf != NULL);

    QCC_DbgLocalData(buf, len);

    while (status == ER_OK) {
        ret = send(static_cast<int>(sockfd), buf, len, MSG_NOSIGNAL);
        if (ret == -1) {
            if (errno == EAGAIN) {
                status = ER_WOULDBLOCK;
            } else {
                status = ER_OS_ERROR;
                QCC_DbgHLPrintf(("Send (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
            }
        } else {
            sent = static_cast<size_t>(ret);
        }
        break;
    }
    return status;
}

QStatus SendTo(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort, uint32_t scopeId,
               const void* buf, size_t len, size_t& sent)
{
    QStatus status = ER_OK;
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);
    ssize_t ret;

    QCC_DbgTrace(("SendTo(sockfd = %d, remoteAddr = %s, remotePort = %u, *buf = <>, len = %lu, sent = <>)",
                  sockfd, remoteAddr.ToString().c_str(), remotePort, len));
    assert(buf != NULL);

    QCC_DbgLocalData(buf, len);

    status = MakeSockAddr(remoteAddr, remotePort, scopeId, &addr, addrLen);
    if (status != ER_OK) {
        return status;
    }

    ret = sendto(static_cast<int>(sockfd), buf, len, MSG_NOSIGNAL,
                 reinterpret_cast<struct sockaddr*>(&addr), addrLen);
    if (ret == -1) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("SendTo (sockfd = %u  addr = %s  port = %u): %d - %s",
                              sockfd, remoteAddr.ToString().c_str(), remotePort, errno, strerror(errno)));
    } else {
        sent = static_cast<size_t>(ret);
    }
    return status;
}

QStatus SendTo(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort,
               const void* buf, size_t len, size_t& sent)
{
    return SendTo(sockfd, remoteAddr, remotePort, 0, buf, len, sent);
}

QStatus Recv(SocketFd sockfd, void* buf, size_t len, size_t& received)
{
    QStatus status = ER_OK;
    ssize_t ret;

    QCC_DbgTrace(("Recv(sockfd = %d, buf = <>, len = %lu, received = <>)", sockfd, len));
    assert(buf != NULL);

    ret = recv(static_cast<int>(sockfd), buf, len, 0);
    if ((ret == -1) && (errno == EWOULDBLOCK)) {
        return ER_WOULDBLOCK;
    }

    if (ret == -1) {
        status = ER_OS_ERROR;
        QCC_DbgHLPrintf(("Recv (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
    } else {
        received = static_cast<size_t>(ret);
        QCC_DbgRemoteData(buf, received);
    }

    return status;
}


QStatus RecvFrom(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort,
                 void* buf, size_t len, size_t& received)
{
    QStatus status = ER_OK;
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);
    ssize_t ret;
    received = 0;

    QCC_DbgTrace(("RecvFrom(sockfd = %d, remoteAddr = %s, remotePort = %u, buf = <>, len = %lu, received = <>)",
                  sockfd, remoteAddr.ToString().c_str(), remotePort, len));
    assert(buf != NULL);

    ret = recvfrom(static_cast<int>(sockfd), buf, len, 0,
                   reinterpret_cast<struct sockaddr*>(&addr), &addrLen);
    if (ret == -1) {
        status = ER_OS_ERROR;
        QCC_DbgHLPrintf(("RecvFrom (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
    } else {
        received = static_cast<size_t>(ret);
        GetSockAddr(&addr, addrLen, remoteAddr, remotePort);
        QCC_DbgPrintf(("Received %u bytes, remoteAddr = %s, remotePort = %u",
                       received, remoteAddr.ToString().c_str(), remotePort));
    }

    QCC_DbgRemoteData(buf, received);

    return status;
}

QStatus RecvWithAncillaryData(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort, IPAddress& localAddr,
                              void* buf, size_t len, size_t& received, int32_t& interfaceIndex)
{
    QStatus status = ER_OK;
    received = 0;
    interfaceIndex = -1;
    uint16_t localPort;

    struct iovec iov[] = { { buf, len } };

    char cbuf[1024];

    struct msghdr msg;
    memset(&msg, 0, sizeof(struct msghdr));

    msg.msg_iov = iov;
    msg.msg_iovlen = ArraySize(iov);
    msg.msg_control = cbuf;
    msg.msg_controllen = (sizeof(cbuf) / sizeof(cbuf[0]));

    struct sockaddr_storage src;
    struct sockaddr_storage dst;
    memset(&src, 0, sizeof(struct sockaddr_storage));
    memset(&dst, 0, sizeof(struct sockaddr_storage));

    IPAddress addr;
    uint16_t port;
    status = GetLocalAddress(sockfd, addr, port);

    if (status == ER_OK && addr.GetAddressFamily() == QCC_AF_INET) {
        reinterpret_cast<struct sockaddr_in*>(&src)->sin_port = port;
        reinterpret_cast<struct sockaddr_in*>(&src)->sin_family = AF_INET;
        msg.msg_name = reinterpret_cast<struct sockaddr_in*>(&src);
        msg.msg_namelen = sizeof(struct sockaddr_in);
    } else if (status == ER_OK && addr.GetAddressFamily()  == QCC_AF_INET6) {
        reinterpret_cast<struct sockaddr_in6*>(&src)->sin6_port = port;
        reinterpret_cast<struct sockaddr_in6*>(&src)->sin6_family = AF_INET6;
        msg.msg_name = reinterpret_cast<struct sockaddr_in6*>(&src);
        msg.msg_namelen = sizeof(struct sockaddr_in6);
    } else {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("RecvWithAncillaryData (sockfd = %u): unknown address family", sockfd));
        return status;
    }

    assert(buf != NULL);

    ssize_t ret = recvmsg(static_cast<int>(sockfd), &msg, 0);

    if (ret < 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("RecvWithAncillaryData (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
        return status;
    }
    received = static_cast<size_t>(ret);

    struct cmsghdr* cmsg;
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if ((cmsg->cmsg_level == IPPROTO_IP) && (cmsg->cmsg_type == IP_PKTINFO)) {
            struct in_pktinfo* i = reinterpret_cast<struct in_pktinfo*>(CMSG_DATA(cmsg));
            reinterpret_cast<struct sockaddr_in*>(&dst)->sin_addr = i->ipi_addr;
            reinterpret_cast<struct sockaddr_in*>(&dst)->sin_family = AF_INET;
            interfaceIndex =  i->ipi_ifindex;
            reinterpret_cast<struct sockaddr*>(&src)->sa_family = AF_INET;
            status = GetSockAddr(&src, sizeof(struct sockaddr_in), remoteAddr, remotePort);
            if (status == ER_OK) {
                status = GetSockAddr(&dst, sizeof(struct sockaddr_in), localAddr, localPort);
            }
            break;
        }
        if ((cmsg->cmsg_level == IPPROTO_IPV6) && (cmsg->cmsg_type == IPV6_PKTINFO)) {
            struct in6_pktinfo* i = reinterpret_cast<struct in6_pktinfo*>(CMSG_DATA(cmsg));
            reinterpret_cast<struct sockaddr_in6*>(&dst)->sin6_addr = i->ipi6_addr;
            reinterpret_cast<struct sockaddr_in6*>(&dst)->sin6_family = AF_INET6;
            interfaceIndex =  i->ipi6_ifindex;
            reinterpret_cast<struct sockaddr*>(&src)->sa_family = AF_INET6;
            status = GetSockAddr(&src, sizeof(struct sockaddr_in6), remoteAddr, remotePort);
            if (status == ER_OK) {
                status = GetSockAddr(&dst, sizeof(struct sockaddr_in6), localAddr, localPort);
            }
            break;
        }
    }
    QCC_DbgRemoteData(buf, received);

    return status;
}

QStatus RecvWithFds(SocketFd sockfd, void* buf, size_t len, size_t& received, SocketFd* fdList, size_t maxFds, size_t& recvdFds)
{
    QStatus status = ER_OK;

    if (!fdList) {
        return ER_BAD_ARG_5;
    }
    if (!maxFds) {
        return ER_BAD_ARG_6;
    }
    QCC_DbgHLPrintf(("RecvWithFds"));

    recvdFds = 0;
    maxFds = std::min(maxFds, SOCKET_MAX_FILE_DESCRIPTORS);

    struct iovec iov[] = { { buf, len } };

    static const size_t sz = CMSG_SPACE(sizeof(struct ucred)) + CMSG_SPACE(SOCKET_MAX_FILE_DESCRIPTORS * sizeof(SocketFd));
    char cbuf[sz];

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    msg.msg_iov = iov;
    msg.msg_iovlen = ArraySize(iov);
    msg.msg_control = cbuf;
    msg.msg_controllen = CMSG_LEN(sz);

    ssize_t ret = recvmsg(sockfd, &msg, 0);
    if (ret == -1) {
        if (errno == EWOULDBLOCK) {
            status = ER_WOULDBLOCK;
        } else {
            status = ER_OS_ERROR;
            QCC_DbgHLPrintf(("RecvWithFds (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
        }
    } else {
        struct cmsghdr* cmsg;
        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if ((cmsg->cmsg_level == SOL_SOCKET) && (cmsg->cmsg_type == SCM_RIGHTS)) {
                recvdFds = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(SocketFd);
                /*
                 * Check we have enough room to return the file descriptors.
                 */
                if (recvdFds > maxFds) {
                    status = ER_OS_ERROR;
                    QCC_LogError(status, ("Too many handles: %d implementation limit is %d", recvdFds, maxFds));
                } else {
                    memcpy(fdList, CMSG_DATA(cmsg), recvdFds * sizeof(SocketFd));
                    QCC_DbgHLPrintf(("Received %d handles %d...", recvdFds, fdList[0]));
                }
                break;
            }
        }
        received = static_cast<size_t>(ret);
    }
    return status;
}

QStatus SendWithFds(SocketFd sockfd, const void* buf, size_t len, size_t& sent, SocketFd* fdList, size_t numFds, uint32_t pid)
{
    QStatus status = ER_OK;

    if (!fdList) {
        return ER_BAD_ARG_5;
    }
    if (!numFds || (numFds > SOCKET_MAX_FILE_DESCRIPTORS)) {
        return ER_BAD_ARG_6;
    }

    QCC_DbgHLPrintf(("SendWithFds"));

    struct iovec iov[] = { { const_cast<void*>(buf), len } };
    size_t sz = numFds * sizeof(SocketFd);
    char* cbuf = new char[CMSG_SPACE(sz)];
    memset(cbuf, 0, CMSG_SPACE(sz));

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    msg.msg_iov = iov;
    msg.msg_iovlen = ArraySize(iov);
    msg.msg_control = cbuf;
    msg.msg_controllen = CMSG_SPACE(sz);

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sz);

    QCC_DbgHLPrintf(("Sending %d file descriptors %d...", numFds, fdList[0]));

    memcpy(CMSG_DATA(cmsg), fdList, sz);

    ssize_t ret = sendmsg(sockfd, &msg, 0);
    if (ret == -1) {
        status = ER_OS_ERROR;
        QCC_DbgHLPrintf(("SendWithFds (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
    } else {
        sent = static_cast<size_t>(ret);
    }

    delete [] cbuf;

    return status;
}

QStatus SocketPair(SocketFd(&sockets)[2])
{
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    return (ret == 0) ? ER_OK : ER_FAIL;
}

QStatus SetBlocking(SocketFd sockfd, bool blocking)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    ssize_t ret;
    if (blocking) {
        ret = fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);
    } else {
        ret = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    }
    if (ret == -1) {
        return ER_OS_ERROR;
    } else {
        return ER_OK;
    }
}

QStatus SetSndBuf(SocketFd sockfd, size_t bufSize)
{
    QStatus status = ER_OK;
    int arg = bufSize;
    int r = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (void*)&arg, sizeof(arg));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting SO_SNDBUF failed: (%d) %s", errno, strerror(errno)));
    }
    return status;
}

QStatus GetSndBuf(SocketFd sockfd, size_t& bufSize)
{
    QStatus status = ER_OK;
    int arg = 0;
    socklen_t len = sizeof(arg);
    int r = getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (void*)&arg, &len);
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Getting SO_SNDBUF failed: (%d) %s", errno, strerror(errno)));
    }
    bufSize = arg;
    return status;
}

QStatus SetRcvBuf(SocketFd sockfd, size_t bufSize)
{
    QStatus status = ER_OK;
    int arg = bufSize;
    int r = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void*)&arg, sizeof(arg));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting SO_RCVBUF failed: (%d) %s", errno, strerror(errno)));
    }
    return status;
}

QStatus GetRcvBuf(SocketFd sockfd, size_t& bufSize)
{
    QStatus status = ER_OK;
    int arg = 0;
    socklen_t len = sizeof(arg);
    int r = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void*)&arg, &len);
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Getting SO_RCVBUF failed: (%d) %s", errno, strerror(errno)));
    }
    bufSize = arg;
    return status;
}

QStatus SetLinger(SocketFd sockfd, bool onoff, uint32_t linger)
{
    QStatus status = ER_OK;
    struct linger l;
    l.l_onoff = onoff;
    l.l_linger = linger;

    int r = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void*)&l, sizeof(l));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting SO_LINGER failed: (%d) %s", errno, strerror(errno)));
    }
    return status;
}

QStatus SetNagle(SocketFd sockfd, bool useNagle)
{
    QStatus status = ER_OK;
    int arg = useNagle ? 1 : -0;
    int r = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void*)&arg, sizeof(int));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting TCP_NODELAY failed: (%d) %s", errno, strerror(errno)));
    }
    return status;
}

/*
 * Some systems do not define SO_REUSEPORT (which is a BSD-ism from the first
 * days of multicast support).  In this case they special case SO_REUSEADDR in
 * the presence of multicast addresses to perform the same function, which is to
 * allow multiple processes to bind to the same multicast address/port.  In this
 * case, SO_REUSEADDR provides the equivalent functionality of SO_REUSEPORT, so
 * it is quite safe to substitute them.
 */
QStatus SetReuseAddress(SocketFd sockfd, bool reuse)
{
    QStatus status = ER_OK;
    int arg = reuse ? 1 : 0;

    /* Linux kernels prior to 3.9 needs SO_REUSEADDR */
    int r = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&arg, sizeof(arg));

    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting SO_REUSEADDR failed: (%d) %s", errno, strerror(errno)));
    }
    return status;
}



QStatus SetReusePort(SocketFd sockfd, bool reuse)
{
#if defined(QCC_OS_DARWIN)
    QStatus status = ER_OK;
    int arg = reuse ? 1 : 0;
    int r = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (void*)&arg, sizeof(arg));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting SO_REUSEPORT failed: (%d) %s", errno, strerror(errno)));
    }
    return status;
#else
    return SetReuseAddress(sockfd, reuse);
#endif
}

#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif

#ifndef IPV6_DROP_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif

/*
 * Getting set to do a multicast join or drop is straightforward but not
 * completely trivial, and the process is identical for both socket options, so
 * we only do the work in one place and select one of the followin oeprations.
 */
enum GroupOp {JOIN, LEAVE};

QStatus MulticastGroupOpInternal(SocketFd sockFd, AddressFamily family, String multicastGroup, String iface, GroupOp op)
{
    /*
     * We assume that No external API will be trying to call here and so asserts
     * are appropriate when checking for completely bogus parameters.
     */
    assert(sockFd >= 0);
    assert(family == AF_INET || family == AF_INET6);
    assert(multicastGroup.size());
    assert(iface.size());
    assert(op == JOIN || op == LEAVE);
    /*
     * Joining a multicast group requires a different approach based on the
     * address family of the socket.  There's no way to get this information
     * from an unbound socket, and it is not unreasonable to join a multicast
     * group before binding; so to avoid an inscrutable initialization order
     * requirement we force the caller to provide this tidbit.
     */
    if (family == QCC_AF_INET) {
        /*
         * Group memberships are associated with both the multicast group itself
         * and also an interface.  In the IPv4 version, we need to provide an
         * interface address.  We borrow the socket passed in to do the required
         * call to find the address from the interface name.
         */
        struct ifreq ifr;
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';

        int rc = ioctl(sockFd, SIOCGIFADDR, &ifr);
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("ioctl(SIOCGIFADDR) failed: (%d) %s", errno, strerror(errno)));
            return ER_OS_ERROR;
        }

        struct ip_mreq mreq;
        mreq.imr_interface.s_addr = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;

        rc = inet_pton(AF_INET, multicastGroup.c_str(), &mreq.imr_multiaddr);
        if (rc != 1) {
            QCC_LogError(ER_OS_ERROR, ("inet_pton() failed: %d - %s", errno, strerror(errno)));
            return ER_OS_ERROR;
        }

        int opt = op == JOIN ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
        rc = setsockopt(sockFd, IPPROTO_IP, opt, reinterpret_cast<const char*>(&mreq), sizeof(mreq));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(%s) failed: %d - %s", op == JOIN ? "IP_ADD_MEMBERSHIP" : "IP_DROP_MEMBERSHIP", errno, strerror(errno)));
            return ER_OS_ERROR;
        }
    } else if (family == QCC_AF_INET6) {
        /*
         * Group memberships are associated with both the multicast group itself
         * and also an interface.  In the IPv6 version, we need to provide an
         * interface index instead of an IP address associated with the
         * interface.
         */
        struct ipv6_mreq mreq;
        mreq.ipv6mr_interface = if_nametoindex(iface.c_str());
        if (mreq.ipv6mr_interface == 0) {
            QCC_LogError(ER_OS_ERROR, ("if_nametoindex() failed: unknown interface"));
            return ER_OS_ERROR;
        }

        int rc = inet_pton(AF_INET6, multicastGroup.c_str(), &mreq.ipv6mr_multiaddr);
        if (rc != 1) {
            QCC_LogError(ER_OS_ERROR, ("inet_pton() failed: %d - %s", errno, strerror(errno)));
            return ER_OS_ERROR;
        }

        int opt = op == JOIN ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP;
        rc = setsockopt(sockFd, IPPROTO_IPV6, opt, reinterpret_cast<const char*>(&mreq), sizeof(mreq));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(IP_ADD_MEMBERSHIP) failed: %d - %s", errno, strerror(errno)));
            return ER_OS_ERROR;
        }
    }
    return ER_OK;
}

QStatus JoinMulticastGroup(SocketFd sockFd, AddressFamily family, String multicastGroup, String iface)
{
    return MulticastGroupOpInternal(sockFd, family, multicastGroup, iface, JOIN);
}

QStatus LeaveMulticastGroup(SocketFd sockFd, AddressFamily family, String multicastGroup, String iface)
{
    return MulticastGroupOpInternal(sockFd, family, multicastGroup, iface, LEAVE);
}

QStatus SetMulticastInterface(SocketFd sockFd, AddressFamily family, qcc::String iface)
{
    /*
     * We assume that No external API will be trying to call here and so asserts
     * are appropriate when checking for completely bogus parameters.
     */
    assert(sockFd >= 0);
    assert(family == AF_INET || family == AF_INET6);
    assert(iface.size());

    /*
     * Associating the multicast interface with a socket requires a different
     * approach based on the address family of the socket.  There's no way to
     * get this information from an unbound socket, and it is not unreasonable
     * to set the interface before binding; so to avoid an inscrutable
     * initialization order requirement we force the caller to provide this
     * tidbit.
     */
    if (family == QCC_AF_INET) {
        /*
         * In the IPv4 version, we need to provide an interface address.  We
         * borrow the socket passed in to do the required call to find the
         * address from the interface name.
         */
        struct ifreq ifr;
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';

        int rc = ioctl(sockFd, SIOCGIFADDR, &ifr);
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("ioctl(SIOCGIFADDR) failed: (%d) %s", errno, strerror(errno)));
            return ER_OS_ERROR;
        }

        struct in_addr addr;
        addr.s_addr = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;

        rc = setsockopt(sockFd, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast<const char*>(&addr), sizeof(addr));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(IP_MULTICAST_IF) failed: %d - %s", errno, strerror(errno)));
            return ER_OS_ERROR;
        }
    } else if (family == QCC_AF_INET6) {
        /*
         * In the IPv6 version, we need to provide an interface index instead of
         * an IP address associated with the interface.
         */
        uint32_t index = if_nametoindex(iface.c_str());

        int rc = setsockopt(sockFd, IPPROTO_IPV6, IPV6_MULTICAST_IF, reinterpret_cast<const char*>(&index), sizeof(index));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(IP_MULTICAST_IF) failed: %d - %s", errno, strerror(errno)));
            return ER_OS_ERROR;
        }
    }
    return ER_OK;
}

QStatus SetMulticastHops(SocketFd sockFd, AddressFamily family, uint32_t hops)
{
    /*
     * We assume that No external API will be trying to call here and so asserts
     * are appropriate when checking for completely bogus parameters.
     */
    assert(sockFd >= 0);
    assert(family == AF_INET || family == AF_INET6);

    /*
     * IPv4 and IPv6 are almost the same.  Of course, not quite, though.
     */
    if (family == QCC_AF_INET) {
        int rc = setsockopt(sockFd, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char*>(&hops), sizeof(hops));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(IP_MULTICAST_TTL) failed: %d - %s", errno, strerror(errno)));
            return ER_OS_ERROR;
        }
    } else if (family == QCC_AF_INET6) {
        int rc = setsockopt(sockFd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, reinterpret_cast<const char*>(&hops), sizeof(hops));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(IP_MULTICAST_HOPS) failed: %d - %s", errno, strerror(errno)));
            return ER_OS_ERROR;
        }
    }
    return ER_OK;
}

QStatus SetBroadcast(SocketFd sockfd, bool broadcast)
{
    QStatus status = ER_OK;
    int arg = broadcast ? 1 : -0;
    int r = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void*)&arg, sizeof(arg));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting SO_BROADCAST failed: (%d) %s", errno, strerror(errno)));
    }
    return status;
}

QStatus SetRecvPktAncillaryData(SocketFd sockfd, AddressFamily addrFamily, bool recv)
{
    /*
     * We assume that No external API will be trying to call here and so asserts
     * are appropriate when checking for completely bogus parameters.
     */
    assert(sockfd >= 0);
    assert(addrFamily == AF_INET || addrFamily == AF_INET6);

    QStatus status = ER_OK;
    int arg = recv ? 1 : -0;
    if (addrFamily == QCC_AF_INET) {
        int r = setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, (void*)&arg, sizeof(arg));
        if (r != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Setting IP_PKTINFO failed: (%d) %s", errno, strerror(errno)));
        }
    } else if (addrFamily == QCC_AF_INET6) {
        int r = setsockopt(sockfd, IPPROTO_IPV6, IPV6_RECVPKTINFO, (char*)&arg, sizeof(arg));
        if (r != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Setting IPV6_RECVPKTINFO failed: (%d) %s", errno, strerror(errno)));
        }
    }
    return status;
}

QStatus SetRecvIPv6Only(SocketFd sockfd, bool recv)
{
    QStatus status = ER_OK;
    int arg = recv ? 1 : -0;
    int r = setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&arg, sizeof(arg));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting IPV6_V6ONLY failed: (%d) %s", errno, strerror(errno)));
    }
    return status;
}

} // namespace qcc

