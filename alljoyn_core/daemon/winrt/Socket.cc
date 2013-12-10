/**
 * @file
 *
 * Define the abstracted socket interface for WinRT
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#include <qcc/IPAddress.h>
#include <qcc/Socket.h>
#include <qcc/IfConfig.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>
#include <qcc/StringUtil.h>
#include <qcc/windows/utility.h>
#include "ScatterGatherList.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "NETWORK"

namespace qcc {

QStatus SendSG(SocketFd sockfd, const ScatterGatherList& sg, size_t& sent)
{
    QStatus status;
    uint8_t* tmpBuf = new uint8_t[sg.MaxDataSize()];
    sg.CopyToBuffer(tmpBuf, sg.MaxDataSize());
    status = Send(sockfd, tmpBuf, sg.DataSize(), sent);
    delete[] tmpBuf;
    return status;
}

QStatus SendToSG(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort,
                 const ScatterGatherList& sg, size_t& sent)
{
    QStatus status;
    uint8_t* tmpBuf = new uint8_t[sg.MaxDataSize()];
    sg.CopyToBuffer(tmpBuf, sg.MaxDataSize());
    status = SendTo(sockfd, remoteAddr, remotePort, tmpBuf, sg.DataSize(), sent);
    delete[] tmpBuf;
    return status;
}

QStatus RecvSG(SocketFd sockfd, ScatterGatherList& sg, size_t& received)
{
    QStatus status = ER_OK;
    uint8_t* tmpBuf = new uint8_t[sg.MaxDataSize()];
    QCC_DbgTrace(("RecvSG(sockfd = %d, sg = <>, received = <>)", sockfd));

    status = Recv(sockfd, tmpBuf, sg.MaxDataSize(), received);
    if (ER_OK == status) {
        sg.CopyFromBuffer(tmpBuf, received);
    }
    QCC_DbgPrintf(("Received %u bytes", received));
    delete[] tmpBuf;
    return status;
}


QStatus RecvFromSG(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort,
                   ScatterGatherList& sg, size_t& received)
{
    QStatus status = ER_OK;
    uint8_t* tmpBuf = new uint8_t[sg.MaxDataSize()];
    QCC_DbgTrace(("RecvToSG(sockfd = %d, remoteAddr = %s, remotePort = %u, sg = <>, sent = <>)",
                  sockfd, remoteAddr.ToString().c_str(), remotePort));

    status = RecvFrom(sockfd, remoteAddr, remotePort, tmpBuf, sg.MaxDataSize(), received);
    if (ER_OK == status) {
        sg.CopyFromBuffer(tmpBuf, received);
    }
    QCC_DbgPrintf(("Received %u bytes", received));
    delete[] tmpBuf;
    return status;
}

}