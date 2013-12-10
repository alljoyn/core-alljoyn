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

namespace Sessions
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using AllJoyn;
    using Sessions.Common;
    using Windows.Foundation;
    using Windows.Foundation.Collections;
    using Windows.UI.Core;
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Controls;
    using Windows.UI.Xaml.Controls.Primitives;
    using Windows.UI.Xaml.Data;
    using Windows.UI.Xaml.Input;
    using Windows.UI.Xaml.Media;
    using Windows.UI.Xaml.Navigation;

    /// <summary>
    /// This is the main sequence of execution which acts as the view/controller for the 
    /// Sessions application.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        /// <summary>
        /// Session object which manages all alljoyn operations which have been specified by the user.
        /// </summary>
        private SessionOperations sessionOps = null;

        /// <summary>
        /// True if the enter key has key down event and false if it has key up event
        /// </summary>
        private bool btnUpDown = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="MainPage"/> class
        /// </summary>
        public MainPage()
        {
            this.InitializeComponent();
            Application.Current.Suspending += new Windows.UI.Xaml.SuspendingEventHandler(App_Suspending);
            Application.Current.Resuming += new EventHandler<Object>(App_Resuming);

            this.sessionOps = new SessionOperations(this);

            // Set OS Logging to false so debug prints are written to log file in My Docs
            // To have ability to write to log file must change manifest file so capabilities
            // includes My Docs access and add 'File Type Association' in Declarations for
            // .log files
            AllJoyn.Debug.UseOSLogging(false);
            Application.Current.Suspending += new Windows.UI.Xaml.SuspendingEventHandler(App_Suspending);
            Application.Current.Resuming += new EventHandler<Object>(App_Resuming);
        }

        private void App_Suspending(Object sender, Windows.ApplicationModel.SuspendingEventArgs e)
        {
            try
            {
                if (this.sessionOps.Bus != null)
                {
                    this.sessionOps.Bus.OnAppSuspend();
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
                if (this.sessionOps.Bus != null)
                {
                    this.sessionOps.Bus.OnAppResume();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Call OnAppResume() failed. Reason: " + AllJoynException.GetErrorMessage(ex.HResult));
            }
        }

        /// <summary>
        /// Output a message to the UI
        /// </summary>
        /// <param name="msg">Message to output</param>
        public async void Output(string msg)
        {
            await Dispatcher.RunAsync(
                CoreDispatcherPriority.Normal, 
                () =>
                {
                    this.output.Text += msg + "\n";
                    this.scrollBar.ScrollToVerticalOffset(this.scrollBar.ExtentHeight);
                });
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            this.Output("If unfamiliar with this program type 'help' and press enter");
        }

        /// <summary>
        /// Because the key down mechanism sends two signals add a switch used to make 
        /// sure the key has come back up since last key down event
        /// </summary>
        /// <param name="sender">Obect which intiated the event</param>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        private void Btn_Release(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Enter)
            {
                this.btnUpDown = false;
            }
        }

        /// <summary>
        /// Called when the user has pressed the enter key to submit a command to the application
        /// </summary>
        /// <param name="sender">Obect which intiated the event</param>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        private async void Enter_Command(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Enter && string.Empty != this.input.Text && !this.btnUpDown)
            {
                string lineInput = this.input.Text.Trim();
                string[] tokens = lineInput.Split(new char[] { ' ', '\t' });
                for (uint i = 0; i < tokens.Length; i++)
                {
                    tokens[i] = tokens[i].Trim();
                }

                this.Output(string.Empty);
                this.Output(">>>>> " + lineInput);

                switch (tokens[0].ToLower())
                {
                    case "debug":
                        try
                        {
                            if (3 == tokens.Length && string.Empty != tokens[1] && string.Empty != tokens[2])
                            {
                                uint level = Convert.ToUInt32(tokens[2]);
                                this.sessionOps.DoDebug(tokens[1], level);
                            }
                            else
                            {
                                this.Output("Usage: debug <modulename> <level>");
                            }
                        }
                        catch (Exception ex)
                        {
                            string errMsg = ex.Message;
                            this.Output("Invalid Debug Level");
                        }

                        break;
                    case "requestname":
                        if (2 == tokens.Length && string.Empty != tokens[1])
                        {
                            this.sessionOps.DoRequestName(tokens[1]);
                        }
                        else
                        {
                            this.Output("Usage: requestname <name>");
                        }

                        break;
                    case "releasename":
                        if (2 == tokens.Length && string.Empty != tokens[1])
                        {
                            this.sessionOps.DoReleaseName(tokens[1]);
                        }
                        else
                        {
                            this.Output("Usage: releasename <name>");
                        }

                        break;
                    case "bind":
                        try
                        {
                            if (2 == tokens.Length || 6 == tokens.Length)
                            {
                                uint port = Convert.ToUInt32(tokens[1]);
                                if (0 == port)
                                {
                                    this.Output("Invalid Port Number");
                                }
                                else
                                {
                                    SessionOpts opts = new SessionOpts(TrafficType.TRAFFIC_MESSAGES, false, ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY);
                                    if (6 == tokens.Length)
                                    {
                                        string[] optsInput = { tokens[2], tokens[3], tokens[4], tokens[5] };
                                        opts = this.GetSessionOpts(optsInput);
                                    }

                                    if (null != opts)
                                    {
                                        this.sessionOps.DoBind(port, opts);
                                    }
                                    else
                                    {
                                        this.Output("Usage: bind <port> [traffic] [isMultipoint] [proximity] [transports]");
                                    }
                                }
                            }
                            else
                            {
                                this.Output("Usage: bind <port> [traffic] [isMultipoint] [proximity] [transports]");
                            }
                        }
                        catch (Exception ex)
                        {
                            string errMsg = ex.Message;
                            this.Output("Invalid Port Number");
                        }

                        break;
                    case "unbind":
                        try
                        {
                            if (2 == tokens.Length)
                            {
                                uint port = Convert.ToUInt32(tokens[1]);
                                if (0 == port)
                                {
                                    this.Output("Invalid Port Number");
                                }
                                else
                                {
                                    this.sessionOps.DoUnbind(port);
                                }
                            }
                            else 
                            {
                                this.Output("Usage: unbind <port>");
                            }
                        }
                        catch (Exception ex) 
                        {
                            string errMsg = ex.Message;
                            this.Output("Invalid Port Number");
                        }

                        break;
                    case "advertise":
                        try
                        {
                            if ((2 == tokens.Length || 3 == tokens.Length) && string.Empty != tokens[1])
                            {
                                TransportMaskType transport = TransportMaskType.TRANSPORT_ANY;
                                if (3 == tokens.Length)
                                {
                                    uint tp = Convert.ToUInt32(tokens[2]);
                                    transport = (TransportMaskType)tp;
                                }

                                this.sessionOps.DoAdvertise(tokens[1], transport);
                            }
                            else
                            {
                                this.Output("Usage: advertise <name> [transports]");
                            }
                        }
                        catch (Exception ex)
                        {
                            string errMsg = ex.Message;
                            this.Output("Invalid Transport Mask");
                        }

                        break;
                    case "canceladvertise":
                        try
                        {
                            if ((2 == tokens.Length || 3 == tokens.Length) && string.Empty != tokens[1])
                            {
                                TransportMaskType transport = TransportMaskType.TRANSPORT_ANY;
                                if (3 == tokens.Length)
                                {
                                    uint tp = Convert.ToUInt32(tokens[2]);
                                    transport = (TransportMaskType)tp;
                                }

                                this.sessionOps.DoCancelAdvertise(tokens[1], transport);
                            }
                            else
                            {
                                this.Output("Usage: canceladvertise <name> [transports]");
                            }
                        }
                        catch (Exception ex)
                        {
                            string errMsg = ex.Message;
                            this.Output("Invalid Transport Mask");
                        }

                        break;
                    case "find":
                        if (2 == tokens.Length && string.Empty != tokens[1])
                        {
                            this.sessionOps.DoFind(tokens[1]);
                        }
                        else
                        {
                            this.Output("Usage: find <name_prefix>");
                        }

                        break;
                    case "cancelfind":
                        if (2 == tokens.Length && string.Empty != tokens[1])
                        {
                            this.sessionOps.DoCancelFind(tokens[1]);
                        }
                        else
                        {
                            this.Output("Usage: cancelfind <name_prefix>");
                        }

                        break;
                    case "list":
                        this.sessionOps.DoList();
                        break;
                    case "join":
                        try
                        {
                            if ((3 == tokens.Length || 7 == tokens.Length) && string.Empty != tokens[1])
                            {
                                uint port = Convert.ToUInt32(tokens[2]);
                                if (0 == port)
                                {
                                    this.Output("Invalid Port Number");
                                }
                                else
                                {
                                    SessionOpts opts = new SessionOpts(TrafficType.TRAFFIC_MESSAGES, false, ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY);
                                    if (7 == tokens.Length)
                                    {
                                        string[] optsInput = { tokens[3], tokens[4], tokens[5], tokens[6] };
                                        opts = this.GetSessionOpts(optsInput);
                                    }

                                    if (null != opts)
                                    {
                                        this.sessionOps.DoJoinAsync(tokens[1], port, opts);
                                    }
                                    else
                                    {
                                        this.Output("Usage: join <name> <port> [traffic] [isMultipoint] [proximity] [transports]");
                                    }
                                }
                            }
                            else
                            {
                                this.Output("Usage: join <name> <port> [traffic] [isMultipoint] [proximity] [transports]");
                            }
                        }
                        catch (Exception ex)
                        {
                            string errMsg = ex.Message;
                            this.Output("Invalid Port Number");
                        }

                        break;
                    case "leave":
                        if (2 == tokens.Length)
                        {
                            uint sessionId = this.GetSessionId(tokens[1]);
                            if (sessionId == 0)
                            {
                                this.Output("Invalid Session ID");
                            }
                            else
                            {
                                this.sessionOps.DoLeave(sessionId);
                            }
                        }
                        else 
                        {
                            this.Output("Usage: leave <sessionId>");
                        }

                        break;
                    case "timeout":
                        try
                        {
                            if (3 == tokens.Length)
                            {
                                uint sessionId = this.GetSessionId(tokens[1]);
                                uint timeOut = Convert.ToUInt32(tokens[2]);
                                if (sessionId == 0 || timeOut == 0)
                                {
                                    this.Output("Usage: timeout <sessionId> <timeout>");
                                }
                                else
                                {
                                    this.sessionOps.DoSetLinkTimeoutAsync(sessionId, timeOut);
                                }
                            }
                            else 
                            {
                                this.Output("Usage: timeout <sessionId> <timeout>");
                            }
                        }
                        catch (Exception ex) 
                        {
                            string errMsg = ex.Message;
                            this.Output("Invalid Timeout argument");
                        }
                        
                        break;
                    case "chat":
                        byte flags1 = (byte)AllJoyn.AllJoynFlagType.ALLJOYN_FLAG_GLOBAL_BROADCAST;
                        if (3 <= tokens.Length)
                        {
                            uint sessionId = this.GetSessionId(tokens[1]);
                            string msg = string.Join(" ", tokens, 2, tokens.Length - 2);
                            if (sessionId == 0)
                            {
                                this.Output("Usage: chat <sessionId> <msg>");
                            }
                            else
                            {
                                this.sessionOps.SendChatSignal(sessionId, msg, flags1);
                            }
                        }
                        else 
                        {
                            this.Output("Usage: chat <sessionId> <msg>");
                        }

                        break;
                    case "cchat":
                        byte flags2 = (byte)AllJoyn.AllJoynFlagType.ALLJOYN_FLAG_COMPRESSED;
                        if (3 <= tokens.Length)
                        {
                            uint sessionId = this.GetSessionId(tokens[1]);
                            string msg = string.Join(" ", tokens, 2, tokens.Length - 2);
                            if (sessionId == 0 || msg == string.Empty)
                            {
                                this.Output("Usage: cchat <sessionId> <msg>");
                            }
                            else
                            {
                                this.sessionOps.SendChatSignal(sessionId, msg, flags2);
                            }
                        }
                        else 
                        {
                            this.Output("Usage: cchat <sessionId> <msg>");
                        }

                        break;
                    case "schat":
                        byte flags3 = (byte)AllJoyn.AllJoynFlagType.ALLJOYN_FLAG_SESSIONLESS;
                        if (2 <= tokens.Length)
                        {
                            string msg = string.Join(" ", tokens, 1, tokens.Length - 1);
                            if (msg == string.Empty)
                            {
                                this.Output("Usage: schat <msg>");
                            }
                            else
                            {
                                this.sessionOps.SendSessionlessChatSignal(msg, flags3);
                            }
                        }
                        else
                        {
                            this.Output("Usage: schat <msg>");
                        }

                        break;
                    case "addmatch":
                        if (2 <= tokens.Length)
                        {
                            string msg = string.Join(" ", tokens, 1, tokens.Length - 1);
                            if (msg == string.Empty)
                            {
                                this.Output("Usage: addmatch <filter>");
                            }
                            else
                            {
                                this.sessionOps.AddRule(msg);
                            }
                        }
                        else
                        {
                            this.Output("Usage: addmatch <filter>");
                        }

                        break;
                    case "chatecho":
                        if (2 == tokens.Length)
                        {
                            if (tokens[1] == "on")
                            {
                                this.sessionOps.SetChatEcho(true);
                            }
                            else if (tokens[1] == "off")
                            {
                                this.sessionOps.SetChatEcho(false);
                            }
                            else
                            {
                                this.Output("Usage: chatecho [on|off]");
                            }
                        }
                        else 
                        {
                            this.Output("Usage: chatecho [on|off]");
                        }

                        break;
                    case "exit":
                        this.sessionOps.Exit();
                        break;
                    case "help":
                        this.sessionOps.PrintHelp();
                        break;
                    default:
                        this.Output("Unknown command: " + tokens[0]);
                        break;
                }

                this.btnUpDown = true;
                await Dispatcher.RunAsync(
                CoreDispatcherPriority.Normal,
                () =>
                {
                    this.input.Text = string.Empty;
                });
            }
            else if (e.Key == Windows.System.VirtualKey.Enter && string.Empty == this.input.Text && !this.btnUpDown)
            {
                this.Output(">>>>>");
                this.btnUpDown = true;
            }
        }

        /// <summary>
        /// Get the session opts from user input
        /// </summary>
        /// <param name="tokens">Arguments supplied by the user</param>
        /// <returns>Session opts</returns>
        private SessionOpts GetSessionOpts(string[] tokens)
        {
            try
            {
                uint traffic = Convert.ToUInt32(tokens[0]);
                bool isMultipoint = tokens[1].ToLower() == "true";
                uint proximity = Convert.ToUInt32(tokens[2]);
                uint transport = Convert.ToUInt32(tokens[3]);
                return new SessionOpts((TrafficType)traffic, isMultipoint, (ProximityType)proximity, (TransportMaskType)transport);
            }
            catch (Exception ex)
            {
                string errMsg = ex.Message;
                return null;
            }
        }

        /// <summary>
        /// If the session id is given as index in session table get session id from session ops,
        /// else convert string to uint.
        /// </summary>
        /// <param name="id">session id argument provided by user</param>
        /// <returns>session id</returns>
        private uint GetSessionId(string id)
        {
            try
            {
                if ('#' == id[0])
                {
                    return this.sessionOps.GetSessionId(Convert.ToUInt32(id.Substring(1)));
                }
                else
                {
                    return Convert.ToUInt32(id);
                }
            }
            catch (Exception ex)
            {
                string errMsg = ex.Message;
                this.Output("Invalid Session ID");
                return 0;
            }
        }
    }
}
