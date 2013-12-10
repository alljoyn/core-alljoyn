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

#include "org_alljoyn_bus_samples_chat_Chat.h"
#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <qcc/Log.h>
#include <qcc/String.h>
#include <new>
#include <android/log.h>
#include <assert.h>


#include <qcc/platform.h>

#define LOG_TAG  "AllJoynChat"

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
static const char* CHAT_SERVICE_INTERFACE_NAME = "org.alljoyn.bus.samples.chat";
static const char* CHAT_SERVICE_WELL_KNOWN_NAME = "org.alljoyn.bus.samples.chat";
static const char* CHAT_SERVICE_OBJECT_PATH = "/chatService";
static const char* NAME_PREFIX = "org.alljoyn.bus.samples.chat";
static const int CHAT_PORT = 27;

/* Forward declaration */
class ChatObject;
class MyBusListener;

/* Static data */
static BusAttachment* s_bus = NULL;
static ChatObject* s_chatObj = NULL;
static qcc::String s_advertisedName;
static MyBusListener* s_busListener = NULL;
static SessionId s_sessionId = 0;

class MyBusListener : public BusListener, public SessionPortListener, public SessionListener {
  public:
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        const char* convName = name + strlen(NAME_PREFIX);
        LOGD("Discovered chat conversation: %s \n", name);

        /* Enable Concurrency since JoinSession can block */
        s_bus->EnableConcurrentCallbacks();

        /* Join the conversation */
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        QStatus status = s_bus->JoinSession(name, CHAT_PORT, NULL, s_sessionId, opts);
        if (ER_OK == status) {
            LOGD("Joined conversation \"%s\"\n", name);
        } else {
            LOGD("JoinSession failed status=%s\n", QCC_StatusText(status));
        }
    }

    /* Accept an incoming JoinSession request */
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        if (sessionPort != CHAT_PORT) {
            LOGE("Rejecting join attempt on non-chat session port %d\n", sessionPort);
            return false;
        }

        LOGD("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x)\n",
             joiner, opts.proximity, opts.traffic, opts.transports);


        return true;
    }

    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
    {
        s_sessionId = id;
        LOGD("SessionJoined with %s (id=%u)\n", joiner, id);
    }


    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
    }
};


/* Bus object */
class ChatObject : public BusObject {
  public:

    ChatObject(BusAttachment& bus, const char* path, JavaVM* vm, jobject jobj) : BusObject(bus, path), vm(vm), jobj(jobj)
    {
        QStatus status;

        /* Add the chat interface to this object */
        const InterfaceDescription* chatIntf = bus.GetInterface(CHAT_SERVICE_INTERFACE_NAME);
        assert(chatIntf);
        AddInterface(*chatIntf);

        /* Store the Chat signal member away so it can be quickly looked up when signals are sent */
        chatSignalMember = chatIntf->GetMember("Chat");
        assert(chatSignalMember);

        /* Register signal handler */
        status =  bus.RegisterSignalHandler(this,
                                            static_cast<MessageReceiver::SignalHandler>(&ChatObject::ChatSignalHandler),
                                            chatSignalMember,
                                            NULL);

        if (ER_OK != status) {
            LOGE("Failed to register s_advertisedNamesignal handler for ChatObject::Chat (%s)", QCC_StatusText(status));
        }
    }

    ~ChatObject()
    {
        JNIEnv* env;
        jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
        assert(JNI_OK == jret);
        env->DeleteGlobalRef(jobj);
    }

    /** Send a Chat signal */
    QStatus SendChatSignal(const char* msg)
    {
        MsgArg chatArg("s", msg);
        uint8_t flags = 0;
        return Signal(NULL, s_sessionId, *chatSignalMember, &chatArg, 1, 0, flags);
    }

    /** Receive a signal from another Chat client */
    void ChatSignalHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg)
    {
        /* Inform Java GUI of this message */
        JNIEnv* env;
        jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
        if (JNI_EDETACHED == jret) {
            vm->AttachCurrentThread(&env, NULL);
        }
        jclass jcls = env->GetObjectClass(jobj);
        jmethodID mid = env->GetMethodID(jcls, "ChatCallback", "(Ljava/lang/String;Ljava/lang/String;)V");
        if (mid == 0) {
            LOGE("Failed to get Java ChatCallback");
        } else {

            jstring jSender = env->NewStringUTF(msg->GetSender());
            if (!jSender) {
                LOGE("NewStringUTF failed");
                return;
            }
            jstring jChatStr = env->NewStringUTF(msg->GetArg(0)->v_string.str);
            if (!jChatStr) {
                LOGE("NewStringUTF failed");
                return;
            }
            env->CallVoidMethod(jobj, mid, jSender, jChatStr);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
            }
        }
        if (JNI_EDETACHED == jret) {
            vm->DetachCurrentThread();
        }
    }


    void ObjectRegistered(void) {
        LOGD("\n Object registered \n");
    }

    /** Release the well-known name if it was acquired */
    void ReleaseName() {
        if (s_bus) {
            uint32_t disposition = 0;

            const ProxyBusObject& dbusObj = s_bus->GetDBusProxyObj();
            Message reply(*s_bus);
            MsgArg arg;
            arg.Set("s", s_advertisedName.c_str());
            QStatus status = dbusObj.MethodCall(org::freedesktop::DBus::InterfaceName,
                                                "ReleaseName",
                                                &arg,
                                                1,
                                                reply,
                                                5000);
            if (ER_OK == status) {
                disposition = reply->GetArg(0)->v_uint32;
            }
            if ((ER_OK != status) || (disposition != DBUS_RELEASE_NAME_REPLY_RELEASED)) {
                LOGE("Failed to release name %s (%s, disposition=%d)", s_advertisedName.c_str(), QCC_StatusText(status), disposition);
            }
        }
    }


  private:
    JavaVM* vm;
    jobject jobj;
    const InterfaceDescription::Member* chatSignalMember;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize AllJoyn and connect to local daemon.
 */
JNIEXPORT jint JNICALL Java_org_alljoyn_bus_samples_chat_Chat_jniOnCreate(JNIEnv* env, jobject jobj,
                                                                          jstring packageNameStrObj)
{
    QStatus status = ER_OK;
    const char* packageNameStr = NULL;
    InterfaceDescription* chatIntf = NULL;
    const char* daemonAddr = "unix:abstract=alljoyn";
    JavaVM* vm;
    jobject jglobalObj = NULL;

    packageNameStr = env->GetStringUTFChars(packageNameStrObj, NULL);
    if (!packageNameStr) {
        LOGE("GetStringUTFChars failed");
        status = ER_FAIL;
        goto exit;
    }
    /* Create message bus */
    s_bus = new BusAttachment(packageNameStr, true);
    if (!s_bus) {
        LOGE("new BusAttachment failed");
        status = ER_FAIL;
        goto exit;
    }

    /* Create org.alljoyn.bus.samples.chat interface */
    status = s_bus->CreateInterface(CHAT_SERVICE_INTERFACE_NAME, chatIntf);
    if (ER_OK != status) {
        LOGE("Failed to create interface \"%s\" (%s)", CHAT_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
        goto exit;
    }
    status = chatIntf->AddSignal("Chat", "s",  "str", 0);
    if (ER_OK != status) {
        LOGE("Failed to AddSignal \"Chat\" (%s)", QCC_StatusText(status));
        goto exit;
    }
    chatIntf->Activate();

    /* Start the msg bus */
    status = s_bus->Start();
    if (ER_OK != status) {
        LOGE("BusAttachment::Start failed (%s)", QCC_StatusText(status));
        goto exit;
    }

    /* Connect to the daemon */
    status = s_bus->Connect(daemonAddr);
    if (ER_OK != status) {
        LOGE("BusAttachment::Connect(\"%s\") failed (%s)", daemonAddr, QCC_StatusText(status));
        goto exit;
    }

    /* Create and register the bus object that will be used to send out signals */
    if (0 != env->GetJavaVM(&vm)) {
        LOGE("GetJavaVM failed");
        status = ER_FAIL;
        goto exit;
    }
    jglobalObj = env->NewGlobalRef(jobj);
    if (!jglobalObj) {
        LOGE("NewGlobalRef failed");
        status = ER_FAIL;
        goto exit;
    }
    s_chatObj = new ChatObject(*s_bus, CHAT_SERVICE_OBJECT_PATH, vm, jglobalObj);
    if (!s_chatObj) {
        LOGE("new ChatObject failed");
        status = ER_FAIL;
        goto exit;
    }
    jglobalObj = NULL; /* ChatObject owns global reference now */
    status = s_bus->RegisterBusObject(*s_chatObj);
    if (ER_OK != status) {
        LOGE("BusAttachment::RegisterBusObject() failed (%s)", QCC_StatusText(status));
        goto exit;
    }
    LOGD("\n Bus Object created and registered \n");

    /* Register a bus listener in order to get discovery indications */
    s_busListener = new MyBusListener();
    if (!s_busListener) {
        LOGE("new BusListener failed");
        status = ER_FAIL;
        goto exit;
    }
    s_bus->RegisterBusListener(*s_busListener);

exit:
    if (ER_OK != status) {
        delete s_bus;
        s_bus = NULL;

        delete s_chatObj;
        s_chatObj = NULL;

        delete s_busListener;
        s_busListener = NULL;

        if (jglobalObj) {
            env->DeleteGlobalRef(jglobalObj);
        }
    }
    if (packageNameStr) {
        env->ReleaseStringUTFChars(packageNameStrObj, packageNameStr);
    }
    return (jint) status;
}




JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_samples_chat_Chat_advertise(JNIEnv* env,
                                                                            jobject jobj,
                                                                            jstring advertiseStrObj)
{
    const char* advertisedNameStr = NULL;
    QStatus status;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionPort sp = CHAT_PORT;

    s_advertisedName = "";
    advertisedNameStr = env->GetStringUTFChars(advertiseStrObj, NULL);
    if (!advertisedNameStr) {
        LOGE("GetStringUTFChars failed");
        status = ER_FAIL;
        goto exit;
    }
    s_advertisedName += NAME_PREFIX;
    s_advertisedName += ".";
    s_advertisedName += advertisedNameStr;

    /* Request name */
    status = s_bus->RequestName(s_advertisedName.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (ER_OK != status) {
        LOGE("RequestName(%s) failed (status=%s)\n", s_advertisedName.c_str(), QCC_StatusText(status));
        goto exit;
    }
    LOGD("\n Request Name was successful");

    /* Bind the session port*/
    status = s_bus->BindSessionPort(sp, opts, *s_busListener);
    if (ER_OK != status) {
        LOGE("BindSessionPort failed (%s)\n", QCC_StatusText(status));
        goto exit;
    }
    LOGD("\n Bind Session Port to %u was successful \n", CHAT_PORT);

    /* Advertise the name */
    status = s_bus->AdvertiseName(s_advertisedName.c_str(), opts.transports);
    if (status != ER_OK) {
        LOGD("Failed to advertise name %s (%s) \n", s_advertisedName.c_str(), QCC_StatusText(status));
        goto exit;
    }
    LOGD("\n Name %s was successfully advertised", s_advertisedName.c_str());

exit:
    if (ER_OK != status) {
        s_bus->CancelAdvertiseName(s_advertisedName.c_str(), opts.transports);
        s_bus->UnbindSessionPort(sp);
        s_bus->ReleaseName(s_advertisedName.c_str());
    }
    if (advertisedNameStr) {
        env->ReleaseStringUTFChars(advertiseStrObj, advertisedNameStr);
    }
    return (jboolean) (ER_OK == status);
}

JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_samples_chat_Chat_joinSession(JNIEnv* env,
                                                                              jobject jobj,
                                                                              jstring joinSessionObj)
{
    const char* sessionNameStr = NULL;
    qcc::String sessionName;
    QStatus status = ER_OK;

    LOGD("\n Inside Join session");

    sessionNameStr = env->GetStringUTFChars(joinSessionObj, NULL);
    if (!sessionNameStr) {
        LOGE("GetStringUTFChars failed");
        status = ER_FAIL;
        goto exit;
    }
    sessionName = "";
    sessionName += NAME_PREFIX;
    sessionName += ".";
    sessionName += sessionNameStr;
    LOGD("\n Name of the session to be joined %s ", sessionName.c_str());

    /* Call joinSession method since we have the name */
    status = s_bus->FindAdvertisedName(sessionName.c_str());
    if (ER_OK != status) {
        LOGE("\n Error while calling FindAdvertisedName \n");
        goto exit;
    }

exit:
    if (sessionNameStr) {
        env->ReleaseStringUTFChars(joinSessionObj, sessionNameStr);
    }
    return (jboolean) (ER_OK == status);
}


/**
 * Called when SimpleClient Java application exits. Performs AllJoyn cleanup.
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_samples_chat_Chat_jniOnDestroy(JNIEnv* env, jobject jobj)
{
    /* Deallocate bus */
    delete s_bus;
    s_bus = NULL;

    /* Deregister the ServiceObject. */
    delete s_chatObj;
    s_chatObj = NULL;

    delete s_busListener;
    s_busListener = NULL;
}


/**
 * Send a broadcast ping message to all handlers registered for org.codeauora.alljoyn.samples.chat.Chat signal.
 */
JNIEXPORT jint JNICALL Java_org_alljoyn_bus_samples_chat_Chat_sendChatMsg(JNIEnv* env,
                                                                          jobject jobj,
                                                                          jstring chatMsgObj)
{
    const char* chatMsg = NULL;
    QStatus status = ER_OK;

    /* Send a signal */
    chatMsg = env->GetStringUTFChars(chatMsgObj, NULL);
    if (!chatMsg) {
        LOGE("GetStringUTFChars failed");
        status = ER_FAIL;
        goto exit;
    }

    status = s_chatObj->SendChatSignal(chatMsg);
    if (ER_OK != status) {
        LOGE("Chat", "Sending signal failed (%s)", QCC_StatusText(status));
        goto exit;
    }

exit:
    if (chatMsg) {
        env->ReleaseStringUTFChars(chatMsgObj, chatMsg);
    }
    return (jint) status;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm,
                                  void* reserved)
{
    /* Set AllJoyn logging */
    //QCC_SetLogLevels("ALLJOYN=7;ALL=1");
    QCC_UseOSLogging(true);

    return JNI_VERSION_1_2;
}

#ifdef __cplusplus
}

#endif
