/**
 * @file
 *
 * Define the abstracted socket interface for Windows
 */

/******************************************************************************
 * Copyright (c) 2009-2011,2014, AllSeen Alliance. All rights reserved.
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

// Do not change the order of these includes; they are order dependent.
#include <Winsock2.h>
#include <Mswsock.h>
#include <ws2tcpip.h>

#include <qcc/IPAddress.h>
#include <qcc/Socket.h>
#include <qcc/IfConfig.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>
#include <qcc/StringUtil.h>
#include <qcc/windows/utility.h>

#include <Status.h>

#define QCC_MODULE "NETWORK"

/* Scatter gather only support on Vista and later */
#if !defined (NTDDI_VERSION) || !defined (NTDDI_VISTA) || (NTDDI_VERSION < NTDDI_VISTA)
#define QCC_USE_SCATTER_GATHER 0
#else
#define QCC_USE_SCATTER_GATHER 1
#endif


namespace qcc {

const SocketFd INVALID_SOCKET_FD = INVALID_SOCKET;
const int MAX_LISTEN_CONNECTIONS = SOMAXCONN;

qcc::String StrError()
{
    WinsockCheck();
    int errnum = WSAGetLastError();
    char msgbuf[256];

    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                        NULL,
                        errnum,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPSTR)msgbuf,
                        sizeof(msgbuf),
                        NULL)) {
        msgbuf[0] = '\0';
    }
    return U32ToString(errnum) + " - " + msgbuf;
}

void MakeSockAddr(const IPAddress& addr,
                  uint16_t port,
                  uint32_t scopeId,
                  SOCKADDR_STORAGE* addrBuf,
                  socklen_t& addrSize)
{
    memset(addrBuf, 0, addrSize);
    if (addr.IsIPv4()) {
        struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(addrBuf);
        sa->sin_family = AF_INET;
        sa->sin_port = htons(port);
        sa->sin_addr.s_addr = addr.GetIPv4AddressNetOrder();
        addrSize = sizeof(*sa);
    } else {
        struct sockaddr_in6* sa = reinterpret_cast<struct sockaddr_in6*>(addrBuf);
        sa->sin6_family = AF_INET6;
        sa->sin6_port = htons(port);
        sa->sin6_flowinfo = 0;
        addr.RenderIPv6Binary(sa->sin6_addr.s6_addr, sizeof(sa->sin6_addr.s6_addr));
        sa->sin6_scope_id = scopeId;
        addrSize = sizeof(*sa);
    }
}

void MakeSockAddr(const IPAddress& addr,
                  uint16_t port,
                  SOCKADDR_STORAGE* addrBuf,
                  socklen_t& addrSize)
{
    return MakeSockAddr(addr, port, 0, addrBuf, addrSize);
}

QStatus GetSockAddr(const SOCKADDR_STORAGE* addrBuf, socklen_t addrSize,
                    IPAddress& addr, uint16_t& port)
{
    QStatus status = ER_OK;
    char hostname[NI_MAXHOST];
    char servInfo[NI_MAXSERV];

    DWORD dwRetval = getnameinfo((struct sockaddr*)addrBuf,
                                 sizeof (SOCKADDR_STORAGE),
                                 hostname, NI_MAXHOST,
                                 servInfo, NI_MAXSERV,
                                 NI_NUMERICHOST | NI_NUMERICSERV);

    if (dwRetval != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("GetSockAddr: %s", StrError().c_str()));
    } else {
        /*
         * In the case of IPv6, the hostname will have the interface number
         * tacked on the end, as in "fe80::20c:29ff:fe7b:6f10%1".  We
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
    WinsockCheck();
    return WSAGetLastError();
}

qcc::String GetLastErrorString()
{
    return StrError();
}

QStatus Socket(AddressFamily addrFamily, SocketType type, SocketFd& sockfd)
{
    WinsockCheck();
    QStatus status = ER_OK;
    SOCKET ret;


    QCC_DbgTrace(("Socket(addrFamily = %d, type = %d, sockfd = <>)", addrFamily, type));

    if (addrFamily == QCC_AF_UNIX) {
        return ER_NOT_IMPLEMENTED;
    }
    ret = socket(static_cast<int>(addrFamily), static_cast<int>(type), 0);
    if (ret == INVALID_SOCKET) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Opening socket: %s", StrError().c_str()));
    } else {
        sockfd = static_cast<SocketFd>(ret);
    }
    return status;
}


QStatus Connect(SocketFd sockfd, const IPAddress& remoteAddr, uint16_t remotePort)
{
    QStatus status = ER_OK;
    int ret;
    SOCKADDR_STORAGE addr;
    socklen_t addrLen = sizeof(addr);

    QCC_DbgTrace(("Connect(sockfd = %d, remoteAddr = %s, remotePort = %hu)",
                  sockfd, remoteAddr.ToString().c_str(), remotePort));

    MakeSockAddr(remoteAddr, remotePort, &addr, addrLen);
    ret = connect(static_cast<SOCKET>(sockfd), reinterpret_cast<struct sockaddr*>(&addr), addrLen);
    if (ret == SOCKET_ERROR) {
        switch (WSAGetLastError()) {
        case WSAEWOULDBLOCK:
        case WSAEALREADY:
            status = ER_WOULDBLOCK;
            break;

        case WSAECONNREFUSED:
            status = ER_CONN_REFUSED;
            break;

        case WSAEISCONN:
            status = ER_OK;
            break;

        default:
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Connecting to %s %d: %s", remoteAddr.ToString().c_str(), remotePort, StrError().c_str()));
            break;
        }
    } else {
        u_long mode = 1; // Non-blocking
        ret = ioctlsocket(sockfd, FIONBIO, &mode);
        if (ret == SOCKET_ERROR) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Failed to set socket non-blocking ", StrError().c_str()));
        }
    }
    return status;
}


QStatus Connect(SocketFd sockfd, const char* pathName)
{
    return ER_NOT_IMPLEMENTED;
}



QStatus Bind(SocketFd sockfd, const IPAddress& localAddr, uint16_t localPort)
{
    QStatus status = ER_OK;
    int ret;
    SOCKADDR_STORAGE addr;
    socklen_t addrLen = sizeof(addr);

    QCC_DbgTrace(("Bind(sockfd = %d, localAddr = %s, localPort = %hu)",
                  sockfd, localAddr.ToString().c_str(), localPort));

    MakeSockAddr(localAddr, localPort, &addr, addrLen);
    ret = bind(static_cast<SOCKET>(sockfd), reinterpret_cast<struct sockaddr*>(&addr), addrLen);
    if (ret == SOCKET_ERROR) {
        status = (WSAGetLastError() == WSAEADDRNOTAVAIL) ? ER_SOCKET_BIND_ERROR : ER_OS_ERROR;
        QCC_DbgPrintf(("Binding to %s %d failed: %s", localAddr.ToString().c_str(), localPort, StrError().c_str()));
    }
    return status;
}


QStatus Bind(SocketFd sockfd, const char* pathName)
{
    return ER_NOT_IMPLEMENTED;
}


QStatus Listen(SocketFd sockfd, int backlog)
{
    QStatus status = ER_OK;
    int ret;

    QCC_DbgTrace(("Listen(sockfd = %d, backlog = %d)", sockfd, backlog));

    ret = listen(static_cast<SOCKET>(sockfd), backlog);
    if (ret == SOCKET_ERROR) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Listening: %s", StrError().c_str()));
    }
    return status;
}


QStatus Accept(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort, SocketFd& newSockfd)
{
    QStatus status = ER_OK;
    SOCKET ret;
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);

    QCC_DbgTrace(("Accept(sockfd = %d, remoteAddr = <>, remotePort = <>)", sockfd));


    ret = accept(static_cast<SOCKET>(sockfd), reinterpret_cast<struct sockaddr*>(&addr), &addrLen);
    if (ret == INVALID_SOCKET) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            status = ER_WOULDBLOCK;
        } else {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Listening: %s", StrError().c_str()));
        }
        newSockfd = qcc::INVALID_SOCKET_FD;
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
        newSockfd = ret;
        u_long mode = 1; // Non-blocking
        ret = ioctlsocket(newSockfd, FIONBIO, &mode);
        if (ret == SOCKET_ERROR) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Failed to set socket non-blocking %s", StrError().c_str()));
            closesocket(newSockfd);
            newSockfd = qcc::INVALID_SOCKET_FD;
        } else {
            QCC_DbgHLPrintf(("Accept(sockfd = %d) newSockfd = %d", sockfd, newSockfd));
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

    QCC_DbgHLPrintf(("Shutdown(sockfd = %d)", sockfd));

    ret = shutdown(static_cast<SOCKET>(sockfd), SD_BOTH);
    if (ret == SOCKET_ERROR) {
        status = ER_OS_ERROR;
    } else {
        /*
         * The winsock documentation recommends flushing data from the IP transport
         * by receiving until the recv returns 0 or fails.
         */
        int ret;
        do {
            char buf[64];
            ret = recv(static_cast<SOCKET>(sockfd), buf, sizeof(buf), 0);
        } while (ret > 0);
    }
    return status;
}


void Close(SocketFd sockfd)
{
    int ret;

    QCC_DbgTrace(("Close (sockfd = %d)", sockfd));
    ret = closesocket(static_cast<SOCKET>(sockfd));
    if (ret == SOCKET_ERROR) {
        QCC_LogError(ER_OS_ERROR, ("Close: (sockfd = %d) %s", sockfd, StrError().c_str()));
    }
}

QStatus SocketDup(SocketFd sockfd, SocketFd& dupSock)
{
    QStatus status = ER_OK;
    WSAPROTOCOL_INFO protocolInfo;

    int ret = WSADuplicateSocket(sockfd, qcc::GetPid(), &protocolInfo);
    if (ret == SOCKET_ERROR) {
        QCC_LogError(ER_OS_ERROR, ("SocketDup: %s", StrError().c_str()));
        status = ER_OS_ERROR;
    } else {
        dupSock = WSASocket(protocolInfo.iAddressFamily,
                            protocolInfo.iSocketType,
                            protocolInfo.iProtocol,
                            &protocolInfo,
                            0,
                            WSA_FLAG_OVERLAPPED);
        if (dupSock == INVALID_SOCKET) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("SocketDup WSASocket: %s", StrError().c_str()));
        }
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

    ret = getsockname(static_cast<SOCKET>(sockfd), reinterpret_cast<struct sockaddr*>(&addrBuf), &addrLen);

    if (ret == SOCKET_ERROR) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Geting Local Address: %s", StrError().c_str()));
    } else {
        QCC_DbgPrintf(("ret = %d  addrBuf.ss_family = %d  addrLen = %d", ret, addrBuf.ss_family, addrLen));
        if (addrBuf.ss_family == AF_INET) {
            struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(&addrBuf);
            uint8_t* portBuf = reinterpret_cast<uint8_t*>(&sa->sin_port);
            addr = IPAddress(reinterpret_cast<uint8_t*>(&sa->sin_addr.s_addr), IPAddress::IPv4_SIZE);
            port = (static_cast<uint16_t>(portBuf[0]) << 8) | static_cast<uint16_t>(portBuf[1]);
        } else if (addrBuf.ss_family == AF_INET6) {
            struct sockaddr_in6* sa = reinterpret_cast<struct sockaddr_in6*>(&addrBuf);
            uint8_t* portBuf = reinterpret_cast<uint8_t*>(&sa->sin6_port);
            addr = IPAddress(reinterpret_cast<uint8_t*>(&sa->sin6_addr.s6_addr), IPAddress::IPv6_SIZE);
            port = (static_cast<uint16_t>(portBuf[0]) << 8) | static_cast<uint16_t>(portBuf[1]);
        } else {
            port = 0;
        }
        QCC_DbgPrintf(("Local Address: %s - %u", addr.ToString().c_str(), port));
    }

    return status;
}


QStatus Send(SocketFd sockfd, const void* buf, size_t len, size_t& sent)
{
    QStatus status = ER_OK;
    size_t ret;

    QCC_DbgTrace(("ERSend(sockfd = %d, *buf = <>, len = %lu, sent = <>)", sockfd, len));
    assert(buf != NULL);

    QCC_DbgLocalData(buf, len);

    ret = send(static_cast<SOCKET>(sockfd), static_cast<const char*>(buf), len, 0);
    if (ret == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            sent = 0;
            status = ER_WOULDBLOCK;
        } else {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Send: %s", StrError().c_str()));
        }
    } else {
        sent = static_cast<size_t>(ret);
        QCC_DbgPrintf(("Sent %u bytes", sent));
    }
    return status;
}


QStatus SendTo(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort, uint32_t scopeId,
               const void* buf, size_t len, size_t& sent)
{
    QStatus status = ER_OK;
    SOCKADDR_STORAGE addr;
    socklen_t addrLen = sizeof(addr);
    size_t ret;

    QCC_DbgTrace(("SendTo(sockfd = %d, remoteAddr = %s, remotePort = %u, *buf = <>, len = %lu, sent = <>)",
                  sockfd, remoteAddr.ToString().c_str(), remotePort, len));
    assert(buf != NULL);

    QCC_DbgLocalData(buf, len);

    MakeSockAddr(remoteAddr, remotePort, scopeId, &addr, addrLen);
    ret = sendto(static_cast<SOCKET>(sockfd), static_cast<const char*>(buf), len, 0,
                 reinterpret_cast<struct sockaddr*>(&addr), addrLen);
    if (ret == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            sent = 0;
            status = ER_WOULDBLOCK;
        } else {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Send: %s", StrError().c_str()));
        }
    } else {
        sent = static_cast<size_t>(ret);
        QCC_DbgPrintf(("Sent %u bytes", sent));
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
    size_t ret;

    QCC_DbgTrace(("Recv(sockfd = %d, buf = <>, len = %lu, received = <>)", sockfd, len));
    assert(buf != NULL);

    ret = recv(static_cast<SOCKET>(sockfd), static_cast<char*>(buf), len, 0);
    if (ret == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            status = ER_WOULDBLOCK;
        } else {
            status = ER_OS_ERROR;
        }
        received = 0;
    } else {
        received = static_cast<size_t>(ret);
        QCC_DbgPrintf(("Received %u bytes", received));
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

    WSABUF iov[] = { { len, reinterpret_cast<char*>(buf) } };

    char cbuf[1024];

    WSAMSG msg;
    memset(&msg, 0, sizeof(msg));

    msg.lpBuffers = iov;
    msg.dwBufferCount = ArraySize(iov);
    msg.Control.buf  = cbuf;
    msg.Control.len = (sizeof(cbuf) / sizeof(cbuf[0]));

    SOCKADDR_STORAGE src;
    SOCKADDR_STORAGE dst;
    memset(&src, 0, sizeof(SOCKADDR_STORAGE));
    memset(&dst, 0, sizeof(SOCKADDR_STORAGE));

    IPAddress addr;
    uint16_t port;
    status = GetLocalAddress(sockfd, addr, port);

    if (status == ER_OK && addr.GetAddressFamily() == QCC_AF_INET) {
        reinterpret_cast<LPSOCKADDR_IN>(&src)->sin_port = port;
        reinterpret_cast<LPSOCKADDR_IN>(&src)->sin_family = AF_INET;
        msg.name = reinterpret_cast<LPSOCKADDR>(&src);
        msg.namelen = sizeof(SOCKADDR_IN);
    } else if (status == ER_OK && addr.GetAddressFamily()  == QCC_AF_INET6) {
        reinterpret_cast<LPSOCKADDR_IN6>(&src)->sin6_port = port;
        reinterpret_cast<LPSOCKADDR_IN6>(&src)->sin6_family = AF_INET6;
        msg.name = reinterpret_cast<LPSOCKADDR>(&src);
        msg.namelen = sizeof(SOCKADDR_IN6);
    } else {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("RecvWithAncillaryData (sockfd = %u): unknown address family", sockfd));
        return status;
    }

    static LPFN_WSARECVMSG WSARecvMsg = NULL;
    DWORD recv;
    DWORD ret;
    if (!WSARecvMsg) {
        GUID guid = WSAID_WSARECVMSG;
        ret = WSAIoctl(static_cast<SOCKET>(sockfd), SIO_GET_EXTENSION_FUNCTION_POINTER,
                       &guid, sizeof(guid),
                       &WSARecvMsg, sizeof(WSARecvMsg),
                       &recv, NULL, NULL);
        if (ret == SOCKET_ERROR) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("RecvWithAncillaryData (sockfd = %u): %s", sockfd, StrError().c_str()));
            return status;
        }
    }

    assert(buf != NULL);

    ret = WSARecvMsg(static_cast<SOCKET>(sockfd), &msg, &recv, NULL, NULL);

    if (ret == SOCKET_ERROR) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("RecvWithAncillaryData (sockfd = %u): %s", sockfd, errno, StrError().c_str()));
        return status;
    }
    received = recv;

    LPWSACMSGHDR cmsg;
    for (cmsg = WSA_CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = WSA_CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
            struct in_pktinfo* i = reinterpret_cast<struct in_pktinfo*>(WSA_CMSG_DATA(cmsg));
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
        if (cmsg->cmsg_level == IPPROTO_IPV6  &&  cmsg->cmsg_type == IPV6_PKTINFO) {
            struct in6_pktinfo* i = reinterpret_cast<struct in6_pktinfo*>(WSA_CMSG_DATA(cmsg));
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

QStatus RecvFrom(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort,
                 void* buf, size_t len, size_t& received)
{
    QStatus status = ER_OK;
    SOCKADDR_STORAGE fromAddr;
    socklen_t addrLen = sizeof(fromAddr);
    size_t ret;
    received = 0;

    QCC_DbgTrace(("RecvFrom(sockfd = %d, buf = <>, len = %lu, received = <>)", sockfd, len));
    assert(buf != NULL);

    ret = recvfrom(static_cast<int>(sockfd), static_cast<char*>(buf), len, 0,
                   reinterpret_cast<sockaddr*>(&fromAddr), &addrLen);
    if (ret == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            status = ER_WOULDBLOCK;
        } else {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Receive: %d", StrError().c_str()));
        }
        received = 0;
    } else {
        received = static_cast<size_t>(ret);
        status = GetSockAddr(&fromAddr, addrLen, remoteAddr, remotePort);
        QCC_DbgPrintf(("Received %u bytes, remoteAddr = %s, remotePort = %u",
                       received, remoteAddr.ToString().c_str(), remotePort));
    }

    QCC_DbgRemoteData(buf, received);

    return status;
}


int InetPtoN(int af, const char* src, void* dst)
{
    WinsockCheck();
    int err = -1;
    if (af == AF_INET6) {
        struct sockaddr_in6 sin6;
        int sin6Len = sizeof(sin6);
        memset(&sin6, 0, sin6Len);
        sin6.sin6_family = AF_INET6;
        err = WSAStringToAddressA((LPSTR)src, AF_INET6, NULL, (struct sockaddr*)&sin6, &sin6Len);
        if (!err) {
            memcpy(dst, &sin6.sin6_addr, sizeof(sin6.sin6_addr));
        }
    } else if (af == AF_INET) {
        struct sockaddr_in sin;
        int sinLen = sizeof(sin);
        memset(&sin, 0, sinLen);
        sin.sin_family = AF_INET;
        err = WSAStringToAddressA((LPSTR)src, AF_INET, NULL, (struct sockaddr*)&sin, &sinLen);
        if (!err) {
            memcpy(dst, &sin.sin_addr, sizeof(sin.sin_addr));
        }
    }
    return err ? -1 : 1;
}

const char* InetNtoP(int af, const void* src, char* dst, socklen_t size)
{
    WinsockCheck();
    int err = -1;
    DWORD sz = (DWORD)size;
    if (af == AF_INET6) {
        struct sockaddr_in6 sin6;
        memset(&sin6, 0, sizeof(sin6));
        sin6.sin6_family = AF_INET6;
        sin6.sin6_flowinfo = 0;
        memcpy(&sin6.sin6_addr, src, sizeof(sin6.sin6_addr));
        err = WSAAddressToStringA((struct sockaddr*)&sin6, sizeof(sin6), NULL, dst, &sz);
    } else if (af == AF_INET) {
        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        memcpy(&sin.sin_addr, src, sizeof(sin.sin_addr));
        err = WSAAddressToStringA((struct sockaddr*)&sin, sizeof(sin), NULL, dst, &sz);
    }
    return err ? NULL : dst;
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

    /*
     * Check if the next read will return OOB data
     */
    u_long marked = 0;
    int ret = ioctlsocket(sockfd, SIOCATMARK, &marked);
    if (ret == SOCKET_ERROR) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("RecvWithFds ioctlsocket: %s", StrError().c_str()));
    }
    if ((status == ER_OK) && !marked) {
        char fdCount;
        ret = recv(sockfd, &fdCount, 1, MSG_OOB);
        if (ret == SOCKET_ERROR) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("RecvWithFds recv (MSG_OOB): %s", StrError().c_str()));
        } else {
            recvdFds = fdCount;
            QCC_DbgHLPrintf(("RecvWithFds OOB %d handles", recvdFds));
            /*
             * Check we have enough room to return the file descriptors.
             */
            if (recvdFds > recvdFds) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("Too many handles: %d implementation limit is %d", recvdFds, maxFds));
            }
        }
        /*
         * The actual file descriptors are all inband and must be read atomically.
         */
        for (size_t i = 0; (i < recvdFds) && (status == ER_OK); ++i) {
            WSAPROTOCOL_INFO protocolInfo;
            uint8_t* buf = reinterpret_cast<uint8_t*>(&protocolInfo);
            size_t sz = sizeof(protocolInfo);
            uint32_t maxSleeps = 100;
            /*
             * The poll/sleep loop is a little cheesy but file descriptors are small and
             * rare so this is highly unlikely to have any impact on performance.
             */
            while (sz && (status == ER_OK)) {
                size_t recvd;
                status = Recv(sockfd, buf, sz, recvd);
                if (status == ER_WOULDBLOCK) {
                    if (--maxSleeps) {
                        qcc::Sleep(1);
                        status = ER_OK;
                        continue;
                    }
                    status = ER_TIMEOUT;
                }
                buf += recvd;
                sz -= recvd;
            }
            if (status == ER_OK) {
                SocketFd fd = WSASocket(protocolInfo.iAddressFamily,
                                        protocolInfo.iSocketType,
                                        protocolInfo.iProtocol,
                                        &protocolInfo,
                                        0,
                                        WSA_FLAG_OVERLAPPED);
                if (fd == INVALID_SOCKET) {
                    status = ER_OS_ERROR;
                    QCC_LogError(status, ("RecvWithFds WSASocket: %s", StrError().c_str()));
                } else {
                    QCC_DbgHLPrintf(("RecvWithFds got handle %u", fd));
                    *fdList++ = fd;
                }
            }
        }
    }
    if (status == ER_OK) {
        status = Recv(sockfd, buf, len, received);
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

    /*
     * We send the file descriptor count as OOB data.
     */
    char oob = static_cast<char>(numFds);
    int ret = send(sockfd, &oob, 1, MSG_OOB);
    if (ret == SOCKET_ERROR) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("RecvWithFds recv (MSG_OOB): %s", StrError().c_str()));
    } else {
        QCC_DbgHLPrintf(("SendWithFds OOB %d handles", oob));
    }
    while (numFds-- && (status == ER_OK)) {
        WSAPROTOCOL_INFO protocolInfo;
        ret = WSADuplicateSocket(*fdList++, pid, &protocolInfo);
        if (ret) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("SendFd WSADuplicateSocket: %s", StrError().c_str()));
        } else {
            uint8_t* buf = reinterpret_cast<uint8_t*>(&protocolInfo);
            size_t sz = sizeof(protocolInfo);
            uint32_t maxSleeps = 100;
            /*
             * The poll/sleep loop is a little cheesy but file descriptors are small and
             * rare so this is highly unlikely to have any impact on performance.
             */
            while (sz && (status == ER_OK)) {
                status = Send(sockfd, buf, sz, sent);
                if (status == ER_WOULDBLOCK) {
                    if (--maxSleeps) {
                        qcc::Sleep(1);
                        status = ER_OK;
                        continue;
                    }
                    status = ER_TIMEOUT;
                }
                buf += sent;
                sz -= sent;
            }
        }
    }
    if (status == ER_OK) {
        status = Send(sockfd, buf, len, sent);
    }
    return status;
}

QStatus SocketPair(SocketFd(&sockets)[2])
{
    QStatus status = ER_OK;
    IPAddress ipAddr("127.0.0.1");
    IPAddress remAddr;
    uint16_t remPort;

    QCC_DbgTrace(("SocketPair()"));

    /* Create sockets */
    status = Socket(QCC_AF_INET, QCC_SOCK_STREAM, sockets[0]);
    if (status != ER_OK) {
        return status;
    }

    status = Socket(QCC_AF_INET, QCC_SOCK_STREAM, sockets[1]);
    if (status != ER_OK) {
        Close(sockets[0]);
        return status;
    }

    /* Bind fd[0] */
    status = Bind(sockets[0], ipAddr, 0);
    if (status != ER_OK) {
        goto socketPairCleanup;
    }

    /* Listen fds[0] */
    status = Listen(sockets[0], 1);
    if (status != ER_OK) {
        goto socketPairCleanup;
    }

    /* Get addr info for fds[0] */
    struct sockaddr_in addrInfo;
    int len = sizeof(addrInfo);
    int ret = getsockname(sockets[0], reinterpret_cast<sockaddr*>(&addrInfo), &len);
    if (ret == SOCKET_ERROR) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("getsockopt failed: %s", StrError().c_str()));
        goto socketPairCleanup;
    }

    /* Connect fds[1] */
    status = Connect(sockets[1], ipAddr, ntohs(addrInfo.sin_port));
    if (status != ER_OK) {
        QCC_LogError(status, ("SocketPair.Connect failed"));
        goto socketPairCleanup;
    }

    /* Accept fds[0] */
    status = Accept(sockets[0], remAddr, remPort, sockets[0]);
    if (status != ER_OK) {
        QCC_LogError(status, ("SocketPair.Accept failed"));
        goto socketPairCleanup;
    }

    /* Make sockets blocking */
    status = SetBlocking(sockets[0], true);
    if (status != ER_OK) {
        QCC_LogError(status, ("SetBlocking fd[0] failed"));
        goto socketPairCleanup;
    }
    status = SetBlocking(sockets[1], true);
    if (status != ER_OK) {
        QCC_LogError(status, ("SetBlocking fd[1] failed"));
        goto socketPairCleanup;
    }

socketPairCleanup:

    /* Higher level code is responsible for cleaning up sockets */

    return status;
}

QStatus SetBlocking(SocketFd sockfd, bool blocking)
{
    QStatus status = ER_OK;

    u_long mode = blocking ? 0 : 1;
    int ret = ioctlsocket(sockfd, FIONBIO, &mode);
    if (ret == SOCKET_ERROR) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Failed to set socket non-blocking %s", StrError().c_str()));
    }
    return status;
}

QStatus SetSndBuf(SocketFd sockfd, size_t bufSize)
{
    QStatus status = ER_OK;
    int arg = bufSize;
    int r = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&arg, sizeof(arg));
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
    int r = getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&arg, &len);
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
    int r = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&arg, sizeof(arg));
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
    int r = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&arg, &len);
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

    int r = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof(l));
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
    int r = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&arg, sizeof(int));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting TCP_NODELAY failed: (%d) %s", GetLastError(), GetLastErrorString().c_str()));
    }
    return status;
}

QStatus SetReuseAddress(SocketFd sockfd, bool reuse)
{
    QStatus status = ER_OK;
    /*
     * On Windows SO_REUSEADDR allows an application to bind an steal a port that is already in use.
     * This is different than the posix behavior and definitely not the expected behavior. Setting
     * SO_EXCLUSIVEADDRUSE prevents other applications from stealing the port from underneath us.
     */
    if (status == ER_OK) {
        int arg = reuse ? 1 : -0;
        int r = setsockopt(sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&arg, sizeof(arg));
        if (r != 0) {
            QCC_LogError(ER_OS_ERROR, ("Setting SO_EXCLUSIVEADDRUSE failed: (%d) %s", GetLastError(), GetLastErrorString().c_str()));
        }
    }
    return status;
}

QStatus SetReusePort(SocketFd sockfd, bool reuse)
{
    QStatus status = ER_OK;
    int arg = reuse ? 1 : -0;
    int r = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&arg, sizeof(arg));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting SO_REUSEADDR failed: (%d) %s", GetLastError(), GetLastErrorString().c_str()));
    }
    return status;
}

void IfConfigByFamily(uint32_t family, std::vector<IfConfigEntry>& entries);

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
    assert(sockFd != INVALID_SOCKET);
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
         * interface address.  There is no convenient socket ioctl in Windows to
         * do what we need (SIOCGIFADDR) so we call into IfConfig since it
         * already does the surprising amount of dirty work required to get this
         * done across the various incarnations of Windows.
         */
        std::vector<IfConfigEntry> entries;
        IfConfigByFamily(AF_INET, entries);

        bool found = false;
        struct ip_mreq mreq;

        for (uint32_t i = 0; i < entries.size(); ++i) {
            if (entries[i].m_name == iface) {
                IPAddress address(entries[i].m_addr);
                mreq.imr_interface.s_addr = address.GetIPv4AddressNetOrder();
                found = true;
            }
        }

        if (!found) {
            QCC_LogError(ER_OS_ERROR, ("can't find address for interface %s", iface.c_str()));
            return ER_OS_ERROR;
        }

        int rc = InetPtoN(AF_INET, multicastGroup.c_str(), &mreq.imr_multiaddr);
        if (rc != 1) {
            QCC_LogError(ER_OS_ERROR, ("InetPtoN() failed: %d - %s", GetLastError(), GetLastErrorString().c_str()));
            return ER_OS_ERROR;
        }

        int opt = op == JOIN ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
        rc = setsockopt(sockFd, IPPROTO_IP, opt, reinterpret_cast<const char*>(&mreq), sizeof(mreq));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(%s) failed: %d - %s", op == JOIN ? "IP_ADD_MEMBERSHIP" : "IP_DROP_MEMBERSHIP", GetLastError(), GetLastErrorString().c_str()));
            return ER_OS_ERROR;
        }
    } else if (family == QCC_AF_INET6) {
        /*
         * Group memberships are associated with both the multicast group itself
         * and also an interface.  In the IPv6 version, we need to provide an
         * interface index instead of an IP address associated with the
         * interface.  There is no convenient call in Windows to do what we need
         * (cf. if_nametoindex) so we call into IfConfig since it already does
         * the surprising amount of dirty work required to get this done across
         * the various incarnations of Windows.
         */
        std::vector<IfConfigEntry> entries;
        IfConfigByFamily(AF_INET6, entries);

        bool found = false;
        struct ipv6_mreq mreq;

        for (uint32_t i = 0; i < entries.size(); ++i) {
            if (entries[i].m_name == iface) {
                mreq.ipv6mr_interface = entries[i].m_index;
                found = true;
            }
        }

        if (!found) {
            QCC_LogError(ER_OS_ERROR, ("can't find interface index for interface %s", iface.c_str()));
            return ER_OS_ERROR;
        }

        int rc = InetPtoN(AF_INET6, multicastGroup.c_str(), &mreq.ipv6mr_multiaddr);
        if (rc != 1) {
            QCC_LogError(ER_OS_ERROR, ("InetPtoN() failed: %d - %s", GetLastError(), GetLastErrorString().c_str()));
            return ER_OS_ERROR;
        }

        int opt = op == JOIN ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP;
        rc = setsockopt(sockFd, IPPROTO_IPV6, opt, reinterpret_cast<const char*>(&mreq), sizeof(mreq));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(IP_ADD_MEMBERSHIP) failed: %d - %s", GetLastError(), GetLastErrorString().c_str()));
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
    assert(sockFd != INVALID_SOCKET);
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
         * address from the interface name.  There is no convenient socket ioctl
         * in Windows to do what we need (SIOCGIFADDR) so we call into IfConfig
         * since it already does the surprising amount of dirty work required to
         * get this done across the various incarnations of Windows.
         */
        std::vector<IfConfigEntry> entries;
        IfConfigByFamily(AF_INET, entries);

        bool found = false;
        struct in_addr addr;

        for (uint32_t i = 0; i < entries.size(); ++i) {
            if (entries[i].m_name == iface) {
                IPAddress address(entries[i].m_addr);
                addr.s_addr = address.GetIPv4AddressNetOrder();
                found = true;
            }
        }

        if (!found) {
            QCC_LogError(ER_OS_ERROR, ("can't find address for interface %s", iface.c_str()));
            return ER_OS_ERROR;
        }

        int rc = setsockopt(sockFd, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast<const char*>(&addr), sizeof(addr));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(IP_MULTICAST_IF) failed: %d - %s", GetLastError(), GetLastErrorString().c_str()));
            return ER_OS_ERROR;
        }
    } else if (family == QCC_AF_INET6) {
        /*
         * In the IPv6 version, we need to provide an interface index instead of
         * an IP address associated with the interface.  There is no convenient
         * call in Windows to do what we need (cf. if_nametoindex) so we call
         * into IfConfig since it already does the surprising amount of dirty
         * work required to get this done across the various incarnations of
         * Windows.
         */
        std::vector<IfConfigEntry> entries;
        IfConfigByFamily(AF_INET6, entries);

        bool found = false;
        uint32_t index = 0;

        for (uint32_t i = 0; i < entries.size(); ++i) {
            if (entries[i].m_name == iface) {
                index = entries[i].m_index;
                found = true;
            }
        }

        if (!found) {
            QCC_LogError(ER_OS_ERROR, ("can't find interface index for interface %s", iface.c_str()));
            return ER_OS_ERROR;
        }

        int rc = setsockopt(sockFd, IPPROTO_IPV6, IP_MULTICAST_IF, reinterpret_cast<const char*>(&index), sizeof(index));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(IP_MULTICAST_IF) failed: %d - %s", GetLastError(), GetLastErrorString().c_str()));
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
    assert(sockFd != INVALID_SOCKET);
    assert(family == AF_INET || family == AF_INET6);

    /*
     * IPv4 and IPv6 are almost the same.  Of course, not quite, though.
     */
    if (family == QCC_AF_INET) {
        int rc = setsockopt(sockFd, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char*>(&hops), sizeof(hops));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(IP_MULTICAST_TTL) failed: %d - %s", GetLastError(), GetLastErrorString().c_str()));
            return ER_OS_ERROR;
        }
    } else if (family == QCC_AF_INET6) {
        int rc = setsockopt(sockFd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, reinterpret_cast<const char*>(&hops), sizeof(hops));
        if (rc == -1) {
            QCC_LogError(ER_OS_ERROR, ("setsockopt(IP_MULTICAST_HOPS) failed: %d - %s", GetLastError(), GetLastErrorString().c_str()));
            return ER_OS_ERROR;
        }
    }
    return ER_OK;
}

QStatus SetBroadcast(SocketFd sockfd, bool broadcast)
{
    QStatus status = ER_OK;
    int arg = broadcast ? 1 : -0;
    int r = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*)&arg, sizeof(arg));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting SO_BROADCAST failed: (%d) %s", GetLastError(), GetLastErrorString().c_str()));
    }
    return status;
}

QStatus SetRecvPktAncillaryData(SocketFd sockfd, AddressFamily addrFamily, bool recv)
{
    /*
     * We assume that No external API will be trying to call here and so asserts
     * are appropriate when checking for completely bogus parameters.
     */
    assert(sockfd != INVALID_SOCKET);
    assert(addrFamily == AF_INET || addrFamily == AF_INET6);

    QStatus status = ER_OK;
    int arg = recv ? 1 : -0;
    if (addrFamily == QCC_AF_INET) {
        int r = setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, (char*)&arg, sizeof(arg));
        if (r != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Setting IP_PKTINFO failed: (%d) %s", errno, strerror(errno)));
        }
    } else if (addrFamily == QCC_AF_INET6) {
        int r = setsockopt(sockfd, IPPROTO_IPV6, IPV6_PKTINFO, (char*)&arg, sizeof(arg));
        if (r != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Setting IPV6_PKTINFO failed: (%d) %s", errno, strerror(errno)));
        }
    }
    return status;
}

QStatus SetRecvIPv6Only(SocketFd sockfd, bool recv)
{
    QStatus status = ER_OK;
    int arg = recv ? 1 : -0;
    int r = setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&arg, sizeof(arg));
    if (r != 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Setting IPV6_V6ONLY failed: (%d) %s", errno, strerror(errno)));
    }
    return status;
}

}   /* namespace */
