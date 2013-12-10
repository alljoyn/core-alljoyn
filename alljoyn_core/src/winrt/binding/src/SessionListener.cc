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

#include "SessionListener.h"

#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <ObjectReference.h>
#include <AllJoynException.h>
#include <BusAttachment.h>
#include <alljoyn/Status.h>

namespace AllJoyn {

SessionListener::SessionListener(BusAttachment ^ bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check bus for invalid values
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create sl to managed _SessionListener
        _mListener = new qcc::ManagedObj<_SessionListener>(bus);
        // Check for allocation error
        if (NULL == _mListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _SessionListener for convenience
        _listener = &(**_mListener);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

SessionListener::SessionListener(const qcc::ManagedObj<_SessionListener>* listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check listener for invalid values
        if (NULL == listener) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attach listener to managed _SessionListener
        _mListener = new qcc::ManagedObj<_SessionListener>(*listener);
        // Check for allocation error
        if (NULL == _mListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _SessionListener for convenience
        _listener = &(**_mListener);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

SessionListener::~SessionListener()
{
    // Delete managed _SessionListener to adjust ref count
    if (NULL != _mListener) {
        delete _mListener;
        _mListener = NULL;
        _listener = NULL;
    }
}

Windows::Foundation::EventRegistrationToken SessionListener::SessionLost::add(SessionListenerSessionLostHandler ^ handler)
{
    // Add handler for SessionLost
    return _listener->_eventsAndProperties->SessionLost::add(handler);
}

void SessionListener::SessionLost::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove handler for SessionLost
    _listener->_eventsAndProperties->SessionLost::remove(token);
}

void SessionListener::SessionLost::raise(ajn::SessionId sessionId)
{
    // Invoke handler for SessionLost
    return _listener->_eventsAndProperties->SessionLost::raise(sessionId);
}

Windows::Foundation::EventRegistrationToken SessionListener::SessionMemberAdded::add(SessionListenerSessionMemberAddedHandler ^ handler)
{
    // Add handler for SessionMemberAdded
    return _listener->_eventsAndProperties->SessionMemberAdded::add(handler);
}

void SessionListener::SessionMemberAdded::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove handler for SessionMemberAdded
    _listener->_eventsAndProperties->SessionMemberAdded::remove(token);
}

void SessionListener::SessionMemberAdded::raise(ajn::SessionId sessionId, Platform::String ^ uniqueName)
{
    // Invoke handler for SessionMemberAdded
    return _listener->_eventsAndProperties->SessionMemberAdded::raise(sessionId, uniqueName);
}

Windows::Foundation::EventRegistrationToken SessionListener::SessionMemberRemoved::add(SessionListenerSessionMemberRemovedHandler ^ handler)
{
    // Add handler for SessionMemberRemoved
    return _listener->_eventsAndProperties->SessionMemberRemoved::add(handler);
}

void SessionListener::SessionMemberRemoved::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove handler for SessionMemberRemoved
    _listener->_eventsAndProperties->SessionMemberRemoved::remove(token);
}

void SessionListener::SessionMemberRemoved::raise(ajn::SessionId sessionId, Platform::String ^ uniqueName)
{
    // Invoke handler for SessionMemberRemoved
    return _listener->_eventsAndProperties->SessionMemberRemoved::raise(sessionId, uniqueName);
}

BusAttachment ^ SessionListener::Bus::get()
{
    // Return Bus from internal ref class
    return _listener->_eventsAndProperties->Bus;
}

_SessionListener::_SessionListener(BusAttachment ^ bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create private ref class
        _eventsAndProperties = ref new __SessionListener();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Add default handler for SessionLost
        _eventsAndProperties->SessionLost += ref new SessionListenerSessionLostHandler([&] (ajn::SessionId sessionId) {
                                                                                           DefaultSessionListenerSessionLostHandler(sessionId);
                                                                                       });
        // Add default handler for SessionMemberAdded
        _eventsAndProperties->SessionMemberAdded += ref new SessionListenerSessionMemberAddedHandler([&] (ajn::SessionId sessionId, Platform::String ^ uniqueName) {
                                                                                                         DefaultSessionListenerSessionMemberAddedHandler(sessionId, uniqueName);
                                                                                                     });
        // Add default handler for SessionMemberRemoved
        _eventsAndProperties->SessionMemberRemoved += ref new SessionListenerSessionMemberRemovedHandler([&] (ajn::SessionId sessionId, Platform::String ^ uniqueName) {
                                                                                                             DefaultSessionListenerSessionMemberRemovedHandler(sessionId, uniqueName);
                                                                                                         });
        // Store BusAttachment
        _eventsAndProperties->Bus = bus;
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_SessionListener::~_SessionListener()
{
    _eventsAndProperties = nullptr;
}

void _SessionListener::DefaultSessionListenerSessionLostHandler(ajn::SessionId sessionId)
{
    // Call the real API
    ajn::SessionListener::SessionLost(sessionId);
}

void _SessionListener::DefaultSessionListenerSessionMemberAddedHandler(ajn::SessionId sessionId, Platform::String ^ uniqueName)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert uniqueName to qcc::String
        qcc::String strUniqueName = PlatformToMultibyteString(uniqueName);
        // Check for conversion error
        if (nullptr != uniqueName && strUniqueName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        ajn::SessionListener::SessionMemberAdded(sessionId, strUniqueName.c_str());
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _SessionListener::DefaultSessionListenerSessionMemberRemovedHandler(ajn::SessionId sessionId, Platform::String ^ uniqueName)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert uniqueName to qcc::String
        qcc::String strUniqueName = PlatformToMultibyteString(uniqueName);
        // Check for conversion error
        if (nullptr != uniqueName && strUniqueName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        ajn::SessionListener::SessionMemberRemoved(sessionId, strUniqueName.c_str());
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _SessionListener::SessionLost(ajn::SessionId sessionId)
{
    // Call the SessionLost handler through the dispatcher
    _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                 _eventsAndProperties->SessionLost(sessionId);
                                                                                                             }));
}

void _SessionListener::SessionMemberAdded(ajn::SessionId sessionId,  const char* uniqueName)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert uniqueName to Platform::String
        Platform::String ^ strUniqueName = MultibyteToPlatformString(uniqueName);
        // Check for conversion error
        if (nullptr == strUniqueName && NULL != uniqueName && uniqueName[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the SessionMemberAdded handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->SessionMemberAdded(sessionId, strUniqueName);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _SessionListener::SessionMemberRemoved(ajn::SessionId sessionId,  const char* uniqueName)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert uniqueName to Platform::String
        Platform::String ^ strUniqueName = MultibyteToPlatformString(uniqueName);
        // Check for conversion error
        if (nullptr == strUniqueName && NULL != uniqueName && uniqueName[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the SessionMemberRemoved handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->SessionMemberRemoved(sessionId, strUniqueName);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

__SessionListener::__SessionListener()
{
    Bus = nullptr;
}

__SessionListener::~__SessionListener()
{
    Bus = nullptr;
}

}
