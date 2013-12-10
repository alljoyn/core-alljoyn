//-----------------------------------------------------------------------
// <copyright file="AllJoynObjects.cpp" company="AllSeen Alliance.">
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

#include "pch.h"
#include "MainPage.xaml.h"
#include "AllJoynObjects.h"
using namespace BasicService;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml;
using namespace AllJoyn;
using namespace AllJoynObjects;
using namespace Windows::Foundation;

/////////////////////////////////////Argument Object//////////////////////////////////////////////////

ArgumentObject::ArgumentObject(Platform::String ^ msg, TextBox ^ tb) : text(nullptr), textBox(nullptr)
{
    text = msg;
    textBox = tb;
}
void ArgumentObject::OnDispactched() {
    textBox->Text += text;
}


/////////////////////////////////////Basic Sample Bus Object//////////////////////////////////////////////////


BasicSampleObject::BasicSampleObject(BusAttachment ^ busAtt, Platform::String ^ path) : busObject(nullptr)
{
    busObject = ref new BusObject(busAtt, path, false);

    // Add the 'cat' interface to bus object
    InterfaceDescription ^ serviveInterface = busAtt->GetInterface(INTERFACE_NAME);
    busObject->AddInterface(serviveInterface);

    // Register the 'cat' method handlers with the object
    InterfaceMember ^ member = serviveInterface->GetMember("cat");
    MessageReceiver ^ receiver = ref new MessageReceiver(busAtt);
    receiver->MethodHandler += ref new MessageReceiverMethodHandler([this]
                                                                        (InterfaceMember ^ member, Message ^ message)
                                                                    {
                                                                        Cat(member, message);
                                                                    }
                                                                    );
    busObject->AddMethodHandler(member, receiver);
}

// Concatenate the two input strings and reply to caller with the result.
void BasicSampleObject::Cat(InterfaceMember ^ member, Message ^ msg)
{
    Platform::String ^ arg1 = msg->GetArg(0)->Value->ToString();
    Platform::String ^ arg2 = msg->GetArg(1)->Value->ToString();
    Platform::String ^ sender = msg->Sender;
    Platform::String ^ result = Platform::String::Concat(arg1, arg2);

    App ^ app = (App ^) Application::Current;
    app->OutputLine("'cat' method was called by '" + sender + "' with the arguments '" + arg1 + "' and '" + arg2 + "'.");

    Platform::Array<Object ^>^ arr = { (Object ^)result };
    MsgArg ^ outArg = ref new MsgArg("s", arr);
    Platform::Array<MsgArg ^>^ returnArray = { outArg };

    //	Reply to the sender with the concatenation of the two arguments
    app->OutputLine("Replying to '" + sender + "' with a return value of '" + result + "'.\n");
    try {
        busObject->MethodReply(msg, returnArray);
    } catch (Platform::Exception ^ ex) {
        app->OutputLine("Method Reply was unsuccessful.");
        app->OutputLine("Error: " + ex->Message->ToString());
    }
}

// return a reference to the bus object
BusObject ^ BasicSampleObject::GetBusObject::get() {
    return busObject;
}



/////////////////////////////////////My Bus Listener//////////////////////////////////////////////////

MyBusListener::MyBusListener(BusAttachment ^ busAtt) :
    busListener(nullptr), sessionPortListener(nullptr), sessionListener(nullptr)
{
    /* Create Bus Listener and register all event handlers  */
    busListener = ref new BusListener(busAtt);
    busListener->NameOwnerChanged +=
        ref new BusListenerNameOwnerChangedHandler([this]
                                                       (Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner)
                                                   {
                                                       NameOwnerChanged(busName, previousOwner, newOwner);
                                                   });
    busListener->BusDisconnected +=
        ref new BusListenerBusDisconnectedHandler([this] ()
                                                  {
                                                      BusDisconnected();
                                                  });
    busListener->BusStopping +=
        ref new BusListenerBusStoppingHandler([this] ()
                                              {
                                                  BusStopping();
                                              });
    busListener->ListenerRegistered +=
        ref new BusListenerListenerRegisteredHandler([this]
                                                         (BusAttachment ^ busAtt)
                                                     {
                                                         ListenerRegistered(busAtt);
                                                     });
    busListener->ListenerUnregistered +=
        ref new BusListenerListenerUnregisteredHandler([this] ()
                                                       {
                                                           ListenerUnregistered();
                                                       });
    busListener->FoundAdvertisedName +=
        ref new BusListenerFoundAdvertisedNameHandler([this]
                                                          (Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
                                                      {
                                                          FoundAdvertisedName(wellKnownName, transport, namePrefix);
                                                      });
    busListener->LostAdvertisedName +=
        ref new BusListenerLostAdvertisedNameHandler([this]
                                                         (Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
                                                     {
                                                         LostAdvertisedName(wellKnownName, transport, namePrefix);
                                                     });

    /* Create Session Listener and register all event handlers  */
    sessionListener = ref new SessionListener(busAtt);
    sessionListener->SessionLost +=
        ref new SessionListenerSessionLostHandler([this]
                                                      (unsigned int sessionId)
                                                  {
                                                      SessionLost(sessionId);
                                                  });
    sessionListener->SessionMemberAdded +=
        ref new SessionListenerSessionMemberAddedHandler([this]
                                                             (unsigned int sessionId, Platform::String ^ uniqueName)
                                                         {
                                                             SessionMemberAdded(sessionId, uniqueName);
                                                         });
    sessionListener->SessionMemberRemoved +=
        ref new SessionListenerSessionMemberRemovedHandler([this]
                                                               (unsigned int sessionId, Platform::String ^ uniqueName)
                                                           {
                                                               SessionMemberRemoved(sessionId, uniqueName);
                                                           });

    /* Create Session Port Listener and register all event handlers  */
    sessionPortListener = ref new SessionPortListener(busAtt);
    sessionPortListener->AcceptSessionJoiner +=
        ref new SessionPortListenerAcceptSessionJoinerHandler([this]
                                                                  (unsigned short sessionPort, Platform::String ^ joiner, SessionOpts ^ sessionOpts)->bool
                                                              {
                                                                  return AcceptSessionJoiner(sessionPort, joiner, sessionOpts);
                                                              });
    sessionPortListener->SessionJoined +=
        ref new SessionPortListenerSessionJoinedHandler([this]
                                                            (unsigned short sessionPort, unsigned int sessionId, Platform::String ^ joiner)
                                                        {
                                                            SessionJoined(sessionPort, sessionId, joiner);
                                                        });
}

// Called by the bus when an external bus is discovered that is advertising a well-known
// name that this attachment has registered interest in via a DBus call to
// org.alljoyn.Bus.FindAdvertisedName
void MyBusListener::FoundAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
{
}
// Called by the bus when an advertisement previously reported through FoundName has become
// unavailable.
void MyBusListener::LostAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
{
}
// Called when the owner of a well-known name changes
void MyBusListener::NameOwnerChanged(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Name Owner Changed (wkn=" + busName + " prevOwner=" + previousOwner + " newOwner=" + newOwner + ")");
}

// Called when there's been a join session request from the client
bool MyBusListener::AcceptSessionJoiner(unsigned short sessionPort, Platform::String ^ joiner, SessionOpts ^ sessionOpts)
{
    if (SERVICE_PORT == sessionPort) {
        App ^ app = (App ^) Application::Current;
        app->OutputLine("Accepting Join Session Request from joiner '" + joiner + "'.");
        return true;
    } else {
        return false;
    }
}
//	Called when a session has been joined by client
void MyBusListener::SessionJoined(unsigned short sessionPort, unsigned int sessId, Platform::String ^ joiner)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Join Session Success (sessionId=" + sessId + ")");
}
//	Called when a BusAttachment this listener is registered with is has become disconnected from the bus.
void MyBusListener::BusDisconnected()
{
}
//	Called when a BusAttachment this listener is registered with is stopping.
void MyBusListener::BusStopping()
{
}
//	Called by the bus when the listener is registered.
void MyBusListener::ListenerRegistered(BusAttachment ^ busAtt)
{
}
//	Called by the bus when the listener is unregistered.
void MyBusListener::ListenerUnregistered()
{
}
//	Called by the bus when an existing session becomes disconnected.
void MyBusListener::SessionLost(unsigned int sessId)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Session Lost (sessionId=" + sessId + ")");
}
//	Called by the bus when a member of a multipoint session is added.
void MyBusListener::SessionMemberAdded(unsigned int sessionId, Platform::String ^ uniqueName)
{
}
//	Called by the bus when a member of a multipoint session is removed.
void MyBusListener::SessionMemberRemoved(unsigned int sessionId, Platform::String ^ uniqueName)
{
}

// return a reference to the bus listener
BusListener ^ MyBusListener::GetBusListener::get() {
    return busListener;
}
// return a reference to the session listener
SessionListener ^ MyBusListener::GetSessionListener::get() {
    return sessionListener;
}
// return a reference to the session port listener
SessionPortListener ^ MyBusListener::GetSessionPortListener::get() {
    return sessionPortListener;
}
