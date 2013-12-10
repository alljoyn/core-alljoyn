//------------------------------------------------------------------------
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

namespace Signal_Consumer_Client
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using AllJoyn;
    using Signal_Consumer_Bus_Listener;
    using Signal_Consumer_Globals;
    using Windows.UI.Core;
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Controls;
    using Windows.UI.Xaml.Navigation;

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        /// <summary>
        /// busAtt provides a way to interface with the D-Bus and interact with other device in
        /// proximity
        /// </summary>
        private static BusAttachment busAtt = null;

        /// <summary>
        /// bus listener for signal consumer
        /// </summary>
        private static SignalConsumerBusListener busListener = null;

        /// <summary>
        /// called when the well-known service name has been discovered
        /// </summary>
        private AutoResetEvent foundNameEvent = new AutoResetEvent(false);

        /// <summary>
        /// Initializes a new instance of the MainPage class.
        /// </summary>
        public MainPage()
        {
            this.InitializeComponent();
            Application.Current.Suspending += new Windows.UI.Xaml.SuspendingEventHandler(App_Suspending);
            Application.Current.Resuming += new EventHandler<Object>(App_Resuming);
        }

        private void App_Suspending(Object sender, Windows.ApplicationModel.SuspendingEventArgs e)
        {
            try
            {
                if (busAtt != null)
                {
                    busAtt.OnAppSuspend();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Call OnAppSuspend() failed. Reason: " + AllJoynException.GetErrorMessage(ex.HResult));
            }
        }

        private void App_Resuming(Object sender, Object e)
        {
            try
            {
                if (busAtt != null)
                {
                    busAtt.OnAppResume();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Call OnAppResume() failed. Reason: " + AllJoynException.GetErrorMessage(ex.HResult));
            }
        }

        /// <summary>
        /// Outputs 'msg' to the text box in the UI of the application. We have to dispatch this to
        /// the UI thread.
        /// </summary>
        /// <param name="msg">Message that will be output in the text box of the UI.</param>
        public async void OutputLine(string msg)
        {
            ArgumentObject ao = new ArgumentObject();
            ao.TextBox = this.TextBlockSignalConsumer; // control in our Xaml page
            ao.Text = msg + "\n";
            await Dispatcher.RunAsync(
                CoreDispatcherPriority.Normal,
                new DispatchedHandler(ao.OnDispatched));
            System.Diagnostics.Debug.WriteLine(msg);
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            // The user interface page is now active so let the application know.
            App app = Application.Current as App;

            app.UiPage = this;

            // This logging is useful for debugging purposes but should not be
            // used for release versions. The output will be in the file "alljoyn.log" in
            // current users Documents directory.
            AllJoyn.Debug.UseOSLogging(true);
            AllJoyn.Debug.SetDebugLevel("ALLJOYN", 7);
            AllJoyn.Debug.SetDebugLevel("ALLJOYN_OBJ", 7);
            AllJoyn.Debug.SetDebugLevel("NS", 7);
            AllJoyn.Debug.SetDebugLevel("THREAD", 7);
            AllJoyn.Debug.SetDebugLevel("ALLJOYN_AUTH", 7);
            AllJoyn.Debug.SetDebugLevel("NETWORK", 7);
            AllJoyn.Debug.SetDebugLevel("STREAM", 7);
        }

        /// <summary>
        /// Connects to the bus and registers a signal handler for when the 'name' property changes.
        /// </summary>
        /// <param name="sender">UI control which signaled the click event.</param>
        /// <param name="e">arguments associated with the click event.</param>
        private void Button_RunClick(object sender, RoutedEventArgs e)
        {
            if (busAtt == null)
            {
                Task task = new Task(async () =>
                {
                    try
                    {
                        busAtt = new BusAttachment("SignalConsumerApp", true, 4);

                        // create and activate the interface
                        InterfaceDescription[] interfaceDescription = new InterfaceDescription[1];
                        busAtt.CreateInterface(SignalConsumerGlobals.InterfaceName, interfaceDescription, false);
                        interfaceDescription[0].AddSignal("nameChanged", "s", "newName", (byte)0, string.Empty);
                        interfaceDescription[0].AddProperty("name", "s", (byte)PropAccessType.PROP_ACCESS_RW);
                        interfaceDescription[0].Activate();

                        busListener = new SignalConsumerBusListener(busAtt, foundNameEvent);
                        OutputLine("BusAttachment and BusListener Created.");
                        busAtt.RegisterBusListener(busListener);
                        OutputLine("BusListener Registered.");

                        busAtt.Start();
                        busAtt.ConnectAsync(SignalConsumerGlobals.ConnectSpec).AsTask().Wait();
                        OutputLine("Bundled Daemon Registered.");
                        OutputLine("BusAttachment Connected to " + SignalConsumerGlobals.ConnectSpec + ".");

                        busAtt.FindAdvertisedName(SignalConsumerGlobals.WellKnownServiceName);
                        foundNameEvent.WaitOne();

                        /* Configure session properties and request a session with device with wellKnownName */
                        SessionOpts sessionOpts = new SessionOpts(
                            SignalConsumerGlobals.SessionProps.TrType,
                            SignalConsumerGlobals.SessionProps.IsMultiPoint,
                            SignalConsumerGlobals.SessionProps.PrType,
                            SignalConsumerGlobals.SessionProps.TmType);
                        SessionOpts[] sessionOptsOut = new SessionOpts[1];
                        OutputLine("Requesting a session with the well known service name.");
                        JoinSessionResult joinResult = await busAtt.JoinSessionAsync(
                            SignalConsumerGlobals.WellKnownServiceName, 
                            SignalConsumerGlobals.SessionProps.SessionPort, 
                            busListener, 
                            sessionOpts, 
                            sessionOptsOut, 
                            null);

                        if (QStatus.ER_OK == joinResult.Status)
                        {
                            OutputLine("Join Session was successful (sessionId=" + joinResult.SessionId + ").");
                            busAtt.AddMatch("type='signal',interface='org.alljoyn.Bus.signal_sample',member='nameChanged'");
                            OutputLine("Subscribed to the 'nameChanged' signal.");
                        }
                        else
                        {
                            OutputLine("Join Session was unsuccessful.");
                        }
                    }
                    catch (Exception ex)
                    {
                        OutputLine("Errors were produced while establishing the application.");
                        QStatus status = AllJoynException.GetErrorCode(ex.HResult);
                        busAtt = null;
                    }
                });
                task.Start();
            }
        }

        /// <summary>
        /// Stops the signal service through tearing down the bus
        /// </summary>
        /// <param name="sender">UI control which signaled the click event</param>
        /// <param name="e">arguments associated with the click event</param>
        private async void Button_StopClick(object sender, RoutedEventArgs e)
        {
            if (busAtt != null)
            {
                try
                {
                    await busAtt.DisconnectAsync(SignalConsumerGlobals.ConnectSpec);
                    await busAtt.StopAsync();

                    this.OutputLine("Signal Consumer has Exited.");
                }
                catch (Exception ex)
                {
                    string exceptionString = ex.ToString();
                    this.OutputLine("Exiting Basic Service Produced Errors!");
                    System.Diagnostics.Debug.WriteLine("Exception: " + exceptionString);
                }

                busAtt = null;
                busListener = null;
            }
        }

        /// <summary>
        /// Container for the argument past to the dispatcher which prints to UI.
        /// </summary>
        private class ArgumentObject
        {
            /// <summary>
            /// Gets or sets the string text.
            /// </summary>
            public string Text { get; set; }

            /// <summary>
            /// Gets or sets the text box which display the text.
            /// </summary>
            public TextBox TextBox { get; set; }

            /// <summary>
            /// Used by the dispacter when sending output to the UI's textbox.
            /// </summary>
            public void OnDispatched()
            {
                this.TextBox.Text += this.Text;
            }
        }
    }
}
