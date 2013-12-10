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
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Storage;
using Windows.Storage.FileProperties;
using Windows.Storage.Streams;
using Windows.Storage.Pickers;
using AllJoyn;
using AllJoynStreaming;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace ServerApp
{
    class AudioStream
    {
        public AudioStream(BusAttachment bus, string name, MP3Reader mp3Reader, UInt32 jitter, UInt32 prefill)
        {
            _mediaStream = new MediaStream(bus, name, mp3Reader.GetDescription());
            _mediaPacer = new MediaPacer(mp3Reader.GetDescription(), jitter, 0);
            _mp3Reader = mp3Reader;
            _prefill = prefill;

            _mediaStream.OnOpen += OnOpen;
            _mediaStream.OnClose += OnClose;
            _mediaStream.OnPlay += OnPlay;
            _mediaStream.OnPause += OnPause;
            _mediaStream.OnSeekRelative += OnSeekRelative;
            _mediaStream.OnSeekAbsolute += OnSeekAbsolute;

            _mediaPacer.JitterMiss += JitterMiss;
            _mediaPacer.RequestFrames += RequestFrames;
            _buffer = new byte[_frameReadLength * _mp3Reader.FrameLen];

        }

        bool OnOpen(SocketStream sock)
        {
            _startTime = 0;
            _timeStamp = _startTime;
            _sinkSock = sock;
            return true;
        }

        void OnClose()
        {
            _mediaPacer.Stop();
        }

        bool OnPlay()
        {
            try {
                if (!_mediaPacer.IsRunning())
                {
                    _mediaPacer.Start(_sinkSock, _timeStamp, _prefill);
                    return true;
                }
            } catch (Exception e) {
            }
            return false;
        }

        bool OnPause()
        {
            try {
                if (_mediaPacer.IsRunning())
                {
                    _mediaPacer.Stop();
                    return true;
                }
            }
            catch (Exception e)
            {
            }
            return true;
        }

        bool OnSeekRelative(Int32 offset, MediaSeekUnits units)
        {
            try
            {
                if (_mediaPacer.IsRunning())
                {
                    _mediaPacer.Stop();
                }
                _mp3Reader.SetPosRelative(offset, units);
                _timeStamp = _startTime + _mp3Reader.Timestamp;
                _mediaPacer.Start(_sinkSock, _timeStamp, _prefill);
                return true;
            }
            catch (Exception e)
            {
            }
            return false;
        }

        bool OnSeekAbsolute(UInt32 position, MediaSeekUnits units)
        {
            try
            {
                if (_mediaPacer.IsRunning())
                {
                    _mediaPacer.Stop();
                }
                _mp3Reader.SetPosAbsolute(position, units);
                _timeStamp = _startTime + _mp3Reader.Timestamp;
                _mediaPacer.Start(_sinkSock, _startTime, _prefill);
            }
            catch (Exception e)
            {
            }
            return true;
        }

        void JitterMiss(UInt32 timestamp, SocketStream sock, UInt32 jitter)
        {
        }

        void RequestFrames(UInt32 timestamp, SocketStream sock, UInt32 maxFrames, UInt32[] gotFrames)
        {
            uint count = 0;
            try 
            {
                for (count = 0; count < maxFrames; count++)
                {
                    IBuffer buffer = WindowsRuntimeBufferExtensions.AsBuffer(_buffer, 0, (Int32)(_mp3Reader.FrameLen));
                    Int32 len = (Int32)_mp3Reader.ReadFrames(buffer, 1);
                    byte[] byteArray = buffer.ToArray();
                    while (len > 0)
                    {
                        Int32[] sent = new Int32[1];
                        sock.Send(byteArray, (Int32)len, sent);
                        len -= sent[0];
                        byte[] byteArray2 = new byte[len];
                        Array.Copy(byteArray, sent[0], byteArray2, 0, len);
                        byteArray = byteArray2;
                    }
                }
            } catch {
                // TODO
            }
            gotFrames[0] = count;
        }

        public static implicit operator MediaStream(AudioStream stream)
        {
            return stream._mediaStream;
        }

        public static implicit operator MediaPacer(AudioStream stream)
        {
            return stream._mediaPacer;
        }

        MP3Reader _mp3Reader;
        MediaStream _mediaStream;
        MediaPacer _mediaPacer;
        UInt32 _prefill;
        SocketStream _sinkSock;
        UInt32 _startTime;
        UInt32 _timeStamp;
        byte[] _buffer;
        Int32 _frameReadLength = 4;

    }

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

        bool _initialized = false;
        bool _connected = false;
        BusAttachment _bus;
        Listeners _listeners;
        string APPLICATION_NAME = "MediaServerApp";
        string MediaServerName = "org.alljoyn.Media.Server"; 
        UInt16 SESSION_PORT = 123;
        MediaSource _mediaSource;
        MP3Reader _mp3Reader;
        AudioStream _audioStream;
        StorageFile _streamingSong;
        MusicProperties _streamingSongMusicProperties;
        BasicProperties _streamingSongBasicProperties;

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

        async void InitializeAllJoyn(){
            Debug.UseOSLogging(true);
            Debug.SetDebugLevel("ALLJOYN", 7);

            _bus = new BusAttachment(APPLICATION_NAME, true, 4);
            string connectSpec = "null:";
            _bus.Start();

            try
            {
                _mp3Reader = new MP3Reader();

                if (_streamingSong != null)
                {
                    _streamingSongBasicProperties = await _streamingSong.GetBasicPropertiesAsync();
                    if (_streamingSongBasicProperties != null)
                    {
                        _streamingSongMusicProperties = await _streamingSong.Properties.GetMusicPropertiesAsync();
                        if (_streamingSongMusicProperties != null)
                        {
                            await _mp3Reader.SetFileAsync(_streamingSong);

                            _bus.ConnectAsync(connectSpec).AsTask().Wait();
                            _connected = true;

                            _listeners = new Listeners(_bus, this);
                            _bus.RegisterBusListener(_listeners);
                            _mediaSource = new MediaSource(_bus);
                            _audioStream = new AudioStream(_bus, "mp3", _mp3Reader, 100, 1000);
                            _mediaSource.AddStream(_audioStream);

                            /* Register MediaServer bus object */ 
                            _bus.RegisterBusObject(_mediaSource.MediaSourceBusObject); 
                             /* Request a well known name */ 
                            _bus.RequestName(MediaServerName, (int)(RequestNameType.DBUS_NAME_REPLACE_EXISTING | RequestNameType.DBUS_NAME_DO_NOT_QUEUE));

                            /* Advertise name */
                            _bus.AdvertiseName(MediaServerName, TransportMaskType.TRANSPORT_ANY);
   
                            /* Bind a session for incoming client connections */
                            SessionOpts opts = new SessionOpts(TrafficType.TRAFFIC_MESSAGES, true, ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY);
                            ushort[] portOut = new ushort[1];
                            _bus.BindSessionPort(SESSION_PORT, portOut, opts, _listeners);
                        }
                    }
                }
            } catch (Exception ex)
            {
                string message = ex.Message;
                QStatus status = AllJoynException.GetErrorCode(ex.HResult);
                string errMsg = AllJoynException.GetErrorMessage(ex.HResult);
            }
        }

        private async void OpenPicker()
        {
            FileOpenPicker picker = new FileOpenPicker();
            picker.FileTypeFilter.Add(".mp3");
            picker.SuggestedStartLocation = PickerLocationId.MusicLibrary;
            _streamingSong = await picker.PickSingleFileAsync();
        }

        private void Button_Click_Pick(object sender, RoutedEventArgs e)
        {
            OpenPicker();
        }


        private void Button_Click_Start(object sender, RoutedEventArgs e)
        {
            if (!_initialized)
            {
                Task _initTask = new Task(() =>
                {
                    InitializeAllJoyn();
                });
                _initTask.Start();
            }
        }

    }
}
