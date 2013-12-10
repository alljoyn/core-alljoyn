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
AsyncTestCase("MessageArgCountTest", {
    _setUp: ondeviceready(function(callback) {
        var createInterface = function(err) {
            assertFalsy(err);
            bus.createInterface({
                name: "org.alljoyn.bus.samples.simple.SimpleInterface",
                method: [
                    { name: 'Ping', signature: 's', returnSignature: 's' }
                ]
            }, connect);
        };
        var connect = function(err) {
            assertFalsy(err);
            bus.connect(otherBusCreate);
        };
        var otherBusCreate = function(err) {
            assertFalsy(err);
            otherBus = new org.alljoyn.bus.BusAttachment();
            otherBus.create(false, otherBusConnect);
        };
        var otherBusConnect = function(err) {
            assertFalsy(err);
            otherBus.connect(callback);
        };
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, createInterface);
    }),
    tearDown: function() {
        if (otherBus) {
            otherBus.destroy();
            delete otherBus;
            otherBus = null;
        }
        bus.destroy();
        delete bus;
        bus = null;
    },

    testReplyError0: function(queue) {
        queue.call(function(callbacks) {
            var registerBusObject = function(err) {
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.samples.simple.SimpleInterface": {
                                              Ping: function(context, inStr) {
                                                  assertError(function() { context.replyError(); }, "TypeError");
                                                  context.replyError(1);
                                              }
                                          }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                otherBus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "Ping", "hello", callbacks.add(onPing));
            };
            var onPing = function(err) {
                assertEquals(1, err.code);
            };
            this._setUp(callbacks.add(registerBusObject));
        });
    },

    testReplyError1: function(queue) {
        queue.call(function(callbacks) {
            var registerBusObject = function(err) {
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.samples.simple.SimpleInterface": {
                                              Ping: function(context, inStr) {
                                                  assertNoError(function() { context.replyError(1); });
                                              }
                                          }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                otherBus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "Ping", "hello", callbacks.add(onPing));
            };
            var onPing = function(err) {
                assertEquals(1, err.code);
            };
            this._setUp(callbacks.add(registerBusObject));
        });
    },

    testReplyError2: function(queue) {
        queue.call(function(callbacks) {
            var registerBusObject = function(err) {
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.samples.simple.SimpleInterface": {
                                              Ping: function(context, inStr) {
                                                  assertNoError(function() { context.replyError("name", "message"); });
                                              }
                                          }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                otherBus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "Ping", "hello", callbacks.add(onPing));
            };
            var onPing = function(err) {
                assertEquals("name", err.name);
                assertEquals("message", err.message);
            };
            this._setUp(callbacks.add(registerBusObject));
        });
    },
    testReplyError3: function(queue) {
        queue.call(function(callbacks) {
            var registerBusObject = function(err) {
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.samples.simple.SimpleInterface": {
                                              Ping: function(context, inStr) {
                                                  assertNoError(function() { context.replyError("name"); });
                                              }
                                          }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                otherBus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "Ping", "hello", callbacks.add(onPing));
            };
            var onPing = function(err) {
                assertEquals("name", err.name);
                assertEquals("", err.message);
            };
            this._setUp(callbacks.add(registerBusObject));
        });
    },
    testReplyError4: function(queue) {
        queue.call(function(callbacks) {
            var registerBusObject = function(err) {
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.samples.simple.SimpleInterface": {
                                              Ping: function(context, inStr) {
                                                  assertNoError(function() { context.replyError("name", callbacks.add(onReplyError)); });
                                              }
                                          }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var onReplyError = function(err) {
                assertFalsy(err);
            }
            var getProxyObj = function(err) {
                assertFalsy(err);
                otherBus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "Ping", "hello", callbacks.add(onPing));
            };
            var onPing = function(err) {
                assertEquals("name", err.name);
                assertEquals("", err.message);
            };
            this._setUp(callbacks.add(registerBusObject));
        });
    },
    testReplyError5: function(queue) {
        queue.call(function(callbacks) {
            var registerBusObject = function(err) {
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.samples.simple.SimpleInterface": {
                                              Ping: function(context, inStr) {
                                                  assertNoError(function() { context.replyError("name", "message", callbacks.add(onReplyError)); });
                                              }
                                          }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var onReplyError = function(err) {
                assertFalsy(err);
            }
            var getProxyObj = function(err) {
                assertFalsy(err);
                otherBus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "Ping", "hello", callbacks.add(onPing));
            };
            var onPing = function(err) {
                assertEquals("name", err.name);
                assertEquals("message", err.message);
            };
            this._setUp(callbacks.add(registerBusObject));
        });
    },
});
