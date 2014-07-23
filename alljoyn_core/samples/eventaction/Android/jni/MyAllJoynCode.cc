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

#include "MyAllJoynCode.h"

using namespace std;
using namespace ajn;
using namespace qcc;
using namespace services;

void MyAllJoynCode::initialize(const char* packageName) {
    QStatus status = ER_OK;

    /* Initialize AllJoyn only once */
    if (!mBusAttachment) {
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

        /* Setup sample rule engine interface */
        InterfaceDescription* ruleEngineIntf = NULL;
        if (!mBusAttachment->GetInterface("org.allseen.sample.rule.engine")) {
            status = mBusAttachment->CreateInterface("org.allseen.sample.rule.engine", ruleEngineIntf);

            /* Add BusMethods */
            ruleEngineIntf->AddMethod("addRule", "(sssssssq)(sssssssq)b", "", "event,action,persist", 0);
            ruleEngineIntf->AddMethod("deleteAllRules", "", "", "", 0);

            if (ER_OK == status) {
                ruleEngineIntf->Activate();
                LOGTHIS("Created and activated the ruleEngine Interface");
            }
        }

        /* Add the match so we receive sessionless signals */
        status = mBusAttachment->AddMatch("sessionless='t'");

        if (ER_OK != status) {
            LOGTHIS("Failed to addMatch for sessionless signals: %s\n", QCC_StatusText(status));
        }
    }
}

void MyAllJoynCode::startRuleEngine() {
    QStatus status;
    /* Startup the rule engine */
    status = ruleEngine.initialize("simple", mBusAttachment);
    if (ER_OK != status) {
        LOGTHIS("Failed to start rule engine");
    }
}

void MyAllJoynCode::Announce(unsigned short version, unsigned short port, const char* busName,
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
            }
            LOGTHIS("(Announce handler) aboutData (key, val) (%s, %s)", key.c_str(), value.v_string.str);
        }
    }
    LOGTHIS("Friendly Name: %s", friendlyName);
    for (AboutClient::ObjectDescriptions::const_iterator it = objectDescs.begin(); it != objectDescs.end(); ++it) {
        qcc::String key = it->first;
        std::vector<qcc::String> vector = it->second;
        for (std::vector<qcc::String>::const_iterator itv = vector.begin(); itv != vector.end(); ++itv) {
            if (itv->compare("org.allseen.sample.rule.engine") == 0) {
                //Save data in a table so we can look it up to join

                JNIEnv* env;
                jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
                if (JNI_EDETACHED == jret) {
                    vm->AttachCurrentThread(&env, NULL);
                }

                jclass jcls = env->GetObjectClass(jobj);
                jmethodID mid = env->GetMethodID(jcls, "foundRuleEngineApplication", "(Ljava/lang/String;Ljava/lang/String;)V");
                if (mid == 0) {
                    LOGTHIS("Failed to get Java foundRuleEngineApplication");
                } else {
                    jstring jName = env->NewStringUTF(busName);
                    jstring jFriendly = env->NewStringUTF(friendlyName);
                    env->CallVoidMethod(jobj, mid, jName, jFriendly);
                    env->DeleteLocalRef(jName);
                    env->DeleteLocalRef(jFriendly);
                }
                if (JNI_EDETACHED == jret) {
                    vm->DetachCurrentThread();
                }
            }
        }
    }
    //TODO: Remove joining session here and move to Application logic
    joinSession(busName, port);
    //pass through to ruleEngine
    ruleEngine.Announce(version, port, busName, objectDescs, aboutData);
}

void MyAllJoynCode::joinSession(const char* sessionName, short port)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = mBusAttachment->JoinSessionAsync(sessionName, port, this, opts, this, ::strdup(sessionName));
    LOGTHIS("JoinSessionAsync status: %d\n", status);
}

void MyAllJoynCode::JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context) {
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
    }
}

char* MyAllJoynCode::introspectWithDescriptions(const char* sessionName, const char* path, int sessionId) {
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
        return NULL;
    }

    //Go ahead and tell AllJoyn to set the interfaces now and save us an Introspection request later
    remoteObj.ParseXml(reply->GetArg(0)->v_string.str);

    return ::strdup(reply->GetArg(0)->v_string.str);
}

void MyAllJoynCode::setEngine(const char* engineName) {
    if (engineName != NULL) {
        //remote engine
        connectedEngineName = ::strdup(engineName);
        joinSession(connectedEngineName, 800);
    } else if (connectedEngineName) {
        leaveSession(mBusSessionMap[connectedEngineName]);
        delete connectedEngineName;
        connectedEngineName = NULL;
    }
}

void MyAllJoynCode::addRule(RuleInfo* event, RuleInfo* action, bool persist)
{
    if (connectedEngineName != NULL) {
        ProxyBusObject remoteObj(*mBusAttachment, connectedEngineName, "/ruleengine", mBusSessionMap[connectedEngineName]);

        const char* ifcName = "org.allseen.sample.rule.engine";

        const InterfaceDescription* theIntf = remoteObj.GetInterface(ifcName);
        if (!theIntf) {
            theIntf = mBusAttachment->GetInterface(ifcName);
            assert(theIntf);
            remoteObj.AddInterface(*theIntf);
        }

        /* Attempt to retrieve introspection from the remote object using sync call */
        const InterfaceDescription::Member* member = theIntf->GetMember("addRule");
        assert(member);

        Message reply(*mBusAttachment);
        MsgArg inputs[3];
        inputs[0].Set("(sssssssq)", event->mUniqueName.c_str(), event->mPath.c_str(),
                      event->mIfaceName.c_str(), event->mMember.c_str(), event->mSignature.c_str(),
                      event->mDeviceId.c_str(), event->mAppId.c_str(), event->mPort);
        inputs[1].Set("(sssssssq)", action->mUniqueName.c_str(), action->mPath.c_str(),
                      action->mIfaceName.c_str(), action->mMember.c_str(), action->mSignature.c_str(),
                      action->mDeviceId.c_str(), action->mAppId.c_str(), action->mPort);
        inputs[2].Set("b", persist);
        QStatus status = remoteObj.MethodCall(*member, inputs, 3, reply);
        if (ER_OK != status) {
            LOGTHIS("Failed to addRule %s", QCC_StatusText(status));
        } else {
            LOGTHIS("Sent addRule method call");
        }
    } else {
        Rule* rule = new Rule(mBusAttachment, event, action);
        ruleEngine.addRule(rule, persist);
    }
}

void MyAllJoynCode::deleteAllRules()
{
    if (connectedEngineName != NULL) {
        ProxyBusObject remoteObj(*mBusAttachment, connectedEngineName, "/ruleengine", mBusSessionMap[connectedEngineName]);
        const char* ifcName = "org.allseen.sample.rule.engine";

        const InterfaceDescription* theIntf = remoteObj.GetInterface(ifcName);
        if (!theIntf) {
            theIntf = mBusAttachment->GetInterface(ifcName);
            assert(theIntf);
            remoteObj.AddInterface(*theIntf);
        }

        /* Attempt to retrieve introspection from the remote object using sync call */
        const InterfaceDescription::Member* member = theIntf->GetMember("deleteAllRules");
        assert(member);

        Message reply(*mBusAttachment);
        QStatus status = remoteObj.MethodCall(*member, NULL, 0, reply);
        if (ER_OK != status) {
            LOGTHIS("Failed to deleteAllRules %s", QCC_StatusText(status));
        } else {
            LOGTHIS("Sent deleteAllRules method call");
        }
    } else {
        ruleEngine.removeAllRules();
    }
}

void MyAllJoynCode::leaveSession(int sessionId)
{
    QStatus status = mBusAttachment->LeaveSession(sessionId);
    if (ER_OK != status) {
        LOGTHIS("LeaveSession failed");
    } else {
        LOGTHIS("LeaveSession successful");
    }
}

void MyAllJoynCode::shutdown()
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

/* From SessionListener */
void MyAllJoynCode::SessionLost(ajn::SessionId sessionId)
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

void MyAllJoynCode::SessionMemberAdded(ajn::SessionId sessionId, const char*uniqueName)
{
    /* Placeholder, not used by this application */
}

void MyAllJoynCode::SessionMemberRemoved(ajn::SessionId sessionId, const char*uniqueName)
{
    /* Placeholder, not used by this application */
}




