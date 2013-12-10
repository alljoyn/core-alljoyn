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

using namespace SignalService;
using namespace std;

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

//	Constants for the Signal Service Application
Platform::String ^ SignalService::INTERFACE_NAME = "org.alljoyn.Bus.signal_sample";
Platform::String ^ SERVICE_NAME = "org.alljoyn.Bus.signal_sample";
Platform::String ^ SERVICE_PATH = "/";
Platform::String ^ CONNECT_SPECS = "tcp:addr=127.0.0.1,port=9956";
unsigned short SignalService::SERVICE_PORT = 25;

unsigned int SignalService::sessionId = 0;
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

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs ^ e)
{
    (void) e;           // Unused parameter
}

//	Primary bus attachment which communicates with other objects over the bus
BusAttachment ^ busAtt = nullptr;
//	Bus object which implements the interface for clients to interact with
BasicSampleObject ^ busObject = nullptr;
//	Bus Listener which handles all bus and session events.
MyBusListener ^ busListener = nullptr;

//	Called when the run service button is clicked, then establishes service advertising
//	well-known service name which implements an interface for clients to interact with.
void SignalService::MainPage::Button_RunService(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    if (!running && nullptr == busAtt) {
        try {
            running = true;

            OutputLine("Establishing the bus, listeners and handlers...");

            //	Setup the bus attachments, bus listeners and bus objects for the service
            Platform::String ^ appName = "signalService";
            busAtt = ref new BusAttachment(appName, true, 4);

            busListener = ref new MyBusListener(busAtt);
            busAtt->RegisterBusListener(busListener->GetBusListener);

            //	Implement the service interface
            InterfaceDescription ^ interfaceDescription = nullptr;
            Platform::Array<InterfaceDescription ^, 1>^ intfArray = { interfaceDescription };
            busAtt->CreateInterface(INTERFACE_NAME, intfArray, false);
            intfArray[0]->AddSignal("nameChanged", "s", "newName", (unsigned char)0, "");
            intfArray[0]->AddProperty("name", "s", (unsigned char)PropAccessType::PROP_ACCESS_RW);
            intfArray[0]->Activate();
            OutputLine("Created Signal Service Interface.");

            busObject = ref new BasicSampleObject(busAtt, SERVICE_PATH);
            busAtt->RegisterBusObject(busObject->GetBusObject);

            busAtt->Start();

            ConnectAllJoyn();
        } catch (Platform::Exception ^ ex) {
            OutputLine("Couldn't successfully setup the alljoyn bus.");
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

        busAtt->RequestName(SERVICE_NAME, (unsigned int)RequestNameType::DBUS_NAME_DO_NOT_QUEUE);
        OutputLine("Obtained the well-known name '" + SERVICE_NAME + "'.");

        busAtt->AdvertiseName(SERVICE_NAME, TransportMaskType::TRANSPORT_ANY);
        OutputLine("Advertising the well-known name '" + SERVICE_NAME + "' for clients to discover.");

    } catch (Platform::Exception ^ ex) {
        OutputLine("Establishing the service with the AllJoyn bus was unsuccessful.");
        OutputLine("Error: " + ex->Message->ToString());
        TearDown();
    }
}

//	Called when stop service is clicked, then tears down bus attachment and terminates service.
void SignalService::MainPage::Button_StopService(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    if (running && nullptr != busAtt) {
        TearDown();
    }
}

// Tear down the service by disconnecting the bus from alljoyn and stop all execution
void MainPage::TearDown()
{
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

                                      OutputLine("Bus attachment disconnected and signal service terminated.\n");
                                  });
}
