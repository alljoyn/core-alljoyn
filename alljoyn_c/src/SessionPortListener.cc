/**
 * @file
 * This implements the C accessable version of the SessionPortListener class using
 * function pointers, and a pass-through implementation of SessionPortListener.
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

#include <alljoyn/SessionPortListener.h>
#include <alljoyn_c/SessionPortListener.h>
#include <string.h>
#include <assert.h>
#include "DeferredCallback.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

namespace ajn {

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of session port related events.
 */
class SessionPortListenerCallbackC : public SessionPortListener {
  public:
    SessionPortListenerCallbackC(const alljoyn_sessionportlistener_callbacks* in_callbacks, const void* in_context)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        memcpy(&callbacks, in_callbacks, sizeof(alljoyn_sessionportlistener_callbacks));
        context = in_context;
    }

    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        QCC_BOOL ret = SessionPortListener::AcceptSessionJoiner(sessionPort, joiner, opts) ? QCC_TRUE : QCC_FALSE;
        if (callbacks.accept_session_joiner != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                ret = callbacks.accept_session_joiner(context, sessionPort, joiner, (alljoyn_sessionopts)(&opts));
            } else {
                DeferredCallback_4<QCC_BOOL, const void*, SessionPort, const char*, alljoyn_sessionopts>* dcb =
                    new DeferredCallback_4<QCC_BOOL, const void*, SessionPort, const char*, alljoyn_sessionopts>(callbacks.accept_session_joiner, context, sessionPort, joiner, (alljoyn_sessionopts)(&opts));
                ret = DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
        return (ret == QCC_FALSE ? false : true);
    }

    virtual void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.session_joined != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.session_joined(context, sessionPort, id, joiner);
            } else {
                DeferredCallback_4<void, const void*, SessionPort, SessionId, const char*>* dcb =
                    new DeferredCallback_4<void, const void*, SessionPort, SessionId, const char*>(callbacks.session_joined, context, sessionPort, id, joiner);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }
  protected:
    alljoyn_sessionportlistener_callbacks callbacks;
    const void* context;
};

}

struct _alljoyn_sessionportlistener_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_sessionportlistener AJ_CALL alljoyn_sessionportlistener_create(const alljoyn_sessionportlistener_callbacks* callbacks, const void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_sessionportlistener) new ajn::SessionPortListenerCallbackC(callbacks, context);
}

void AJ_CALL alljoyn_sessionportlistener_destroy(alljoyn_sessionportlistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(listener != NULL && "listener parameter must not be NULL");
    delete (ajn::SessionPortListenerCallbackC*)listener;
}
