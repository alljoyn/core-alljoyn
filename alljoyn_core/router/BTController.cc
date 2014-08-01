/**
 * @file
 *
 * BusObject responsible for controlling/handling Bluetooth delegations and
 * implements the org.alljoyn.Bus.BTController interface.
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

#include <qcc/platform.h>

#include <set>
#include <vector>

#include <qcc/Environ.h>
#include <qcc/GUID.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <alljoyn/AllJoynStd.h>

#include "BTController.h"
#include "BLEEndpoint.h"

#define QCC_MODULE "BLE"


using namespace std;
using namespace qcc;

static const uint32_t ABSOLUTE_MAX_CONNECTIONS = 7; /* BT can't have more than 7 direct connections */
static const uint32_t DEFAULT_MAX_CONNECTIONS =  3; /* Gotta allow 1 connection for car-kit/headset/headphones */

namespace ajn {

static const char* bluetoothObjPath = "/org/alljoyn/Bus/BluetoothController";

static const SessionOpts BTSESSION_OPTS(SessionOpts::TRAFFIC_MESSAGES,
                                        false,
                                        SessionOpts::PROXIMITY_ANY,
                                        TRANSPORT_LOCAL);

BTController::BTController(BusAttachment& bus, BluetoothDeviceInterface& bt) :
    BusObject(bluetoothObjPath),
    bus(bus),
    bt(bt),
    maxConnections(min(StringToU32(Environ::GetAppEnviron()->Find("ALLJOYN_MAX_BT_CONNECTIONS"), 0, DEFAULT_MAX_CONNECTIONS),
                       ABSOLUTE_MAX_CONNECTIONS)),
    listening(false),
    devAvailable(false),
    dispatcher("BTC-Dispatcher")
{
    dispatcher.Start();
}


BTController::~BTController()
{
    // Don't need to remove our bus name change listener from the router (name
    // table) since the router is already destroyed at this point in time.

    dispatcher.Stop();
    dispatcher.Join();

    bus.UnregisterBusObject(*this);
}


void BTController::ObjectRegistered() {
}


QStatus BTController::Init()
{
    return ER_OK;
}

void BTController::BLEDeviceAvailable(bool on)
{
    QCC_DbgTrace(("BTController::BLEDeviceAvailable(<%s>)", on ? "on" : "off"));
    DispatchOperation(new BLEDevAvailDispatchInfo(on));
}


void BTController::NameOwnerChanged(const qcc::String& alias,
                                    const String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                                    const String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer)
{
    QCC_DbgTrace(("BTController::NameOwnerChanged(alias = %s, oldOwner = %s, newOwner = %s)",
                  alias.c_str(),
                  oldOwner ? oldOwner->c_str() : "<null>",
                  newOwner ? newOwner->c_str() : "<null>"));
    if (oldOwner && (alias == *oldOwner) && (alias != bus.GetUniqueName())) {
        DispatchOperation(new NameLostDispatchInfo(alias));
    } else if (!oldOwner && newOwner && (alias == org::alljoyn::Daemon::WellKnownName)) {
        /*
         * Need to bind the session port here instead of in the
         * ObjectRegistered() function since there is a race condition because
         * there is a race condition between which object will get registered
         * first: AllJoynObj or BTController.  Since AllJoynObj must be
         * registered before we can bind the session port we wait for
         * AllJoynObj to acquire its well known name.
         */
        SessionPort port = ALLJOYN_BTCONTROLLER_SESSION_PORT;
        QStatus status = bus.BindSessionPort(port, BTSESSION_OPTS, *this);
        if (status != ER_OK) {
            QCC_LogError(status, ("BindSessionPort(port = %04x, opts = <%x, %x, %x>, listener = %p)",
                                  port,
                                  BTSESSION_OPTS.traffic, BTSESSION_OPTS.proximity, BTSESSION_OPTS.transports,
                                  this));
        }
    }
}

void BTController::JoinSessionCB(QStatus status, SessionId sessionID, const SessionOpts& opts, void* context)
{
    QCC_LogError(ER_FAIL, ("BTController - Not Implemented"));
}


void BTController::DeferredBLEDeviceAvailable(bool on)
{
    QCC_DbgTrace(("BTController::DeferredBLEDeviceAvailable(<%s>)", on ? "on" : "off"));
    lock.Lock(MUTEX_CONTEXT);

    if (on && !devAvailable) {
        devAvailable = true;
        bt.StartListen();
    } else if (!on && devAvailable) {
        if (listening) {
            bt.StopListen();
            listening = false;
        }

        blacklist->clear();

        devAvailable = false;
    }

    lock.Unlock(MUTEX_CONTEXT);
}

void BTController::DeferredNameLostHander(const String& name)
{
    QCC_LogError(ER_FAIL, ("BTController - Not Implemented"));
}


void BTController::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    QCC_DbgTrace(("BTController::AlarmTriggered(alarm = <>, reasons = %s)", QCC_StatusText(reason)));
    DispatchInfo* op = static_cast<DispatchInfo*>(alarm->GetContext());
    assert(op);

    if (reason == ER_OK) {
        QCC_DbgPrintf(("Handling deferred operation:"));
        switch (op->operation) {
        case DispatchInfo::NAME_LOST:
            QCC_DbgPrintf(("    Process local bus name lost"));
            DeferredNameLostHander(static_cast<NameLostDispatchInfo*>(op)->name);
            break;

        case DispatchInfo::BT_DEVICE_AVAILABLE:
            QCC_DbgPrintf(("    BT device available"));
            DeferredBLEDeviceAvailable(static_cast<BLEDevAvailDispatchInfo*>(op)->on);
            break;
        }
    }

    delete op;
}

}
