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
AsyncTestCase("BusErrorTest", {
    _setUp: ondeviceready(function(callback) {
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        bus.destroy();
    },

    testBusError: function(queue) {
        queue.call(function(callbacks) {
            var nameHasOwner = function(err) {
                assertFalsy(err);
                bus.nameHasOwner("test.foo", callbacks.add(done));
            };
            var done = function(err, has) {
                assertEquals("BusError", err.name);
                assertEquals(org.alljoyn.bus.BusError.BUS_NOT_CONNECTED, err.code);
            };
            this._setUp(callbacks.add(nameHasOwner));
        });
    },

    testTypeError: function(queue) {
        queue.call(function(callbacks) {
            var registerSignalHandler = function(err) {
                try {
                    bus.registerSignalHandler();
                } catch (err) {
                    assertEquals("TypeError", org.alljoyn.bus.BusError.name);
                    assertEquals("not enough arguments", org.alljoyn.bus.BusError.message);
                }
            };
            this._setUp(callbacks.add(registerSignalHandler));
        });
    },
});
