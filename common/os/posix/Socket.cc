/**
 * @file
 *
 * Define the abstracted socket interface for Linux
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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


QStatus MakeSockAddr(const IPAddress& addr, uint16_t port,
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
        sa.sin6_scope_id = 0;  // TODO: What should go here???
        addrSize = sizeof(sa);
        memcpy(addrBuf, &sa, sizeof(sa));
    }
    return ER_OK;
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

    assert(sockfd);
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


QStatus SendTo(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort,
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

    status = MakeSockAddr(remoteAddr, remotePort, &addr, addrLen);
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

QStatus SetReuseAddrPort(SocketFd sockfd, bool reuse)
{
    QStatus status = ER_OK;
    int arg = reuse ? 1 : -0;

    /* Linux kernels prior to 3.9 needs SO_REUSEADDR but Darwin needs SO_REUSEPORT for this to work. */
#if defined(QCC_OS_DARWIN)
    int r = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (void*)&arg, sizeof(arg));
#else
    int r = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&arg, sizeof(arg));
#endif

    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting SO_REUSEADDR failed: (%d) %s", errno, strerror(errno)));
    }
    return status;
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
    assert(sockFd);
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
    assert(sockFd);
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
    assert(sockFd);
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

} // namespace qcc

