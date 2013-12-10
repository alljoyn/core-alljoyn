/*
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
var APPLICATION_NAME = 'Videostreaming',
    SESSION_PORT = 111,
    SOURCE_SERVICE_NAME = 'trm.streaming.source',
    SOURCE_INTERFACE_NAME = 'trm.streaming.Source',
    SOURCE_OBJECT_PATH = '/trm/streaming/Source',
    SINK_SERVICE_NAME = 'trm.streaming.sink',
    SINK_INTERFACE_NAME = 'trm.streaming.Sink',
    SINK_OBJECT_PATH = '/trm/streaming/Sink';

AllJoyn.MsgArg.setTypeCoercionMode("weak");
AllJoyn.Debug.useOSLogging(true);
AllJoyn.Debug.setDebugLevel("HTTP_SERVER", 7);
/*
 * Helper function for retreiving event arguments.
 * The 1st event argument is stored in evt.target, and the rest of the arguments are stored in evt.detail as an array.
 */
function GetEventArg(evt, index) {
    if (index == 0)
        return evt.target;
    else if (index > 0)
        return evt.detail[(index - 1)];
    else
        return nullptr;
}

function VideoSourceBusObject(bus, path) {
    console.log('VideoSourceBusObject constructor()');
    this._bsObject = new AllJoyn.BusObject(bus, path, false);

    try {
        /* Add the interface to this object */
        this.videoSrcIfac = bus.getInterface(SOURCE_INTERFACE_NAME);
        this._bsObject.addInterface(this.videoSrcIfac);

        this.getHandler = function (evt) {
            console.log('VideoSourceBusObject getHandler()');
            try {
                var ifaceMember = GetEventArg(evt, 0);
                var propName = GetEventArg(evt, 1);
                var retVal = GetEventArg(evt, 2);
                if (propName == 'Streams') {
                    var srcs = [],
                        url;
                    console.log('The number of Streams = ' + source.sources.length);
                    for (port in source.sources) {
                        var struct_array = new Array();
                        struct_array[0] = source.sources[port].file.name;
                        struct_array[1] = port;
                        var arg = new AllJoyn.MsgArg('(su)', struct_array);
                        srcs.push(arg);
                    }
                    retVal[0] = new AllJoyn.MsgArg('a(su)', new Array(srcs));
                } else {
                    console.log('Error: property ' + propName + ' does not exist');
                }
            } catch (err) {
                var h = AllJoyn.AllJoynException.getErrorMessage(err.number);
                console.log('Error: VideoSourceBusObject getHandler exception: ' + m);
            }
        }
        this._bsObject.addEventListener('get', this.getHandler);
    } catch (err) {
        var m = AllJoyn.AllJoynException.getErrorMessage(err.number);
        console.log('Error: VideoSourceBusObject constructor exception: ' + m);
    }
};

function VideoSinkBusObject(bus, path) {
    console.log('VideoSinkBusObject constructor()');

    this._bsObject = new AllJoyn.BusObject(bus, path, false);

    /* Add the interface to this object */
    this.videoSinkIfac = bus.getInterface(SINK_INTERFACE_NAME);
    this._bsObject.addInterface(this.videoSinkIfac);

    this.getHandler = function (evt) {
        console.log('VideoSinkBusObject GetHandler');
        try {
            var ifaceMember = GetEventArg(evt, 0);
            var propName = GetEventArg(evt, 1);
            var retVal = GetEventArg(evt, 2);

            if (propName == 'Playlist') {
                console.log('Playlist Length = ' + sink.playlist.length);
                var list = [];
                for (var i = 0; i < sink.playlist.length; i++) {
                    list.push(sink.playlist[i].name);
                }
                retVal[0] = new AllJoyn.MsgArg('as', new Array(list));
            } else if (propName == 'NowPlaying') {
                var msgargs = new Array();
                msgargs[0] = sink.nowPlaying;
                retVal[0] = new AllJoyn.MsgArg('i', msgargs);
            } else {
                console.log('Property ' + propName + ' does not exist');
            }
        } catch (err) {
            var m = AllJoyn.AllJoynException.getErrorMessage(err.number);
            console.log('VideoSinkBusObject getHandler exception: ' + m);
        }
    }
    this._bsObject.addEventListener('get', this.getHandler);

    this.propsChangedSignalMember = this.videoSinkIfac.getMember('PropertiesChanged');

    this.pushMethodReceiver = new AllJoyn.MessageReceiver(bus);
    pushHandler = function (evt) {
        var message = GetEventArg(evt, 1);
        var fname = message.getArg(0).value;
        var name = message.getArg(1).value;
        var port = message.getArg(2).value;
        var sender = message.sender;

        console.log('Push Handler(' + fname + ',' + name + ',' + port + ')');

        sink.playlist.push({ fname: fname, name: name, port: port, mpname: sender });
        var props = [];
        props.push('Playlist');
        sink.notifyPropertiesChanged(props);
    };
    this.pushMethodReceiver.addEventListener('methodhandler', pushHandler);
    this.pushMethodMember = this.videoSinkIfac.getMember('Push')
    this._bsObject.addMethodHandler(this.pushMethodMember, this.pushMethodReceiver);


    this.playMethodReceiver = new AllJoyn.MessageReceiver(bus);
    function playHandler(evt) {
        console.log('Play Handler()');
        onPlay();
    }

    this.playMethodReceiver.addEventListener('methodhandler', playHandler);
    this.playMethodMember = this.videoSinkIfac.getMember('Play')
    this._bsObject.addMethodHandler(this.playMethodMember, this.playMethodReceiver);

    this.pauseReceiver = new AllJoyn.MessageReceiver(bus);
    this.pauseHandler = function (evt) {
        console.log('Pause Handler()');
        onPause();
    }
    this.pauseReceiver.addEventListener('methodhandler', this.pauseHandler);
    this.pauseMethodMember = this.videoSinkIfac.getMember('Pause')
    this._bsObject.addMethodHandler(this.pauseMethodMember, this.pauseReceiver);

    this.previousReceiver = new AllJoyn.MessageReceiver(bus);
    this.previousHandler = function (evt) {
        console.log('Previous Handler()');

        if (sink.playlist.length) {
            if (sink.nowPlaying > 0) {
                --sink.nowPlaying;
            }
            console.log('Previous');
            sink.load();
        }
    }
    this.previousReceiver.addEventListener('methodhandler', this.previousHandler);
    this.previousMethodMember = this.videoSinkIfac.getMember('Previous')
    this._bsObject.addMethodHandler(this.previousMethodMember, this.previousReceiver);

    this.nextMethodReceiver = new AllJoyn.MessageReceiver(bus);
    this.nextHandler = function (evt) {
        console.log('Next()');

        if (sink.playlist.length) {
            if (sink.nowPlaying < (sink.playlist.length - 1)) {
                ++sink.nowPlaying;
            }
            console.log('Next');
            sink.load();
        }
    }
    this.nextMethodReceiver.addEventListener('methodhandler', this.nextHandler);
    this.nextMethodMember = this.videoSinkIfac.getMember('Next')
    this._bsObject.addMethodHandler(this.nextMethodMember, this.nextMethodReceiver);
};

var addInterfaces = function (bus) {
    var srcIfceXml = "<interface name=\"trm.streaming.Source\">" +
    "   <property name=\"Streams\" type=\"a(su)\" access=\"read\"/>" +
    "   <signal name=\"PropertiesChanged\">" +
    "      <arg name=\"ifceName\" type=\"s\" direction=\"out\"/>" +
    "      <arg name=\"dict\" type=\"a{sv}\" direction=\"out\"/>" +
    "      <arg name=\"props\" type=\"as\" direction=\"out\"/>" +
    "   </signal>" +
    "</interface>";
    bus.createInterfacesFromXml(srcIfceXml);

    var sinkIfceXml = "<interface name=\"trm.streaming.Sink\">" +
    "   <property name=\"Playlist\" type=\"as\" access=\"read\"/>" +
    "   <property name=\"NowPlaying\" type=\"i\" access=\"read\"/>" +
    "   <signal name=\"PropertiesChanged\">" +
    "      <arg name=\"ifceName\" type=\"s\" direction=\"out\"/>" +
    "      <arg name=\"dict\" type=\"a{sv}\" direction=\"out\"/>" +
    "      <arg name=\"props\" type=\"as\" direction=\"out\"/>" +
    "   </signal>" +
    "   <method name=\"Push\">" +
    "      <arg name=\"fname\" type=\"s\" direction=\"in\"/>" +
    "      <arg name=\"name\" type=\"s\" direction=\"in\"/>" +
    "      <arg name=\"port\" type=\"u\" direction=\"in\"/>" +
    "   </method>" +
    "   <method name=\"Play\">" +
    "   </method>" +
    "   <method name=\"Pause\">" +
    "   </method>" +
    "   <method name=\"Previous\">" +
    "   </method>" +
    "   <method name=\"Next\">" +
    "   </method>" +
    "</interface>";
    bus.createInterfacesFromXml(sinkIfceXml);
};

var source = (function () {

    var bus,
        sourceObject,
        sinkProxyObject,
        sessionId = -1,
        sources = {},
        callbacks = {};

    var start = function (onStart) {
        var status,
            name;

        callbacks = {
            onStart: onStart
        };

        bus = new AllJoyn.BusAttachment(APPLICATION_NAME, true, 4);
        callbacks.onStart && callbacks.onStart(bus.globalGUIDString);
        addInterfaces(bus);
        bus.start();
        bus.connectAsync("null:").then(function () {
            console.log("Connected to Bus Successfully");
            onConnected();
        });

        function onConnected() {
            sourceObject = new VideoSourceBusObject(bus, SOURCE_OBJECT_PATH);
            bus.registerBusObject(sourceObject._bsObject)
            var spListener = new AllJoyn.SessionPortListener(bus);
            spListener.addEventListener('acceptsessionjoiner', function onSessionPortListenerAcceptSessionJoiner(evt) {
                console.log("Event acceptsessionjoiner");
                return true;
            });
            spListener.addEventListener("sessionjoined", function onSessionPortListenerSessionJoined(evt) {
                sessionId = GetEventArg(evt, 1);
                console.log("Event sessionjoined sessionId = " + sessionId);
                bus.setLinkTimeoutAsync(sessionId, 10, null).then(function () {
                });
            });
            var sessionOptIn = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES, true, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY)
            var sessionPortOut = new Array(1);
            bus.bindSessionPort(SESSION_PORT, sessionPortOut, sessionOptIn, spListener);

            name = SOURCE_SERVICE_NAME + '-' + bus.globalGUIDString;
            bus.requestName(name, AllJoyn.RequestNameType.dbus_NAME_DO_NOT_QUEUE | AllJoyn.RequestNameType.debus_NAME_FLAG_REPLACE_EXISTING);
            bus.advertiseName(name, AllJoyn.TransportMaskType.transport_ANY);
        }
    };

    var notifyPropertiesChanged = function (props) {
        try {
            var videoSrcIfce = bus.getInterface(SOURCE_INTERFACE_NAME);
            var propsChangedSignalMember = videoSrcIfce.getMember("PropertiesChanged");
            var msgarg = new Array();

            msgarg[0] = new AllJoyn.MsgArg('s', ['trm.streaming.Source']);

            msgarg[1] = new AllJoyn.MsgArg('a{sv}', new Array([]));
            msgarg[2] = new AllJoyn.MsgArg('as', [props]);
            sourceObject._bsObject.signal("", sessionId, propsChangedSignalMember, msgarg, 0, 0);
        } catch (err) {
            var r = 0;
        }
    }

    var addSource = function (file) {
        console.log('addSource ' + file.name);
        var status,
            opts;
        var spListener = new AllJoyn.SessionPortListener(bus);
        spListener.addEventListener('acceptsessionjoiner', function onSessionPortListenerAcceptSessionJoiner() {
            console.log("Event acceptsessionjoiner");
            return true;
        });
        spListener.addEventListener("sessionjoined", function onSessionPortListenerSessionJoined(evt) {
            console.log("Event sessionjoined");
            var sessionPort = GetEventArg(evt, 0);
            var sessionId = GetEventArg(evt, 1);
            var joiner = GetEventArg(evt, 2);

            var socketStreams = new Array(1);
            bus.getSessionSocketStream(sessionId, socketStreams);
            var fd = socketStreams[0];
            sources[sessionPort].url = window.URL.createObjectURL(sources[sessionPort].file);
            sources[sessionPort].id = sessionId;
            console.log('onJoined sending ' + sources[sessionPort].url + ' for name ' + sources[sessionPort].file.name + ' port:' + sessionPort + ' id:' + sessionId);
            AllJoyn.StreamSourceHost.send(fd, sources[sessionPort].file);
        });

        var sListener = new AllJoyn.SessionListener(bus);
        sListener.addEventListener('sessionlost', function onSessionListenerSessionLost() {
            console.log('onLost id:' + id);
            var id = GetEventArg(evt, 0);
            for (port in sources) {
                if (sources[port].id === id) {
                    if (sources[port].url) {
                        console.log('onLost revoking ' + sources[port].url + ' name:' + sources[port].file.name + ' port:' + port);
                        window.URL.revokeObjectURL(sources[port].url);
                        sources[port].url = null;
                    }
                    break;
                }
            }
        });
        var sessionOptIn = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_RAW_RELIABLE, false, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY)
        var sessionPortOut = new Array(1);
        try {
            bus.bindSessionPort(AllJoyn.SessionPortType.session_PORT_ANY, sessionPortOut, sessionOptIn, spListener);
        } catch (err) {
            var m = AllJoyn.AllJoynException.getErrorMessage(err.hResult);
            console.log('Caught exception : ' + m);
        }

        sources[sessionPortOut[0]] = {
            url: null,
            file: file,
            id: 0
        };
        console.log('bound ' + file.name + ' to ' + sessionPortOut);

        var props = [];
        props.push('Streams');
        notifyPropertiesChanged(props);
    };

    var removeSource = function (name) {
        console.log('removeSource ' + name);
        var status;

        for (port in sources) {
            if (sources[port].file.name === name) {
                try {
                    bus.unbindSessionPort(port);
                } catch (err) {
                    console.log('Unbind session failed');
                    return;
                }

                console.log('unbound ' + sources[port].file.name + ' from ' + sources[port].url + ',' + port);
                if (sources[port].url) {
                    window.URL.revokeObjectURL(sources[port].url);
                    sources[port].url = null;
                }
                delete sources[port];
                var props = [];
                props.push('Streams');
                notifyPropertiesChanged(props);
                break;
            }
        }
    };

    var that = {
        get sources() { return sources; },
        start: start,
        addSource: addSource,
        removeSource: removeSource
    };

    return that;
})();

var sink = (function () {
    var bus,
        sinkObject = null,
        sessionId = -1,
        playlist = [],
        nowPlaying = -1,
        callbacks = {},
        httpServer = null;
    var log = function (prefix) {
        var msg = prefix;
        msg += ': playlist=[';
        for (var i = 0; i < playlist.length; ++i) {
            msg += '{ fname: ' + playlist[i].fname + ', name: ' + playlist[i].name + ', port: ' + playlist[i].port + ' },';
        }
        msg += '],nowPlaying=' + nowPlaying;
        console.log(msg);
    };

    var getPlaylist = function () {
        var pl = [],
            i;

        for (i = 0; i < playlist.length; ++i) {
            pl[i] = playlist[i].fname
        }
        return pl;
    };

    var notifyPropertiesChanged = function (props) {
        var msgarg = new Array();
        msgarg[0] = new AllJoyn.MsgArg('s', ['trm.streaming.Sink']);
        msgarg[1] = new AllJoyn.MsgArg('a{sv}', new Array([]));
        msgarg[2] = new AllJoyn.MsgArg("as", new Array(props));
        sinkObject._bsObject.signal("", sessionId, sinkObject.propsChangedSignalMember, msgarg, 0, 0);

        if (sink.nowPlaying === -1) {
            sink.nowPlaying = 0;
            load();
        }

        callbacks.onPlaylistChanged && onPlaylistChanged();
    }

    var load = function () {
        var opts_in = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_RAW_RELIABLE, true, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY);
        var opts_out = new Array(1);
        bus.joinSessionAsync(playlist[sink.nowPlaying].name, playlist[sink.nowPlaying].port, null, opts_in, opts_out, null).then(function (result) {
            try {
                var status = result.status;
                if (status) {
                    console.log("Join session '" + name + "' failed [(" + status + ")]");
                } else {
                    var id = result.sessionId;
                    httpServer = new AllJoyn.HttpServer();
                    httpServer.startAsync().then(function () {
                        var socketStream = new Array(1);
                        var url = new Array(1);
                        bus.getSessionSocketStream(id, socketStream);
                        httpServer.createObjectURL(socketStream[0], url);
                        console.log('CreateObjectURL: ' + url[0]);
                        callbacks.onLoad && onLoad(url[0]);
                        var props = [];
                        props.push("Playlist");
                        notifyPropertiesChanged(props);
                        callbacks.onPlaylistChanged && onPlaylistChanged();
                    });
                }
            } catch (err) {
                var h = AllJoyn.AllJoynException.getErrorMessage(err.number);
                console.log('joinSessionAsync() caught exception = ' + h);
            }
        });

    };

    var start = function (onStart, onLoad, onPlay, onPause, onPlaylistChanged) {
        var status,
            name;

        callbacks = {
            onStart: onStart,
            onLoad: onLoad,
            onPlay: onPlay,
            onPause: onPause,
            onPlaylistChanged: onPlaylistChanged
        };

        bus = new AllJoyn.BusAttachment(APPLICATION_NAME, true, 4);
        callbacks.onStart && callbacks.onStart(bus.globalGUIDString);
        addInterfaces(bus);

        var bl = new AllJoyn.BusListener(bus);
        bl.addEventListener('onnameownerchanged', function (evt) {
            console.log('Event onnameownerchanged');
            try {
                var name = GetEventArg(evt, 0);
                var previousOwner = GetEventArg(evt, 1);
                var newOwner = GetEventArg(evt, 2);

                var i,
                    n;

                console.log('onNameOwnerChanged name:' + name + ' previousOwner:' + previousOwner + ' newOwner:' + newOwner);
                n = playlist.length;
                for (i = 0; i < playlist.length;) {
                    if (previousOwner && !newOwner && playlist[i].mpname === name) {
                        playlist.splice(i, 1);
                        if (i < sink.nowPlaying) {
                            --sink.nowPlaying;
                        } else if (i === sink.nowPlaying) {
                            sink.nowPlaying = -1;
                        }
                    } else {
                        ++i;
                    }
                }
                if (playlist.length !== n) {
                    var props = [];
                    props.push("Playlist");
                    props.push("NowPlaying");
                    notifyPropertiesChanged(props);

                    console.log('onNameOwnerChanged');
                    callbacks.onPlaylistChanged && onPlaylistChanged();
                }
            } catch (err) {
                var m = AllJoyn.AllJoynException.getErrorMessage(err.number);
                console.log('onnameownerchanged handler caught exception : ' + m);
            }
        });

        bus.registerBusListener(bl);
        bus.start();
        bus.connectAsync("null:").then(function () {
            try {
                sinkObject = new VideoSinkBusObject(bus, SINK_OBJECT_PATH);
                bus.registerBusObject(sinkObject._bsObject);
                var spListener = new AllJoyn.SessionPortListener(bus);
                spListener.addEventListener('acceptsessionjoiner', function (evt) {
                    console.log('Event acceptsessionjoiner');
                    return true;
                });
                spListener.addEventListener('sessionjoined', function (evt) {
                    console.log('Event sessionjoined');
                    sessionId = GetEventArg(evt, 1);
                });

                var sessionOptIn = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES, true, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY)
                var sessionPortOut = new Array(1);

                bus.bindSessionPort(SESSION_PORT, sessionPortOut, sessionOptIn, spListener);
                name = SINK_SERVICE_NAME + '-' + bus.globalGUIDString;
                bus.requestName(name, AllJoyn.RequestNameType.dbus_NAME_DO_NOT_QUEUE | AllJoyn.RequestNameType.dbus_NAME_FLAG_REPLACE_EXISTING);
                bus.advertiseName(name, AllJoyn.TransportMaskType.transport_ANY);
            } catch (err) {
                var m = AllJoyn.AllJoynException.getErrorMessage(err.number);
                console.log('connectAsync() caught exception : ' + m);
            }
        });
    };

    var that = {
        playlist: playlist,
        nowPlaying : nowPlaying,
        notifyPropertiesChanged: notifyPropertiesChanged,
        load : load,
        start: start
    };

    return that;
})();

var browser = (function () {
    var SERVICE_NAME_PREFIX = 'trm.streaming',
        SESSION_PORT = 111;

    var bus,
        sourceId = -1,
        onSourceChanged = null,
        sinkId = -1,
        onSinkChanged = null,
        sourceProxyObjs = {},
        sinkProxyObjs = {}
    ;

    var getSourceProperties = function (name, sessionId) {
        var proxyObj = sourceProxyObjs[sessionId];
        try {
            proxyObj.getAllPropertiesAsync(SOURCE_INTERFACE_NAME, null, 100000 * 1000).then(function (result) {
                var info = [];
                var status = result.status;
                var props = result.value;
                var msgargs = props.value;
                var i = 0, len = msgargs.length;
                for (; i < len; i++) {
                    if (msgargs[i].key == 'Streams' && msgargs[i].value && msgargs[i].value.value) {
                        var entries = msgargs[i].value.value;
                        var j = 0, len2 = entries.length;
                        for (; j < len2; j++) {
                            info.push(entries[j].value);
                        }
                        break;
                    }
                }

                if (status != 0) {
                    console.log('Fail to get all properties');
                } else {
                    if (onSourceChanged) {
                        onSourceChanged(info);
                    }
                }
            }, function () {
                var i = 0;
            });
        } catch (err) {
            var m = AllJoyn.AllJoynException.getErrorMessage(err.number);
            console.log('browser.getSourceProperties() exception : ' + m);
        }
    };

    var onSourcePropertiesChanged = function (evt) {
        var sessionId = GetEventArg(evt, 2).sessionId;
        var sender = GetEventArg(evt, 2).sender;
        getSourceProperties(sender, sessionId);
    };

    var joinSource = function (onChanged, name) {
        console.log('browser.joinSource()');
        var opts_in = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES, true, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY);
        var opts_out = new Array(1);
        bus.joinSessionAsync(name, SESSION_PORT, null, opts_in, opts_out, null).then(function (result) {
            var status = result.status;
            if (status != 0) {
                console.log('Join session \'' + name + '\' failed [(' + status + ')]');
            } else {
                console.log('Join session \'' + name + '\' succeed [(' + status + ')]');
                var sessionId = result.sessionId;
                var opts = result.opts;
                onSourceChanged = onChanged;

                var proxyBusObject = new AllJoyn.ProxyBusObject(bus, name, SOURCE_OBJECT_PATH, sessionId);
                proxyBusObject.introspectRemoteObjectAsync(null).then(function (result) {
                    if (result.status == AllJoyn.QStatus.er_OK) {
                        sourceProxyObjs[sessionId] = proxyBusObject;
                        getSourceProperties(name, sessionId);
                    } else {
                        console.log('Fail to introspectRemoteObject ' + name);
                    }
                });
            }
        });

    };
    var leaveSource = function () {
        console.log('browser.leaveSource()');
        try {
            if (sourceId !== -1) {
                bus.leaveSession(sourceId);
                sourceId = -1;
                onSourceChanged = null;
            }
        } catch (err) {
            var m = AllJoyn.AllJoynException.getErrorMessage(err.number);
            console.log('leaveSource exception : ' + m);
        }
    };

    var getSinkProperties = function (name, sessionId) {
        console.log('browser.getSinkProperties()');
        var proxyObj = sinkProxyObjs[sessionId]
        proxyObj.getAllPropertiesAsync(SINK_INTERFACE_NAME, null, 10 * 1000).then(function (result) {
            var status = result.status;
            var props = result.value.value;
            if (status != 0) {
                console.log('Fail to get all properties');
            } else {
                if (onSinkChanged) {
                    onSinkChanged(props);
                }
            }
        });
    }

    var onSinkPropertiesChanged = function (evt) {
        console.log('browser.onSinkPropertiesChanged()');
        var sessionId = GetEventArg(evt, 2).sessionId;
        var sender = GetEventArg(evt, 2).sender;
        getSinkProperties(sender, sessionId);
    };

    var joinSink = function (onChanged, name) {
        console.log('browser.joinSink()');
        var opts_in = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES, true, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY);
        var opts_out = new Array(1);
        bus.joinSessionAsync(name, SESSION_PORT, null, opts_in, opts_out, null).then(function (result) {
            var status = result.status;
            if (status != 0) {
                console.log('Join session \'' + name + '\' failed [(' + status + ')]');
            } else {
                console.log('Join session \'' + name + '\' succedded [(' + status + ')]');
                sinkId = result.sessionId;
                onSinkChanged = onChanged;

                var proxyBusObject = new AllJoyn.ProxyBusObject(bus, name, SINK_OBJECT_PATH, sinkId);
                try {
                    proxyBusObject.introspectRemoteObjectAsync(null).then(function (result) {
                        if (result.status == AllJoyn.QStatus.er_OK) {
                            sinkProxyObjs[sinkId] = proxyBusObject;
                            getSinkProperties(name, sinkId);
                        } else {
                            console.log('Fail to introspectRemoteObject ' + name);
                        }
                    });
                } catch (err) {
                    var m = AllJoyn.AllJoynException.getErrorMessage(err.number);
                }
            }
        });
    };

    var leaveSink = function () {
        console.log('browser.leaveSink()');
        try {
            if (sinkId !== -1) {
                bus.leaveSession(sinkId);
                sinkId = -1;
                onSinkChanged = null;
            }
        } catch (err) {
            var m = AllJoyn.AllJoynException.getErrorMessage(err.number);
            console.log('leaveSink exception : ' + m);
        }
    };

    var start = function (onFoundSource, onLostSource, onFoundSink, onLostSink) {
        var status;

        bus = new AllJoyn.BusAttachment(APPLICATION_NAME, true, 4);
        console.log(bus.globalGUIDString);
        addInterfaces(bus);
        var busListener = new AllJoyn.BusListener(bus);
        busListener.addEventListener('foundadvertisedname', function (evt) {
            var name = GetEventArg(evt, 0);
            var transport = GetEventArg(evt, 1);
            var namePrefix = GetEventArg(evt, 2);
            console.log('onFoundAdvertisedName(' + name + ',' + transport + ',' + namePrefix + ')');
            if (name.indexOf('.source') >= 0) {
                onFoundSource(name);
            } else {
                onFoundSink(name);
            }
        });

        busListener.addEventListener('lostadvertisedname', function (evt) {
            var name = GetEventArg(evt, 0);
            var transport = GetEventArg(evt, 1);
            var namePrefix = GetEventArg(evt, 2);

            console.log('lostadvertisedname(' + name + ',' + transport + ',' + namePrefix + ')');
            if (name.indexOf('.source') >= 0) {
                onLostSource(name);
            } else {
                onLostSink(name);
            }
        });
        bus.registerBusListener(busListener);
        bus.start();
        bus.connectAsync("null:").then(function () {
            console.log('Connect to AllJoyn successfully');

            var srcInterface = bus.getInterface('trm.streaming.Source');
            var srcPropChangedSignal = srcInterface.getMember('PropertiesChanged');
            var srcPropChangedReceiver = new AllJoyn.MessageReceiver(bus);
            srcPropChangedReceiver.addEventListener('signalhandler', onSourcePropertiesChanged);
            bus.registerSignalHandler(srcPropChangedReceiver, srcPropChangedSignal, SOURCE_OBJECT_PATH);


            var sinkInterface = bus.getInterface(SINK_INTERFACE_NAME);
            var sinkPropChangedSignal = sinkInterface.getMember('PropertiesChanged');
            var sinkPropChangedReceiver = new AllJoyn.MessageReceiver(bus);
            sinkPropChangedReceiver.addEventListener('signalhandler', onSinkPropertiesChanged);
            bus.registerSignalHandler(sinkPropChangedReceiver, sinkPropChangedSignal, SINK_OBJECT_PATH);

            try {
                bus.findAdvertisedName(SERVICE_NAME_PREFIX);
            } catch (err) {
                console.log('Find \'' + SERVICE_NAME_PREFIX + '\' failed ');
            }
        });
    };

    var push = function (fname, name, port, sink) {
        console.log('browser.push(fname = ' + fname + ' name = ' + name + ' port = ' + port + ' sink = ' + sink + ')');
        try {
            var proxyObj = sinkProxyObjs[sinkId];
            var sinkIface = bus.getInterface(SINK_INTERFACE_NAME);
            var msgArgs = new Array(3);
            msgArgs[0] = new AllJoyn.MsgArg('s', [fname]);
            msgArgs[1] = new AllJoyn.MsgArg('s', [name]);
            msgArgs[2] = new AllJoyn.MsgArg('u', [port]);
            proxyObj.methodCallAsync(sinkIface.getMember('Push'), msgArgs, null, 10 * 1000, 0).then(function (result) {
                var m = result.message;
            });
        } catch (err) {
            console.log(AllJoyn.AllJoynException.getErrorMessage(err.number));
        }
    };

    var play = function (sink) {
        console.log('browser.play(sink = ' + sink + ')');
        try {
            var proxyObj = sinkProxyObjs[sinkId];
            var sinkIface = bus.getInterface(SINK_INTERFACE_NAME);
            proxyObj.methodCallAsync(sinkIface.getMember('Play'), null, null, 10 * 1000, 0).then(function () {
            });
        } catch (err) {
            console.log(AllJoyn.AllJoynException.getErrorMessage(err.number));
        }
    };

    var pause = function (sink) {
        console.log('browser.pause(sink = ' + sink + ')');
        try {
            var proxyObj = sinkProxyObjs[sinkId];
            var sinkIface = bus.getInterface(SINK_INTERFACE_NAME);
            proxyObj.methodCallAsync(sinkIface.getMember('Pause'), null, null, 10 * 1000, 0).then(function () {
            });
        } catch (err) {
            console.log(AllJoyn.AllJoynException.getErrorMessage(err.number));
        }
    };

    var previous = function (sink) {
        console.log('browser.previous(sink = ' + sink + ')');
        try {
            var proxyObj = sinkProxyObjs[sinkId];
            var sinkIface = bus.getInterface(SINK_INTERFACE_NAME);
            proxyObj.methodCallAsync(sinkIface.getMember('Previous'), null, null, 10 * 1000, 0).then(function () {
            });
        } catch (err) {
            console.log(AllJoyn.AllJoynException.getErrorMessage(err.number));
        }
    };

    var next = function (sink) {
        console.log('browser.next(sink = ' + sink + ')');
        try {
            var proxyObj = sinkProxyObjs[sinkId];
            var sinkIface = bus.getInterface(SINK_INTERFACE_NAME);
            proxyObj.methodCallAsync(sinkIface.getMember('Next'), null, null, 10 * 1000, 0).then(function () {
            });
        } catch (err) {
            console.log(AllJoyn.AllJoynException.getErrorMessage(err.number));
        }
    };

    var that = {
        start: start,
        joinSource: joinSource,
        leaveSource: leaveSource,
        joinSink: joinSink,
        leaveSink: leaveSink,
        push: push,
        play: play,
        pause: pause,
        previous: previous,
        next: next
    };

    return that;
})();
