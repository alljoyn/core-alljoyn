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

#include "DeferredCallback.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

namespace ajn {
bool DeferredCallback::initilized = false;
std::list<DeferredCallback*> DeferredCallback::sPendingCallbacks;
qcc::Thread* DeferredCallback::sMainThread = NULL;
bool DeferredCallback::sMainThreadCallbacksOnly = false;
qcc::Mutex* DeferredCallback::sCallbackListLock = nullptr;

}

int AJ_CALL alljoyn_unity_deferred_callbacks_process()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ajn::DeferredCallback::TriggerCallbacks();
}

void AJ_CALL alljoyn_unity_set_deferred_callback_mainthread_only(QCC_BOOL mainthread_only)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::DeferredCallback::sMainThreadCallbacksOnly = (mainthread_only == QCC_TRUE ? true : false);
}