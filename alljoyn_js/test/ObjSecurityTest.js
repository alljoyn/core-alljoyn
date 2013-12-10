/*
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
AsyncTestCase("ObjSecurityTest", {
    _setUp: ondeviceready(function(callback) {
        bus = new org.alljoyn.bus.BusAttachment();
        var createInterface = function(err) {
            bus.createInterface({
                name: "test.SecureInterface",
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
                                  true,
                                  createOtherBus);
        };
        var createOtherBus = function(err) {
            otherBus = new org.alljoyn.bus.BusAttachment();
            otherBus.create(false, callback);
        };
        bus.create(false, createInterface);
    }),
    tearDown: function() {
        bus.destroy();
        otherBus.destroy();
    },

    testSrpKeyxPass: function(queue) {
        queue.call(function(callbacks) {
            var enablePeerSecurity = function(err) {
                bus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        assertEquals("ALLJOYN_SRP_KEYX", authMechanism);
                        assertEquals(otherBus.uniqueName, peerName);
                        assertEquals(1, authCount);
                        assertEquals(credentials.ONE_TIME_PWD, credMask);
                        credentials.password = "123456";
                        return true;
                    }),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
                        assertEquals("ALLJOYN_SRP_KEYX", authMechanism);
                        assertEquals(otherBus.uniqueName, peerName);
                        assertTrue(success);
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
                        assertEquals("ALLJOYN_SRP_KEYX", authMechanism);
                        assertEquals(bus.uniqueName, peerName);
                        assertEquals(1, authCount);
                        assertEquals(credentials.PASSWORD, credMask);
                        credentials.password = "123456";
                        return true;
                    }),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
                        assertEquals("ALLJOYN_SRP_KEYX", authMechanism);
                        assertEquals(bus.uniqueName, peerName);
                        assertTrue(success);
                    })
                }, callbacks.add(otherBusClearKeyStore));
            };
            var otherBusClearKeyStore = function(err) {
                assertFalsy(err);
                otherBus.clearKeyStore(callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                otherBus.getProxyBusObject(bus.uniqueName + "/test", callbacks.add(ping));
            };
            var ping = function(err, proxyObj) {
                assertFalsy(err);
                proxyObj.methodCall("test.SecureInterface", "Ping", "pong", callbacks.add(done));
            };
            var done = function(err, context, outStr) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(enablePeerSecurity));
        });
    },

    testSrpKeyxFail: function(queue) {
        queue.call(function(callbacks) {
            var enablePeerSecurity = function(err) {
                bus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        assertEquals("ALLJOYN_SRP_KEYX", authMechanism);
                        assertEquals(otherBus.uniqueName, peerName);
                        assertEquals(credentials.ONE_TIME_PWD, credMask);
                        credentials.password = "123456";
                        return true;
                    }, 2),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
                        assertEquals("", authMechanism);
                        assertEquals(otherBus.uniqueName, peerName);
                        assertFalse(success);
                    })
                }, clearKeyStore);
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
                        assertEquals("ALLJOYN_SRP_KEYX", authMechanism);
                        assertEquals(bus.uniqueName, peerName);
                        assertEquals(credentials.PASSWORD, credMask);
                        credentials.password = "654321";
                        return (authCount < 2) ? true : false;
                    }, 2),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
                        assertEquals("", authMechanism);
                        assertEquals(bus.uniqueName, peerName);
                        assertFalse(success);
                    }),
                    onSecurityViolation: callbacks.add(function(status, message) {
                        assertEquals(org.alljoyn.bus.BusError.AUTH_FAIL, status);
                    })
                }, callbacks.add(otherBusClearKeyStore));
            };
            var otherBusClearKeyStore = function(err) {
                assertFalsy(err);
                otherBus.clearKeyStore(callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                otherBus.getProxyBusObject(bus.uniqueName + "/test", callbacks.add(ping));
            };
            var ping = function(err, proxyObj) {
                assertFalsy(err);
                proxyObj.methodCall("test.SecureInterface", "Ping", "pong", callbacks.add(function(){}));
            };
            this._setUp(callbacks.add(enablePeerSecurity));
        });
    },

    testRsaKeyxPass: function(queue) {
        queue.call(function(callbacks) {
            var enablePeerSecurity = function(err) {
                bus.enablePeerSecurity("ALLJOYN_RSA_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        assertEquals("ALLJOYN_RSA_KEYX", authMechanism);
                        assertEquals(otherBus.uniqueName, peerName);
                        if (credentials.NEW_PASSWORD & credMask) {
                            credentials.password = "654321";
                        }
                        return true;
                    }, 2),
                    onVerify: callbacks.add(function(authMechanism, peerName, credentials) {
                        return true;
                    }),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
                        assertEquals("ALLJOYN_RSA_KEYX", authMechanism);
                        assertEquals(otherBus.uniqueName, peerName);
                        assertTrue(success);
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
                otherBus.enablePeerSecurity("ALLJOYN_RSA_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        assertEquals("ALLJOYN_RSA_KEYX", authMechanism);
                        assertEquals(bus.uniqueName, peerName);
                        if (credentials.NEW_PASSWORD & credMask) {
                            credentials.password = "123456";
                        }
                        return true;
                    }, 2),
                    onVerify: callbacks.add(function(authMechanism, peerName, credentials) {
                        return true;
                    }),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
                        assertEquals("ALLJOYN_RSA_KEYX", authMechanism);
                        assertEquals(bus.uniqueName, peerName);
                        assertTrue(success);
                    })
                }, callbacks.add(otherBusClearKeyStore));
            };
            var otherBusClearKeyStore = function(err) {
                assertFalsy(err);
                otherBus.clearKeyStore(callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                otherBus.getProxyBusObject(bus.uniqueName + "/test", callbacks.add(ping));
            };
            var ping = function(err, proxyObj) {
                assertFalsy(err);
                proxyObj.methodCall("test.SecureInterface", "Ping", "pong", callbacks.add(function(){}));
            };
            this._setUp(callbacks.add(enablePeerSecurity));
        });
    },

    testMultipleAuthMechanisms: function(queue) {
        queue.call(function(callbacks) {
            var enablePeerSecurity = function(err) {
                assertFalsy(err);
                bus.enablePeerSecurity("ALLJOYN_RSA_KEYX ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        assertEquals("ALLJOYN_SRP_KEYX", authMechanism);
                        assertEquals(otherBus.uniqueName, peerName);
                        assertEquals(credentials.ONE_TIME_PWD, credMask);
                        credentials.password = "123456";
                        return true;
                    }),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
                        assertEquals("ALLJOYN_SRP_KEYX", authMechanism);
                        assertEquals(otherBus.uniqueName, peerName);
                        assertTrue(success);
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
                        assertEquals("ALLJOYN_SRP_KEYX", authMechanism);
                        assertEquals(bus.uniqueName, peerName);
                        assertEquals(credentials.PASSWORD, credMask);
                        credentials.password = "123456";
                        return true;
                    }),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
                        assertEquals("ALLJOYN_SRP_KEYX", authMechanism);
                        assertEquals(bus.uniqueName, peerName);
                        assertTrue(success);
                    }),
                }, callbacks.add(otherBusClearKeyStore));
            };
            var otherBusClearKeyStore = function(err) {
                assertFalsy(err);
                otherBus.clearKeyStore(callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(getProxy));
            };
            var getProxy = function(err) {
                assertFalsy(err);
                otherBus.getProxyBusObject(bus.uniqueName + "/test", callbacks.add(ping));
            };
            var ping = function(err, proxyObj) {
                assertFalsy(err);
                proxyObj.methodCall("test.SecureInterface", "Ping", "pong", callbacks.add(function(){}));
            };
            this._setUp(callbacks.add(enablePeerSecurity));
        });
    },
});
