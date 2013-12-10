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

#include "org_alljoyn_bus_samples_simpleservice_Service.h"
#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <qcc/Log.h>
#include <new>
//#include <set>
#include <assert.h>
#include <android/log.h>

#define LOG_TAG  "SimpleService"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/* Missing (from NDK) log macros (use to be in cutils/log.h) */
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
static qcc::String wellKnownName;
static qcc::String serviceName;

/* Forward decl */
class ServiceObject;
class MyBusListener;

/* Static data */
static BusAttachment* s_bus = NULL;
static ServiceObject* s_obj = NULL;
static MyBusListener* s_busListener = NULL;


class MyBusListener : public BusListener, public SessionPortListener, public SessionListener {
  public:

    MyBusListener(JavaVM* vm, jobject& jobj) : vm(vm), jobj(jobj), _id(0) { }

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
    }

    /* Accept an incoming JoinSession request */
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        if (sessionPort != SESSION_PORT) {
            LOGE("Rejecting join attempt on non-chat session port %d\n", sessionPort);
            return false;
        }

        LOGD("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x)\n",
             joiner, opts.proximity, opts.traffic, opts.transports);


        return true;
    }

    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
    {
        LOGD("SessionJoined with %s (id=%u)\n", joiner, id);
        _id = id;
    }

    void SessionLost(SessionId id, SessionLostReason reason)
    {
        LOGD("SessionLost (id=%u). Reason = %u.\n", id, reason);
        _id = 0;
    }

  private:
    JavaVM* vm;
    jobject jobj;

  public:
    SessionId _id;
};


/* Bus object */
class ServiceObject : public BusObject {
  public:

    ServiceObject(BusAttachment& bus, const char* path, JavaVM* vm, jobject jobj)
        : BusObject(bus, path), vm(vm), jobj(jobj), isNameAcquired(false)
    {
        QStatus status;

        /* Add the service interface to this object */
        const InterfaceDescription* regTestIntf = bus.GetInterface(SIMPLE_SERVICE_INTERFACE_NAME);
        assert(regTestIntf);
        AddInterface(*regTestIntf);

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { regTestIntf->GetMember("Ping"), static_cast<MessageReceiver::MethodHandler>(&ServiceObject::Ping) }
        };
        status = AddMethodHandlers(methodEntries, ARRAY_SIZE(methodEntries));
        if (ER_OK != status) {
            LOGE("Failed to register method handlers for ServiceObject (%s)", QCC_StatusText(status));
        }
    }

    void ObjectRegistered(void) {
        LOGD("\n Object registered \n\n");
    }


    /** Implement org.alljoyn.bus.samples.simple.Ping by returning the passed in string */
    void Ping(const InterfaceDescription::Member* member, Message& msg)
    {
        const char* pingStr = msg->GetArg(0)->v_string.str;

        LOGD("Pinged from %s with: %s\n", msg->GetSender(), pingStr);

        /* Inform Java GUI of this ping */
        JNIEnv* env;
        jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
        if (JNI_EDETACHED == jret) {
            vm->AttachCurrentThread(&env, NULL);
        }

        jclass jcls = env->GetObjectClass(jobj);
        jmethodID mid = env->GetMethodID(jcls, "PingCallback", "(Ljava/lang/String;Ljava/lang/String;)V");
        if (mid == 0) {
            LOGE("Failed to get Java PingCallback");
        } else {
            jstring jSender = env->NewStringUTF(msg->GetSender());
            jstring jPingStr = env->NewStringUTF(pingStr);
            env->CallVoidMethod(jobj, mid, jSender, jPingStr);
            env->DeleteLocalRef(jSender);
            env->DeleteLocalRef(jPingStr);
        }

        /* Reply with same string that was sent to us */
        MsgArg reply(*(msg->GetArg(0)));
        QStatus status = MethodReply(msg, &reply, 1);
        if (ER_OK != status) {
            LOGE("Ping: Error sending reply (%s)", QCC_StatusText(status));
        }

        if (JNI_EDETACHED == jret) {
            vm->DetachCurrentThread();
        }
    }

  private:
    JavaVM* vm;
    jobject jobj;
    bool isNameAcquired;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     org_alljoyn_bus_samples_simpleservice_Service
 * Method:    simpleOnCreate
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_alljoyn_bus_samples_simpleservice_Service_simpleOnCreate(JNIEnv* env, jobject jobj)
{
    QStatus status = ER_OK;
    /* Set AllJoyn logging */
    // QCC_SetLogLevels("ALLJOYN=7;ALL=1");
    QCC_UseOSLogging(true);

    return ER_OK;
}

/*
 * Class:     org_alljoyn_bus_samples_simpleservice_Service
 * Method:    startService
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_samples_simpleservice_Service_startService(JNIEnv* env, jobject jobj,
                                                                                           jstring jServiceName, jstring packageNameStrObj)
{
    QStatus status = ER_OK;

    jboolean iscopy;
    const char* serviceNameStr = env->GetStringUTFChars(jServiceName, &iscopy);
    serviceName = "";
    serviceName += SIMPLE_SERVICE_WELL_KNOWN_NAME_PREFIX;
    serviceName += serviceNameStr;

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    /* Initialize AllJoyn only once */
    if (!s_bus) {

        jobject gjobj = env->NewGlobalRef(jobj);

        const char* packageNameStr = env->GetStringUTFChars(packageNameStrObj, &iscopy);
        s_bus = new BusAttachment("service", true);

        /* Add org.alljoyn.samples.simple interface */
        InterfaceDescription* testIntf = NULL;
        status = s_bus->CreateInterface(SIMPLE_SERVICE_INTERFACE_NAME, testIntf);
        if (ER_OK == status) {
            testIntf->AddMethod("Ping", "s",  "s", "inStr,outStr", 0);
            testIntf->Activate();
        } else {
            LOGE("Failed to create interface %s (%s)", SIMPLE_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
        }

        /* Start the msg bus */
        if (ER_OK == status) {
            status = s_bus->Start();
        } else {
            LOGE("BusAttachment::Start failed (%s)", QCC_StatusText(status));
        }

        /* Connect to the daemon */
        if (ER_OK == status) {
            status = s_bus->Connect();
            if (ER_OK != status) {
                LOGE("Connect to %s failed (%s)", s_bus->GetConnectSpec().c_str(), QCC_StatusText(status));
            }
        }

        /* Register the bus listener */
        JavaVM* vm;
        env->GetJavaVM(&vm);
        if (ER_OK == status) {
            s_busListener = new MyBusListener(vm, gjobj);
            s_bus->RegisterBusListener(*s_busListener);
            LOGD("\n Bus Listener registered \n");
        }

        /* Register service object */
        s_obj = new ServiceObject(*s_bus, SIMPLE_SERVICE_OBJECT_PATH, vm, gjobj);
        s_bus->RegisterBusObject(*s_obj);

        /* Bind the session port*/
        if (ER_OK == status) {
            SessionPort sp = SESSION_PORT;
            status = s_bus->BindSessionPort(sp, opts, *s_busListener);
            if (ER_OK != status) {
                LOGE("BindSessionPort failed (%s)\n", QCC_StatusText(status));
            } else {
                LOGD("\n Bind Session Port to %d was successful \n", SESSION_PORT);
            }
        }
        env->ReleaseStringUTFChars(packageNameStrObj, packageNameStr);
    }

    /* Request name */
    status = s_bus->RequestName(serviceName.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (ER_OK != status) {
        LOGE("RequestName(%s) failed (status=%s)\n", serviceName.c_str(), QCC_StatusText(status));
        status = (status == ER_OK) ? ER_FAIL : status;
    } else {
        LOGD("\n Request Name was successful");
    }

    /* Advertise the name */
    if (ER_OK == status) {

        status = s_bus->AdvertiseName(serviceName.c_str(), opts.transports);
        if (status != ER_OK) {
            LOGD("Failed to advertise name %s (%s) \n", serviceName.c_str(), QCC_StatusText(status));
        } else {
            LOGD("\n Name %s was successfully advertised", serviceName.c_str());
        }
    }
    env->ReleaseStringUTFChars(jServiceName, serviceNameStr);
    env->DeleteLocalRef(jServiceName);

    return (jboolean) true;
}

/*
 * Class:     org_alljoyn_bus_samples_simpleservice_Service
 * Method:    stopService
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_samples_simpleservice_Service_stopService(JNIEnv* env, jobject jobj, jstring jServiceName)
{
    if (!s_bus) {
        return;
    }

    jboolean isCopy;
    const char* serviceNameStr = env->GetStringUTFChars(jServiceName, &isCopy);
    qcc::String serviceName(SIMPLE_SERVICE_WELL_KNOWN_NAME_PREFIX);
    serviceName += serviceNameStr;


    /* Stop advertising the name */
    LOGD("Canceling advertise name %s", serviceName.c_str());
    QStatus status = s_bus->CancelAdvertiseName(serviceName.c_str(), TRANSPORT_ANY);
    if (status != ER_OK) {
        LOGE("CancelAdvertiseName failed with %s", QCC_StatusText(status));
    }

    /* Release the name */
    status =  s_bus->ReleaseName(serviceName.c_str());
    if (status != ER_OK) {
        LOGE("ReleaseName failed with %s", QCC_StatusText(status));
    }


    if (s_busListener->_id != 0) {
        s_bus->LeaveSession(s_busListener->_id);
        s_busListener->_id = 0;
    }

    env->ReleaseStringUTFChars(jServiceName, serviceNameStr);
    env->DeleteLocalRef(jServiceName);

    return;
}

/*
 * Class:     org_alljoyn_bus_samples_simpleservice_Service
 * Method:    simpleOnDestroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_samples_simpleservice_Service_simpleOnDestroy(JNIEnv* env, jobject jobj)
{
    /* Deallocate bus */
    if (s_bus) {
        delete s_bus;
        s_bus = NULL;
    }

    if (s_busListener) {
        delete s_busListener;
        s_busListener = NULL;
    }

    /* Unregister and deallocate service object */
    if (s_obj) {
        delete s_obj;
        s_obj = NULL;
    }
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
