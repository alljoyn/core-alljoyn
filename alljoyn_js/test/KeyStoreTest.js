/*
 * Copyright AllSeen Alliance. All rights reserved.
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
AsyncTestCase("KeyStoreTest", {
    _setUp: ondeviceready(function(callback) {
        var createInterface = function(err) {
            bus.createInterface({
                name: "test.SecureInterface",
                secure: true,
                method: [ { name: 'Ping', signature: 's', returnSignature: 's' } ]
            }, registerBusObject);
        };
        var registerBusObject = function(err) {
            bus.registerBusObject("/test",
                                  {
                                      "test.SecureInterface": {
                                          Ping: function(context, inStr) { context.reply(inStr); }
                                      }
                                  },
                                  false,
                                  otherBusCreate);
        };
        var otherBusCreate = function(err) {
            otherBus = new org.alljoyn.bus.BusAttachment();
            otherBus.create(false, callback);
        };
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, createInterface);
    }),
    tearDown: function() {
        otherBus.destroy();
        bus.destroy();
    },

    testReloadKeyStore: function(queue) {
        queue.call(function(callbacks) {
            var enablePeerSecurity = function(err) {
                assertFalsy(err);
                bus.enablePeerSecurity("ALLJOYN_SRP_KEYX", callbacks.add(reloadKeyStore));
            };
            var reloadKeyStore = function(err) {
                assertFalsy(err);
                bus.reloadKeyStore(callbacks.add(function(err) { assertFalsy(err); }));
            };
            this._setUp(callbacks.add(enablePeerSecurity));
        });
    },

    testClearKeys: function(queue) {
        queue.call(function(callbacks) {
            var enablePeerSecurity = function(err) {
                assertFalsy(err);
                bus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        credentials.password = "123456";
                        return true;
                    })
                }, callbacks.add(clearKeyStore));
            };
            var clearKeyStore = function(err) {
                assertFalsy(err);
                bus.clearKeyStore(callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(otherBusEnablePeerSecurity));
            };

            var otherBusEnablePeerSecurity = function(err) {
                assertFalsy(err);
                otherBus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        credentials.password = "123456";
                        return true;
                    })
                }, callbacks.add(otherBusClearKeyStore));
            };
            var otherBusClearKeyStore = function(err) {
                assertFalsy(err);
                otherBus.clearKeyStore(callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(otherBusGetProxyBusObject));
            };

            var otherBusGetProxyBusObject = function(err) {
                otherBus.getProxyBusObject(bus.uniqueName + "/test", callbacks.add(ping));
            };
            var ping = function(err, proxy) {
                proxy.methodCall("test.SecureInterface", "Ping", "pong", callbacks.add(otherBusGetPeerGUID));
            };
            var otherBusGetPeerGUID = function(err, context, outStr) {
                otherBus.getPeerGUID(bus.uniqueName, callbacks.add(otherBusClearKeys));
            };
            var otherBusClearKeys = function(err, guid) {
                otherBus.clearKeys(guid, callbacks.add(function(err) { assertFalsy(err); }));
            };
            this._setUp(callbacks.add(enablePeerSecurity));
        });
    },

    testSetGetKeyExpiration: function(queue) {
        queue.call(function(callbacks) {
            var enablePeerSecurity = function(err) {
                assertFalsy(err);
                bus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        credentials.password = "123456";
                        return true;
                    })
                }, callbacks.add(clearKeyStore));
            };
            var clearKeyStore = function(err) {
                assertFalsy(err);
                bus.clearKeyStore(callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(otherBusEnablePeerSecurity));
            };

            var otherBusEnablePeerSecurity = function(err) {
                assertFalsy(err);
                otherBus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        credentials.password = "123456";
                        return true;
                    })
                }, callbacks.add(otherBusClearKeyStore));
            };
            var otherBusClearKeyStore = function(err) {
                assertFalsy(err);
                otherBus.clearKeyStore(callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(otherBusGetProxyBusObject));
            };

            var otherBusGetProxyBusObject = function(err) {
                otherBus.getProxyBusObject(bus.uniqueName + "/test", callbacks.add(ping));
            };
            var ping = function(err, proxy) {
                proxy.methodCall("test.SecureInterface", "Ping", "pong", callbacks.add(otherBusGetPeerGUID));
            };
            var otherBusGetPeerGUID = function(err, context, outStr) {
                otherBus.getPeerGUID(bus.uniqueName, callbacks.add(otherBusSetKeyExpiration));
            };
            var guid;
            var otherBusSetKeyExpiration = function(err, g) {
                guid = g;
                otherBus.setKeyExpiration(guid, 100, callbacks.add(otherBusGetKeyExpiration));
            };
            var otherBusGetKeyExpiration = function(err) {
                otherBus.getKeyExpiration(guid, callbacks.add(done));
            };
            var done = function(err, keyExpiration) {
                assertEquals(100, keyExpiration);
            };
            this._setUp(callbacks.add(enablePeerSecurity));
        });
    },

    testCredentialsExpiration: function(queue) {
        queue.call(function(callbacks) {
            var enablePeerSecurity = function(err) {
                assertFalsy(err);
                bus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        credentials.password = "123456";
                        assertUndefined(credentials.expiration);
                        credentials.expiration = 100;
                        assertEquals(100, credentials.expiration);
                        return true;
                    })
                }, callbacks.add(clearKeyStore));
            };
            var clearKeyStore = function(err) {
                assertFalsy(err);
                bus.clearKeyStore(callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(otherBusEnablePeerSecurity));
            };

            var otherBusEnablePeerSecurity = function(err) {
                assertFalsy(err);
                otherBus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        credentials.password = "123456";
                        assertUndefined(credentials.expiration);
                        return true;
                    })
                }, callbacks.add(otherBusClearKeyStore));
            };
            var otherBusClearKeyStore = function(err) {
                assertFalsy(err);
                otherBus.clearKeyStore(callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(otherBusGetProxyBusObject));
            };

            var otherBusGetProxyBusObject = function(err) {
                otherBus.getProxyBusObject(bus.uniqueName + "/test", callbacks.add(ping));
            };
            var ping = function(err, proxy) {
                proxy.methodCall("test.SecureInterface", "Ping", "pong", callbacks.add(otherBusGetPeerGUID));
            };
            var otherBusGetPeerGUID = function(err, context, outStr) {
                otherBus.getPeerGUID(bus.uniqueName, callbacks.add(otherBusGetKeyExpiration));
            };
            var otherBusGetKeyExpiration = function(err, guid) {
                otherBus.getKeyExpiration(guid, callbacks.add(done));
            };
            var done = function(err, keyExpiration) {
                assertTrue((100 - keyExpiration) < 5);
            };
            this._setUp(callbacks.add(enablePeerSecurity));
        });
    },

    testAddLogonEntry: function(queue) {
        queue.call(function(callbacks) {
            var enablePeerSecurity = function(err) {
                assertFalsy(err);
                bus.enablePeerSecurity("ALLJOYN_SRP_LOGON", callbacks.add(clearKeyStore));
            };
            var clearKeyStore = function(err) {
                assertFalsy(err);
                bus.clearKeyStore(callbacks.add(addLogonEntry));
            };
            var addLogonEntry = function(err) {
                assertFalsy(err);
                bus.addLogonEntry("ALLJOYN_SRP_LOGON", "userName", "password", callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(otherBusCreate));
            };
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusEnablePeerSecurity));
            };
            var otherBusEnablePeerSecurity = function(err) {
                assertFalsy(err);
                otherBus.enablePeerSecurity("ALLJOYN_SRP_LOGON", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        assertEquals((credentials.PASSWORD | credentials.USER_NAME), credMask);
                        credentials.userName = "userName";
                        credentials.password = "password";
                        return true;
                    })
                }, callbacks.add(otherBusClearKeyStore));
            };
            var otherBusClearKeyStore = function(err) {
                assertFalsy(err);
                otherBus.clearKeyStore(callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(otherBusGetProxyBusObject));
            };

            var otherBusGetProxyBusObject = function(err) {
                otherBus.getProxyBusObject(bus.uniqueName + "/test", callbacks.add(ping));
            };
            var ping = function(err, proxy) {
                proxy.methodCall("test.SecureInterface", "Ping", "pong", callbacks.add(function(err) { assertFalsy(err); }));
            };
            this._setUp(callbacks.add(enablePeerSecurity));
        });
    },
});
