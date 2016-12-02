/**
 * @file
 *
 * Platform specific logger for posix platforms
 */

/******************************************************************************
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 *
 *****************************************************************************/

#include <qcc/platform.h>

#include <qcc/OSLogger.h>

#if defined (QCC_OS_ANDROID)

#include <android/log.h>

static void AndroidLogCB(DbgMsgType type, const char* module, const char* msg, void* context)
{
    int priority;
    switch (type) {
    case DBG_LOCAL_ERROR:
    case DBG_REMOTE_ERROR:
        priority = ANDROID_LOG_ERROR;
        break;

    case DBG_HIGH_LEVEL:
        priority = ANDROID_LOG_INFO;
        break;

    case DBG_GEN_MESSAGE:
        priority = ANDROID_LOG_DEBUG;
        break;

    case DBG_API_TRACE:
        priority = ANDROID_LOG_VERBOSE;
        break;

    default:
    case DBG_REMOTE_DATA:
    case DBG_LOCAL_DATA:
        priority = ANDROID_LOG_VERBOSE;
        break;
    }
    __android_log_write(priority, module, msg);
}

QCC_DbgMsgCallback QCC_GetOSLogger(bool useOSLog)
{
    return useOSLog ? AndroidLogCB : NULL;
}

#else // plain posix

QCC_DbgMsgCallback QCC_GetOSLogger(bool useOSLog)
{
    QCC_UNUSED(useOSLog);
    return NULL;
}

#endif