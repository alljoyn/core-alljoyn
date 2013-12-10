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

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vector>
#include <ppltasks.h>

using namespace BasicClient;
using namespace std;
using namespace Windows::System::Threading;

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
using namespace AllJoyn;
using namespace Concurrency;
using namespace Windows::UI::Core;
using namespace AllJoynObjects;

/* constants for basic client application */
Platform::String ^ INTERFACE_NAME = "org.alljoyn.Bus.sample";
Platform::String ^ SERVICE_NAME = "org.alljoyn.Bus.sample";
Platform::String ^ SERVICE_PATH = "/sample";
Platform::String ^ CONNECT_SPECS = "tcp:addr=127.0.0.1,port=9956";
unsigned short SERVICE_PORT = 25;

unsigned int sessionId = 0;
bool running = false;

//	Primary bus attachmet which allows interactions over the D-Bus
BusAttachment ^ busAtt = nullptr;
//	Remote object which allows interaction with the service's bus object
ProxyBusObject ^ proxyBusObject = nullptr;
//	Bus Listener which handles events happening over the bus
MyBusListener ^ busListener = nullptr;

// Creates an instance of the MainPage
MainPage::MainPage()
{
    InitializeComponent();

    App ^ app = (App ^) Application::Current;

    app->SetUiPage(this);
}

//	Output text to the UI's textbox
void MainPage::OutputLine(Platform::String ^ msg) {

    ArgumentObject ^ ao = ref new ArgumentObject(msg + "\n", this->TextBlockClient);

    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                             [this, ao] ()
                             {
                                 ao->OnDispactched();
                             }));
}

/// Invoked when this page is about to be displayed in a Frame.
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs ^ e)
{
    (void) e;           // Unused parameter

    // This logging is useful for debugging purposes but should not be
    // used for release versions. The output will be in the file "alljoyn.log" in
    // current users Documents directory.
    AllJoyn::Debug::UseOSLogging(true);
    AllJoyn::Debug::SetDebugLevel("TCP", 7);
    AllJoyn::Debug::SetDebugLevel("ALLJOYN", 7);
    AllJoyn::Debug::SetDebugLevel("ALLJOYN_OBJ", 7);
    AllJoyn::Debug::SetDebugLevel("ALLJOYN_DAEMON", 7);
}

// Called when the start client button is clicked to start the basic client which calls the
// 'cat' method of the service providing "Hello " and "World!" as arguments
void BasicClient::MainPage::Button_RunClient(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    if (!running && busAtt == nullptr) {
        try {
            running = true;

            OutputLine("Establishing the bus, listeners and handlers...");
            // Set up the bus attachment, listener and proxy bus object for application
            Platform::String ^ appName = "basicClient";
            busAtt = ref new BusAttachment(appName, true, 4);

            proxyBusObject = ref new ProxyBusObject(busAtt, SERVICE_NAME, SERVICE_PATH, 0);

            busListener = ref new MyBusListener(busAtt, proxyBusObject);
            busAtt->RegisterBusListener(busListener->GetBusListener);

            busAtt->Start();

            //	Called when well-known service name is discovered, then tries to establish a session
            BusListener ^ bl = busListener->GetBusListener;
            bl->FoundAdvertisedName += ref new BusListenerFoundAdvertisedNameHandler([this]
                                                                                         (Platform::String ^ name, TransportMaskType transportMask, Platform::String ^ namePrefix)
                                                                                     {
                                                                                         //	Send a session request to the service discovered with well-known name of interest
                                                                                         SessionOpts ^ sessionOpts = ref new SessionOpts(TrafficType::TRAFFIC_MESSAGES, false,
                                                                                                                                         ProximityType::PROXIMITY_ANY, TransportMaskType::TRANSPORT_ANY);
                                                                                         Platform::Array<SessionOpts ^, 1>^ optsOut = { ref new SessionOpts() };

                                                                                         IAsyncOperation<AllJoyn::JoinSessionResult ^>^ joinOp =
                                                                                             busAtt->JoinSessionAsync(SERVICE_NAME, SERVICE_PORT, busListener->GetSessionListener, sessionOpts, optsOut, nullptr);
                                                                                         concurrency::task<AllJoyn::JoinSessionResult ^> joinTask(joinOp);
                                                                                         joinTask.then( [this] (concurrency::task<AllJoyn::JoinSessionResult ^> joinTask)
                                                                                                        {
                                                                                                            AllJoyn::JoinSessionResult ^ joinResults = joinTask.get();
                                                                                                            QStatus status = joinResults->Status;
                                                                                                            if (QStatus::ER_OK == status) {
                                                                                                                sessionId = joinResults->SessionId;
                                                                                                                OutputLine("Join Session was successful (sessionId=" + sessionId + ").");

                                                                                                                return proxyBusObject->IntrospectRemoteObjectAsync(nullptr);
                                                                                                            } else {
                                                                                                                OutputLine("Join Session was unsuccessful.");
                                                                                                            }
                                                                                                        }).then( [this] (concurrency::task<AllJoyn::IntrospectRemoteObjectResult ^> introspectTask)
                                                                                                                 {
                                                                                                                     IntrospectRemoteObjectResult ^ introResults = introspectTask.get();
                                                                                                                     if (QStatus::ER_OK == introResults->Status) {
                                                                                                                         OutputLine("Introspection of the service bus object was successful.");
                                                                                                                         CallCatMethod();
                                                                                                                     } else {
                                                                                                                         this->OutputLine("Introspection of the service bus object was unsuccessful.");
                                                                                                                         busAtt->LeaveSession(sessionId);
                                                                                                                     }
                                                                                                                 });
                                                                                     });

            ConnectAllJoyn();
        } catch (Platform::Exception ^ ex) {
            OutputLine("A problem occurred while trying to connect to the service.");
            OutputLine("Exception: " + ex->Message->ToString());
            running = false;
            busAtt = nullptr;
        }

    }
}

// Call the 'cat' method implemented by the service with args 'Hello ' and 'World!',
// expecting 'Hello World!' as the return argument
void MainPage::CallCatMethod()
{
    //	Call the 'cat' method implemented by the service
    Platform::Array<Object ^, 1>^ argArray1 = { "Hello " };
    MsgArg ^ arg1 = ref new MsgArg("s", argArray1);

    Platform::Array<Object ^, 1>^ argArray2 = { "World!" };
    MsgArg ^ arg2 = ref new MsgArg("s", argArray2);

    Platform::Array<MsgArg ^, 1>^ args = { arg1, arg2 };

    OutputLine("Calling the 'cat' method with arguments 'Hello ' and 'World!'.");
    InterfaceDescription ^ interfaceDescription = busAtt->GetInterface(INTERFACE_NAME);
    InterfaceMember ^ member = interfaceDescription->GetMember("cat");

    IAsyncOperation<AllJoyn::MethodCallResult ^>^ methodCallOp =
        proxyBusObject->MethodCallAsync(member, args, nullptr, (unsigned int)1000, (unsigned char)0);
    concurrency::task<AllJoyn::MethodCallResult ^> methodCallTask(methodCallOp);
    methodCallTask.then( [this] (concurrency::task<AllJoyn::MethodCallResult ^> methodCallTask)
                         {
                             AllJoyn::MethodCallResult ^ callResults = methodCallTask.get();
                             if (AllJoynMessageType::MESSAGE_METHOD_RET == callResults->Message->Type) {
                                 Message ^ message = callResults->Message;
                                 Platform::String ^ sender = message->Sender;
                                 Platform::String ^ result = message->GetArg(0)->Value->ToString();

                                 OutputLine("Reply from '" + sender + "' returned value '" + result + "'.");
                             } else {
                                 AllJoynMessageType err = callResults->Message->Type;
                                 OutputLine("The 'cat' method call produced errors.");
                             }

                             TearDown();
                         });
}

// Tear down the client application by disconnecting and stop the bus attachment
void MainPage::TearDown()
{
    //	tear down bus
    IAsyncAction ^ disconnectOp = busAtt->DisconnectAsync(CONNECT_SPECS);
    concurrency::task<void> disconnectTask(disconnectOp);
    disconnectTask.then( [this] ()
                         {
                             return busAtt->StopAsync();
                         }).then( [this] ()
                                  {
                                      busAtt = nullptr;
                                      busListener = nullptr;
                                      proxyBusObject = nullptr;
                                      running = false;
                                      OutputLine("Client has been disconnected and terminated.\n");
                                  });
}

//	Connects to alljoyn by creating bundled daemon and connecting bus attachment. Looks for
//	well-known name after completion
void MainPage::ConnectAllJoyn()
{
    OutputLine("Connecting to AllJoyn...");
    auto connectTask = create_task(busAtt->ConnectAsync(CONNECT_SPECS));
    connectTask.then( [this, connectTask] ()
                      {
                          try {
                              connectTask.get();

                              OutputLine("Successfully Connected to the AllJoyn bus.");
                              busAtt->FindAdvertisedName(SERVICE_NAME);
                          } catch (...) {
                              ConnectAllJoyn();
                          }
                      });
}