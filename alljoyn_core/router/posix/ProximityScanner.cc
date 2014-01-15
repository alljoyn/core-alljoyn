/**
 * @file
 * ProximityScanner provides the scan results used by the Discovery framework and Rendezvous server
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>
#include <map>
#include <stdio.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <alljoyn/Status.h>
#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>

#include <utility>
#include "ProximityScanner.h"

#if defined(QCC_OS_DARWIN)
#if defined(QCC_OS_IPHONE)
#include <SystemConfiguration/CaptiveNetwork.h>
#else
#import <CoreWLAN/CoreWLAN.h>
#endif
#endif


#define QCC_MODULE "PROXIMITY_SCANNER"
#define LOG_TAG  "ProximityScanner"

#if defined (QCC_OS_ANDROID)
        #include <android/log.h>
        #include <jni.h>

        #ifndef LOGD
        #define LOGD(...) (__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
        #endif

        #ifndef LOGI
        #define LOGI(...) (__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
        #endif

        #ifndef LOGE
        #define LOGE(...) (__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
        #endif

JavaVM* proxJVM = NULL;
JNIEnv* psenv = NULL;
jclass CLS_AllJoynAndroidExt = NULL;
jclass CLS_ScanResultMessage = NULL;
jmethodID MID_AllJoynAndroidExt_Scan = NULL;

#endif




using namespace qcc;

namespace ajn {

ProximityScanner::ProximityScanner(BusAttachment& bus) : bus(bus) {
    QCC_DbgTrace(("ProximityScanner::ProximityScanner()"));
}

void ProximityScanner::PrintBSSIDMap(std::map<qcc::String, qcc::String> mymap) {

    std::map<qcc::String, qcc::String>::iterator it;
    for (it = mymap.begin(); it != mymap.end(); it++) {
        //QCC_DbgPrintf(("\n BSSID : %s", it->first.c_str()));
    }

}


class MyBusListener : public BusListener, public SessionListener {
  public:
    MyBusListener() : BusListener(), sessionId(0) { }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        //QCC_DbgPrintf(("\n Found the SERVICE ..... Wooooohoooo !!"));

    }

    SessionId GetSessionId() const { return sessionId; }

  private:
    SessionId sessionId;
};

//void ProximityScanner::StopScan() {
//
//
//	MyBusListener* g_busListener;
//	g_busListener = new MyBusListener();
//	bus.RegisterBusListener(*g_busListener);
//
//
//
//
//
//
//}

//
//	There are many different platform and build combinations in which proximity scan function
//	can be used
//	1. Android without bundled daemon in which case it makes AllJoyn calls to the standalone Android service app called alljoyn_android_ext
//	2. Android with bundled daemon in Java app in which case we need not make AllJoyn calls to the standalone proximity service
//	3. Android with bundled daemon in native code. An implementation for this is not present since we do not support this
//	4. Linux wihtout or without bundled daemon in native or Java or any other code. We make DBus calls to wpa_supplicant
//	5. Windows with or without bundled daemon in any code. We use Windows SDK APIs to get scan results
//	6. iOS/Darwin with or without bundled daemon. We get scan results using CaptiveNetwork APIs
//	7. Windows with or without bundled daemon. We use Windows SDK APIs to get scan results
//

// Case 1 and 2: If platform is Android. It internall checks if it is a bundled daemon or not

#if defined(QCC_OS_ANDROID)

void ProximityScanner::Scan(bool request_scan) {

    QCC_DbgTrace(("ProximityScanner::Scan()"));

    //
    // Check if this is an instance of bundled daemon running or a standalone alljoyn-daemon
    // In case of AllJoyn.apk we will go with the same approach as standalone daemon that is we
    // will have a separate app that handles ICE proximity information
    //
    if (proxJVM != NULL) {
        // Check if proxJVM was initialized
        if (proxJVM == NULL) {
            // Here since proxJVM is null there is no point in proceeding
            scanResults.clear();
            return;
        }

        //
        // Attach to the current thread
        //
        jint jret = proxJVM->GetEnv((void**)&psenv, JNI_VERSION_1_2);
        if (JNI_EDETACHED == jret) {
            proxJVM->AttachCurrentThread(&psenv, NULL);
        }

        //
        // Clear the scan results maps before storing any results in it
        //
        scanResults.clear();

        //
        // Once we are attached to the current thread we call the function Scan in AllJoynAndroidExt
        //
        jboolean jRequestScan = (jboolean)request_scan;
        jobjectArray scanresults = (jobjectArray)psenv->CallStaticObjectMethod(CLS_AllJoynAndroidExt, MID_AllJoynAndroidExt_Scan, jRequestScan);
        if (psenv->ExceptionCheck()) {
            LOGE("Exception thrown after method call Scan");
            psenv->ExceptionDescribe();
        }
        if (scanresults == NULL) {
            LOGE("Scan results returned nothing");
            return;
        }

        // We print out the size of scan results
        jsize scanresultsize = psenv->GetArrayLength(scanresults);

        //
        // Store the field ids of the structure ScanResultMessage class
        //

        jfieldID bssidFID = psenv->GetFieldID(CLS_ScanResultMessage, "bssid", "Ljava/lang/String;");
        if (bssidFID == NULL) {
            LOGE("Error while getting the field id bssid");
        }
        jfieldID ssidFID = psenv->GetFieldID(CLS_ScanResultMessage, "ssid", "Ljava/lang/String;");
        if (ssidFID == NULL) {
            LOGE("Error while getting the field id ssid");
        }
        jfieldID attachedFID = psenv->GetFieldID(CLS_ScanResultMessage, "attached", "Z");
        if (attachedFID == NULL) {
            LOGE("Error while getting the field id attached");
        }

        //
        // Parse through the array of structure returned by the Scan method call
        // The structure is of type ScanResultMessage
        //
        for (int i = 0; i < scanresultsize; i++) {

            jobject scanresult = psenv->GetObjectArrayElement(scanresults, i);
            if (scanresult == NULL) {
                LOGE("Error while getting the scan result object from the array");
            }

            jstring jbssid = (jstring)psenv->GetObjectField(scanresult, bssidFID);
            if (jbssid == NULL) {
                LOGE("Could not retrieve bssid from the scan results object");
            }

            jstring jssid = (jstring)psenv->GetObjectField(scanresult, ssidFID);
            if (jssid == NULL) {
                LOGE("Could not retrieve ssid from the scan results object");
            }

            jboolean jattached = psenv->GetBooleanField(scanresult, attachedFID);

            const char*bssid = psenv->GetStringUTFChars(jbssid, NULL);
            const char*ssid = psenv->GetStringUTFChars(jssid, NULL);
            bool attached = (bool)jattached;

#ifndef NDEBUG
            if (bssid != NULL) {
                LOGD("BSSID = %s    SSID = %s    attached = %s", bssid, ssid, (attached) ? "true" : "false");
            }
#endif
            qcc::String bssid_str(bssid);
            qcc::String ssid_str(ssid);
            scanResults.insert(std::map<std::pair<qcc::String, qcc::String>, bool>::value_type(std::make_pair(bssid_str, ssid_str), attached));

        }

        //
        // Detach current native thread from the JVM thread
        //
        proxJVM->DetachCurrentThread();
    } else {
        //
        // We are not running inside bundled daemon
        //
        QStatus status;
        MyBusListener* g_busListener;

        g_busListener = new MyBusListener();
        bus.RegisterBusListener(*g_busListener);

        bool hasOwer;

        uint32_t starttime = GetTimestamp();

        while (true) {
            status = bus.NameHasOwner("org.alljoyn.proximity.proximityservice", hasOwer);
            if (ER_OK != status) {
                QCC_LogError(status, ("Error while calling NameHasOwner"));
            }
            if (hasOwer) {
                QCC_DbgPrintf(("NameHasOwnwer: Android Helper Service running"));
                break;
            } else {
                QCC_DbgPrintf(("No Android service owner found yet"));
                // If service is not present there is not point in sleeping. We can return empty data
                scanResults.clear();
                return;
                //qcc::Sleep(5000);
            }

        }

        ProxyBusObject*remoteObj = new ProxyBusObject(bus, "org.alljoyn.proximity.proximityservice", "/ProximityService", 0);

        status = remoteObj->IntrospectRemoteObject();
        if (ER_OK != status) {
            QCC_LogError(status, ("Problem while introspecting the remote object /ProximityService"));
        } else {
            QCC_DbgPrintf(("Introspection on the remote object /ProximityService successful"));
        }

        // Call the remote method SCAN on the service

        // Why do change request_scan here ? This handles the situation where the service was killed by the OS and
        // we are not able to get the scan results

        QCC_DbgPrintf(("Time before Scan  %d", starttime));
        Message reply(bus);
        MsgArg arg;
        arg.Set("b", request_scan);
        status = remoteObj->MethodCall("org.alljoyn.proximity.proximityservice", "Scan", &arg, 1, reply, 35000);
        //status = remoteObj->MethodCall("org.alljoyn.proximity.proximityservice", "Scan", NULL, 0, reply, 35000);
        if (ER_OK != status) {
            QCC_LogError(status, ("Problem while calling method Scan on the remote object"));
            qcc::String errorMsg;
            reply->GetErrorName(&errorMsg);
            QCC_LogError(status, ("Call to Scan returned error message : %s", errorMsg.c_str()));
            scanResults.clear();
            return;

        } else {
            QCC_DbgPrintf(("Method call Scan was successful \n"));
        }

        //
        // Clear the map before storing any results in in
        //
        scanResults.clear();
        //
        // Copy the results from the reply to the scanResults map
        //
        MsgArg*scanArray;
        size_t scanArraySize;
        const MsgArg*args = reply->GetArg(0);
        if (args == NULL) {
            return;
        }
        status = args->Get("a(ssb)", &scanArraySize, &scanArray);
        if (ER_OK != status) {
            QCC_LogError(status, ("Error while unmarshalling the array of structs recevied from the service"));
        }
        //
        // We populate the scanResultsMap only when we have results
        //
        if (ER_OK == status && scanArraySize > 0) {
            QCC_DbgPrintf(("Array size of scan results > 0"));
            for (size_t i = 0; i < scanArraySize; i++) {
                char* bssid;
                char* ssid;
                bool attached;

                status = scanArray[i].Get("(ssb)", &bssid, &ssid, &attached);
                if (ER_OK != status) {
                    QCC_LogError(status, ("Error while getting the struct members Expected signature = %s", scanArray[i].Signature().c_str()));
                } else {
                    qcc::String bssid_str(bssid);
                    qcc::String ssid_str(ssid);
                    scanResults.insert(std::map<std::pair<qcc::String, qcc::String>, bool>::value_type(std::make_pair(bssid_str, ssid_str), attached));
                }

            }

            QCC_DbgPrintf(("From Scan function"));
            std::map<std::pair<qcc::String, qcc::String>, bool>::iterator it;
            for (it = scanResults.begin(); it != scanResults.end(); it++) {
                QCC_DbgPrintf(("BSSID = %s , SSID = %s, attached = %s", it->first.first.c_str(), it->first.second.c_str(), (it->second ? "true" : "false")));
            }


        } else {
            // We do not have any scan results returned by the Android service ??
            QCC_DbgPrintf(("No Scan results were returned by the service. Either Wifi is turned off or there are no APs around"));
        }

        QCC_DbgPrintf(("Time after Scan processing %d", GetTimestamp() - starttime));
        // end if
    }

}
// Case no 4: If it is Linux build we make DBus calls to the wpa_supplicant
#elif defined (QCC_OS_LINUX)
void ProximityScanner::Scan(bool request_scan) {
}

#elif defined(QCC_OS_DARWIN)

#if defined(QCC_OS_IPHONE) && !defined(QCC_OS_IPHONE_SIMULATOR)
void ProximityScanner::Scan(bool request_scan) {
    QCC_DbgTrace(("ProximityScanner::Scan()"));

    QCC_DbgPrintf(("Retrieving BSSID from CaptiveNetwork API for iOS..."));


    // Start with a clean slate
    //
    scanResults.clear();

    // Ask iOS for a list of available network interfaces on the device
    //
    CFArrayRef supportedInterfaces = CNCopySupportedInterfaces();

    // Walk through the list of interfaces and find the wifi interface
    // On iOS the name of the wifi interface will always be "en0"
    //
    bool foundWifiInterface = false;
    for (int i = 0; i < CFArrayGetCount(supportedInterfaces); i++) {

        CFStringRef interfaceNameString = (CFStringRef)CFArrayGetValueAtIndex(supportedInterfaces, i);

        if (CFStringCompare(CFSTR("en0"), interfaceNameString, 0) == kCFCompareEqualTo) {

            CFStringEncoding encodingMethod = CFStringGetSystemEncoding();
            CFDictionaryRef networkInfo = CNCopyCurrentNetworkInfo(interfaceNameString);
            if (NULL == networkInfo) {
                continue;
            }
            CFStringRef bssidString = (CFStringRef)CFDictionaryGetValue(networkInfo, kCNNetworkInfoKeyBSSID);
            CFStringRef ssidString = (CFStringRef)CFDictionaryGetValue(networkInfo, kCNNetworkInfoKeySSID);
            qcc::String bssid_str(CFStringGetCStringPtr(bssidString, encodingMethod));
            qcc::String ssid_str(CFStringGetCStringPtr(ssidString, encodingMethod));

            CFRelease(networkInfo);

            bool attached = true; // always true on iOS, since we can only see the BSSID of the router we are affiliated with.

            scanResults.insert(std::map<std::pair<qcc::String, qcc::String>, bool>::value_type(std::make_pair(bssid_str, ssid_str), attached));

            QCC_DbgPrintf(("From Scan function"));
            std::map<std::pair<qcc::String, qcc::String>, bool>::iterator it;
            for (it = scanResults.begin(); it != scanResults.end(); it++) {
                QCC_DbgPrintf(("BSSID = %s , SSID = %s, attached = %s", it->first.first.c_str(), it->first.second.c_str(), (it->second ? "true" : "false")));
            }

            foundWifiInterface = true;
        }

        if (foundWifiInterface) {
            break;
        }
    }

    CFRelease(supportedInterfaces);
}

#else

void ProximityScanner::Scan(bool request_scan) {
    QCC_DbgTrace(("ProximityScanner::Scan()"));

}

#endif

#endif
}

