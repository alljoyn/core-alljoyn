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

using namespace SignalConsumerClient;

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
using namespace concurrency;
using namespace Windows::UI::Core;
using namespace AllJoynObjects;

/* constants for the signal consumer application */
Platform::String ^ SignalConsumerClient::INTERFACE_NAME = "org.alljoyn.Bus.signal_sample";
Platform::String ^ SERVICE_NAME = "org.alljoyn.Bus.signal_sample";
Platform::String ^ SERVICE_PATH = "/";
Platform::String ^ CONNECT_SPECS = "tcp:addr=127.0.0.1,port=9956";
unsigned short SERVICE_PORT = 25;

unsigned int sessionId = 0;
bool running = false;

// Creates an instance of the MainPage
MainPage::MainPage()
{
    InitializeComponent();

    App ^ app = (App ^) Application::Current;

    app->SetUiPage(this);
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
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

//	Output text to the UI's textbox
void MainPage::OutputLine(Platform::String ^ msg) {

    ArgumentObject ^ ao = ref new ArgumentObject(msg + "\n", this->TextBlockClient);

    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                             [this, ao] ()
                             {
                                 ao->OnDispactched();
                             }));
}

//	Primary bus attachmet which allows interactions over the D-Bus
BusAttachment ^ busAtt = nullptr;
//	Bus Listener which handles events happening over the bus
MyBusListener ^ busListener = nullptr;

//	Called when the run button is clicked, then finds and joins a session with the service
//	and registers to intercept the 'nameChanged' signal
void SignalConsumerClient::MainPage::Button_RunClient(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    if (!running && busAtt == nullptr) {
        try {
            running = true;

            OutputLine("Establishing the bus, listeners and handlers...");

            // Set up the bus attachment, listener and proxy bus object for application
            Platform::String ^ appName = "SignalConsumer";
            busAtt = ref new BusAttachment(appName, true, 4);

            // Implement the service interface
            Platform::Array<InterfaceDescription ^>^ interfaceDescription = ref new Platform::Array<InterfaceDescription ^>(1);
            busAtt->CreateInterface(INTERFACE_NAME, interfaceDescription, false);
            interfaceDescription[0]->AddSignal("nameChanged", "s", "newName", (unsigned char)0, "");
            interfaceDescription[0]->AddProperty("name", "s", (unsigned char)PropAccessType::PROP_ACCESS_RW);
            interfaceDescription[0]->Activate();

            // Create and register bus listener to handle bus events
            busListener = ref new MyBusListener(busAtt);
            busAtt->RegisterBusListener(busListener->GetBusListener);

            //	Connect to the bus and find well-known service name
            busAtt->Start();

            //	Called when well-known service name is discovered, then tries to establish a session
            BusListener ^ bl = busListener->GetBusListener;
            bl->FoundAdvertisedName += ref new BusListenerFoundAdvertisedNameHandler([this]
                                                                                         (Platform::String ^ name, TransportMaskType transportMask, Platform::String ^ namePrefix)
                                                                                     {
                                                                                         SessionOpts ^ sessionOpts = ref new SessionOpts(TrafficType::TRAFFIC_MESSAGES, false,
                                                                                                                                         ProximityType::PROXIMITY_ANY, TransportMaskType::TRANSPORT_ANY);
                                                                                         Platform::Array<SessionOpts ^, 1>^ optsOut = { ref new SessionOpts() };

                                                                                         IAsyncOperation<AllJoyn::JoinSessionResult ^>^ joinOp =
                                                                                             busAtt->JoinSessionAsync(SERVICE_NAME, SERVICE_PORT, busListener->GetSessionListener, sessionOpts, optsOut, nullptr);
                                                                                         concurrency::task<AllJoyn::JoinSessionResult ^> joinTask(joinOp);
                                                                                         joinTask.then( [this] (concurrency::task<AllJoyn::JoinSessionResult ^> joinTask)
                                                                                                        {
                                                                                                            AllJoyn::JoinSessionResult ^ results = joinTask.get();
                                                                                                            if (QStatus::ER_OK == results->Status) {
                                                                                                                OutputLine("Joined session with the service successfully (sessionId=" + results->SessionId + ").");
                                                                                                            } else {
                                                                                                                OutputLine("Join Session was unsuccessful.");
                                                                                                            }
                                                                                                        });
                                                                                     });

            ConnectAllJoyn();
        } catch (Platform::Exception ^ ex) {
            OutputLine("Could not successfully establish the client app.");
            OutputLine("Error: " + ex->Message->ToString());
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
                              // Add requirements for the signal intercepted by the bus attachment (subscribe to 'nameChanged' signal)
                              busAtt->AddMatch("type='signal',interface='org.alljoyn.Bus.signal_sample',member='nameChanged'");
                              busAtt->FindAdvertisedName(SERVICE_NAME);
                          } catch (...) {
                              ConnectAllJoyn();
                          }
                      });
}

//	Called when the stop button is clicked, then tears down the bus attachment and disconnects
//	from the bus
void SignalConsumerClient::MainPage::Button_StopClient(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    if (running && nullptr != busAtt) {
        //	Tear down the bus attachment
        IAsyncAction ^ disconnectOp = busAtt->DisconnectAsync(CONNECT_SPECS);
        concurrency::task<void> disconnectTask(disconnectOp);
        disconnectTask.then( [this] ()
                             {
                                 return busAtt->StopAsync();
                             }).then( [this] ()
                                      {
                                          busAtt = nullptr;
                                          busListener = nullptr;
                                          running = false;
                                          OutputLine("The signal consumer app has exited.");
                                      });
    }
}
