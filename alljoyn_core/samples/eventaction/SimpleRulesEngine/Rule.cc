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

#include "Rule.h"

using namespace std;
using namespace ajn;
using namespace qcc;

#ifndef LOGTHIS
#if TARGET_ANDROID
#include <jni.h>
#include <android/log.h>

#define LOG_TAG  "JNI_EventActionBrowser"
#define LOGTHIS(...) (__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#else
#define LOGTHIS(...) printf(__VA_ARGS__)
#endif
#endif

QStatus Rule::enable()
{
    QStatus status = ER_OK;

    if (eventMember != NULL) {  //enabled called a second time
        return status;
    }

    const InterfaceDescription* theIntf = mBus->GetInterface(mEvent->mIfaceName.c_str());
    if (!theIntf) {
        InterfaceDescription* tempIntf;
        status = mBus->CreateInterface(mEvent->mIfaceName.c_str(), tempIntf);
        tempIntf->AddSignal(mEvent->mMember.c_str(), mEvent->mSignature.c_str(), mEvent->mSignature.c_str(), 0);
        tempIntf->Activate();
    }
    theIntf = mBus->GetInterface(mEvent->mIfaceName.c_str());
    eventMember = theIntf->GetMember(mEvent->mMember.c_str());

    status =  mBus->RegisterSignalHandler(this,
                                          static_cast<MessageReceiver::SignalHandler>(&Rule::EventHandler),
                                          eventMember,
                                          NULL);

    qcc::String matchRule = "type='signal',interface='";
    matchRule.append(mEvent->mIfaceName);
    matchRule.append("',member='");
    matchRule.append(mEvent->mMember);
    matchRule.append("'");
    status = mBus->AddMatch(matchRule.c_str());

    LOGTHIS("Registered a rule for the event: %s to to invoke action %s(%s)",
            matchRule.c_str(), mAction->mMember.c_str(), mAction->mSignature.c_str());

    return status;
}

void Rule::EventHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
{
    if (mEvent->mUniqueName.compare(msg->GetSender())) {
        return;
    }
    LOGTHIS("Received the event (%s) from %s", mEvent->mMember.c_str(), mEvent->mUniqueName.c_str());
    if (mSessionId == 0) {
        LOGTHIS("Going to join async session/port %s/%d", mAction->mUniqueName.c_str(), mAction->mPort);
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        QStatus status = mBus->JoinSessionAsync(mAction->mUniqueName.c_str(),
                                                mAction->mPort, this, opts, this, this);
    } else {
        //Have a session so call the method to execute the action
        LOGTHIS("Already in session %s/%d", mAction->mUniqueName.c_str(), mAction->mPort);
        callAction();
    }
}

void Rule::JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context) {
    Rule* rule = (Rule*)context;
    LOGTHIS("Joined session %s/%d", rule->mAction->mUniqueName.c_str(), sessionId);
    rule->mSessionId = sessionId;
    rule->callAction();
}

void Rule::callAction() {
    QStatus status = ER_OK;
    if (!actionObject.IsValid()) {
        actionObject = ProxyBusObject(*mBus, mAction->mUniqueName.c_str(), mAction->mPath.c_str(), mSessionId);
        const InterfaceDescription* theIntf = mBus->GetInterface(mAction->mIfaceName.c_str());
        if (!theIntf) {
            InterfaceDescription* tempIntf;
            status = mBus->CreateInterface(mAction->mIfaceName.c_str(), tempIntf);
            tempIntf->AddMethod(mAction->mMember.c_str(), mAction->mSignature.c_str(), "", "", 0);
            tempIntf->Activate();
        }

        status = actionObject.AddInterface(mAction->mIfaceName.c_str());
    }
    if (ER_OK == status) {
        MsgArg args;
        LOGTHIS("Calling device(%s) action %s::%s(%s)",
                mAction->mUniqueName.c_str(), mAction->mIfaceName.c_str(),
                mAction->mMember.c_str(), mAction->mSignature.c_str());
        //TODO: set args to action->mSignature & fill in dummy values if they exist
        status = actionObject.MethodCall(mAction->mIfaceName.c_str(), mAction->mMember.c_str(), &args, 0);
        LOGTHIS("MethodCall status: %s(%x)", QCC_StatusText(status), status);
    }
}

QStatus Rule::disable()
{
    QStatus status = ER_OK;
    const InterfaceDescription* theIntf = mBus->GetInterface(mEvent->mIfaceName.c_str());
    eventMember = theIntf->GetMember(mEvent->mMember.c_str());
//	if(eventMember) {
//		status = mBus->UnregisterSignalHandler(this,
//						static_cast<MessageReceiver::SignalHandler>(&Rule::EventHandler),
//						eventMember,
//						NULL);
//	}
    eventMember = NULL;

    return status;
}

/* From SessionListener */
void Rule::SessionLost(ajn::SessionId sessionId)
{

}

void Rule::SessionMemberAdded(ajn::SessionId sessionId, const char*uniqueName)
{

}

void Rule::SessionMemberRemoved(ajn::SessionId sessionId, const char*uniqueName)
{

}
