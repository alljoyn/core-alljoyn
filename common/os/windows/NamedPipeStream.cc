/**
 * @file
 *
 * Named Pipe streaming operations for windows.
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

/*
 * Compile this file only for Windows version 10 or greater.
 */
 #if (_WIN32_WINNT > 0x0603)

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Time.h>
#include <windows.h>
#include <msajtransport.h>

#include <qcc/FileStream.h>
#include <qcc/Windows/NamedPipeStream.h>

using namespace std;
using namespace qcc;

/** @internal */
#define QCC_MODULE  "NETWORK"

static HANDLE DuplicateBusHandle(HANDLE inHandle)
{
    HANDLE outHandle = INVALID_HANDLE_VALUE;

    BOOL ret = DuplicateHandle(GetCurrentProcess(),
                               inHandle,
                               GetCurrentProcess(),
                               &outHandle,
                               0,
                               FALSE,
                               DUPLICATE_SAME_ACCESS);
    if (ret != TRUE) {
        QCC_LogError(ER_OS_ERROR, ("Duplicating bus handle failed (0x%08X)", ::GetLastError()));
    }
    return outHandle;
}

NamedPipeStream::NamedPipeStream(HANDLE busHandle) :
    isConnected(true),
    busHandle(busHandle),
    sourceEvent(new Event(busHandle, Event::IO_READ)),
    sinkEvent(new Event(busHandle, Event::IO_WRITE)),
    isDetached(false),
    sendTimeout(Event::WAIT_FOREVER)
{
}

NamedPipeStream::NamedPipeStream(const NamedPipeStream& other) :
    isConnected(other.isConnected),
    busHandle((other.busHandle == INVALID_HANDLE_VALUE) ? INVALID_HANDLE_VALUE : DuplicateBusHandle(other.busHandle)),
    sourceEvent(new Event(busHandle, Event::IO_READ)),
    sinkEvent(new Event(busHandle, Event::IO_WRITE)),
    isDetached(other.isDetached),
    sendTimeout(Event::WAIT_FOREVER)
{
}

NamedPipeStream::~NamedPipeStream()
{
    delete sourceEvent;
    sourceEvent = NULL;
    delete sinkEvent;
    sinkEvent = NULL;

    if (busHandle != INVALID_HANDLE_VALUE) {
        AllJoynCloseBusHandle(busHandle);
        busHandle = INVALID_HANDLE_VALUE;
    }
}

QStatus NamedPipeStream::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    if (reqBytes == 0) {
        actualBytes = 0;
        return isConnected ? ER_OK : ER_READ_ERROR;
    }

    if (busHandle == INVALID_HANDLE_VALUE) {
        return ER_INIT_FAILED;
    }

    DWORD readBytes;
    BOOL success;
    QStatus status = ER_OK;

    while (true) {
        if (!isConnected) {
            status = ER_READ_ERROR;
            break;
        }

        /*
         * Non-blocking read
         */
        success = AllJoynReceiveFromBus(busHandle, buf, reqBytes, &readBytes, nullptr);

        if (success == FALSE) {
            QCC_LogError(ER_FAIL, ("AllJoynReceiveFromBus failed. The other end closed the pipe."));
            status = ER_SOCK_OTHER_END_CLOSED;
            isConnected = false;
        } else if (readBytes == 0) {
            status = Event::Wait(*sourceEvent, timeout);
            if (status == ER_OK) {
                continue;
            }
        } else {
            QCC_DbgTrace(("AllJoynReceiveFromBus(busHandle = %p, buf = <>, reqBytes = %lu, readBytes = %lu)", busHandle, reqBytes, readBytes));
        }
        break;
    }

    actualBytes = readBytes;
    return status;
}


/*
 *  PlaceHolder for future implementation. Currently, not being used, so just calling directly into PullBytes(buf, reqBytes, actualBytes, timeout);
 */
QStatus NamedPipeStream::PullBytesAndFds(void* buf, size_t reqBytes, size_t& actualBytes, SocketFd* fdList, size_t& numFds, uint32_t timeout)
{
    numFds = 0;
    return PullBytes(buf, reqBytes, actualBytes, timeout);
}


QStatus NamedPipeStream::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{
    if (numBytes == 0) {
        numSent = 0;
        return ER_OK;
    }

    if (busHandle == INVALID_HANDLE_VALUE) {
        return ER_INIT_FAILED;
    }

    DWORD writeBytes;
    BOOL success = TRUE;
    QStatus status = ER_OK;

    while (true) {
        if (!isConnected) {
            status = ER_WRITE_ERROR;
            break;
        }

        success = AllJoynSendToBus(busHandle, buf, numBytes, &writeBytes, nullptr);

        if (success == FALSE) {
            QCC_LogError(ER_FAIL, ("AllJoynSendToBus failed. The other end closed the pipe (0x%08X).", ::GetLastError()));
            status = ER_SOCK_OTHER_END_CLOSED;
            isConnected = false;
        } else if (writeBytes == 0) {
            status = Event::Wait(*sinkEvent, sendTimeout);
            if (status == ER_OK) {
                continue;
            }
        }
        break;
    }
    QCC_DbgTrace(("AllJoynSendToBus(busHandle = %p, *buf = <>, numBytes = %lu, numSent = %lu)", busHandle, numBytes, writeBytes));

    if (status == ER_OK) {
        numSent = writeBytes;
    } else {
        QCC_LogError(ER_FAIL, ("PushBytes failed!"));
    }

    return status;
}


/*
 *  PlaceHolder for future implementation. Currently, not being used, so just calling directly into PushBytes(buf, numBytes, numSent)
 */
QStatus NamedPipeStream::PushBytesAndFds(const void* buf, size_t numBytes, size_t& numSent, SocketFd* fdList, size_t numFds, uint32_t pid)
{
    if (numBytes == 0) {
        return ER_BAD_ARG_2;
    }

    if (numFds != 0) {
        return ER_BAD_ARG_5;
    }

    return (PushBytes(buf, numBytes, numSent));
}


void NamedPipeStream::Close()
{
    isConnected = false;
    isDetached = true;
}

#endif //#if (_WIN32_WINNT > 0x0603)
