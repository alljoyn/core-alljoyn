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
#ifndef _PROXIMITY_SCANNER_H_
#define _PROXIMITY_SCANNER_H_

#include <qcc/platform.h>
#include <map>

#include <qcc/Timer.h>
#include <qcc/Event.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <alljoyn/BusAttachment.h>

//
// We need JNI only when we are doing Java bindings
//

#if defined (QCC_OS_ANDROID)

#include <jni.h>

extern JavaVM* proxJVM;
extern JNIEnv* psenv;
extern jclass CLS_AllJoynAndroidExt;
extern jmethodID MID_AllJoynAndroidExt_Scan;
extern jclass CLS_ScanResultMessage;

#endif

class ProximityScanEngine;

namespace ajn {

class ProximityScanner {

  public:

    ProximityScanner(BusAttachment& bus);
    //void Scan();
    void Scan(bool request_scan);
    void PrintBSSIDMap(std::map<qcc::String, qcc::String>);
    std::map<std::pair<qcc::String, qcc::String>, bool> scanResults;
    BusAttachment& bus;


};

}

#endif

