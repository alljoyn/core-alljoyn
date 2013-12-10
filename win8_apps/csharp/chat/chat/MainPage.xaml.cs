// ****************************************************************************
// <copyright file="MainPage.xaml.cs" company="AllSeen Alliance.">
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// </copyright>
// ******************************************************************************

namespace Chat
{
    using System;
    using System.Collections.ObjectModel;
    using System.Linq;
    using System.Threading.Tasks;
    using AllJoyn;
    using Chat.Common;
    using Windows.UI.Core;
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Controls;
    using Windows.UI.Xaml.Input;
    using Windows.UI.Xaml.Navigation;

    /// <summary>
    /// The main page of the user interface.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        /// <summary>
        /// The path to the interface object.
        /// </summary>
        private const string ObjectPath = "/chatService";

        /// <summary>
        /// The application name.
        /// </summary>
        private const string ApplicationName = "chat";

        /// <summary>
        /// The name prefix used for finding and advertised name.
        /// </summary>
        private const string NamePrefix = "org.alljoyn.bus.samples.chat";

        /// <summary>
        /// The port to bind the session to.
        /// </summary>
        private const ushort ContactPort = 27;

        /// <summary>
        /// The transports to use.
        /// </summary>
        private TransportMaskType alljoynTransports = TransportMaskType.TRANSPORT_ANY;

        /// <summary>
        /// The bus we are attached to.
        /// </summary>
        private BusAttachment bus;

        /// <summary>
        /// The listeners for this bus.
        /// </summary>
        private Listeners busListeners;

        /// <summary>
        /// An instance of the class which handles chatting with another device.
        /// </summary>
        private ChatSessionObject chatService;

        /// <summary>
        /// The channel being hosted for others to join to.
        /// </summary>
        private string channelHosted = null;

        /// <summary>
        /// The one and only channel connected to another device.
        /// </summary>
        private string channelJoined = null;

        /// <summary>
        /// Set to true if AllJoyn has been intialized.
        /// </summary>
        private bool initialized = false;

        /// <summary>
        /// The collection of channels available for chat.
        /// </summary>
        private ObservableCollection<string> channels = new ObservableCollection<string>();

        /// <summary>
        /// The collection of messages in the current chat session.
        /// </summary>
        private ObservableCollection<MessageItem> chatLog = new ObservableCollection<MessageItem>();

        /// <summary>
        /// The Window dispatcher. This is used to get messages into the UI thread.
        /// </summary>
        private CoreDispatcher coreDispatcher;

        /// <summary>
        /// Initializes a new instance of the MainPage class. 
        /// </summary>
        public MainPage()
        {
            this.InitializeComponent();
            this.SessionId = 0;
            this.coreDispatcher = Window.Current.CoreWindow.Dispatcher;
            this.ChannelsComboBox.DataContext = this.Channels;
            this.ChatLogView.DataContext = this.ChatLog;
            Application.Current.Suspending += new Windows.UI.Xaml.SuspendingEventHandler(App_Suspending);
            Application.Current.Resuming += new EventHandler<Object>(App_Resuming);
        }

        private void App_Suspending(Object sender, Windows.ApplicationModel.SuspendingEventArgs e) {
            try
            {
                if (bus != null)
                {
                    bus.OnAppSuspend();
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
                if (bus != null)
                {
                    bus.OnAppResume();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Call OnAppResume() failed. Reason: " + AllJoynException.GetErrorMessage(ex.HResult));
            }
        }

        /// <summary>
        /// Gets the collection of messages in the current chat session.
        /// </summary>
        public ObservableCollection<MessageItem> ChatLog
        {
            get
            {
                return this.chatLog;
            }
        }

        /// <summary>
        /// Gets the collection of channels available for chat.
        /// </summary>
        public ObservableCollection<string> Channels
        {
            get
            {
                return this.channels;
            }
        }

        /// <summary>
        /// Gets or sets the session ID of the current chat session.
        /// </summary>
        public uint SessionId { get; set; }

        /// <summary>
        /// Add a channel name to the channel combo box.
        /// </summary>
        /// <param name="name">The name of the channel to add. This includes the name prefix.</param>
        public async void AddChannelName(string name)
        {
            if (!string.IsNullOrWhiteSpace(name))
            {
                await this.coreDispatcher.RunAsync(
                    CoreDispatcherPriority.Normal,
                    () =>
                    {
                        if (name.StartsWith(NamePrefix + '.') &&
                            (string.IsNullOrEmpty(this.channelHosted) ||
                            !name.StartsWith(NamePrefix + '.' + this.channelHosted)))
                        {
                            string channel = name.Remove(0, NamePrefix.Length + 1);
                            Channels.Remove(channel);
                            Channels.Add(channel);
                            if (ChannelsComboBox.SelectedIndex < 0)
                            {
                                ChannelsComboBox.SelectedIndex = 0;
                            }
                        }
                    });
            }
        }

        /// <summary>
        /// Remove a channel name from collection of channel names.
        /// </summary>
        /// <param name="name">The name of the channel to remove.</param>
        public async void RemoveChannelName(string name)
        {
            await this.coreDispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                if (name.StartsWith(NamePrefix + '.'))
                {
                    string channel = name.Remove(0, NamePrefix.Length + 1);
                    Channels.Remove(channel);
                }
            });
        }

        /// <summary>
        /// This method is called when the listener gets a session event. If the session lost is the curret
        /// active chat session we clean up things and get ready for another chat session.
        /// </summary>
        /// <param name="id">The id of the lost session.</param>
        public async void SessionLost(uint id)
        {
            if (id == this.SessionId)
            {
                await this.coreDispatcher.RunAsync(
                    CoreDispatcherPriority.Normal,
                    () =>
                    {
                        this.RemoveChannelName(this.channelJoined);
                        this.SessionId = 0;
                        this.channelJoined = null;
                        this.JoinChannelButton.Content = "Join Channel";
                        this.ChannelsComboBox.IsEnabled = true;
                        this.DisplayStatus("Session " + id + " is lost, reconnect is required");
                    });
            }
        }

        /// <summary>
        /// Send this chat message to the user interface thread for display.
        /// </summary>
        /// <param name="id">The session id of the chat message.</param>
        /// <param name="sender">The name of the sender on the other device.</param>
        /// <param name="messsage">The message the sender sent.</param>
        public async void OnChat(uint id, string sender, string messsage)
        {
            await this.coreDispatcher.RunAsync(
                CoreDispatcherPriority.Normal,
                () =>
                {
                    DateTime localtime = DateTime.Now;
                    MessageItem msgItem = new MessageItem(id, localtime.ToString(), sender, messsage);
                    ChatLog.Insert(0, msgItem);
                });
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            if (!this.initialized)
            {
                this.initialized = true;
                this.InitializeAllJoyn();
            }
        }

        /// <summary>
        /// Initialize the AllJoyn portions of the application.
        /// </summary>
        private void InitializeAllJoyn()
        {
            Task t1 = new Task(() =>
            {
                try
                {
                    Debug.UseOSLogging(true);
                    //Debug.SetDebugLevel("ALL", 1);
                    //Debug.SetDebugLevel("ALLJOYN", 7);

                    bus = new BusAttachment(ApplicationName, true, 4);
                    bus.Start();

                    const string connectSpec = "null:";
                    bus.ConnectAsync(connectSpec).AsTask().Wait();
                    DisplayStatus("Connected to AllJoyn successfully.");

                    busListeners = new Listeners(bus, this);
                    bus.RegisterBusListener(busListeners);
                    chatService = new ChatSessionObject(bus, ObjectPath, this);
                    bus.RegisterBusObject(chatService);
                    bus.FindAdvertisedName(NamePrefix);
                }
                catch (Exception ex)
                {
                    QStatus stat = AllJoynException.GetErrorCode(ex.HResult);
                    string errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                    DisplayStatus("InitializeAllJoyn Error : " + errMsg);
                }
            });
            t1.Start();
        }

        /// <summary>
        /// The handler for when the Start Channel button is pressed.
        /// </summary>
        /// <param name="sender">The parameter is not used.</param>
        /// <param name="e">The parameter is not used.</param>
        private void StartChannelBtnClicked(object sender, RoutedEventArgs e)
        {
            string name = this.EnterChannelTextView.Text;

            if (this.ValidateHostName(name))
            {
                if (this.StartChannelButton.Content.Equals("Start Channel"))
                {
                    this.StartChannelButton.Content = "Stop Channel";
                    this.StartChannel(name);
                    this.EnterChannelTextView.IsReadOnly = true;
                }
                else
                {
                    this.StartChannelButton.Content = "Start Channel";
                    this.EnterChannelTextView.IsReadOnly = false;
                    this.StopChannel();
                }
            }
            else
            {
                this.DisplayStatus("Invalid channel name.");
            }
        }

        /// <summary>
        /// The handler for the Join Channel button.
        /// </summary>
        /// <param name="sender">The parameter is not used.</param>
        /// <param name="e">The parameter is not used.</param>
        private void JoinChannelBtnClicked(object sender, RoutedEventArgs e)
        {
            int index = this.ChannelsComboBox.SelectedIndex;

            if (this.JoinChannelButton.Content.Equals("Join Channel"))
            {
                if (this.SessionId == 0 && this.Channels.Count > 0)
                {
                    string wkn = this.Channels.ElementAt(index);
                    this.JoinChannel(wkn);
                }
            }
            else
            {
                if (this.SessionId != 0)
                {
                    this.LeaveChannel();
                    this.JoinChannelButton.Content = "Join Channel";
                    this.ChannelsComboBox.IsEnabled = true;
                }
            }
        }

        /// <summary>
        /// The handler for the Send Message button.
        /// </summary>
        /// <param name="sender">The parameter is not used.</param>
        /// <param name="e">The parameter is not used.</param>
        private void SendMessageBtnClicked(object sender, RoutedEventArgs e)
        {
            string message = this.MessageBox.Text;
            if (!string.IsNullOrWhiteSpace(message))
            {
                this.SendMyMessage(this.SessionId, message);
            }
        }

        /// <summary>
        /// Handler for the message box event of a key being released. Look for the key "Enter" being
        /// released. If the key was Enter and the message box is not empty then send the message.
        /// </summary>
        /// <param name="sender">The parameter is not used.</param>
        /// <param name="e">This contains the active keyboard key.</param>
        private void OnMessageBoxKeyUp(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Enter)
            {
                string message = MessageBox.Text;

                if (!string.IsNullOrWhiteSpace(message))
                {
                    this.SendMyMessage(this.SessionId, message);
                }
            }
        }

        /// <summary>
        /// This method sends the current message to the chat service for delivery to the remote device.
        /// </summary>
        /// <param name="id">The session ID associated with this message.</param>
        /// <param name="message">The actual message being sent.</param>
        private void SendMyMessage(uint id, string message)
        {
            try
            {
                this.OnChat(this.SessionId, "Me : ", message);
                this.chatService.SendChatSignal(this.SessionId, message);
                this.MessageBox.Text = string.Empty;
            }
            catch (Exception ex)
            {
                QStatus stat = AllJoynException.GetErrorCode(ex.HResult);
                string errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                DisplayStatus("SendMessage Error : " + errMsg);
            }
        }

        /// <summary>
        /// Start up the channel we are hosting.
        /// </summary>
        /// <param name="name">The name, minus the name prefix, for the channel we are hosting.</param>
        private void StartChannel(string name)
        {
            try
            {
                this.channelHosted = name;
                string wellKnownName = this.MakeWellKnownName(name);
                this.bus.RequestName(wellKnownName, (int)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);
                this.bus.AdvertiseName(wellKnownName, alljoynTransports);
                ushort[] portOut = new ushort[1];
                SessionOpts opts = new SessionOpts(TrafficType.TRAFFIC_MESSAGES, true, ProximityType.PROXIMITY_ANY, alljoynTransports);
                this.bus.BindSessionPort(ContactPort, portOut, opts, this.busListeners);
                DisplayStatus("Start Chat channel " + name + " successfully");
            }
            catch (Exception ex)
            {
                QStatus stat = AllJoynException.GetErrorCode(ex.HResult);
                string errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                DisplayStatus("StartChannel Error : " + errMsg);
            }
        }

        /// <summary>
        /// Check this name for validity as part of valid host name of the form "NamePrefix + '.' + name".
        /// See also http://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-names
        /// </summary>
        /// <param name="name">The name to check for validity.</param>
        /// <returns>If a valid name then return true. Otherwise return false.</returns>
        private bool ValidateHostName(string name)
        {
            bool returnValue = false;

            if (!string.IsNullOrWhiteSpace(name) && !char.IsDigit(name[0]))
            {
                // Warning! There is a bit of cleverness here!
                // If lastCharWasDot results in being initialized to true then in later tests
                // the name will be invalidated for having a leading '.'.
                bool lastCharWasDot = name[0] == '.';

                returnValue = true;

                foreach (char c in name)
                {
                    if ((!char.IsLetterOrDigit(c) && c != '_' && c != '-' && c != '.') ||
                        (c == '.' && lastCharWasDot))
                    {
                        returnValue = false;
                        break;
                    }

                    lastCharWasDot = c == '.';
                }

                if (returnValue)
                {
                    const int DBusMaximumNameLength = 255;
                    string wellKnownName = this.MakeWellKnownName(name);

                    returnValue = wellKnownName.Length <= DBusMaximumNameLength;
                }
            }

            return returnValue;
        }

        /// <summary>
        /// Make a well known name from this name fragment. The name fragment should have been previously
        /// validated using ValidateHostName().
        /// </summary>
        /// <param name="name">The name fragment to make the well known bus name from.</param>
        /// <returns>The well known name.</returns>
        private string MakeWellKnownName(string name)
        {
            return NamePrefix + '.' + name;
        }

        /// <summary>
        /// Shut down the hosted channel.
        /// </summary>
        private void StopChannel()
        {
            try
            {
                var wellKnownName = this.MakeWellKnownName(this.channelHosted);
                this.bus.CancelAdvertiseName(wellKnownName, alljoynTransports);
                this.bus.UnbindSessionPort(ContactPort);
                this.bus.ReleaseName(wellKnownName);
                this.Channels.Remove(this.channelHosted);

                this.channelHosted = null;
            }
            catch (Exception ex)
            {
                QStatus stat = AllJoynException.GetErrorCode(ex.HResult);
                string errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                DisplayStatus("StopChannel Error : " + errMsg);
            }
        }

        /// <summary>
        /// Join an existing channel with the given name.
        /// </summary>
        /// <param name="name">The name of the channel to join.</param>
        private async void JoinChannel(string name)
        {
            try
            {
                this.DisplayStatus("Joining chat session: " + name);
                string wellKnownName = this.MakeWellKnownName(name);
                SessionOpts opts = new SessionOpts(TrafficType.TRAFFIC_MESSAGES, true, ProximityType.PROXIMITY_ANY, alljoynTransports);
                SessionOpts[] opts_out = new SessionOpts[1];
                this.channelJoined = name;
                JoinSessionResult result = await this.bus.JoinSessionAsync(
                                                                           wellKnownName,
                                                                           ContactPort,
                                                                           this.busListeners,
                                                                           opts,
                                                                           opts_out,
                                                                           null);

                if (result.Status == QStatus.ER_OK)
                {
                    this.SessionId = result.SessionId;
                    this.DisplayStatus("Join chat session " + this.SessionId + " successfully");
                    await this.coreDispatcher.RunAsync(
                        CoreDispatcherPriority.Normal,
                        () => { this.JoinChannelButton.Content = "Leave Channel"; });
                }
                else
                {
                    this.DisplayStatus("Join chat session " + result.SessionId + " failed. Error: " + result.Status);
                }
            }
            catch (Exception ex)
            {
                QStatus stat = AllJoynException.GetErrorCode(ex.HResult);
                string errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                DisplayStatus("JoinChannel Error : " + errMsg);
            }
        }

        /// <summary>
        /// Leave the currently active chat session.
        /// </summary>
        private void LeaveChannel()
        {
            try
            {
                this.bus.LeaveSession(this.SessionId);
                this.DisplayStatus("Leave chat session " + this.SessionId + " successfully");
            }
            catch (Exception ex)
            {
                QStatus stat = AllJoynException.GetErrorCode(ex.HResult);
                string errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.DisplayStatus("Leave chat session " + this.SessionId + " failed: " + errMsg);
            }

            this.SessionId = 0;
        }

        /// <summary>
        /// Output a status message using the UI thread. Allows empty and all whitespace messages to
        /// be output but not null.
        /// </summary>
        /// <param name="status">The status message to output.</param>
        public async void DisplayStatus(string status)
        {
            if (status != null)
            {
                await this.coreDispatcher.RunAsync(
                    CoreDispatcherPriority.Normal,
                    () => { this.AJStatus.Text = status; });
            }
        }

        /// <summary>
        /// A container class for a single message.
        /// </summary>
        public class MessageItem
        {
            /// <summary>
            /// Initializes a new instance of the MessageItem class. 
            /// </summary>
            /// <param name="session">The session id of the current chat session.</param>
            /// <param name="time">The time stamp of the message.</param>
            /// <param name="sender">The name of the sender.</param>
            /// <param name="msg">The actual message.</param>
            public MessageItem(uint session, string time, string sender, string msg)
            {
                this.SessionId = session;
                this.Time = time;
                this.Sender = sender;
                this.Message = msg;
            }

            /// <summary>
            /// Gets or sets the session ID this message is assocated with. 
            /// </summary>
            public uint SessionId { get; set; }

            /// <summary>
            /// Gets or sets the time stamp of this message.
            /// </summary>
            public string Time { get; set; }

            /// <summary>
            /// Gets or sets the name of the sender.
            /// </summary>
            public string Sender { get; set; }

            /// <summary>
            /// Gets or sets the actual chat message.
            /// </summary>
            public string Message { get; set; }
        }
    }
}