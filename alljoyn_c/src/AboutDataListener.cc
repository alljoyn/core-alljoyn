/**
 * @file
 * This code is experimental, and as such has not been fully tested.
 * Please help make it more robust by contributing fixes if you find problems.
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include <alljoyn_c/AboutDataListener.h>
#include <alljoyn/AboutDataListener.h>
#include <alljoyn_c/MsgArg.h>
#include <alljoyn/MsgArg.h>
#include "DeferredCallback.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_ABOUT_DATA_LISTENER_C"

namespace ajn {

class AboutDataListenerCallbackC : AboutDataListener {
  public:
    AboutDataListenerCallbackC(const alljoyn_aboutdatalistener_callbacks* in_callbacks, const void* in_context)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        memcpy(&callbacks, in_callbacks, sizeof(alljoyn_aboutdatalistener_callbacks));
        context = in_context;
    }

    QStatus GetAboutData(MsgArg* msgArg, const char* language)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        QStatus ret = ER_OK;
        if (callbacks.about_datalistener_getaboutdata != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                ret = callbacks.about_datalistener_getaboutdata(context, (alljoyn_msgarg)msgArg, language);
            } else {
                DeferredCallback_3<QStatus, const void*, alljoyn_msgarg, const char*>* dcb =
                    new DeferredCallback_3<QStatus, const void*, alljoyn_msgarg, const char*>(callbacks.about_datalistener_getaboutdata,
                                                                                              context, (alljoyn_msgarg)msgArg, language);
                ret = DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
        return ret;
    }

    QStatus GetAnnouncedAboutData(MsgArg* msgArg)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        QStatus ret = ER_OK;
        if (callbacks.about_datalistener_getannouncedaboutdata != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                ret = callbacks.about_datalistener_getannouncedaboutdata(context, (alljoyn_msgarg)msgArg);
            } else {
                DeferredCallback_2<QStatus, const void*, alljoyn_msgarg>* dcb =
                    new DeferredCallback_2<QStatus, const void*, alljoyn_msgarg>(callbacks.about_datalistener_getannouncedaboutdata,
                                                                                 context, (alljoyn_msgarg)msgArg);
                ret = DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
        return ret;
    }

  protected:
    alljoyn_aboutdatalistener_callbacks callbacks;
    const void* context;
};

}

struct _alljoyn_aboutdatalistener_handle {
    /* Empty by design */
};

alljoyn_aboutdatalistener AJ_CALL alljoyn_aboutdatalistener_create(const alljoyn_aboutdatalistener_callbacks* callbacks,
                                                                   const void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_aboutdatalistener) new ajn::AboutDataListenerCallbackC(callbacks, context);
}

void AJ_CALL alljoyn_aboutdatalistener_destroy(alljoyn_aboutdatalistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::AboutDataListenerCallbackC*)listener;
}

