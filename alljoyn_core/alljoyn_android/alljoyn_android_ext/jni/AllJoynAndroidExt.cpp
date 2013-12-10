/**
 * @file
 * This is the native code that handles the bus communication part of the
 * Android service used for getting Wifi scan results from the Android
 * framework
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

#include "org_alljoyn_jni_AllJoynAndroidExt.h"
#include "org_alljoyn_jni_ScanResultMessage.h"
#include <alljoyn/BusAttachment.h>
#include <qcc/String.h>
#include <android/log.h>
#include <qcc/Log.h>
#include <assert.h>
#include <alljoyn/DBusStd.h>

#define LOG_TAG  "AllJoynAndroidExt"
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


using namespace ajn;
using namespace qcc;

class ScanService;
static const char* SCAN_SERVICE_INTERFACE_NAME = "org.alljoyn.proximity.proximityservice";
static const char* SCAN_SERVICE_OBJECT_PATH = "/ProximityService";
static const char* SERVICE_NAME = "org.alljoyn.proximity.proximityservice";
static jclass CLS_ScanResultMessage = NULL;

static jfieldID bssidFID = NULL;
static jfieldID ssidFID = NULL;
static jfieldID attachedFID = NULL;

static BusAttachment* s_bus = NULL;
static ScanService* s_obj = NULL;



class ScanService : public BusObject {
  public:
    ScanService(BusAttachment& bus, const char* path, JavaVM* vm, jobject jobj) : BusObject(bus, path), vm(vm), jobj(jobj)
    {

        QStatus status;

        /* Add the chat interface to this object */
        const InterfaceDescription* scanIntf = bus.GetInterface(SCAN_SERVICE_INTERFACE_NAME);
        assert(scanIntf);
        AddInterface(*scanIntf);

        /* Store the Chat signal member away so it can be quickly looked up when signals are sent */
        scanMethodMember = scanIntf->GetMember("Scan");
        assert(scanMethodMember);

        const MethodEntry methodEntries[] = {
            { scanIntf->GetMember("Scan"), static_cast<MessageReceiver::MethodHandler>(&ScanService::Scan) }
        };

        status = AddMethodHandlers(methodEntries, ARRAY_SIZE(methodEntries));

        if (ER_OK != status) {
            LOGE("Failed to register method handlers for AllJoynAndroidExtService (%s)", QCC_StatusText(status));
        }

    }

    void Scan(const InterfaceDescription::Member* member, Message& msg)
    {
        bool request_scan = msg->GetArg(0)->v_bool;

        LOGD("Pinged from %s with: %d\n", msg->GetSender(), request_scan);


        JNIEnv* env;
        jobjectArray scanresults;
        MsgArg*args = NULL;

        LOGD("Before getting environment\n");
        if (!vm) {
            LOGD("VM is NULL\n");
        }
        jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
        if (JNI_EDETACHED == jret) {
            vm->AttachCurrentThread(&env, NULL);
            LOGD("Attached to VM thread \n");
        }
        LOGD("After getting environment\n");

        LOGD("Before getting class AllJoynAndroidExt\n");
        jclass jcls = env->GetObjectClass(jobj);

        // get field id
        // get object field
        // call method on that object

        LOGD("After getting class AllJoynAndroidExt\n");

        jmethodID mid = env->GetMethodID(jcls, "Scan", "(Z)[Lorg/alljoyn/jni/ScanResultMessage;");
        if (mid == 0) {
            LOGE("Failed to get Java Scan");
        } else {
            //jboolean jRequestScan = env->NewStringUTF(request_scan);
            LOGE("Calling Java method Scan");
            jboolean jRequestScan = (jboolean)request_scan;
            scanresults = (jobjectArray)env->CallObjectMethod(jobj, mid, jRequestScan);
        }
        if (scanresults == NULL) {
            LOGE("Scan results returned nothing");
            MsgArg reply("a(ssb)", 0, args);
            QStatus status = MethodReply(msg, &reply, 1);
            if (ER_OK != status) {
                LOGE("Ping: Error sending reply (%s)", QCC_StatusText(status));
            }
        } else {
            jsize scanresultsize = env->GetArrayLength(scanresults);


            LOGE("Length of scan results : %d", scanresultsize);

            LOGD(" *****************************Printing the scan results***************************** ");

            //MsgArg*args = NULL;
            // Check of the scan results
            if (scanresultsize  > 0) {
                args = new MsgArg[scanresultsize];
            }

            // Create the array to be returned with the size = scanresultsize


            jfieldID bssidFID = env->GetFieldID(CLS_ScanResultMessage, "bssid", "Ljava/lang/String;");
            if (bssidFID == NULL) {
                LOGD("Error while getting the field id bssid");
            }
            jfieldID ssidFID = env->GetFieldID(CLS_ScanResultMessage, "ssid", "Ljava/lang/String;");
            if (ssidFID == NULL) {
                LOGD("Error while getting the field id ssid");
            }

            jfieldID attachedFID = env->GetFieldID(CLS_ScanResultMessage, "attached", "Z");
            if (attachedFID == NULL) {
                LOGD("Error while getting the field id attached");
            }

            for (int i = 0; i < scanresultsize; i++) {

                jobject scanresult = env->GetObjectArrayElement(scanresults, i);
                if (scanresult == NULL) {
                    LOGD("Error while getting the scan result object from the array");
                }

                jstring jbssid = (jstring)env->GetObjectField(scanresult, bssidFID);
                if (jbssid == NULL) {
                    LOGE("Could not retrieve bssid from the scan results object");
                }

                jstring jssid = (jstring)env->GetObjectField(scanresult, ssidFID);
                if (jssid == NULL) {
                    LOGE("Could not retrieve ssid from the scan results object");
                }

                jboolean jattached = env->GetBooleanField(scanresult, attachedFID);

                const char*bssid = env->GetStringUTFChars(jbssid, NULL);
                const char*ssid = env->GetStringUTFChars(jssid, NULL);
                bool attached = (bool)jattached;
                if (bssid != NULL) {

                    args[i].Set("(ssb)", bssid, ssid, attached);
                }

                if (bssid != NULL) {
                    //LOGD("BSSID = %s    SSID = %s    attached = %s",bssid,ssid,(attached) ? "true" : "false");
                }


                // Populate the array to be returned


                LOGD("BSSID = %s    SSID = %s    attached = %s", bssid, ssid, (attached) ? "true" : "false");

            }


            LOGD(" *********************************************************************************** ");

            /* Reply with same string that was sent to us */
            MsgArg reply("a(ssb)", scanresultsize, args);
            QStatus status = MethodReply(msg, &reply, 1);
            if (ER_OK != status) {
                LOGE("Ping: Error sending reply (%s)", QCC_StatusText(status));
            }
        }




        if (JNI_EDETACHED == jret) {
            vm->DetachCurrentThread();
        }

    }

  private:
    JavaVM* vm;
    jobject jobj;
    const InterfaceDescription::Member* scanMethodMember;
};


#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_alljoyn_jni_AllJoynAndroidExt
 * Method:    jniOnCreate
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_alljoyn_jni_AllJoynAndroidExt_jniOnCreate(JNIEnv*env, jobject jobj, jstring packageNameStrObj) {


    QStatus status = ER_OK;
    const char* packageNameStr = NULL;
    const char* daemonAddr = "unix:abstract=alljoyn";
    InterfaceDescription* scanIntf = NULL;
    JavaVM* vm;
    jobject jglobalObj = NULL;

    jglobalObj = env->NewGlobalRef(jobj);

    env->GetJavaVM(&vm);

    packageNameStr = env->GetStringUTFChars(packageNameStrObj, NULL);
    if (!packageNameStr) {
        LOGE("GetStringUTFChars failed");
        status = ER_FAIL;
    }

    while (true) {

        /* Create message bus */
        s_bus = new BusAttachment("AllJoynAndroidExtService", true);
        if (!s_bus) {
            LOGE("new BusAttachment failed");
        } else {
            /* Create org.alljoyn.bus.samples.chat interface */
            status = s_bus->CreateInterface(SCAN_SERVICE_INTERFACE_NAME, scanIntf);
            if (ER_OK != status) {
                LOGE("Failed to create interface \"%s\" (%s)", SCAN_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
            } else {
                status = scanIntf->AddMethod("Scan", "b",  "a(ssb)", "results");
                if (ER_OK != status) {
                    LOGE("Failed to AddMethod \"Scan\" (%s)", QCC_StatusText(status));
                }

                status = scanIntf->AddMethod("GetHomeDir", NULL, "s", "results");
                if (ER_OK != status) {
                    LOGE("Failed to AddMethod \"GetHomeDir\" (%s)", QCC_StatusText(status));
                }

                scanIntf->Activate();

                /* Register service object */
                s_obj = new ScanService(*s_bus, SCAN_SERVICE_OBJECT_PATH, vm, jglobalObj);
                status = s_bus->RegisterBusObject(*s_obj);
                if (ER_OK != status) {
                    LOGE("BusAttachment::RegisterBusObject failed (%s)", QCC_StatusText(status));
                } else {
                    /* Start the msg bus */
                    status = s_bus->Start();
                    if (ER_OK != status) {
                        LOGE("BusAttachment::Start failed (%s)", QCC_StatusText(status));
                    } else {
                        /* Connect to the daemon */
                        status = s_bus->Connect(daemonAddr);
                        if (ER_OK != status) {
                            LOGE("BusAttachment::Connect(\"%s\") failed (%s)", daemonAddr, QCC_StatusText(status));
                            s_bus->Disconnect(daemonAddr);
                            s_bus->UnregisterBusObject(*s_obj);
                            delete s_obj;
                        } else {
                            LOGE("BusAttachment::Connect(\"%s\") SUCCEDDED (%s)", daemonAddr, QCC_StatusText(status));
                            break;
                        }
                    }
                }
            }
        }
        LOGD("Sleeping before trying to reconnect to the daemon");
        sleep(5);
        LOGD("Up from sleep");
    }


    /* Request name */
    status = s_bus->RequestName(SERVICE_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (ER_OK != status) {
        LOGE("RequestName(%s) failed (status=%s)\n", SERVICE_NAME, QCC_StatusText(status));
        status = (status == ER_OK) ? ER_FAIL : status;
    } else {
        LOGE("\n Request Name was successful");
    }


    return (jint) 1;


}

/*
 * Class:     org_alljoyn_jni_AllJoynAndroidExt
 * Method:    jniOnDestroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_jni_AllJoynAndroidExt_jniOnDestroy(JNIEnv*, jobject) {

}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm,
                                  void* reserved)
{
    /* Set AllJoyn logging */
    //QCC_SetLogLevels("ALLJOYN=7;ALL=1");
    //QCC_UseOSLogging(true);


    JNIEnv* env;
    LOGD("Before getting environment\n");
    if (!vm) {
        LOGD("VM is NULL\n");
    }
    jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
    if (JNI_EDETACHED == jret) {
        vm->AttachCurrentThread(&env, NULL);
        LOGD("Attached to VM thread \n");
    }
    LOGD("After getting environment\n");

    jclass clazz;
    clazz = env->FindClass("org/alljoyn/jni/ScanResultMessage");
    if (!clazz) {
        LOGD("*********************** error while loading the class *******************");
        return JNI_ERR;
    } else {
        //LOGD("org/alljoyn/jni/ScanResultMessage loaded SUCCESSFULLY");
    }

    CLS_ScanResultMessage = (jclass)env->NewGlobalRef(clazz);

    return JNI_VERSION_1_2;
}


#ifdef __cplusplus
}
#endif

