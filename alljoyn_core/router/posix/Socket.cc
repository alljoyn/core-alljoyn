/**
 * @file
 *
 * Define the abstracted socket interface for Linux
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
#include "ScatterGatherList.h"
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/Status.h>

#define QCC_MODULE "NETWORK"

#if defined(QCC_OS_DARWIN)
#define MSG_NOSIGNAL 0
#endif

namespace qcc {

extern QStatus MakeSockAddr(const char* path, struct sockaddr_storage* addrBuf, socklen_t& addrSize);
extern QStatus MakeSockAddr(const IPAddress& addr, uint16_t port, struct sockaddr_storage* addrBuf, socklen_t& addrSize);
extern QStatus GetSockAddr(const sockaddr_storage* addrBuf, socklen_t addrSize, IPAddress& addr, uint16_t& port);

static QStatus SendSGCommon(SocketFd sockfd, struct sockaddr_storage* addr, socklen_t addrLen,
                            const ScatterGatherList& sg, size_t& sent)
{
    QStatus status = ER_OK;
    ssize_t ret;
    struct msghdr msg;
    size_t index;
    struct iovec* iov;
    ScatterGatherList::const_iterator iter;
    QCC_DbgTrace(("SendSGCommon(sockfd = %d, *addr, addrLen, sg[%u:%u/%u], sent = <>)",
                  sockfd, sg.Size(), sg.DataSize(), sg.MaxDataSize()));

    iov = new struct iovec[sg.Size()];
    for (index = 0, iter = sg.Begin(); iter != sg.End(); ++index, ++iter) {
        iov[index].iov_base = iter->buf;
        iov[index].iov_len = iter->len;
        QCC_DbgLocalData(iov[index].iov_base, iov[index].iov_len);
    }

    msg.msg_name = addr;
    msg.msg_namelen = addrLen;
    msg.msg_iov = iov;
    msg.msg_iovlen = sg.Size();
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    ret = sendmsg(static_cast<int>(sockfd), &msg, MSG_NOSIGNAL);
    if (ret == -1) {
        if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
            status = ER_WOULDBLOCK;
        } else {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("SendSGCommon (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
        }
    } else {
        sent = static_cast<size_t>(ret);
    }
    delete[] iov;
    return status;
}

QStatus SendSG(SocketFd sockfd, const ScatterGatherList& sg, size_t& sent)
{
    QCC_DbgTrace(("SendSG(sockfd = %d, sg[%u:%u/%u], sent = <>)",
                  sockfd, sg.Size(), sg.DataSize(), sg.MaxDataSize()));

    return SendSGCommon(sockfd, NULL, 0, sg, sent);
}

QStatus SendToSG(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort,
                 const ScatterGatherList& sg, size_t& sent)
{
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);

    QCC_DbgTrace(("SendToSG(sockfd = %d, remoteAddr = %s, remotePort = %u, sg[%u:%u/%u], sent = <>)",
                  sockfd, remoteAddr.ToString().c_str(), remotePort,
                  sg.Size(), sg.DataSize(), sg.MaxDataSize()));

    QStatus status = MakeSockAddr(remoteAddr, remotePort, &addr, addrLen);
    if (status != ER_OK) {
        return status;
    }

    return SendSGCommon(sockfd, &addr, addrLen, sg, sent);
}

static QStatus RecvSGCommon(SocketFd sockfd, struct sockaddr_storage* addr, socklen_t* addrLen,
                            ScatterGatherList& sg, size_t& received)
{
    QStatus status = ER_OK;
    ssize_t ret;
    struct msghdr msg;
    size_t index;
    struct iovec* iov;
    ScatterGatherList::const_iterator iter;
    QCC_DbgTrace(("RecvSGCommon(sockfd = &d, addr, addrLen, sg = <>, received = <>)",
                  sockfd));

    iov = new struct iovec[sg.Size()];
    for (index = 0, iter = sg.Begin(); iter != sg.End(); ++index, ++iter) {
        iov[index].iov_base = iter->buf;
        iov[index].iov_len = iter->len;
    }

    msg.msg_name = addr;
    msg.msg_namelen = *addrLen;
    msg.msg_iov = iov;
    msg.msg_iovlen = sg.Size();
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    ret = recvmsg(static_cast<int>(sockfd), &msg, 0);
    if (ret == -1) {
        status = ER_OS_ERROR;
        QCC_DbgHLPrintf(("RecvSGCommon (sockfd = %u): %d - %s", sockfd, errno, strerror(errno)));
    } else {
        received = static_cast<size_t>(ret);
        sg.SetDataSize(static_cast<size_t>(ret));
        *addrLen = msg.msg_namelen;
    }
    delete[] iov;

#if !defined(NDEBUG)
    if (1) {
        size_t rxcnt = received;
        QCC_DbgPrintf(("Received %u bytes", received));
        for (iter = sg.Begin(); rxcnt > 0 && iter != sg.End(); ++iter) {
            QCC_DbgRemoteData(iter->buf, std::min(rxcnt, iter->len));
            rxcnt -= std::min(rxcnt, iter->len);
        }
    }
#endif

    return status;
}


QStatus RecvSG(SocketFd sockfd, ScatterGatherList& sg, size_t& received)
{
    socklen_t addrLen = 0;
    QCC_DbgTrace(("RecvSG(sockfd = %d, sg = <>, received = <>)", sockfd));

    return RecvSGCommon(sockfd, NULL, &addrLen, sg, received);
}


QStatus RecvFromSG(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort,
                   ScatterGatherList& sg, size_t& received)
{
    QStatus status;
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);

    status = RecvSGCommon(sockfd, &addr, &addrLen, sg, received);
    if (ER_OK == status) {
        GetSockAddr(&addr, addrLen, remoteAddr, remotePort);
        QCC_DbgTrace(("RecvFromSG(sockfd = %d, remoteAddr = %s, remotePort = %u, sg = <>, sent = <>)",
                      sockfd, remoteAddr.ToString().c_str(), remotePort));
    }
    return status;
}
} // namespace qcc

