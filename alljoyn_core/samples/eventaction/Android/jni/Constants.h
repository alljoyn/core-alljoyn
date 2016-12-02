/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#ifndef _CONSTANTS_
#define _CONSTANTS_

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <qcc/Log.h>
#include <new>

#ifndef LOGTHIS
#if TARGET_ANDROID
#include <jni.h>
#include <android/log.h>

#define LOG_TAG  "JNI_EventActionBrowser"
#define LOGTHIS(...) (__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#else
#define LOGTHIS(...)
#endif
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#endif //_CONSTANTS