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
AsyncTestCase("SocketFdArgCountTest", {
    _setUp: ondeviceready(function(callback) {
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    _wrap: function(queue, f) {
        queue.call(function(callbacks) {
            this._setUp(callbacks.add(f));
        });
    },
    tearDown: function() {
        bus.destroy();
    },

    testConstructor0: function(queue) {
        this._wrap(queue, function(err) {
            assertError(function() { new org.alljoyn.bus.SocketFd(); }, "TypeError");
        });
    },

    testRecv0: function(queue) {
        this._wrap(queue, function(err) {
            var socket = new org.alljoyn.bus.SocketFd(-1);
            assertError(function() { socket.recv(); }, "TypeError");
        });
    },

    testSend0: function(queue) {
        this._wrap(queue, function(err) {
            var socket = new org.alljoyn.bus.SocketFd(-1);
            assertError(function() { socket.send(); }, "TypeError");
        });
    },
});
