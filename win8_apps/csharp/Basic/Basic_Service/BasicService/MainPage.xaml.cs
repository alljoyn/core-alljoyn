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

namespace Basic_CSharp_
{
    using System;
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using AllJoyn;
    using Basic_Service_Bus_Listener;
    using Basic_Service_Bus_Object;
    using Service_Globals;
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
        /// busObject provides the interface which the client applications will remotely access
        /// </summary>
        private static BasicServiceBusObject busObject = null;

        /// <summary>
        /// busAtt provides a way to interface with the D-Bus and interact with other device in
        /// proximity
        /// </summary>
        private static BusAttachment busAtt = null;

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
        /// Outputs 'msg' to the text box in the UI of the application
        /// </summary>
        /// <param name="msg">Message that will be output in the text box of the UI</param>
        public async void OutputLine(string msg)
        {
            ArgumentObject ao = new ArgumentObject();
            ao.TextBox = this.TextBlockService; // control in our Xaml page
            ao.ScrollBar = this.ScrollViewerMainText;
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
        /// Established the service functionality of the application by creating and registering
        /// </summary>
        /// <param name="sender">UI control which signaled the click event</param>
        /// <param name="e">arguments associated with the click event</param>
        private void Button_RunClick(object sender, RoutedEventArgs e)
        {
            if (busObject == null && busAtt == null)
            {
                Task task = new Task(async () =>
                {
                    try
                    {
                        busAtt = new BusAttachment("ServiceApp", true, 4);

                        busObject = new BasicServiceBusObject(busAtt);

                        BasicServiceBusListener busListener = new BasicServiceBusListener(busAtt);
                        OutputLine("BusAttachment, BusListener and BusObject Created.");
                        busAtt.RegisterBusListener(busListener);
                        OutputLine("BusListener Registered.");

                        busAtt.Start();
                        await busAtt.ConnectAsync(BasicServiceGlobals.ConnectSpec);
                        OutputLine("Bundled Daemon Registered.");
                        OutputLine("BusAttachment Connected to " + BasicServiceGlobals.ConnectSpec + ".");
                       
                        SessionOpts sessionOpts = new SessionOpts(
                                BasicServiceGlobals.SessionProps.TrType,
                                BasicServiceGlobals.SessionProps.IsMultiPoint,
                                BasicServiceGlobals.SessionProps.PrType,
                                BasicServiceGlobals.SessionProps.TmType);
                        try
                        {
                            ushort[] portOut = new ushort[1];
                            busAtt.BindSessionPort(BasicServiceGlobals.SessionProps.SessionPort, portOut, sessionOpts, busListener);

                            busAtt.RequestName(BasicServiceGlobals.WellKnownServiceName, (int)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);

                            busAtt.AdvertiseName(BasicServiceGlobals.WellKnownServiceName, TransportMaskType.TRANSPORT_ANY);
                            OutputLine("Name is Being Advertised as: " + BasicServiceGlobals.WellKnownServiceName);
                        }
                        catch (COMException ce)
                        {
                            QStatus s = AllJoynException.GetErrorCode(ce.HResult);
                            OutputLine("Error occurred while trying to establish the service over the bus.");
                            TearDown();
                        }
                    }
                    catch (Exception ex)
                    {
                        System.Diagnostics.Debug.WriteLine("Errors were produced while trying to establish the service.");
                        QStatus status = AllJoynException.GetErrorCode(ex.HResult);
                        busObject = null;
                        busAtt = null;
                    }
                });
                task.Start();
            }
        }

        /// <summary>
        /// terminates the service tread when corresponding button in the UI is clicked
        /// </summary>
        /// <param name="sender">control in the UI which triggered event</param>
        /// <param name="e">properties of the event which occured</param>
        private void Button_StopClick(object sender, RoutedEventArgs e)
        {
            if (busObject != null && busAtt != null)
            {
                this.TearDown();
            }
        }

        /// <summary>
        /// Terminates the service application by discarding the alljoyn object
        /// </summary>
        private async void TearDown()
        {
            try
            {
                await busAtt.DisconnectAsync(BasicServiceGlobals.ConnectSpec);
                await busAtt.StopAsync();

                this.OutputLine("Basic Service has been exited.");
                this.OutputLine(string.Empty);
            }
            catch (Exception ex)
            {
                this.OutputLine("An error was produced while closing the service: " + ex.Message.ToString());
            }

            busObject = null;
            busAtt = null;
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
            /// Gets or sets the ScrollViewer associated with the output text box.
            /// </summary>
            public ScrollViewer ScrollBar { get; set; }

            /// <summary>
            /// Used by the dispacter when sending output to the UI's textbox.
            /// </summary>
            public void OnDispatched()
            {
                this.TextBox.Text += this.Text;

                // Scroll to the bottom of the text.
                if (this.ScrollBar != null)
                {
                    this.ScrollBar.ScrollToVerticalOffset(this.ScrollBar.ExtentHeight);
                }
            }
        }
    }
}