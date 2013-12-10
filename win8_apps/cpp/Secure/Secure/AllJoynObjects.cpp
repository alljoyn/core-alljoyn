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

using namespace Secure;
using namespace Windows::UI::Xaml::Controls;
using namespace AllJoyn;
using namespace Windows::UI::Xaml;
using namespace concurrency;
using namespace Windows::Foundation;
using namespace Windows::Security::Cryptography;
using namespace AllJoynObjects;

//////////////////////////////ArgumentObject////////////////////////////////

//	Encapsulation object for dispatcher to use when printing message to UI
ArgumentObject::ArgumentObject(Platform::String ^ msg, TextBox ^ tb) : text(nullptr), textBox(nullptr)
{
    text = msg;
    textBox = tb;
}
void ArgumentObject::OnDispactched() {
    textBox->Text += text;
}


//////////////////////////////SecureServiceObject////////////////////////////////

SecureServiceObject::SecureServiceObject(BusAttachment ^ busAtt, Platform::String ^ path) :
    busObject(nullptr)
{
    busObject = ref new BusObject(busAtt, path, false);

    //	Create the 'Ping' interface
    InterfaceDescription ^ interfaceDescription = nullptr;
    Platform::Array<InterfaceDescription ^, 1>^ intfArray = { interfaceDescription };
    busAtt->CreateInterface(INTERFACE_NAME, intfArray, true);
    intfArray[0]->AddMethod("Ping", "s", "s", "inStr,outStr", (unsigned char)0, "");
    intfArray[0]->Activate();

    busObject->AddInterface(intfArray[0]);

    App ^ app = (App ^) Application::Current;
    Platform::String ^ text = "Created the 'Ping' method interface.";
    app->OutputLine(text);

    // Register the 'Ping' method handlers with the bus object
    InterfaceMember ^ member = intfArray[0]->GetMember("Ping");
    MessageReceiver ^ receiver = ref new MessageReceiver(busAtt);
    receiver->MethodHandler += ref new MessageReceiverMethodHandler([this]
                                                                        (InterfaceMember ^ member, Message ^ message)
                                                                    {
                                                                        Ping(member, message);
                                                                    }
                                                                    );

    busObject->AddMethodHandler(member, receiver);
}

// Handles calls to the service's 'Ping' method by printing out provided message to user
// and replying to caller with their message
void SecureServiceObject::Ping(InterfaceMember ^ member, Message ^ message)
{
    Platform::String ^ pingMsg = message->GetArg(0)->Value->ToString();
    Platform::String ^ sender = message->Sender;

    App ^ app = (App ^) Application::Current;
    app->OutputLine(sender + " says: '" + pingMsg + "'");

    //	Reply to the sender with message
    Platform::Array<Object ^>^ arr = { (Object ^)pingMsg };
    MsgArg ^ returnArg = ref new MsgArg("s", arr);
    Platform::Array<MsgArg ^>^ returnArray = { returnArg };

    app->OutputLine("Replying to '" + sender + "' with " + pingMsg + ".\n");
    busObject->MethodReply(message, returnArray);
}

// return a reference to the bus object
BusObject ^ SecureServiceObject::GetBusObject::get() {
    return busObject;
}


//////////////////////////////////ServiceBusListener////////////////////////////////////

ServiceBusListener::ServiceBusListener(BusAttachment ^ busAtt) :
    busListener(nullptr), sessionPortListener(nullptr), sessionListener(nullptr), authListener(nullptr)
{
    /* Create Auth Listener and register all event handlers  */
    authListener = ref new AuthListener(busAtt);
    authListener->RequestCredentials +=
        ref new AuthListenerRequestCredentialsAsyncHandler( [this]
                                                                (Platform::String ^ authMechanism, Platform::String ^ peerName, unsigned short authCount,
                                                                Platform::String ^ userName, unsigned short credMask, AuthContext ^ authContext)->QStatus
                                                            {
                                                                return RequestCredentials(authMechanism, peerName, authCount, userName, credMask, authContext);
                                                            });
    authListener->AuthenticationComplete +=
        ref new AuthListenerAuthenticationCompleteHandler( [this]
                                                               (Platform::String ^ authMechanism, Platform::String ^ peerName, bool success)
                                                           {
                                                               AuthenticationComplete(authMechanism, peerName, success);
                                                           });

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
//	Called when user credentials are requested and generates a random pin for client to enter.
//	Authentication mechanism requests user credentials. If the user name is not an empty string
//	the request is for credentials for that specific user. A count allows the listener to decide
//	whether to allow or reject multiple authentication attempts to the same peer.
QStatus ServiceBusListener::RequestCredentials(Platform::String ^ authMechanism, Platform::String ^ peerName, unsigned short authCount,
                                               Platform::String ^ userName, unsigned short credMask, AuthContext ^ authContext)
{
    App ^ app = (App ^) Application::Current;
    if ("ALLJOYN_SRP_KEYX" == authMechanism && (credMask & (unsigned short)CredentialType::CRED_PASSWORD)) {
        if (authCount <= 3) {
            app->OutputLine("RequestCredentials for authenticating " + peerName + " using " + authMechanism + " mechanism.");
            // generate random 6 digit pin number
            unsigned int random = CryptographicBuffer::GenerateRandomNumber();
            random %= 1000000;
            if (random < 100000) {
                random += 100000;
            }
            Platform::String ^ pinStr = random.ToString();
            app->OutputPin(pinStr);
            app->OutputLine("One Time Password shown above.");
            Credentials ^ creds = ref new Credentials();
            creds->Expiration = 120;
            creds->Password = pinStr;
            authListener->RequestCredentialsResponse(authContext, true, creds);

            return QStatus::ER_OK;
        }
    }
    return QStatus::ER_AUTH_FAIL;
}
//	Reports successful or unsuccessful completion of authentication.
void ServiceBusListener::AuthenticationComplete(Platform::String ^ authMechanism, Platform::String ^ peerName, bool success)
{
    App ^ app = (App ^) Application::Current;
    if (success) {
        app->OutputLine("Authentification for " + peerName + " was " + "successful!");
    } else {
        app->OutputLine("Authentification for " + peerName + " was " + "unsuccessful.");
    }
}

// Called by the bus when an external bus is discovered that is advertising a well-known
// name that this attachment has registered interest in via a DBus call to
// org.alljoyn.Bus.FindAdvertisedName
void ServiceBusListener::FoundAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
{
}
// Called by the bus when an advertisement previously reported through FoundName has become
// unavailable.
void ServiceBusListener::LostAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
{
}
// Called when the owner of a well-known name changes
void ServiceBusListener::NameOwnerChanged(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Name Owner Changed (wkn=" + busName + " prevOwner=" + previousOwner + " newOwner=" + newOwner + ")");
}

// Called when there's been a join session request from the client
bool ServiceBusListener::AcceptSessionJoiner(unsigned short sessionPort, Platform::String ^ joiner, SessionOpts ^ sessionOpts)
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
void ServiceBusListener::SessionJoined(unsigned short sessionPort, unsigned int sessId, Platform::String ^ joiner)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Join Session Success (sessionId=" + sessId + ")");
    Platform::Array<unsigned int>^ timeOut = ref new Platform::Array<unsigned int>(1);
    try {

        IAsyncOperation<AllJoyn::SetLinkTimeoutResult ^>^ timeOutOp = busListener->Bus->SetLinkTimeoutAsync(sessId, 40, timeOut);
        concurrency::task<AllJoyn::SetLinkTimeoutResult ^> timeOutTask(timeOutOp);
        timeOutTask.then( [this, app] (concurrency::task<AllJoyn::SetLinkTimeoutResult ^> timeOutTask)
                          {
                              AllJoyn::SetLinkTimeoutResult ^ timeOutResults = timeOutTask.get();
                              if (QStatus::ER_OK != timeOutResults->Status) {
                                  app->OutputLine("A problem occurred when setting the link timeout for the session.");
                              }
                          });

    } catch (...) {
        app->OutputLine("A problem occurred when setting the link timeout.");
    }
}
//	Called when a BusAttachment this listener is registered with is has become disconnected from the bus.
void ServiceBusListener::BusDisconnected()
{
}
//	Called when a BusAttachment this listener is registered with is stopping.
void ServiceBusListener::BusStopping()
{
}
//	Called by the bus when the listener is registered.
void ServiceBusListener::ListenerRegistered(BusAttachment ^ busAtt)
{
}
//	Called by the bus when the listener is unregistered.
void ServiceBusListener::ListenerUnregistered()
{
}
//	Called by the bus when an existing session becomes disconnected.
void ServiceBusListener::SessionLost(unsigned int sessId)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Session Lost (sessionId=" + sessId + ")");
}
//	Called by the bus when a member of a multipoint session is added.
void ServiceBusListener::SessionMemberAdded(unsigned int sessionId, Platform::String ^ uniqueName)
{
}
//	Called by the bus when a member of a multipoint session is removed.
void ServiceBusListener::SessionMemberRemoved(unsigned int sessionId, Platform::String ^ uniqueName)
{
}
// return a reference to the bus listener
BusListener ^ ServiceBusListener::GetBusListener::get() {
    return busListener;
}
// return a reference to the session listener
SessionListener ^ ServiceBusListener::GetSessionListener::get() {
    return sessionListener;
}
// return a reference to the session port listener
SessionPortListener ^ ServiceBusListener::GetSessionPortListener::get() {
    return sessionPortListener;
}
// return a reference to the auth listener
AuthListener ^ ServiceBusListener::GetAuthListener::get() {
    return authListener;
}


//////////////////////////////////ClientBusListener////////////////////////////////////

ClientBusListener::ClientBusListener(BusAttachment ^ busAtt) :
    busListener(nullptr), sessionListener(nullptr), authListener(nullptr)
{
    /* Create Auth Listener and register all event handlers  */
    authListener = ref new AuthListener(busAtt);
    authListener->RequestCredentials +=
        ref new AllJoyn::AuthListenerRequestCredentialsAsyncHandler( [this]
                                                                         (Platform::String ^ authMechanism, Platform::String ^ peerName, unsigned short authCount,
                                                                         Platform::String ^ userName, unsigned short credMask, AuthContext ^ authContext)->QStatus
                                                                     {
                                                                         return RequestCredentials(authMechanism, peerName, authCount, userName, credMask, authContext);
                                                                     });
    authListener->AuthenticationComplete +=
        ref new AuthListenerAuthenticationCompleteHandler( [this]
                                                               (Platform::String ^ authMechanism, Platform::String ^ peerName, bool success)
                                                           {
                                                               AuthenticationComplete(authMechanism, peerName, success);
                                                           });

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

//	Called when user credentials are requested.
//	Authentication mechanism requests user credentials. If the user name is not an empty string
//	the request is for credentials for that specific user. A count allows the listener to decide
//	whether to allow or reject multiple authentication attempts to the same peer.
QStatus ClientBusListener::RequestCredentials(Platform::String ^ authMechanism, Platform::String ^ peerName, unsigned short authCount,
                                              Platform::String ^ userName, unsigned short credMask, AuthContext ^ authContext)
{
    App ^ app = (App ^) Application::Current;
    if ("ALLJOYN_SRP_KEYX" == authMechanism && (credMask & (unsigned short)CredentialType::CRED_PASSWORD)) {
        if (authCount <= 3) {
            app->OutputLine("RequestCredentials for authenticating " + peerName + " using " + authMechanism + " mechanism.");
            app->OutputLine("Please Enter Password in CommandLine and Click Send: ");

            //store context for the response
            context = authContext;

            return QStatus::ER_OK;
        }
    }
    return QStatus::ER_AUTH_FAIL;
}
//	Reports successful or unsuccessful completion of authentication.
void ClientBusListener::AuthenticationComplete(Platform::String ^ authMechanism, Platform::String ^ peerName, bool success)
{
    App ^ app = (App ^) Application::Current;
    if (success) {
        app->OutputLine("Authentification for " + peerName + " was " + "successful!");
    } else {
        app->OutputLine("Authentification for " + peerName + " was " + "unsuccessful.");
    }
}

// Called by the bus when an external bus is discovered that is advertising a well-known
// name that this attachment has registered interest in via a DBus call to
// org.alljoyn.Bus.FindAdvertisedName
void ClientBusListener::FoundAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
{
}
// Called by the bus when an advertisement previously reported through FoundName has become
// unavailable.
void ClientBusListener::LostAdvertisedName(Platform::String ^ wellKnownName, TransportMaskType transport, Platform::String ^ namePrefix)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Lost Advertised Name '" + wellKnownName + "'.");
}
// Called when the owner of a well-known name changes
void ClientBusListener::NameOwnerChanged(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Name Owner Changed (wkn=" + busName + " prevOwner=" + previousOwner + " newOwner=" + newOwner + ")");
}
//	Called when a BusAttachment this listener is registered with is has become disconnected from the bus.
void ClientBusListener::BusDisconnected()
{
}
//	Called when a BusAttachment this listener is registered with is stopping.
void ClientBusListener::BusStopping()
{
}
//	Called by the bus when the listener is registered.
void ClientBusListener::ListenerRegistered(BusAttachment ^ busAtt)
{
}
//	Called by the bus when the listener is unregistered.
void ClientBusListener::ListenerUnregistered()
{
}
//	Called by the bus when an existing session becomes disconnected.
void ClientBusListener::SessionLost(unsigned int sessId)
{
    App ^ app = (App ^) Application::Current;
    app->OutputLine("Session Lost (sessionId=" + sessId + ")");
}
//	Called by the bus when a member of a multipoint session is added.
void ClientBusListener::SessionMemberAdded(unsigned int sessionId, Platform::String ^ uniqueName)
{
}
//	Called by the bus when a member of a multipoint session is removed.
void ClientBusListener::SessionMemberRemoved(unsigned int sessionId, Platform::String ^ uniqueName)
{
}
// return a reference to the bus listener
BusListener ^ ClientBusListener::GetBusListener::get() {
    return busListener;
}
// return a reference to the session listener
SessionListener ^ ClientBusListener::GetSessionListener::get() {
    return sessionListener;
}
// return a reference to the auth listener
AuthListener ^ ClientBusListener::GetAuthListener::get() {
    return authListener;
}