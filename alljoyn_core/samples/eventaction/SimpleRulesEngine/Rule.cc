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
#define LOGTHIS(...) printf(__VA_ARGS__); printf("\n");
#endif
#endif

QStatus Rule::enable()
{
    QStatus status = ER_OK;

    if (eventMember != NULL) {  //enabled called a second time
        return status;
    }

    qcc::String matchRule = "type='signal',interface='";
    matchRule.append(mEvent->mIfaceName);
    matchRule.append("',member='");
    matchRule.append(mEvent->mMember);
    matchRule.append("'");
    LOGTHIS("Going to setup a rule for the event: %s to to invoke action %s(%s)",
            matchRule.c_str(), mAction->mMember.c_str(), mAction->mSignature.c_str());

    InterfaceDescription* tempIntf;
    status = mBus->CreateInterface(mEvent->mIfaceName.c_str(), tempIntf);
    if (status == ER_OK && tempIntf) {
        tempIntf->AddSignal(mEvent->mMember.c_str(), mEvent->mSignature.c_str(), mEvent->mSignature.c_str(), 0);
        eventMember = tempIntf->GetMember(mEvent->mMember.c_str());
    } else if (status == ER_BUS_IFACE_ALREADY_EXISTS) {
        LOGTHIS("Interface already exists, getting it from the BusAttachment");
        const InterfaceDescription* existingIntf = mBus->GetInterface(mEvent->mIfaceName.c_str());
        eventMember = existingIntf->GetMember(mEvent->mMember.c_str());
    }

    if (eventMember) {
        status =  mBus->RegisterSignalHandler(this,
                                              static_cast<MessageReceiver::SignalHandler>(&Rule::EventHandler),
                                              eventMember,
                                              NULL);

        if (status == ER_OK) {
            status = mBus->AddMatch(matchRule.c_str());

            LOGTHIS("Registered a rule for the event: %s to to invoke action %s(%s)",
                    matchRule.c_str(), mAction->mMember.c_str(), mAction->mSignature.c_str());
        } else {
            LOGTHIS("Error registering the signal handler: %s(%d)", QCC_StatusText(status), status);
        }
    } else {
        LOGTHIS("Event Member is null, interface create status %s(%d)", QCC_StatusText(status), status);
    }

    return status;
}

void Rule::EventHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
{
    if (!mAction || !mEvent || mEvent->mUniqueName.compare(msg->GetSender())) {
        return;
    }
    LOGTHIS("Received the event (%s) from %s", mEvent->mMember.c_str(), mEvent->mUniqueName.c_str());
    if (mSessionId == 0) {
        LOGTHIS("Going to join async session/port %s/%d", mAction->mUniqueName.c_str(), mAction->mPort);
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        QStatus status = mBus->JoinSessionAsync(mAction->mUniqueName.c_str(),
                                                mAction->mPort, this, opts, this, this);
        if (ER_OK != status) {
            LOGTHIS("Failed to JoinSession");
        }
    } else {
        //Have a session so call the method to execute the action
        LOGTHIS("Already in session %s/%d/%d", mAction->mUniqueName.c_str(), mAction->mPort, mSessionId);
        callAction();
    }
}

void Rule::JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context) {
    Rule* rule = (Rule*)context;
    LOGTHIS("Joined session %s/%d status: %s(%x)", rule->mAction->mUniqueName.c_str(), sessionId, QCC_StatusText(status), status);
    if (status == ER_OK || status == ER_ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED) {
        rule->mSessionId = sessionId;
        rule->callAction();
    }
}

void Rule::callAction() {
    QStatus status = ER_OK;
    if (!actionObject || !actionObject->ImplementsInterface(mAction->mIfaceName.c_str())) {
        LOGTHIS("Creating ProxyBusObject with SessionId: %d", mSessionId);
        actionObject = new ProxyBusObject(*mBus, mAction->mUniqueName.c_str(), mAction->mPath.c_str(), mSessionId);
        if (actionObject != NULL) {
            mBus->EnableConcurrentCallbacks();
            status = actionObject->IntrospectRemoteObject();
        } else {
            LOGTHIS("Failed to create ProxyBusObject");
        }
    }
    if (ER_OK == status && actionObject) {
        MsgArg args;
        LOGTHIS("Calling device(%s) action %s::%s(%s)",
                mAction->mUniqueName.c_str(), mAction->mIfaceName.c_str(),
                mAction->mMember.c_str(), mAction->mSignature.c_str());
        //TODO: set args to action->mSignature & fill in dummy values if they exist
        status = actionObject->MethodCallAsync(mAction->mIfaceName.c_str(), mAction->mMember.c_str(),
                                               this, static_cast<MessageReceiver::ReplyHandler>(&Rule::AsyncCallReplyHandler),
                                               &args, 0, NULL, 10000);
        LOGTHIS("MethodCall status: %s(%x)", QCC_StatusText(status), status);
    } else {
        LOGTHIS("Failed MethodCall status: %s(%x)", QCC_StatusText(status), status);
    }
}

void Rule::AsyncCallReplyHandler(Message& msg, void* context) {
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);
    if (MESSAGE_METHOD_RET != msg->GetType()) {
        LOGTHIS("Failed MethodCall message return type: %d", msg->GetType());
        LOGTHIS("Failed MethodCall message Error name: %s", msg->GetErrorDescription().c_str());
    } else {
        LOGTHIS("Action should have been executed");
    }
}

QStatus Rule::disable()
{
    QStatus status = ER_OK;
    qcc::String matchRule = "type='signal',interface='";
    matchRule.append(mEvent->mIfaceName);
    matchRule.append("',member='");
    matchRule.append(mEvent->mMember);
    matchRule.append("'");
    if (eventMember) {
        status = mBus->RemoveMatch(matchRule.c_str());
        LOGTHIS("Removed match status: %d", status);
        if (status == ER_OK) {
            status = mBus->UnregisterSignalHandler(this,
                                                   static_cast<MessageReceiver::SignalHandler>(&Rule::EventHandler),
                                                   eventMember,
                                                   NULL);
        }
    }
    eventMember = NULL;

    LOGTHIS("Unregistered a rule for the event: %s to to invoke action %s(%s)",
            matchRule.c_str(), mAction->mMember.c_str(), mAction->mSignature.c_str());

    return status;
}

/* From SessionListener */
void Rule::SessionLost(ajn::SessionId sessionId)
{
    LOGTHIS("Unable to communicate with action device, lost the session.");
    if (actionObject) {
        delete actionObject;
        actionObject = NULL;
    }
    mSessionId = 0;
}

void Rule::SessionMemberAdded(ajn::SessionId sessionId, const char*uniqueName)
{

}

void Rule::SessionMemberRemoved(ajn::SessionId sessionId, const char*uniqueName)
{

}
