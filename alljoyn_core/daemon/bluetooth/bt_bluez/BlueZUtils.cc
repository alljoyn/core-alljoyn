/**
 * @file
 * Utility classes for the BlueZ implementation of BT transport.
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>

#include "BDAddress.h"
#include "BlueZ.h"
#include "BlueZIfc.h"
#include "BlueZUtils.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_BT"

using namespace ajn;
using namespace std;
using namespace qcc;

namespace ajn {
namespace bluez {


BTSocketStream::BTSocketStream(SocketFd sock) :
    SocketStream(sock),
    buffer(NULL),
    offset(0),
    fill(0)
{
    struct l2cap_options opts;
    socklen_t optlen = sizeof(opts);
    int ret;
    ret = getsockopt(sock, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (ret == -1) {
        QCC_LogError(ER_OS_ERROR, ("Failed to get in/out MTU for L2CAP socket, using default of 672"));
        inMtu = 672;
        outMtu = 672;
    } else {
        inMtu = opts.imtu;
        outMtu = opts.omtu;
    }
    buffer = new uint8_t[inMtu];
}


QStatus BTSocketStream::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    if (!IsConnected()) {
        return ER_FAIL;
    }
    if (reqBytes == 0) {
        actualBytes = 0;
        return ER_OK;
    }

    QStatus status;
    size_t avail = fill - offset;

    if (avail > 0) {
        /* Pull from internal buffer */
        actualBytes = min(avail, reqBytes);
        memcpy(buf, &buffer[offset], actualBytes);
        offset += actualBytes;
        status = ER_OK;
    } else if (reqBytes >= inMtu) {
        /* Pull directly into user buffer */
        status = SocketStream::PullBytes(buf, reqBytes, actualBytes, timeout);
    } else {
        /* Pull into internal buffer */
        status = SocketStream::PullBytes(buffer, inMtu, avail, timeout);
        if (status == ER_OK) {
            actualBytes = min(avail, reqBytes);
            /* Partial copy from internal buffer to user buffer */
            memcpy(buf, buffer, actualBytes);
            fill = avail;
            offset = actualBytes;
        }
    }
    return status;
}


QStatus BTSocketStream::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{
    QStatus status;
    int retry = 400;

    do {
        errno = 0;
        status = SocketStream::PushBytes(buf, min(numBytes, outMtu), numSent);
        if ((status == ER_OS_ERROR) && ((errno == EAGAIN) ||
                                        (errno == EBUSY) ||
                                        (errno == ENOMEM) ||
                                        (errno == EFAULT))) {
            // BlueZ reports ENOMEM and EFAULT when it should report EBUSY or EAGAIN, just wait and try again.
            qcc::Sleep(50);
            --retry;
        } else {
            // Don't retry on success or errors not listed above.
            retry = 0;
        }
    } while (retry > 0);

    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to send data over BT for 20 seconds (errno: %d - %s)", errno, strerror(errno)));
    }

    return status;
}


} // namespace bluez
} // namespace ajn

