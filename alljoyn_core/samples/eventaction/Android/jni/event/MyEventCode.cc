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

#include "MyEventCode.h"

using namespace std;
using namespace ajn;
using namespace qcc;
using namespace services;

void MyEventCode::initialize(const char* packageName) {
    QStatus status = ER_OK;

    /* Initialize AllJoyn only once */
    if (!mBusAttachment) {

        QCC_SetLogLevels("ALLJOYN_OBJ=7");
        QCC_SetLogLevels("ER_DEBUG_ALLJOYN_OBJ=7");
        QCC_SetDebugLevel("MyEventCode", 15);
        QCC_UseOSLogging(true);
        /*
         * All communication through AllJoyn begins with a BusAttachment.
         *
         * A BusAttachment needs a name. The actual name is unimportant except for internal
         * security. As a default we use the class name as the name.
         *
         * By default AllJoyn does not allow communication between devices (i.e. bus to bus
         * communication).  The second argument must be set to Receive to allow
         * communication between devices.
         */
        mBusAttachment = new BusAttachment(packageName, true);

        /* Start the msg bus */
        if (ER_OK == status) {
            status = mBusAttachment->Start();
        } else {
            LOGTHIS("BusAttachment::Start failed");
        }
        /* Connect to the daemon */
        if (ER_OK == status) {
            status = mBusAttachment->Connect();
            if (ER_OK != status) {
                LOGTHIS("BusAttachment Connect failed.");
            }
        }
        LOGTHIS("Created BusAttachment and connected");


        AnnouncementRegistrar::RegisterAnnounceHandler(*mBusAttachment, *this, NULL, 0);

        /* Add the match so we receive sessionless signals */
        status = mBusAttachment->AddMatch("sessionless='t'");

        if (ER_OK != status) {
            LOGTHIS("Failed to addMatch for sessionless signals: %s\n", QCC_StatusText(status));
        }
    }
}

void MyEventCode::Announce(unsigned short version, unsigned short port, const char* busName,
                           const ObjectDescriptions& objectDescs,
                           const AboutData& aboutData)
{
    LOGTHIS("Found about application with busName, port %s, %d", busName, port);
    /* For now lets just assume everything has events and actions and join */
    char* friendlyName = NULL;
    for (AboutClient::AboutData::const_iterator it = aboutData.begin(); it != aboutData.end(); ++it) {
        qcc::String key = it->first;
        ajn::MsgArg value = it->second;
        if (value.typeId == ALLJOYN_STRING) {
            if (key.compare("DeviceName") == 0) {
                friendlyName = ::strdup(value.v_string.str);
                mBusFriendlyMap.insert(std::pair<qcc::String, qcc::String>(busName, friendlyName));
                mBusPortMap.insert(std::pair<qcc::String, short>(busName, port));
                LOGTHIS("Friendly Name: %s (%s)", friendlyName, busName);
                free(friendlyName);
            }
            LOGTHIS("(Announce handler) aboutData (key, val) (%s, %s)", key.c_str(), value.v_string.str);
        }
    }

    joinSession(busName, port);
}

void MyEventCode::joinSession(const char* sessionName, short port)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = mBusAttachment->JoinSessionAsync(sessionName, port, this, opts, this, ::strdup(sessionName));
    LOGTHIS("JoinSessionAsync status: %d\n", status);
}

void MyEventCode::JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context) {
    if (status == ER_OK || status == ER_ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED) {
        //cast context to a char* then check sessionName to save off correct sessionId value
        char* sessionName = (char*)context;
        LOGTHIS("Joined the sesssion %s have sessionId %d\n", sessionName, sessionId);
        mBusSessionMap.insert(std::pair<qcc::String, int>(sessionName, sessionId));

        JNIEnv* env;
        jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
        if (JNI_EDETACHED == jret) {
            vm->AttachCurrentThread(&env, NULL);
        }

        jclass jcls = env->GetObjectClass(jobj);
        jmethodID mid = env->GetMethodID(jcls, "foundEventActionApplication", "(Ljava/lang/String;ILjava/lang/String;)V");
        if (mid == 0) {
            LOGTHIS("Failed to get Java foundEventActionApplication");
        } else {
            jstring jName = env->NewStringUTF(sessionName);
            jstring jFriendly = env->NewStringUTF(mBusFriendlyMap[sessionName].c_str());
            env->CallVoidMethod(jobj, mid, jName, sessionId, jFriendly);
            env->DeleteLocalRef(jName);
            env->DeleteLocalRef(jFriendly);
        }
        if (JNI_EDETACHED == jret) {
            vm->DetachCurrentThread();
        }
    } else {
        LOGTHIS("Error joining %s status %d", (char*)context, status);
        if (status == ER_ALLJOYN_JOINSESSION_REPLY_FAILED) {
            JNIEnv* env;
            jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
            if (JNI_EDETACHED == jret) {
                vm->AttachCurrentThread(&env, NULL);
            }

            jclass jcls = env->GetObjectClass(jobj);
            jmethodID mid = env->GetMethodID(jcls, "failedJoinEventActionApplication", "(Ljava/lang/String;)V");
            if (mid == 0) {
                LOGTHIS("Failed to get Java foundEventEventActionApplication");
            } else {
                jstring jName = env->NewStringUTF((char*)context);
                env->CallVoidMethod(jobj, mid, jName);
                env->DeleteLocalRef(jName);
            }
            if (JNI_EDETACHED == jret) {
                vm->DetachCurrentThread();
            }
        }
    }
}

void MyEventCode::InformFound(char* sessionName, int sessionId, char* friendly)
{
    JNIEnv* env;
    jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
    if (JNI_EDETACHED == jret) {
        vm->AttachCurrentThread(&env, NULL);
    }

    jclass jcls = env->GetObjectClass(jobj);
    jmethodID mid = env->GetMethodID(jcls, "foundEventActionApplication", "(Ljava/lang/String;ILjava/lang/String;)V");
    if (mid == 0) {
        LOGTHIS("Failed to get Java foundEventActionApplication");
    } else {
        jstring jName = env->NewStringUTF(sessionName);
        jstring jFriendly = env->NewStringUTF(friendly);
        env->CallVoidMethod(jobj, mid, jName, sessionId, jFriendly);
        env->DeleteLocalRef(jName);
        env->DeleteLocalRef(jFriendly);
    }
    if (JNI_EDETACHED == jret) {
        vm->DetachCurrentThread();
    }
}

char* MyEventCode::introspectWithDescriptions(const char* sessionName, const char* path, int sessionId) {
    LOGTHIS("introspectWithDescriptions the sesssion %s, path %s, id %d\n", sessionName, path, sessionId);
    ProxyBusObject remoteObj(*mBusAttachment, sessionName, path, sessionId);
    QStatus status = ER_OK;

    const char* ifcName = org::allseen::Introspectable::InterfaceName;

    const InterfaceDescription* introIntf = remoteObj.GetInterface(ifcName);
    if (!introIntf) {
        introIntf = mBusAttachment->GetInterface(ifcName);
        assert(introIntf);
        remoteObj.AddInterface(*introIntf);
    }

    /* Attempt to retrieve introspection from the remote object using sync call */
    Message reply(*mBusAttachment);
    const InterfaceDescription::Member* introMember = introIntf->GetMember("IntrospectWithDescription");
    assert(introMember);

    MsgArg inputs[1];
    inputs[0].Set("s", "en");
    status = remoteObj.MethodCall(*introMember, inputs, 1, reply, 30000);

    /* Parse the XML reply */
    if (status != ER_OK) {
        LOGTHIS("Introspection error: %d", status);
        mBusAttachment->LeaveSession(sessionId);
        return NULL;
    }

    //Go ahead and tell AllJoyn to set the interfaces now and save us an Introspection request later
    remoteObj.ParseXml(reply->GetArg(0)->v_string.str);

    return ::strdup(reply->GetArg(0)->v_string.str);
}

void MyEventCode::callAction(ActionInfo* action)
{
    ajn::ProxyBusObject* actionObject;
    ajn::SessionId mSessionId;
    QStatus status = ER_OK;
    short port = mBusPortMap[action->mUniqueName];

    LOGTHIS("callAction on %s, port %d\n", action->mUniqueName.c_str(), port);
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = mBusAttachment->JoinSession(action->mUniqueName.c_str(), port, this,  mSessionId, opts);
    if ((ER_OK == status || ER_ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED == status)) {
        LOGTHIS("Creating ProxyBusObject with SessionId: %d", mSessionId);
        actionObject = new ProxyBusObject(*mBusAttachment, action->mUniqueName.c_str(), action->mPath.c_str(), mSessionId);
        //actionObject = new ProxyBusObject(*mBusAttachment, action->mUniqueName.c_str(), action->mPath.c_str(), 0);
        const InterfaceDescription* actionIntf = mBusAttachment->GetInterface(action->mIfaceName.c_str());
        if (actionIntf) {
            actionObject->AddInterface(*actionIntf);
        } else {
            status = actionObject->IntrospectRemoteObject();
            LOGTHIS("Introspect Object called, %s(%x)", QCC_StatusText(status), status);
        }
        actionIntf = mBusAttachment->GetInterface(action->mIfaceName.c_str());
        if (actionIntf && actionObject) {
            LOGTHIS("Calling device(%s) action %s::%s(%s)",
                    action->mUniqueName.c_str(), action->mIfaceName.c_str(),
                    action->mMember.c_str(), action->mSignature.c_str());
            //status = actionObject->MethodCall(action->mIfaceName.c_str(), action->mMember.c_str(), NULL, 0, );
            Message reply(*mBusAttachment);
            const InterfaceDescription::Member* methodMember = actionIntf->GetMember(action->mMember.c_str());
            status = actionObject->MethodCall(*methodMember, NULL, 0, reply);
            LOGTHIS("MethodCall status: %s(%x)", QCC_StatusText(status), status);
        } else {
            LOGTHIS("Failed MethodCall status: %s(%x)", QCC_StatusText(status), status);
        }
        mBusAttachment->LeaveSession(mSessionId);
    } else {
        LOGTHIS("Failed to join session status: %s(%x)", QCC_StatusText(status), status);
    }
}

bool MyEventCode::enableEvent(EventInfo* event)
{
    QStatus status = ER_OK;
    const ajn::InterfaceDescription::Member* eventMember = NULL;
    bool ret = false;

    qcc::String matchRule = "type='signal',interface='";
    matchRule.append(event->mIfaceName);
    matchRule.append("',member='");
    matchRule.append(event->mMember);
    matchRule.append("'");

    LOGTHIS("Going to setup a handler for the event: %s",
            matchRule.c_str());

    const InterfaceDescription* existingIntf = mBusAttachment->GetInterface(event->mIfaceName.c_str());
    if (existingIntf) {
        eventMember = existingIntf->GetMember(event->mMember.c_str());
    }

    if (eventMember) {
        status =  mBusAttachment->RegisterSignalHandler(this,
                                                        static_cast<MessageReceiver::SignalHandler>(&MyEventCode::EventHandler),
                                                        eventMember,
                                                        NULL);

        if (status == ER_OK) {
            status = mBusAttachment->AddMatch(matchRule.c_str());
            ret = true;
        } else {
            LOGTHIS("Error registering the signal handler: %s(%d)", QCC_StatusText(status), status);
        }
    } else {
        LOGTHIS("Event Member is null, get interface status %s(%d)", QCC_StatusText(status), status);
    }

    return ret;
}

void MyEventCode::EventHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
{
    JNIEnv* env;
    jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
    if (JNI_EDETACHED == jret) {
        vm->AttachCurrentThread(&env, NULL);
    }

    jclass jcls = env->GetObjectClass(jobj);
    jmethodID mid = env->GetMethodID(jcls, "onEventReceived", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if (mid == 0) {
        LOGTHIS("Failed to get Java onEventReceived");
    } else {
        jstring jFrom = env->NewStringUTF(msg->GetSender());
        jstring jPath = env->NewStringUTF(srcPath);
        jstring jIFace = env->NewStringUTF(member->iface->GetName());
        jstring jMember = env->NewStringUTF(member->name.c_str());
        jstring jSig = env->NewStringUTF(member->signature.c_str());
        env->CallVoidMethod(jobj, mid, jFrom, jPath, jIFace, jMember, jSig);
        env->DeleteLocalRef(jFrom);
        env->DeleteLocalRef(jPath);
        env->DeleteLocalRef(jIFace);
        env->DeleteLocalRef(jMember);
        env->DeleteLocalRef(jSig);
    }
    if (JNI_EDETACHED == jret) {
        vm->DetachCurrentThread();
    }
}
//onEventReceived(String from, String path,
//String iface, String member, String sig)

void MyEventCode::leaveSession(int sessionId)
{
    QStatus status = mBusAttachment->LeaveSession(sessionId);
    if (ER_OK != status) {
        LOGTHIS("LeaveSession failed");
    } else {
        LOGTHIS("LeaveSession successful");
    }
}

void MyEventCode::shutdown()
{
    /* Cancel the advertisement */
    /* Unregister the Bus Listener */
    mBusAttachment->UnregisterBusListener(*((BusListener*)this));
    /* Deallocate the BusAttachment */
    if (mBusAttachment) {
        delete mBusAttachment;
        mBusAttachment = NULL;
    }
}

void MyEventCode::AsyncCallReplyHandler(Message& msg, void* context) {
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

/* From SessionListener */
void MyEventCode::SessionLost(ajn::SessionId sessionId)
{
    JNIEnv* env;
    jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
    if (JNI_EDETACHED == jret) {
        vm->AttachCurrentThread(&env, NULL);
    }

    jclass jcls = env->GetObjectClass(jobj);
    jmethodID mid = env->GetMethodID(jcls, "lostEventActionApplication", "(I)V");
    if (mid == 0) {
        LOGTHIS("Failed to get Java lostEventActionApplication");
    } else {
        env->CallVoidMethod(jobj, mid, sessionId);
    }
    if (JNI_EDETACHED == jret) {
        vm->DetachCurrentThread();
    }
}




