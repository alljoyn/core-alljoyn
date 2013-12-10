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

namespace Basic_Client
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using AllJoyn;
    using Basic_Client_Bus_Listener;
    using Client_Globals;
    using Windows.Foundation;
    using Windows.UI.Core;
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Controls;
    using Windows.UI.Xaml.Navigation;

    /// <summary>
    /// Space which contains the dynamic functionality of the page when dealing with user interactions
    /// </summary>
    public sealed partial class MainPage : Page
    {
        /// <summary>
        /// True if a client process is currently running, else false
        /// </summary>
        private static bool runningClient = false;

        /// <summary>
        /// Event corresponding to the disovery of the well known service name
        /// </summary>
        private AutoResetEvent foundNameEvent = new AutoResetEvent(false);

        /// <summary>
        /// Bus which implements the basic client application over AllJoyn
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
            ao.TextBox = this.TextBlockClient; // control in our Xaml page
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
        /// Connects to the bus, finds the service and calls the 'cat' with the two 
        /// arguments "Hello " and "World!"
        /// </summary>
        /// <param name="sender">UI control which signaled the click event</param>
        /// <param name="e">arguments associated with the click event</param>
        private void Button_RunClick(object sender, RoutedEventArgs e)
        {
            if (!runningClient)
            {
                Task task = new Task(async () =>
                {
                    try
                    {
                        runningClient = true;

                        busAtt = new BusAttachment("ClientApp", true, 4);
                        this.OutputLine("BusAttachment Created.");

                        BasicClientBusListener basicClientBusListener = new BasicClientBusListener(busAtt, foundNameEvent);
                        busAtt.RegisterBusListener(basicClientBusListener);
                        this.OutputLine("BusListener Registered.");

                        /* Create and register the bundled daemon. The client process connects to daemon over tcp connection */
                        busAtt.Start();
                        await busAtt.ConnectAsync(BasicClientGlobals.ConnectSpec);
                        this.OutputLine("Bundled Daemon Registered.");
                        this.OutputLine("BusAttachment Connected to " + BasicClientGlobals.ConnectSpec + ".");

                        busAtt.FindAdvertisedName(BasicClientGlobals.WellKnownServiceName);
                        foundNameEvent.WaitOne();

                        /* Configure session properties and request a session with device with wellKnownName */
                        SessionOpts sessionOpts = new SessionOpts(
                            BasicClientGlobals.SessionProps.TrType, 
                            BasicClientGlobals.SessionProps.IsMultiPoint, 
                            BasicClientGlobals.SessionProps.PrType, 
                            BasicClientGlobals.SessionProps.TmType);
                        SessionOpts[] sOptsOut = new SessionOpts[1];
                        JoinSessionResult joinResults = await busAtt.JoinSessionAsync(
                            BasicClientGlobals.WellKnownServiceName,
                            BasicClientGlobals.SessionProps.SessionPort,
                            basicClientBusListener,
                            sessionOpts,
                            sOptsOut,
                            null);
                        QStatus status = joinResults.Status;
                        if (QStatus.ER_OK != status)
                        {
                            this.OutputLine("Joining a session with the Service was unsuccessful.");
                        }
                        else
                        {
                            this.OutputLine("Join Session was successful (sessionId=" + joinResults.SessionId + ").");
                        }

                        // Create the proxy for the service interface by introspecting the service bus object
                        ProxyBusObject proxyBusObject = new ProxyBusObject(busAtt, BasicClientGlobals.WellKnownServiceName, BasicClientGlobals.ServicePath, 0);
                        if (QStatus.ER_OK == status)
                        {
                            IntrospectRemoteObjectResult introResult = await proxyBusObject.IntrospectRemoteObjectAsync(null);
                            status = introResult.Status;
                            if (QStatus.ER_OK != status)
                            {
                                this.OutputLine("Introspection of the service bus object failed.");
                            }
                            else
                            {
                                this.OutputLine("Introspection of the service bus object was successful.");
                            }
                        }

                        if (QStatus.ER_OK == status)
                        {
                            // Call 'cat' method with the two string to be concatenated ("Hello" and " World!")
                            MsgArg[] catMe = new MsgArg[2];
                            catMe[0] = new MsgArg("s", new object[] { "Hello" });
                            catMe[1] = new MsgArg("s", new object[] { " World!" });

                            InterfaceDescription interfaceDescription = proxyBusObject.GetInterface(BasicClientGlobals.InterfaceName);
                            InterfaceMember interfaceMember = interfaceDescription.GetMember("cat");

                            this.OutputLine("Calling the 'cat' method of the service with args 'Hello' and ' World!'");
                            MethodCallResult callResults = await proxyBusObject.MethodCallAsync(interfaceMember, catMe, null, 100000, 0);
                            Message msg = callResults.Message;
                            if (msg.Type == AllJoynMessageType.MESSAGE_METHOD_RET)
                            {
                                string strRet = msg.GetArg(0).Value as string;
                                this.OutputLine("Sender '" + msg.Sender + "' returned the value '" + strRet + "'");
                            }
                            else
                            {
                                this.OutputLine("The 'cat' method call produced errors of type: " + msg.Type.ToString());
                            }
                        }

                        TearDown();
                    }
                    catch (Exception ex)
                    {
                        QStatus s = AllJoynException.GetErrorCode(ex.HResult);
                    }
                });
                task.Start();
            }
        }

        /// <summary>
        /// Disconnects the client from alljoyn
        /// </summary>
        private async void TearDown()
        {
            try
            {
                // Tear down the client
                await this.busAtt.DisconnectAsync(BasicClientGlobals.ConnectSpec);
                await this.busAtt.StopAsync();
                this.OutputLine("Basic Client has Exited.\n");
            }
            catch (Exception ex)
            {
                this.OutputLine("An error occurred while closing the client.");
                this.OutputLine("Error: " + ex.Message);
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