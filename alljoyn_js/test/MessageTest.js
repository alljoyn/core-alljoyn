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
AsyncTestCase("MessageTest", {
    _setUp: ondeviceready(function(callback) {
        otherBus = undefined;
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        otherBus && otherBus.destroy();
        bus.destroy();
    },

    testMessage: function(queue) {
        queue.call(function(callbacks) {
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                otherBus.createInterface({
                    name: "org.alljoyn.bus.samples.simple.SimpleInterface",
                    method: [
                        { name: 'Ping', signature: 's', returnSignature: 's', argNames: 'inStr,outStr' }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                otherBus.registerBusObject("/testobject", {
                    "org.alljoyn.bus.samples.simple.SimpleInterface": {
                        Ping: function(context, inStr) {
                            assertFalse(context.isUnreliable);
                            assertUndefined(context.authMechanism);
                            assertEquals("s", context.signature);
                            assertEquals("/testobject", context.objectPath);
                            assertEquals("org.alljoyn.bus.samples.simple.SimpleInterface", context.interfaceName);
                            assertEquals("Ping", context.memberName);
                            assertEquals(bus.uniqueName, context.sender);
                            assertEquals(otherBus.uniqueName, context.destination);
                            assertEquals(0, context.sessionId);
                            assertNotNull(otherBus.timestamp);
                            context.reply(inStr);
                        }
                    }
                }, callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(otherBus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "Ping", "hello", callbacks.add(onPing));
            };
            var onPing = function(err, context, outStr) {
                assertFalsy(err);
                assertFalse(context.isUnreliable);
                assertUndefined(context.authMechanism);
                assertEquals("s", context.signature);
                assertUndefined(context.objectPath);
                assertUndefined(context.interfaceName);
                assertUndefined(context.memberName);
                assertEquals(otherBus.uniqueName, context.sender);
                assertEquals(bus.uniqueName, context.destination);
                assertEquals(0, context.sessionId);
                assertNotNull(bus.timestamp);
            };
            this._setUp(callbacks.add(otherBusCreate));
        });
    },

    testReplyError: function(queue) {
        queue.call(function(callbacks) {
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                otherBus.createInterface({
                    name: "org.alljoyn.bus.samples.simple.SimpleInterface",
                    method: [
                        { name: 'PingError', signature: 's', returnSignature: 's' },
                        { name: 'PingStatus', signature: 's', returnSignature: 's' }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var errorMessage = "Sample error";
            var registerBusObject = function(err) {
                assertFalsy(err);
                otherBus.registerBusObject("/testobject", {
                    "org.alljoyn.bus.samples.simple.SimpleInterface": {
                        PingError: function(context, inStr) {
                            if (errorMessage) {
                                context.replyError("org.alljoyn.bus.samples.simple.Error", errorMessage);
                            } else {
                                context.replyError("org.alljoyn.bus.samples.simple.Error");
                            }
                        },
                        PingStatus: function(context, inStr) {
                            context.replyError(1);
                        }
                    }
                }, callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(otherBus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var testobject;
            var ping = function(err, proxyObj) {
                assertFalsy(err);
                testobject = proxyObj;
                testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "PingError", "hello", callbacks.add(onPing));
            };
            var onPing = function(err, context, outStr) {
                assertEquals("org.alljoyn.bus.samples.simple.Error", err.name);
                if (errorMessage) {
                    assertEquals(errorMessage, err.message);
                    errorMessage = undefined;
                    testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "PingError", "hello", callbacks.add(onPing));
                } else {
                    assertEquals("", err.message);
                    testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "PingStatus", "hello", callbacks.add(onPingStatus));
                }
            };
            var onPingStatus = function(err) {
                assertEquals(1, err.code);
            };
            this._setUp(callbacks.add(otherBusCreate));
        });
    },
});

