/*
 * Copyright AllSeen Alliance. All rights reserved.
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

    var ABOUTDATA;
    var that = [],
        serviceBus = -1,
        aboutObj,
        PORT = 27,
        hostSessionId = -1;

    var OBJECT_PATH = "/example/path";

    /*
     * Get the instance of our plugin.  All our scripting will begin with this.
     */
    aj = org.alljoyn.bus;

    var start = function() {
        var init = function(err) {
            serviceBus = new aj.BusAttachment();
            serviceBus.create(true, createInterface);
        }
        var createInterface = function(err) {
            if(err) {
                alert("bindSessionPort error");
                return;
            }
            serviceBus.createInterface({
                name: "com.example.about.feature.interface.sample"
            }, registerBusObject);
        };

        var registerBusObject = function(err) {
            if(err) {
                alert("createInterface failed [(" + err + ")]");
                return;
            }

            busObject = {
                "com.example.about.feature.interface.sample": { }
            };
            serviceBus.registerBusObject(OBJECT_PATH, busObject, false, true, connect);
        };

        var connect = function(err) {
            serviceBus.connect(bindSessionPort);
        };

        var bindSessionPort = function(err) {
            if(err) {
                alert("connect failed [(" + err + ")]");
                return;
            }
            serviceBus.bindSessionPort({
                port: PORT,
                isMultipoint: true,
                /*
                 * This method is called when a client tries to join the session we have bound.
                 * It asks us if we want to accept the client into our session.
                 */
                onAccept: function(port, joiner, opts) {
                    return (port === PORT);
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
            //}, getAboutObject);
            }, advertiseName);
        }

        var getAboutObject = function(err) {
            if(err) {
                alert("registerBusObject failed [(" + err + ")]");
                return;
            }
            serviceBus.getAboutObj(announce);
        }

        var advertiseName = function(err) {
            if(err) {
                alert("announce error");
                return false;
            }
            var uniqueName = serviceBus.uniqueName;

            serviceBus.advertiseName(uniqueName, aj.SessionOpts.TRANSPORT_ANY, announce);
            console.log("About Service done");
        }

        var announce = function(err) {
            serviceBus.getAboutObj(function(err, aboutObj) {
                if(err) {
                    alert("getAboutObject failed [(" + err + ")]");
                    return;
                }
                aboutObj.announce(PORT, ABOUTDATA, done);
            });
        }


        var done = function(err) {
            if (err) {
                alert("Advertise name '" + wellKnownName + "' failed [(" + err + ")]");
                return;
            }
        };

        if(serviceBus != -1) {
            var disconnect = function(err) {
                if (err) {
                    alert("UnregisterBusObject from AllJoyn failed [(" + err + ")]");
                }
                serviceBus.disconnect(function(err) {
                    if (err) {
                        alert("Disconnect from AllJoyn failed [(" + err + ")]");
                    }
                    serviceBus.destroy();
                    serviceBus = -1;
                    init();
                });
            };
            console.log("start destroy bus");
            serviceBus.unregisterBusObject(OBJECT_PATH, disconnect);
        }
        else {
            init();
        }
    };

    that.setAboutData = function(aboutData) {
        ABOUTDATA = aboutData;
    }

    that.start = function() {
        navigator.requestPermission("org.alljoyn.bus", function() { start(); });
    }

    that.stop = function() {
        var disconnect = function(err) {
            serviceBus.disconnect(done);
        };
        var done = function(err) {
            if (err) {
                alert("Disconnect from AllJoyn failed [(" + err + ")]");
            }
            serviceBus.destroy();
            serviceBus = -1;
            console.log("bus destroyed");
        };
        if(serviceBus != -1) {
            serviceBus.unregisterBusObject(OBJECT_PATH, disconnect);
        }
    }


    return that;

}());
