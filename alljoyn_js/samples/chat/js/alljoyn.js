/*
 * Copyright (c) 2011, 2013, AllSeen Alliance. All rights reserved.
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
var alljoyn = (function() {
        /*
         * The well-known name prefix which all bus attachments hosting a channel will use.
         *
         * The NAME_PREFIX and the channel name are composed to give the well-known name that a
         * hosting bus attachment will request and advertise.
         */
        var NAME_PREFIX = "org.alljoyn.bus.samples.chat",
        /* The well-known session port used as the contact port for the chat service. */
            CONTACT_PORT = 27,
        /* The object path used to identify the service "location" in the bus attachment. */
            OBJECT_PATH = "/chatService";

        var that,
            aj,
            bus,
            channelName,
            channelNames,
            /*
             * The session identifier of the "use" session that the application uses to talk to
             * remote instances.  Set to -1 if not connectecd.
             */
            sessionId = -1,
            /*
             * The session identifier of the "host" session that the application provides for remote
             * devices.  Set to -1 if not connected.
             */
            hostSessionId = -1,
            /*
             * A flag indicating that the application has joined a chat channel that it is hosting.
             * See the long comment in joinChannel() for a description of this rather
             * non-intuitively complicated case.
             */
            joinedToSelf = false,
            addChannelName,
            removeChannelName,
            busObject;

        /*
         * Get the instance of our plugin.  All our scripting will begin with this.
         */
        aj = org.alljoyn.bus;

        /*
         * The channels list is the list of all well-known names that correspond to channels we
         * might conceivably be interested in.  The "use" tab will display this list in the
         * "joinChannel" form.  Choosing one will result in a joinSession call.
         */
        channelNames = [];
        addChannelName = function(name) {
            name = name.slice(NAME_PREFIX.length + 1);
            removeChannelName(name);
            channelNames.push(name);
        };
        removeChannelName = function(name) {
            name = name.slice(NAME_PREFIX.length + 1);
            for (var i = 0; i < channelNames.length; ++i) {
                if (channelNames[i] === name) {
                    channelNames.splice(i, 1);
                    break;
                }
            }
        };

        var start = function() {
            var onChat;

            /*
             * All communication through AllJoyn begins with a BusAttachment.
             *
             * By default AllJoyn does not allow communication between devices (i.e. bus to bus
             * communication). The optional first argument must be set to true to allow
             * communication between devices.
             */
            bus = new aj.BusAttachment();
            /*
             * Register an instance of an AllJoyn bus listener that knows what to do with found and
             * lost advertised name notifications.
             */
            var registerBusListener = function(err) {
                bus.registerBusListener({
                    /*
                     * This method is called when AllJoyn discovers a remote attachment that is
                     * hosting a chat channel.  We expect that since we only do a findAdvertisedName
                     * looking for instances of the chat well-known name prefix we will only find
                     * names that we know to be interesting.  When we find a remote application that
                     * is hosting a channel, we add its channel name it to the list of available
                     * channels selectable by the user.
                     */
                    onFoundAdvertisedName: function(name, transport, namePrefix) {
                        addChannelName(name);
                        that.onname();
                    },
                    /*
                     * This method is called when AllJoyn decides that a remote bus attachment that
                     * is hosting a chat channel is no longer available.  When we lose a remote
                     * application that is hosting a channel, we remote its name from the list of
                     * available channels selectable by the user.
                     */
                    onLostAdvertisedName: function(name, transport, namePrefix) {
                        removeChannelName(name);
                        that.onname();
                    }
                }, createInterface);
            };
            /*
             * Specify an AllJoyn interface.  The name by which this interface will be known is the
             * property name of the bus.interfaces object.  In this case, it is
             * "org.alljoyn.bus.samples.chat".
             */
            var createInterface = function(err) {
                bus.createInterface({
                    name: "org.alljoyn.bus.samples.chat",
                    /*
                     * Specify a Chat signal of the interface.  The argument of the signal is a string.
                     */
                    signal: [
                        { name: "Chat", signature: "s", argNames: "str" }
                    ]
                }, registerBusObject);
            };
            /*
             * To make a service available to other AllJoyn peers, first register a BusObject with
             * the BusAttachment at a specific object path.  Our service is implemented by the
             * ChatService BusObject found at the "/chatService" object path.
             */
            var registerBusObject = function(err) {
                busObject = {
                    "org.alljoyn.bus.samples.chat": {
                        /*
                         * Signal emitter methods are created implicitly when the bus object is
                         * registered.  When registration is complete, there will exist a method
                         * 'bus[OBJECT_PATH]["org.alljoyn.bus.samples.chat"].Chat'.
                         */
                    }
                };
                bus.registerBusObject(OBJECT_PATH, busObject, connect);
            };
            /* Connect the BusAttachment to the bus. */
            var connect = function(err) {
                bus.connect(registerSignalHandler);
            };
            /*
             * The signal handler for messages received from the AllJoyn bus.
             *
             * Since the messages sent on a chat channel will be sent using a bus signal, we need to
             * provide a signal handler to receive those signals.  This is it.
             */
            var registerSignalHandler = function(err) {
                if (err) {
                    alert("Connect to AllJoyn failed [(" + err + ")]");
                    return;
                }
                onChat = function(context, str) {
                    /*
                     * See the long comment in joinChannel() for more explanation of why this is needed.
                     *
                     * The only time we allow a signal from the hosted session ID to pass through is if
                     * we are in the joined to self state.  If the source of the signal is us, we also
                     * filter out the signal since we are going to locally echo the signal.
                     */
                    if (context.sender === bus.uniqueName) {
                        return;
                    }
                    if ((context.sessionId == hostSessionId && joinedToSelf) ||
                        (context.sessionId == sessionId)) {
                        that.onchat(context.sender, str);
                    }
                };
                bus.registerSignalHandler(onChat, "org.alljoyn.bus.samples.chat.Chat", findAdvertisedName);
            };
            /*
             * Start discovering any attachments that are hosting chat sessions.  Since this is a
             * core bit of functionalty for the "use" side of the app, we always do this at startup.
             */
            var findAdvertisedName = function(err) {
                bus.findAdvertisedName(NAME_PREFIX, done);
            };
            var done = function(err) {
                if (err) {
                    alert("Find name '" + NAME_PREFIX + "' failed [(" + err + ")]");
                }
            };
            bus.create(true, registerBusListener);
        };

        that = {
            get channelNames() { return channelNames; }
        };

        that.start = function() {
            navigator.requestPermission("org.alljoyn.bus", function() { start(); });
        }

        /*
         * Starting a channel consists of binding a session port, requesting a well-known name, and
         * advertising the well-known name.
         */
        that.startChannel = function(name) {
            var wellKnownName = NAME_PREFIX + '.' + name;

            var requestName = function(err) {
                if (err) {
                    alert("Bind session port " + CONTACT_PORT + " failed [(" + err + ")]");
                    return;
                }
                /* Request a well-known name from the bus... */
                bus.requestName(wellKnownName, aj.BusAttachment.DBUS_NAME_FLAG_DO_NOT_QUEUE, advertiseName);
            };
            var advertiseName = function(err) {
                if (err) {
                    alert("Request name '" + wellKnownName + "' failed [(" + err + ")]");
                    return;
                }
                /* ...and advertise the same well-known name. */
                bus.advertiseName(wellKnownName, aj.SessionOpts.TRANSPORT_ANY, done);
            };
            var done = function(err) {
                if (err) {
                    alert("Advertise name '" + wellKnownName + "' failed [(" + err + ")]");
                    return;
                }
                channelName = name;
            };
            /* Create a new session listening on the contact port of the chat service. */
            bus.bindSessionPort({
                port: CONTACT_PORT,
                isMultipoint: true,
                /*
                 * This method is called when a client tries to join the session we have bound.
                 * It asks us if we want to accept the client into our session.
                 */
                onAccept: function(port, joiner, opts) {
                    return (port === CONTACT_PORT);
                },
                /*
                 * If we return true in onAccept, we admit a new client into our session.  The
                 * session does not really exist until a client joins, at which time the session
                 * is created and a session ID is assigned.  This method communicates to us that
                 * this event has happened, and provides the new session ID for us to use.
                 */
                onJoined: function(port, id, joiner) {
                    hostSessionId = id;
                }
            }, requestName);
        };

        /* Releases all resources acquired in startChannel. */
        that.stopChannel = function() {
            var wellKnownName = NAME_PREFIX + '.' + channelName;

            bus.cancelAdvertiseName(wellKnownName, aj.SessionOpts.TRANSPORT_ANY, unbindSessionPort);
            var unbindSessionPort = function(err) {
                if (err) {
                    alert("Cancel advertise name '" + wellKnownName + "' failed [(" + err + ")]");
                    return;
                }
                bus.unbindSessionPort(CONTACT_PORT, releaseName);
            };
            var releaseName = function(err) {
                bus.releaseName(wellKnownName, done);
            };
            var done = function(err) {
                if (err) {
                    alert("Release name '" + wellKnownName + "' failed [(" + err + ")]");
                }
            };
        };

        /* Joins an existing session. */
        that.joinChannel = function(onjoined, onerror, name) {
            /*
             * There is a relatively non-intuitive behavior of multipoint sessions that one needs to
             * grok in order to understand the code below.  The important thing to understand is
             * that there can be only one endpoint for a multipoint session in a particular bus
             * attachment.  This endpoint can be created explicitly by a call to joinSession() or
             * implicitly by a call to bindSessionPort().  An attempt to call joinSession() on a
             * session port we have created with bindSessionPort() will result in an error.
             *
             * When we call bindSessionPort(), we do an implicit joinSession() and thus signals
             * (which correspond to our chat messages) will begin to flow from the hosted chat
             * channel as soon as we begin to host a corresponding session.
             *
             * To achieve sane user interface behavior, we need to block those signals from the
             * implicit join done by the bind until our user joins the bound chat channel.  If we do
             * not do this, the chat messages from the chat channel hosted by the application will
             * appear in the chat channel joined by the application.
             *
             * Since the messages flow automatically, we can accomplish this by turning a filter on
             * and off in the chat signal handler.  So if we detect that we are hosting a channel,
             * and we find that we want to join the hosted channel we turn the filter off.
             *
             * We also need to be able to send chat messages to the hosted channel.  This means we
             * need to point the mChatInterface at the session ID of the hosted session.  There is
             * another complexity here since the hosted session doesn't exist until a remote session
             * has joined.  This means that we don't have a session ID to use in chat() until a
             * remote device does a joinSession on our hosted session.  This, in turn, means that we
             * have to remember the session ID when we get an onJoined() callback in the
             * SessionPortListener passed into bindSessionPort().
             *
             * So, to summarize, these next few lines handle a relatively complex case.  When we
             * host a chat channel, we do a bindSessionPort which *enables* the creation of a
             * session.  As soon as a remote device joins the hosted chat channel, a session is
             * actually created, and the SessionPortListener onJoined() callback is fired.  At that
             * point, we remember the hosted session's sessionId that we can use to send chat
             * messages to the channel we are hosting.  As soon as the session comes up, we begin
             * receiving chat messages from the session, so we need to filter them until the user
             * joins the hosted chat channel.  In a separate timeline, the user can decide to join
             * the chat channel she is hosting.  She can do so either before or after the
             * corresponding session has been created as a result of a remote device joining the
             * hosted session.  If she joins the hosted channel before the underlying session is
             * created, her chat messages will be discarded.  If she does so after the underlying
             * session is created, there will be a session emitter waiting to use to send chat
             * messages.  In either case, the signal filter will be turned off in order to listen to
             * remote chat messages.
             */
            if (name === channelName) {
                joinedToSelf = true;
                setTimeout(onjoined, 0);
                return;
            }

            /*
             * The well-known name of the existing session is the concatenation of the NAME_PREFIX
             * and a channel name from the channelNames array.
             */
            var wellKnownName = NAME_PREFIX + '.' + name;

            var onJoined = function(err, id, opts) {
                if (err) {
                    alert("Join session '" + wellKnownName + "' failed [(" + err + ")]");
                    onerror();
                    return;
                }
                sessionId = id;
                onjoined();
            };
            bus.joinSession({
                host: wellKnownName,
                port: CONTACT_PORT,
                isMultipoint: true,
                /*
                 * This method is called when the last remote participant in the chat session
                 * leaves for some reason and we no longer have anyone to chat with.
                 */
                onLost: function(id, reason) {
                    that.onlost(name);
                }
            }, onJoined);
        };

        /* Releases all resources acquired in joinChannel. */
        that.leaveChannel = function() {
            var done = function(err) {
                sessionId = -1;
                joinedToSelf = false;
            }
            if (!joinedToSelf) {
                bus.leaveSession(sessionId, done);
            } else {
                done();
            }
        };

        /* Sends a message out over an existing remote session. */
        that.chat = function(str) {
            var id;

            /*
             * This is a call to the implicit signal emitter method described above when the bus
             * object was registered.  Note the use of the optional parameters to specify the
             * session ID of the remote session.
             *
             * If we are joined to a remote session, we send the message over the remote session
             * ID.  If we are implicitly joined to a session we are hosting, we send the message
             * over the hosted session ID.
             */
            var done = function(err) {
                if (err) {
                    alert("Chat failed [(" + err + ")]");
                    return;
                }
                that.onchat("Me", str);
            };
            id = joinedToSelf ? hostSessionId : sessionId;
            busObject.signal("org.alljoyn.bus.samples.chat", "Chat", str, { sessionId: id }, done);
        };

        return that;
    }());
