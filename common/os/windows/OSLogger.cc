/**
 * @file
 *
 * Platform specific logger for Windows
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

#include <stdio.h>

#include <qcc/OSLogger.h>

static void WindowsLogCB(DbgMsgType type, const char* module, const char* msg, void* context)
{
    QCC_UNUSED(type);
    QCC_UNUSED(module);
    QCC_UNUSED(context);
    OutputDebugStringA(msg);
}

QCC_DbgMsgCallback QCC_GetOSLogger(bool useOSLog)
{
    return useOSLog ? WindowsLogCB : NULL;
}