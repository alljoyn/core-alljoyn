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

namespace BusStress
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using BusStress.Common;
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
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        /// <summary>
        /// Primary object which executes the the stress operation threads
        /// </summary>
        private StressManager stressManager;

        /// <summary>
        /// Initializes a new instance of the <see cref="MainPage"/> class.
        /// </summary>
        public MainPage()
        {
            this.InitializeComponent();

            StressType[] stressOpts = { StressType.Default, StressType.Service, StressType.Client };
            StressTypeComboBox.DataContext = stressOpts;
            StressTypeComboBox.SelectedIndex = 0;

            this.stressManager = new StressManager(this);
            this.stressManager.CurrentlyRunning = false;
        }

        /// <summary>
        /// Output a message to the UI
        /// </summary>
        /// <param name="msg">Message to be output to the user</param>
        public async void Output(string msg)
        {
            await Dispatcher.RunAsync(
                CoreDispatcherPriority.Normal,
                () =>
                {
                    output.Text += msg + "\n";
                    this.scrollBar.ScrollToVerticalOffset(this.scrollBar.ExtentHeight);
                });
            System.Diagnostics.Debug.WriteLine(msg);
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
        }

        /// <summary>
        /// Starts the application with the input provided by the user.
        /// </summary>
        /// <param name="sender">Object which initiated the event signal.</param>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        private void Start_Click(object sender, RoutedEventArgs e)
        {
            if (!this.stressManager.CurrentlyRunning)
            {
                this.stressManager.CurrentlyRunning = true;

                // Set OS Logging to true so debug prints are written to the output window of the
                // Visual Studio Debugger, or set to false to have debug prints written to an
                // alljoyn.log file in Documents Library
                AllJoyn.Debug.UseOSLogging(false);
                AllJoyn.Debug.SetDebugLevel("ALLJOYN", 7);
                AllJoyn.Debug.SetDebugLevel("ALLJOYN_OBJ", 7);

                // Get the input from the user
                ArgumentObject argObj = new ArgumentObject();
                argObj.NumOfIterations = (uint)IterationSlider.Value;
                argObj.NumOfTasks = (uint)TreadSlider.Value;
                argObj.StopThreadBeforeJoin = StopTreadsSwitch.IsOn;
                argObj.DeleteBusAttachments = DeleteBusSwitch.IsOn;
                argObj.IsMultipoint = MultipointSwitch.IsOn;
                argObj.StressOperation = (StressType)StressTypeComboBox.SelectedValue;

                if (!argObj.DeleteBusAttachments)
                {
                    argObj.NumOfIterations = 1;
                }

                this.stressManager.RunStressOperations(argObj);
            }
        }
    }
}
