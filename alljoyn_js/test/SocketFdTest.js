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
AsyncTestCase("SocketFdTest", {
    _setUp: ondeviceready(function(callback) {
        otherBus = undefined;
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        otherBus && otherBus.destroy();
        bus.destroy();
    },

    testConstructor: function(queue) {
        assertNotNull(new org.alljoyn.bus.SocketFd(-1));
        assertNotNull(new org.alljoyn.bus.SocketFd("-1"));
    },

    testFd: function(queue) {
        var socket = new org.alljoyn.bus.SocketFd(-1);
        assertEquals("-1", socket.fd);
        assertEquals(-1, socket.fd);
    },

    testClose: function(queue) {
        var socket = new org.alljoyn.bus.SocketFd(-1);
        socket.close();
    },

    testShutdown: function(queue) {
        var socket = new org.alljoyn.bus.SocketFd(-1);
        /* This should be an error since the socket isn't open. */
        assertNotEquals(0, socket.shutdown());
    },

    testSendRecv: function(queue) {
        queue.call(function(callbacks) {
            var SESSION_PORT = 111;

            var startSession = function() {
                var connect = function(err) {
                    assertFalsy(err);
                    bus.connect();
                };
                assertEquals(0, bus.bindSessionPort({
                    port: SESSION_PORT,
                    traffic: org.alljoyn.bus.SessionOpts.TRAFFIC_RAW_RELIABLE,
                    transport: org.alljoyn.bus.SessionOpts.TRANSPORT_LOCAL,
                    onAccept: function(port, joiner, opts) {
                        return true;
                    },
                    onJoined: callbacks.add(function(port, id, opts) {
                        var fd = bus.getSessionFd(id);
                        assertEquals(4, fd.send([1, 2, 3, 4]));
                    })
                }));
            };

            var joinSession = function() {
                otherBus = new org.alljoyn.bus.BusAttachment();
                var onJoinSession = callbacks.add(function(id, opts) {
                    var fd = otherBus.getSessionFd(id);
                    var buf = new Array(4);
                    assertEquals(4, fd.recv(buf));
                    assertEquals([1, 2, 3, 4], buf);
                });
                var otherBusConnect = function(err) {
                    assertFalsy(err);
                    otherBus.connect();
                };
                assertEquals(0, otherBus.joinSession({
                    host: bus.uniqueName,
                    port: SESSION_PORT,
                    traffic: org.alljoyn.bus.SessionOpts.TRAFFIC_RAW_RELIABLE
                }, callbacks.add(onJoinSession)));
            };

            startSession();
            joinSession();
        });
    },
});

