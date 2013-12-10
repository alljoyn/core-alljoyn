//-----------------------------------------------------------------------
// <copyright file="MainPage.xaml.h" company="AllSeen Alliance.">
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

#include "MainPage.g.h"

namespace Chat {
// Constants for the chat application
extern Platform::String ^ INTERFACE_NAME;
extern Platform::String ^ NAME_PREFIX;
extern Platform::String ^ CHAT_SERVICE_OBJECT_PATH;
extern unsigned short CHAT_PORT;
extern Platform::String ^ CONNECT_SPECS;

extern unsigned int sessionId;                          //	Session ID of the currently joined service channel
extern unsigned int hostedSessionId;            //	Session ID of the currently hosted channel

//	Holds all of the available channels which have been discovered
extern Platform::Collections::Vector<Platform::String ^>^ channels;

//	Channel that this app is currently hosting, nullptr if no hosted channels
extern Platform::String ^ channelHosted;
//	Channel which the current application is joined with
extern Platform::String ^ channelJoined;

/// <summary>
/// An empty page that can be used on its own or navigated to within a Frame.
/// </summary>
[Windows::Foundation::Metadata::WebHostHiddenAttribute]
public ref class MainPage sealed {
  public:
    MainPage();
    void SetupAllJoyn(MainPage ^ page);
    void MainPage::UpdateStatus(Platform::String ^ msg);
    void MainPage::UpdateChannelControls(unsigned int sessId, bool successful);
    void MainPage::ConnectAllJoyn();
    void MainPage::UpdateChat(Platform::String ^ ts, Platform::String ^ sender, Platform::String ^ messageArg, unsigned int sessionId);
    void MainPage::DisconnectService(Platform::String ^ channelName, Platform::String ^ wellKnownName, MainPage ^ page);
    void MainPage::BuildService(Platform::String ^ channelName, Platform::String ^ wellKnownName, MainPage ^ page);
    void MainPage::UpdateChannels(Platform::String ^ wellKnownName, bool rm);
    void MainPage::JoinChannel(MainPage ^ page, int index);
    void MainPage::LeaveChannel(MainPage ^ page);
  protected:
    virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e) override;
  private:
    void StartChannelBtnClicked(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void JoinChannelBtnClicked(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void SendMessageBtnClicked(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void OnMessageBoxKeyDown(Platform::Object ^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs ^ e);
    void OnStartChannelKeyDown(Platform::Object ^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs ^ e);
};
}
