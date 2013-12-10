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
using namespace SignalConsumerClient;
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


/////////////////////////////////////My Bus Listener//////////////////////////////////////////////////


MyBusListener::MyBusListener(BusAttachment ^ busAtt) : busListener(nullptr), sessionListener(nullptr)
{
    //	Create and register the signal handler which will handle the 'nameChanged' signal
    MessageReceiver ^ signalReceiver = ref new MessageReceiver(busAtt);
    signalReceiver->SignalHandler += ref new MessageReceiverSignalHandler([this]
                                                                              (InterfaceMember ^ member, Platform::String ^ sourcePath, Message ^ message)
                                                                          {
                                                                              NameChangedSignalHandler(member, sourcePath, message);
                                                                          }
                                                                          );

    Platform::String ^ signalName = "nameChanged";
    InterfaceDescription ^ interfaceDescription = busAtt->GetInterface(INTERFACE_NAME);
    InterfaceMember ^ signal = interfaceDescription->GetSignal(signalName);
    Platform::String ^ srcPath = "";
    busAtt->RegisterSignalHandler(signalReceiver, signal, srcPath);

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
}
//	Called when the 'name' property of the service has changed.
void MyBusListener::NameChangedSignalHandler(InterfaceMember ^ member, Platform::String ^ sourcePath, Message ^ message)
{
    Platform::String ^ newName = message->GetArg(0)->Value->ToString();

    App ^ app = (App ^) Application::Current;
    app->OutputLine("-----=============##############NameChanged Signal from '" + sourcePath + "'#############=============-----");
    app->OutputLine("New Name: '" + newName + "'\n");
}
// Called by the bus when an external bus is discovered that is advertising a well-known
// name that this attachment has registered interest in via a DBus call to
// org.alljoyn.Bus.FindAdvertisedName
void MyBusListener::FoundAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Found Advertised Name '" + wellKnownName + "'.");
}
// Called by the bus when an advertisement previously reported through FoundName has become
// unavailable.
void MyBusListener::LostAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Lost Advertised Name '" + wellKnownName + "'.");
}
// Called when the owner of a well-known name changes
void MyBusListener::NameOwnerChanged(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Name Owner Changed (wkn=" + busName + " prevOwner=" + previousOwner + " newOwner=" + newOwner + ")");
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
