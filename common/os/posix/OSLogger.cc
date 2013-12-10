/**
 * @file
 *
 * Platform specific logger for posix platforms
 */

/******************************************************************************
 *
 * Copyright (c) 2012-2013, AllSeen Alliance. All rights reserved.
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
    return NULL;
}

#endif
