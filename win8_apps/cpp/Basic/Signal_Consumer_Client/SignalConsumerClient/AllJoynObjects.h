//-----------------------------------------------------------------------
// <copyright file="AllJoynObjects.h" company="AllSeen Alliance.">
//     Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//        Permission to use, copy, modify, and/or distribute this software for any
//        purpose with or without fee is hereby granted, provided that the above
//        copyright notice and this permission notice appear in all copies.
//
//        THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//        WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//        MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//        ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//        WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//        ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//        OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// </copyright>
//-----------------------------------------------------------------------

#pragma once

#include "MainPage.xaml.h"
using namespace SignalConsumerClient;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Interop;
using namespace AllJoyn;

namespace AllJoynObjects {

//	Encapsulation object for dispatcher to use when printing message to UI
[Windows::Foundation::Metadata::WebHostHiddenAttribute]
public ref class ArgumentObject sealed {
  public:
    ArgumentObject(Platform::String ^ msg, TextBox ^ tb);
    void OnDispactched();
  private:
    Platform::String ^ text;
    TextBox ^ textBox;
};

// Bus Listener which handles all bus events of interest
public ref class MyBusListener sealed {
  public:

    MyBusListener(BusAttachment ^ busAtt);
    //	Called when the 'name' property of the service has changed.
    void NameChangedSignalHandler(InterfaceMember ^ member, Platform::String ^ sourcePath, Message ^ message);
    //	Called when the service advertising the well-known name of interest have been discovered
    void FoundAdvertisedName(Platform::String ^ name, TransportMaskType transport, Platform::String ^ namePrefix);
    // Called by the bus when an advertisement previously reported through FoundName has become
    // unavailable.
    void LostAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix);
    // Called when the owner of a well-known name changes
    void NameOwnerChanged(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner);
    //	Called when bus attachment has been disconnected from the D-Bus
    void BusDisconnected();
    //	Called when bus attachment is stopping
    void BusStopping();
    //	Called by bus attachment when the bus listener is registered
    void ListenerRegistered(BusAttachment ^ busAtt);
    //	Called by bus attachment when the bus listener is unregistered
    void ListenerUnregistered();
    //	Called by the bus when an existing session becomes disconnected.
    void SessionLost(unsigned int sessionId);
    //	Called by the bus when a member of a multipoint session is added.
    void SessionMemberAdded(unsigned int sessionId, Platform::String ^ uniqueName);
    //	Called by the bus when a member of a multipoint session is removed.
    void SessionMemberRemoved(unsigned int sessionId, Platform::String ^ uniqueName);

    property BusListener ^ GetBusListener { BusListener ^ get(); }
    property SessionListener ^ GetSessionListener { SessionListener ^ get(); }
  private:
    BusListener ^ busListener;                                      //primary listener which handles events occurring over the bus
    SessionListener ^ sessionListener;                      //primary listener which handles events occurring in the session
};

}
