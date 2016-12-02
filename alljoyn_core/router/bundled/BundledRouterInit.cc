/**
 * @file
 * Implementation of class for launching a bundled router
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
#include <qcc/Mutex.h>
#include <qcc/StaticGlobals.h>
#include <alljoyn/Init.h>
#include "BundledRouter.h"
#include "RouterGlobals.h"
#include <limits>

static ajn::BundledRouter* bundledRouter = NULL;

extern "C" {

static uint32_t allJoynRouterInitCount = 0;
static qcc::Mutex allJoynRouterInitLock;

QStatus AJ_CALL AllJoynRouterInitImpl(AJ_PCSTR configXml)
{
    QStatus status = ER_OK;

    allJoynRouterInitLock.Lock();

    if (allJoynRouterInitCount == 0) {
        ajn::RouterGlobals::Init();
        bundledRouter = new ajn::BundledRouter(configXml);
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

QStatus AJ_CALL AllJoynRouterInit(void)
{
    return AllJoynRouterInitImpl("");
}

QStatus AJ_CALL AllJoynRouterInitWithConfig(AJ_PCSTR configXml)
{
    return AllJoynRouterInitImpl(configXml);
}

QStatus AJ_CALL AllJoynRouterShutdown(void)
{
    allJoynRouterInitLock.Lock();

    QCC_ASSERT(allJoynRouterInitCount > 0);
    allJoynRouterInitCount--;

    if (allJoynRouterInitCount == 0) {
        delete bundledRouter;
        bundledRouter = NULL;
        ajn::RouterGlobals::Shutdown();
    }

    allJoynRouterInitLock.Unlock();

    return ER_OK;
}


}