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

namespace Blank
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using AllJoyn;
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
        /// The transport connection spec string.
        /// </summary>
        private const string ConnectSpec = "tcp:addr=127.0.0.1,port=9955";

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

            // Create the BusAttachment
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
            this.Bus = new BusAttachment("App", true, 4);
            Listeners = new Listeners(this.Bus);
            this.Bus.Start();

            this.ConnectBus = new Task(() =>
            {
                this.ConnectOp = this.Bus.ConnectAsync(ConnectSpec);
                this.ConnectOp.Completed = new AsyncActionCompletedHandler(this.BusConnected);
            });

            this.ConnectBus.Start();
        }

        /// <summary>
        /// Handles the initial connection state for connecting up the BusAttachment for service
        /// </summary>
        /// <param name="sender">The IAsyncAction interface which represents an asynchronous action
        /// that does not return a result and does not have progress notifications.</param>
        /// <param name="status">The parameter is not used.</param>
        private void BusConnected(IAsyncAction sender, AsyncStatus status)
        {
            try
            {
                sender.GetResults();
            }
            catch
            {
                this.ConnectBus = new Task(() =>
                {
                    ManualResetEvent evt = new ManualResetEvent(false);

                    evt.WaitOne(ConnectionRetryWaitTimeMilliseconds);
                    this.ConnectOp = this.Bus.ConnectAsync(ConnectSpec);
                    this.ConnectOp.Completed = new AsyncActionCompletedHandler(this.BusConnected);
                });

                this.ConnectBus.Start();
            }
        }
    }
}
