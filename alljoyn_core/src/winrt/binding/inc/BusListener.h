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

#include <alljoyn/BusListener.h>
#include <TransportMaskType.h>
#include <qcc/ManagedObj.h>

namespace AllJoyn {

ref class BusListener;
ref class BusAttachment;

/// <summary>
///Called by the bus when the listener is registered. This give the listener implementation the
///opportunity to save a reference to the bus.
/// </summary>
/// <param name="bus">The bus the listener is registered with.</param>
public delegate void BusListenerListenerRegisteredHandler(BusAttachment ^ bus);

/// <summary>
///Called by the bus when the listener is unregistered.
/// </summary>
public delegate void BusListenerListenerUnregisteredHandler();

/// <summary>
///Called by the bus when an external bus is discovered that is advertising a well-known name
///that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
/// </summary>
/// <param name="name">A well known name that the remote bus is advertising.</param>
/// <param name="transport">Transport that received the advertisement.</param>
/// <param name="namePrefix">The well-known name prefix used in call to FindAdvertisedName that triggered this callback.</param>
public delegate void BusListenerFoundAdvertisedNameHandler(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix);

/// <summary>
///Called by the bus when an advertisement previously reported through FoundName has become unavailable.
/// </summary>
/// <param name="name">A well known name that the remote bus is advertising that is of interest to this attachment.</param>
/// <param name="transport">Transport that stopped receiving the given advertised name.</param>
/// <param name="namePrefix">The well-known name prefix that was used in a call to FindAdvertisedName that triggered this callback.</param>
public delegate void BusListenerLostAdvertisedNameHandler(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix);

/// <summary>
///Called by the bus when the ownership of any well-known name changes.
/// </summary>
/// <param name="busName">The well-known name that has changed.</param>
/// <param name="previousOwner">The unique name that previously owned the name or NULL if there was no previous owner.</param>
/// <param name="newOwner">The unique name that now owns the name or NULL if the there is no new owner.</param>
public delegate void BusListenerNameOwnerChangedHandler(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner);

/// <summary>
///Called when a BusAttachment this listener is registered with is stopping.
/// </summary>
public delegate void BusListenerBusStoppingHandler();

/// <summary>
///Called when a BusAttachment this listener is registered with is has become disconnected from the bus.
/// </summary>
public delegate void BusListenerBusDisconnectedHandler();

ref class __BusListener {
  private:
    friend ref class BusListener;
    friend class _BusListener;
    __BusListener();
    ~__BusListener();

    event BusListenerListenerRegisteredHandler ^ ListenerRegistered;
    event BusListenerListenerUnregisteredHandler ^ ListenerUnregistered;
    event BusListenerFoundAdvertisedNameHandler ^ FoundAdvertisedName;
    event BusListenerLostAdvertisedNameHandler ^ LostAdvertisedName;
    event BusListenerNameOwnerChangedHandler ^ NameOwnerChanged;
    event BusListenerBusStoppingHandler ^ BusStopping;
    event BusListenerBusDisconnectedHandler ^ BusDisconnected;
    property BusAttachment ^ Bus;
};

class _BusListener : protected ajn::BusListener {
  protected:
    friend class qcc::ManagedObj<_BusListener>;
    friend ref class BusListener;
    friend ref class BusAttachment;
    _BusListener(BusAttachment ^ bus);
    ~_BusListener();

    void DefaultBusListenerListenerRegisteredHandler(BusAttachment ^ bus);
    void DefaultBusListenerListenerUnregisteredHandler();
    void DefaultBusListenerFoundAdvertisedNameHandler(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix);
    void DefaultBusListenerLostAdvertisedNameHandler(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix);
    void DefaultBusListenerNameOwnerChangedHandler(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner);
    void DefaultBusListenerBusStoppingHandler();
    void DefaultBusListenerBusDisconnectedHandler();
    void ListenerRegistered(ajn::BusAttachment* bus);
    void ListenerUnregistered();
    void FoundAdvertisedName(const char* name, ajn::TransportMask transport, const char* namePrefix);
    void LostAdvertisedName(const char* name, ajn::TransportMask transport, const char* namePrefix);
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner);
    void BusStopping();
    void BusDisconnected();

    __BusListener ^ _eventsAndProperties;
};

public ref class BusListener sealed {
  public:
    BusListener(BusAttachment ^ bus);

    /// <summary>
    /// Raised by the bus when the listener is registered.
    /// </summary>
    event BusListenerListenerRegisteredHandler ^ ListenerRegistered
    {
        Windows::Foundation::EventRegistrationToken add(BusListenerListenerRegisteredHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(BusAttachment ^ bus);
    }

    /// <summary>
    /// Raised by the bus when the listener is unregistered.
    /// </summary>
    event BusListenerListenerUnregisteredHandler ^ ListenerUnregistered
    {
        Windows::Foundation::EventRegistrationToken add(BusListenerListenerUnregisteredHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise();
    }

    /// <summary> Raised by the bus when an external bus is discovered that is advertising a well-known name
    ///that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
    /// </summary>
    event BusListenerFoundAdvertisedNameHandler ^ FoundAdvertisedName
    {
        Windows::Foundation::EventRegistrationToken add(BusListenerFoundAdvertisedNameHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix);
    }

    /// <summary>
    /// Raised by the bus when an advertisement previously reported through FoundName has become unavailable.
    /// </summary>
    event BusListenerLostAdvertisedNameHandler ^ LostAdvertisedName
    {
        Windows::Foundation::EventRegistrationToken add(BusListenerLostAdvertisedNameHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix);
    }

    /// <summary>
    /// Raised by the bus when the ownership of any well-known name changes.
    /// </summary>
    event BusListenerNameOwnerChangedHandler ^ NameOwnerChanged
    {
        Windows::Foundation::EventRegistrationToken add(BusListenerNameOwnerChangedHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner);
    }

    /// <summary>
    /// Raised when a BusAttachment this listener is registered with is stopping.
    /// </summary>
    event BusListenerBusStoppingHandler ^ BusStopping
    {
        Windows::Foundation::EventRegistrationToken add(BusListenerBusStoppingHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise();
    }

    /// <summary>
    /// Raised when a BusAttachment this listener is registered with is has become disconnected from the bus.
    /// </summary>
    event BusListenerBusDisconnectedHandler ^ BusDisconnected
    {
        Windows::Foundation::EventRegistrationToken add(BusListenerBusDisconnectedHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise();
    }

    property BusAttachment ^ Bus
    {
        BusAttachment ^ get();
    }

  private:
    friend ref class BusAttachment;
    BusListener(const qcc::ManagedObj<_BusListener>* listener);
    ~BusListener();

    qcc::ManagedObj<_BusListener>* _mListener;
    _BusListener* _listener;
};

}
// BusListener.h
