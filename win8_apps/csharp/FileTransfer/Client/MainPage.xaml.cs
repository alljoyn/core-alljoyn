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

namespace FileTransferClient
{
    using AllJoyn;
    using FileTransferClient.Common;
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Windows.Foundation;
    using Windows.Storage;
    using Windows.Storage.Pickers;
    using Windows.Storage.Streams;
    using Windows.UI.Core;
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Controls;
    using Windows.UI.Xaml.Navigation;

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private FolderPicker Folder { get; set; }
        /// <summary>
        /// The folder where the user specifies the file to be written
        /// </summary>
        private StorageFolder destFolder;

        /// <summary>
        /// Name of the file tranferred from the service
        /// </summary>
        private string fileName = string.Empty;

        /// <summary>
        /// The bus object that handles the file transfer.
        /// </summary>
        private FileTransferBusObject BusObject { get; set; }

        /// <summary>
        /// Initializes a new instance of the MainPage class.
        /// </summary>
        public MainPage()
        {
            this.InitializeComponent();
            // Hook the application suspending/resuming events to the handlers so that exclusively-held resource (eg., network port) is release and other applications are allowed to acquire the resource
            Application.Current.Suspending += new Windows.UI.Xaml.SuspendingEventHandler(App_Suspending);
            Application.Current.Resuming += new EventHandler<Object>(App_Resuming);
        }

        private void App_Suspending(Object sender, Windows.ApplicationModel.SuspendingEventArgs e)
        {
            App app = Application.Current as App;
            try
            {
                if (app.Bus != null)
                {
                    app.Bus.OnAppSuspend();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Call OnAppSuspend() failed. Reason: " + AllJoynException.GetErrorMessage(ex.HResult));
            }
        }

        private void App_Resuming(Object sender, Object e)
        {
            App app = Application.Current as App;
            try
            {
                if (app.Bus != null)
                {
                    app.Bus.OnAppResume();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Call OnAppResume() failed. Reason: " + AllJoynException.GetErrorMessage(ex.HResult));
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the class has been initialized or not.
        /// </summary>
        private bool Initialized { get; set; }

        /// <summary>
        /// Outputs 'msg' to the text box in the UI of the application
        /// </summary>
        /// <param name="msg">Message that will be output in the text box of the UI</param>
        /// <param name="completed">A value to indicate the file transfer has been completed.</param>
        public void OutputLine(string msg, bool completed = false)
        {
            if (completed)
            {
                this.ButtonRunClient.IsEnabled = true;
                this.BusObject = null;
            }

            ArgumentObject ao = new ArgumentObject();
            ao.TextBox = this.TextBlockClient;
            ao.Text = msg + "\n";
            Dispatcher.RunAsync(
                CoreDispatcherPriority.Normal,
                new DispatchedHandler(ao.OnDispatched)).AsTask();
            System.Diagnostics.Debug.WriteLine(msg);
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            if (!this.Initialized)
            {
                // The user interface page is now active so let the application know.
                App app = Application.Current as App;

                app.UiPage = this;

                // This logging is useful for debugging purposes but should not be
                // used for release versions. The output will be in the file "alljoyn.log" in
                // current users Documents directory.
                //AllJoyn.Debug.UseOSLogging(true);
                //AllJoyn.Debug.SetDebugLevel("TCP", 7);
                //AllJoyn.Debug.SetDebugLevel("ALLJOYN", 7);
                //AllJoyn.Debug.SetDebugLevel("ALLJOYN_OBJ", 7);
                //AllJoyn.Debug.SetDebugLevel("NETWORK", 7);

                // Initialize per-page application callbacks.
                app.Listeners.FoundAdvertisedName += this.Listeners_FoundAdvertisedName;
                app.Listeners.LostAdvertisedName += this.Listeners_LostAdvertisedName;

                this.Initialized = true;
            }
        }

        /// <summary>
        /// Handler for the FoundAdvertisedName event.
        /// </summary>
        /// <param name="name">A well known name that the remote bus is advertising.</param>
        /// <param name="transport">Transport that received the advertisement.</param>
        /// <param name="namePrefix">The well-known name prefix used in call to FindAdvertisedName
        /// that triggered this event.</param>
        private void Listeners_FoundAdvertisedName(string name, TransportMaskType transport, string namePrefix)
        {
            this.ButtonRunClient.IsEnabled = true;
            this.OutputLine("Found advertised name and enabled Run button.");
        }

        /// <summary>
        /// Handler for the LostAdvertisedName event.
        /// </summary>
        /// <param name="name">A well known name that the remote bus is advertising.</param>
        /// <param name="transport">Transport that received the advertisement.</param>
        /// <param name="namePrefix">The well-known name prefix used in call to FindAdvertisedName
        /// that triggered this event.</param>
        private void Listeners_LostAdvertisedName(string name, TransportMaskType transport, string namePrefix)
        {
            this.ButtonRunClient.IsEnabled = false;
            this.BusObject = null;
            this.OutputLine("Lost advertised name and disabled Run button.");
        }

        /// <summary>
        /// Gets the destination folder for the file to be received, create the interface and start the
        /// transfer.
        /// </summary>
        /// <param name="sender">UI control which signaled the click event</param>
        /// <param name="e">arguments associated with the click event</param>
        private async void Button_RunClick(object sender, RoutedEventArgs e)
        {
            // get the destination folder for the file
            if (null == this.Folder)
            {
                this.Folder = new FolderPicker();
                this.Folder.ViewMode = PickerViewMode.List;
                this.Folder.SuggestedStartLocation = PickerLocationId.DocumentsLibrary;
                this.Folder.FileTypeFilter.Add("*");
            }

            this.destFolder = await this.Folder.PickSingleFolderAsync();

            App app = Application.Current as App;

            if (null == this.BusObject)
            {
                this.BusObject = new FileTransferBusObject(app.Bus);
            }

            if (null != this.BusObject && this.BusObject.StartJoinSessionTask(this.destFolder))
            {
                this.ButtonRunClient.IsEnabled = false;
            }

            this.OutputLine("Starting transfer.");
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
