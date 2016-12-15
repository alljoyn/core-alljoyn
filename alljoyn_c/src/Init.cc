/**
 * @file
 * Init implements the library init and shutdown functions.
 */

/******************************************************************************
 *  *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include <alljoyn_c/Init.h>
#include "DeferredCallback.h"
#include <alljoyn/Init.h>
#include <qcc/LockLevel.h>
#include <limits>

#define QCC_MODULE "ALLJOYN_C"

/*
 * Disable LockOrderChecker for the Init lock, because this lock is being used
 * before LockOrderChecker & Thread modules have been initialized.
 */
static qcc::Mutex alljoyn_init_lock(qcc::LOCK_LEVEL_CHECKING_DISABLED);
static uint32_t alljoyn_init_count = 0;

QStatus AJ_CALL alljoyn_init(void)
{
    QStatus status = ER_OK;

    alljoyn_init_lock.Lock();

    if (alljoyn_init_count == 0) {
        status = AllJoynInit();
        if (status == ER_OK) {
            ajn::DeferredCallback::Init();
            alljoyn_init_count = 1;
        }
    } else if (alljoyn_init_count == std::numeric_limits<uint32_t>::max()) {
        QCC_ASSERT(!"Incorrect alljoyn_init count");
        status = ER_INVALID_APPLICATION_STATE;
    } else {
        alljoyn_init_count++;
    }

    alljoyn_init_lock.Unlock();

    return status;
}

QStatus AJ_CALL alljoyn_shutdown(void)
{
    QStatus status = ER_OK;

    alljoyn_init_lock.Lock();

    QCC_ASSERT(alljoyn_init_count > 0);
    alljoyn_init_count--;

    if (alljoyn_init_count == 0) {
        ajn::DeferredCallback::Shutdown();
        status =  AllJoynShutdown();
    }

    alljoyn_init_lock.Unlock();

    return status;
}

QStatus AJ_CALL alljoyn_routerinit(void)
{
#ifdef ROUTER
    return AllJoynRouterInit();
#else
    QCC_ASSERT(!"alljoyn_routerinit called when bundled router was not built in.");
    return ER_NOT_IMPLEMENTED;
#endif
}

QStatus AJ_CALL alljoyn_routerinitwithconfig(AJ_PCSTR configXml)
{
#ifdef ROUTER
    return AllJoynRouterInitWithConfig(configXml);
#else
    QCC_ASSERT(!"alljoyn_routerinitwithconfig called when bundled router was not built in.");
    QCC_UNUSED(configXml);
    return ER_NOT_IMPLEMENTED;
#endif
}

QStatus AJ_CALL alljoyn_routershutdown(void)
{
#ifdef ROUTER
    return AllJoynRouterShutdown();
#else
    QCC_ASSERT(!"alljoyn_routershutdown called when bundled router was not built in.");
    return ER_NOT_IMPLEMENTED;
#endif
}