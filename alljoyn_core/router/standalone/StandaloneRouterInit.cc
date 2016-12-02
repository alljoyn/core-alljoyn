/**
 * @file
 * Static global creation and destruction
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

#include <qcc/platform.h>
#include <qcc/Logger.h>
#include <qcc/Mutex.h>
#include <qcc/StaticGlobals.h>
#include <alljoyn/Init.h>
#include "RouterGlobals.h"
#include <limits>

extern "C" {

static uint32_t allJoynRouterInitCount = 0;
static qcc::Mutex allJoynRouterInitLock;

QStatus AJ_CALL AllJoynRouterInit(void)
{
    QStatus status = ER_OK;

    allJoynRouterInitLock.Lock();

    if (allJoynRouterInitCount == 0) {
        ajn::RouterGlobals::Init();
        allJoynRouterInitCount = 1;
    } else if (allJoynRouterInitCount == std::numeric_limits<uint32_t>::max()) {
        QCC_ASSERT(!"Incorrect allJoynRouterInitCount count");
        status = ER_INVALID_APPLICATION_STATE;
    } else {
        allJoynRouterInitCount++;
    }

    allJoynRouterInitLock.Unlock();

    return status;
}

QStatus AJ_CALL AllJoynRouterInitWithConfig(AJ_PCSTR configXml)
{
    QCC_UNUSED(configXml);
    qcc::Log(LOG_ERR, "AllJoynRouterInitWithConfig can only by used with a bundled router. "
             "For the standalone router, please use the \"--config-file\" option instead.");
    return ER_INVALID_APPLICATION_STATE;
}

QStatus AJ_CALL AllJoynRouterShutdown(void)
{
    allJoynRouterInitLock.Lock();

    QCC_ASSERT(allJoynRouterInitCount > 0);
    allJoynRouterInitCount--;

    if (allJoynRouterInitCount == 0) {
        ajn::RouterGlobals::Shutdown();
    }

    allJoynRouterInitLock.Unlock();

    return ER_OK;
}

}