/**
 * @file
 * This implements the C accessable version of the SessionListener class using
 * function pointers, and a pass-through implementation of SessionListener.
 */

/******************************************************************************
 * Copyright (c) 2009-2014 AllSeen Alliance. All rights reserved.
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

#include <alljoyn/SessionListener.h>
#include <alljoyn_c/SessionListener.h>
#include <string.h>
#include <assert.h>
#include "DeferredCallback.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

namespace ajn {

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of session related events.
 */
class SessionListenerCallbackC : public SessionListener {
  public:
    SessionListenerCallbackC(const alljoyn_sessionlistener_callbacks* in_callbacks, const void* in_context)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        memcpy(&callbacks, in_callbacks, sizeof(alljoyn_sessionlistener_callbacks));
        context = in_context;
    }

    virtual void SessionLost(SessionId sessionId, SessionLostReason reason)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.session_lost != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.session_lost(context, sessionId, static_cast<alljoyn_sessionlostreason>(reason));
            } else {
                DeferredCallback_3<void, const void*, SessionId, alljoyn_sessionlostreason>* dcb =
                    new DeferredCallback_3<void, const void*, SessionId, alljoyn_sessionlostreason>(callbacks.session_lost, context, sessionId, static_cast<alljoyn_sessionlostreason>(reason));
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    virtual void SessionMemberAdded(SessionId sessionId, const char* uniqueName)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.session_member_added != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.session_member_added(context, sessionId, uniqueName);
            } else {
                DeferredCallback_3<void, const void*, SessionId, const char*>* dcb =
                    new DeferredCallback_3<void, const void*, SessionId, const char*>(callbacks.session_member_added, context, sessionId, uniqueName);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.session_member_removed != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.session_member_removed(context, sessionId, uniqueName);
            } else {
                DeferredCallback_3<void, const void*, SessionId, const char*>* dcb =
                    new DeferredCallback_3<void, const void*, SessionId, const char*>(callbacks.session_member_removed, context, sessionId, uniqueName);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }
  protected:
    alljoyn_sessionlistener_callbacks callbacks;
    const void* context;
};

}

struct _alljoyn_sessionlistener_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_sessionlistener AJ_CALL alljoyn_sessionlistener_create(const alljoyn_sessionlistener_callbacks* callbacks, const void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_sessionlistener) new ajn::SessionListenerCallbackC(callbacks, context);
}

void AJ_CALL alljoyn_sessionlistener_destroy(alljoyn_sessionlistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(listener != NULL && "listener parameter must not be NULL");
    delete (ajn::SessionListenerCallbackC*)listener;
}
