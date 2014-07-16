/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#ifndef _RULE_
#define _RULE_

#include <alljoyn/Status.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/Session.h>
#include <qcc/String.h>
#include "RuleInfo.h"
#include <qcc/Mutex.h>

class Rule : public ajn::SessionListener, public ajn::MessageReceiver {
  public:
    Rule(ajn::BusAttachment* bus, RuleInfo* event, RuleInfo* action)
        : mBus(bus), mEvent(event), mAction(action), eventMember(NULL), mSessionId(0), actionObject(NULL), mLock()
    { };

    virtual ~Rule();

    QStatus enable();
    QStatus disable();

    qcc::String toString() {
        qcc::String ret = "";
        ret.append(mEvent->toString());
        ret.append("|");
        ret.append(mAction->toString());
        return ret;
    }

    uint8_t actionReady() {
        return mAction->mPort != 0;
    }

    uint8_t eventReady() {
        return mEvent->mPort != 0;
    }

    const char* getActionSessionName() { return mAction->mUniqueName.c_str(); }
    const char* getEventSessionName() { return mEvent->mUniqueName.c_str(); }

    uint8_t isEventMatch(qcc::String deviceId, qcc::String appId) {
        return mEvent->mDeviceId.compare(deviceId) == 0 && mEvent->mAppId.compare(appId) == 0;
    }

    uint8_t isActionMatch(qcc::String deviceId, qcc::String appId) {
        return mAction->mDeviceId.compare(deviceId) == 0 && mAction->mAppId.compare(appId) == 0;
    }

    void setActionPort(short port) { mAction->mPort = port; }
    void setEventPort(short port) { mEvent->mPort = port; }

    void addToEvent(qcc::String deviceId, qcc::String appId) { mEvent->setSenderInfo(deviceId, appId); }
    void addToAction(qcc::String deviceId, qcc::String appId) { mAction->setSenderInfo(deviceId, appId); }

    void modifyEventSessionName(const char*sessionName);
    void modifyActionSessionName(const char*sessionName);


  private:

    /* From SessionListener */
    virtual void SessionLost(ajn::SessionId sessionId);

    virtual void SessionMemberAdded(ajn::SessionId sessionId, const char*uniqueName);

    virtual void SessionMemberRemoved(ajn::SessionId sessionId, const char*uniqueName);

    void EventHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg);

    void callAction();
    void AsyncCallReplyHandler(ajn::Message& msg, void* context);

    const ajn::InterfaceDescription::Member* eventMember;
    ajn::SessionId mSessionId;
    ajn::ProxyBusObject* actionObject;

    ajn::BusAttachment*mBus;

    RuleInfo* mEvent;
    RuleInfo* mAction;
    qcc::Mutex mLock;
};

#endif //_RULE_

