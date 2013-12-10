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

var APPLICATION_NAME = "chat";
/*
 * NAME_PREFIX is the well-known name prefix which all bus attachments hosting a channel will use.
 * NAME_PREFIX and the channel name are composed to give the well-known name that a
 * hosting bus attachment will request and advertise.
 */
var NAME_PREFIX = "org.alljoyn.bus.samples.chat";
var CHAT_SERVICE_INTERFACE_NAME = "org.alljoyn.bus.samples.chat";
/* The well-known session port used as the contact port for the chat service. */
var CONTACT_PORT = 27;
/* The object path used to identify the service "location" in the bus attachment. */
var OBJECT_PATH = "/chatService";
/* Use WIFI-Direct transport or not */
var enableWfdTransport = false;

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
        return null;
}

function ChatSessionObject(bus, path) {

    this._bsObject = new AllJoyn.BusObject(bus, path, false);

    /* Add the interface to this object */
    this.chatIfac = bus.getInterface(CHAT_SERVICE_INTERFACE_NAME);

    this._bsObject.addInterface(this.chatIfac);

    this.chatSignalReceiver = new AllJoyn.MessageReceiver(bus);

    this.chatSignalHandler = function (evt) {
        //var ifaceMember = GetEventArg(evt, 0);
        //var path = GetEventArg(evt, 1);
        var message = GetEventArg(evt, 2);
        if (message.sender == message.rcvEndpointName || message.sessionId != alljoyn.sessionId) {
            return;
        }
        var chatMsg = message.getArg(0).value;
        var sender = message.sender;
        alljoyn.onchat(alljoyn.sessionId, sender + ": " + chatMsg);
    }

    this.chatSignalReceiver.addEventListener('signalhandler', this.chatSignalHandler);

    this.chatSignalMember = this.chatIfac.getMember("Chat");

    bus.registerSignalHandler(this.chatSignalReceiver, this.chatSignalMember, path);

    this.SendChatSignal = function (id, msg, flags) {
        var msgarg = new AllJoyn.MsgArg("s", new Array(msg));
        this._bsObject.signal("", id, this.chatSignalMember, new Array(msgarg), 0, flags);
    };
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
        alljoyn.addChannelName(name);
        alljoyn.onname();
    }

    this.BusListenerLostAdvertisedName = function (evt) {
        var name = GetEventArg(evt, 0);
        var previousOwner = GetEventArg(evt, 1);
        var newOwner = GetEventArg(evt, 1);
        alljoyn.removeChannelName(name);
        alljoyn.onname();
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

var alljoyn = (function () {

    var that,
        bus,
        myHostChannel,
        channelNames,
        sessionId,
        addChannelName,
        removeChannelName,
        alljoynTransports;

    /*
	 * The channels list is the list of all well-known names that correspond to channels we
     * might conceivably be interested in.
     */
    channelNames = [];
    sessionId = 0;
    that = {
        get channelNames() { return channelNames; },
        get myHostChannel() { return myHostChannel; },
        get sessionId() { return sessionId; },
        set sessionId(val) { return sessionId = val; }
    };

    addChannelName = function (name) {
        name = name.slice(NAME_PREFIX.length + 1);
        if (name != that.myHostChannel) {
            removeChannelName(name);
            channelNames.push(name);
        }
    };
    removeChannelName = function (name) {
        name = name.slice(NAME_PREFIX.length + 1);
        for (var i = 0; i < channelNames.length; ++i) {
            if (channelNames[i] === name) {
                channelNames.splice(i, 1);
                break;
            }
        }
    };

    that.start = function () {
        try {
            /*
             * Enable debugging for specific native modules
             */
            AllJoyn.Debug.useOSLogging(true);

            AllJoyn.Debug.setDebugLevel("IPNS", 7);

            /*
             * AllJoyn should map JS weak type to AllJoyn strict data type
             */
            AllJoyn.MsgArg.setTypeCoercionMode("weak");

            alljoynTransports = AllJoyn.TransportMaskType.transport_ANY;
            if (enableWfdTransport) {
                alljoynTransports |= AllJoyn.TransportMaskType.transport_WFD;
            }

            /*
             * All communication through AllJoyn begins with a BusAttachment.
             *
             * By default AllJoyn does not allow communication between devices (i.e. bus to bus
             * communication). The second argument must be set to true to allow communication between devices.
             */
            bus = new AllJoyn.BusAttachment(APPLICATION_NAME, true, 4);
            /*
             * Register an instance of an AllJoyn bus listener that knows what to do with found and
             * lost advertised name notifications.
             */
            busListener = new Listeners(bus);
            bus.registerBusListener(busListener._bl);

            var chatIfacArr = new Array(1);
            var chatIfac = null;
            bus.createInterface(CHAT_SERVICE_INTERFACE_NAME, chatIfacArr, false);
            chatIfac = chatIfacArr[0];
            chatIfac.addSignal("Chat", "s", "str", 0, "");
            chatIfac.activate();

            sessionTestObj = new ChatSessionObject(bus, OBJECT_PATH);
            bus.registerBusObject(sessionTestObj._bsObject);
            bus.start();
            alljoyn.connect();
        } catch (err) {
            var errCode = AllJoyn.AllJoynException.FromCOMException(err.number);
            var errMsg = AllJoyn.AllJoynException.GetExceptionMessage(err.number);
            var stack = err.stack;
            console.log(stack);
            displayStatus("Initialize AllJoyn Error : " + errMsg);
        }
    };

    that.connect = function () {
        /* Connect the BusAttachment to the bus. */
        var connectSpec = "null:";
        bus.connectAsync(connectSpec).then(function () {
            displayStatus("Connect to AllJoyn successfully.");
            bus.findAdvertisedName(NAME_PREFIX);
        }, function (err) {
            displayStatus("Connect to AllJoyn failed. Error : " + AllJoyn.AllJoynException.getErrorMessage(err.number));
        });
    };

    that.addChannelName = addChannelName;
    that.removeChannelName = removeChannelName;
    /*
     * Starting a channel consists of binding a session port, requesting a well-known name, and
     * advertising the well-known name.
     */
    that.startChannel = function (name) {
        var wellKnownName = NAME_PREFIX + '.' + name;
        /* Create a new session listening on the contact port of the chat service. */

        var sessionOptIn = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES, true, AllJoyn.ProximityType.proximity_ANY, alljoynTransports)
        var sessionPortOut = new Array(1);
        bus.bindSessionPort(CONTACT_PORT, sessionPortOut, sessionOptIn, busListener._spListener);

        /* Request a well-known name from the bus... */
        bus.requestName(wellKnownName, AllJoyn.RequestNameType.dbus_NAME_DO_NOT_QUEUE);

        /* ...and advertise the same well-known name. */
        bus.advertiseName(wellKnownName, alljoynTransports);
        myHostChannel = name;
    };

    /* Releases all resources acquired in startChannel. */
    that.stopChannel = function () {
        var wellKnownName = NAME_PREFIX + '.' + myHostChannel;
        bus.cancelAdvertiseName(wellKnownName, alljoynTransports);
        bus.unbindSessionPort(CONTACT_PORT);
        bus.releaseName(wellKnownName);
        sessionId = 0;
    };

    /* Joins an existing session. */
    that.joinChannel = function (onjoined, onerror, name) {
        /*
         * The well-known name of the existing session is the concatenation of the NAME_PREFIX
         * and a channel name from the channelNames array.
         */

        var wellKnownName = NAME_PREFIX + '.' + name,
                onJoined,
                onError,
                status;

        onError = function (error) {
            onerror();
        };

        var sessionOptsArray = new Array(1);
        var opts = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES, false, AllJoyn.ProximityType.proximity_ANY, alljoynTransports);
        bus.joinSessionAsync(wellKnownName, CONTACT_PORT, busListener._sListener, opts, sessionOptsArray, null).then(function (reply) {
            var status = reply.status;
            sessionId = reply.sessionId;
            if (status == 0) {
                onjoined();
            } else {
                onerror(status);
            }
        });
    };

    /* Releases all resources acquired in joinChannel. */
    that.leaveChannel = function () {
        bus.leaveSession(that.sessionId);
    };

    /* Sends a message out over an existing remote session. */
    that.chat = function (str) {
        /*
         * This is a call to the implicit signal emitter method described above when the bus
         * object was registered.  Note the use of the optional parameters to specify the
         * session ID of the remote session.
         */
        sessionTestObj.SendChatSignal(that.sessionId, str, 0);
        alljoyn.onchat(sessionId, "Me: " + str);
    };

    // Hook the application suspending/resuming events to the handlers so that exclusively-held resource (eg., network port) is release and other applications are allowed to acquire the resource
    // For suspending event, the application can either register the event handler to "suspending" event or "checkpoint" event
    //Windows.UI.WebUI.WebUIApplication.addEventListener("suspending", suspendingHandler);
    WinJS.Application.addEventListener("checkpoint", suspendingHandler);
    function suspendingHandler(eventArgs) {
        try {
            if (bus != null && bus.onAppResume != undefined) {
                bus.onAppSuspend();
            }
        } catch (err) {
            console.log(err.stack);
        }
    }

    Windows.UI.WebUI.WebUIApplication.addEventListener("resuming", resumingHandler);
    function resumingHandler() {
        try {
            if (bus != null && bus.onAppResume != undefined) {
                bus.onAppResume();
            }
        } catch (err) {
            console.log(err.stack);
        }
    }
    return that;
}());
