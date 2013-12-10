/**
 * @file STUNSocketStream.cc
 *
 * Sink/Source wrapper for STUN.
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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

#include <qcc/Socket.h>
#include <qcc/Stream.h>
#include <qcc/String.h>

#include <alljoyn/Status.h>
#include <STUNSocketStream.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "STUN_SOCKET_STREAM"

namespace ajn {

static SocketFd CopySock(const SocketFd& inFd)
{
    SocketFd outFd;
    QStatus status = SocketDup(inFd, outFd);
    return (status == ER_OK) ? outFd : -1;
}

STUNSocketStream::STUNSocketStream(Stun* stunPtr) :
    isConnected(true),
    stunPtr(stunPtr),
    sock(stunPtr->GetSocketFD()),
    sourceEvent(new Event(sock, Event::IO_READ, false)),
    sinkEvent(new Event(sock, Event::IO_WRITE, false)),
    isDetached(false)
{
}

STUNSocketStream::STUNSocketStream(const STUNSocketStream& other) :
    isConnected(other.isConnected),
    stunPtr(other.stunPtr),
    sock(CopySock(other.sock)),
    sourceEvent(new Event(sock, Event::IO_READ, false)),
    sinkEvent(new Event(*sourceEvent, Event::IO_WRITE, false)),
    isDetached(other.isDetached)
{
}

STUNSocketStream STUNSocketStream::operator=(const STUNSocketStream& other)
{
    if (this != &other) {
        Close();
        isConnected = other.isConnected;
        stunPtr = other.stunPtr;
        sock = CopySock(other.sock);
        if (sourceEvent) {
            delete sourceEvent;
            sourceEvent = NULL;
        }
        sourceEvent = new Event(sock, Event::IO_READ, false);
        if (sinkEvent) {
            delete sinkEvent;
            sinkEvent = NULL;
        }
        sinkEvent = new Event(*sourceEvent, Event::IO_WRITE, false);
        isDetached = other.isDetached;
    }
    return *this;
}

STUNSocketStream::~STUNSocketStream()
{
    Close();
    if (sourceEvent) {
        delete sourceEvent;
    }
    if (sinkEvent) {
        delete sinkEvent;
    }
}

void STUNSocketStream::Close()
{
    if (isConnected) {
        if (!isDetached) {
            stunPtr->Shutdown();
        }
        isConnected = false;
    }
    if ((SOCKET_ERROR != sock) && !isDetached) {
        stunPtr->Close();
        sock = SOCKET_ERROR;
    }
}

QStatus STUNSocketStream::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    if (!isConnected) {
        return ER_FAIL;
    }
    if (reqBytes == 0) {
        actualBytes = 0;
        return ER_OK;
    }
    QStatus status;
    while (true) {
        status = stunPtr->AppRecv(buf, reqBytes, actualBytes);
        if (ER_WOULDBLOCK == status) {
            status = Event::Wait(*sourceEvent, timeout);
            if (ER_OK != status) {
                break;
            }
        } else {
            break;
        }
    }

    if ((ER_OK == status) && (0 == actualBytes)) {
        /* Other end has closed */
        Close();
        status = ER_SOCK_OTHER_END_CLOSED;
    }
    return status;
}

QStatus STUNSocketStream::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{
    if (!isConnected) {
        return ER_FAIL;
    }
    if (numBytes == 0) {
        numSent = 0;
        return ER_OK;
    }
    QStatus status;
    while (true) {
        status = stunPtr->AppSend(buf, numBytes, numSent);
        if (ER_WOULDBLOCK == status) {
            status = Event::Wait(*sinkEvent);
            if (ER_OK != status) {
                break;
            }
        } else {
            break;
        }
    }

    return status;
}

} // namespace ajn
