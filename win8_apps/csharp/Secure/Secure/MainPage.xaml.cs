//-----------------------------------------------------------------------
// <copyright file="MainPage.xaml.cs" company="AllSeen Alliance.">
//    Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//       Permission to use, copy, modify, and/or distribute this software for any
//       purpose with or without fee is hereby granted, provided that the above
//       copyright notice and this permission notice appear in all copies.
//
//       THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//       WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//       MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//       ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//       WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//       ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//       OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// </copyright>
//-----------------------------------------------------------------------

namespace Secure
{
    using System;
    using System.Threading;
    using Secure.Common;
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
        /// The text for the Start/Stop Server when the server is stopped.
        /// </summary>
        private const string StartServer = "Start Server";

        /// <summary>
        /// The text for the Start/Stop Server when the server is running.
        /// </summary>
        private const string StopServer = "Stop Server";

        /// <summary>
        /// The text for the Start/Stop of client when the client is stopped.
        /// </summary>
        private const string StartClient = "Start Client";

        /// <summary>
        /// The text for the Start/Stop of client A is running.
        /// </summary>
        private const string StopClient = "Stop Client";

        /// <summary>
        /// The application name for the client.
        /// </summary>
        private const string ApplicationNameClient = "SRPSecurityClient";

        /// <summary>
        /// Initializes a new instance of the MainPage class.
        /// </summary>
        public MainPage()
        {
            this.InitializeComponent();
            this.PinReady = new AutoResetEvent(false);
        }

        /// <summary>
        /// Gets or sets the event to be set when there is a pin waiting from the user
        /// </summary>
        private AutoResetEvent PinReady { get; set; }

        /// <summary>
        /// Gets or sets the service.
        /// </summary>
        private Service SharedService { get; set; }

        /// <summary>
        /// Gets or sets an instance of the client.
        /// </summary>
        private Client Client { get; set; }

        /// <summary>
        /// Gets or sets the ScrollViewer for the output text box.
        /// </summary>
        private ScrollViewer Scrollbar { get; set; }

        /// <summary>
        /// Outputs 'msg' to the text box in the UI of the application as well as the debug output
        /// console.
        /// </summary>
        /// <param name="msg">Message that will be output in the text box of the UI.</param>
        public void OutputLine(string msg)
        {
            // If this is attempted at OnNavigatedTo() time the result is null.
            this.GetScrollBar();

            string timestampedMsg = DateTime.Now.ToString() + ": " + msg;
            ArgumentObject ao = new ArgumentObject(timestampedMsg + "\n", this.textBoxOutput, this.Scrollbar);

            Dispatcher.RunAsync(
                CoreDispatcherPriority.Normal,
                new DispatchedHandler(ao.OnDispatched)).AsTask();

            System.Diagnostics.Debug.WriteLine(timestampedMsg);
        }

        /// <summary>
        /// Output a PIN to the "PIN to use" textbox.
        /// </summary>
        /// <param name="pin">The PIN to put in the textbox.</param>
        public void OutputPIN(string pin)
        {
            PinTransfer pt = new PinTransfer();

            pt.Pin = pin;
            pt.TextBox = this.textBoxPinServer; // The control in our Xaml page

            Dispatcher.RunAsync(
                CoreDispatcherPriority.Normal,
                new DispatchedHandler(pt.OnDispatched)).AsTask();
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

            // Set the text on the buttons.
            this.buttonStartServer.Content = MainPage.StartServer;
            this.buttonClient.Content = MainPage.StartClient;
        }

        /// <summary>
        /// If necesary get the ScrollViewer and save it away.
        /// </summary>
        private void GetScrollBar()
        {
            if (this.Scrollbar == null)
            {
                this.Scrollbar = VisualTreeHelperEx.GetFirstDescendantOfType<ScrollViewer>(this.textBoxOutput);
            }
        }

        /// <summary>
        /// The handler for the clicking of the Start Server button.
        /// </summary>
        /// <param name="sender">The parameter is not used.</param>
        /// <param name="e">The parameter is not used.</param>
        private void StartServerClick(object sender, RoutedEventArgs e)
        {
            if (this.SharedService == null)
            {
                this.OutputLine("Starting the Server.");
                this.SharedService = new Service();

                this.buttonStartServer.Content = StopServer;

                // Disable the client if the server is running.
                this.EnableClientButtons(false);
            }
            else
            {
                this.OutputLine("Stopping the Server.");
                this.SharedService.Stop();
                this.SharedService = null;

                this.buttonStartServer.Content = StartServer;

                // Enable the client if the server is running.
                this.EnableClientButtons(true);
                this.OutputLine("====================");
            }
        }

        /// <summary>
        /// Enable/disable all the client controls.
        /// </summary>
        /// <param name="enable">true if the controls should be enabled.
        /// false if they should be disabled.</param>
        private void EnableClientButtons(bool enable)
        {
            this.buttonClient.IsEnabled = enable;
            this.buttonSendPin.IsEnabled = enable;
            this.textBoxPinClient.IsReadOnly = !enable;
        }

        /// <summary>
        /// The handler for the clicking of a Start Client button.
        /// </summary>
        /// <param name="sender">The parameter is not used.</param>
        /// <param name="e">The parameter is not used.</param>
        private void StartClientClick(object sender, RoutedEventArgs e)
        {
            if (this.Client == null)
            {
                this.OutputLine("Starting client.");
                this.Client = new Client(MainPage.ApplicationNameClient, this.PinReady);

                this.buttonClient.Content = MainPage.StopClient;

                // Disable the server if the client is running.
                this.buttonStartServer.IsEnabled = false;
            }
            else
            {
                this.OutputLine("Stopping client.");
                this.Client.Stop();
                this.Client = null;

                this.buttonClient.Content = MainPage.StartClient;

                // Enable the server if the client is stopped.
                this.buttonStartServer.IsEnabled = true;
                this.OutputLine("====================");
            }
        }

        /// <summary>
        /// The handler for the clicking of a Send Pin button.
        /// </summary>
        /// <param name="sender">The parameter is not used.</param>
        /// <param name="e">The parameter is not used.</param>
        private void SendPinClick(object sender, RoutedEventArgs e)
        {
            string pin = textBoxPinClient.Text;

            if (!string.IsNullOrEmpty(pin) && null != this.Client)
            {
                this.Client.Pin = pin.Trim();
                textBoxPinClient.Text = string.Empty;
                this.PinReady.Set();
            }
        }

        /// <summary>
        /// Container for the argument passed to the dispatcher which prints to UI.
        /// </summary>
        private class ArgumentObject
        {
            /// <summary>
            /// Initializes a new instance of the ArgumentObject class.
            /// </summary>
            /// <param name="text">The text to output to the text box.</param>
            /// <param name="box">The text box to send the text to.</param>
            /// <param name="scroll">The ScrollViewer for the text box. May be null.</param>
            public ArgumentObject(string text, TextBox box, ScrollViewer scroll = null)
            {
                this.TextBox = box;
                this.ScrollBar = scroll;
                this.Text = text;
            }

            /// <summary>
            /// Gets or sets the string text.
            /// </summary>
            private string Text { get; set; }

            /// <summary>
            /// Gets or sets the text box which display the text.
            /// </summary>
            private TextBox TextBox { get; set; }

            /// <summary>
            /// Gets or sets the ScrollViewer associated with the text box.
            /// </summary>
            private ScrollViewer ScrollBar { get; set; }

            /// <summary>
            /// Used by the dispacter when sending output to the UI's textbox.
            /// </summary>
            public void OnDispatched()
            {
                this.TextBox.Text += this.Text;

                if (null != this.ScrollBar)
                {
                    this.ScrollBar.ScrollToVerticalOffset(this.ScrollBar.ExtentHeight);
                }
            }
        }

        /// <summary>
        /// Container for the argument passed to the dispatcher which is put it in the
        /// "PIN to use" text box.
        /// </summary>
        private class PinTransfer
        {
            /// <summary>
            /// Gets or sets the pin.
            /// </summary>
            public string Pin { get; set; }

            /// <summary>
            /// Gets or sets the TextBox to output the pin to.
            /// </summary>
            public TextBox TextBox { get; set; }

            /// <summary>
            /// Outputs the pin to the given text box when called by the dispatcher.
            /// </summary>
            public void OnDispatched()
            {
                this.TextBox.Text = "Pin: " + this.Pin;
            }
        }
    }
}
