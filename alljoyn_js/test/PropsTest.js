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
AsyncTestCase("PropsTest", {
    _setUp: ondeviceready(function(callback) {
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        bus.destroy();
    },

    testProps: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.PropsInterface",
                    property: [
                        { name: "StringProp", signature: "s", access: "readwrite" },
                        { name: "IntProp", signature: "i", access: "readwrite" }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/testProperties",
                                      function() {
                                          var stringProp = "Hello";
                                          var intProp = 6;
                                          return {
                                              "org.alljoyn.bus.PropsInterface": {
                                                  get StringProp() { return stringProp; },
                                                  set StringProp(value) { stringProp = value; },
                                                  get IntProp() { return intProp; },
                                                  set IntProp(value) { intProp = value; }
                                              }
                                          };
                                      }(),
                                      false,
                                      callbacks.add(getDbus));
            };
            var getDbus = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject("org.freedesktop.DBus/org/freedesktop/DBus", callbacks.add(requestName));
            };
            var requestName = function(err, dbus) {
                assertFalsy(err);
                dbus.methodCall("org.freedesktop.DBus", "RequestName", "org.alljoyn.bus.samples.props", 0, callbacks.add(onRequestName));
            };
            var onRequestName = function(err, context, result) {
                assertFalsy(err);
                assertEquals(1, result);
                bus.getProxyBusObject("org.alljoyn.bus.samples.props/testProperties", callbacks.add(get));
            };
            var proxy;
            var get = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;
                proxy.methodCall("org.freedesktop.DBus.Properties", "Get", "org.alljoyn.bus.PropsInterface", "StringProp", callbacks.add(onGetStringProp));
            };
            var onGetStringProp = function(err, context, value) {
                assertFalsy(err);
                assertEquals("Hello", value);
                proxy.methodCall("org.freedesktop.DBus.Properties", "Set", "org.alljoyn.bus.PropsInterface", "StringProp", { "s": "MyNewValue" }, callbacks.add(onSetStringProp));
            };
            var onSetStringProp = function(err, context) {
                assertFalsy(err);
                proxy.methodCall("org.freedesktop.DBus.Properties", "Get", "org.alljoyn.bus.PropsInterface", "IntProp", callbacks.add(onGetIntProp));
            };
            var onGetIntProp = function(err, context, value) {
                assertFalsy(err);
                assertEquals(6, value);
                proxy.methodCall("org.freedesktop.DBus.Properties", "GetAll", "org.alljoyn.bus.PropsInterface", callbacks.add(onGetAll));
            };
            var onGetAll = function(err, context, values) {
                assertFalsy(err);
                assertEquals("MyNewValue", values.StringProp);
                assertEquals(6, values.IntProp);
            };
            this._setUp(callbacks.add(connect));
        });
    }
});
