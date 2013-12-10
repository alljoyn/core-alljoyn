/**
 * @file
 * This is the native code that handles the bus communication part of the
 * Android service used for controlling Wifi P2p via the Android framework
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/BusAttachment.h>
#include <qcc/String.h>
#include <android/log.h>
#include <qcc/Log.h>
#include <assert.h>
#include <alljoyn/DBusStd.h>

#include <jni.h>

#define LOG_TAG  "P2pHelperService"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


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

#define ER_GENERAL -1  // @@ TODO  lazy...
#define ER_P2P_NOT_CONNECTED ER_BUS_NOT_CONNECTED

using namespace ajn;
using namespace qcc;

class P2pService;
static const char* P2P_SERVICE_INTERFACE_NAME = "org.alljoyn.bus.p2p.P2pInterface";
static const char* P2P_SERVICE_OBJECT_PATH = "/P2pService";
static const char* P2P_SERVICE_NAME = "org.alljoyn.bus.p2p";

static BusAttachment* s_bus = NULL;
static P2pService* s_obj = NULL;
static bool isStandalone = true;

class P2pService : public BusObject {
  public:
    P2pService(BusAttachment& bus, const char* path, JavaVM* vm, jobject jobj) : BusObject(bus, path), vm(vm), jobj(jobj)
    {
        QStatus status;
        jint jret;

        LOGI("P2pService(): construct");

        //
        // Set up the AllJoyn side of things. First, add the P2p interface to this object
        //
        const InterfaceDescription* p2pIntf = bus.GetInterface(P2P_SERVICE_INTERFACE_NAME);
        assert(p2pIntf);
        AddInterface(*p2pIntf);

        advertiseNameMember = p2pIntf->GetMember("AdvertiseName");
        assert(advertiseNameMember);
        cancelAdvertiseNameMember = p2pIntf->GetMember("CancelAdvertiseName");
        assert(cancelAdvertiseNameMember);
        findAdvertisedNameMember = p2pIntf->GetMember("FindAdvertisedName");
        assert(findAdvertisedNameMember);
        cancelFindAdvertisedNameMember = p2pIntf->GetMember("CancelFindAdvertisedName");
        assert(cancelFindAdvertisedNameMember);
        establishLinkMember = p2pIntf->GetMember("EstablishLink");
        assert(establishLinkMember);
        releaseLinkMember = p2pIntf->GetMember("ReleaseLink");
        assert(releaseLinkMember);
        getInterfaceNameFromHandleMember = p2pIntf->GetMember("GetInterfaceNameFromHandle");
        assert(getInterfaceNameFromHandleMember);

        const MethodEntry methodEntries[] = {
            { p2pIntf->GetMember("AdvertiseName"), static_cast<MessageReceiver::MethodHandler>(&P2pService::handleAdvertiseName) },
            { p2pIntf->GetMember("CancelAdvertiseName"), static_cast<MessageReceiver::MethodHandler>(&P2pService::handleCancelAdvertiseName) },
            { p2pIntf->GetMember("FindAdvertisedName"), static_cast<MessageReceiver::MethodHandler>(&P2pService::handleFindAdvertisedName) },
            { p2pIntf->GetMember("CancelFindAdvertisedName"), static_cast<MessageReceiver::MethodHandler>(&P2pService::handleCancelFindAdvertisedName) },
            { p2pIntf->GetMember("EstablishLink"), static_cast<MessageReceiver::MethodHandler>(&P2pService::handleEstablishLink) },
            { p2pIntf->GetMember("ReleaseLink"), static_cast<MessageReceiver::MethodHandler>(&P2pService::handleReleaseLink) },
            { p2pIntf->GetMember("GetInterfaceNameFromHandle"), static_cast<MessageReceiver::MethodHandler>(&P2pService::handleGetInterfaceNameFromHandle) }
        };

        //
        // Finally, add the AllJoyn method handlers to the BusObject */
        //
        status = AddMethodHandlers(methodEntries, ARRAY_SIZE(methodEntries));
        if (ER_OK != status) {
            LOGE("P2pService(): Failed to register method handlers for P2pService (%s)", QCC_StatusText(status));
        }

        //
        // Now setup the JNI side of things
        //
        JNIEnv* env = attachEnv(&jret);

        //
        // Create a reference to the provided P2pHelperService Java object that remains
        // meaningful in other calls into our various functions.  We create a weak
        // reference so that we don't interfere with garbage collection.  Note well
        // that you can't use weak references directly, you must always create a
        // valid local reference from the weak reference before using it.
        //
        jhelper = env->NewWeakGlobalRef(jobj);
        if (!jhelper) {
            LOGE("P2pService(): Can't make NewWeakGlobalRef()\n");
            return;
        }

        //
        // Get a reference to the class from which the provided object was created.
        // We need the class in order to introspect its methods.
        //
        jclass clazz = env->GetObjectClass(jobj);
        if (!clazz) {
            LOGE("P2pService(): Can't GetObjectClass()\n");
            return;
        }

        LOGI("P2pService(): Mapping methods\n");

        //
        // Java associates method names with a method ID which we will need in order
        // to actually call the method.  We need to provide Java with a signature in
        // case the method is overloaded.  We do this once in the constructor so we
        // don't have to do possibly painfully expensive introspection calls when
        // our code is actually doing its main job.
        //
        MID_findAdvertisedName = env->GetMethodID(clazz, "FindAdvertisedName", "(Ljava/lang/String;)I");
        if (!MID_findAdvertisedName) {
            LOGE("P2pService(): Can't locate FindAdvertisedName()\n");
            return;
        }

        MID_cancelFindAdvertisedName = env->GetMethodID(clazz, "CancelFindAdvertisedName", "(Ljava/lang/String;)I");
        if (!MID_cancelFindAdvertisedName) {
            LOGE("P2pService(): Can't locate CancelFindAdvertisedName()\n");
            return;
        }

        MID_advertiseName = env->GetMethodID(clazz, "AdvertiseName", "(Ljava/lang/String;Ljava/lang/String;)I");
        if (!MID_advertiseName) {
            LOGE("P2pService(): Can't locate AdvertiseName()\n");
            return;
        }

        MID_cancelAdvertiseName = env->GetMethodID(clazz, "CancelAdvertiseName", "(Ljava/lang/String;Ljava/lang/String;)I");
        if (!MID_cancelAdvertiseName) {
            LOGE("P2pService(): Can't locate CancelAdvertiseName()\n");
            return;
        }

        MID_establishLink = env->GetMethodID(clazz, "EstablishLink", "(Ljava/lang/String;I)I");
        if (!MID_establishLink) {
            LOGE("P2pService(): Can't locate EstablishLink()\n");
            return;
        }

        MID_releaseLink = env->GetMethodID(clazz, "ReleaseLink", "(I)I");
        if (!MID_releaseLink) {
            LOGE("P2pService(): Can't locate ReleaseLink()\n");
            return;
        }

        MID_getInterfaceNameFromHandle = env->GetMethodID(clazz, "GetInterfaceNameFromHandle", "(I)Ljava/lang/String;");
        if (!MID_getInterfaceNameFromHandle) {
            LOGE("P2pService(): Can't locate GetInterfaceNameFromHandle()\n");
            return;
        }

        LOGI("P2pService(): Mapping signals\n");

        onFoundAdvertisedNameMember = p2pIntf->GetMember("OnFoundAdvertisedName");
        onLostAdvertisedNameMember = p2pIntf->GetMember("OnLostAdvertisedName");
        onLinkEstablishedMember = p2pIntf->GetMember("OnLinkEstablished");
        onLinkErrorMember = p2pIntf->GetMember("OnLinkError");
        onLinkLostMember = p2pIntf->GetMember("OnLinkLost");

        sessionId = 0; // Where to get this?

        detachEnv(jret);

        initialized = true;
    }

    ~P2pService() {
        LOGI("P2pService(): destruct");
    }

    //
    // All JNI functions are accessed indirectly through a pointer provided by the
    // Java virtual machine.  When working with this pointer, we have to be careful
    // about arranging to get the current thread associated with the environment so
    // we provide a convenience function that does what we need.
    //
    JNIEnv* attachEnv(jint* jret)
    {
        LOGI("attachEnv()");
        JNIEnv* env;
        *jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
        if (*jret == JNI_EDETACHED) {
            LOGI("attaching");
            int ret = vm->AttachCurrentThread(&env, NULL);
        }
        return env;
    }

    void detachEnv(jint jret) {
        LOGI("detachEnv()");
        if (jret == JNI_EDETACHED) {
            LOGI("detaching");
            vm->DetachCurrentThread();
        }
    }

    //
    // Tell the P2P framework that the daemon wants to find names with the provided
    // prefix using pre-association service discovery.
    //
    int FindAdvertisedName(const char* namePrefix)
    {
        int result = ER_OK;
        jint jret;
        LOGI("FindAdvertisedName()");

        if (!initialized) {
            LOGE("FindAdvertisedName(): Not initialized");
            return ER_GENERAL;
        }

        JNIEnv* env = attachEnv(&jret);

        jstring jnamePrefix = env->NewStringUTF(namePrefix);
        if (env->ExceptionCheck()) {
            LOGE("FindAdvertisedName(): Exception converting parameter <namePrefix>");
            result = ER_GENERAL;
        } else {
            jobject jo = env->NewLocalRef(jhelper);
            if (!jo) {
                LOGE("FindAdvertisedName(): Can't get Java object");
                result = ER_GENERAL;
            } else {
                result = env->CallIntMethod(jo, MID_findAdvertisedName, jnamePrefix);
                if (env->ExceptionCheck()) {
                    LOGE("FindAdvertisedName(): Exception calling Java");
                    result = ER_GENERAL;
                }
            }
        }

        detachEnv(jret);

        //
        // Status is really only that the framework could start the operation.  The
        // only time you'll probably get an error back from the framework is if it
        // cannot possibly support Wi-Fi Direct -- for example, if it is running
        // on a Gingerbread device.
        //
        return result;
    }

    //
    // Tell the P2P framework that the daemon is no longer interested in services
    // with the provided prefix.
    //
    int CancelFindAdvertisedName(const char* namePrefix)
    {
        int result = ER_OK;
        jint jret;
        LOGI("CancelFindAdvertisedName()\n");

        if (!initialized) {
            LOGD("CancelFindAdvertisedName(): Not initialized\n");
            return ER_GENERAL;
        }

        JNIEnv* env = attachEnv(&jret);

        jstring jnamePrefix = env->NewStringUTF(namePrefix);
        if (env->ExceptionCheck()) {
            LOGE("CancelFindAdvertisedName(): Exception converting parameter <namePrefix>\n");
            result = ER_GENERAL;
        } else {
            jobject jo = env->NewLocalRef(jhelper);
            if (!jo) {
                LOGE("CancelFindAdvertisedName(): Can't get Java object\n");
                result = ER_GENERAL;
            } else {
                result = env->CallIntMethod(jo, MID_cancelFindAdvertisedName, jnamePrefix);
                if (env->ExceptionCheck()) {
                    LOGE("CancelFindAdvertisedName(): Exception calling Java\n");
                    result = ER_GENERAL;
                }
            }
        }

        detachEnv(jret);

        return result;
    }

    //
    // Call Java P2PHelperService.advertiseName
    //
    int AdvertiseName(const char* name, const char* guid)
    {
        int result = ER_OK;
        jint jret;
        LOGI("P2pService::AdvertiseName()");

        if (!initialized) {
            LOGD("P2pService::AdvertiseName(): Not initialized");
            return ER_GENERAL;
        }

        JNIEnv* env = attachEnv(&jret);

        jstring jname = env->NewStringUTF(name);
        if (env->ExceptionCheck()) {
            LOGE("P2pService::AdvertiseName(): Exception converting parameter <name>");
            result = ER_GENERAL;
        } else {
            jstring jguid = env->NewStringUTF(guid);
            if (env->ExceptionCheck()) {
                LOGE("P2pService::AdvertiseName(): Exception converting parameter <guid>");
                result = ER_GENERAL;
            } else {
                jobject jo = env->NewLocalRef(jhelper);
                if (!jo) {
                    LOGE("P2pService::AdvertiseName(): Can't get Java object");
                    result = ER_GENERAL;
                } else {
                    result = env->CallIntMethod(jo, MID_advertiseName, jname, jguid);
                    if (env->ExceptionCheck()) {
                        LOGE("P2pService::AdvertiseName(): Exception calling Java");
                        result = ER_GENERAL;
                    }
                }
            }
        }

        detachEnv(jret);

        return result;
    }

    int CancelAdvertiseName(const char* name, const char* guid)
    {
        int result = ER_OK;
        jint jret;
        LOGD("CancelAdvertiseName()\n");

        if (!initialized) {
            LOGD("CancelAdvertiseName(): Not initialized\n");
            return ER_GENERAL;
        }

        JNIEnv* env = attachEnv(&jret);

        jstring jname = env->NewStringUTF(name);
        if (env->ExceptionCheck()) {
            LOGE("CancelAdvertiseName(): Exception converting parameter <name>");
            result = ER_GENERAL;
        } else {
            jstring jguid = env->NewStringUTF(guid);
            if (env->ExceptionCheck()) {
                LOGE("CancelAdvertiseName(): Exception converting parameter <guid>");
                result = ER_GENERAL;
            } else {
                jobject jo = env->NewLocalRef(jhelper);
                if (!jo) {
                    LOGE("CancelAdvertiseName(): Can't get Java object");
                    result = ER_GENERAL;
                } else {
                    int result = env->CallIntMethod(jo, MID_cancelAdvertiseName, jname, jguid);
                    if (env->ExceptionCheck()) {
                        LOGE("CancelAdvertiseName(): Exception calling Java");
                        result = ER_GENERAL;
                    }
                }
            }
        }

        detachEnv(jret);

        return result;
    }

    int EstablishLink(const char* device, int groupOwnerIntent)
    {
        int handle = 0;
        jint jret;
        LOGI("EstablishLink(%s, %d)", device, groupOwnerIntent);

        if (!initialized) {
            LOGE("EstablishLink(): Not initialized\n");
            return ER_GENERAL;
        }

        JNIEnv* env = attachEnv(&jret);

        jstring jdevice = env->NewStringUTF(device);
        if (env->ExceptionCheck()) {
            LOGE("EstablishLink(): Exception converting parameter <device>");
        } else {
            jobject jo = env->NewLocalRef(jhelper);
            if (!jo) {
                LOGE("EstablishLink(): Can't get Java object");
            } else {
                handle = env->CallIntMethod(jo, MID_establishLink, jdevice, groupOwnerIntent);
                if (env->ExceptionCheck()) {
                    LOGE("EstablishLink(): Exception calling Java");
                    handle = 0;
                }
            }
        }

        detachEnv(jret);

        return handle;
    }

    //
    // Communicate that the daemon is done with the link identified by the provided
    // handle.
    //
    int ReleaseLink(int handle)
    {
        int result = ER_OK;
        jint jret;
        LOGI("ReleaseLink()");

        if (!initialized) {
            LOGE("ReleaseLink(): Not initialized");
            return ER_GENERAL;
        }

        JNIEnv* env = attachEnv(&jret);

        jobject jo = env->NewLocalRef(jhelper);
        if (!jo) {
            LOGE("ReleaseLink(): Can't get Java object");
            result = ER_GENERAL;
        } else {
            int status = env->CallIntMethod(jo, MID_releaseLink, handle);
            if (env->ExceptionCheck()) {
                LOGE("ReleaseLink(): Exception calling Java");
                result = ER_GENERAL;
            }
        }

        detachEnv(jret);

        return result;
    }

    //
    // In order to route packets over particular links which may have link-local
    // addreses, the daemon may need to construct a zone ID to append to IP
    // addresses found during the multicast name service discovery phase.  To do
    // this, it needs the interface name of the network device used to send packets
    // to the destination.
    //
    // This function is used to get a netork interface name string corresponding to
    // the device associated with the P2P handle.
    //
    int GetInterfaceNameFromHandle(int handle, char* buffer, int buflen)
    {
        int result = ER_OK;
        jint jret;
        LOGI("GetInterfaceNameFromHandle()");

        if (!initialized) {
            LOGE("GetInterfaceNameFromHandle(): Not initialized");
            return ER_GENERAL;
        }

        JNIEnv* env = attachEnv(&jret);

        jobject jo = env->NewLocalRef(jhelper);
        if (!jo) {
            LOGE("GetInterfaceNameFromHandle(): Can't get Java object");
            result = ER_GENERAL;
        } else {
            jstring jname = (jstring)env->CallObjectMethod(jo, MID_getInterfaceNameFromHandle, handle);
            if (env->ExceptionCheck()) {
                LOGE("GetInterfaceNameFromHandle(): Exception calling Java");
                result = ER_GENERAL;
            } else if (jname) {
                const char* tmp = env->GetStringUTFChars(jname, NULL);
                strncpy(buffer, tmp, buflen);
                buffer[buflen] = '\0';
                env->ReleaseStringUTFChars(jname, tmp);
                result = ER_OK;
            } else {
                LOGE("GetInterfaceNameFromHandle(): Could not get interface name");
                result = ER_GENERAL;
            }
        }

        detachEnv(jret);

        return result;
    }

    //
    // AllJoyn Method handlers: Unmarshall the AllJoyn method parameters
    //
    void handleFindAdvertisedName(const InterfaceDescription::Member* member, Message& msg)
    {
        String namePrefix = msg->GetArg(0)->v_string.str;

        LOGD("handleFindAdvertisedName called from %s with: %s\n", msg->GetSender(), namePrefix.c_str());

        int result = FindAdvertisedName(namePrefix.c_str());

        LOGI("handleFindAdvertisedName replying with %d", result);
        MsgArg reply("i", result);
        QStatus status = MethodReply(msg, &reply, 1);
        if (ER_OK != status) {
            LOGE("handleFindAdvertisedName: Error sending reply (%s)", QCC_StatusText(status));
        }
    }

    void handleCancelFindAdvertisedName(const InterfaceDescription::Member* member, Message& msg)
    {
        String namePrefix = msg->GetArg(0)->v_string.str;

        LOGD("handleCancelFindAdvertisedName called from %s with: %s\n", msg->GetSender(), namePrefix.c_str());

        int result = CancelFindAdvertisedName(namePrefix.c_str());

        LOGI("handleCancelFindAdvertisedName replying with %d", result);
        MsgArg reply("i", result);
        QStatus status = MethodReply(msg, &reply, 1);
        if (ER_OK != status) {
            LOGE("handleCancelAdvertisedName: Error sending reply (%s)", QCC_StatusText(status));
        }
    }

    void handleAdvertiseName(const InterfaceDescription::Member* member, Message& msg)
    {
        String name = msg->GetArg(0)->v_string.str;
        String guid = msg->GetArg(1)->v_string.str;

        LOGD("handleAdvertiseName called from %s with: %s, %s\n", msg->GetSender(), name.c_str(), guid.c_str());

        int result = AdvertiseName(name.c_str(), guid.c_str());

        LOGI("handleAdvertiseName replying with %d", result);
        MsgArg reply("i", result);
        QStatus status = MethodReply(msg, &reply, 1);
        if (ER_OK != status) {
            LOGE("handleAdvertiseName: Error sending reply (%s)", QCC_StatusText(status));
        }
    }

    void handleCancelAdvertiseName(const InterfaceDescription::Member* member, Message& msg)
    {
        String name = msg->GetArg(0)->v_string.str;
        String guid = msg->GetArg(1)->v_string.str;

        LOGD("handleCancelAdvertiseName called from %s with: %s, %s\n", msg->GetSender(), name.c_str(), guid.c_str());

        int result = CancelAdvertiseName(name.c_str(), guid.c_str());

        LOGI("handleCancelAdvertisedName replying with %d", result);
        MsgArg reply("i", result);
        QStatus status = MethodReply(msg, &reply, 1);
        if (ER_OK != status) {
            LOGE("handleCancelAdvertiseName: Error sending reply (%s)", QCC_StatusText(status));
        }
    }

    void handleEstablishLink(const InterfaceDescription::Member* member, Message& msg)
    {
        String device = msg->GetArg(0)->v_string.str;
        int intent = msg->GetArg(1)->v_int32;

        LOGD("handleEstablishLink called from %s with: %s, %d\n", msg->GetSender(), device.c_str(), intent);

        int result = EstablishLink(device.c_str(), intent);

        LOGI("handleEstablishLink replying with %d", result);
        MsgArg reply("i", result);
        QStatus status = MethodReply(msg, &reply, 1);
        if (ER_OK != status) {
            LOGE("handleEstablishLink: Error sending reply (%s)", QCC_StatusText(status));
        }
    }

    void handleReleaseLink(const InterfaceDescription::Member* member, Message& msg)
    {
        int handle = msg->GetArg(0)->v_int32;

        LOGD("handleReleaseLink called from %s with: %d\n", msg->GetSender(), handle);

        int result = ReleaseLink(handle);

        LOGI("handleReleaseLink replying with %d", result);
        MsgArg reply("i", result);
        QStatus status = MethodReply(msg, &reply, 1);
        if (ER_OK != status) {
            LOGE("handleReleaseLink: Error sending reply (%s)", QCC_StatusText(status));
        }
    }

    void handleGetInterfaceNameFromHandle(const InterfaceDescription::Member* member, Message& msg)
    {
        int handle = msg->GetArg(0)->v_int32;
        char buf[64];

        LOGD("handleGetInterfaceNameFromHandle called from %s with: %d\n", msg->GetSender(), handle);

        int result = GetInterfaceNameFromHandle(handle, buf, 64);

        LOGI("handleGetInterfaceName replying with %d", result);
        MsgArg reply("s", buf);
        QStatus status = MethodReply(msg, &reply, 1);
        if (ER_OK != status) {
            LOGE("handleGetInterfaceNameFromHandle: Error sending reply (%s)", QCC_StatusText(status));
        }
    }

    int sendOnFoundAdvertisedName(const char* name, const char* namePrefix, const char* guid, const char* device) {
        MsgArg args[4];
        args[0].Set("s", name);
        args[1].Set("s", namePrefix);
        args[2].Set("s", guid);
        args[3].Set("s", device);

        LOGI("sendOnFoundAdvertisedName(%s, %s, %s, %s)", name, namePrefix, guid, device);
        QStatus status = Signal(NULL, sessionId, *onFoundAdvertisedNameMember, args, 4, 0);
        if (ER_OK != status) {
            LOGE("sendOnFoundAdvertisedName: Error sending signal (%s)", QCC_StatusText(status));
        }
        return static_cast<int>(status);
    }

    int sendOnLostAdvertisedName(const char* name, const char* namePrefix, const char* guid, const char* device) {
        MsgArg args[4];
        args[0].Set("s", name);
        args[1].Set("s", namePrefix);
        args[2].Set("s", guid);
        args[3].Set("s", device);

        LOGI("sendOnLostAdvertisedName(%s, %s, %s, %s)", name, namePrefix, guid, device);
        QStatus status = Signal(NULL, sessionId, *onLostAdvertisedNameMember, args, 4, 0);
        if (ER_OK != status) {
            LOGE("sendOnLostAdvertisedName: Error sending signal (%s)", QCC_StatusText(status));
        }
        return static_cast<int>(status);
    }

    int sendOnLinkEstablished(int handle, const char*interfaceName) {
        MsgArg args[2];
        args[0].Set("i", handle);
        args[1].Set("s", interfaceName);

        LOGI("sendOnLinkEstablished(%d, %s)", handle, interfaceName);
        QStatus status = Signal(NULL, sessionId, *onLinkEstablishedMember, args, 2, 0);
        if (ER_OK != status) {
            LOGE("sendOnLinkEstablished: Error sending signal (%s)", QCC_StatusText(status));
        }
        return static_cast<int>(status);
    }

    int sendOnLinkError(int handle, int error) {
        MsgArg args[2];
        args[0].Set("i", handle);
        args[1].Set("i", error);

        LOGI("sendOnLinkError(%d, %d)", handle, error);
        QStatus status = Signal(NULL, sessionId, *onLinkErrorMember, args, 2, 0);
        if (ER_OK != status) {
            LOGE("sendOnLinkError: Error sending signal (%s)", QCC_StatusText(status));
        }
        return static_cast<int>(status);
    }

    int sendOnLinkLost(int handle) {
        MsgArg arg("i", handle);

        LOGI("sendOnLinkLost(%d)", handle);
        QStatus status = Signal(NULL, sessionId, *onLinkLostMember, &arg, 1, 0);
        if (ER_OK != status) {
            LOGE("sendOnLinkLost: Error sending signal (%s)", QCC_StatusText(status));
        }
        return static_cast<int>(status);
    }

    jobject getJObject() {
        return jobj;
    }

  private:
    JavaVM* vm;
    jobject jobj;
    SessionId sessionId;
    const InterfaceDescription::Member* findAdvertisedNameMember;
    const InterfaceDescription::Member* cancelFindAdvertisedNameMember;
    const InterfaceDescription::Member* advertiseNameMember;
    const InterfaceDescription::Member* cancelAdvertiseNameMember;
    const InterfaceDescription::Member* establishLinkMember;
    const InterfaceDescription::Member* releaseLinkMember;
    const InterfaceDescription::Member* getInterfaceNameFromHandleMember;
    const InterfaceDescription::Member* onFoundAdvertisedNameMember;
    const InterfaceDescription::Member* onLostAdvertisedNameMember;
    const InterfaceDescription::Member* onLinkEstablishedMember;
    const InterfaceDescription::Member* onLinkErrorMember;
    const InterfaceDescription::Member* onLinkLostMember;
    bool initialized;
    jobject jhelper;
    jmethodID MID_findAdvertisedName;
    jmethodID MID_cancelFindAdvertisedName;
    jmethodID MID_advertiseName;
    jmethodID MID_cancelAdvertiseName;
    jmethodID MID_establishLink;
    jmethodID MID_releaseLink;
    jmethodID MID_getInterfaceNameFromHandle;

};



#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     org_alljoyn_bus_p2p_service_P2pHelperService
 * Method:    jniInit
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_p2p_service_P2pHelperService_jniOnCreate(JNIEnv*env, jobject jobj, jstring connectSpec) {

    QStatus status = ER_OK;
    InterfaceDescription* p2pIntf = NULL;
    JavaVM* vm;
    jobject jglobalObj = NULL;

    jglobalObj = env->NewGlobalRef(jobj);

    env->GetJavaVM(&vm);

    LOGI("jniOnCreate");

    /* Create message bus */
    s_bus = new BusAttachment("P2pHelperService", true);
    if (!s_bus) {
        LOGE("new BusAttachment failed");
        return false;
    } else {
        /* Create org.alljoyn.bus.p2p.service interface */
        status = s_bus->CreateInterface(P2P_SERVICE_INTERFACE_NAME, p2pIntf);
        if (ER_OK != status) {
            LOGE("Failed to create interface \"%s\" (%s)", P2P_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
            delete s_bus;
            s_bus = NULL;
            return false;
        } else {

            p2pIntf->AddMethod("FindAdvertisedName",         "s",  "i",  "namePrefix,result");
            p2pIntf->AddMethod("CancelFindAdvertisedName",   "s",  "i",  "namePrefix,result");
            p2pIntf->AddMethod("AdvertiseName",              "ss", "i",  "name,guid,result");
            p2pIntf->AddMethod("CancelAdvertiseName",        "ss", "i",  "name,guid,result");
            p2pIntf->AddMethod("EstablishLink",              "si", "i",  "device,intent,result");
            p2pIntf->AddMethod("ReleaseLink",                "i",  "i",  "handle,result");
            p2pIntf->AddMethod("GetInterfaceNameFromHandle", "i",  "s",  "handle,interface");

            p2pIntf->AddSignal("OnFoundAdvertisedName",      "ssss", "name,namePrefix,guid,device");
            p2pIntf->AddSignal("OnLostAdvertisedName",       "ssss", "name,namePrefix,guid,device");
            p2pIntf->AddSignal("OnLinkEstablished",          "is",    "handle,interfaceName");
            p2pIntf->AddSignal("OnLinkError",                "ii",   "handle,error");
            p2pIntf->AddSignal("OnLinkLost",                 "i",    "handle");

            p2pIntf->Activate();

            /* Create the P2P service object */
            s_obj = new P2pService(*s_bus, P2P_SERVICE_OBJECT_PATH, vm, jglobalObj);
            status = s_bus->RegisterBusObject(*s_obj);
            if (ER_OK != status) {
                LOGE("BusAttachment::RegisterBusObject failed (%s)", QCC_StatusText(status));
                delete s_obj;
                delete s_bus;
                s_obj = NULL;
                s_bus = NULL;
                return false;
            } else {
                /* Start the msg bus */
                status = s_bus->Start();
                if (ER_OK != status) {
                    LOGE("BusAttachment::Start failed (%s)", QCC_StatusText(status));
                    delete s_obj;
                    delete s_bus;
                    s_obj = NULL;
                    s_bus = NULL;
                    return false;
                } else {
                    /* Connect to the daemon */
                    const char* cSpec = env->GetStringUTFChars(connectSpec, NULL);
                    status = s_bus->Connect(cSpec);
                    if (ER_OK != status) {
                        LOGE("BusAttachment::Connect(\"%s\") failed (%s)", cSpec, QCC_StatusText(status));
                        s_bus->Disconnect(cSpec);
                        s_bus->UnregisterBusObject(*s_obj);
                        delete s_obj;
                        delete s_bus;
                        s_obj = NULL;
                        s_bus = NULL;
                        env->ReleaseStringUTFChars(connectSpec, cSpec);
                        return false;
                    } else {
                        const char*cConnectSpec = s_bus->GetConnectSpec().c_str();
                        isStandalone = (strcmp(cConnectSpec, "null:") == 0) ? true  : false;
                        LOGE("BusAttachment::Connect(\"%s\") SUCCEEDED (%s)", cConnectSpec, QCC_StatusText(status));
                    }
                    env->ReleaseStringUTFChars(connectSpec, cSpec);
                }
            }
        }
    }

    /* Request name */
    status = s_bus->RequestName(P2P_SERVICE_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (ER_OK != status) {
        LOGE("RequestName(%s) failed (status=%s)\n", P2P_SERVICE_NAME, QCC_StatusText(status));
        status = (status == ER_OK) ? ER_FAIL : status;
    } else {
        LOGI("Request Name was successful");
    }

    return true;
}

/*
 * Class:     org_alljoyn_bus_p2p_service_P2pHelperService
 * Method:    jniOnDestroy
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_p2p_service_P2pHelperService_jniOnDestroy(JNIEnv* env, jobject jobj, jstring connectSpec) {

    LOGI("jniOnDestroy");

    const char* cSpec = env->GetStringUTFChars(connectSpec, NULL);
    if (s_bus) {
        s_bus->ReleaseName(P2P_SERVICE_NAME);
        s_bus->Disconnect(cSpec);
        if (s_obj) {
            s_bus->UnregisterBusObject(*s_obj);
        }
    }
    env->ReleaseStringUTFChars(connectSpec, cSpec);

    delete s_obj;
    delete s_bus;
    s_obj = NULL;
    s_bus = NULL;
}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_p2p_service_P2pHelperService_jniOnFoundAdvertisedName(JNIEnv* env, jobject jobj, jstring name, jstring namePrefix, jstring guid, jstring device) {
    int status = ER_P2P_NOT_CONNECTED;

    if (s_obj) {
        const char* cName = env->GetStringUTFChars(name, NULL);
        const char* cNamePrefix = env->GetStringUTFChars(namePrefix, NULL);
        const char* cGuid = env->GetStringUTFChars(guid, NULL);
        const char* cDevice = env->GetStringUTFChars(device, NULL);

        status = s_obj->sendOnFoundAdvertisedName(cName, cNamePrefix, cGuid, cDevice);

        env->ReleaseStringUTFChars(name, cName);
        env->ReleaseStringUTFChars(namePrefix, cNamePrefix);
        env->ReleaseStringUTFChars(guid, cGuid);
        env->ReleaseStringUTFChars(device, cDevice);
    }
    return status;

}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_p2p_service_P2pHelperService_jniOnLostAdvertisedName(JNIEnv* env, jobject jobj, jstring name, jstring namePrefix, jstring guid, jstring device) {
    int status = ER_P2P_NOT_CONNECTED;

    if (s_obj) {
        const char* cName = env->GetStringUTFChars(name, NULL);
        const char* cNamePrefix = env->GetStringUTFChars(namePrefix, NULL);
        const char* cGuid = env->GetStringUTFChars(guid, NULL);
        const char* cDevice = env->GetStringUTFChars(device, NULL);

        status = s_obj->sendOnLostAdvertisedName(cName, cNamePrefix, cGuid, cDevice);

        env->ReleaseStringUTFChars(name, cName);
        env->ReleaseStringUTFChars(namePrefix, cNamePrefix);
        env->ReleaseStringUTFChars(guid, cGuid);
        env->ReleaseStringUTFChars(device, cDevice);
    }
    return static_cast<int>(status);
}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_p2p_service_P2pHelperService_jniOnLinkEstablished(JNIEnv* env, jobject jobj, jint handle, jstring name) {
    int status = ER_P2P_NOT_CONNECTED;

    if (s_obj) {
        const char* cName = env->GetStringUTFChars(name, NULL);
        status = s_obj->sendOnLinkEstablished(handle, cName);
        env->ReleaseStringUTFChars(name, cName);
    }
    return static_cast<int>(status);
}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_p2p_service_P2pHelperService_jniOnLinkError(JNIEnv* env, jobject jobj, jint handle, jint error) {
    int status = ER_P2P_NOT_CONNECTED;

    if (s_obj) {
        status = s_obj->sendOnLinkError(handle, error);
    }
    return static_cast<int>(status);
}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_p2p_service_P2pHelperService_jniOnLinkLost(JNIEnv* env, jobject jobj, jint handle) {
    int status = ER_P2P_NOT_CONNECTED;

    if (s_obj) {
        status = s_obj->sendOnLinkLost(handle);
    }
    return static_cast<int>(status);
}

/*
 * Class:     org_alljoyn_bus_p2p_service_P2pHelperService
 * Method:    jniInit
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_p2p_service_P2pHelperService_jniCheckStandalone(JNIEnv*env, jobject jobj) {
    return isStandalone;
}

#ifdef __cplusplus
}
#endif
