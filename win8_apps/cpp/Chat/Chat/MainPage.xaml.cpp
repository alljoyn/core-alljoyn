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
#include <ppltasks.h>
#include <time.h>
#include <Windows.h>

#include "AllJoynObjects.h"

using namespace Chat;

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

// Constants for the chat application
Platform::String ^ Chat::INTERFACE_NAME = "org.alljoyn.bus.samples.chat";
Platform::String ^ Chat::NAME_PREFIX = "org.alljoyn.bus.samples.chat.";
Platform::String ^ Chat::CHAT_SERVICE_OBJECT_PATH = "/chatService";
unsigned short Chat::CHAT_PORT = 27;

Platform::String ^ Chat::CONNECT_SPECS = "tcp:addr=127.0.0.1,port=9956";

unsigned int Chat::sessionId = 0;               //	Session ID of the currently joined service channel
unsigned int Chat::hostedSessionId = 0; //	Session ID of the currently hosted channel

//	Channel that this app is currently hosting, nullptr if no hosted channels
Platform::String ^ Chat::channelHosted = nullptr;
//	Channel which the current application is joined with
Platform::String ^ Chat::channelJoined = nullptr;

//	Holds all of the available channels which have been discovered
Platform::Collections::Vector<Platform::String ^>^ Chat::channels = ref new Platform::Collections::Vector<Platform::String ^>();

//	Primary entrance of program execution
MainPage::MainPage()
{
    InitializeComponent();

    App ^ app = (App ^) Application::Current;
    app->SetUiPage(this);

    ChannelsComboBox->DataContext = channels;

    SetupAllJoyn(this);
}

/// Invoked when this page is about to be displayed in a Frame.
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs ^ e)
{
    (void) e;           // Unused parameter
}

//	Primary bus attachmet which allows interactions over the D-Bus
BusAttachment ^ busAtt = nullptr;
//	Primary object which handles and interacts with the signals sent over chat
ChatObject ^ chatObject = nullptr;
//	Bus Listener which handles events happening over the bus
MyBusListener ^ busListener = nullptr;

//	Set up the standard alljoyn object to be used in the application
void MainPage::SetupAllJoyn(MainPage ^ page)
{
    UpdateStatus("Establishing the bus and registering handlers....");

    // Set up the bus attachment, listener and proxy bus object for application
    busAtt = ref new BusAttachment("Chat", true, 4);

    busListener = ref new MyBusListener(busAtt);
    busAtt->RegisterBusListener(busListener->GetBusListener);

    chatObject = ref new ChatObject(busAtt, CHAT_SERVICE_OBJECT_PATH);
    busAtt->RegisterBusObject(chatObject->GetBusObject);

    //	Connect to the bus and find well-known service name
    busAtt->Start();

    ConnectAllJoyn();
}

//	Connects to alljoyn by creating bundled daemon and connecting bus attachment. Looks for
//	well-known name after completion
void MainPage::ConnectAllJoyn()
{
    UpdateStatus("Connecting to AllJoyn...");
    auto connectTask = create_task(busAtt->ConnectAsync(CONNECT_SPECS));
    connectTask.then( [this, connectTask] ()
                      {
                          try {
                              connectTask.get();
                              UpdateStatus("Connected to AllJoyn successfully.");

                              // Add requirements for the signal intercepted by the bus attachment
                              busAtt->AddMatch("type='signal',interface='org.alljoyn.bus.samples.chat',member='Chat'");
                              busAtt->FindAdvertisedName(NAME_PREFIX);
                          } catch (...) {
                              ConnectAllJoyn();
                          }
                      });
}

//	If enter key is pressed with focus on start/stop channel text box the service is created
//	with specified channel name or the current channel is torn down
void Chat::MainPage::OnStartChannelKeyDown(Platform::Object ^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs ^ e)
{
    if (e->Key == Windows::System::VirtualKey::Enter) {
        Chat::MainPage::StartChannelBtnClicked(sender, e);
    }
}
//	Called when start/stop channel button is clicked then either starts a service as a channel
//	with the given user input as part of the well-known name or tears down the existing service
//	depending on state of the app
void Chat::MainPage::StartChannelBtnClicked(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    Platform::String ^ channelName = MainPage::EnterChannelTextView->Text;
    Platform::String ^ wellKnownName = NAME_PREFIX + channelName;

    if ("" != channelName && "Start Channel" == MainPage::StartChannelButton->Content->ToString()) {
        BuildService(channelName, wellKnownName, this);
    } else if (nullptr != channelHosted && "Stop Channel" == MainPage::StartChannelButton->Content->ToString()) {
        DisconnectService(channelName, wellKnownName, this);
    }
}

//	Called when join channel button is clicked then joins the selected channel if not
//	currently connected to a channel, otherwise leaves the channel currently connected to
void Chat::MainPage::JoinChannelBtnClicked(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    int index = ChannelsComboBox->SelectedIndex;

    if (0 <= index && (size_t)index < channels->Size && "Join Channel" == JoinChannelButton->Content->ToString()) {
        JoinChannel(this, index);

    } else if (nullptr != channelJoined && "Leave Channel" == JoinChannelButton->Content->ToString()) {
        LeaveChannel(this);
    }
}

//	Called when enter key is press with focus in the message text box then sends message to
//	all user part of currently joined session
void Chat::MainPage::OnMessageBoxKeyDown(Platform::Object ^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs ^ e)
{
    if (e->Key == Windows::System::VirtualKey::Enter) {
        Chat::MainPage::SendMessageBtnClicked(sender, e);
    }
}
//	Called when send message button is clicked then sends message to all user part of currently
//	joined session
void Chat::MainPage::SendMessageBtnClicked(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    Platform::String ^ msg = MainPage::MessageBox->Text;

    if ((nullptr != channelJoined || nullptr != channelHosted) && "" != msg) {
        chatObject->SendChatMessage(msg);
        MainPage::MessageBox->Text = "";
    }
}

// Join the user selected channel and set timeout for the session for the case where the
// service leaves proximity.
void MainPage::JoinChannel(MainPage ^ page, int index)
{
    Platform::String ^ subString;
    try {
        subString = channels->GetAt(index);
        Platform::String ^ wellKnownName = NAME_PREFIX + subString;
        page->UpdateStatus("Joining channel named '" + subString + "'.");

        page->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                       [page] ()
                                       {
                                           page->ChannelsComboBox->IsEnabled = false;
                                           page->JoinChannelButton->IsEnabled = false;
                                       }));

        TrafficType traffic = TrafficType::TRAFFIC_MESSAGES;
        ProximityType proximity = ProximityType::PROXIMITY_ANY;
        TransportMaskType transport = TransportMaskType::TRANSPORT_ANY;
        SessionOpts ^ sessionOpts = ref new SessionOpts(traffic, true, proximity, transport);
        Platform::Array<SessionOpts ^, 1>^ optsOut = { ref new SessionOpts() };

        IAsyncOperation<AllJoyn::JoinSessionResult ^>^ joinOp =
            busAtt->JoinSessionAsync(wellKnownName, CHAT_PORT, busListener->GetSessionListener, sessionOpts, optsOut, nullptr);
        concurrency::task<AllJoyn::JoinSessionResult ^> joinTask(joinOp);
        joinTask.then( [page, subString] (concurrency::task<AllJoyn::JoinSessionResult ^> joinTask)
                       {
                           AllJoyn::JoinSessionResult ^ results = joinTask.get();
                           QStatus status = results->Status;
                           unsigned int sessId = results->SessionId;
                           if (QStatus::ER_OK == status) {
                               channelJoined = subString;
                               page->UpdateChannelControls(sessId, true);
                               Platform::Array<unsigned int>^ timeOut = ref new Platform::Array<unsigned int>(1);
                               IAsyncOperation<AllJoyn::SetLinkTimeoutResult ^>^ timeOutOp =
                                   busAtt->SetLinkTimeoutAsync(sessId, 40, timeOut);
                               concurrency::task<AllJoyn::SetLinkTimeoutResult ^> timeOutTask(timeOutOp);
                               timeOutTask.then( [page] (concurrency::task<AllJoyn::SetLinkTimeoutResult ^> timeOutTask)
                                                 {
                                                     AllJoyn::SetLinkTimeoutResult ^ timeOutResults = timeOutTask.get();
                                                     if (QStatus::ER_OK != timeOutResults->Status) {
                                                         page->UpdateStatus("A problem occurred when setting the link timeout for the session.");
                                                     }
                                                 });
                           } else if (QStatus::ER_ALLJOYN_JOINSESSION_REPLY_UNREACHABLE == status) {
                               page->UpdateStatus("The channel you've selected is no longer available.");
                               page->UpdateChannels(subString, true);
                           } else {
                               page->UpdateStatus("A problem occurred when trying to join the channel.");
                               page->UpdateChannelControls(sessId, false);
                           }
                       });

    } catch (...) {
        UpdateStatus("A problem occurred when trying to join a session with selected channel.");
        Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                 [this] ()
                                 {
                                     ChannelsComboBox->IsEnabled = true;
                                     JoinChannelButton->IsEnabled = true;
                                 }));
    }
}

// Leave the currently joined channel
void MainPage::LeaveChannel(MainPage ^ page)
{
    page->UpdateStatus("Attempting to leave channel named '" + channelJoined + "'...");

    page->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                   [page] ()
                                   {
                                       page->JoinChannelButton->IsEnabled = false;
                                   }));

    try {
        busAtt->LeaveSession(sessionId);
        page->UpdateChannelControls(sessionId, true);
    } catch (...) {
        page->UpdateStatus("A problem occurred when trying to leave the session (sessionId=" + sessionId + ")");
        page->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                       [page] ()
                                       {
                                           page->JoinChannelButton->IsEnabled = true;
                                       }));
    }
}

//	Build a service with the user specified channel name by:
//	1)	Requesting well-known name	2)	Binding Session Port	3)	Advertising well-known name
void MainPage::BuildService(Platform::String ^ channelName, Platform::String ^ wellKnownName, MainPage ^ page)
{
    UpdateStatus("Establishing a service channel with name '" + channelName + "'...");

    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                             [page] ()
                             {
                                 page->EnterChannelTextView->IsReadOnly = true;
                                 page->StartChannelButton->IsEnabled = false;
                             }));

    try {
        SessionOpts ^ sessionOpts = ref new SessionOpts(TrafficType::TRAFFIC_MESSAGES,
                                                        true, ProximityType::PROXIMITY_ANY, TransportMaskType::TRANSPORT_ANY);
        Platform::Array<unsigned short>^ portOut = ref new Platform::Array<unsigned short>(1);

        busAtt->BindSessionPort(CHAT_PORT, portOut, sessionOpts, busListener->GetSessionPortListener);

        unsigned int DBUS_NAME_DO_NOT_QUEUE = 4;
        busAtt->RequestName(wellKnownName, DBUS_NAME_DO_NOT_QUEUE);

        busAtt->AdvertiseName(wellKnownName, TransportMaskType::TRANSPORT_ANY);

        channelHosted = channelName;
        Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                 [page] ()
                                 {
                                     page->StartChannelButton->Content = "Stop Channel";
                                     page->StartChannelButton->IsEnabled = true;
                                 }));
        UpdateStatus("Now hosting channel named '" + channelName + "'.");

    } catch (...) {
        UpdateStatus("A problem occurred while trying to advertise the well-known name.");
        Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                 [page] ()
                                 {
                                     page->EnterChannelTextView->IsReadOnly = false;
                                     page->StartChannelButton->IsEnabled = true;
                                 }));
    }
}

//	Tear down the existing channel service by:
//	1) Cancel Advertising WKN	2) Unbind Session Port	3) Releases the WKN
void MainPage::DisconnectService(Platform::String ^ channelName, Platform::String ^ wellKnownName, MainPage ^ page)
{
    //	Tear down the service
    MainPage::StartChannelButton->IsEnabled = false;

    if (0 != hostedSessionId) {
        try {
            busAtt->LeaveSession(hostedSessionId);
        } catch (...) {
            UpdateStatus("A Problem occurred when leaving session (sessionId=" + hostedSessionId + ")");
        }
    }
    try {
        busAtt->UnbindSessionPort(CHAT_PORT);
        busAtt->CancelAdvertiseName(wellKnownName, TransportMaskType::TRANSPORT_ANY);
        busAtt->ReleaseName(wellKnownName);

        hostedSessionId = 0;
        channelHosted = nullptr;

        UpdateStatus("Discarded service channel named '" + channelName + "'.");

    } catch (...) {
        UpdateStatus("A Problem occurred when tearing down the service.");
    }

    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                             [page] ()
                             {
                                 page->StartChannelButton->Content = "Start Channel";
                                 page->EnterChannelTextView->IsReadOnly = false;
                                 page->StartChannelButton->IsEnabled = true;
                             }));
}

//	Updates the current status of the application to the status text block of the UI
void MainPage::UpdateStatus(Platform::String ^ msg) {
    Platform::String ^ debug = msg + "\n";
    OutputDebugStringW(debug->Data());
    ArgumentObject ^ ao = ref new ArgumentObject(msg, this->AJStatus);

    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                             [this, ao] ()
                             {
                                 ao->OnDispactched();
                             }));
}

//	Updates the chat log on the screen for current conversations
void MainPage::UpdateChat(Platform::String ^ ts, Platform::String ^ sender, Platform::String ^ messageArg, unsigned int sessionId) {
    Platform::String ^ me = "Me";
    Platform::String ^ msg;
    if (me == sender) {
        msg = ts + "  From: " + sender + "\t\t" + messageArg + "\n";
    } else {
        msg = ts + "  From " + sender + "\t" + messageArg + "\n";
    }

    ChatArg ^ ao = ref new ChatArg(msg, this->ChatLogView);

    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                             [this, ao] ()
                             {
                                 ao->OnDispactched();
                             }));
}

//	Updates the list of available channels (well-known names) as they're discovered and lost
void MainPage::UpdateChannels(Platform::String ^ wellKnownName, bool remove)
{
    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                             [this, wellKnownName, remove] ()
                             {
                                 unsigned int index = -1;
                                 unsigned int existingIndex;
                                 bool found;
                                 found  = channels->IndexOf(wellKnownName, &existingIndex);
                                 if (!remove && !found) {
                                     unsigned int i;
                                     for (i = 0; i < channels->Size; i++) {
                                         if (-1 == Platform::String::CompareOrdinal(wellKnownName, channels->GetAt(i))) {
                                             index = i;
                                             break;
                                         }
                                     }
                                     if (-1 == index) {
                                         channels->Append(wellKnownName);
                                     } else {
                                         channels->SetAt(index, wellKnownName);
                                     }
                                 } else if (remove && found) {
                                     channels->RemoveAt(existingIndex);
                                 }
                             }));
}

//	Updates the controls of the UI depending on whether the user is currently connected to a channel
void MainPage::UpdateChannelControls(unsigned int sessId, bool successful)
{
    if (successful) {
        Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                 [this, sessId] ()
                                 {
                                     if ("Join Channel" == this->JoinChannelButton->Content->ToString() && 0 == sessionId) {
                                         sessionId = sessId;

                                         this->JoinChannelButton->Content = "Leave Channel";
                                         this->JoinChannelButton->IsEnabled = true;
                                         this->AJStatus->Text = "Successfully joined session with '" + channelJoined +
                                                                "' (sessionId=" + sessId + ")";
                                     } else if ("Leave Channel" == this->JoinChannelButton->Content->ToString() && sessId == sessionId) {
                                         this->AJStatus->Text = "Disconnected from channel named '" + channelJoined + "'.";

                                         channelJoined = nullptr;
                                         sessionId = 0;

                                         this->JoinChannelButton->Content = "Join Channel";
                                         this->JoinChannelButton->IsEnabled = true;
                                         this->ChannelsComboBox->IsEnabled = true;
                                     }
                                 }));
    } else {
        Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(
                                 [this, sessId] ()
                                 {
                                     this->JoinChannelButton->IsEnabled = true;
                                     this->ChannelsComboBox->IsEnabled = true;
                                 }));
    }
}
