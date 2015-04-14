/**
 * @file
 * File for holding global variables.
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#include <qcc/StaticGlobals.h>

#ifdef CRYPTO_CNG
#include <qcc/CngCache.h>
#endif
#include <qcc/Logger.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/Time.h>
#ifdef QCC_OS_GROUP_WINDOWS
#include <qcc/windows/utility.h>
#endif
#include "Crypto.h"
#include "DebugControl.h"

#define QCC_MODULE "STATICGLOBALS"

namespace qcc {

class StaticGlobals {
  public:
    static QStatus Init()
    {
#if defined(QCC_OS_GROUP_WINDOWS)
        WSADATA wsaData;
        WORD version = MAKEWORD(2, 0);
        int error = WSAStartup(version, &wsaData);
        if (error) {
            printf("WSAStartup failed with error: %d\n", error);
            return ER_OS_ERROR;
        }
#endif
        qcc::TimestampInit();
        Event::Init();
        Environ::Init();
        String::Init();
        DebugControl::Init();
        LoggerSetting::Init();
        QStatus status = Thread::Init();
        if (status != ER_OK) {
            Shutdown();
            return status;
        }
        Crypto::Init();
        return ER_OK;
    }

    static QStatus Shutdown()
    {
        Crypto::Shutdown();
        Thread::Shutdown();
        LoggerSetting::Shutdown();
        DebugControl::Shutdown();
        String::Shutdown();
        Environ::Shutdown();
        Event::Shutdown();
        qcc::TimestampShutdown();
#if defined(QCC_OS_GROUP_WINDOWS)
        int error = WSACleanup();
        if (SOCKET_ERROR == error) {
            printf("WSACleanup failed with error: %d\n", WSAGetLastError());
            /*
             * Continue on to shutdown as much as possible since recovery path
             * is not clear.
             */
        }
#endif
        return ER_OK;
    }
};

QStatus Init()
{
    return StaticGlobals::Init();
}

QStatus Shutdown()
{
    return StaticGlobals::Shutdown();
}

} /* namespace qcc */
