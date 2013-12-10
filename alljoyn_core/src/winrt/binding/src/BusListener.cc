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

#include "BusListener.h"

#include <qcc/String.h>
#include <BusAttachment.h>
#include <qcc/winrt/utility.h>
#include <ObjectReference.h>
#include <AllJoynException.h>
#include <ctxtcall.h>
#include <ppltasks.h>

namespace AllJoyn {

BusListener::BusListener(BusAttachment ^ bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in bus
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Create the managed reference to _BusListener
        _mListener = new qcc::ManagedObj<_BusListener>(bus);
        if (NULL == _mListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store the unmanaged pointer to the listener for convenience
        _listener = &(**_mListener);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

BusListener::BusListener(const qcc::ManagedObj<_BusListener>* listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in listener
        if (NULL == listener) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attached to the managed pointer
        _mListener = new qcc::ManagedObj<_BusListener>(*listener);
        if (NULL == _mListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store the unmanaged pointer to the listener for convenience
        _listener = &(**_mListener);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

BusListener::~BusListener()
{
    // Delete the managed pointer to adjust the ref count
    if (NULL != _mListener) {
        delete _mListener;
        _mListener = NULL;
        _listener = NULL;
    }
}

Windows::Foundation::EventRegistrationToken BusListener::ListenerRegistered::add(BusListenerListenerRegisteredHandler ^ handler)
{
    // Add a handler for ListenerRegistered to the interal ref class
    return _listener->_eventsAndProperties->ListenerRegistered::add(handler);
}

void BusListener::ListenerRegistered::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove a handler for ListenerRegistered to the interal ref class
    _listener->_eventsAndProperties->ListenerRegistered::remove(token);
}

void BusListener::ListenerRegistered::raise(BusAttachment ^ bus)
{
    // Invoke a handler for ListenerRegistered to the interal ref class
    return _listener->_eventsAndProperties->ListenerRegistered::raise(bus);
}

Windows::Foundation::EventRegistrationToken BusListener::ListenerUnregistered::add(BusListenerListenerUnregisteredHandler ^ handler)
{
    // Add a handler for ListenerUnregistered to the interal ref class
    return _listener->_eventsAndProperties->ListenerUnregistered::add(handler);
}

void BusListener::ListenerUnregistered::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove a handler for ListenerUnregistered to the interal ref class
    _listener->_eventsAndProperties->ListenerUnregistered::remove(token);
}

void BusListener::ListenerUnregistered::raise()
{
    // Invoke a handler for ListenerUnregistered to the interal ref class
    return _listener->_eventsAndProperties->ListenerUnregistered::raise();
}

Windows::Foundation::EventRegistrationToken BusListener::FoundAdvertisedName::add(BusListenerFoundAdvertisedNameHandler ^ handler)
{
    // Add a handler for FoundAdvertisedName to the interal ref class
    return _listener->_eventsAndProperties->FoundAdvertisedName::add(handler);
}

void BusListener::FoundAdvertisedName::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove a handler for FoundAdvertisedName to the interal ref class
    _listener->_eventsAndProperties->FoundAdvertisedName::remove(token);
}

void BusListener::FoundAdvertisedName::raise(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix)
{
    // Invoke a handler for FoundAdvertisedName to the interal ref class
    return _listener->_eventsAndProperties->FoundAdvertisedName::raise(name, transport, namePrefix);
}

Windows::Foundation::EventRegistrationToken BusListener::LostAdvertisedName::add(BusListenerLostAdvertisedNameHandler ^ handler)
{
    // Add a handler for LostAdvertisedName to the interal ref class
    return _listener->_eventsAndProperties->LostAdvertisedName::add(handler);
}

void BusListener::LostAdvertisedName::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove a handler for LostAdvertisedName to the interal ref class
    _listener->_eventsAndProperties->LostAdvertisedName::remove(token);
}

void BusListener::LostAdvertisedName::raise(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix)
{
    // Invoke a handler for LostAdvertisedName to the interal ref class
    return _listener->_eventsAndProperties->LostAdvertisedName::raise(name, transport, namePrefix);
}

Windows::Foundation::EventRegistrationToken BusListener::NameOwnerChanged::add(BusListenerNameOwnerChangedHandler ^ handler)
{
    // Add a handler for NameOwnerChanged to the interal ref class
    return _listener->_eventsAndProperties->NameOwnerChanged::add(handler);
}

void BusListener::NameOwnerChanged::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove a handler for NameOwnerChanged to the interal ref class
    _listener->_eventsAndProperties->NameOwnerChanged::remove(token);
}

void BusListener::NameOwnerChanged::raise(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner)
{
    // Invoke a handler for NameOwnerChanged to the interal ref class
    return _listener->_eventsAndProperties->NameOwnerChanged::raise(busName, previousOwner, newOwner);
}

Windows::Foundation::EventRegistrationToken BusListener::BusStopping::add(BusListenerBusStoppingHandler ^ handler)
{
    // Add a handler for BusStopping to the interal ref class
    return _listener->_eventsAndProperties->BusStopping::add(handler);
}

void BusListener::BusStopping::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove a handler for BusStopping to the interal ref class
    _listener->_eventsAndProperties->BusStopping::remove(token);
}

void BusListener::BusStopping::raise()
{
    // Invoke a handler for BusStopping to the interal ref class
    return _listener->_eventsAndProperties->BusStopping::raise();
}

Windows::Foundation::EventRegistrationToken BusListener::BusDisconnected::add(BusListenerBusDisconnectedHandler ^ handler)
{
    // Add a handler for BusDisconnected to the interal ref class
    return _listener->_eventsAndProperties->BusDisconnected::add(handler);
}

void BusListener::BusDisconnected::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove a handler for BusDisconnected to the interal ref class
    _listener->_eventsAndProperties->BusDisconnected::remove(token);
}

void BusListener::BusDisconnected::raise()
{
    // Invoke a handler for BusDisconnected to the interal ref class
    return _listener->_eventsAndProperties->BusDisconnected::raise();
}

BusAttachment ^ BusListener::Bus::get()
{
    // Get the Bus property from the internal ref class
    return _listener->_eventsAndProperties->Bus;
}

_BusListener::_BusListener(BusAttachment ^ bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create the internal ref class
        _eventsAndProperties = ref new __BusListener();
        // Check for allocation errors
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Set the default handler for ListenerRegistered
        _eventsAndProperties->ListenerRegistered += ref new BusListenerListenerRegisteredHandler([&] (BusAttachment ^ bus) {
                                                                                                     DefaultBusListenerListenerRegisteredHandler(bus);
                                                                                                 });
        // Set the default handler for ListenerUnregistered
        _eventsAndProperties->ListenerUnregistered += ref new BusListenerListenerUnregisteredHandler([&] () {
                                                                                                         DefaultBusListenerListenerUnregisteredHandler();
                                                                                                     });
        // Set the default handler for FoundAdvertisedName
        _eventsAndProperties->FoundAdvertisedName += ref new BusListenerFoundAdvertisedNameHandler([&] (Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix) {
                                                                                                       DefaultBusListenerFoundAdvertisedNameHandler(name, transport, namePrefix);
                                                                                                   });
        // Set the default handler for LostAdvertisedName
        _eventsAndProperties->LostAdvertisedName += ref new BusListenerLostAdvertisedNameHandler([&] (Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix) {
                                                                                                     DefaultBusListenerLostAdvertisedNameHandler(name, transport, namePrefix);
                                                                                                 });
        // Set the default handler for NameOwnerChanged
        _eventsAndProperties->NameOwnerChanged += ref new BusListenerNameOwnerChangedHandler([&] (Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner) {
                                                                                                 DefaultBusListenerNameOwnerChangedHandler(busName, previousOwner, newOwner);
                                                                                             });
        // Set the default handler for BusStopping
        _eventsAndProperties->BusStopping += ref new BusListenerBusStoppingHandler([&] () {
                                                                                       DefaultBusListenerBusStoppingHandler();
                                                                                   });
        // Set the default handler for BusDisconnected
        _eventsAndProperties->BusDisconnected += ref new BusListenerBusDisconnectedHandler([&] () {
                                                                                               DefaultBusListenerBusDisconnectedHandler();
                                                                                           });
        // Set the value of Bus
        _eventsAndProperties->Bus = bus;
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_BusListener::~_BusListener()
{
    _eventsAndProperties = nullptr;
}

void _BusListener::DefaultBusListenerListenerRegisteredHandler(BusAttachment ^ bus)
{
    // Get the unmanaged version
    ajn::BusAttachment* b = bus->_busAttachment;
    // Call the real API
    ajn::BusListener::ListenerRegistered(b);
}

void _BusListener::DefaultBusListenerListenerUnregisteredHandler()
{
    // Call the real API
    ajn::BusListener::ListenerUnregistered();
}

void _BusListener::DefaultBusListenerFoundAdvertisedNameHandler(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for failed conversion
        if (nullptr != name && strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert namePrefix to qcc::String
        qcc::String strNamePrefix = PlatformToMultibyteString(namePrefix);
        // Check for failed conversion
        if (nullptr != namePrefix && strNamePrefix.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        ajn::BusListener::FoundAdvertisedName(strName.c_str(), (ajn::TransportMask)(int)transport, strNamePrefix.c_str());
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusListener::DefaultBusListenerLostAdvertisedNameHandler(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for failed conversion
        if (nullptr != name && strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert namePrefix to qcc::String
        qcc::String strNamePrefix = PlatformToMultibyteString(namePrefix);
        // Check for failed conversion
        if (nullptr != namePrefix && strNamePrefix.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        ajn::BusListener::LostAdvertisedName(strName.c_str(), (ajn::TransportMask)(int)transport, strNamePrefix.c_str());
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusListener::DefaultBusListenerNameOwnerChangedHandler(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert busName to qcc::String
        qcc::String strBusName = PlatformToMultibyteString(busName);
        // Check for failed conversion
        if (nullptr != busName && strBusName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert previousOwner to qcc::String
        qcc::String strPreviousOwner = PlatformToMultibyteString(previousOwner);
        // Check for failed conversion
        if (nullptr != previousOwner && strPreviousOwner.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert newOwner to qcc::String
        qcc::String strNewOwner = PlatformToMultibyteString(newOwner);
        // Check for failed conversion
        if (nullptr != newOwner && strNewOwner.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        ajn::BusListener::NameOwnerChanged(strBusName.c_str(), strPreviousOwner.c_str(), strNewOwner.c_str());
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusListener::DefaultBusListenerBusStoppingHandler()
{
    // Call the real API
    ajn::BusListener::BusStopping();
}

void _BusListener::DefaultBusListenerBusDisconnectedHandler()
{
    // Call the real API
    ajn::BusListener::BusDisconnected();
}

void _BusListener::ListenerRegistered(ajn::BusAttachment* bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid vvalues in bus
        if (NULL == bus) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create wrapped BusAttachment
        AllJoyn::BusAttachment ^ ba = ref new AllJoyn::BusAttachment(bus);
        // Check for failed allocation
        if (nullptr == ba) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call ListenerRegistered through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->ListenerRegistered(ba);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusListener::ListenerUnregistered()
{
    // Call ListenerUnregistered through the dispatcher
    _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                 _eventsAndProperties->ListenerUnregistered();
                                                                                                             }));
}

void _BusListener::FoundAdvertisedName(const char* name, ajn::TransportMask transport, const char* namePrefix)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert name to Platform::String
        Platform::String ^ strName = MultibyteToPlatformString(name);
        // Check for failed conversion
        if (nullptr == strName && name != NULL && name[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert namePrefix to Platform::String
        Platform::String ^ strNamePrefix = MultibyteToPlatformString(namePrefix);
        // Check for failed conversion
        if (nullptr == strNamePrefix && namePrefix != NULL && namePrefix[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call FoundAdvertisedName through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->FoundAdvertisedName(strName, (TransportMaskType)(int)transport, strNamePrefix);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusListener::LostAdvertisedName(const char* name, ajn::TransportMask transport, const char* namePrefix)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert name to Platform::String
        Platform::String ^ strName = MultibyteToPlatformString(name);
        // Check for failed conversion
        if (nullptr == strName && name != NULL && name[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert namePrefix to Platform::String
        Platform::String ^ strNamePrefix = MultibyteToPlatformString(namePrefix);
        // Check for failed conversion
        if (nullptr == strNamePrefix && namePrefix != NULL && namePrefix[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call LostAdvertisedName through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->LostAdvertisedName(strName, (TransportMaskType)(int)transport, strNamePrefix);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusListener::NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert busName to Platform::String
        Platform::String ^ strBusName = MultibyteToPlatformString(busName);
        // Check for failed conversion
        if (nullptr == strBusName && busName != NULL && busName[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert previousOwner to Platform::String
        Platform::String ^ strPreviousOwner = MultibyteToPlatformString(previousOwner);
        // Check for failed conversion
        if (nullptr == strPreviousOwner && previousOwner != NULL && previousOwner[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert newOwner to Platform::String
        Platform::String ^ strNewOwner = MultibyteToPlatformString(newOwner);
        // Check for failed conversion
        if (nullptr == strNewOwner && newOwner != NULL && newOwner[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call NameOwnerChanged through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->NameOwnerChanged(strBusName, strPreviousOwner, strNewOwner);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusListener::BusStopping()
{
    // Call BusStopping through the dispatcher
    _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                 _eventsAndProperties->BusStopping();
                                                                                                             }));
}

void _BusListener::BusDisconnected()
{
    // Call BusDisconnected through the dispatcher
    _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                 _eventsAndProperties->BusDisconnected();
                                                                                                             }));
}

__BusListener::__BusListener()
{
    Bus = nullptr;
}

__BusListener::~__BusListener()
{
    Bus = nullptr;
}

}
