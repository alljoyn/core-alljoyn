/**
 * @file
 *
 * BusController is responsible for responding to standard DBus messages
 * directed at the bus itself.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2012,2014 AllSeen Alliance. All rights reserved.
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

#include "BusController.h"
#include "DaemonRouter.h"
#include "BusInternal.h"

#define QCC_MODULE "ALLJOYN_DAEMON"

using namespace std;
using namespace qcc;
using namespace ajn;

BusController::BusController(Bus& alljoynBus, AuthListener* authListener) :
    bus(alljoynBus),
    authListener(authListener),
    dbusObj(bus, this),
    alljoynObj(bus, this),
    sessionlessObj(bus, this),
#ifndef NDEBUG
    alljoynDebugObj(this),
#endif
    initComplete(false)

{
    DaemonRouter& router(reinterpret_cast<DaemonRouter&>(bus.GetInternal().GetRouter()));
    router.SetBusController(this);
}

BusController::~BusController()
{
    Stop();
    Join();
}

QStatus BusController::Init(const qcc::String& listenSpecs)
{
    QStatus status;

    /*
     * Start the object initialization chain (see ObjectRegistered callback below)
     */
    status = dbusObj.Init();
    if (ER_OK != status) {
        QCC_LogError(status, ("DBusObj::Init failed"));
    } else {
        if (status == ER_OK) {
            status = bus.Start();
        }
        if (status == ER_OK) {
            while (!initComplete) {
                qcc::Sleep(4);
            }
        }
        if (status == ER_OK) {
            status = bus.StartListen(listenSpecs.c_str());
            if (status != ER_OK) {
                bus.Stop();
                bus.Join();
            }
        }
    }

    return status;
}

QStatus BusController::Stop()
{
    QStatus status = dbusObj.Stop();
    if (status != ER_OK) {
        QCC_LogError(status, ("dbusObj::Stop failed"));
    }

    QStatus tStatus = alljoynObj.Stop();
    if (tStatus != ER_OK) {
        QCC_LogError(tStatus, ("alljoynObj::Stop failed"));
    }
    status = (status == ER_OK) ? tStatus : status;

    tStatus = sessionlessObj.Stop();
    if (tStatus != ER_OK) {
        QCC_LogError(tStatus, ("sessionlessObj::Stop failed"));
    }
    status = (status == ER_OK) ? tStatus : status;

#ifndef NDEBUG
    tStatus = alljoynDebugObj.Stop();
    if (tStatus != ER_OK) {
        QCC_LogError(tStatus, ("alljoynDebugObj::Stop failed"));
    }
    status = (status == ER_OK) ? tStatus : status;
#endif

    tStatus = bus.Stop();
    if (tStatus != ER_OK) {
        QCC_LogError(tStatus, ("bus::Stop failed"));
    }
    status = (status == ER_OK) ? tStatus : status;

    return status;
}

QStatus BusController::Join()
{
    QStatus status = dbusObj.Join();
    if (status != ER_OK) {
        QCC_LogError(status, ("dbusObj::Join failed"));
    }

    QStatus tStatus = alljoynObj.Join();
    if (tStatus != ER_OK) {
        QCC_LogError(tStatus, ("alljoynObj::Join failed"));
    }
    status = (status == ER_OK) ? tStatus : status;

    tStatus = sessionlessObj.Join();
    if (tStatus != ER_OK) {
        QCC_LogError(tStatus, ("sessionlessObj::Join failed"));
    }
    status = (status == ER_OK) ? tStatus : status;

#ifndef NDEBUG
    tStatus = alljoynDebugObj.Join();
    if (tStatus != ER_OK) {
        QCC_LogError(tStatus, ("alljoynDebugObj::Join failed"));
    }
    status = (status == ER_OK) ? tStatus : status;
#endif

    tStatus = bus.Join();
    if (tStatus != ER_OK) {
        QCC_LogError(tStatus, ("bus::Join failed"));
    }
    status = (status == ER_OK) ? tStatus : status;

    return status;
}

/*
 * The curious code below is to force various bus objects to be registered in order:
 *
 * /org/freedesktop/DBus
 * /org/alljoyn/Bus
 * /org/alljoyn/Debug
 *
 * The last one is optional and only registered for debug builds
 */
void BusController::ObjectRegistered(BusObject* obj)
{
    QStatus status;
    bool isDone = false;

    if (obj == &dbusObj) {
        status = alljoynObj.Init();
        if (status != ER_OK) {
            isDone = true;
            QCC_LogError(status, ("alljoynObj::Init failed"));
        }
    }
    if (obj == &alljoynObj) {
        status = sessionlessObj.Init();
        if (status != ER_OK) {
            isDone = true;
            QCC_LogError(status, ("sessionlessObj::Init failed"));
        }
    }
#ifndef NDEBUG
    if (obj == &sessionlessObj) {
        status = alljoynDebugObj.Init();
        if (status != ER_OK) {
            isDone = true;
            QCC_LogError(status, ("alljoynDebugObj::Init failed"));
        }
    }
    if (obj == &alljoynDebugObj) {
        isDone = true;
    }
#else
    if (obj == &sessionlessObj) {
        isDone = true;
    }
#endif

    if (isDone) {
        initComplete = true;
    }
}
