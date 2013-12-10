/**
 * @file
 * BTEndpoint implementation for Windows.
 */

/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
#include "WindowsBTEndpoint.h"

#define QCC_MODULE "ALLJOYN_BT"

namespace ajn {

WindowsBTEndpoint::~WindowsBTEndpoint()
{
    QCC_DbgTrace(("WindowsBTEndpoint::~WindowsBTEndpoint()"));

    ajn::BTTransport::BTAccessor* accessor = btStream.GetAccessor();

    if (accessor) {
        accessor->EndPointsRemove(this);
    }

    if (connectionCompleteEvent) {
        ::CloseHandle(connectionCompleteEvent);
        connectionCompleteEvent = NULL;
    }  else {
        QCC_LogError(ER_INIT_FAILED, ("connectionCompleteEvent is NULL!"));
    }

    connectionStatus = ER_FAIL;
}

QStatus WindowsBTEndpoint::WaitForConnectionComplete(bool incoming)
{
    QCC_DbgTrace(("WindowsBTEndpoint::WaitForConnectionComplete(address = 0x%012I64X)",
                  GetRemoteDeviceAddress()));

    connectionStatus = ER_INIT_FAILED;

    if (connectionCompleteEvent) {
        const DWORD waitTimeInMilliseconds = 30000;
        DWORD waitStatus = WaitForSingleObject(connectionCompleteEvent, waitTimeInMilliseconds);

        switch (waitStatus) {
        case WAIT_OBJECT_0:
            if (incoming) {
                uint8_t nul = 255;
                size_t recvd;
                connectionStatus = btStream.PullBytes(&nul, sizeof(nul), recvd, waitTimeInMilliseconds);
                if ((connectionStatus != ER_OK) || (nul != 0)) {
                    connectionStatus = (connectionStatus == ER_OK) ? ER_FAIL : connectionStatus;
                    QCC_LogError(connectionStatus, ("Did not receive initial nul byte"));
                }
            } else {
                uint8_t nul = 0;
                size_t sent;
                connectionStatus = btStream.PushBytes(&nul, sizeof(nul), sent);
            }
            break;

        case WAIT_TIMEOUT:
            QCC_DbgPrintf(("WaitForConnectionComplete() timeout! (%u mS)", waitTimeInMilliseconds));
            connectionStatus = ER_TIMEOUT;
            break;

        default:
            connectionStatus = ER_FAIL;
            break;
        }
    } else {
        QCC_LogError(connectionStatus, ("connectionCompleteEvent is NULL!"));
    }

    return connectionStatus;
}

void WindowsBTEndpoint::SetConnectionComplete(QStatus status)
{
    QCC_DbgTrace(("WindowsBTEndpoint::SetConnectionComplete(handle = %p, status = %s)",
                  GetChannelHandle(), QCC_StatusText(status)));

    connectionStatus = status;

    if (GetChannelHandle()) {
        if (connectionCompleteEvent) {
            ::SetEvent(connectionCompleteEvent);
        } else {
            QCC_LogError(ER_INIT_FAILED, ("connectionCompleteEvent is NULL!"));
        }
    } else {
        QCC_LogError(ER_INIT_FAILED, ("connectionCompleteEvent orphaned (channel is NULL)"));
    }
}



} // namespace ajn
