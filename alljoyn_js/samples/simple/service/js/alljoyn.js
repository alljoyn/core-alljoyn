/*
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
var service = (function() {
        var SERVICE_NAME = "org.alljoyn.bus.samples.simple",
            CONTACT_PORT = 42;

        var that,
            aj,
            bus;

        aj = org.alljoyn.bus;

        var start = function() {
            /*
             * All communication through AllJoyn begins with a BusAttachment.
             *
             * By default AllJoyn does not allow communication between devices (i.e. bus to bus
             * communication). The optional first argument must be set to true to allow
             * communication between devices.
             */
            bus = new aj.BusAttachment();
            /*
             * Specify an AllJoyn interface.  The name by which this interface will be known is the
             * property name of the bus.interfaces object.  In this case, it is
             * "org.alljoyn.bus.samples.simple.SimpleInterface".
             */
            var createInterface = function(err) {
                bus.createInterface({
                    name: "org.alljoyn.bus.samples.simple.SimpleInterface",
                    /*
                     * Specify a Ping method of the interface.  The input argument and return value are
                     * both strings.
                     */
                    method: [
                        { name: "Ping", signature: "s", returnSignature: "s", argNames: "inStr,outStr" }
                    ]
                }, registerBusObject);
            };
            /*
             * To make a service available to other AllJoyn peers, register a BusObject with the
             * BusAttachment at a specific path.
             *
             * Our service is an object located at the "/SimpleService" path.
             */
            var registerBusObject = function(err) {
                bus.registerBusObject("/SimpleService", {
                    /*
                     * Our service implements the org.alljoyn.bus.samples.simple.SimpleInterface
                     * interface.
                     */
                    "org.alljoyn.bus.samples.simple.SimpleInterface": {
                        /*
                         * This function is called when a remote attachment makes a method call on this
                         * bus object.  All method handler functions are called with a message context
                         * followed by the method arguments.
                         *
                         * Returning a value from the method is done by calling the reply method of the
                         * context.
                         */
                        Ping: function(context, str) {
                            that.onping(context.sender, str);
                            /* Echo the input argument back as the method return value. */
                            context.reply(str);
                            that.onping(bus.uniqueName, str);
                        }
                    }
                }, connect);
            };
            /*
             * The next steps in making a service available to other AllJoyn peers is to connect the
             * BusAttachment to the bus, bind a session port, and obtain a well-known name.
             */
            var connect = function(err) {
                /* Connect the BusAttachment to the bus. */
                bus.connect(bindSessionPort);
            };
            /* Create a new session listening on the contact port of the simple service. */
            var bindSessionPort = function(err) {
                if (err) {
                    alert("Connect to AllJoyn failed [(" + err + ")]");
                    return;
                }
                bus.bindSessionPort({
                    port: CONTACT_PORT,
                    onAccept: function(port, joiner, opts) {
                        return (port === CONTACT_PORT);
                    },
                    onJoined: function(port, id, joiner) {
                        that.onstatus("(" + joiner + ") joined session '" + id + "'");
                    }
                }, requestName);
            };
            /* Request a well-known name from the bus... */
            var requestName = function(err) {
                if (err) {
                    alert("Bind session port failed [(" + err + ")]");
                    return;
                }
                bus.requestName(SERVICE_NAME, aj.BusAttachment.DBUS_NAME_FLAG_REPLACE_EXISTING |
                                              aj.BusAttachment.DBUS_NAME_FLAG_DO_NOT_QUEUE, advertiseName);
            };
            /* ...and advertise the same well-known name. */
            var advertiseName = function(err) {
                if (err) {
                    alert("Request '" + SERVICE_NAME + "' failed [(" + err + ")]");
                    return;
                }
                bus.advertiseName(SERVICE_NAME, aj.SessionOpts.TRANSPORT_ANY, done);
            };
            var done = function(err) {
                if (err) {
                    alert("Advertise '" + SERVICE_NAME + "' failed [(" + err + ")]");
                    return;
                }
            };
            bus.create(true, createInterface);
        };

        that = {};

        that.start = function() {
            navigator.requestPermission("org.alljoyn.bus", function() { start(); });
        }

        /* Release all resources acquired in start. */
        that.stop = function() {
            var disconnect = function(err) {
                bus.disconnect(done);
            };
            var done = function(err) {
                if (err) {
                    alert("Disconnect from AllJoyn failed [(" + err + ")]");
                }
                bus.destroy();
            };
            bus.unregisterBusObject("/SimpleService", disconnect);
        };

        return that;
    }());
