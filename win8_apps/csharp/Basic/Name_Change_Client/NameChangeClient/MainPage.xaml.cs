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

namespace Name_Change_Client
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using AllJoyn;
    using Name_Change_Bus_Listener;
    using Name_Change_Globals;
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
        /// current session id for the client-service application
        /// </summary>
        private static uint sessionId = 0;

        /// <summary>
        /// True if a client process is currently running, else false
        /// </summary>
        private static bool runningClient = false;

        /// <summary>
        /// Called with the disovery of the well known service name
        /// </summary>
        private AutoResetEvent foundNameEvent = new AutoResetEvent(false);

        /// <summary>
        /// Bus which implements the name change client application over AllJoyn
        /// </summary>
        private BusAttachment busAtt = null;

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
                System.Diagnostics.Debug.WriteLine("OnAppSuspend() failed. Reason = " + AllJoynException.GetErrorMessage(ex.HResult));
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
            ao.TextBox = this.TextBlockClient; // control in our Xaml page
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
            AllJoyn.Debug.SetDebugLevel("TCP", 7);
            AllJoyn.Debug.SetDebugLevel("ALLJOYN", 7);
            AllJoyn.Debug.SetDebugLevel("ALLJOYN_OBJ", 7);
            AllJoyn.Debug.SetDebugLevel("ALLJOYN_DAEMON", 7);
            AllJoyn.Debug.SetDebugLevel("ALLJOYN", 7);
            AllJoyn.Debug.SetDebugLevel("NS", 7);
            AllJoyn.Debug.SetDebugLevel("THREAD", 7);
            AllJoyn.Debug.SetDebugLevel("ALLJOYN_AUTH", 7);
            AllJoyn.Debug.SetDebugLevel("NETWORK", 7);
            AllJoyn.Debug.SetDebugLevel("STREAM", 7);
        }

        /// <summary>
        /// Connects to the bus, finds the service and sets the 'name' property to the value 
        /// specified by the user.
        /// </summary>
        /// <param name="sender">UI control which signaled the click event.</param>
        /// <param name="e">Arguments associated with the click event.</param>
        private void Button_RunNameChangeClient(object sender, RoutedEventArgs e)
        {
            if (this.TextBox_Input.Text == string.Empty)
            {
                this.OutputLine("You must provide an argument to run Name Change Client!");
            }

            if (!runningClient && this.TextBox_Input.Text != string.Empty)
            {
                string newName = this.TextBox_Input.Text;
                Task task = new Task(async () =>
                {
                    try
                    {
                        runningClient = true;

                        busAtt = new BusAttachment("NameChangeApp", true, 4);
                        OutputLine("BusAttachment Created.");

                        NameChangeBusListener busListener = new NameChangeBusListener(busAtt, foundNameEvent);
                        busAtt.RegisterBusListener(busListener);
                        OutputLine("BusListener Registered.");

                        /* Create and register the bundled daemon. The client process connects to daemon over tcp connection */
                        busAtt.Start();
                        await busAtt.ConnectAsync(NameChangeGlobals.ConnectSpec);
                        OutputLine("Bundled Daemon Registered.");
                        OutputLine("BusAttachment Connected to " + NameChangeGlobals.ConnectSpec + ".");

                        busAtt.FindAdvertisedName(NameChangeGlobals.WellKnownServiceName);
                        foundNameEvent.WaitOne();

                        /* Configure session properties and request a session with device with wellKnownName */
                        SessionOpts sOpts = new SessionOpts(
                            NameChangeGlobals.SessionProps.TrType,
                            NameChangeGlobals.SessionProps.IsMultiPoint,
                            NameChangeGlobals.SessionProps.PrType,
                            NameChangeGlobals.SessionProps.TmType);
                        SessionOpts[] sOptsOut = new SessionOpts[1];
                        JoinSessionResult joinResults = await busAtt.JoinSessionAsync(
                            NameChangeGlobals.WellKnownServiceName, 
                            NameChangeGlobals.SessionProps.SessionPort, 
                            busListener, 
                            sOpts, 
                            sOptsOut, 
                            null);
                        QStatus status = joinResults.Status;
                        if (QStatus.ER_OK == status) 
                        {
                            this.OutputLine("Join Session was successful (sessionId=" + joinResults.SessionId + ").");
                        }
                        else 
                        {
                            this.OutputLine("Join Session was unsuccessful.");
                        }

                        ProxyBusObject pbo = new ProxyBusObject(busAtt, NameChangeGlobals.WellKnownServiceName, NameChangeGlobals.ServicePath, sessionId);
                        if (QStatus.ER_OK == status)
                        {
                            IntrospectRemoteObjectResult introResult = await pbo.IntrospectRemoteObjectAsync(null);
                            status = introResult.Status;
                            if (QStatus.ER_OK == status)
                            {
                                this.OutputLine("Introspection of the service object was successful.");
                            }
                            else
                            {
                                this.OutputLine("Introspection of the service object was unsuccessful.");
                            }
                        }

                        if (QStatus.ER_OK == status)
                        {
                            object[] obj = new object[] { newName };
                            MsgArg msg = new MsgArg("s", obj);
                            SetPropertyResult setResult = await pbo.SetPropertyAsync(NameChangeGlobals.InterfaceName, "name", msg, null, 2000);
                        }

                        TearDown();
                    }
                    catch (Exception ex)
                    {
                        QStatus s = AllJoynException.GetErrorCode(ex.HResult);
                        OutputLine("Error: " + ex.ToString());
                        runningClient = false;
                    }
                });
                task.Start();
            }
        }

        /// <summary>
        /// Disconnects and disposes the primary alljoyn object which implements the app
        /// </summary>
        private async void TearDown()
        {
            try
            {
                await this.busAtt.DisconnectAsync(NameChangeGlobals.ConnectSpec);
                await this.busAtt.StopAsync();
                this.OutputLine("Basic Client has Exited.\n");
            }
            catch (Exception ex)
            {
                this.OutputLine("Errors were produced while exiting the application: " + ex.Message.ToString());
            }

            runningClient = false;
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
