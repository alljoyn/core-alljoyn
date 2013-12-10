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
#include "AllJoynObjects.h"
#include <algorithm>
#include <string.h>
#include <ppltasks.h>

using namespace Chat;
using namespace Windows::UI::Xaml::Controls;
using namespace AllJoyn;
using namespace Windows::UI::Xaml;
using namespace concurrency;
using namespace Windows::Foundation;
using namespace AllJoynObjects;

//////////////////////////////ArgumentObject////////////////////////////////

//	Encapsulation object for dispatcher to use when printing message to UI
ArgumentObject::ArgumentObject(Platform::String ^ msg, TextBlock ^ tb) : text(nullptr), textBlock(nullptr)
{
    text = msg;
    textBlock = tb;
}
void ArgumentObject::OnDispactched() {
    textBlock->Text = text;
}

//////////////////////////////////ChatArg////////////////////////////////////

//	Encapsulation object for dispatcher to use when printing message to chat view of the UI
ChatArg::ChatArg(Platform::String ^ msg, TextBox ^ tb) : text(nullptr), textBox(nullptr)
{
    text = msg;
    textBox = tb;
}
void ChatArg::OnDispactched()
{
    Platform::String ^ oldText = textBox->Text;
    textBox->Text = text + oldText;
}

//////////////////////////////////ChatObject////////////////////////////////////

//	Bus Object which handles all chat signals received and sent to other chat apps
ChatObject::ChatObject(BusAttachment ^ busAtt, Platform::String ^ path) : busObject(nullptr), signalMember(nullptr)
{
    busObject = ref new BusObject(busAtt, path, false);

    App ^ app = (App ^) Application::Current;
    app->UpdateStatus("Creating the chat object and registering the chat signal handlers...");

    // Add the chat interface to the bus object
    InterfaceDescription ^ chatInterface = nullptr;
    Platform::Array<InterfaceDescription ^, 1>^ intfArray = { chatInterface };
    busAtt->CreateInterface(INTERFACE_NAME, intfArray, false);
    intfArray[0]->AddSignal("Chat", "s", "str", (unsigned char)0, "");
    intfArray[0]->Activate();
    busObject->AddInterface(intfArray[0]);

    // Add and register the signal handlers
    chatInterface = busAtt->GetInterface(INTERFACE_NAME);
    signalMember = chatInterface->GetSignal("Chat");
    MessageReceiver ^ chatReceiver = ref new MessageReceiver(busAtt);
    chatReceiver->SignalHandler += ref new MessageReceiverSignalHandler([this]
                                                                            (InterfaceMember ^ member, Platform::String ^ srcPath, Message ^ message)
                                                                        {
                                                                            ChatSignalHandler(member, srcPath, message);
                                                                        }
                                                                        );
    busAtt->RegisterSignalHandler(chatReceiver, signalMember, CHAT_SERVICE_OBJECT_PATH);
}
//	Called when a 'chat' signal is recieved, then outputs message to user
void ChatObject::ChatSignalHandler(InterfaceMember ^ member, Platform::String ^ srcPath, Message ^ message)
{
    Platform::String ^ time = getTime();
    Platform::String ^ sender = message->Sender;
    Platform::String ^ messageArg = message->GetArg(0)->Value->ToString();
    unsigned int sessionId = message->SessionId;

    App ^ app = (App ^) Application::Current;
    app->UpdateChat(time, sender, messageArg, sessionId);
}
//	Called when the user requests to send a message over a channel
void ChatObject::SendChatMessage(Platform::String ^ msg)
{
    chatSignal(sessionId, msg);
    chatSignal(hostedSessionId, msg);

    Platform::String ^ time = getTime();

    App ^ app = (App ^) Application::Current;
    app->UpdateChat(time, "Me", msg, sessionId);
}
//	Send a signal with given message to the specified sessionId
void ChatObject::chatSignal(unsigned int sessId, Platform::String ^ msg)
{
    if (sessId != 0) {
        Platform::Array<Object ^, 1>^ objs = { msg };
        MsgArg ^ msgArg = ref new MsgArg("s", objs);
        Platform::Array<MsgArg ^, 1>^ msgArgs = { msgArg };

        App ^ app = (App ^) Application::Current;
        try {
            busObject->Signal("", sessId, signalMember, msgArgs, 100,
                              (unsigned char)AllJoynFlagType::ALLJOYN_FLAG_GLOBAL_BROADCAST);
        } catch (std::exception ex) {
            int i = 0;
        } catch (Platform::Exception ^ ex) {
            app->UpdateStatus(ex->Message->ToString());
        } catch (...) {
            App ^ app = (App ^) Application::Current;
            app->UpdateStatus("A problem occurred when trying to send the message (sessionId=" + sessId + ")");
        }
    }
}
//	Get the current system time in string format
Platform::String ^ ChatObject::getTime()
{
    auto cal = ref new Windows::Globalization::Calendar();
    cal->SetToNow();
    Platform::String ^ hour = cal->HourAsString();
    if (hour->Length() == 1) {
        hour = "0" + hour;
    }
    Platform::String ^ mins = cal->MinuteAsString();
    if (mins->Length() == 1) {
        mins = "0" + mins;
    }
    Platform::String ^ secs = cal->SecondAsString();
    if (secs->Length() == 1) {
        secs = "0" + secs;
    }
    Platform::String ^ timeStamp = hour + ":" + mins + ":" + secs;
    return timeStamp;
}
// return a reference to the bus object
BusObject ^ ChatObject::GetBusObject::get() {
    return busObject;
}

//////////////////////////////////MyBusListener////////////////////////////////////
MyBusListener::MyBusListener(BusAttachment ^ busAtt) :
    busListener(nullptr), sessionPortListener(nullptr), sessionListener(nullptr)
{
    /* Create Bus Listener and register all event handlers  */
    busListener = ref new BusListener(busAtt);
    busListener->NameOwnerChanged += ref new BusListenerNameOwnerChangedHandler([this]
                                                                                    (Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner)
                                                                                {
                                                                                    NameOwnerChanged(busName, previousOwner, newOwner);
                                                                                });
    busListener->BusDisconnected += ref new BusListenerBusDisconnectedHandler([this] ()
                                                                              {
                                                                                  BusDisconnected();
                                                                              });
    busListener->BusStopping += ref new BusListenerBusStoppingHandler([this] ()
                                                                      {
                                                                          BusStopping();
                                                                      });
    busListener->ListenerRegistered += ref new BusListenerListenerRegisteredHandler([this]
                                                                                        (BusAttachment ^ busAtt)
                                                                                    {
                                                                                        ListenerRegistered(busAtt);
                                                                                    });
    busListener->ListenerUnregistered += ref new BusListenerListenerUnregisteredHandler([this] ()
                                                                                        {
                                                                                            ListenerUnregistered();
                                                                                        });
    busListener->FoundAdvertisedName += ref new BusListenerFoundAdvertisedNameHandler([this]
                                                                                          (Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
                                                                                      {
                                                                                          FoundAdvertisedName(wellKnownName, transport, namePrefix);
                                                                                      });
    busListener->LostAdvertisedName += ref new BusListenerLostAdvertisedNameHandler([this]
                                                                                        (Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
                                                                                    {
                                                                                        LostAdvertisedName(wellKnownName, transport, namePrefix);
                                                                                    });

    /* Create Session Listener and register all event handlers  */
    sessionListener = ref new SessionListener(busAtt);
    sessionListener->SessionLost += ref new SessionListenerSessionLostHandler([this]
                                                                                  (unsigned int sessionId)
                                                                              {
                                                                                  SessionLost(sessionId);
                                                                              });
    sessionListener->SessionMemberAdded += ref new SessionListenerSessionMemberAddedHandler([this]
                                                                                                (unsigned int sessionId, Platform::String ^ uniqueName)
                                                                                            {
                                                                                                SessionMemberAdded(sessionId, uniqueName);
                                                                                            });
    sessionListener->SessionMemberRemoved += ref new SessionListenerSessionMemberRemovedHandler([this]
                                                                                                    (unsigned int sessionId, Platform::String ^ uniqueName)
                                                                                                {
                                                                                                    SessionMemberRemoved(sessionId, uniqueName);
                                                                                                });

    /* Create Session Port Listener and register all event handlers  */
    sessionPortListener = ref new SessionPortListener(busAtt);
    sessionPortListener->AcceptSessionJoiner += ref new SessionPortListenerAcceptSessionJoinerHandler([this]
                                                                                                          (unsigned short sessionPort, Platform::String ^ joiner, SessionOpts ^ sessionOpts)->bool
                                                                                                      {
                                                                                                          return AcceptSessionJoiner(sessionPort, joiner, sessionOpts);
                                                                                                      });
    sessionPortListener->SessionJoined += ref new SessionPortListenerSessionJoinedHandler([this]
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
    Platform::String ^ subString = GetName(wellKnownName);

    if (subString != channelHosted) {
        App ^ app = (App ^) Application::Current;
        app->UpdateChannels(subString, false);

        app->UpdateStatus("Found Advertised Name '" + subString + "'");
    }
}
// Called by the bus when an advertisement previously reported through FoundName has become
// unavailable.
void MyBusListener::LostAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
{
    Platform::String ^ subString = GetName(wellKnownName);
    App ^ app = (App ^) Application::Current;
    app->UpdateChannels(subString, true);

    app->UpdateStatus("Lost Advertised Name '" + subString + "'");
}
// Called when the owner of a well-known name changes
void MyBusListener::NameOwnerChanged(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner)
{
    App ^ app = (App ^) Application::Current;
    app->UpdateStatus("Name Owner Changed (WKN=<" + busName + "> prevOwner=<" + previousOwner + "> newOwner=<" + newOwner + ">)");
}

// Called when there's been a join session request from the client
bool MyBusListener::AcceptSessionJoiner(unsigned short sessionPort, Platform::String ^ joiner, SessionOpts ^ sessionOpts)
{
    if (CHAT_PORT == sessionPort) {
        App ^ app = (App ^) Application::Current;
        app->UpdateStatus("Accepting join session request from '" + joiner + "'.");
        return true;
    }
    return false;
}
//	Called when a session has been joined by client
void MyBusListener::SessionJoined(unsigned short sessionPort, unsigned int sessId, Platform::String ^ joiner)
{
    App ^ app = (App ^) Application::Current;
    app->UpdateStatus("'" + joiner + "' has susccessfully joined service session (sessionId=" + sessId + ")");
    hostedSessionId = sessId;
    Platform::Array<unsigned int>^ timeOut = ref new Platform::Array<unsigned int>(1);
    try {

        IAsyncOperation<AllJoyn::SetLinkTimeoutResult ^>^ timeOutOp = busListener->Bus->SetLinkTimeoutAsync(sessId, 40, timeOut);
        concurrency::task<AllJoyn::SetLinkTimeoutResult ^> timeOutTask(timeOutOp);
        timeOutTask.then( [this, app] (concurrency::task<AllJoyn::SetLinkTimeoutResult ^> timeOutTask)
                          {
                              AllJoyn::SetLinkTimeoutResult ^ timeOutResults = timeOutTask.get();
                              if (QStatus::ER_OK != timeOutResults->Status) {
                                  app->UpdateStatus("A problem occurred when setting the link timeout for the session.");
                              }
                          });

    } catch (...) {
        app->UpdateStatus("A problem occurred when setting the link timeout.");
    }
}
//	Called when bus attachment has been disconnected from the D-Bus
void MyBusListener::BusDisconnected()
{
}
//	Called when bus attachment is stopping
void MyBusListener::BusStopping()
{
}
//	Called by the bus when an existing session becomes disconnected.
void MyBusListener::SessionLost(unsigned int sessId)
{
    if (sessionId == sessId) {
        App ^ app = (App ^) Application::Current;
        app->UpdateStatus("Lost session with '" + channelJoined + "'");
        app->UpdateChannelControls(sessId, true);
    } else if (hostedSessionId == sessId) {
        hostedSessionId = 0;
    }
}
//	Called by the bus when a member of a multipoint session is added.
void MyBusListener::SessionMemberAdded(unsigned int sessionId, Platform::String ^ uniqueName)
{
}
//	Called by the bus when a member of a multipoint session is removed.
void MyBusListener::SessionMemberRemoved(unsigned int sessionId, Platform::String ^ uniqueName)
{
}
//	Called by bus attachment when the bus listener is registered
void MyBusListener::ListenerRegistered(BusAttachment ^ busAtt)
{
}
//	Called by bus attachment when the bus listener is unregistered
void MyBusListener::ListenerUnregistered()
{
}
//	Gets the substring of a WKN to exclude the prefix
Platform::String ^ MyBusListener::GetName(Platform::String ^ wellKnownName)
{
    const wchar_t* raw = wellKnownName->Data();
    const int size = wellKnownName->Length() - NAME_PREFIX->Length();
    wchar_t* dest = (wchar_t*)malloc((size + 1) * sizeof(*dest));
    wcsncpy_s(dest, size + 1, raw + NAME_PREFIX->Length(), size);
    Platform::String ^ subString = ref new Platform::String(dest, size);
    free((void*)dest);
    return subString;
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
