// ****************************************************************************
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
// ******************************************************************************

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using System.Threading;
using qcc.winrt;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace SocketStress
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        const int Clients = 8;
        const int Servers = 4;
        const int PatternLength = 256 * 100; // send a few mtus
        const int Port = 9922;
        const int MaxSendBuffer = 122880;
        const string Address = "127.0.0.1";

        Task serverTaskMain;
        Task[] _serverTasks = new Task[Servers];
        Task[] _clientTasks = new Task[Clients];
        int _successCount = 0;
        int _failCount = 0;
        volatile int serversRunning = 0;
        volatile int clientsRunning = 0;

        public MainPage()
        {
            this.InitializeComponent();
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            if (serversRunning != 0)
            {
                return;
            }

            Interlocked.Increment(ref serversRunning);

            // Server start
            serverTaskMain = new Task(() =>
                {
                    // Server setup
                    SocketWrapper[] listenSocketArr = new SocketWrapper[1];
                    uint q = SocketsWrapper.Socket(AddressFamily.QCC_AF_INET, SocketType.QCC_SOCK_STREAM, listenSocketArr);
                    SocketWrapper listenSocket = listenSocketArr[0];

                    q = listenSocket.Bind("0.0.0.0", Port);
                    q = listenSocket.Listen(10);

                    for (int s = 0; s < Servers; s++)
                    {
                        if (s != 0)
                        {
                            Interlocked.Increment(ref serversRunning);
                        }
                        _serverTasks[s] = new Task(() =>
                            {
                                string[] remoteAddr = new string[1];
                                int[] remotePort = new int[1];
                                SocketWrapper[] newSocketArr = new SocketWrapper[1];

                                while (listenSocket.Accept(remoteAddr, remotePort, newSocketArr) == 0)
                                {
                                    int bytesLeft = PatternLength;
                                    int byteCount = 0;
                                    using (SocketWrapper newSocket = newSocketArr[0])
                                    {
                                        int[] sentArr = new int[1];
                                        while (bytesLeft != 0)
                                        {
                                            byte[] tempBytes = new byte[Math.Min(bytesLeft, MaxSendBuffer)];
                                            for (int i = 0; i < tempBytes.Length; i++)
                                            {
                                                tempBytes[i] = (byte)((i + byteCount) % 256);
                                            }
                                            q = newSocket.Send(tempBytes, tempBytes.Length, sentArr);
                                            if (q != 0)
                                            {                                                
                                                break;
                                            }
                                            byteCount += sentArr[0];
                                            bytesLeft -= sentArr[0];
                                        }
                                    }
                                }

                                Interlocked.Decrement(ref serversRunning);
                            });
                        _serverTasks[s].Start();
                    }
                });
            serverTaskMain.Start();
        }

        private void Button_Click_2(object sender, RoutedEventArgs e)
        {
            if (clientsRunning != 0)
            {
                return;
            }

            Interlocked.Increment(ref clientsRunning);

            // Client start
            for (int c = 0; c < Clients; c++)
            {
                if (c != 0)
                {
                    Interlocked.Increment(ref clientsRunning);
                }

                _clientTasks[c] = new Task(() =>
                {
                    while (true)
                    {
                        SocketWrapper[] clientSocketArr = new SocketWrapper[1];
                        SocketsWrapper.Socket(AddressFamily.QCC_AF_INET, SocketType.QCC_SOCK_STREAM, clientSocketArr);
                        using (SocketWrapper clientSocket = clientSocketArr[0])
                        {
                            uint q = clientSocket.Connect(Address, Port);
                            byte[] tempBytes = new byte[PatternLength];
                            int[] receivedArr = new int[1];

                            int bytesLeft = tempBytes.Length;
                            int byteCount = 0;
                            bool success = true;
                            do
                            {
                                q = clientSocket.Recv(tempBytes, tempBytes.Length, receivedArr);
                                if (q != 0)
                                {
                                    success = false;
                                    break;
                                }

                                for (int i = 0; i < receivedArr[0]; i++)
                                {
                                    if (tempBytes[i] != ((i + byteCount) % 256))
                                    {
                                        success = false;
                                        break;
                                    }
                                }

                                byteCount += receivedArr[0];
                                bytesLeft -= receivedArr[0];
                            }
                            while (bytesLeft > 0);

                            // Update the count
                            Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                                {
                                    if (success)
                                    {
                                        successCount.Text = (++_successCount).ToString();
                                    }
                                    else
                                    {
                                        failCount.Text = (++_failCount).ToString();
                                    }
                                }); 
                        }
                    }

                    Interlocked.Decrement(ref clientsRunning);
                });
                _clientTasks[c].Start();
            }
        }
    }
}
