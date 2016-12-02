/**
 * @file
 */
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

#include <alljoyn_c/AboutListener.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/MsgArg.h>
#include "DeferredCallback.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

namespace ajn {

class AboutListenerCallbackC : AboutListener {
  public:
    AboutListenerCallbackC(const alljoyn_aboutlistener_callback* in_callback, const void* in_context)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        memcpy(&callback, in_callback, sizeof(alljoyn_aboutlistener_callback));
        context = in_context;
    }

    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callback.about_listener_announced != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callback.about_listener_announced(context, busName, version, port, (alljoyn_msgarg) & objectDescriptionArg, (alljoyn_msgarg) & aboutDataArg);
            } else {
                DeferredCallback_6<void,
                                   const void*,
                                   const char*,
                                   uint16_t,
                                   alljoyn_sessionport,
                                   alljoyn_msgarg,
                                   alljoyn_msgarg>* dcb =
                    new DeferredCallback_6<void,
                                           const void*,
                                           const char*,
                                           uint16_t,
                                           alljoyn_sessionport,
                                           alljoyn_msgarg,
                                           alljoyn_msgarg>(
                        callback.about_listener_announced,
                        context,
                        busName,
                        version,
                        port,
                        (alljoyn_msgarg) & objectDescriptionArg,
                        (alljoyn_msgarg) & aboutDataArg);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

  protected:
    alljoyn_aboutlistener_callback callback;
    const void* context;
};

}

struct _alljoyn_aboutlistener_handle {
    /* Empty by design */
};

alljoyn_aboutlistener AJ_CALL alljoyn_aboutlistener_create(const alljoyn_aboutlistener_callback* callback,
                                                           const void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_aboutlistener) new ajn::AboutListenerCallbackC(callback, context);
}

void AJ_CALL alljoyn_aboutlistener_destroy(alljoyn_aboutlistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::AboutListenerCallbackC*)listener;
}
