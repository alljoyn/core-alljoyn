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
AsyncTestCase("BusAttachmentTest", {
    _setUp: ondeviceready(function(callback) {
        bus = new org.alljoyn.bus.BusAttachment();
        otherBus = undefined;
        bus.create(false, callback);
    }),
    tearDown: function() {
        bus.destroy();
        otherBus && otherBus.destroy();
    },

    testDisconnectedAttributes: function(queue) {
        queue.call(function(callbacks) {
            var timestamp = function(err) {
                assertNull(bus.uniqueName);
                assertNotNull(bus.globalGUIDString);
                bus.getTimestamp(callbacks.add(done));
            };
            var done = function(err, ts) {
                assertNotNull(ts);
            };
            this._setUp(callbacks.add(timestamp));
        });
    },

    testConnectedAttributes: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                bus.connect(callbacks.add(uniqueName));
            };
            var uniqueName = function(err) {
                assertFalsy(err);
                assertNotNull(bus.uniqueName);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testPeerSecurityEnabled: function(queue) {
        queue.call(function(callbacks) {
            var getDisabled = function(err) {
                bus.getPeerSecurityEnabled(callbacks.add(disabled));
            };
            var disabled = function(err, enabled) {
                assertFalsy(err);
                assertFalse(enabled);
                bus.enablePeerSecurity("ALLJOYN_SRP_KEYX", callbacks.add(getEnabled));
            };
            var getEnabled = function(err) {
                assertFalsy(err);
                bus.getPeerSecurityEnabled(callbacks.add(enabled));
            };
            var enabled = function(err, enabled) {
                assertFalsy(err);
                assertTrue(enabled);
            };
            this._setUp(callbacks.add(getDisabled));
        });
    },

    testRegisterUnregisterBusObject: function(queue) {
        queue.call(function(callbacks) {
            var register = function(err) {
                bus.registerBusObject("/busObject", {}, false, callbacks.add(unregister));
            };
            var unregister = function(err) {
                assertFalsy(err);
                bus.unregisterBusObject("/busObject", callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(register));
        });
    },

    testConnect: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                bus.connect(callbacks.add(disconnect));
            };
            var disconnect = function(err) {
                assertFalsy(err);
                bus.disconnect(callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testConstructor: function(queue) {
        assertNotNull(new org.alljoyn.bus.BusAttachment());
    },

    testEnumerate: function(queue) {
        queue.call(function(callbacks) {
            var property = {};
            var enumObject = function(err) {
                for (var n in bus) {
                    property[n] = true;
                }
                assertTrue(property["globalGUIDString"]);
                assertTrue(property["uniqueName"]);
                assertTrue(property["connect"]);
                assertTrue(property["disconnect"]);
                assertTrue(property["registerSignalHandler"]);
                assertTrue(property["unregisterSignalHandler"]);

                bus.createInterface({ name: "org.sample.Foo" }, callbacks.add(getInterfaces));
            };
            var getInterfaces = function(err) {
                assertFalsy(err);
                bus.getInterfaces(callbacks.add(enumInterfaces));
            };
            var enumInterfaces = function(err, interfaces) {
                assertFalsy(err);
                property = {};
                for (var i = 0; i < interfaces.length; ++i) {
                    property[interfaces[i].name] = true;
                }
                assertTrue(property["org.alljoyn.Bus"]);
                assertTrue(property["org.alljoyn.Bus.Peer.Authentication"]);
                assertTrue(property["org.alljoyn.Bus.Peer.HeaderCompression"]);
                assertTrue(property["org.freedesktop.DBus"]);
                assertTrue(property["org.freedesktop.DBus.Introspectable"]);
                assertTrue(property["org.freedesktop.DBus.Peer"]);
                assertTrue(property["org.freedesktop.DBus.Properties"]);
                assertTrue(property["org.sample.Foo"]);
            };
            this._setUp(callbacks.add(enumObject));
        });
    },

    testCreateGetInterfaceDescription: function(queue) {
        queue.call(function(callbacks) {
            var interfaceDescription = { name: "test.interface" };
            var createInterface = function(err) {
                bus.createInterface(interfaceDescription, callbacks.add(getInterface));
            };
            var getInterface = function(err) {
                assertFalsy(err);
                bus.getInterface(interfaceDescription.name, callbacks.add(equals));
            };
            var equals = function(err, intf) {
                assertFalsy(err);
                assertEquals(interfaceDescription, intf);
            };
            this._setUp(callbacks.add(createInterface));
        });
    },

    testCreateGetInterfaceDescription2: function(queue) {
        queue.call(function(callbacks) {
            var interfaceDescription = { name: "test.interface", secPolicy: org.alljoyn.bus.IfcSecurity.REQUIRED };
            var createInterface = function(err) {
                bus.createInterface(interfaceDescription, callbacks.add(getInterface));
            };
            var getInterface = function(err) {
                assertFalsy(err);
                bus.getInterface(interfaceDescription.name, callbacks.add(equals));
            };
            var equals = function(err, intf) {
                assertFalsy(err);
                assertEquals(interfaceDescription, intf);
            };
            this._setUp(callbacks.add(createInterface));
        });
    },


    testCreateGetInterfaceDescription3: function(queue) {
        queue.call(function(callbacks) {
            var interfaceDescription = { name: "test.interface", secPolicy: org.alljoyn.bus.IfcSecurity.OFF };
            var createInterface = function(err) {
                bus.createInterface(interfaceDescription, callbacks.add(getInterface));
            };
            var getInterface = function(err) {
                assertFalsy(err);
                bus.getInterface(interfaceDescription.name, callbacks.add(equals));
            };
            var equals = function(err, intf) {
                assertFalsy(err);
                assertEquals(interfaceDescription, intf);
            };
            this._setUp(callbacks.add(createInterface));
        });
    },

    testProxy: function(queue) {
        queue.call(function(callbacks) {
            var getProxyObj = function(err) {
                bus.getProxyBusObject("org.freedesktop.DBus/org/freedesktop/DBus", callbacks.add(done));
            };
            var done = function(err, proxyObj) {
                assertFalsy(err);
                assertNotNull(proxyObj);
            };
            this._setUp(callbacks.add(getProxyObj));
        });
    },

    testRegisterUnregisterMultipleSignalHandlers: function(queue) {
        queue.call(function(callbacks) {
            var onNameOwnerChanged0 = callbacks.add(function(context, name, oldOwner, newOwner) {
                bus.unregisterSignalHandler(onNameOwnerChanged0, "org.freedesktop.DBus.NameOwnerChanged", callbacks.add(function(err) {
                    assertFalsy(err);
                }));
            });
            var onNameOwnerChanged1 = callbacks.add(function(context, name, oldOwner, newOwner) {
                bus.unregisterSignalHandler(onNameOwnerChanged1, "org.freedesktop.DBus.NameOwnerChanged", callbacks.add(function(err) {
                    assertFalsy(err);
                }));
            });

            var connect = function(err) {
                bus.connect(callbacks.add(registerSignalHandler));
            };
            var registerSignalHandler = function(err) {
                assertFalsy(err);
                bus.registerSignalHandler(onNameOwnerChanged0, "org.freedesktop.DBus.NameOwnerChanged", callbacks.add(function(err) {
                    assertFalsy(err);
                    bus.registerSignalHandler(onNameOwnerChanged1, "org.freedesktop.DBus.NameOwnerChanged", callbacks.add(otherBusCreate));
                }))};
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                /* This will cause a NameOwnerChanged signal to be fired. */
                otherBus.connect(callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testSignalHandler: function(queue) {
        queue.call(function(callbacks) {
            var onNameOwnerChanged = callbacks.add(function(context, name, oldOwner, newOwner) {
                assertEquals("NameOwnerChanged", context.memberName);
            });

            var connect = function(err) {
                bus.connect(callbacks.add(registerSignalHandler));
            };
            var registerSignalHandler = function(err) {
                assertFalsy(err);
                bus.registerSignalHandler(onNameOwnerChanged, "org.freedesktop.DBus.NameOwnerChanged", callbacks.add(otherBusCreate));
            };
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                /* This will cause a NameOwnerChanged signal to be fired. */
                otherBus.connect(callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testSameHandlerDifferentSignals: function(queue) {
        queue.call(function(callbacks) {
            var busObject = {
                "org.alljoyn.bus.PlayerState": {}
            };
            var connect = function(err) {
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.PlayerState",
                    signal: [
                        { name: "PlayerPosition", signature: "uuu", argNames: "x,y,rotation" },
                        { name: "PlayerVelocity", signature: "ii", argNames: "dx,xy" }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/game/player", busObject, false, callbacks.add(registerSignalHandler));
            };
            var registerSignalHandler = function(err) {
                assertFalsy(err);
                var signal = {};
                var onPlayer = callbacks.add(function(context) {
                    signal[context.memberName] = true;
                    if (signal.PlayerPosition && signal.PlayerVelocity) {
                        assertTrue(true);
                    }
                }, 2);
                bus.registerSignalHandler(onPlayer, "org.alljoyn.bus.PlayerState.PlayerPosition", callbacks.add(function(err) {
                    bus.registerSignalHandler(onPlayer, "org.alljoyn.bus.PlayerState.PlayerVelocity", callbacks.add(emitSignal));
                }));
            };
            var emitSignal = function(err) {
                assertFalsy(err);
                busObject.signal("org.alljoyn.bus.PlayerState", "PlayerPosition", 100, 200, 300, callbacks.add(function(err) {
                    assertFalsy(err);
                    busObject.signal("org.alljoyn.bus.PlayerState", "PlayerVelocity", +30, -40, callbacks.add(done));
                }));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testBroadcastSignal: function(queue) {
        queue.call(function(callbacks) {
            var busObject = {
                "org.alljoyn.bus.samples.aroundme.chat": {}
            };
            var connect = function(err) {
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.samples.aroundme.chat",
                    signal: [
                        { name: 'Chat', signature: 'ss', argNames: 'sender,text' }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/chatService", busObject, false, callbacks.add(registerSignalHandler));
            };
            var registerSignalHandler = function(err) {
                assertFalsy(err);
                var onChat = callbacks.add(function(context, sender, text) {
                    assertEquals(32, context.flags);
                });
                bus.registerSignalHandler(onChat, "org.alljoyn.bus.samples.aroundme.chat.Chat", callbacks.add(emitSignal));
            };
            var emitSignal = function(err) {
                assertFalsy(err);
                busObject.signal("org.alljoyn.bus.samples.aroundme.chat", "Chat", "sender", "text", { flags: 0x20 }, callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testReplaceSignalHandler: function(queue) {
        queue.call(function(callbacks) {
            var busObject = {
                "org.alljoyn.bus.PlayerState": {}
            };
            var connect = function(err) {
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.PlayerState",
                    signal: [
                        { name: "PlayerPosition", signature: "uuu", argNames: "x,y,rotation" },
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/game/player", busObject, false, callbacks.add(registerSignalHandler));
            };
            var registerSignalHandler = function(err) {
                assertFalsy(err);
                var signalled = 0;
                var onPlayerPosition = callbacks.add(function(context, x, y, rotation) {
                    if (++signalled === 1) {
                        /* Delay to ensure that a second signal doesn't arrive. */
                        setTimeout(completeTest, 250);
                    }
                });
                var completeTest = callbacks.add(function() {
                    assertEquals(1, signalled);
                });
                bus.registerSignalHandler(onPlayerPosition, "org.alljoyn.bus.PlayerState.PlayerPosition", "/game/player", callbacks.add(function(err) {
                    assertFalsy(err);
                    bus.registerSignalHandler(onPlayerPosition, "org.alljoyn.bus.PlayerState.PlayerPosition", "/game/player", callbacks.add(emitSignal));
                }));
            };
            var emitSignal = function(err) {
                assertFalsy(err);
                busObject.signal("org.alljoyn.bus.PlayerState", "PlayerPosition", 100, 200, 300, callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testSignalSourcepath: function(queue) {
        queue.call(function(callbacks) {
            var player0 = {
                "org.alljoyn.bus.PlayerState": {}
            };
            var player1 = {
                "org.alljoyn.bus.PlayerState": {}
            };
            var connect = function(err) {
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.PlayerState",
                    signal: [
                        { name: "PlayerPosition", signature: "uuu", argNames: "x,y,rotation" },
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/game/player0", player0, false, callbacks.add(function(err) {
                    assertFalsy(err);
                    bus.registerBusObject("/game/player1", player1, false, callbacks.add(registerSignalHandler));
                }));
            };
            var onPlayerPosition = callbacks.add(function(context, x, y, rotation) {
                assertEquals("/game/player0", context.objectPath);
            });
            var registerSignalHandler = function(err) {
                assertFalsy(err);
                bus.registerSignalHandler(onPlayerPosition, "org.alljoyn.bus.PlayerState.PlayerPosition", "/game/player0", callbacks.add(function(err) {
                    assertFalsy(err);
                    bus.registerSignalHandler(onPlayerPosition, "org.alljoyn.bus.PlayerState.PlayerPosition", "/game/player1", callbacks.add(unregisterSignalHandler));
                }));
            };
            var unregisterSignalHandler = function(err) {
                assertFalsy(err);
                bus.unregisterSignalHandler(onPlayerPosition, "org.alljoyn.bus.PlayerState.PlayerPosition", "/game/player1", callbacks.add(emitSignal));
            };
            var emitSignal = function(err) {
                assertFalsy(err);
                player0.signal("org.alljoyn.bus.PlayerState", "PlayerPosition", 100, 200, 300, callbacks.add(function(err) {
                    assertFalsy(err);
                    player1.signal("org.alljoyn.bus.PlayerState", "PlayerPosition", 400, 500, 600, callbacks.add(done));
                }));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testEnumerateProxyBusObjects: function(queue) {
        var path = {};
        queue.call(function(callbacks) {
            var onIntrospect = function(proxyObj) {
                return callbacks.add(function(err) {
                    assertFalsy(err);
                    path[proxyObj.path] = true;
                    var introspectChild = function(err, children) {
                        assertFalsy(err);
                        for (var i = 0; i < children.length; ++i) {
                            children[i].introspect(onIntrospect(children[i]));
                        }
                    };
                    proxyObj.getChildren(callbacks.add(introspectChild));
                });
            };

            var connect = function(err) {
                bus.connect(callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + "/", callbacks.add(introspect));
            };
            var introspect = function(err, proxyObj) {
                assertFalsy(err);
                proxyObj.introspect(onIntrospect(proxyObj));
            };
            this._setUp(callbacks.add(connect));
        });
        queue.call(function(callbacks) {
            assertTrue(path["/"]);
            assertTrue(path["/org"]);
            assertTrue(path["/org/alljoyn"]);
            assertTrue(path["/org/alljoyn/Bus"]);
            assertTrue(path["/org/alljoyn/Bus/Peer"]);
        });
    },

    testBusAttachmentAccessAfterDestroy: function(queue) {
        queue.call(function(callbacks) {
            var otherBusCreate = function() {
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusDestroy));
            };
            var otherBusDestroy = function(err) {
                assertFalsy(err);
                otherBus.destroy(callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                err && console.log('err = ' + err.code);
                assertFalsy(err);
                try {
                    otherBus.connect(onConnect);
                } catch (ex) {
                    assertEquals(ex.name, 'TypeError');
                }
            };
            var onConnect = function(err) {
                fail("This callback should not have been called.");
            };
            this._setUp(callbacks.add(otherBusCreate));
        });
    },
});

