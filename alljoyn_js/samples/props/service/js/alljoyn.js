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
var alljoyn = (function() {
        var NAME = "org.alljoyn.bus.samples.props",
            OBJECT_PATH = "/testProperties";

        var that,
            bus,
            busObject;

        var start = function() {
            bus = new org.alljoyn.bus.BusAttachment();
            var connect = function(err) {
                bus.connect(createInterface);
            };
            var createInterface = function(err) {
                if (err) {
                    alert("Connect to AllJoyn failed [(" + err + ")]");
                    return;
                }
                bus.createInterface({
                    name: "org.alljoyn.bus.samples.props.PropsInterface",
                    /*
                     * Specify two properties of this interface, a string property and an integer
                     * property.  Both properties can be got and set.
                     */
                    property: [
                        { name: "StringProp", signature: "s", access: "readwrite" },
                        { name: "IntProp", signature: "i", access: "readwrite" }
                    ]
                }, registerBusObject);
            };
            var registerBusObject = function(err) {
                busObject = (function() {
                    var stringProp = "Hello";
                    var intProp = 6;
                    /*
                     * An implementation of PropsInterface.  Property get and set implementations
                     * are implemented as JavaScript getters and setters.  The JavaScript name of
                     * the property is identical to the name specified in the AllJoyn interface.
                     */
                    return {
                        "org.alljoyn.bus.samples.props.PropsInterface": {
                            get StringProp() { return stringProp; },
                            set StringProp(value) {
                                stringProp = value;
                                that.onstringprop(stringProp);
                            },
                            get IntProp() { return intProp; },
                            set IntProp(value) {
                                intProp = value;
                                that.onintprop(intProp);
                            }
                        }
                    };
                }());
                bus.registerBusObject(OBJECT_PATH, busObject, requestName);
            };
            var requestName = function(err) {
                bus.requestName(NAME, done);
            };
            var done = function(err) {
                if (err) {
                    alert("Request name '" + NAME + "' failed [(" + err + ")]");
                    return;
                }
                that.onstringprop(busObject["org.alljoyn.bus.samples.props.PropsInterface"].StringProp);
                that.onintprop(busObject["org.alljoyn.bus.samples.props.PropsInterface"].IntProp);
            };
            bus.create(true, connect);
        };

        that = {};

        that.start = function() {
            navigator.requestPermission("org.alljoyn.bus", function() { start(); });
        }

        return that;
    }());
