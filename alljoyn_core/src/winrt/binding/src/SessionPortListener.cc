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

#include "SessionPortListener.h"

#include <SessionOpts.h>
#include <ObjectReference.h>
#include <AllJoynException.h>
#include <BusAttachment.h>
#include <alljoyn/Status.h>

namespace AllJoyn {

SessionPortListener::SessionPortListener(BusAttachment ^ bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check bus for invalid values
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Create managed _SessionPortListener
        _mListener = new qcc::ManagedObj<_SessionPortListener>(bus);
        // Check for allocation error
        if (NULL == _mListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _SessionPortListener for convenience
        _listener = &(**_mListener);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

SessionPortListener::SessionPortListener(const qcc::ManagedObj<_SessionPortListener>* listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check listener for invalid values
        if (NULL == listener) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attach listener to managed _SessionPortListener
        _mListener = new qcc::ManagedObj<_SessionPortListener>(*listener);
        // Check for allocation error
        if (NULL == _mListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _SessionPortListener for convenience
        _listener = &(**_mListener);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

SessionPortListener::~SessionPortListener()
{
    // Delete managed _SessionPortListener to adjust ref count
    if (NULL != _mListener) {
        delete _mListener;
        _mListener = NULL;
        _listener = NULL;
    }
}

Windows::Foundation::EventRegistrationToken SessionPortListener::AcceptSessionJoiner::add(SessionPortListenerAcceptSessionJoinerHandler ^ handler)
{
    // Add handler for AcceptSessionJoiner
    return _listener->_eventsAndProperties->AcceptSessionJoiner::add(handler);
}

void SessionPortListener::AcceptSessionJoiner::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove handler for AcceptSessionJoiner
    _listener->_eventsAndProperties->AcceptSessionJoiner::remove(token);
}

bool SessionPortListener::AcceptSessionJoiner::raise(ajn::SessionPort sessionPort, Platform::String ^ joiner, SessionOpts ^ opts)
{
    // Invoke handler for AcceptSessionJoiner
    return _listener->_eventsAndProperties->AcceptSessionJoiner::raise(sessionPort, joiner, opts);
}

Windows::Foundation::EventRegistrationToken SessionPortListener::SessionJoined::add(SessionPortListenerSessionJoinedHandler ^ handler)
{
    // Add handler for SessionJoined
    return _listener->_eventsAndProperties->SessionJoined::add(handler);
}

void SessionPortListener::SessionJoined::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove handler for SessionJoined
    _listener->_eventsAndProperties->SessionJoined::remove(token);
}

void SessionPortListener::SessionJoined::raise(ajn::SessionPort sessionPort, ajn::SessionId id, Platform::String ^ joiner)
{
    // Invoke handler for SessionJoined
    _listener->_eventsAndProperties->SessionJoined::raise(sessionPort, id, joiner);
}

BusAttachment ^ SessionPortListener::Bus::get()
{
    // Return Bus from internal ref class
    return _listener->_eventsAndProperties->Bus;
}

_SessionPortListener::_SessionPortListener(BusAttachment ^ bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create internal ref class
        _eventsAndProperties = ref new __SessionPortListener();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Add default handler for AcceptSessionJoiner
        _eventsAndProperties->AcceptSessionJoiner += ref new SessionPortListenerAcceptSessionJoinerHandler([&] (ajn::SessionPort sessionPort, Platform::String ^ joiner, SessionOpts ^ opts)->bool {
                                                                                                               return DefaultSessionPortListenerAcceptSessionJoinerHandler(sessionPort, joiner, opts);
                                                                                                           });
        // Add default handler for SessionJoined
        _eventsAndProperties->SessionJoined += ref new SessionPortListenerSessionJoinedHandler([&] (ajn::SessionPort sessionPort, ajn::SessionId id, Platform::String ^ joiner) {
                                                                                                   DefaultSessionPortListenerSessionJoinedHandler(sessionPort, id, joiner);
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

_SessionPortListener::~_SessionPortListener()
{
    _eventsAndProperties = nullptr;
}

bool _SessionPortListener::DefaultSessionPortListenerAcceptSessionJoinerHandler(ajn::SessionPort sessionPort, Platform::String ^ joiner, SessionOpts ^ opts)
{
    ::QStatus status = ER_OK;
    bool result = false;

    while (true) {
        // Convert joiner to qcc::String
        qcc::String strJoiner = PlatformToMultibyteString(joiner);
        // Check for conversion failure
        if (nullptr != joiner && strJoiner.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get unmanaged SessionOpts
        ajn::SessionOpts* sessionOpts = opts->_sessionOpts;
        // Call the real API
        result = ajn::SessionPortListener::AcceptSessionJoiner(sessionPort, strJoiner.c_str(), *sessionOpts);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

void _SessionPortListener::DefaultSessionPortListenerSessionJoinedHandler(ajn::SessionPort sessionPort, ajn::SessionId id, Platform::String ^ joiner)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert joiner to qcc::String
        qcc::String strJoiner = PlatformToMultibyteString(joiner);
        // Check for conversion failure
        if (nullptr != joiner && strJoiner.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        ajn::SessionPortListener::SessionJoined(sessionPort, id, strJoiner.c_str());
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

bool _SessionPortListener::AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
{
    ::QStatus status = ER_OK;
    bool result = false;

    while (true) {
        // Convert joiner to Platform::String
        Platform::String ^ strJoiner = MultibyteToPlatformString(joiner);
        // Check for conversion failure
        if (nullptr == strJoiner && joiner != NULL && joiner[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create SessionOpts
        SessionOpts ^ sessionOpts = ref new SessionOpts(&opts);
        // Check for allocation error
        if (nullptr == sessionOpts) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call AccpetSessionJoiner handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     result = _eventsAndProperties->AcceptSessionJoiner(sessionPort, strJoiner, sessionOpts);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

void _SessionPortListener::SessionJoined(ajn::SessionPort sessionPort, ajn::SessionId id, const char* joiner)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert joiner to Platform::String
        Platform::String ^ strJoiner = MultibyteToPlatformString(joiner);
        // Check for conversion failure
        if (nullptr == strJoiner && joiner != NULL && joiner[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call SessionJoined handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->SessionJoined(sessionPort, id, strJoiner);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

__SessionPortListener::__SessionPortListener()
{
    Bus = nullptr;
}

__SessionPortListener::~__SessionPortListener()
{
    Bus = nullptr;
}

}
