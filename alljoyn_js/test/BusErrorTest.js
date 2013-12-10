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

