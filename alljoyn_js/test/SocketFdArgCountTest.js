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

