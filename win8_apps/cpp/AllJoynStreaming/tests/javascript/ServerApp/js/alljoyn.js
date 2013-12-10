

var bus;
var mediaSource;
var mp3Reader;
var mp3Pacer;
var mp3Stream;
var streamingSong;

var APPLICATION_NAME = "MediaServer";
var MEDIA_SERVER_NAME = "org.alljoyn.Media.Server";
var SESSION_PORT = 123;
var mimeType = "audio/mpeg";
var sessionId = 0;
var jitter = 0;

function GetEventArg(evt, index) {
    if (index == 0)
        return evt.target;
    else if (index > 0)
        return evt.detail[(index - 1)];
    else
        return nullptr;
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
            bus.joinSessionAsync(MEDIA_SERVER_NAME, SESSION_PORT, listeners._sListener, opts, opts_out, null).then(function (result) {
                sessionId = result.sessionId;
                ConnectToServer();
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
        if (sessionId == 0) {
            sessionId = GetEventArg(evt, 1);
        }
        var joiner = GetEventArg(evt, 2);
    }

    this.SessionPortListenerAcceptSessionJoiner = function (evt) {
        var sessionPort = GetEventArg(evt, 0);
        var sessionId = GetEventArg(evt, 1);
        var joiner = GetEventArg(evt, 2);
        if (sessionPort == SESSION_PORT) {
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
    httpStreamer = new AllJoynStreaming.MediaHTTPStreamer(mediaSink);
    httpStreamer.start(sock, description.mimeType, description.size, HTTP_PORT);
}

function OnPlay() {
}

function OnPause() {
}

function OnClose() {
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
    });
})();

var start_btn = document.getElementById('start_btn');
start_btn.onclick = function () {
    if (streamingSong == null) {
        return;
    }

    mp3Reader = new AllJoynStreaming.MP3Reader();
    mp3Reader.setFileAsync(streamingSong).then(function (result) {
        if(result) {
            WinJS.log && WinJS.log("SetFileAsync success: ", "MediaServer", "status");
            mediaSource = new AllJoynStreaming.MediaSource(bus);

            /* Register MediaServer bus object */ 
            bus.registerBusObject(mediaSource.mediaSourceBusObject); 
            /* Request a well known name */ 
            bus.requestName(MEDIA_SERVER_NAME, AllJoyn.RequestNameType.debus_NAME_REPLACE_EXISTING | AllJoyn.RequestNameType.debus_NAME_DO_NOT_QUEUE);

            /* Advertise name */
            bus.advertiseName(MEDIA_SERVER_NAME, AllJoyn.TransportMaskType.transport_ANY);
   
            /* Bind a session for incoming client connections */
            var opts = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES, true, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY);
            var portOut = new Array(1);
            bus.bindSessionPort(SESSION_PORT, portOut, opts, listeners._spListener);

            mp3Pacer = new AllJoynStreaming.MP3Pacer(mp3Reader, jitter);
            mp3Stream = new AllJoynStreaming.MP3Stream(bus, "mp3", mp3Reader, mp3Pacer);
            mp3Stream.preFill = 1000;
            mediaSource.addStream(mp3Stream.stream);

        } else {
            WinJS.log && WinJS.log("SetFileAsync file: ", "MediaServer", "error");
        }
    });
}

var pick_btn = document.getElementById('pick_btn');
pick_btn.onclick = function () {
    var fileOpenPicker = new Windows.Storage.Pickers.FileOpenPicker();
    //fileOpenPicker.viewMode = Windows.Storage.Pickers.PickerViewMode.thumbnail;
    fileOpenPicker.suggestedStartLocation = Windows.Storage.Pickers.PickerLocationId.musicLibrary;
    fileOpenPicker.fileTypeFilter.append(".mp3");
    fileOpenPicker.pickSingleFileAsync().then(function (file) {
        if (file) {
            streamingSong = file;
            // Application now has read/write access to the picked file
            WinJS.log && WinJS.log("Picked song: " + file.name, "MediaServer", "status");
        } else {
            // The picker was dismissed with no selected file
            WinJS.log && WinJS.log("Operation cancelled.", "MediaServer", "status");
        }
    });
}