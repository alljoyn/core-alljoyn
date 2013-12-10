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
AsyncTestCase("GameTest", {
    _setUp: ondeviceready(function(callback) {
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        bus.destroy();
    },

    testGame: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.PlayerState",
                    signal: [
                        { name: "PlayerPosition", signature: "uuu", argNames: "x,y,rotation" }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var player = {
                "org.alljoyn.bus.PlayerState": {}
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/game/player", player, false, callbacks.add(registerSignalHandler));
            };
            var registerSignalHandler = function(err) {
                assertFalsy(err);
                bus.registerSignalHandler(callbacks.add(onPlayerPosition), "org.alljoyn.bus.PlayerState.PlayerPosition", callbacks.add(getDbus));
            };
            var onPlayerPosition = function(context, x, y, rotation) {
                assertEquals(100, x);
                assertEquals(200, y);
                assertEquals(100, rotation);
            };
            var getDbus = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject("org.freedesktop.DBus/org/freedesktop/DBus", callbacks.add(requestName));
            };
            var requestName = function(err, dbus) {
                assertFalsy(err);
                dbus.methodCall("org.freedesktop.DBus", "RequestName", "org.alljoyn.bus.game", 0, callbacks.add(onRequestName));
            };
            var onRequestName = function(err, context, result) {
                assertFalsy(err);
                assertEquals(1, result);
                player.signal("org.alljoyn.bus.PlayerState", "PlayerPosition", 100, 200, 100, callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(connect));
        });
    }
});
