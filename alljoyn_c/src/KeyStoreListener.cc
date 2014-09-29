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

#include <alljoyn/KeyStoreListener.h>
#include <alljoyn_c/KeyStoreListener.h>
#include <string.h>
#include <assert.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

namespace ajn {

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of keystore related events.
 */
class KeyStoreListenerCallbackC : public KeyStoreListener {
  public:
    KeyStoreListenerCallbackC(const alljoyn_keystorelistener_callbacks* in_callbacks, const void* in_context)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        memcpy(&callbacks, in_callbacks, sizeof(alljoyn_keystorelistener_callbacks));
        context = in_context;
    }

    virtual QStatus LoadRequest(KeyStore& keyStore)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        assert(callbacks.load_request != NULL && "load_request callback required.");
        return callbacks.load_request(context, (alljoyn_keystorelistener) this, (alljoyn_keystore)(&keyStore));
    }

    virtual QStatus StoreRequest(KeyStore& keyStore)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        assert(callbacks.store_request != NULL && "store_request callback required.");
        return callbacks.store_request(context, (alljoyn_keystorelistener) this, (alljoyn_keystore)(&keyStore));
    }
  protected:
    alljoyn_keystorelistener_callbacks callbacks;
    const void* context;
};

}

struct _alljoyn_keystorelistener_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_keystorelistener AJ_CALL alljoyn_keystorelistener_create(const alljoyn_keystorelistener_callbacks* callbacks, const void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(callbacks->load_request != NULL && "load_request callback required.");
    assert(callbacks->store_request != NULL && "store_request callback required.");
    return (alljoyn_keystorelistener) new ajn::KeyStoreListenerCallbackC(callbacks, context);
}

void AJ_CALL alljoyn_keystorelistener_destroy(alljoyn_keystorelistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(listener != NULL && "listener parameter must not be NULL");
    delete (ajn::KeyStoreListenerCallbackC*)listener;
}

QStatus AJ_CALL alljoyn_keystorelistener_putkeys(alljoyn_keystorelistener listener, alljoyn_keystore keyStore,
                                                 const char* source, const char* password)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::KeyStore& ks = *((ajn::KeyStore*)keyStore);
    return ((ajn::KeyStoreListener*)listener)->PutKeys(ks, source, password);
}

QStatus AJ_CALL alljoyn_keystorelistener_getkeys(alljoyn_keystorelistener listener, alljoyn_keystore keyStore,
                                                 char* sink, size_t* sink_sz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String sinkStr;
    ajn::KeyStore& ks = *((ajn::KeyStore*)keyStore);
    QStatus ret = ((ajn::KeyStoreListener*)listener)->GetKeys(ks, sinkStr);
    if (sink) {
        strncpy(sink, sinkStr.c_str(), *sink_sz);
        //prevent sting not being nul terminated.
        sink[*sink_sz - 1] = '\0';
    }
    if (*sink_sz < sinkStr.length() + 1) {
        ret = ER_BUFFER_TOO_SMALL;
    }
    *sink_sz = sinkStr.length() + 1;
    return ret;
}
