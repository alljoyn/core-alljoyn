/**
 * @file
 *
 * BusController is responsible for responding to standard DBus and Bus
 * specific messages directed at the bus itself.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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

#ifndef _ALLJOYN_BUSCONTROLLER_H
#define _ALLJOYN_BUSCONTROLLER_H

#include <qcc/platform.h>

#include <alljoyn/MsgArg.h>

#include "Bus.h"
#include "DBusObj.h"
#include "AllJoynObj.h"
#include "AllJoynDebugObj.h"
#include "SessionlessObj.h"
#include "ProtectedAuthListener.h"

namespace ajn {

/**
 * BusController is responsible for responding to DBus and AllJoyn
 * specific messages directed at the bus itself.
 */
class BusController {
  public:

    /**
     * Constructor
     *
     * @param bus           Bus to associate with this controller.
     * @param authListener  Optional authentication listener
     */
    BusController(Bus& bus, AuthListener* authListener = NULL);

    /**
     * Destructor
     */
    virtual ~BusController();

    /**
     * Initialize the bus controller and start the bus
     *
     * @param listeSpecs  The listen specs for the bus.
     *
     * @return  Returns ER_OK if controller was successfully initialized.
     */
    QStatus Init(const qcc::String& listenSpecs);

    /**
     * Stop the bus controller.
     *
     * @return ER_OK if successful.
     */
    QStatus Stop();

    /**
     * Join the bus controller.
     *
     * @return ER_OK if successful.
     */
    QStatus Join();

    /**
     * Return the daemon bus object responsible for org.alljoyn.Bus.
     *
     * @return The AllJoynObj.
     */
    AllJoynObj& GetAllJoynObj() { return alljoynObj; }

    /**
     * Return the bus associated with this bus controller
     *
     * @return Return the bus
     */
    Bus& GetBus() { return bus; }

    /**
     * ObjectRegistered callback.
     *
     * @param obj   BusObject that has been registered
     */
    void ObjectRegistered(BusObject* obj);

    /**
     * Attempt to start a service to handle the message received.
     *
     * @param msg       The message received.
     * @param sendingEP The endpoint the message was received on
     */
    QStatus StartService(Message& msg, BusEndpoint sendingEP) {
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Get the sessionlessObj.
     *
     * @return   The sessionlessObj singleton
     */
    SessionlessObj& GetSessionlessObj() {
        return sessionlessObj;
    }

    /**
     * Send a sessionless message to the SessionlessObj.
     *
     * @param msg  Sessionless message to be pushed.
     * @return     ER_OK if successful
     */
    QStatus PushSessionlessMessage(Message& msg) {
        return sessionlessObj.PushMessage(msg);
    }

    /**
     * Add a rule for an endpoint.
     *
     * @param epName   The name of the endpoint that this rule applies to.
     * @param rule     Rule for endpoint
     */
    void AddRule(const qcc::String& epName, Rule& rule) {
        sessionlessObj.AddRule(epName, rule);
    }

    /**
     * Remove a rule for an endpoint.
     *
     * @param epName      The name of the endpoint that rule applies to.
     * @param rule        Rule to remove.
     */
    void RemoveRule(const qcc::String& epName, Rule& rule) {
        sessionlessObj.RemoveRule(epName, rule);
    }

    /**
     * Get the auth listener for this bus controller
     */
    AuthListener* GetAuthListener() { return authListener; }

  private:

    Bus& bus;

    /** Listener for authenticating clients */
    AuthListener* authListener;

    /** Bus object responsible for org.freedesktop.DBus */
    DBusObj dbusObj;

    /** Bus object responsible for org.alljoyn.Bus */
    AllJoynObj alljoynObj;

    /** Bus object responsible for org.alljoyn.Sessionless */
    SessionlessObj sessionlessObj;

#ifndef NDEBUG
    /** Bus object responsible for org.alljoyn.Debug */
    debug::AllJoynDebugObj alljoynDebugObj;
#endif

    /** Event to wait on while initialization completes */
    bool initComplete;
};

}

#endif
