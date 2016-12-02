/**
 * @file
 *
 * This file just wraps platform specific header files that define the thread
 * abstraction interface.
 */

/******************************************************************************
 *
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#ifndef _OS_QCC_THREAD_H
#define _OS_QCC_THREAD_H

#include <qcc/platform.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

/**
 * Windows uses __stdcall for certain API calls
 */
#define STDCALL __stdcall

/**
 * Redefine Windows' Sleep function to match Linux.
 *
 * @param x     Number of seconds to sleep.
 */
#define sleep(x) Sleep(x)

namespace qcc {

typedef HANDLE ThreadHandle;            ///< Window process handle typedef.
typedef unsigned int ThreadId;          ///< Windows uses uint process IDs.

/**
 * Windows' thread function return type
 */
typedef unsigned int ThreadInternalReturn;

}

#endif