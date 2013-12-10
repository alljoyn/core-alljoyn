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
using namespace AllJoyn;

namespace Secure {
/* constants for basic service application */
extern Platform::String ^ INTERFACE_NAME;
extern Platform::String ^ SERVICE_NAME;
extern Platform::String ^ SERVICE_PATH;
extern Platform::String ^ CONNECT_SPECS;
extern unsigned short SERVICE_PORT;

// Context required to respond to a RequestCredentials request
extern AuthContext ^ context;

/// <summary>
/// An empty page that can be used on its own or navigated to within a Frame.
/// </summary>
[Windows::Foundation::Metadata::WebHostHiddenAttribute]
public ref class MainPage sealed {
  public:
    MainPage();
    void MainPage::OutputLine(Platform::String ^ msg);
    void MainPage::OutputPin(Platform::String ^ msg);
    void MainPage::ConnectServerToAllJoyn();
    void MainPage::BuildService();
    void MainPage::ServerTearDown();
    void MainPage::ClientTearDown();
    void MainPage::CallPingMethod();
    void MainPage::ConnectClientToAllJoyn();
  protected:
    virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e) override;
  private:
    void StartServerClick(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void StartClientClick(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void SendPinClick(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
};
}
