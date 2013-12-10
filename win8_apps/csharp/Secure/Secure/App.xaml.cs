//-----------------------------------------------------------------------
// <copyright file="App.xaml.cs" company="AllSeen Alliance.">
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

namespace Secure
{
    using System;
    using Windows.ApplicationModel;
    using Windows.ApplicationModel.Activation;
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Controls;
    using AllJoyn;

    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    public sealed partial class App : Application
    {
        /// <summary>
        /// Describes the type of security being used.
        /// </summary>
        public const string SecurityType = "ALLJOYN_SRP_KEYX";

        /// <summary>
        /// The transport connection spec string.
        /// </summary>
        public const string ConnectSpec = "tcp:addr=127.0.0.1,port=9956";

        /// <summary>
        /// The name of the interface.
        /// </summary>
        public const string InterfaceName = "org.alljoyn.bus.samples.secure.SecureInterface";

        /// <summary>
        /// The name of the service.
        /// </summary>
        public const string ServiceName = "org.alljoyn.bus.samples.secure";

        /// <summary>
        /// The path for the service.
        /// </summary>
        public const string ServicePath = "/SecureService";

        /// <summary>
        /// The port used for the service.
        /// </summary>
        public const ushort ServicePort = 42;

        /// <summary>
        /// The time to wait between retries in mS.
        /// </summary>
        public const int ConnectionRetryWaitTimeMilliseconds = 1000;

        public BusAttachment Bus { get; set; }

        /// <summary>
        /// Initializes a new instance of the App classs. This is the singleton application object.
        /// This is the first line of authored code executed, and as such is the logical equivalent
        /// of main() or WinMain().
        /// </summary>
        public App()
        {
            this.InitializeComponent();
            this.Suspending += this.OnSuspending;
            this.Resuming += new EventHandler<Object>(OnResuming);
        }

        /// <summary>
        /// Gets or sets the User Interface page. This is null until the page has been created.
        /// It is most useful for sending output to the user via the method OutputLine(string msg).
        /// </summary>
        public MainPage UiPage { get; set; }

        /// <summary>
        /// Outputs 'msg' to the text box in the UI of the application. If the Main page of the application
        /// is not available the message is lost.
        /// </summary>
        /// <param name="msg">Message that will be output in the text box of the UI.</param>
        public static void OutputLine(string msg)
        {
            App app = Application.Current as App;

            if (null != app && null != app.UiPage)
            {
                app.UiPage.OutputLine(msg);
            }
        }

        /// <summary>
        /// Outputs 'pin' to the "PIN to use" text box in the UI of the application. If the Main
        /// page of the application is not available the PIN is lost.
        /// </summary>
        /// <param name="pin">PIN that will be output in the "PIN to use" of the UI.</param>
        public static void OutputPIN(string pin)
        {
            App app = Application.Current as App;

            if (null != app && null != app.UiPage)
            {
                app.UiPage.OutputPIN(pin);
            }
        }

        /// <summary>
        /// Invoked when the application is launched normally by the end user.  Other entry points
        /// will be used when the application is launched to open a specific file, to display
        /// search results, and so forth.
        /// </summary>
        /// <param name="args">Details about the launch request and process.</param>
        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            // Do not repeat app initialization when already running, just ensure that
            // the window is active
            if (args.PreviousExecutionState == ApplicationExecutionState.Running)
            {
                Window.Current.Activate();
                return;
            }

            if (args.PreviousExecutionState == ApplicationExecutionState.Terminated)
            {
                // TODO: Load state from previously suspended application
            }

            // Create a Frame to act navigation context and navigate to the first page
            var rootFrame = new Frame();

            if (!rootFrame.Navigate(typeof(MainPage)))
            {
                throw new Exception("Failed to create initial page");
            }

            // Place the frame in the current Window and ensure that it is active
            Window.Current.Content = rootFrame;
            Window.Current.Activate();
        }

        /// <summary>
        /// Invoked when application execution is being suspended.  Application state is saved
        /// without knowing whether the application will be terminated or resumed with the contents
        /// of memory still intact.
        /// </summary>
        /// <param name="sender">The source of the suspend request.</param>
        /// <param name="e">Details about the suspend request.</param>
        private void OnSuspending(object sender, SuspendingEventArgs e)
        {
            var deferral = e.SuspendingOperation.GetDeferral();

            // TODO: Save application state and stop any background activity
            try
            {
                if (Bus != null)
                {
                    Bus.OnAppSuspend();
                }
            } catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Call OnAppSuspend() failed. Reason: " + AllJoynException.GetErrorMessage(ex.HResult));
            }

            deferral.Complete();
        }

        private void OnResuming(Object sender, Object e)
        {
            try
            {
                if (Bus != null)
                {
                    Bus.OnAppResume();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Call OnAppResume() failed. Reason: " + AllJoynException.GetErrorMessage(ex.HResult));
            }
        }
    }
}
