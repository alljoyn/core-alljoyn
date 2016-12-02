/**
 * @file
 * File for holding global variables.
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#include <qcc/StaticGlobals.h>

#ifdef CRYPTO_CNG
#include <qcc/CngCache.h>
#endif
#include <qcc/Logger.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/PerfCounters.h>
#ifdef QCC_OS_GROUP_WINDOWS
#include <qcc/windows/utility.h>
#include <qcc/windows/NamedPipeWrapper.h>
#endif
#include "Crypto.h"
#include "DebugControl.h"

#define QCC_MODULE "STATICGLOBALS"

namespace qcc {

/* Counters easily found from a debugger, incremented for frequent SCL actions */
volatile uint32_t s_PerfCounters[PERF_COUNTER_COUNT];

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
        NamedPipeWrapper::Init();
#endif

        Event::Init();
        Environ::Init();
        String::Init();
        DebugControl::Init();
        LoggerSetting::Init();
        QStatus status = Thread::StaticInit();
        if (status != ER_OK) {
            Shutdown();
            return status;
        }
        status = Crypto::Init();
        if (status != ER_OK) {
            Shutdown();
            return status;
        }
        return ER_OK;
    }

    static QStatus Shutdown()
    {
        Crypto::Shutdown();
        Thread::StaticShutdown();
        LoggerSetting::Shutdown();
        DebugControl::Shutdown();
        String::Shutdown();
        Environ::Shutdown();
        Event::Shutdown();

#if defined(QCC_OS_GROUP_WINDOWS)
        NamedPipeWrapper::Shutdown();
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