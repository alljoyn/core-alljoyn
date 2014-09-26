/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

#include "DeferredCallback.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

namespace ajn {
bool DeferredCallback::initilized = false;
std::list<DeferredCallback*> DeferredCallback::sPendingCallbacks;
qcc::Thread* DeferredCallback::sMainThread = NULL;
bool DeferredCallback::sMainThreadCallbacksOnly = false;
qcc::Mutex DeferredCallback::sCallbackListLock;

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
