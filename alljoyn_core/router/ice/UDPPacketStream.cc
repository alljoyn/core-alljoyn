/**
 * @file
 * UDPPacketStream is a UDP based implementation of the PacketStream interface.
 */

/******************************************************************************
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
#include <qcc/Util.h>
#include <errno.h>
#include <assert.h>

#include <qcc/Event.h>
#include <qcc/Debug.h>
#include <qcc/StringUtil.h>
#include "UDPPacketStream.h"
#include "NetworkInterface.h"

#define QCC_MODULE "PACKET"

using namespace std;
using namespace qcc;

namespace ajn {

UDPPacketStream::UDPPacketStream(const char* ifaceName, uint16_t port) :
    ipAddr(),
    port(port),
    mtu(0),
    sock(-1),
    sourceEvent(&Event::neverSet),
    sinkEvent(&Event::alwaysSet)
{
    QCC_DbgPrintf(("UDPPacketStream::UDPPacketStream(ifaceName='ifaceName', port=%u)", ifaceName, port));

    NetworkInterface nwInterfaces(true);
    QStatus status = nwInterfaces.UpdateNetworkInterfaces();
    if (status == ER_OK) {
        std::vector<qcc::IfConfigEntry>::iterator it = nwInterfaces.liveInterfaces.begin();
        for (; it != nwInterfaces.liveInterfaces.end(); ++it) {
            if (it->m_name == ifaceName) {
                mtu = it->m_mtu;
                ipAddr = qcc::IPAddress(it->m_addr);
            }
        }
    }
}

UDPPacketStream::UDPPacketStream(const qcc::IPAddress& addr, uint16_t port) :
    ipAddr(addr),
    port(port),
    mtu(1472),
    sock(-1),
    sourceEvent(&Event::neverSet),
    sinkEvent(&Event::alwaysSet)
{
    QCC_DbgPrintf(("UDPPacketStream::UDPPacketStream(addr='%s', port=%u)", ipAddr.ToString().c_str(), port));

    NetworkInterface nwInterfaces(true);
    QStatus status = nwInterfaces.UpdateNetworkInterfaces();
    if (status == ER_OK) {
        std::vector<qcc::IfConfigEntry>::iterator it = nwInterfaces.liveInterfaces.begin();
        for (; it != nwInterfaces.liveInterfaces.end(); ++it) {
            if (qcc::IPAddress(it->m_addr) == addr) {
                mtu = it->m_mtu;
            }
        }
    }
}

UDPPacketStream::UDPPacketStream(const qcc::IPAddress& addr, uint16_t port, size_t mtu) :
    ipAddr(addr),
    port(port),
    mtu(mtu),
    sock(-1),
    sourceEvent(&Event::neverSet),
    sinkEvent(&Event::alwaysSet)
{
    QCC_DbgPrintf(("UDPPacketStream::UDPPacketStream(addr='%s', port=%u, mtu=%lu)", ipAddr.ToString().c_str(), port, mtu));
}

UDPPacketStream::~UDPPacketStream()
{
    if (sourceEvent != &Event::neverSet) {
        delete sourceEvent;
        sourceEvent = &Event::neverSet;
    }
    if (sinkEvent != &Event::alwaysSet) {
        delete sinkEvent;
        sinkEvent = &Event::alwaysSet;
    }
    if (sock >= 0) {
        Close(sock);
        sock = -1;
    }
}

QStatus UDPPacketStream::Start()
{
    QCC_DbgPrintf(("UDPPacketStream::Start(addr=%s, port=%u)", ipAddr.ToString().c_str(), port));
    /* Create a socket */
    QStatus status = qcc::Socket(ipAddr.GetAddressFamily(), QCC_SOCK_DGRAM, sock);

    /* Bind socket */
    if (status == ER_OK) {
        status = Bind(sock, ipAddr, port);
        if (status == ER_OK) {
            if (port == 0) {
                status = GetLocalAddress(sock, ipAddr, port);
                if (status != ER_OK) {
                    QCC_DbgPrintf(("UDPPacketStream::Start Bind: GetLocalAddress failed"));
                }
            }

            if (status == ER_OK) {
                sourceEvent = new qcc::Event(sock, qcc::Event::IO_READ, false);
                sinkEvent = new qcc::Event(sock, qcc::Event::IO_WRITE, false);
            }
        } else {
            QCC_LogError(status, ("UDPPacketStream bind failed"));
        }
    } else {
        QCC_LogError(status, ("UDPPacketStream socket() failed"));
    }

    if (status != ER_OK) {
        Close(sock);
        sock = -1;
    }
    return status;
}

String UDPPacketStream::GetIPAddr() const
{
    // return inet_ntoa(((struct sockaddr_in*)&sa)->sin_addr);
    return ipAddr.ToString();
}

QStatus UDPPacketStream::Stop()
{
    QCC_DbgPrintf(("UDPPacketStream::Stop()"));
    return ER_OK;
}

QStatus UDPPacketStream::PushPacketBytes(const void* buf, size_t numBytes, PacketDest& dest)
{

    assert(numBytes <= mtu);
    size_t sendBytes = numBytes;
    size_t sent = 0;
    IPAddress ipAddr(dest.ip, dest.addrSize);
    QStatus status = qcc::SendTo(sock, ipAddr, dest.port, buf, sendBytes, sent);
    status = (sent == numBytes) ? ER_OK : ER_OS_ERROR;
    if (status != ER_OK) {
        if (sent == (size_t) -1) {
            QCC_LogError(status, ("sendto failed: %s (%d)", ::strerror(errno), errno));
        } else {
            QCC_LogError(status, ("Short udp send: exp=%d, act=%d", numBytes, sent));
        }
    }
    return status;
}

QStatus UDPPacketStream::PullPacketBytes(void* buf, size_t reqBytes, size_t& actualBytes,
                                         PacketDest& sender, uint32_t timeout)
{
    QStatus status = ER_OK;
    assert(reqBytes >= mtu);
    size_t recvBytes = reqBytes;
    IPAddress tmpIpAddr;
    uint16_t tmpPort = 0;
    status =  qcc::RecvFrom(sock, tmpIpAddr, tmpPort, buf, recvBytes, actualBytes);
    if (ER_OK != status) {
        QCC_LogError(status, ("recvfrom failed: %s", ::strerror(errno)));
    } else {
        tmpIpAddr.RenderIPBinary(sender.ip, IPAddress::IPv6_SIZE);
        sender.addrSize = tmpIpAddr.Size();
        sender.port = tmpPort;
    }
    return status;
}

String UDPPacketStream::ToString(const PacketDest& dest) const
{
    IPAddress ipAddr(dest.ip, dest.addrSize);
    String ret = ipAddr.ToString();
    ret += " (";
    ret += U32ToString(dest.port);
    ret += ")";
    return ret;
}

}
