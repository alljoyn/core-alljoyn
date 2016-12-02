/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
AsyncTestCase("ProxyMethodTest", {
    _setUp: ondeviceready(function(callback) {
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        bus.destroy();
    },

    testNoReply: function(queue) {
        queue.call(function(callbacks) {
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.NoReply",
                    method: [
                        { name: 'Ping', signature: 's', argNames: 'inStr',
                          'org.freedesktop.DBus.Method.NoReply': true }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.NoReply": {
                                              Ping: callbacks.add(function(context, inStr) {})
                                          }
                                      },
                                      false,
                                      callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(getProxyBusObject));
            };
            var getProxyBusObject = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.NoReply", "Ping", "hello", { flags: 0x01 }, callbacks.add(done));
            };
            var done = function(err, context) {
                assertFalsy(err);
                assertUndefined(context.sender);
            };
            this._setUp(callbacks.add(createInterface));
        });
    },

    testFlags: function(queue) {
        queue.call(function(callbacks) {
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.Flags",
                    method: [
                        { name: 'Ping', signature: 's', returnSignature: 's', argNames: 'inStr,outStr' }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.Flags": {
                                              Ping: callbacks.add(function(context, inStr) {
                                                                      assertEquals(0x02, context.flags & 0x02);
                                                                      context.reply(inStr);
                                                                  })
                                           }
                                      },
                                      false,
                                      callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(getProxyBusObject));
            };
            var getProxyBusObject = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.Flags", "Ping", "hello", { flags: 0x02 }, callbacks.add(onPing));
            };
            var onPing = function(err, context, outStr) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(createInterface));
        });
    },

    testTimeout: function(queue) {
        queue.call(function(callbacks) {
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.Timeout",
                    method: [
                        { name: 'Ping', signature: 's', returnSignature: 's', argNames: 'inStr,outStr' }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.Timeout": {
                                          Ping: function(context, inStr) { /* no reply */ }
                                          }
                                      },
                                      false,
                                      callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(getProxyBusObject));
            };
            var getProxyBusObject = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.Timeout", "Ping", "hello", { timeout: 10 }, callbacks.add(onPing));
            };
            var onPing = function(err, context, outStr) {
                assertEquals("org.alljoyn.Bus.Timeout", err.name);
            };
            this._setUp(callbacks.add(createInterface));
        });
    },
});