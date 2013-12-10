//-----------------------------------------------------------------------
// <copyright file="MainPage.xaml.cs" company="AllSeen Alliance.">
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

namespace Blank
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using AllJoyn;
    using Windows.Foundation;
    using Windows.Foundation.Collections;
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Controls;
    using Windows.UI.Xaml.Controls.Primitives;
    using Windows.UI.Xaml.Data;
    using Windows.UI.Xaml.Input;
    using Windows.UI.Xaml.Media;
    using Windows.UI.Xaml.Navigation;

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        /// <summary>
        /// Initializes a new instance of the MainPage class.
        /// </summary>
        public MainPage()
        {
            this.InitializeComponent();
            // Hook the application suspending/resuming events to the handlers so that exclusively-held resource (eg., network port) is release and other applications are allowed to acquire the resource
            Application.Current.Suspending += new Windows.UI.Xaml.SuspendingEventHandler(App_Suspending);
            Application.Current.Resuming += new EventHandler<Object>(App_Resuming);
        }

        private void App_Suspending(Object sender, Windows.ApplicationModel.SuspendingEventArgs e)
        {
            App app = Application.Current as App;
            try
            {
                if (app.Bus != null)
                {
                    app.Bus.OnAppSuspend();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Call OnAppSuspend() failed. Reason: " + AllJoynException.GetErrorMessage(ex.HResult));
            }
        }

        private void App_Resuming(Object sender, Object e)
        {
            App app = Application.Current as App;
            try
            {
                if (app.Bus != null)
                {
                    app.Bus.OnAppResume();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Call OnAppResume() failed. Reason: " + AllJoynException.GetErrorMessage(ex.HResult));
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the class has been initialized or not.
        /// </summary>
        private bool Initialized { get; set; }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            if (!this.Initialized)
            {
                App app = Application.Current as App;
                
                // Initialize your per-page application callbacks here
                app.Listeners.FoundAdvertisedName += this.Listeners_FoundAdvertisedName;

                // Start looking for advertised names
                app.Bus.FindAdvertisedName("org");

                this.Initialized = true;
            }
        }

        /// <summary>
        /// Handler for the FoundAdvertisedName event.
        /// </summary>
        /// <param name="name">A well known name that the remote bus is advertising.</param>
        /// <param name="transport">Transport that received the advertisement.</param>
        /// <param name="namePrefix">The well-known name prefix used in call to FindAdvertisedName
        /// that triggered this event.</param>
        private void Listeners_FoundAdvertisedName(string name, TransportMaskType transport, string namePrefix)
        {
            throw new NotImplementedException();
        }
    }
}
