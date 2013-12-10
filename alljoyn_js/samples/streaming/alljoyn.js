/*
 * Copyright (c) 2012-2013, AllSeen Alliance. All rights reserved.
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
var onError = function(err) {
    if (err) alert(err);
};

var addInterfaces = function(bus, callback) {
    var createInterface = function(err) {
        bus.createInterface({
            name: 'trm.streaming.Sink',
            property: [
                { name: 'Playlist', signature: 'as', access: 'read' },
                { name: 'NowPlaying', signature: 'i', access: 'read' }
            ],
            signal: [
                { name: 'PropertiesChanged', signature: 'sa{sv}as' }
            ],
            method: [
                { name: 'Push', signature: 'sssu', argNames: 'fname,type,name,port' },
                { name: 'Play' },
                { name: 'Pause' },
                { name: 'Previous' },
                { name: 'Next' },
            ]
        }, callback);
    };
    bus.createInterface({
        name: 'trm.streaming.Source',
        property: [
            { name: 'Streams', signature: 'a(ssu)', access: 'read' }
        ],
        signal: [
            { name: 'PropertiesChanged', signature: 'sa{sv}as' }
        ]
    }, createInterface);
};

var source = (function() {
    var SERVICE_NAME = 'trm.streaming.source',
        SESSION_PORT = 111,
        OBJECT_PATH = '/trm/streaming/Source';

    var bus,
        sessionId = -1,
        sources = {},
        callbacks = {},
        busObject;

    var start = function(onStart) {
        var name;

        callbacks = {
            onStart: onStart
        };

        bus = new org.alljoyn.bus.BusAttachment();
        var createInterface = function(err) {
            callbacks.onStart && callbacks.onStart(bus.globalGUIDString);
            addInterfaces(bus, registerBusObject);
        };
        var registerBusObject = function(err) {
            busObject = {
                'trm.streaming.Source' : {
                    get Streams() {
                        var srcs = [],
                            url;

                        for (port in sources) {
                            srcs.push([sources[port].file.name, sources[port].file.type, port]);
                        }
                        return srcs;
                    }
                }
            };
            bus.registerBusObject(OBJECT_PATH, busObject, connect);
        };
        var connect = function(err) {
            bus.connect(bindSessionPort);
        };
        var bindSessionPort = function(err) {
            if (err) {
                alert('Connect to AllJoyn failed [(' + err + ')]');
                return;
            }
            bus.bindSessionPort({
                port: SESSION_PORT,
                isMultipoint: true,
                onAccept: function(port, joiner, opts) {
                    return true;
                },
                onJoined: function(port, id, joiner) {
                    sessionId = id;
                    bus.setLinkTimeout(sessionId, 10);
                },
                onLost: function(id, reason) {
                    console.log('Lost session id:' + id + ' reason:' + reason);
                }
            }, requestName);
        };
        var requestName = function(err) {
            if (err) {
                alert('Bind session failed [(' + err + ')]');
                return;
            }
            name = SERVICE_NAME + '-' + bus.globalGUIDString;
            bus.requestName(name, bus.DBUS_NAME_FLAG_REPLACE_EXISTING | bus.DBUS_NAME_FLAG_DO_NOT_QUEUE, advertiseName);
        };
        var advertiseName = function(err) {
            if (err) {
                alert('Request \'' + name + '\' failed [(' + err + ')]');
                return;
            }
            bus.advertiseName(name, org.alljoyn.bus.SessionOpts.TRANSPORT_ANY, done);
        };
        var done = function(err) {
            if (err) {
                alert('Advertise \'' + name + '\' failed [(' + err + ')]');
            }
        };
        bus.create(true, createInterface);
    };

    var addSource = function(file) {
        var opts;

        opts = {
            traffic: org.alljoyn.bus.SessionOpts.TRAFFIC_RAW_RELIABLE,
            transport: org.alljoyn.bus.SessionOpts.TRANSPORT_WLAN,
            onAccept: function(port, joiner, opts) {
                console.log('onAccept port:' + port);
                return true;
            },
            onJoined: function(port, id, opts) {
                var send = function(err, fd) {
                    sources[port].url = window.URL.createObjectURL(sources[port].file);
                    sources[port].id = id;
                    console.log('onJoined sending ' + sources[port].url + ' for name ' + sources[port].file.name + ' port:' + port + ' id:' + id);
                    fd.send(sources[port].url, function(err) {});
                }
                bus.getSessionFd(id, send);
            },
            onLost: function(id, reason) {
                console.log('onLost id:' + id + ' reason:' + reason);
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
            }
        };
        var signal = function(err, port) {
            if (err) {
                alert('Bind session failed [(' + err + ')]');
                return;
            }
            sources[port] = {
                url: null,
                file: file,
                id: 0
            };
            console.log('bound ' + file.name + ' to ' + port);
            busObject.signal('trm.streaming.Source', 'PropertiesChanged', 'trm.streaming.Source', {}, ['Streams'], { sessionId: sessionId }, function() {});
        };
        bus.bindSessionPort(opts, signal);
    };

    var removeSource = function(name) {
        for (port in sources) {
            if (sources[port].file.name === name) {
                var signal = function(err) {
                    if (err) {
                        alert('Unbind session failed [(' + err + ')]');
                        return;
                    }
                    console.log('unbound ' + sources[port].file.name + ' from ' + sources[port].url + ',' + port);
                    if (sources[port].url) {
                        window.URL.revokeObjectURL(sources[port].url);
                        sources[port].url = null;
                    }
                    delete sources[port];
                    busObject.signal('trm.streaming.Source', 'PropertiesChanged', 'trm.streaming.Source', {}, ['Streams'], { sessionId: sessionId }, function() {});
                }
                bus.unbindSessionPort(port, signal);
                return;
            }
        }
    };

    var that = {
        start: start,
        addSource: addSource,
        removeSource: removeSource
    };

    return that;
})();

var sink = (function() {
    var SERVICE_NAME = 'trm.streaming.sink',
        SESSION_PORT = 111,
        OBJECT_PATH = '/trm/streaming/Sink';

    var bus,
        sessionId = -1,
        playlist = [],
        nowPlaying = -1,
        callbacks = {},
        busObject;

    var log = function(prefix) {
        var msg = prefix;
        msg += ': playlist=[';
        for (var i = 0; i < playlist.length; ++i) {
            msg += '{ fname: ' + playlist[i].fname + ', name: ' + playlist[i].name + ', port: ' + playlist[i].port + ' },';
        }
        msg += '],nowPlaying=' + nowPlaying;
        console.log(msg);
    };

    var getPlaylist = function() {
        var pl = [],
            i;

        for (i = 0; i < playlist.length; ++i) {
            pl[i] = playlist[i].fname
        }
        return pl;
    };

    var load = function() {
        var onJoined = function(err, id, opts) {
            if (err) {
                alert("Join session '" + name + "' failed [(" + err + ")]");
                return;
            }
            var createObjectURL = function(err, fd) {
                org.alljoyn.bus.SocketFd.createObjectURL(fd, function(request) {
                    request.status = 200;
                    request.statusText = 'OK';
                    request.setResponseHeader('Date', new Date().toUTCString());
                    request.setResponseHeader('Content-type', playlist[nowPlaying].type);
                    request.send();
                }, load);
            };
            var load = function(err, url) {
                callbacks.onLoad && callbacks.onLoad(url);
                busObject.signal('trm.streaming.Sink', 'PropertiesChanged', 'trm.streaming.Sink', {}, ['NowPlaying'], { sessionId: sessionId }, function() {});
                callbacks.onPlaylistChanged && callbacks.onPlaylistChanged();
            };
            bus.getSessionFd(id, createObjectURL);
        };
        bus.joinSession({
            host: playlist[nowPlaying].name,
            port: playlist[nowPlaying].port,
            traffic: org.alljoyn.bus.SessionOpts.TRAFFIC_RAW_RELIABLE
        }, onJoined);
    };

    var start = function(onStart, onLoad, onPlay, onPause, onPlaylistChanged) {
        var name;

        callbacks = {
            onStart: onStart,
            onLoad: onLoad,
            onPlay: onPlay,
            onPause: onPause,
            onPlaylistChanged: onPlaylistChanged
        };

        bus = new org.alljoyn.bus.BusAttachment();
        var createInterface = function(err) {
            callbacks.onStart && callbacks.onStart(bus.globalGUIDString);
            addInterfaces(bus, registerBusListener);
        };
        var registerBusListener = function(err) {
            bus.registerBusListener({
                onNameOwnerChanged: function(name, previousOwner, newOwner) {
                    var i,
                        n;

                    console.log('onNameOwnerChanged name:' + name + ' previousOwner:' + previousOwner + ' newOwner:' + newOwner);
                    n = playlist.length;
                    for (i = 0; i < playlist.length; ) {
                        if (previousOwner && !newOwner && playlist[i].mpname === name) {
                            playlist.splice(i, 1);
                            if (i < nowPlaying) {
                                --nowPlaying;
                            } else if (i === nowPlaying) {
                                nowPlaying = -1;
                            }
                        } else {
                            ++i;
                        }
                    }
                    if (playlist.length !== n) {
                        bus[OBJECT_PATH]['trm.streaming.Sink'].PropertiesChanged('trm.streaming.Sink', {}, ['Playlist', 'NowPlaying'], { sessionId: sessionId });
                        log('onNameOwnerChanged');
                        callbacks.onPlaylistChanged && callbacks.onPlaylistChanged();
                    }
                }
            }, registerBusObject);
        };
        var registerBusObject = function(err) {
            busObject = {
                'trm.streaming.Sink' : {
                    get Playlist() { return getPlaylist(); },
                    get NowPlaying() { return nowPlaying; },
                    Push: function(context, fname, type, name, port) {
                        console.log('Push(' + fname + ',' + type + ',' + name + ',' + port + ')');
                        context.reply();

                        playlist.push({ fname: fname, type: type, name: name, port: port, mpname: context.sender });
                        busObject.signal('trm.streaming.Sink', 'PropertiesChanged', 'trm.streaming.Sink', {}, ['Playlist'], { sessionId: sessionId }, function() {});
                        if (nowPlaying === -1) {
                            nowPlaying = 0;
                            load();
                        }
                        log('Push');
                        callbacks.onPlaylistChanged && callbacks.onPlaylistChanged();
                    },
                    Play: function(context) {
                        console.log('Play()');
                        context.reply();
                        callbacks.onPlay && callbacks.onPlay();
                    },
                    Pause: function(context) {
                        console.log('Pause()');
                        context.reply();
                        callbacks.onPause && callbacks.onPause();
                    },
                    Previous: function(context) {
                        console.log('Previous()');
                        context.reply();

                        if (playlist.length) {
                            if (nowPlaying > 0) {
                                --nowPlaying;
                            }
                            log('Previous');
                            load();
                        }
                    },
                    Next: function(context) {
                        console.log('Next()');
                        context.reply();

                        if (playlist.length) {
                            if (nowPlaying < (playlist.length - 1)) {
                                ++nowPlaying;
                            }
                            log('Next');
                            load();
                        }
                    },
                }
            };
            bus.registerBusObject(OBJECT_PATH, busObject, connect);
        };
        var connect = function(err) {
            bus.connect(bindSessionPort);
        };
        var bindSessionPort = function(err) {
            if (err) {
                alert('Connect to AllJoyn failed [(' + err + ')]');
                return;
            }
            bus.bindSessionPort({
                port: SESSION_PORT,
                isMultipoint: true,
                onAccept: function(port, joiner, opts) {
                    return true;
                },
                onJoined: function(port, id, joiner) {
                    sessionId = id;
                },
                onMemberRemoved: function(id, name) {
                    console.log('onMemberRemoved: id:' + id + ' name:' + name);
                }
            }, requestName);
        };
        var requestName = function(err) {
            if (err) {
                alert('Bind session failed [(' + err + ')]');
                return;
            }
            name = SERVICE_NAME + '-' + bus.globalGUIDString;
            bus.requestName(name, bus.DBUS_NAME_FLAG_REPLACE_EXISTING | bus.DBUS_NAME_FLAG_DO_NOT_QUEUE, advertiseName);
        };
        var advertiseName = function(err) {
            if (err) {
                alert('Request \'' + name + '\' failed [(' + err + ')]');
                return;
            }
            bus.advertiseName(name, org.alljoyn.bus.SessionOpts.TRANSPORT_ANY, done);
        };
        var done = function(err) {
            if (err) {
                alert('Advertise \'' + name + '\' failed [(' + err + ')]');
            }
        };
        bus.create(true, createInterface);
    };

    var that = {
        get playlist() { return getPlaylist(); },
        get nowPlaying() { return nowPlaying; },
        start: start
    };

    return that;
})();

var browser = (function() {
    var SERVICE_NAME = 'trm.streaming',
        SESSION_PORT = 111;

    var bus,
        sourceId = -1,
        onSourceChanged = null,
        sinkId = -1,
        onSinkChanged = null;

    var getSourceProperties = function(name, sessionId) {
        var getAll = function(err, proxyObj) {
            proxyObj.methodCall('org.freedesktop.DBus.Properties', 'GetAll', 'trm.streaming.Source', done);
        };
        var done = function(err, context, props) {
            if (onSourceChanged) {
                onSourceChanged(props);
            }
        };
        bus.getProxyBusObject(name + '/trm/streaming/Source:sessionId=' + sessionId, getAll);
    };

    var onSourcePropertiesChanged = function(context, name, changed, invalidated) {
        getSourceProperties(context.sender, context.sessionId);
    };

    var joinSource = function(onChanged, name) {
        var onJoined = function(err, id, opts) {
            if (err) {
                alert('Join session \'' + name + '\' failed [(' + err + ')]');
                return;
            }
            sourceId = id;
            onSourceChanged = onChanged;
            getSourceProperties(name, sourceId);
        };
        bus.joinSession({
            host: name,
            port: SESSION_PORT,
            isMultipoint: true
        }, onJoined);
    };
    var leaveSource = function() {
        if (sourceId !== -1) {
            var done = function(err) {
                sourceId = -1;
                onSourceChanged = null;
            };
            bus.leaveSession(sourceId, done);
        }
    };

    var getSinkProperties = function(name, sessionId) {
        var getAll = function(err, proxyObj) {
            proxyObj.methodCall('org.freedesktop.DBus.Properties', 'GetAll', 'trm.streaming.Sink', done);
        };
        var done = function(err, context, props) {
            if (onSinkChanged) {
                onSinkChanged(props);
            }
        };
        bus.getProxyBusObject(name + '/trm/streaming/Sink:sessionId=' + sessionId, getAll);
    };

    var onSinkPropertiesChanged = function(context, name, changed, invalidated) {
        getSinkProperties(context.sender, context.sessionId);
    };

    var joinSink = function(onChanged, name) {
        var onJoined = function(err, id, opts) {
            if (err) {
                alert('Join session \'' + name + '\' failed [(' + err + ')]');
                return;
            }
            sinkId = id;
            onSinkChanged = onChanged;
            getSinkProperties(name, sinkId);
        };
        bus.joinSession({
            host: name,
            port: SESSION_PORT,
            isMultipoint: true
        }, onJoined);
    };
    var leaveSink = function() {
        if (sinkId !== -1) {
            var done = function(err) {
                sinkId = -1;
                onSinkChanged = null;
            };
            bus.leaveSession(sinkId, done);
        }
    };

    var start = function(onFoundSource, onLostSource, onFoundSink, onLostSink) {
        bus = new org.alljoyn.bus.BusAttachment();
        var createInterface = function(err) {
            console.log(bus.globalGUIDString);
            addInterfaces(bus, registerBusListener);
        };
        var registerBusListener = function(err) {
            bus.registerBusListener({
                onFoundAdvertisedName: function(name, transport, namePrefix) {
                    console.log('onFoundAdvertisedName(' + name + ',' + transport + ',' + namePrefix + ')');
                    if (name.indexOf('.source') >= 0) {
                        onFoundSource(name);
                    } else {
                        onFoundSink(name);
                    }
                },
                onLostAdvertisedName: function(name, transport, namePrefix) {
                    console.log('onLostAdvertisedName(' + name + ',' + transport + ',' + namePrefix + ')');
                    if (name.indexOf('.source') >= 0) {
                        onLostSource(name);
                    } else {
                        onLostSink(name);
                    }
                }
            }, connect);
        };
        var connect = function(err) {
            bus.connect(registerSignalHandler1);
        }
        var registerSignalHandler1 = function(err) {
            if (err) {
                alert('Connect to AllJoyn failed [(' + err + ')]');
                return;
            }
            bus.registerSignalHandler(onSourcePropertiesChanged, 'trm.streaming.Source.PropertiesChanged', registerSignalHandler2);
        };
        var registerSignalHandler2 = function(err) {
            bus.registerSignalHandler(onSinkPropertiesChanged, 'trm.streaming.Sink.PropertiesChanged', findAdvertisedName);
        };
        var findAdvertisedName = function(err) {
            bus.findAdvertisedName(SERVICE_NAME, done);
        };
        var done = function(err) {
            if (err) {
                alert('Find \'' + SERVICE_NAME + '\' failed [(' + err + ')]');
            }
        };
        bus.create(true, createInterface);
    };

    var push = function(fname, type, name, port, sink) {
        var done = function(err, proxyObj) {
            proxyObj.methodCall('trm.streaming.Sink', 'Push', fname, type, name, port, onError);
        };
        bus.getProxyBusObject(sink + '/trm/streaming/Sink:sessionId=' + sinkId, done);
    };

    var play = function(sink) {
        var done = function(err, proxyObj) {
            proxyObj.methodCall('trm.streaming.Sink', 'Play', onError);
        };
        bus.getProxyBusObject(sink + '/trm/streaming/Sink:sessionId=' + sinkId, done);
    };
    var pause = function(sink) {
        var done = function(err, proxyObj) {
            proxyObj.methodCall('trm.streaming.Sink', 'Pause', onError);
        };
        bus.getProxyBusObject(sink + '/trm/streaming/Sink:sessionId=' + sinkId, done);
    };
    var previous = function(sink) {
        var done = function(err, proxyObj) {
            proxyObj.methodCall('trm.streaming.Sink', 'Previous', onError);
        };
        bus.getProxyBusObject(sink + '/trm/streaming/Sink:sessionId=' + sinkId, done);
    };
    var next = function(sink) {
        var done = function(err, proxyObj) {
            proxyObj.methodCall('trm.streaming.Sink', 'Next', onError);
        };
        bus.getProxyBusObject(sink + '/trm/streaming/Sink:sessionId=' + sinkId, done);
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
