/*
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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
 */

#include "org_alljoyn_bus_samples_simpleclient_Client.h"
#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusListener.h>
#include <qcc/Log.h>
#include <qcc/String.h>
#include <new>
#include <android/log.h>
#include <assert.h>

#define LOG_TAG  "SimpleClient"

/* Missing (from NDK) log macros (cutils/log.h) */
#ifndef LOGD
#define LOGD(...) (__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGI
#define LOGI(...) (__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGE
#define LOGE(...) (__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

using namespace ajn;

/* constants */
static const char* SIMPLE_SERVICE_INTERFACE_NAME = "org.alljoyn.bus.samples.simple";
static const char* SIMPLE_SERVICE_WELL_KNOWN_NAME_PREFIX = "org.alljoyn.bus.samples.simple.";
static const char* SIMPLE_SERVICE_OBJECT_PATH = "/simpleService";
static const int SESSION_PORT = 33;

/* forward decl */
class MyBusListener;

/* Static data */
static BusAttachment* s_bus = NULL;
static MyBusListener* s_busListener = NULL;

/* Local types */
class MyBusListener : public BusListener, public SessionPortListener, public SessionListener {
  public:
    MyBusListener(JavaVM* vm, jobject& jobj) : vm(vm), jobj(jobj) { }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        LOGD("\n\nENTERED FoundName with name=%s ", name);
        size_t prefixLen = strlen(SIMPLE_SERVICE_WELL_KNOWN_NAME_PREFIX);
        if (0 == strncmp(SIMPLE_SERVICE_WELL_KNOWN_NAME_PREFIX, name, prefixLen)) {
            /* Found a name that matches service prefix. Inform Java GUI of this name */
            JNIEnv* env;
            jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
            if (JNI_EDETACHED == jret) {
                vm->AttachCurrentThread(&env, NULL);
            }
            jclass jcls = env->GetObjectClass(jobj);
            jmethodID mid = env->GetMethodID(jcls, "FoundNameCallback", "(Ljava/lang/String;)V");
            if (mid == 0) {
                LOGE("Failed to get Java FoundNameCallback");
            } else {
                jstring jName = env->NewStringUTF(name + prefixLen);
                LOGE("Calling FoundNameCallback");
                env->CallVoidMethod(jobj, mid, jName);
                env->DeleteLocalRef(jName);
            }
            if (JNI_EDETACHED == jret) {
                vm->DetachCurrentThread();
            }
        }
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        LOGD("\n\nENTERED LostAdvertisedName with name=%s ", name);
        size_t prefixLen = strlen(SIMPLE_SERVICE_WELL_KNOWN_NAME_PREFIX);
        if (0 == strncmp(SIMPLE_SERVICE_WELL_KNOWN_NAME_PREFIX, name, prefixLen)) {
            /* Lost a name that matches service prefix. Inform Java GUI of this name */
            JNIEnv* env;
            jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
            if (JNI_EDETACHED == jret) {
                vm->AttachCurrentThread(&env, NULL);
            }
            jclass jcls = env->GetObjectClass(jobj);
            jmethodID mid = env->GetMethodID(jcls, "LostNameCallback", "(Ljava/lang/String;)V");
            if (mid == 0) {
                LOGE("Failed to get Java LostNameCallback");
            } else {
                jstring jName = env->NewStringUTF(name + prefixLen);
                LOGE("Calling LostNameCallback");
                env->CallVoidMethod(jobj, mid, jName);
                env->DeleteLocalRef(jName);
            }
            if (JNI_EDETACHED == jret) {
                vm->DetachCurrentThread();
            }
        }
    }

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        LOGD("\n NameOwnerchaged received ... busName = %s .... previousOwner = %s, newOwner = %s", busName, previousOwner, newOwner);
    }

    void SessionLost(SessionId sessionId, SessionLostReason reason)
    {
        LOGD("SessionLost(%u) received. Reason = %u.", sessionId, reason);

        /* Inform Java GUI of this disconnect */
        JNIEnv* env;
        jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
        if (JNI_EDETACHED == jret) {
            vm->AttachCurrentThread(&env, NULL);
        }

        jclass jcls = env->GetObjectClass(jobj);
        jmethodID mid = env->GetMethodID(jcls, "DisconnectCallback", "(II)V");
        if (mid == 0) {
            LOGE("Failed to get Java DisconnectCallback");
        } else {
            env->CallVoidMethod(jobj, mid, jint(sessionId), jint(reason));
        }

        if (JNI_EDETACHED == jret) {
            vm->DetachCurrentThread();
        }
    }

  private:
    JavaVM* vm;
    jobject jobj;
};


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize AllJoyn and connect to local daemon.
 */
JNIEXPORT jint JNICALL Java_org_alljoyn_bus_samples_simpleclient_Client_simpleOnCreate(JNIEnv* env,
                                                                                       jobject jobj,
                                                                                       jstring packageNameStrObj)
{
    QStatus status = ER_OK;

    /* Set AllJoyn logging */
    // QCC_SetLogLevels("ALLJOYN=7;ALL=1");
    QCC_UseOSLogging(true);

    jobject gjobj = env->NewGlobalRef(jobj);

    jboolean iscopy;
    const char* packageNameStr = env->GetStringUTFChars(packageNameStrObj, &iscopy);

    /* Create message bus */
    s_bus = new BusAttachment(packageNameStr, true);

    /* Add org.alljoyn.bus.samples.simple interface */
    InterfaceDescription* testIntf = NULL;
    status = s_bus->CreateInterface(SIMPLE_SERVICE_INTERFACE_NAME, testIntf);
    if (ER_OK == status) {
        testIntf->AddMethod("Ping", "s",  "s", "outStr, inStr", 0);
        testIntf->Activate();
    } else {
        LOGE("Failed to create interface \"%s\" (%s)", SIMPLE_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
    }


    /* Start the msg bus */
    if (ER_OK == status) {
        status = s_bus->Start();
        if (ER_OK != status) {
            LOGE("BusAttachment::Start failed (%s)", QCC_StatusText(status));
        }
    }

    LOGD("\n Registering BUS Listener\n");
    /* Install discovery and name changed callbacks */
    if (ER_OK == status) {
        JavaVM* vm;
        env->GetJavaVM(&vm);
        s_busListener = new MyBusListener(vm, gjobj);
        s_bus->RegisterBusListener(*s_busListener);
    }

    LOGD("\n Connecting to Daemon \n");
    /* Connect to the daemon */
    if (ER_OK == status) {
        status = s_bus->Connect();
        if (ER_OK != status) {
            LOGE("BusAttachment::Connect(\"%s\") failed (%s)", s_bus->GetConnectSpec().c_str(), QCC_StatusText(status));
        }
    }
    LOGD("\n Looking for ADVERTISED name \n");
    /* Find an advertised name with the Prefix */
    status = s_bus->FindAdvertisedName(SIMPLE_SERVICE_WELL_KNOWN_NAME_PREFIX);
    if (ER_OK != status) {
        LOGE("Error while calling FindAdvertisedName \n");
    }

    env->ReleaseStringUTFChars(packageNameStrObj, packageNameStr);

    return jint(status);
}

/**
 * Request the local AllJoyn daemon to connect to a remote daemon.
 */
JNIEXPORT jint JNICALL Java_org_alljoyn_bus_samples_simpleclient_Client_joinSession(JNIEnv* env,
                                                                                    jobject jobj,
                                                                                    jstring jBusName)
{
    SessionId sessionId = 0;
    jboolean iscopy;
    if (NULL == s_bus) {
        return jboolean(false);
    }

    /* Join the conversation */
    const char* busName = env->GetStringUTFChars(jBusName, &iscopy);
    qcc::String nameStr(SIMPLE_SERVICE_WELL_KNOWN_NAME_PREFIX);
    nameStr.append(busName);

    LOGD("\n Joining session with name : %s\n\n", nameStr.c_str());

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = s_bus->JoinSession(nameStr.c_str(), SESSION_PORT, s_busListener, sessionId, opts);
    if ((ER_OK == status)) {
        LOGD("Joined conversation : %s \n \n Session ID : %u", nameStr.c_str(), sessionId);
    } else {
        LOGD("JoinSession failed .. status=%s\n", QCC_StatusText(status));
    }

    return jint(sessionId);
}

/**
 * Request the local daemon to disconnect from the remote daemon.
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_samples_simpleclient_Client_leaveSession(JNIEnv* env,
                                                                                         jobject jobj,
                                                                                         jint jSessionId)
{
    if (NULL == s_bus) {
        return jboolean(false);
    }

    /* Leave the session */
    SessionId sessionId = (SessionId) jSessionId;
    QStatus status = s_bus->LeaveSession(sessionId);
    if (ER_OK != status) {
        LOGE("LeaveSession(%u) failed (%s)", sessionId, QCC_StatusText(status));
    }
    return jboolean(ER_OK == status);
}

/**
 * Called when SimpleClient Java application exits. Performs AllJoyn cleanup.
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_samples_simpleclient_Client_simpleOnDestroy(JNIEnv* env, jobject jobj)
{
    /* Deallocate bus */
    if (NULL != s_bus) {
        delete s_bus;
        s_bus = NULL;
    }

    if (NULL != s_busListener) {
        delete s_busListener;
        s_busListener = NULL;
    }
}


/**
 * Invoke the remote method org.alljoyn.bus.samples.simple.Ping on the /simpleService object
 * located within the bus attachment named org.alljoyn.bus.samples.simple.
 */
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_samples_simpleclient_Client_simplePing(JNIEnv* env,
                                                                                      jobject jobj,
                                                                                      jint jSessionId,
                                                                                      jstring jNamePrefix,
                                                                                      jstring jPingStr)
{
    LOGD("\n Calling ping now ..... \n");
    /* Call the remote method */
    jboolean iscopy;
    const char* pingStr = env->GetStringUTFChars(jPingStr, &iscopy);
    const char* namePrefix = env->GetStringUTFChars(jNamePrefix, &iscopy);
    SessionId sessionId = (SessionId) jSessionId;
    Message reply(*s_bus);
    MsgArg pingStrArg("s", pingStr);

    qcc::String nameStr(SIMPLE_SERVICE_WELL_KNOWN_NAME_PREFIX);
    nameStr.append(namePrefix);

    LOGD("\n Service Name : %s   \n Object Path : %s \n Session ID : %u ", nameStr.c_str(), SIMPLE_SERVICE_OBJECT_PATH, sessionId);

    ProxyBusObject remoteObj = ProxyBusObject(*s_bus, nameStr.c_str(), SIMPLE_SERVICE_OBJECT_PATH, sessionId);
    QStatus status = remoteObj.AddInterface(SIMPLE_SERVICE_INTERFACE_NAME);
    if (ER_OK == status) {
        status = remoteObj.MethodCall(SIMPLE_SERVICE_INTERFACE_NAME, "Ping", &pingStrArg, 1, reply, 5000);
        if (ER_OK == status) {
            LOGI("%s.%s (path=%s) returned \"%s\"", nameStr.c_str(), "Ping", SIMPLE_SERVICE_OBJECT_PATH, reply->GetArg(0)->v_string.str);
        } else {
            LOGE("MethodCall on %s.%s failed (%s)", nameStr.c_str(), "Ping", QCC_StatusText(status));
        }
    } else {
        LOGE("Failed to add interface %s to remote bus obj", SIMPLE_SERVICE_INTERFACE_NAME);
    }

    if (ER_OK == status) {
        LOGI("Ping reply is: \"%s\"", reply->GetArg(0)->v_string.str);
    }
    env->ReleaseStringUTFChars(jPingStr, pingStr);
    return env->NewStringUTF((ER_OK == status) ? reply->GetArg(0)->v_string.str : "");

}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm,
                                  void* reserved)
{
    QCC_UseOSLogging(true);
    return JNI_VERSION_1_2;
}

#ifdef __cplusplus
}

#endif
