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

#include "KeyStoreListener.h"

#include <BusAttachment.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <ObjectReference.h>
#include <AllJoynException.h>

namespace AllJoyn {

KeyStoreListener::KeyStoreListener(BusAttachment ^ bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check bus for invalid values
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create _KeyStoreListener managed object
        _mListener = new qcc::ManagedObj<_KeyStoreListener>(bus);
        // Check for allocation error
        if (NULL == _mListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _KeyStoreListener for convenience
        _listener = &(**_mListener);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

KeyStoreListener::KeyStoreListener(const qcc::ManagedObj<_KeyStoreListener>* listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check listener for invalid values
        if (NULL == listener) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attach listener to managed object
        _mListener = new qcc::ManagedObj<_KeyStoreListener>(*listener);
        if (NULL == _mListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _KeyStoreListener for convenience
        _listener = &(**_mListener);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

KeyStoreListener::~KeyStoreListener()
{
    // Delete managed _KeyStoreListener to adjust ref count
    if (NULL != _mListener) {
        delete _mListener;
        _mListener = NULL;
        _listener = NULL;
    }
}

Windows::Foundation::EventRegistrationToken KeyStoreListener::GetKeys::add(KeyStoreListenerGetKeysHandler ^ handler)
{
    // Add a handler for GetKeys
    return _listener->_eventsAndProperties->GetKeys::add(handler);
}

void KeyStoreListener::GetKeys::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove a handler for GetKeys
    _listener->_eventsAndProperties->GetKeys::remove(token);
}

Platform::String ^ KeyStoreListener::GetKeys::raise()
{
    // Invoke a handler to GetKeys
    return _listener->_eventsAndProperties->GetKeys::raise();
}

Windows::Foundation::EventRegistrationToken KeyStoreListener::GetPassword::add(KeyStoreListenerGetPasswordHandler ^ handler)
{
    // Add a handler for GetPassword
    return _listener->_eventsAndProperties->GetPassword::add(handler);
}

void KeyStoreListener::GetPassword::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove a handler for GetPassword
    _listener->_eventsAndProperties->GetPassword::remove(token);
}

Platform::String ^ KeyStoreListener::GetPassword::raise()
{
    // Invoke a handler for GetPassword
    return _listener->_eventsAndProperties->GetPassword::raise();
}

Windows::Foundation::EventRegistrationToken KeyStoreListener::PutKeys::add(KeyStoreListenerPutKeysHandler ^ handler)
{
    // Add a handler for PutKeys
    return _listener->_eventsAndProperties->PutKeys::add(handler);
}

void KeyStoreListener::PutKeys::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove a handler for PutKeys
    _listener->_eventsAndProperties->PutKeys::remove(token);
}

void KeyStoreListener::PutKeys::raise(Platform::String ^ keys)
{
    // Invoke a handler for PutKeys
    return _listener->_eventsAndProperties->PutKeys::raise(keys);
}

BusAttachment ^ KeyStoreListener::Bus::get()
{
    // Get BusAttachment from the internal ref class
    return _listener->_eventsAndProperties->Bus;
}

_KeyStoreListener::_KeyStoreListener(BusAttachment ^ bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create internal ref class
        _eventsAndProperties = ref new __KeyStoreListener();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Add default handler for GetKeys
        _eventsAndProperties->GetKeys += ref new KeyStoreListenerGetKeysHandler([&] ()->Platform::String ^ {
                                                                                    return DefaultKeyStoreListenerGetKeysHandler();
                                                                                });
        // Add default handler for GetPassword
        _eventsAndProperties->GetPassword += ref new KeyStoreListenerGetPasswordHandler([&] ()->Platform::String ^ {
                                                                                            return DefaultKeyStoreListenerGetPasswordHandler();
                                                                                        });
        // Add default handler for PutKeys
        _eventsAndProperties->PutKeys += ref new KeyStoreListenerPutKeysHandler([&] (Platform::String ^ keys) {
                                                                                    DefaultKeyStoreListenerPutKeysHandler(keys);
                                                                                });
        // Store BusAttachment in the private ref class
        _eventsAndProperties->Bus = bus;
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_KeyStoreListener::~_KeyStoreListener()
{
    _eventsAndProperties = nullptr;
}

Platform::String ^ _KeyStoreListener::DefaultKeyStoreListenerGetKeysHandler()
{
    return nullptr;
}

Platform::String ^ _KeyStoreListener::DefaultKeyStoreListenerGetPasswordHandler()
{
    return nullptr;
}

void _KeyStoreListener::DefaultKeyStoreListenerPutKeysHandler(Platform::String ^ keys)
{
}

::QStatus _KeyStoreListener::LoadRequest(ajn::KeyStore& keyStore)
{
    ::QStatus status = ER_FAIL;

    // Call the PutKeys handler through the dispatcher
    _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                 while (true) {
                                                                                                                     // Get the keys
                                                                                                                     Platform::String ^ source = _eventsAndProperties->GetKeys();
                                                                                                                     // Check for no keys
                                                                                                                     if (nullptr == source) {
                                                                                                                         status = ER_FAIL;
                                                                                                                         break;
                                                                                                                     }
                                                                                                                     // Convert source to qcc::String
                                                                                                                     qcc::String strSource = PlatformToMultibyteString(source);
                                                                                                                     // Check for conversion error
                                                                                                                     if (strSource.empty()) {
                                                                                                                         status = ER_OUT_OF_MEMORY;
                                                                                                                         break;
                                                                                                                     }
                                                                                                                     // Get the password
                                                                                                                     Platform::String ^ password = _eventsAndProperties->GetPassword();
                                                                                                                     // Check for no password
                                                                                                                     if (nullptr == password) {
                                                                                                                         status = ER_FAIL;
                                                                                                                         break;
                                                                                                                     }
                                                                                                                     // Convert password to qcc::String
                                                                                                                     qcc::String strPassword = PlatformToMultibyteString(password);
                                                                                                                     // Check for conversion error
                                                                                                                     if (strPassword.empty()) {
                                                                                                                         status = ER_OUT_OF_MEMORY;
                                                                                                                         break;
                                                                                                                     }
                                                                                                                     // Call the handler
                                                                                                                     status = PutKeys(keyStore, strSource, strPassword);
                                                                                                                     break;
                                                                                                                 }
                                                                                                             }));

    return status;
}

::QStatus _KeyStoreListener::StoreRequest(ajn::KeyStore& keyStore)
{
    ::QStatus status = ER_FAIL;

    // Call the StoreRequest handler through the dispatcher
    _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                 while (true) {
                                                                                                                     qcc::String sink;
                                                                                                                     // Call the real API
                                                                                                                     status = GetKeys(keyStore, sink);
                                                                                                                     // Check for failure
                                                                                                                     if (ER_OK != status) {
                                                                                                                         break;
                                                                                                                     }
                                                                                                                     // Convert sink to Platform::String
                                                                                                                     Platform::String ^ strSink = MultibyteToPlatformString(sink.c_str());
                                                                                                                     // Check for conversion failure
                                                                                                                     if (nullptr == strSink && !sink.empty()) {
                                                                                                                         status = ER_OUT_OF_MEMORY;
                                                                                                                         break;
                                                                                                                     }
                                                                                                                     // Call the handler
                                                                                                                     _eventsAndProperties->PutKeys(strSink);
                                                                                                                     status = ER_OK;
                                                                                                                     break;
                                                                                                                 }
                                                                                                             }));

    return status;
}

__KeyStoreListener::__KeyStoreListener()
{
    Bus = nullptr;
}

__KeyStoreListener::~__KeyStoreListener()
{
    Bus = nullptr;
}

}
