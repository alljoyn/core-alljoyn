/**
 * @file
 *
 * This file implements a BLE based physical link for communication.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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
#if !defined(QCC_OS_DARWIN)
#include <qcc/BLEStream.h>
#include <fcntl.h>
#include <errno.h>

#include <termios.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>

#define QCC_MODULE "BLE"

using namespace qcc;
using namespace std;

namespace qcc {

QStatus BLE(qcc::String& remObj)
{
    qcc::String devName = "";

    return BLE(devName, remObj);
}

QStatus BLE(const qcc::String& devName, qcc::String& remObj)
{
    QCC_DbgTrace(("BLE(devName=%s)", devName.c_str()));
    remObj = "";

    /*
     * Validate and set parameters
     */

    QCC_DbgPrintf(("Ble::ReadCallback()"));
    return ER_NOT_IMPLEMENTED;
}

} /* namespace qcc */

BLEStream::BLEStream(BLEStreamAccessor* accessor, qcc::String remObj) :
    remObj(remObj),
    locAcc(accessor)
{

}

BLEStream::~BLEStream()
{
    //Not Implemented
}

/* This frame size is chosen because it is the Max possible size for a BLE
 * SLAP payload.
 */
#define RX_BUFSIZE  20
static uint8_t RxBuffer[RX_BUFSIZE];

QStatus BLEStream::PullBytes(void* buf, size_t numBytes, size_t& actualBytes, uint32_t timeout)
{
    QCC_DbgPrintf(("Ble::PullBytes()"));
    return ER_NOT_IMPLEMENTED;
}

void BLEStream::Close()
{
    QCC_DbgPrintf(("Ble::close()"));
}

QStatus BLEStream::PushBytes(const void* buf, size_t numBytes, size_t& actualBytes)
{
    QCC_DbgPrintf(("Ble::PushBytes()"));
    return locAcc->PushBytes(remObj, buf, numBytes, actualBytes);
}

/* BLEController Should not use a BLE Stream, but rather the BLE Accessor, and
 * store the dbus object path to the remote device as a private storage string
 * */
BLEController::BLEController(BLEStream* bleStream, StreamReadListener* readListener) :
    StreamController(readListener),
    m_bleStream(bleStream), exitCount(0)
{
}

QStatus BLEController::Start()
{
    return ER_OK;
}

QStatus BLEController::Stop()
{
    return ER_OK;
}

QStatus BLEController::Join()
{
    while (!exitCount) {
        qcc::Sleep(100);
    }
    return ER_OK;
}

QStatus BLEController::ReadCallback(const uint8_t* buffer, size_t bytes)
{
    QCC_DbgPrintf(("Ble::ReadCallback()"));
    if (bytes > RX_BUFSIZE) {
        return ER_BUFFER_TOO_SMALL;
    }
    memcpy(RxBuffer, buffer, bytes);
    m_readListener->ReadEventTriggered(RxBuffer, bytes);
    return ER_OK;
}

QStatus BLEController::PushBytes(const void* buf, size_t numBytes, size_t& actualBytes)
{
    QCC_DbgPrintf(("BLEController::PushBytes()"));
    return m_bleStream->PushBytes(buf, numBytes, actualBytes);
}

#endif
