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

namespace FileTransferClient
{
    using AllJoyn;
    using FileTransferClient.Common;
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Windows.ApplicationModel;
    using Windows.ApplicationModel.Activation;
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
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    public sealed partial class App : Application
    {
        /// <summary>
        /// The time to wait between retries in mS.
        /// </summary>
        private const int ConnectionRetryWaitTimeMilliseconds = 1000;

        /// <summary>
        /// Initializes a new instance of the App classs. This is the singleton application object.
        /// This is the first line of authored code executed, and as such is the logical equivalent
        /// of main() or WinMain().
        /// </summary>
        public App()
        {
            this.InitializeComponent();
            this.Suspending += this.OnSuspending;
        }

        /// <summary>
        /// Gets or sets the BusAttachment for the application.
        /// </summary>
        public BusAttachment Bus { get; set; }

        /// <summary>
        /// Gets or sets the collection of listeners for this application BusAttachment.
        /// </summary>
        public Listeners Listeners { get; set; }

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
        /// <param name="completed">A value to indicate the file transfer has been completed.</param>
        public static void OutputLine(string msg, bool completed = false)
        {
            App app = Application.Current as App;

            if (null != app && null != app.UiPage)
            {
                app.UiPage.OutputLine(msg, completed);
            }
        }

        /// <summary>
        /// Gets or sets the task that connects to the bus.
        /// </summary>
        private Task ConnectBus { get; set; }

        /// <summary>
        /// Gets or sets the interface returned when connecting to the AllJoyn bus asyncronously.
        /// </summary>
        private IAsyncAction ConnectOp { get; set; }

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

            this.InitializeAllJoyn();

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
            deferral.Complete();
        }

        /// <summary>
        /// Initializes the global bus attachment
        /// </summary>
        private void InitializeAllJoyn()
        {
            this.Bus = new BusAttachment("ClientApp", true, 4);
            this.Listeners = new Listeners(this.Bus);
            this.Bus.RegisterBusListener(this.Listeners);

            this.Bus.Start();

            this.ConnectBus = new Task(() =>
            {
                this.ConnectToBus();
            });

            this.ConnectBus.Start();
        }

        /// <summary>
        /// Do the asyncronous bus connection and set up the completion handler.
        /// </summary>
        private void ConnectToBus()
        {
            this.ConnectOp = this.Bus.ConnectAsync(ClientGlobals.ConnectSpecs);
            this.ConnectOp.Completed = new AsyncActionCompletedHandler(this.BusConnected);
        }

        /// <summary>
        /// Handles the initial connection state for connecting up the BusAttachment for service
        /// </summary>
        /// <param name="sender">The IAsyncAction interface which represents an asynchronous action
        /// that does not return a result and does not have progress notifications.</param>
        /// <param name="status">Specifies the status of an asynchronous operation.</param>
        private void BusConnected(IAsyncAction sender, AsyncStatus status)
        {
            try
            {
                sender.GetResults();

                string result = null;

                switch (status)
                {
                case AsyncStatus.Canceled:
                    result = "canceled";
                    break;
                case AsyncStatus.Completed:
                    result = "completed";
                    break;
                case AsyncStatus.Error:
                    result = "error";
                    break;
                case AsyncStatus.Started:
                    result = "started";
                    break;
                default:
                    result = "unexpected status";
                    break;
                }

                if (null != result)
                {
                    string message = string.Format(
                                                   "BusConnectAsync({0}) result = '{1}'.",
                                                   ClientGlobals.ConnectSpecs,
                                                   result);
                    App.OutputLine(message);

                    if (AsyncStatus.Completed == status)
                    {
                        this.Bus.FindAdvertisedName(ClientGlobals.ServiceName);
                        message = string.Format("Called FindAdvertiseName({0}).", ClientGlobals.ServiceName);
                        App.OutputLine(message);
                    }
                }
            }
            catch
            {
                this.ConnectBus = new Task(() =>
                {
                    ManualResetEvent evt = new ManualResetEvent(false);

                    evt.WaitOne(App.ConnectionRetryWaitTimeMilliseconds);
                    this.ConnectToBus();
                });

                this.ConnectBus.Start();
            }
        }

        /// <summary>
        /// Handler for the FoundAdvertisedName event.
        /// </summary>
        /// <param name="name">A well known name that the remote bus is advertising.</param>
        /// <param name="transport">Transport that received the advertisement.</param>
        /// <param name="namePrefix">The well-known name prefix used in call to FindAdvertisedName
        /// that triggered this event.</param>
        public void Listeners_FoundAdvertisedName(string name, TransportMaskType transport, string namePrefix)
        {
            App.OutputLine("Found advertised name in App handler.");
        }

        /// <summary>
        /// Handler for the LostAdvertisedName event.
        /// </summary>
        /// <param name="name">A well known name that the remote bus is advertising.</param>
        /// <param name="transport">Transport that received the advertisement.</param>
        /// <param name="namePrefix">The well-known name prefix used in call to FindAdvertisedName
        /// that triggered this event.</param>
        public void Listeners_LostAdvertisedName(string name, TransportMaskType transport, string namePrefix)
        {
            App.OutputLine("Lost advertised name in App handler.");
        }    
    }
}
