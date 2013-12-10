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

using namespace BasicService;

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
Platform::String ^ BasicService::INTERFACE_NAME = "org.alljoyn.Bus.sample";
Platform::String ^ SERVICE_NAME = "org.alljoyn.Bus.sample";
Platform::String ^ SERVICE_PATH = "/sample";
Platform::String ^ CONNECT_SPECS = "tcp:addr=127.0.0.1,port=9956";
unsigned short BasicService::SERVICE_PORT = 25;

bool running = false;

//	Main Page initialization
MainPage::MainPage()
{
    InitializeComponent();

    App ^ app = (App ^) Application::Current;

    app->SetUiPage(this);
}

//	Output text to the UI's textbox
void MainPage::OutputLine(Platform::String ^ msg) {

    ArgumentObject ^ ao = ref new ArgumentObject(msg + "\n", this->TextBlockService);

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
//	Bus Object which implements the interface over the bus for clients to interact with
BasicSampleObject ^ busObject = nullptr;
//	Bus Listener which handles events happening over the bus
MyBusListener ^ busListener = nullptr;

// Called when the start service button is clicked to start the basic service so clients can
// interact with the 'cat' method
void BasicService::MainPage::Button_RunService(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    if (!running && busAtt == nullptr) {
        try {
            running = true;

            OutputLine("Establishing the bus, listeners and handlers...");

            //	Create and register components of the basic service
            Platform::String ^ appName = "basicService";
            busAtt = ref new BusAttachment(appName, true, 4);

            busListener = ref new MyBusListener(busAtt);
            busAtt->RegisterBusListener(busListener->GetBusListener);

            //	Create the 'cat' interface
            Platform::Array<InterfaceDescription ^>^ intfArray = ref new Platform::Array<InterfaceDescription ^>(1);
            busAtt->CreateInterface(INTERFACE_NAME, intfArray, false);
            intfArray[0]->AddMethod("cat", "ss", "s", "inStr1,inStr2,outStr", (unsigned char)0, "");
            intfArray[0]->Activate();
            OutputLine("Created the 'cat' method interface.");

            busObject = ref new BasicSampleObject(busAtt, SERVICE_PATH);
            busAtt->RegisterBusObject(busObject->GetBusObject);

            busAtt->Start();

            ConnectAllJoyn();
        } catch (Platform::Exception ^ ex) {
            OutputLine("Could not successfully setup the alljoyn bus.");
            OutputLine("Error: " + ex->Message->ToString());
            running = false;
            busAtt = nullptr;
        }
    }
}

//	Connects to alljoyn by creating bundled daemon and connecting bus attachment
void MainPage::ConnectAllJoyn()
{
    OutputLine("Connecting to AllJoyn...");
    auto connectTask = create_task(busAtt->ConnectAsync(CONNECT_SPECS));
    connectTask.then( [this, connectTask] ()
                      {
                          try {
                              connectTask.get();

                              BuildService();
                          } catch (...) {
                              ConnectAllJoyn();
                          }
                      });
}

//	Executed after alljoyn connection has been established then sets up service by
//	1)	Requesting well-known name	2)	Binding Session Port	3)	Advertising well-known name
void MainPage::BuildService()
{
    OutputLine("Successfully Connected to the AllJoyn bus.");
    try {
        SessionOpts ^ sessionOpts = ref new SessionOpts(TrafficType::TRAFFIC_MESSAGES,
                                                        false,
                                                        ProximityType::PROXIMITY_ANY,
                                                        TransportMaskType::TRANSPORT_ANY);
        Platform::Array<unsigned short>^ portOut = ref new Platform::Array<unsigned short>(1);
        busAtt->BindSessionPort(SERVICE_PORT, portOut, sessionOpts, busListener->GetSessionPortListener);
        OutputLine("Bound session port (Port#=" + SERVICE_PORT + ").");

        unsigned int DBUS_NAME_DO_NOT_QUEUE = 4;
        busAtt->RequestName(SERVICE_NAME, DBUS_NAME_DO_NOT_QUEUE);
        OutputLine("Obtained well-known name '" + SERVICE_NAME + "'.");

        busAtt->AdvertiseName(SERVICE_NAME, TransportMaskType::TRANSPORT_ANY);
        OutputLine("Advertising the well-known name '" + SERVICE_NAME + "' for clients to discover.");

    } catch (Platform::Exception ^ ex) {
        OutputLine("Couldn't successfully establish the service with the alljoyn bus.");
        OutputLine("Error: " + ex->Message->ToString());
        TearDown();
    }
}


// Called when the stop service button is clicked to end the service process
void BasicService::MainPage::Button_StopService(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    if (running && nullptr != busAtt) {
        TearDown();
    }
}

// Tear down the service application by disconnecting the bus from alljoyn and stop all execution
void MainPage::TearDown()
{
    //	Tear down bus attachment and terminate service
    IAsyncAction ^ disconnectOp = busAtt->DisconnectAsync(CONNECT_SPECS);
    concurrency::task<void> disconnectTask(disconnectOp);
    disconnectTask.then( [this] ()
                         {
                             return busAtt->StopAsync();
                         }).then( [this] ()
                                  {
                                      busAtt = nullptr;
                                      busListener = nullptr;
                                      busObject = nullptr;
                                      running = false;
                                      OutputLine("The Basic Service Application has been terminated.\n");
                                  });
}
