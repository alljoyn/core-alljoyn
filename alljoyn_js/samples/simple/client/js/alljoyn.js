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
var client = (function() {
        var SERVICE_NAME = "org.alljoyn.bus.samples.simple",
            CONTACT_PORT = 42;

        var that,
            bus,
            sessionId,
            isConnected = false,
            isStoppingDiscovery = false;

        var start = function() {
            /*
             * All communication through AllJoyn begins with a BusAttachment.
             *
             * By default AllJoyn does not allow communication between devices (i.e. bus to bus
             * communication). The optional first argument must be set to true to allow
             * communication between devices.
             */
            bus = new org.alljoyn.bus.BusAttachment();
            /*
             * Register a bus listener to receive advertised names.  We look for a specific
             * advertised name below (the well-known SERVICE_NAME).
             */
            var registerBusListener = function(err) {
                bus.registerBusListener({
                    onFoundAdvertisedName: function(name, transport, namePrefix) {
                        /*
                         * We found someone advertising the well-known SERVICE_NAME.  Now join a
                         * session of that someone.
                         */

                        /*
                         * If discovery is currently being stopped we won't join to any other
                         * sessions.
                         */
                        if (isStoppingDiscovery) {
                            return;
                        }
                        /*
                         * In order to join the session, we need to provide the well-known contact
                         * port.  This is pre-arranged between both sides as part of the definition
                         * of the simple service.
                         */
                        var onJoined = function(err, id, opts) {
                            if (err) {
                                alert("Join session '" + name + "' failed [(" + err + ")]");
                                return;
                            }
                            /*
                             * As a result of joining the session, we get a session identifier which
                             * we must use to identify the created session communication channel
                             * whenever we talk to the remote side.
                             *
                             * Save the session identifier until we try to talk to the remote side.
                             */
                            sessionId = id;
                            isConnected = true;
                            that.onstatus("Session '" + id + "' joined");
                        };
                        bus.joinSession({
                            host: name,
                            port: CONTACT_PORT,
                            onLost: function(id, reason) { that.onstatus("Session '" + id + "' lost (reason " + reason + ")"); }
                        }, onJoined);
                    }
                }, connect);
            };
            /* To communicate with AllJoyn objects, we must connect the BusAttachment to the bus. */
            var connect = function(err) {
                if (err) {
                    alert("Register bus listener failed [(" + err + ")]");
                    return;
                }
                bus.connect(findAdvertisedName);
            };
            /*
             * Now find an instance of the AllJoyn object we want to call.  We start by looking for
             * a name, then join a session of the device that is advertising that name.
             *
             * In this case, we are looking for the well-known SERVICE_NAME.
             */
            var findAdvertisedName = function(err) {
                if (err) {
                    alert("Connect failed [(" + err + ")]");
                    return;
                }
                bus.findAdvertisedName(SERVICE_NAME, done);
            };
            var done = function(err) {
                if (err) {
                    alert("Find advertised name failed [(" + err + ")]");
                    return;
                }
            };
            bus.create(true, registerBusListener);
        };

        that = {};

        that.start = function() {
            /* The user must allow the page to access AllJoyn. */
            navigator.requestPermission("org.alljoyn.bus", function() { start(); });
        }

        that.stop = function() {
            /* Release all resources acquired in the start. */
            var disconnect = function(err) {
                if (err) {
                    alert("Leave session failed [(" + err + ")]");
                }
                bus.disconnect(done);
            };
            var done = function(err) {
                if (err) {
                    alert("Disconnect failed [(" + err + ")]");
                }
                bus.destroy();
            };
            isStoppingDiscovery = true;
            if (isConnected) {
                bus.leaveSession(sessionId, disconnect);
            } else {
                disconnect();
            }
        };

        /*
         * Call the service's Ping method through a ProxyBusObject.
         *
         * This will also log the string that was sent to the service and the string that was
         * received from the service to the user interface.
         */
        that.ping = function(str) {
            /*
             * To communicate with an AllJoyn object, we create a ProxyBusObject.  A ProxyBusObject
             * is composed of a name, path, and sessionID.
             *
             * This ProxyBusObject is located at the well-known SERVICE_NAME, under path
             * "/SimpleService", and uses the sessionID we received earlier when we joined the
             * session.
             */
            var ping = function(err, proxyObj) {
                /* Make the Ping method call on the simpleInterface proxy object's interface. */
                that.onping("Me", str);
                proxyObj.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "Ping", str, function(err, context, str) {
                    if (err) {
                        that.onstatus("Ping failed " + error);
                        return;
                    }
                    that.onping(context.sender, str);
                });
            };
            bus.getProxyBusObject(SERVICE_NAME + "/SimpleService:sessionId=" + sessionId, ping);
        };

        return that;
    }());
