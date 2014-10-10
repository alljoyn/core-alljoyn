/**
 * @file
 * This implements the C accessable version of the BusListener class using
 * function pointers, and a pass-through implementation of BusListener.
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

#include <alljoyn/BusListener.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn_c/BusListener.h>
#include <string.h>
#include <assert.h>
#include "DeferredCallback.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

static void AJ_CALL __PropertyChanged(alljoyn_buslistener_bus_prop_changed_ptr fcn, const void* context, const char* prop_name, alljoyn_msgarg prop_value)
{
    fcn(context, prop_name, prop_value);

    if (prop_value) {
        alljoyn_msgarg_destroy(prop_value);
    }
}


namespace ajn {

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of bus related events.
 */
class BusListenerCallbackC : BusListener {
  public:
    BusListenerCallbackC(const alljoyn_buslistener_callbacks* in_callbacks, const void* in_context)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        memcpy(&callbacks, in_callbacks, sizeof(alljoyn_buslistener_callbacks));
        context = in_context;
    }

    void ListenerRegistered(BusAttachment* bus)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.listener_registered != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.listener_registered(context, (alljoyn_busattachment)bus);
            } else {
                DeferredCallback_2<void, const void*, alljoyn_busattachment>* dcb =
                    new DeferredCallback_2<void, const void*, alljoyn_busattachment>(callbacks.listener_registered, context, (alljoyn_busattachment)bus);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    void ListenerUnregistered()
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.listener_unregistered != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.listener_unregistered(context);
            } else {
                DeferredCallback_1<void, const void*>* dcb =
                    new DeferredCallback_1<void, const void*>(callbacks.listener_unregistered, context);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.found_advertised_name != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.found_advertised_name(context, name, transport, namePrefix);
            } else {
                DeferredCallback_4<void, const void*, const char*, TransportMask, const char*>* dcb =
                    new DeferredCallback_4<void, const void*, const char*, TransportMask, const char*>(callbacks.found_advertised_name, context, name, transport, namePrefix);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.lost_advertised_name != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.lost_advertised_name(context, name, transport, namePrefix);
            } else {
                DeferredCallback_4<void, const void*, const char*, TransportMask, const char*>* dcb =
                    new DeferredCallback_4<void, const void*, const char*, TransportMask, const char*>(callbacks.lost_advertised_name, context, name, transport, namePrefix);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.name_owner_changed != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.name_owner_changed(context, busName, previousOwner, newOwner);
            } else {
                DeferredCallback_4<void, const void*, const char*, const char*, const char*>* dcb =
                    new DeferredCallback_4<void, const void*, const char*, const char*, const char*>(callbacks.name_owner_changed, context, busName, previousOwner, newOwner);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    void BusStopping()
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.bus_stopping != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.bus_stopping(context);
            } else {
                DeferredCallback_1<void, const void*>* dcb =
                    new DeferredCallback_1<void, const void*>(callbacks.bus_stopping, context);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    void BusDisconnected()
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.bus_disconnected != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.bus_disconnected(context);
            } else {
                DeferredCallback_1<void, const void*>* dcb =
                    new DeferredCallback_1<void, const void*>(callbacks.bus_disconnected, context);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    void PropertyChanged(const char* propName, const MsgArg* propValue)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.property_changed != NULL) {
            alljoyn_msgarg msg_arg = propValue ? alljoyn_msgarg_create_and_set(propValue->Signature().c_str(), propValue->v_variant.val) : NULL;
            // must wrap the user function in order to properly clean up msg_arg
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                __PropertyChanged(callbacks.property_changed, context, propName, msg_arg);
            } else {
                DeferredCallback_4<void, alljoyn_buslistener_bus_prop_changed_ptr, const void*, const char*, alljoyn_msgarg>* dcb =
                    new DeferredCallback_4<void, alljoyn_buslistener_bus_prop_changed_ptr, const void*, const char*, alljoyn_msgarg>(
                        __PropertyChanged, callbacks.property_changed, context, propName, msg_arg);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

  protected:
    alljoyn_buslistener_callbacks callbacks;
    const void* context;
};

}

struct _alljoyn_buslistener_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_buslistener AJ_CALL alljoyn_buslistener_create(const alljoyn_buslistener_callbacks* callbacks, const void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_buslistener) new ajn::BusListenerCallbackC(callbacks, context);
}

void AJ_CALL alljoyn_buslistener_destroy(alljoyn_buslistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(listener != NULL && "listener parameter must not be NULL");
    delete (ajn::BusListenerCallbackC*)listener;
}

