

var bus;
var mediaSink;
var mediaRender;
var httpStreamer;

var APPLICATION_NAME = "Player";
var MEDIA_SERVER_NAME = "org.alljoyn.Media.Server";
var SESSION_PORT = 123;
var mimeType = "audio/mpeg";
var HTTP_PORT = 9998;
var sessionId = 0;
var streamOpend = false;
var started = false;
var paused = false;

function GetEventArg(evt, index) {
    if (index == 0)
        return evt.target;
    else if (index > 0)
        return evt.detail[(index - 1)];
    else
        return nullptr;
}

var ConnectToServer = function () {
    mediaSink.connectSourceAsync(MEDIA_SERVER_NAME, sessionId).then(function () {
        mediaSink.listStreamsAsync().then(function (result) {
            var streams = result.streams;
            for (var i = 0; i < streams.length; i++) {
                switch (streams[i].mtype) {
                    case AllJoynStreaming.MediaType.audio:
                        break;

                    case AllJoynStreaming.MediaType.video:
                        break;

                    case AllJoynStreaming.MediaType.image:
                        break;

                    case AllJoynStreaming.MediaType.application:
                    case AllJoynStreaming.MediaType.text:
                    case AllJoynStreaming.MediaType.other:
                        break;
                }

                if (mimeType == streams[i].mimeType) {
                    _selectedStream = i;
                    break;
                }
            }

            // Found a valid stream
            if (_selectedStream != -1) {
                var streamName = streams[_selectedStream].streamName;
                mediaSink.openStreamAsync(streamName, mediaRender).then(function () {
                    streamOpend = true;
                });
            }
        });
    });
}

function Listeners(bus) {
    this._sListener = new AllJoyn.SessionListener(bus);
    this._spListener = new AllJoyn.SessionPortListener(bus);
    this._bl = new AllJoyn.BusListener(bus);

    this.BusListenerBusDisconnected = function () {
        var i = 0;
    }

    this.BusListenerBusStopping = function () {
        var i = 0;
    }

    this.BusListenerListenerRegistered = function (evt) {
    }

    this.BusListenerListenerUnregistered = function () {
        var i = 0;
    }

    this.BusListenerFoundAdvertisedName = function (evt) {
        var name = GetEventArg(evt, 0);
        var transport = GetEventArg(evt, 1);
        var namePrefix = GetEventArg(evt, 2);

        if (sessionId == 0 && namePrefix == MEDIA_SERVER_NAME) {
            var opts = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES, true, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY);
            var opts_out = new Array(1);
            bus.enableConcurrentCallbacks();
            bus.joinSessionAsync(MEDIA_SERVER_NAME, SESSION_PORT, listeners._sListener, opts, opts_out, null).then(function (result) {
                if (result.status == 0) {
                    sessionId = result.sessionId;
                    ConnectToServer();
                }
            });
        }
    }

    this.BusListenerLostAdvertisedName = function (evt) {
        var name = GetEventArg(evt, 0);
        var previousOwner = GetEventArg(evt, 1);
        var newOwner = GetEventArg(evt, 1);

    }

    this.BusListenerNameOwnerChanged = function (evt) {
        var name = GetEventArg(evt, 0);
        var previousOwner = GetEventArg(evt, 1);
        var newOwner = GetEventArg(evt, 2);
    };

    this.BusListenerListenerUnregistered = function () {
        var i = 0;
    }

    this.SessionPortListenerSessionJoined = function (evt) {
        var sessionPort = GetEventArg(evt, 0);
        if (alljoyn.sessionId == 0) {
            alljoyn.sessionId = GetEventArg(evt, 1);
        }
        var joiner = GetEventArg(evt, 2);
    }

    this.SessionPortListenerAcceptSessionJoiner = function (evt) {
        var sessionPort = GetEventArg(evt, 0);
        var sessionId = GetEventArg(evt, 1);
        var joiner = GetEventArg(evt, 2);
        if (sessionPort == CONTACT_PORT) {
            return true;
        } else {
            return false;
        }
    }

    this.SessionListenerSessionLost = function (evt) {
        var sessionId = GetEventArg(evt, 0);
    };

    this.SessionListenerSessionMemberAdded = function (evt) {
        var sessionId = GetEventArg(evt, 0);
        var uniqueName = GetEventArg(evt, 1);
    };

    this.SessionListenerSessionMemberRemoved = function (evt) {
        var sessionId = GetEventArg(evt, 0);
        var uniqueName = GetEventArg(evt, 1);
    };

    this._bl.addEventListener('busdisconnected', this.BusListenerBusDisconnected);
    this._bl.addEventListener('busstopping', this.BusListenerBusStopping);

    this._bl.addEventListener('listenerregistered', this.BusListenerListenerRegistered);
    this._bl.addEventListener('listenerunregistered', this.BusListenerListenerUnregistered);
    this._bl.addEventListener('foundadvertisedname', this.BusListenerFoundAdvertisedName);
    this._bl.addEventListener('lostadvertisedname', this.BusListenerLostAdvertisedName);
    this._bl.addEventListener('nameownerchanged', this.BusListenerNameOwnerChanged);

    this._spListener.addEventListener('acceptsessionjoiner', this.SessionPortListenerAcceptSessionJoiner);
    this._spListener.addEventListener("sessionjoined", this.SessionPortListenerSessionJoined);

    this._sListener.addEventListener('sessionlost', this.SessionListenerSessionLost);
    this._sListener.addEventListener('sessionmemberadded', this.SessionListenerSessionMemberAdded);
    this._sListener.addEventListener('sessionmemberremoved', this.SessionListenerSessionMemberRemoved);
}

function OnOpen(evt) {
    var description = GetEventArg(evt, 1);
    var sock = GetEventArg(evt, 2);
    if (httpStreamer != null) {
        httpStreamer.stop();
    }
    httpStreamer = new AllJoynStreaming.MediaHTTPStreamer(mediaSink);
    httpStreamer.start(sock, description.mimeType, description.size, HTTP_PORT);
    started = true;
    paused = false;
}

function OnPlay() {
    paused = false;
}

function OnPause() {
    paused = true;
}

function OnClose() {
    paused = true;
}

(function InitializeAllJoyn() {
    AllJoyn.Debug.useOSLogging(true);
    AllJoyn.Debug.setDebugLevel("ALLJOYN", 7);

    var connectSpec = "null:";

    try {
        bus = new AllJoyn.BusAttachment(APPLICATION_NAME, true, 4);
    }
    catch (err) {
        var m = AllJoyn.AllJoynException.getErrorMessage(err.HResult);
    }
    bus.start();

    bus.connectAsync(connectSpec).then(function () {

        listeners = new Listeners(bus);
        bus.registerBusListener(listeners._bl);

        mediaSink = new AllJoynStreaming.MediaSink(bus);
        mediaRender = new AllJoynStreaming.MediaRenderer();
        mediaRender.addEventListener('onopen', OnOpen);
        mediaRender.addEventListener('onplay', OnPlay);
        mediaRender.addEventListener('onpause', OnPause);
        mediaRender.addEventListener('onclose', OnClose);

        bus.findAdvertisedName(MEDIA_SERVER_NAME);
    });
})();
