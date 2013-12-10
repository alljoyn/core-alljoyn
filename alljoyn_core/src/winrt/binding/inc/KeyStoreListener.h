/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#pragma once

#include <alljoyn/KeyStoreListener.h>
#include <qcc/ManagedObj.h>

namespace AllJoyn {

ref class BusAttachment;

public delegate Platform::String ^ KeyStoreListenerGetKeysHandler();
public delegate Platform::String ^ KeyStoreListenerGetPasswordHandler();
public delegate void KeyStoreListenerPutKeysHandler(Platform::String ^ keys);

ref class __KeyStoreListener {
  private:
    friend ref class KeyStoreListener;
    friend class _KeyStoreListener;
    __KeyStoreListener();
    ~__KeyStoreListener();

    event KeyStoreListenerGetKeysHandler ^ GetKeys;
    event KeyStoreListenerGetPasswordHandler ^ GetPassword;
    event KeyStoreListenerPutKeysHandler ^ PutKeys;
    property BusAttachment ^ Bus;
};

class _KeyStoreListener : protected ajn::KeyStoreListener {
  protected:
    friend class qcc::ManagedObj<_KeyStoreListener>;
    friend ref class KeyStoreListener;
    friend ref class BusAttachment;
    _KeyStoreListener(BusAttachment ^ bus);
    ~_KeyStoreListener();

    Platform::String ^ DefaultKeyStoreListenerGetKeysHandler();
    Platform::String ^ DefaultKeyStoreListenerGetPasswordHandler();
    void DefaultKeyStoreListenerPutKeysHandler(Platform::String ^ keys);
    ::QStatus LoadRequest(ajn::KeyStore& keyStore);
    ::QStatus StoreRequest(ajn::KeyStore& keyStore);

    __KeyStoreListener ^ _eventsAndProperties;
};

public ref class KeyStoreListener sealed {
  public:
    KeyStoreListener(BusAttachment ^ bus);

    event KeyStoreListenerGetKeysHandler ^ GetKeys
    {
        Windows::Foundation::EventRegistrationToken add(KeyStoreListenerGetKeysHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        Platform::String ^ raise();
    }

    event KeyStoreListenerGetPasswordHandler ^ GetPassword
    {
        Windows::Foundation::EventRegistrationToken add(KeyStoreListenerGetPasswordHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        Platform::String ^ raise();
    }

    event KeyStoreListenerPutKeysHandler ^ PutKeys
    {
        Windows::Foundation::EventRegistrationToken add(KeyStoreListenerPutKeysHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(Platform::String ^ keys);
    }

    property BusAttachment ^ Bus
    {
        BusAttachment ^ get();
    }

  private:
    friend ref class BusAttachment;
    KeyStoreListener(const qcc::ManagedObj<_KeyStoreListener>* listener);
    ~KeyStoreListener();

    qcc::ManagedObj<_KeyStoreListener>* _mListener;
    _KeyStoreListener* _listener;
};

}
// KeyStoreListener.h
