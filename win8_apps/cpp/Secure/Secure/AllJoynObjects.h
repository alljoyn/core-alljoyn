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

using namespace Secure;
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

//	Bus Object which handles calls to the 'Ping' method and verifying credentials
public ref class SecureServiceObject sealed {
  public:
    SecureServiceObject(BusAttachment ^ busAtt, Platform::String ^ path);

    // Handles calls to the service's 'Ping' method by printing out provided message to
    // user and return same message in reply
    void Ping(InterfaceMember ^ member, Message ^ message);

    property BusObject ^ GetBusObject { BusObject ^ get(); }
  private:
    BusObject ^ busObject;                      //Primary bus object implementing interface over the bus
};

// Bus Listener which handles all bus events of interest
public ref class ServiceBusListener sealed {
  public:
    ServiceBusListener(BusAttachment ^ busAtt);

    // Called by the bus when an external bus is discovered that is advertising a well-known
    // name that this attachment has registered interest in via a DBus call to
    // org.alljoyn.Bus.FindAdvertisedName
    void FoundAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix);
    // Called by the bus when an advertisement previously reported through FoundName has become
    // unavailable.
    void LostAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix);
    // Called when the owner of a well-known name changes
    void NameOwnerChanged(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner);

    // Called when there's been a join session request from the client
    bool AcceptSessionJoiner(unsigned short sessionPort, Platform::String ^ joiner, SessionOpts ^ sessionOpts);
    //	Called when a session has been joined by client
    void SessionJoined(unsigned short sessionPort, unsigned int sessId, Platform::String ^ joiner);
    //	Called when bus attachment has been disconnected from the D-Bus
    void BusDisconnected();
    //	Called when bus attachment is stopping
    void BusStopping();
    //	Called by the bus when an existing session becomes disconnected.
    void SessionLost(unsigned int sessId);
    //	Called by the bus when a member of a multipoint session is added.
    void SessionMemberAdded(unsigned int sessionId, Platform::String ^ uniqueName);
    //	Called by the bus when a member of a multipoint session is removed.
    void SessionMemberRemoved(unsigned int sessionId, Platform::String ^ uniqueName);
    //	Called by bus attachment when the bus listener is registered
    void ListenerRegistered(BusAttachment ^ busAtt);
    //	Called by bus attachment when the bus listener is unregistered
    void ListenerUnregistered();

    //	Called when user credentials are requested.
    //	Authentication mechanism requests user credentials. If the user name is not an empty string
    //	the request is for credentials for that specific user. A count allows the listener to decide
    //	whether to allow or reject multiple authentication attempts to the same peer.
    QStatus RequestCredentials(Platform::String ^ authMechanism, Platform::String ^ peerName, unsigned short authCount,
                               Platform::String ^ userName, unsigned short credMask, AuthContext ^ authContext);
    //	Reports successful or unsuccessful completion of authentication.
    void AuthenticationComplete(Platform::String ^ authMechanism, Platform::String ^ peerName, bool success);

    property BusListener ^ GetBusListener { BusListener ^ get(); }
    property SessionListener ^ GetSessionListener { SessionListener ^ get(); }
    property SessionPortListener ^ GetSessionPortListener { SessionPortListener ^ get(); }
    property AuthListener ^ GetAuthListener { AuthListener ^ get(); }
  private:
    BusListener ^ busListener;                                          //primary listener which handles events occurring over the bus
    SessionListener ^ sessionListener;                          //handles session events
    SessionPortListener ^ sessionPortListener;          //handles session port events
    AuthListener ^ authListener;                                        //handles authorization process
};


// Bus Listener which handles all bus events of interest
public ref class ClientBusListener sealed {
  public:
    ClientBusListener(BusAttachment ^ busAtt);

    // Called by the bus when an external bus is discovered that is advertising a well-known
    // name that this attachment has registered interest in via a DBus call to
    // org.alljoyn.Bus.FindAdvertisedName
    void FoundAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix);
    // Called by the bus when an advertisement previously reported through FoundName has become
    // unavailable.
    void LostAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix);
    // Called when the owner of a well-known name changes
    void NameOwnerChanged(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner);
    //	Called when bus attachment has been disconnected from the D-Bus
    void BusDisconnected();
    //	Called when bus attachment is stopping
    void BusStopping();
    //	Called by the bus when an existing session becomes disconnected.
    void SessionLost(unsigned int sessId);
    //	Called by the bus when a member of a multipoint session is added.
    void SessionMemberAdded(unsigned int sessionId, Platform::String ^ uniqueName);
    //	Called by the bus when a member of a multipoint session is removed.
    void SessionMemberRemoved(unsigned int sessionId, Platform::String ^ uniqueName);
    //	Called by bus attachment when the bus listener is registered
    void ListenerRegistered(BusAttachment ^ busAtt);
    //	Called by bus attachment when the bus listener is unregistered
    void ListenerUnregistered();

    //	Called when user credentials are requested.
    //	Authentication mechanism requests user credentials. If the user name is not an empty string
    //	the request is for credentials for that specific user. A count allows the listener to decide
    //	whether to allow or reject multiple authentication attempts to the same peer.
    QStatus RequestCredentials(Platform::String ^ authMechanism, Platform::String ^ peerName, unsigned short authCount,
                               Platform::String ^ userName, unsigned short credMask, AuthContext ^ authContext);
    //	Reports successful or unsuccessful completion of authentication.
    void AuthenticationComplete(Platform::String ^ authMechanism, Platform::String ^ peerName, bool success);

    property BusListener ^ GetBusListener { BusListener ^ get(); }
    property SessionListener ^ GetSessionListener { SessionListener ^ get(); }
    property AuthListener ^ GetAuthListener { AuthListener ^ get(); }
  private:
    BusListener ^ busListener;                  //primary listener which handles events occurring over the bus
    SessionListener ^ sessionListener;  //handles session events
    AuthListener ^ authListener;                //handles authorization process
};

}