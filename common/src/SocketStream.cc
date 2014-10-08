/**
 * @file SocketStream.cc
 *
 * Sink/Source wrapper for Socket.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014 AllSeen Alliance. All rights reserved.
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
#include <qcc/SocketStream.h>
#include <qcc/Stream.h>
#include <qcc/String.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "NETWORK"


static int MakeSock(AddressFamily family, SocketType type)
{
    SocketFd sock = qcc::INVALID_SOCKET_FD;
    QStatus status = Socket(family, type, sock);
    if (ER_OK != status) {
        QCC_LogError(status, ("Socket failed"));
        sock = -1;
    }
    return sock;
}

static SocketFd CopySock(const SocketFd& inFd)
{
    SocketFd outFd;
    QStatus status = SocketDup(inFd, outFd);
    return (status == ER_OK) ? outFd : qcc::INVALID_SOCKET_FD;
}

SocketStream::SocketStream(SocketFd sock) :
    isConnected(true),
    sock(sock),
    sourceEvent(new Event(sock, Event::IO_READ)),
    sinkEvent(new Event(*sourceEvent, Event::IO_WRITE, false)),
    isDetached(false),
    sendTimeout(Event::WAIT_FOREVER)
{
}


SocketStream::SocketStream(AddressFamily family, SocketType type) :
    isConnected(false),
    sock(MakeSock(family, type)),
    sourceEvent(new Event(sock, Event::IO_READ)),
    sinkEvent(new Event(*sourceEvent, Event::IO_WRITE, false)),
    isDetached(false),
    sendTimeout(Event::WAIT_FOREVER)
{

}

SocketStream::SocketStream(const SocketStream& other) :
    isConnected(other.isConnected),
    sock(CopySock(other.sock)),
    sourceEvent(new Event(sock, Event::IO_READ)),
    sinkEvent(new Event(*sourceEvent, Event::IO_WRITE, false)),
    isDetached(other.isDetached),
    sendTimeout(other.sendTimeout)
{
}

SocketStream SocketStream::operator=(const SocketStream& other)
{
    if (this != &other) {
        if (isConnected) {
            QCC_LogError(ER_FAIL, ("Cannot assign to a connected SocketStream"));
            return *this;
        }
        isConnected = other.isConnected;
        sock = CopySock(other.sock);
        delete sourceEvent;
        sourceEvent = new Event(sock, Event::IO_READ);
        delete sinkEvent;
        sinkEvent = new Event(*sourceEvent, Event::IO_WRITE, false);
        isDetached = other.isDetached;
        sendTimeout = other.sendTimeout;
    }

    return *this;
}

SocketStream::~SocketStream()
{
    /*
     * Must delete the events before closing the socket they monitor
     */
    delete sourceEvent;
    sourceEvent = NULL;
    delete sinkEvent;
    sinkEvent = NULL;
    /*
     * OK to close the socket now.
     */
    qcc::Close(sock);
}

QStatus SocketStream::Connect(qcc::String& host, uint16_t port)
{
    IPAddress ipAddr(host);
    QStatus status = qcc::Connect(sock, ipAddr, port);

    if (ER_WOULDBLOCK == status) {
        status = Event::Wait(*sinkEvent, Event::WAIT_FOREVER);
        if (ER_OK == status) {
            status = qcc::Connect(sock, ipAddr, port);
        }
    }

    isConnected = (ER_OK == status);
    return status;
}

QStatus SocketStream::Connect(qcc::String& path)
{
    QStatus status = qcc::Connect(sock, path.c_str());
    if (ER_WOULDBLOCK == status) {
        status = Event::Wait(*sinkEvent, Event::WAIT_FOREVER);
        if (ER_OK == status) {
            status = qcc::Connect(sock, path.c_str());
        }
    }
    isConnected = (ER_OK == status);
    return status;
}

void SocketStream::Close()
{
    isConnected = false;
    if (!isDetached) {
        qcc::SetLinger(sock, true, 0);
        qcc::Shutdown(sock);
    }
}

QStatus SocketStream::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    if (reqBytes == 0) {
        actualBytes = 0;
        return isConnected ? ER_OK : ER_READ_ERROR;
    }
    QStatus status;
    while (true) {
        if (!isConnected) {
            return ER_READ_ERROR;
        }
        status = Recv(sock, buf, reqBytes, actualBytes);
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
        isConnected = false;
        status = ER_SOCK_OTHER_END_CLOSED;
    }
    return status;
}

QStatus SocketStream::PullBytesAndFds(void* buf, size_t reqBytes, size_t& actualBytes, SocketFd* fdList, size_t& numFds, uint32_t timeout)
{
    QStatus status;
    size_t recvdFds = 0;
    while (true) {
        if (!isConnected) {
            return ER_READ_ERROR;
        }
        /*
         * There will only be one set of file descriptors read in each call to this function
         * so once we have received file descriptors we revert to the standard Recv call.
         */
        if (recvdFds) {
            status = Recv(sock, buf, reqBytes, actualBytes);
        } else {
            status = RecvWithFds(sock, buf, reqBytes, actualBytes, fdList, numFds, recvdFds);
        }
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
        isConnected = false;
        status = ER_SOCK_OTHER_END_CLOSED;
    }
    numFds = recvdFds;
    return status;
}

QStatus SocketStream::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{
    if (numBytes == 0) {
        numSent = 0;
        return ER_OK;
    }
    QStatus status;
    while (true) {
        if (!isConnected) {
            return ER_WRITE_ERROR;
        }
        status = qcc::Send(sock, buf, numBytes, numSent);
        if (ER_WOULDBLOCK == status) {
            if (sendTimeout == Event::WAIT_FOREVER) {
                status = Event::Wait(*sinkEvent);
            } else {
                status = Event::Wait(*sinkEvent, sendTimeout);
            }
            if (ER_OK != status) {
                break;
            }
        } else {
            break;
        }
    }
    return status;
}

QStatus SocketStream::PushBytesAndFds(const void* buf, size_t numBytes, size_t& numSent, SocketFd* fdList, size_t numFds, uint32_t pid)
{
    if (numBytes == 0) {
        return ER_BAD_ARG_2;
    }
    if (numFds == 0) {
        return ER_BAD_ARG_5;
    }
    QStatus status;
    while (true) {
        if (!isConnected) {
            return ER_WRITE_ERROR;
        }
        status = qcc::SendWithFds(sock, buf, numBytes, numSent, fdList, numFds, pid);
        if (ER_WOULDBLOCK == status) {
            if (sendTimeout == Event::WAIT_FOREVER) {
                status = Event::Wait(*sinkEvent);
            } else {
                status = Event::Wait(*sinkEvent, sendTimeout);
            }
            if (ER_OK != status) {
                break;
            }
        } else {
            break;
        }
    }
    return status;
}

QStatus SocketStream::SetNagle(bool reuse)
{
    return qcc::SetNagle(sock, reuse);
}
