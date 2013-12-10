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
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using System.Threading.Tasks;

using AllJoyn;
using AllJoynStreaming;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace PlayerApp
{
    public class Listeners
    {
        public Listeners(BusAttachment bus, MainPage host)
        {
            _hostPage = host;
            _busListeners = new BusListener(bus);
            _busListeners.BusDisconnected += new BusListenerBusDisconnectedHandler(BusListenerBusDisconnected);
            _busListeners.BusStopping += new BusListenerBusStoppingHandler(BusListenerBusStopping);
            _busListeners.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(BusListenerFoundAdvertisedName);
            _busListeners.ListenerRegistered += new BusListenerListenerRegisteredHandler(BusListenerListenerRegistered);
            _busListeners.ListenerUnregistered += new BusListenerListenerUnregisteredHandler(BusListenerListenerUnregistered);
            _busListeners.LostAdvertisedName += new BusListenerLostAdvertisedNameHandler(BusListenerLostAdvertisedName);
            _busListeners.NameOwnerChanged += new BusListenerNameOwnerChangedHandler(BusListenerNameOwnerChanged);

            _sessionPortListener = new SessionPortListener(bus);
            _sessionPortListener.AcceptSessionJoiner += new SessionPortListenerAcceptSessionJoinerHandler(SessionPortListenerAcceptSessionJoiner);
            _sessionPortListener.SessionJoined += new SessionPortListenerSessionJoinedHandler(SessionPortListenerSessionJoined);

            _sessionListener = new SessionListener(bus);
            _sessionListener.SessionLost += new SessionListenerSessionLostHandler(SessionListenerSessionLost);
            _sessionListener.SessionMemberAdded += new SessionListenerSessionMemberAddedHandler(SessionListenerSessionMemberAdded);
            _sessionListener.SessionMemberRemoved += new SessionListenerSessionMemberRemovedHandler(SessionListenerSessionMemberRemoved);
        }

        void SessionListenerSessionMemberRemoved(uint sessionId, string member)
        {
            int i = 0;
            i++;
        }

        void SessionListenerSessionMemberAdded(uint sessionId, string member)
        {
            int i = 0;
            i++;
        }

        void SessionListenerSessionLost(uint sessionId)
        {
        }

        void BusListenerNameOwnerChanged(string busName, string oldOwner, string newOwner)
        {
            int i = 0;
            i++;
        }

        void BusListenerLostAdvertisedName(string wkn, TransportMaskType transport, string namePrefix)
        {
        }

        void BusListenerListenerUnregistered()
        {
            int i = 0;
            i++;
        }

        void BusListenerListenerRegistered(BusAttachment bus)
        {
            int i = 0;
            i++;
        }

        void BusListenerFoundAdvertisedName(string wkn, TransportMaskType transport, string namePrefix)
        {
            if (namePrefix == MainPage.MEDIA_SERVER_NAME)
            {
                _hostPage.OnFoundMediaServer();
            }
        }

        void BusListenerBusStopping()
        {
            int i = 0;
            i++;
        }

        void BusListenerBusDisconnected()
        {
            int i = 0;
            i++;
        }

        private void SessionPortListenerSessionJoined(ushort sessionPort, uint sessionId, string joiner)
        {
        }

        private bool SessionPortListenerAcceptSessionJoiner(ushort sessionPort, string joiner, SessionOpts sessionOpts)
        {
            return true;
        }

        public static implicit operator BusListener(Listeners listeners)
        {
            return listeners._busListeners;
        }

        public static implicit operator SessionPortListener(Listeners listeners)
        {
            return listeners._sessionPortListener;
        }

        public static implicit operator SessionListener(Listeners listeners)
        {
            return listeners._sessionListener;
        }

        SessionListener _sessionListener;
        SessionPortListener _sessionPortListener;
        BusListener _busListeners;
        MainPage _hostPage;
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
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
            if(!_initialized) {
                InitializeAllJoyn();
                _initialized = true;
            }
        }

        private void InitializeAllJoyn()
        {
            Task _t1 = new Task(() =>
            {
                Debug.UseOSLogging(true);
                //Debug.SetDebugLevel("ALLJOYN", 7);

                string connectSpec = "null:";

                try
                {
                    _bus = new BusAttachment(APPLICATION_NAME, true, 4);
                }
                catch (Exception ex)
                {
                    QStatus stat = AllJoynException.GetErrorCode(ex.HResult);
                }
                _bus.Start();

                _bus.ConnectAsync(connectSpec).AsTask().Wait();

                _listeners = new Listeners(_bus, this);
                _bus.RegisterBusListener(_listeners);

                _mediaSink = new MediaSink(_bus);
                _mediaRender = new MediaRenderer();
                _mediaRender.OnOpen += OnOpen;
                _mediaRender.OnPlay += OnPlay;
                _mediaRender.OnPause += OnPause;
                _mediaRender.OnClose += OnClose;

                _bus.FindAdvertisedName(MEDIA_SERVER_NAME);
            });
            _t1.Start();
        }

        public async void OnFoundMediaServer()
        {
            if (_sessionId == 0)
            {
                /*
                 * Join the session - if we need a specific qos we would check it here.
                 */
                SessionOpts opts = new SessionOpts(TrafficType.TRAFFIC_MESSAGES, true, ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY);
                SessionOpts[] opts_out = new SessionOpts[1];
                JoinSessionResult result = await _bus.JoinSessionAsync(MEDIA_SERVER_NAME, SESSION_PORT, _listeners, opts, opts_out, null);
                _sessionId = result.SessionId;
                ConnectToServer();
            }
        }

        async void ConnectToServer()
        {
            try
            {
                _mediaSink.ConnectSourceAsync(MEDIA_SERVER_NAME, _sessionId).AsTask().Wait();
                ListStreamResult result = await _mediaSink.ListStreamsAsync();
                MediaDescription[] streams = result.Streams;
                for (int i = 0; i < streams.Length; i++)
                {
                    switch (streams[i].MType)
                    {
                        case MediaType.AUDIO:
                            break;

                        case MediaType.VIDEO:
                            break;

                        case MediaType.IMAGE:
                            break;

                        case MediaType.APPLICATION:
                        case MediaType.TEXT:
                        case MediaType.OTHER:
                            break;
                    }

                    if (mimeType == streams[i].MimeType)
                    {
                        _selectedStream = i;
                        break;
                    }
                }

                // Found a valid stream
                if (_selectedStream != -1)
                {
                    string streamName = streams[_selectedStream].StreamName;
                    _mediaSink.OpenStreamAsync(streamName, _mediaRender).AsTask().Wait();

                    if (_stress)
                    {
                        StressLoop();
                    }
                    else
                    {
                        _mediaSink.PlayAsync().AsTask().Wait();
                        Task.Delay(1000000).AsAsyncAction().AsTask().Wait();
                        _mediaSink.PauseAsync(_drain).AsTask().Wait();
                        Task.Delay(4).AsAsyncAction().AsTask().Wait(); ;
                        _mediaSink.PlayAsync().AsTask().Wait();
                        Task.Delay(1000000).AsAsyncAction().AsTask().Wait();
                    }

                    _mediaSink.CloseStreamAsync(streamName).AsTask().Wait();
                }
            }
            catch (Exception e)
            {
                var m = AllJoynException.GetErrorMessage(e.HResult);
                var h = AllJoynException.GetErrorCode(e.HResult);
            }
        }

        void OnOpen(MediaSink mediaSink, MediaDescription description, SocketStream sock)
        {
            _sockStream = sock;
            if (_useHttp)
            {
                if (_httpStreamer != null)
                {
                    _httpStreamer.Stop();
                }
                _httpStreamer = new MediaHTTPStreamer(_mediaSink);
                _httpStreamer.Start(sock, description.MimeType, (UInt32)description.Size, HTTP_PORT);
            } else {
                _running = true;
                RxLoop();
            }
        }

        void OnClose(SocketStream sock) 
        { 
            if (_httpStreamer != null) { 
                _httpStreamer.Stop();
                _httpStreamer = null;
            } else {
                _running = false;
            }

            // TODO Close file
        }
        
        void OnPause(SocketStream sock) 
        {} 
        
        void OnPlay(SocketStream sock)
        {
            _rxCount = 0;
        }

        private void StressLoop()
        { 
            var random = new Random(); 
            while (true) {
                uint cmd = (uint)random.Next()%64; 
                /*if (cmd < 2) { 
                    if (cmd == 0) { 
                        _bus.LeaveSession(_sessionId);  
                        _sessionId = 0;  
                    }   
                    break; 
                } */
                switch (cmd % 5) { 
                    case 0: 
                        _mediaSink.PlayAsync().AsTask().Wait();
                        continue;           
                    case 1:
                        _mediaSink.PauseAsync(_drain).AsTask().Wait();
                        continue; 
                    case 2:
                        _mediaSink.SeekRelativeAsync(15000, (byte)MediaSeekUnits.MILLISECONDS).AsTask().Wait();
                        continue; 
                    case 3:
                        _mediaSink.SeekRelativeAsync(-15000, (byte)MediaSeekUnits.MILLISECONDS).AsTask().Wait();
                        continue; 
                    case 4:
                        _mediaSink.SeekAbsoluteAsync(0, (byte)MediaSeekUnits.MILLISECONDS).AsTask().Wait();
                        continue;
                    default: 
                        continue; 
                } 
            } 
        }

        private void RxLoop()
        {
            Task rxLoopTask = new Task(() =>
            {
                try
                {
                    while (_running)
                    {
                        byte[] buf = new byte[2048];
                        int[] received = new int[1];
                        // sockstream is blocking
                        _sockStream.Recv(buf, buf.Length, received);
                        if (received[0] >= 0)
                        {
                            _rxCount += received[0];
                            // TODO write to file
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                catch (Exception e)
                {
                    var m = AllJoynException.GetErrorMessage(e.HResult);
                    var h = AllJoynException.GetErrorCode(e.HResult);
                }
            });

            rxLoopTask.Start();
        }

        private void Button_Click_Start(object sender, RoutedEventArgs e)
        {
            if (_httpStreamer == null)
            {
                // The HTTP Streamer is not running yet, wait ....
                return;
            }
            try
            {
                if (HttpStreamerBtn.Content.Equals("Start"))
                {
                    if (!_mediaPathSet)
                    {
                        HttpMediaElement.Source = new Uri("http://localhost:" + HTTP_PORT + "/1.mp3");
                        _mediaPathSet = true;
                    }
                    HttpMediaElement.Play();
                    HttpStreamerBtn.Content = "Stop";
                }
                else
                {
                    HttpMediaElement.Stop();
                    HttpStreamerBtn.Content = "Start";
                }
            }
            catch (Exception ex)
            {
                var m = ex.Message;
            }
        }

        bool _initialized = false;
        BusAttachment _bus;
        public static string APPLICATION_NAME = "Player";
        public static string MEDIA_SERVER_NAME = "org.alljoyn.Media.Server";
        public static ushort SESSION_PORT = 123;
        public static string mimeType = "audio/mpeg";
        public static ushort HTTP_PORT = 9998;
        UInt32 _sessionId = 0;
        Listeners _listeners;
        MediaSink _mediaSink;
        MediaRenderer _mediaRender;
        MediaHTTPStreamer _httpStreamer;
        SocketStream _sockStream = null;
        int _selectedStream = -1;
        bool _stress = false;
        bool _useHttp = true;
        bool _drain = false;
        bool _running = false;
        int _rxCount = 0;
        bool _mediaPathSet = false;
    }
}
