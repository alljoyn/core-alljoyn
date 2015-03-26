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

#include <alljoyn/Status.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/SessionPortListener.h>
#include <alljoyn/SessionListener.h>
#include <qcc/String.h>
#include <stdio.h>
#include <qcc/platform_cpp.h>

#include "../SimpleRuleEngine.h"
#include "OptParser.h"
#include "RuleBusObject.h"

#ifndef _MY_ALLJOYN_CODE_
#define _MY_ALLJOYN_CODE_

class MyAllJoynCode;

class MyAllJoynCode :
    public ajn::AboutListener,
    public ajn::SessionPortListener,
    public ajn::SessionListener {
  public:
    /**
     * Construct a MyAllJoynCode object
     *
     */
    MyAllJoynCode()
        : aboutData(), mBusAttachment(NULL), ruleEngine(), ruleBusObject(NULL)
    { };

    /**
     * Destructor
     */
    ~MyAllJoynCode() {
        shutdown();
    };

    /**
     * Setup AllJoyn, creating the objects needed and registering listeners.
     *
     * @param packageName	This value is provided to the BusAttachment constructor to name the application
     *
     */
    void initialize(const char* packageName);

    /**
     * Free up and release the objects used
     */
    void shutdown();

  private:

    /* From About */
    void Announced(const char* busName, uint16_t version, ajn::SessionPort port, const ajn::MsgArg& objectDescriptionArg, const ajn::MsgArg& aboutDataArg);

    /* From SessionPortListener */
    virtual bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts);

    virtual void SessionJoined(ajn::SessionPort sessionPort, ajn::SessionId id, const char* joiner);

    /* From SessionListener */
    virtual void SessionLost(ajn::SessionId sessionId);

    virtual void SessionMemberAdded(ajn::SessionId sessionId, const char*uniqueName);

    virtual void SessionMemberRemoved(ajn::SessionId sessionId, const char*uniqueName);

    char HexToChar(char c);

    void HexStringToBytes(const qcc::String& hex, uint8_t* outBytes, size_t len);

  private:
    std::map<qcc::String, qcc::String> mBusFriendlyMap;

    ajn::AboutData aboutData;
    ajn::BusAttachment* mBusAttachment;

    SimpleRuleEngine ruleEngine;
    RuleBusObject* ruleBusObject;
};

#endif //_MY_ALLJOYN_CODE_
