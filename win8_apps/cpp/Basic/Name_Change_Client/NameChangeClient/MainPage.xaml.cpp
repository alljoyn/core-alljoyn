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

#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <ppltasks.h>

using namespace std;

using namespace NameChangeClient;

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

//	Constants for the name change client application
Platform::String ^ INTERFACE_NAME = "org.alljoyn.Bus.signal_sample";
Platform::String ^ SERVICE_NAME = "org.alljoyn.Bus.signal_sample";
Platform::String ^ SERVICE_PATH = "/";
Platform::String ^ CONNECT_SPECS = "tcp:addr=127.0.0.1,port=9956";
unsigned short SERVICE_PORT = 25;

bool running = false;
unsigned int sessionId = 0;

// Initializes an instance of the main page
MainPage::MainPage()
{
    InitializeComponent();

    App ^ app = (App ^) Application::Current;

    app->SetUiPage(this);
}

//	Output text to the UI's textbox
void MainPage::OutputLine(Platform::String ^ msg) {

    ArgumentObject ^ ao = ref new ArgumentObject(msg + "\n", this->TextNameChange);

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

//	Primary bus attachmet which allows interactions over the D-Bus
BusAttachment ^ busAtt = nullptr;
//	Remote object which allows interaction with the service's bus object
ProxyBusObject ^ proxyBusObject = nullptr;
//	Bus Listener which handles events happening over the bus
MyBusListener ^ busListener = nullptr;

void NameChangeClient::MainPage::Button_RunClient(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    if ("" == this->Input->Text) {
        OutputLine("You must provide a new name for the 'name' property on the command line to run the app.");
    }
    if (!running && busAtt == nullptr && "" != this->Input->Text) {
        try {
            running = true;

            OutputLine("Establishing the bus, listeners and handlers...");

            // Set up the bus attachment, listener and proxy bus object for application
            busAtt = ref new BusAttachment("basicClient", true, 4);

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
                                                                                                                OutputLine("Joined session with the service (sessionId=" + joinResults->SessionId + ").");
                                                                                                                sessionId = joinResults->SessionId;
                                                                                                                return proxyBusObject->IntrospectRemoteObjectAsync(nullptr);
                                                                                                            } else {
                                                                                                                OutputLine("Joined session request returned with errors.");
                                                                                                            }
                                                                                                        }).then( [this] (concurrency::task<AllJoyn::IntrospectRemoteObjectResult ^> introspectTask)
                                                                                                                 {
                                                                                                                     IntrospectRemoteObjectResult ^ introResults = introspectTask.get();
                                                                                                                     if (QStatus::ER_OK == introResults->Status) {
                                                                                                                         OutputLine("Introspection of the service object was successful.");

                                                                                                                         Platform::String ^ input = this->Input->Text;
                                                                                                                         //	Set the 'name' property of the service's interface to user's input
                                                                                                                         Platform::Array<Object ^, 1>^ arr = { input };
                                                                                                                         MsgArg ^ arg = ref new MsgArg("s", arr);

                                                                                                                         this->OutputLine("Calling set property for 'name' with value '" + input + "'");
                                                                                                                         IAsyncOperation<AllJoyn::SetPropertyResult ^>^ setPropOp =
                                                                                                                             proxyBusObject->SetPropertyAsync(INTERFACE_NAME, "name", arg, nullptr, 2000);
                                                                                                                         concurrency::task<AllJoyn::SetPropertyResult ^> setPropTask(setPropOp);
                                                                                                                         setPropTask.then( [this] (concurrency::task<AllJoyn::SetPropertyResult ^> setPropTask)
                                                                                                                                           {
                                                                                                                                               this->OutputLine("Successfully called the set property for 'name'.");
                                                                                                                                               TearDown();
                                                                                                                                           });
                                                                                                                     } else {
                                                                                                                         this->OutputLine("Introspection of the service object was unsuccessful.");
                                                                                                                         busAtt->LeaveSession(sessionId);
                                                                                                                     }
                                                                                                                 });


                                                                                     });

            ConnectAllJoyn();
        } catch (Platform::Exception ^ ex) {
            OutputLine("Couldn't successfully establish the client app.");
            running = false;
            busAtt = nullptr;
        }
    }
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

//	Dissassembles the alljoyn object and terminates the client application
void MainPage::TearDown()
{
    //	Tear down the bus attachment and terminate the client
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

                                      OutputLine("Name Changed Client has terminated.\n");
                                  });
}
