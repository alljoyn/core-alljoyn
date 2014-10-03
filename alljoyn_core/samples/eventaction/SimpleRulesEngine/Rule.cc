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

#define DELETE_IF(x) { if (x) { delete x; x = NULL; } }

Rule::~Rule() {
    mLock.Lock(MUTEX_CONTEXT);
    DELETE_IF(actionObject);
    DELETE_IF(mAction);
    DELETE_IF(mEvent);
    if (eventMember) {
        eventMember = NULL;
    }
    mLock.Unlock(MUTEX_CONTEXT);
}

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

    mLock.Lock(MUTEX_CONTEXT);

    const InterfaceDescription* existingIntf = mBus->GetInterface(mEvent->mIfaceName.c_str());
    if (existingIntf) {
        eventMember = existingIntf->GetMember(mEvent->mMember.c_str());
    } else {
        //introspect to collect interface
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        mBus->EnableConcurrentCallbacks();
        status = mBus->JoinSession(mEvent->mUniqueName.c_str(), mEvent->mPort, this,  mSessionId, opts);
        if ((ER_OK == status || ER_ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED == status) && !actionObject) {
            LOGTHIS("Init: Creating ProxyBusObject with SessionId: %d", mSessionId);
            ProxyBusObject* tempObj = new ProxyBusObject(*mBus, mEvent->mUniqueName.c_str(), mEvent->mPath.c_str(), mSessionId);
            if (tempObj != NULL) {
                status = tempObj->IntrospectRemoteObject();
                LOGTHIS("Init:Introspect Object called, status(%d)", status);
            } else {
                LOGTHIS("Init:Failed to create ProxyBusObject");
            }
            DELETE_IF(tempObj);
        }
        if (ER_OK == status) {
            status = mBus->LeaveSession(mSessionId);
        }
        const InterfaceDescription* existingIntfAgain = mBus->GetInterface(mEvent->mIfaceName.c_str());
        if (existingIntfAgain) {
            eventMember = existingIntfAgain->GetMember(mEvent->mMember.c_str());
        }
    }

    mLock.Unlock(MUTEX_CONTEXT);

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
    QStatus status = ER_OK;
    if (!mAction || !mEvent) {
        return;
    }
    LOGTHIS("Received the event (%s) from %s", mEvent->mMember.c_str(), mEvent->mUniqueName.c_str());
    if (mEvent->mUniqueName.compare(msg->GetSender())) {
        LOGTHIS("Ignore since not the sender we are interested in");
        return;
    }
    mLock.Lock(MUTEX_CONTEXT);
    if (mSessionId == 0) {
        LOGTHIS("Going to join session/port %s/%d", mAction->mUniqueName.c_str(), mAction->mPort);
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        mBus->EnableConcurrentCallbacks();
        status = mBus->JoinSession(mAction->mUniqueName.c_str(), mAction->mPort, this,  mSessionId, opts);
        LOGTHIS("JoinSession status: %s(%x)", QCC_StatusText(status), status);
    }
    if ((ER_OK == status || ER_ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED == status) && !actionObject) {
        LOGTHIS("Creating ProxyBusObject with SessionId: %d", mSessionId);
        actionObject = new ProxyBusObject(*mBus, mAction->mUniqueName.c_str(), mAction->mPath.c_str(), mSessionId);
        const InterfaceDescription* actionIntf = mBus->GetInterface(mAction->mIfaceName.c_str());
        if (actionIntf) {
            actionObject->AddInterface(*actionIntf);
        } else {
            //Only introspect if we do not have the interface for the Action
            mBus->EnableConcurrentCallbacks();
            status = actionObject->IntrospectRemoteObject();
            LOGTHIS("Introspect Object called, %s(%x)", QCC_StatusText(status), status);
        }
    }
    mLock.Unlock(MUTEX_CONTEXT);
    //Have a session so call the method to execute the action
    LOGTHIS("Calling action %s/%d/%d", mAction->mUniqueName.c_str(), mAction->mPort, mSessionId);
    callAction();
}

void Rule::callAction() {
    QStatus status = ER_OK;
    if (actionObject) {
        LOGTHIS("Calling device(%s) action %s::%s(%s)",
                mAction->mUniqueName.c_str(), mAction->mIfaceName.c_str(),
                mAction->mMember.c_str(), mAction->mSignature.c_str());
        status = actionObject->MethodCallAsync(mAction->mIfaceName.c_str(), mAction->mMember.c_str(),
                                               this, static_cast<MessageReceiver::ReplyHandler>(&Rule::AsyncCallReplyHandler),
                                               NULL, 0, NULL, 10000);
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
    mLock.Lock(MUTEX_CONTEXT);
    if (eventMember) {
        status = mBus->UnregisterSignalHandler(this,
                                               static_cast<MessageReceiver::SignalHandler>(&Rule::EventHandler),
                                               eventMember,
                                               NULL);
        LOGTHIS("Unregister Signal Handler status: %d", status);
        if (status == ER_OK) {
            status = mBus->RemoveMatch(matchRule.c_str());
            LOGTHIS("Removed match status: %d", status);
        }
        if (mSessionId != 0) {
            status = mBus->LeaveSession(mSessionId);
            LOGTHIS("Leave Session status: %d", status);
            mSessionId = 0;
        }
    }
    mLock.Unlock(MUTEX_CONTEXT);
    eventMember = NULL;
    DELETE_IF(actionObject);

    LOGTHIS("Unregistered a rule for the event: %s to to invoke action %s(%s)",
            matchRule.c_str(), mAction->mMember.c_str(), mAction->mSignature.c_str());

    return status;
}

void Rule::modifyEventSessionName(const char*sessionName)
{
    mBus->EnableConcurrentCallbacks();
    disable();
    mEvent->mUniqueName = sessionName;
    enable();
}

void Rule::modifyActionSessionName(const char*sessionName)
{
    mBus->EnableConcurrentCallbacks();
    disable();
    mAction->mUniqueName = sessionName;
    enable();
}


/* From SessionListener */
void Rule::SessionLost(ajn::SessionId sessionId)
{
    LOGTHIS("Unable to communicate with action device, lost the session.");
    mSessionId = 0;
}

void Rule::SessionMemberAdded(ajn::SessionId sessionId, const char*uniqueName)
{

}

void Rule::SessionMemberRemoved(ajn::SessionId sessionId, const char*uniqueName)
{

}
