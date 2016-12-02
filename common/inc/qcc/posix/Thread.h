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

#include <pthread.h>
#include <sys/select.h>

/* Linux does not use Windows conventions for STDCALL and CDECL */
#define STDCALL

namespace qcc {

typedef pthread_t ThreadHandle;        ///< Linux uses pthreads under the hood.
typedef ThreadHandle ThreadId;         ///< Linux thread IDs are the thread handles.
typedef void* ThreadInternalReturn;    ///< Return type for pthreads.
}

#endif