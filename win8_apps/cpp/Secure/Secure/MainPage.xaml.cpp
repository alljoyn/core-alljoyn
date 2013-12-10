//-----------------------------------------------------------------------
// <copyright file="MainPage.xaml.cpp" company="AllSeen Alliance.">
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

#include "pch.h"
#include "MainPage.xaml.h"

#include "AllJoynObjects.h"
#include <ppltasks.h>

using namespace Secure;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace std;
using namespace AllJoyn;
using namespace Concurrency;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::System::Threading;
using namespace AllJoynObjects;

/* constants for basic service application */
Platform::String ^ Secure::INTERFACE_NAME = "org.alljoyn.bus.samples.secure.SecureInterface";
Platform::String ^ Secure::SERVICE_NAME = "org.alljoyn.bus.samples.secure";
Platform::String ^ Secure::SERVICE_PATH = "/SecureService";
Platform::String ^ Secure::CONNECT_SPECS = "tcp:addr=127.0.0.1,port=9956";
unsigned short Secure::SERVICE_PORT = 42;

bool running = false;

// Context required to respond to a RequestCredentials request
AuthContext ^ Secure::context = nullptr;

Platform::String ^ startServer = "Start Server";
Platform::String ^ stopServer = "Stop Server";
Platform::String ^ startClient = "Start Client";
Platform::String ^ stopClient = "Stop Client";

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
    InitializeComponent();

    App ^ app = (App ^) Application::Current;

    app->SetUiPage(this);
}

//	Output text to the UI's textbox
void MainPage::OutputLine(Platform::String ^ msg) {

    ArgumentObject ^ ao = ref new ArgumentObject(msg + "\n", this->textBoxOutput);

    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                             [this, ao] ()
                             {
                                 ao->OnDispactched();
                             }));
}

// Output the pin to the user when the service is running for authentication
void MainPage::OutputPin(Platform::String ^ msg)
{
    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                             [this, msg] ()
                             {
                                 textBoxPinServer->Text = msg;
                             }));
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs ^ e)
{
    (void) e;           // Unused parameter
}

//	Primary bus attachmet which allows interactions over the D-Bus
BusAttachment ^ busAtt = nullptr;

///////////////////////////////////Client////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

//	Bus Object which proxys the service's interface allowing client interaction
ProxyBusObject ^ proxyBusObject = nullptr;
//	Bus Listener which handles events happening over the bus
ClientBusListener ^ clientBusListener = nullptr;
// SessionId of the current client-service session
unsigned int clientSessionId = 0;

// Called when the start client button is clicked and can be in two states.
// --> If application is not running this call will start the client, authorize the user
//     with the service and call the ping function.
// --> If the application is already running the client will be disconnected and stopped
void Secure::MainPage::StartClientClick(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    //check to see if the client needs to be set up or stopped
    if (!running && startClient == buttonClient->Content->ToString()) {
        //start up the client
        running = true;

        OutputLine("Establishing the bus, listeners and handlers for the client app...");
        try {
            // Set up the bus attachment, listener and proxy bus object for application
            Platform::String ^ appName = "SecureClient";
            busAtt = ref new BusAttachment(appName, true, 4);

            clientBusListener = ref new ClientBusListener(busAtt);
            busAtt->RegisterBusListener(clientBusListener->GetBusListener);

            proxyBusObject = ref new ProxyBusObject(busAtt, SERVICE_NAME, SERVICE_PATH, 0);

            busAtt->Start();

            //	Called when well-known service name is discovered, then tries to establish a session
            BusListener ^ bl = clientBusListener->GetBusListener;
            bl->FoundAdvertisedName +=
                ref new BusListenerFoundAdvertisedNameHandler([this]
                                                                  (Platform::String ^ name, TransportMaskType transportMask, Platform::String ^ namePrefix)
                                                              {
                                                                  OutputLine("Found well-known service name: " + namePrefix + ".");
                                                                  //	Send a session request to the service discovered with well-known name of interest
                                                                  SessionOpts ^ sessionOpts = ref new SessionOpts(TrafficType::TRAFFIC_MESSAGES, false,
                                                                                                                  ProximityType::PROXIMITY_ANY, TransportMaskType::TRANSPORT_ANY);
                                                                  Platform::Array<SessionOpts ^, 1>^ optsOut = { ref new SessionOpts() };

                                                                  IAsyncOperation<AllJoyn::JoinSessionResult ^>^ joinOp =
                                                                      busAtt->JoinSessionAsync(SERVICE_NAME, SERVICE_PORT, clientBusListener->GetSessionListener, sessionOpts, optsOut, nullptr);
                                                                  concurrency::task<AllJoyn::JoinSessionResult ^> joinTask(joinOp);
                                                                  joinTask.then( [this] (concurrency::task<AllJoyn::JoinSessionResult ^> joinTask)
                                                                                 {
                                                                                     AllJoyn::JoinSessionResult ^ joinResults = joinTask.get();
                                                                                     QStatus status = joinResults->Status;
                                                                                     if (QStatus::ER_OK == status) {
                                                                                         clientSessionId = joinResults->SessionId;
                                                                                         OutputLine("Join session was successful (sessionId=" + joinResults->SessionId + ").");

                                                                                         Platform::Array<unsigned int>^ timeOut = ref new Platform::Array<unsigned int>(1);
                                                                                         return busAtt->SetLinkTimeoutAsync(joinResults->SessionId, 40, timeOut);

                                                                                     } else {
                                                                                         OutputLine("Join session request was unsuccessful.");
                                                                                     }

                                                                                 }).then([this] (concurrency::task<AllJoyn::SetLinkTimeoutResult ^> timeOutTask)
                                                                                         {
                                                                                             AllJoyn::SetLinkTimeoutResult ^ timeOutResults = timeOutTask.get();
                                                                                             if (QStatus::ER_OK != timeOutResults->Status) {
                                                                                                 OutputLine("A problem occurred when setting the link timeout for the session.");
                                                                                             }
                                                                                             return proxyBusObject->IntrospectRemoteObjectAsync(nullptr);

                                                                                         }).then( [this] (concurrency::task<AllJoyn::IntrospectRemoteObjectResult ^> introspectTask)
                                                                                                  {
                                                                                                      IntrospectRemoteObjectResult ^ introResult = introspectTask.get();
                                                                                                      if (QStatus::ER_OK == introResult->Status) {
                                                                                                          OutputLine("Introspection of the service bus object was successful.");
                                                                                                          try{
                                                                                                              CallPingMethod();
                                                                                                          }catch (...) {
                                                                                                              OutputLine("A problem occurred when calling the 'Ping' method.");
                                                                                                          }
                                                                                                      } else {
                                                                                                          OutputLine("Introspection of the service bus object was unsuccessful.");
                                                                                                          busAtt->LeaveSession(clientSessionId);
                                                                                                      }
                                                                                                  });
                                                              });

            // Set up the authorized security for the Ping function
            // Note: This must be done b/t busAtt->Start() and busAtt->ConnectAsync()
            busAtt->EnablePeerSecurity("ALLJOYN_SRP_KEYX", clientBusListener->GetAuthListener, "/.alljoyn_keystore/s_central.ks", true);

            if (clearKS_checkBox->IsChecked) {
                // Clears previous authenications that have been stored for service to recognize the user
                busAtt->ClearKeyStore();
            }

            ConnectClientToAllJoyn();
        } catch (...) {
            OutputLine("A problem occurred while trying to start the client application. Exiting.");
            busAtt = nullptr;
            clientBusListener = nullptr;
            proxyBusObject = nullptr;
            running = false;
        }
    } else if (running && stopClient == buttonClient->Content->ToString()) {
        //Tear down the client
        ClientTearDown();
    }
}

//	Connects to alljoyn by creating bundled daemon and connecting bus attachment. Looks for
//	well-known name after completion
void MainPage::ConnectClientToAllJoyn()
{
    OutputLine("Connecting to AllJoyn...");
    auto connectTask = create_task(busAtt->ConnectAsync(CONNECT_SPECS));
    connectTask.then( [this, connectTask] ()
                      {
                          try {
                              connectTask.get();

                              OutputLine("Successfully connected to AllJoyn.");
                              busAtt->FindAdvertisedName(SERVICE_NAME); \
                              Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                                       [this] ()
                                                       {
                                                           buttonClient->Content = stopClient;
                                                       }));
                          } catch (...) {
                              ConnectClientToAllJoyn();
                          }
                      });
}

// Call the service's 'Ping' method with a default message and print out return.
// **Authentication will begin when trying to call the service's 'Ping' method.
void MainPage::CallPingMethod()
{
    Platform::String ^ clientMsg = "Hello from a Client";
    Platform::Array<Object ^, 1>^ argArray = { clientMsg };
    MsgArg ^ arg = ref new MsgArg("s", argArray);
    Platform::Array<MsgArg ^, 1>^ args = { arg };

    InterfaceDescription ^ interfaceDescription = busAtt->GetInterface(INTERFACE_NAME);
    InterfaceMember ^ member = interfaceDescription->GetMember("Ping");

    IAsyncOperation<AllJoyn::MethodCallResult ^>^ methodCallOp =
        proxyBusObject->MethodCallAsync(member, args, nullptr, (unsigned int)1000, (unsigned char)0);
    concurrency::task<AllJoyn::MethodCallResult ^> methodCallTask(methodCallOp);
    methodCallTask.then( [this, clientMsg] (concurrency::task<AllJoyn::MethodCallResult ^> methodCallTask)
                         {
                             OutputLine("Called the 'Ping' method with '" + clientMsg + "'");
                             AllJoyn::MethodCallResult ^ results = methodCallTask.get();
                             if (results->Message->Type == AllJoynMessageType::MESSAGE_METHOD_RET) {
                                 OutputLine("The 'Ping' method call was successful and returned:");
                                 OutputLine(results->Message->GetArg(0)->Value->ToString());
                             } else {
                                 OutputLine("Authentication has failed or the method call to 'Ping' has produced errors.");
                             }
                             ClientTearDown();
                         });
}

// During authentication the user will be asks for a pin that will be submitted through this event
// by responding to the RequestCredentials query
void Secure::MainPage::SendPinClick(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    Platform::String ^ userPin = MainPage::textBoxPinClient->Text;
    if ("" != userPin && nullptr != context) {
        Credentials ^ creds = ref new Credentials();
        creds->Password = userPin;
        try {
            clientBusListener->GetAuthListener->RequestCredentialsResponse(context, true, creds);
        } catch (...) {
            OutputLine("The Request Credentials response was unsuccessful.");
        }
        context = nullptr;
    } else {
        OutputLine("You must be in the authentication procedure and enter a pin to submit authentication.");
    }
}

// Tear down the client application by disconnecting and stopping the bus
void MainPage::ClientTearDown()
{
    if (running && nullptr != busAtt) {
        //	tear down bus
        IAsyncAction ^ disconnectOp = busAtt->DisconnectAsync(CONNECT_SPECS);
        concurrency::task<void> disconnectTask(disconnectOp);
        disconnectTask.then( [this] ()
                             {
                                 return busAtt->StopAsync();
                             }).then( [this] ()
                                      {
                                          busAtt = nullptr;
                                          clientBusListener = nullptr;
                                          proxyBusObject = nullptr;
                                          OutputLine("Client has been disconnected and terminated.\n");
                                          Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                                                   [this] ()
                                                                   {
                                                                       buttonClient->Content = startClient;
                                                                   }));
                                          running = false;
                                      });
    }
}


///////////////////////////////////SERVICE////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

//	Bus Object which implements the interface over the bus for clients to interact with
SecureServiceObject ^ busObject = nullptr;
//	Bus Listener which handles events happening over the bus
ServiceBusListener ^ serviceBusListener = nullptr;

// Called when the start server button is clicked and can be in two states.
// --> If application is not running this call will start the server, bind session port and
//	   advertise the service for clients to join and call the ping method
// --> If the application is already running it will tear down the service
void Secure::MainPage::StartServerClick(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    //check whether we need to start or stop the service
    if (!running && startServer == buttonStartServer->Content->ToString()) {
        running = true;
        // start the service
        OutputLine("Establishing the bus, listeners and handlers for the service app...");

        try {
            //	Create and register components of the secure service
            Platform::String ^ appName = "SecureService";
            busAtt = ref new BusAttachment(appName, true, 4);

            serviceBusListener = ref new ServiceBusListener(busAtt);
            busAtt->RegisterBusListener(serviceBusListener->GetBusListener);

            busObject = ref new SecureServiceObject(busAtt, SERVICE_PATH);
            busAtt->RegisterBusObject(busObject->GetBusObject);

            busAtt->Start();

            // Set up the authorized security for the Ping function
            // Note: This must be done b/t busAtt->Start() and busAtt->ConnectAsync()
            busAtt->EnablePeerSecurity("ALLJOYN_SRP_KEYX", serviceBusListener->GetAuthListener, "/.alljoyn_keystore/s_central.ks", true);

            ConnectServerToAllJoyn();
        } catch (...) {
            OutputLine("Couldn't establish the service application.");
            busAtt = nullptr;
            serviceBusListener = nullptr;
            busObject = nullptr;
            running = false;
        }
    } else if (running && stopServer == buttonStartServer->Content->ToString()) {
        // stop the service
        ServerTearDown();
    }
}

//	Connects to alljoyn by creating bundled daemon and connecting bus attachment
void MainPage::ConnectServerToAllJoyn()
{
    OutputLine("Connecting to AllJoyn...");
    auto connectTask = create_task(busAtt->ConnectAsync(CONNECT_SPECS));
    connectTask.then( [this, connectTask] ()
                      {
                          try {
                              connectTask.get();

                              BuildService();
                          } catch (...) {
                              ConnectServerToAllJoyn();
                          }
                      });
}

//	Executed after alljoyn connection has been established then sets up service by
//	1)	Binding Session Port	2)	Requesting well-known name	3)	Advertising well-known name
void MainPage::BuildService()
{
    OutputLine("Successfully Connected to the AllJoyn bus.");

    try {
        SessionOpts ^ sessionOpts = ref new SessionOpts(TrafficType::TRAFFIC_MESSAGES,
                                                        false, ProximityType::PROXIMITY_ANY, TransportMaskType::TRANSPORT_ANY);
        Platform::Array<unsigned short>^ portOut = ref new Platform::Array<unsigned short>(1);

        busAtt->BindSessionPort(SERVICE_PORT, portOut, sessionOpts, serviceBusListener->GetSessionPortListener);

        OutputLine("Binding session port (Port#=" + SERVICE_PORT + ")...");

        unsigned int flags = ((unsigned int)RequestNameType::DBUS_NAME_DO_NOT_QUEUE) | ((unsigned int)RequestNameType::DBUS_NAME_REPLACE_EXISTING);
        busAtt->RequestName(SERVICE_NAME, flags);

        OutputLine("Requesting the well-known name '" + SERVICE_NAME + "'...");

        busAtt->AdvertiseName(SERVICE_NAME, TransportMaskType::TRANSPORT_ANY);

        OutputLine("Advertising the well-known name '" + SERVICE_NAME + "' for clients to discover...");

        Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                 [this] ()
                                 {
                                     buttonStartServer->Content = stopServer;
                                 }));
    } catch (Platform::Exception ^ ex) {
        OutputLine("Could not successfully build the serivce.");
        OutputLine("Exception: " + ex->Message->ToString());
        ServerTearDown();
    }
}

// Tear down the server by disconnecting and stopping the busAtt
void MainPage::ServerTearDown()
{
    if (running) {
        //	Tear down bus attachment and terminate service
        IAsyncAction ^ disconnectOp = busAtt->DisconnectAsync(CONNECT_SPECS);
        concurrency::task<void> disconnectTask(disconnectOp);
        disconnectTask.then( [this] ()
                             {
                                 return busAtt->StopAsync();
                             }).then( [this] ()
                                      {
                                          busAtt = nullptr;
                                          serviceBusListener = nullptr;
                                          busObject = nullptr;

                                          OutputLine("The Basic Service Application has been terminated.\n");
                                          Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                                                   [this] ()
                                                                   {
                                                                       buttonStartServer->Content = startServer;
                                                                   }));
                                          running = false;
                                      });
    }
}