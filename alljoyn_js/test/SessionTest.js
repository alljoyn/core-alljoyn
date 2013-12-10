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
AsyncTestCase("SessionTest", {
    _setUp: ondeviceready(function(callback) {
        otherBus = undefined;
        otherBus1 = undefined;
        otherBus2 = undefined;
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        otherBus && otherBus.destroy();
        otherBus1 && otherBus1.destroy();
        otherBus2 && otherBus2.destroy();
        bus.destroy();
    },

    testBindUnbind: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(bindDefaults));
            };
            var bindDefaults = function(err) {
                assertFalsy(err);
                /* Use defaults */
                var sessionOpts = {};
                bus.bindSessionPort(sessionOpts, callbacks.add(unbindDefaults));
            };
            var unbindDefaults = function(err, port) {
                assertFalsy(err);
                bus.unbindSessionPort(port, callbacks.add(bind));
            };
            var bind = function(err) {
                assertFalsy(err);
                /* Specify parameters */
                var sessionOpts = { port: 0,
                                    traffic: 1,
                                    isMultipoint: false,
                                    proximity: 0xff,
                                    transports: 0xffff };
                bus.bindSessionPort(sessionOpts, callbacks.add(unbind));
            };
            var unbind = function(err, port) {
                assertFalsy(err);
                bus.unbindSessionPort(port, callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testJoinAcceptJoinedLeaveLost: function(queue) {
        queue.call(function(callbacks) {
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(otherBusBind));
            };
            var otherBusBind = function(err) {
                var onAccept = function(port, joiner, opts) {
                    return true;
                };
                var onJoined = function(port, id, joiner) {
                    /* There is a joiner, so no need to be bound anymore. */
                    var setSessionListener = function(err) {
                        otherBus.setSessionListener(id, { onLost: callbacks.add(onLost) }, callbacks.add(leaveSession));
                    };
                    var onLost = function(id, reason) {
                        assertEquals(reason, org.alljoyn.bus.SessionLostReason.REMOTE_END_LEFT_SESSION)
                    }
                    var leaveSession = function(err) {
                        bus.leaveSession(id, callbacks.add(function(err) { assertFalsy(err); }));
                    };
                    otherBus.unbindSessionPort(port, callbacks.add(setSessionListener));
                };
                var sessionOpts = { onAccept: callbacks.add(onAccept),
                                    onJoined: callbacks.add(onJoined) };
                otherBus.bindSessionPort(sessionOpts, callbacks.add(connect));
            };
            var port;
            var connect = function(err, sessionPort) {
                assertFalsy(err);
                port = sessionPort;
                bus.connect(callbacks.add(joinSession));
            };
            var joinSession = function(err) {
                assertFalsy(err);
                bus.joinSession({ host: otherBus.uniqueName,
                                  port: port },
                                callbacks.add(onJoinSession));
            };
            var onJoinSession = function(err, id, opts) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(otherBusCreate));
        });
    },

    testRemoveSessionMember: function(queue) {
        queue.call(function(callbacks) {
            var joined = false;
            var sessionListenerSet = false;
            var sessionId = 0;
            var port;

            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(bind));
            };
            var bind = function(err) {
                var onAccept = function(port, joiner, opts) {
                    return true;
                };
                var onJoined = function(port, id, joiner) {
                    sessionId = id;
                    var onLost = function(id, reason) {
                        assertEquals(reason, org.alljoyn.bus.SessionLostReason.REMOTE_END_LEFT_SESSION);
                    }
                    var listenerSet = function(err) {
                        assertFalsy(err);
                        sessionListenerSet = true;
                        removeSessionMember1();
                    };
                    bus.setSessionListener(id, { onLost: callbacks.add(onLost) }, callbacks.add(listenerSet));
                };
                var sessionOpts = { onAccept: callbacks.add(onAccept, 2),
                                    onJoined: callbacks.add(onJoined),
                                    isMultipoint: true };
                bus.bindSessionPort(sessionOpts, callbacks.add(createOtherBus1));
            };

            var createOtherBus1 = function(err, sessionPort) {
                assertFalsy(err);
                port = sessionPort;
                otherBus1 = new org.alljoyn.bus.BusAttachment();
                otherBus1.create(false, callbacks.add(createOtherBus2));
            };
            var createOtherBus2 = function(err) {
                assertFalsy(err);
                otherBus2 = new org.alljoyn.bus.BusAttachment();
                otherBus2.create(false, callbacks.add(connectOtherBus1));
            };
            var connectOtherBus1 = function(err) {
                assertFalsy(err);
                otherBus1.connect(callbacks.add(connectOtherBus2));
            };
            var connectOtherBus2 = function(err) {
                assertFalsy(err);
                otherBus2.connect(callbacks.add(joinSessionOtherBus1));
            };
            var joinSessionOtherBus1 = function(err) {
                assertFalsy(err);
                otherBus1.joinSession({ host: bus.uniqueName,
                                        port: port },
                                      callbacks.add(setSessionListener1));
            };
            var setSessionListener1 = function(err, id, opt) {
                assertFalsy(err);
                otherBus1.setSessionListener(id, { onLost: callbacks.add(onLostOtherBus1) }, callbacks.add(joinSessionOtherBus2));
            };
            var joinSessionOtherBus2 = function(err) {
                assertFalsy(err);
                otherBus2.joinSession({ host: bus.uniqueName,
                                        port: port },
                                      callbacks.add(setSessionListener2));
            };
            var setSessionListener2 = function(err, id, opt) {
                assertFalsy(err);
                joined = true;
                otherBus2.setSessionListener(id, { onLost: callbacks.add(onLostOtherBus2) }, callbacks.add(onListenersSet));
            };
            var onListenersSet = function(err) {
                assertFalsy(err);
                removeSessionMember1();
            };
            var onLostOtherBus1 = function(id, reason) {
                assertEquals(reason, org.alljoyn.bus.SessionLostReason.REMOVED_BY_BINDER);
            };
            var onLostOtherBus2 = function(id, reason) {
                assertEquals(reason, org.alljoyn.bus.SessionLostReason.REMOVED_BY_BINDER);
            };
            var removeSessionMember1 = function(err) {
                if (joined && sessionListenerSet) {
                    bus.removeSessionMember(sessionId, otherBus1.uniqueName, callbacks.add(removeSessionMember2));
                }
            };
            var removeSessionMember2 = function(err) {
                assertFalsy(err);
                bus.removeSessionMember(sessionId, otherBus2.uniqueName, callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
                bus.unbindSessionPort(port, callbacks.add(function(err) { assertFalsy(err); }));
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testSetLinkTimeout: function(queue) {
        queue.call(function(callbacks) {
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(bindSessionPort));
            };
            var bindSessionPort = function(err) {
                var onAccept = function(port, joiner, opts) {
                    return true;
                };
                var onJoined = function(port, id, joiner) {
                    /* There is a joiner, so no need to be bound anymore. */
                    otherBus.unbindSessionPort(port, callbacks.add(function(err) { assertFalsy(err); }));
                };
                var onLost = function(id, reason) {
                };
                var sessionOpts = { onAccept: callbacks.add(onAccept),
                                    onJoined: callbacks.add(onJoined),
                                    onLost: callbacks.add(onLost) };
                otherBus.bindSessionPort(sessionOpts, callbacks.add(connect));
            };
            var port;
            var connect = function(err, sessionPort) {
                assertFalsy(err);
                port = sessionPort;
                bus.connect(callbacks.add(joinSession));
            };
            var joinSession = function(err) {
                assertFalsy(err);
                bus.joinSession({ host: otherBus.uniqueName,
                                  port: port }, callbacks.add(onJoinSession));
            };
            var id;
            var onJoinSession = function(err, sessionId, opts) {
                assertFalsy(err);
                id = sessionId;
                setTimeout(function() { bus.setLinkTimeout(id, 10, callbacks.add(leaveSession)); }, 30);
            };
            var leaveSession = function(err, timeout) {
                assertFalsy(err);
                assertEquals(10, timeout);
                bus.leaveSession(id, callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(otherBusCreate));
        });
    },

    testJoinerMemberAddedRemoved: function(queue) {
        queue.call(function(callbacks) {
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(bindSessionPort));
            };
            var onAccept = function(port, joiner, opts) {
                return true;
            };
            var onJoined = function(port, id, joiner) {
                /* There is a joiner, so no need to be bound anymore. */
                otherBus.unbindSessionPort(port, callbacks.add(function(err) { assertFalsy(err); }));
            };
            var bindSessionPort = function(err) {
                assertFalsy(err);
                var sessionOpts = { isMultipoint: true,
                                    onAccept: callbacks.add(onAccept),
                                    onJoined: callbacks.add(onJoined) };
                otherBus.bindSessionPort(sessionOpts, callbacks.add(connect));
            };
            var port;
            var connect = function(err, sessionPort) {
                assertFalsy(err);
                port = sessionPort;
                bus.connect(callbacks.add(joinSession));
            };
            var onMemberAdded = function(id, uniqueName) {
                otherBus.leaveSession(id, callbacks.add(function(err) { assertFalsy(err); }));
            };
            var onMemberRemoved = function(id, uniqueName) {
            };
            var joinSession = function(err) {
                assertFalsy(err);
                bus.joinSession({ host: otherBus.uniqueName,
                                  port: port,
                                  onMemberAdded: callbacks.add(onMemberAdded),
                                  onMemberRemoved: callbacks.add(onMemberRemoved) }, callbacks.add(onJoinSession));
            };
            var onJoinSession = function(err, id, opts) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(otherBusCreate));
        });
    },

    testBinderMemberAddedRemoved: function(queue) {
        queue.call(function(callbacks) {
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(bindSessionPort));
            };
            var onAccept = function(port, joiner, opts) {
                return true;
            };
            var onJoined = function(port, id, joiner) {
            };
            var onMemberAdded = function(id, uniqueName) {
            };
            var onMemberRemoved = function(id, uniqueName) {
            };
            var bindSessionPort = function(err) {
                assertFalsy(err);
                var sessionOpts = { isMultipoint: true,
                                    onAccept: callbacks.add(onAccept),
                                    onJoined: callbacks.add(onJoined),
                                    onMemberAdded: callbacks.add(onMemberAdded),
                                    onMemberRemoved: callbacks.add(onMemberRemoved) };
                otherBus.bindSessionPort(sessionOpts, callbacks.add(connect));
            };
            var port;
            var connect = function(err, sessionPort) {
                assertFalsy(err);
                port = sessionPort;
                bus.connect(callbacks.add(joinSession));
            };
            var joinSession = function(err) {
                assertFalsy(err);
                bus.joinSession({ host: otherBus.uniqueName,
                                  port: port }, callbacks.add(onJoinSession));
            };
            var onJoinSession = function(err, id, opts) {
                assertFalsy(err);
                bus.leaveSession(id, callbacks.add(function(err) { assertFalsy(err); }));
            };
            this._setUp(callbacks.add(otherBusCreate));
        });
    },


    testBinderMemberAddedRemoved2: function(queue) {
        // Regression test for ALLJOYN-1938
        queue.call(function(callbacks) {
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(bindSessionPort));
            };
            var onAccept = function(port, joiner, opts) {
                return true;
            };
            var onJoined = function(port, id, joiner) {
            };
            var onMemberAdded = function(id, uniqueName) {
            };
            var onMemberRemoved = function(id, uniqueName) {
            };
            var bindSessionPort = function(err) {
                assertFalsy(err);
                var sessionOpts = { isMultipoint: true,
                                    onAccept: callbacks.add(onAccept, 2),
                                    onJoined: callbacks.add(onJoined, 2),
                                    onMemberAdded: callbacks.add(onMemberAdded, 2),
                                    onMemberRemoved: callbacks.add(onMemberRemoved, 2) };
                otherBus.bindSessionPort(sessionOpts, callbacks.add(connect));
            };
            var port;
            var connect = function(err, sessionPort) {
                assertFalsy(err);
                port = sessionPort;
                bus.connect(callbacks.add(joinSession));
            };
            var joinSession = function(err) {
                assertFalsy(err);
                bus.joinSession({ host: otherBus.uniqueName,
                                  port: port }, callbacks.add(onJoinSession));
            };
            var onJoinSession = function(err, id, opts) {
                assertFalsy(err);
                bus.leaveSession(id, callbacks.add(joinSession2));
            };
            var joinSession2 = function(err) {
                assertFalsy(err);
                bus.joinSession({ host: otherBus.uniqueName,
                                  port: port }, callbacks.add(onJoinSession2));
            };
            var onJoinSession2 = function(err, id, opts) {
                assertFalsy(err);
                bus.leaveSession(id, callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(otherBusCreate));
        });
    },

});
