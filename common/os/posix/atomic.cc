/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 *
 ******************************************************************************/
#if defined(QCC_MAEMO) and defined(QCC_X86)

#include <qcc/platform.h>

#include <pthread.h>

#include <qcc/atomic.h>

static pthread_mutex_t atomicLock = PTHREAD_MUTEX_INITIALIZER;

namespace qcc {

int32_t IncrementAndFetch(volatile int32_t* mem)
{
    int32_t ret;

    pthread_mutex_lock(&atomicLock);
    ret = ++(*mem);
    pthread_mutex_unlock(&atomicLock);
    return ret;
}

int32_t DecrementAndFetch(volatile int32_t* mem)
{
    int32_t ret;

    pthread_mutex_lock(&atomicLock);
    ret = --(*mem);
    pthread_mutex_unlock(&atomicLock);
    return ret;
}

}

#endif