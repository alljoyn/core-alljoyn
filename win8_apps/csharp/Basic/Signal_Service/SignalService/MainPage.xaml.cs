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

namespace Signal_Service
{
    using System;
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using AllJoyn;
    using Basic_Service_Bus_Object;
    using Signal_Service_Bus_Listener;
    using Signal_Service_Globals;
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
        /// busObject provides the interface which the client applications will remotely access.
        /// </summary>
        private static SignalServiceBusObject busObject = null;

        /// <summary>
        /// busAtt provides a way to interface with the D-Bus and interact with other device in
        /// proximity.
        /// </summary>
        private static BusAttachment busAtt = null;

        /// <summary>
        /// busListener provides the event handlers for common events during connection creation
        /// and joining sessions.
        /// </summary>
        private static SignalServiceBusListener busListener = null;

        /// <summary>
        /// name corresponds to the name property inplemented in the interface.
        /// </summary>
        private static string name = string.Empty;

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
            ao.TextBox = this.TextBlockService; // This is the control in our Xaml page.
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
        /// connects with the bus, creates an interface and advertises a well-known name for
        /// clients to join a session with.
        /// </summary>
        /// <param name="sender">UI control which signaled the click event.</param>
        /// <param name="e">arguments associated with the click event.</param>
        private void Button_RunClick(object sender, RoutedEventArgs e)
        {
            if (busObject == null && busAtt == null)
            {
                Task task = new Task(async () =>
                {
                    try
                    {
                        busAtt = new BusAttachment("SignalServiceApp", true, 4);

                        busObject = new SignalServiceBusObject(busAtt);
                        OutputLine("BusObject Created.");

                        busListener = new SignalServiceBusListener(busAtt);
                        OutputLine("BusAttachment and BusListener Created.");
                        busAtt.RegisterBusListener(busListener);
                        OutputLine("BusListener Registered.");

                        busAtt.Start();
                        await busAtt.ConnectAsync(SignalServiceGlobals.ConnectSpec);
                        OutputLine("Bundled Daemon Registered.");
                        OutputLine("BusAttachment Connected to " + SignalServiceGlobals.ConnectSpec + ".");

                        SessionOpts sessionOpts = new SessionOpts(
                            SignalServiceGlobals.SessionProps.TrType,
                            SignalServiceGlobals.SessionProps.IsMultiPoint,
                            SignalServiceGlobals.SessionProps.PrType,
                            SignalServiceGlobals.SessionProps.TmType);
                        try
                        {
                            ushort[] portOut = new ushort[1];
                            busAtt.BindSessionPort(SignalServiceGlobals.SessionProps.SessionPort, portOut, sessionOpts, busListener);

                            busAtt.RequestName(SignalServiceGlobals.WellKnownServiceName, (int)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);

                            busAtt.AdvertiseName(SignalServiceGlobals.WellKnownServiceName, TransportMaskType.TRANSPORT_ANY);
                            OutputLine("Name is Being Advertised as: " + SignalServiceGlobals.WellKnownServiceName);
                        }
                        catch (COMException ce)
                        {
                            QStatus s = AllJoynException.GetErrorCode(ce.HResult);
                            OutputLine("Errors were produced while establishing the service.");
                            TearDown();
                        }
                    }
                    catch (Exception ex)
                    {
                        OutputLine("Errors occurred while setting up the service.");
                        QStatus status = AllJoynException.GetErrorCode(ex.HResult);
                        busObject = null;
                        busAtt = null;
                    }
                });
                task.Start();
            }
        }

        /// <summary>
        /// Stops execution of the service by disconnecting the busAttachment and diposing of
        /// the busAttachment and busObject.
        /// </summary>
        /// <param name="sender">UI control which signaled the click event.</param>
        /// <param name="e">arguments associated with the click event.</param>
        private void Button_StopClick(object sender, RoutedEventArgs e)
        {
            if (busObject != null && busAtt != null)
            {
                this.TearDown();
            }
        }

        /// <summary>
        /// Disconnect the bus from AllJoyn and stop its execution
        /// </summary>
        private async void TearDown()
        {
            try
            {
                await busAtt.DisconnectAsync(SignalServiceGlobals.ConnectSpec);
                await busAtt.StopAsync();

                this.OutputLine("Signal Service has been disconnected and terminated.");
            }
            catch (Exception ex)
            {
                this.OutputLine("Exiting Basic Service Produced Errors!");
                System.Diagnostics.Debug.WriteLine("Exception: ");
                System.Diagnostics.Debug.WriteLine(ex.ToString());
            }

            busObject = null;
            busAtt = null;
            busListener = null;
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
