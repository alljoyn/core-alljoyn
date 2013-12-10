/*
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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
AsyncTestCase("ProxyBusObjectTest", {
    _setUp: ondeviceready(function(callback) {
        otherBus = undefined;
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        otherBus && otherBus.destroy();
        bus.destroy();
    },

    /*
     * Test that bus.proxy["/foo"].children["bar"] is the same as bus.proxy["/foo/bar"].
     */
    /*
     * NOTE: This test is disabled.  The underlying C++ code does not implement this
     * functionality, so I am ignoring implementing it in JavaScript.
     */
    disabled_testChildObjectIsSameAs: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(getProxyX));
            };
            /* Get a couple references to un-introspected objects. */
            var x, a;
            var getProxyX = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject("org.freedesktop.DBus/org/freedesktop", callbacks.add(getProxyA));
            };
            var getProxyA = function(err, proxyX) {
                x = proxyX;
                assertFalsy(err);
                bus.getProxyBusObject("org.freedesktop.DBus/org", callbacks.add(introspect));
            };
            var introspect = function(err, proxyA) {
                a = proxyA
                assertFalsy(err);
                a.introspect(callbacks.add(onIntrospect));
            };
            var onIntrospect = function(err) {
                assertFalsy(err);
                /*
                 * Compare that the previously un-introspected object is the same as the
                 * introspected one.
                 */
                a.getChildren("freedesktop", callbacks.add(verify0));
            };
            var b;
            var verify0 = function(err, child) {
                assertFalsy(err);
                b = child;
                assertSame(x, b); // TODO This doesn't work in chrome.
                                  // I've verified that I'm returning the same NPObject*.

                /*
                 * Compare that getting the object again via bus.proxy returns the
                 * introspected one.
                 */
                bus.getProxyBusObject("org.freedesktop.DBus/org/freedesktop", callbacks.add(verify1));
            };
            var verify1 = function(err, proxyX) {
                assertFalsy(err);
                x = proxyX;
                assertSame(x, b); // TODO This doesn't work in chrome
                                  // I've verified that I'm returning the same NPObject*.
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testEnumerate: function(queue) {
        // TODO need to go through this one, what is it actually testing?
        //      this is testing that I can enumerate interfaces and children, and that they are updated after introspection
        //      this is probably no longer relevant, should be moved into introspection test case
        queue.call(function(callbacks) {
            var getProxyObj = function(err) {
                assertFalsy(err);
                proxy = bus.getProxyBusObject("org.alljoyn.Bus/org/alljoyn/Bus", callbacks.add(connect));
            };
            var proxy;
            var connect = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;

                var actual = {};
                for (var n in proxy) {
                    actual[n] = true;
                }
                //ProxyBusObject ATTRIBUTES
                assertTrue(actual["serviceName"]);
                assertTrue(actual["path"]);
                assertTrue(actual["sessionId"]);
                //ProxyBusObject OPERATIONS
                assertTrue(actual["getChildren"]);
                assertTrue(actual["getInterface"]);
                assertTrue(actual["getInterfaces"]);
                assertTrue(actual["introspect"]);
                assertTrue(actual["methodCall"]);
                assertTrue(actual["parseXML"]);
                assertTrue(actual["secureConnection"]);

                bus.connect(callbacks.add(getChildren));
            };
            var getChildren = function(err) {
                assertFalsy(err);
                proxy.getChildren(callbacks.add(getInterfaces));
            };
            var getInterfaces = function(err, children) {
                assertFalsy(err);
                assertTrue(children.length === 0);
                proxy.getInterfaces(callbacks.add(getInterface));
            };
            var getInterface = function(err, interfaces) {
                assertFalsy(err);
                var actual = {};
                for (var i = 0; i < interfaces.length; ++i) {
                    actual[interfaces[i].name] = true;
                }

                assertTrue(actual["org.freedesktop.DBus.Peer"]);

                proxy.getInterface('org.freedesktop.DBus.Peer', callbacks.add(introspect));
            };
            var introspect = function(err, intf) {
                assertFalsy(err);

                actual = {};
                for (var n in intf ) {
                    actual[n] = true;
                }

                assertEquals("org.freedesktop.DBus.Peer", intf.name);

                var actual = {};
                //for (var n in intf.method) {
                for (var i = 0; i < intf.method.length; ++i) {
                    actual[intf.method[i].name] = true;
                }
                assertTrue(actual["GetMachineId"]);
                assertTrue(actual["Ping"]);

                proxy.introspect(callbacks.add(onIntrospect));
            };

            var onIntrospect = function(err) {
                assertFalsy(err);
                proxy.getInterfaces(callbacks.add(onGetInterfaces));
            };

            var onGetInterfaces = function(err, interfaces) {
                var actual = {};
                for (var i = 0; i < interfaces.length; ++i) {
                    actual[interfaces[i].name] = true;
                }
                assertTrue(actual["org.alljoyn.Bus"]);
                assertTrue(actual["org.alljoyn.Daemon"]);
                assertTrue(actual["org.freedesktop.DBus.Introspectable"]);
                assertTrue(actual["org.freedesktop.DBus.Peer"]);

                proxy.getChildren(callbacks.add(getPeerInterface));
            };
            var getPeerInterface = function(err, children) {
                assertFalsy(err);
                var actual = {};
                for (var n in children) {
                    actual[children[n].path] = true;
                }
                assertTrue(actual["/org/alljoyn/Bus/Peer"]);

                proxy.getInterface('org.freedesktop.DBus.Peer', callbacks.add(done));
            };
            var done = function(err, intf) {
                var actual = {};
                for (var n in intf.method) {
                    actual[intf.method[n].name] = true;
                }
                assertTrue(actual["GetMachineId"]);
                assertTrue(actual["Ping"]);
            };
            this._setUp(callbacks.add(getProxyObj));
        });
    },

    testAnnotations: function(queue) {
        queue.call(function(callbacks) {
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                otherBus.createInterface({
                    name: "org.alljoyn.bus.samples.simple.SimpleInterface",
                    method: [
                        { name: 'Ping', signature: 's', returnSignature: 's', argNames: 'inStr,outStr',
                          'org.freedesktop.DBus.Deprecated': true }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                otherBus.registerBusObject("/testobject",
                                           {
                                               "org.alljoyn.bus.samples.simple.SimpleInterface": {}
                                           },
                                           false,
                                           callbacks.add(otherBusConnect));
            };
            var otherBusConnect = function(err) {
                assertFalsy(err);
                otherBus.connect(callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                proxy = bus.getProxyBusObject(otherBus.uniqueName + "/testobject", callbacks.add(introspect));
            };
            var introspect = function(err, proxy) {
                assertFalsy(err);
                proxy.introspect(callbacks.add(onIntrospect));
            };
            var onIntrospect = function(err) {
                assertFalsy(err);
                bus.getInterface("org.alljoyn.bus.samples.simple.SimpleInterface", callbacks.add(done));
            };
            var done = function(err, intf) {
                assertFalsy(err);
                assertEquals("true", intf.method[0]["org.freedesktop.DBus.Deprecated"]);
            };
            this._setUp(callbacks.add(otherBusCreate));
        });
    },

    testIntrospection: function(queue) {
        queue.call(function(callbacks) {
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject("org.alljoyn.Bus/org/alljoyn/Bus", callbacks.add(connect));
            };
            var proxy;
            var connect = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;
                bus.connect(callbacks.add(introspect));
            };
            var introspect = function(err) {
                assertFalsy(err);
                proxy.introspect(callbacks.add(onIntrospect));
            };
            var onIntrospect = function() {
                var actual = {};
                for (var name in proxy) {
                    actual[name] = proxy[name];
                }
                /*
                 * Chrome and Firefox both return an Object for the proxy
                 * interfaces.  Android returns a Function.
                 */
                assertNotNull(actual["org.alljoyn.Bus"]);
                assertNotNull(actual["org.freedesktop.DBus.Introspectable"]);
                assertNotNull(actual["org.freedesktop.DBus.Peer"]);

                proxy.getChildren(callbacks.add(done));
            };
            var done = function(err, children) {
                assertFalsy(err);
                var actual = {};
                for (var child in children) {
                    actual[child] = children[child];
                }
                assertNotNull(actual["Peer"]);
            };
            this._setUp(callbacks.add(getProxyObj));
        });
    },

    testParseIntrospection: function(queue) {
        var xml =
            '<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"' +
            '"http://standards.freedesktop.org/dbus/introspect-1.0.dtd">' +
            '<node>' +
            '  <interface name="test.Interface">' +
            '    <method name="method1">' +
            '      <arg name="in" type="s" direction="in"/>' +
            '      <arg name="out" type="s" direction="out"/>' +
            '    </method>' +
            '    <method name="method2">' +
            '      <arg type="s" direction="in"/>' +
            '      <arg type="s" direction="out"/>' +
            '    </method>' +
            '    <method name="method3">' +
            '      <arg type="s" direction="in"/>' +
            '    </method>' +
            '    <method name="method4">' +
            '    </method>' +
            '    <signal name="signal1">' +
            '      <arg name="in" type="s" direction="in"/>' +
            '    </signal>' +
            '    <signal name="signal2">' +
            '      <arg type="s" direction="in"/>' +
            '    </signal>' +
            '    <signal name="signal3">' +
            '    </signal>' +
            '    <property name="prop1" type="s" access="readwrite"/>' +
            '    <property name="prop2" type="s" access="read"/>' +
            '    <property name="prop3" type="s" access="write"/>' +
            '  </interface>' +
            '  <interface name="org.freedesktop.DBus.Properties">' +
            '    <method name="Get">' +
            '      <arg name="interface" type="s" direction="in"/>' +
            '      <arg name="propname" type="s" direction="in"/>' +
            '      <arg name="value" type="v" direction="out"/>' +
            '    </method>' +
            '    <method name="GetAll">' +
            '      <arg name="interface" type="s" direction="in"/>' +
            '      <arg name="props" type="a{sv}" direction="out"/>' +
            '    </method>' +
            '    <method name="Set">' +
            '      <arg name="interface" type="s" direction="in"/>' +
            '      <arg name="propname" type="s" direction="in"/>' +
            '      <arg name="value" type="v" direction="in"/>' +
            '    </method>' +
            '  </interface>' +
            '  <interface name="org.freedesktop.DBus.Introspectable">' +
            '    <method name="Introspect">' +
            '      <arg name="data" type="s" direction="out"/>' +
            '    </method>' +
            '  </interface>' +
            '</node>';
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + "/busObject", callbacks.add(parseXML));
            };
            var proxy;
            var parseXML = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;
                proxy.parseXML(xml, callbacks.add(getInterface));
            };
            var getInterface = function(err) {
                assertFalsy(err);
                proxy.getInterface("test.Interface", callbacks.add(done));
            };
            var done = function(err, intf) {
                assertNotNull(intf);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testPath: function(queue) {
        queue.call(function(callbacks) {
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject("org.freedesktop.DBus/org/freedesktop/DBus", callbacks.add(done));
            };
            var done = function(err, proxy) {
                assertFalsy(err);
                assertEquals("/org/freedesktop/DBus", proxy.path);
                assertEquals("org.freedesktop.DBus", proxy.serviceName);
            };
            this._setUp(callbacks.add(getProxyObj));
        });
    },

    testSecureConnection: function(queue) {
        queue.call(function(callbacks) {
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "test.SecureInterface",
                    secPolicy: org.alljoyn.bus.IfcSecurity.REQUIRED,
                    method: [ { name: 'Ping', signature: 's', returnSignature: 's' } ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                // Note: leaving secure parameter unset to exercise default code path in binding.
                bus.registerBusObject("/test",
                                      {
                                          "test.SecureInterface": {
                                              Ping: function(context, inStr) { context.reply(inStr); }
                                          }
                                      },
                                      callbacks.add(enablePeerSecurity));
            };
            var enablePeerSecurity = function(err) {
                assertFalsy(err);
                bus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        credentials.password = "123456";
                        return true;
                    }),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
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
                bus.connect(callbacks.add(otherBusCreate));
            };
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusEnablePeerSecurity));
            };
            var otherBusEnablePeerSecurity = function(err) {
                assertFalsy(err);
                otherBus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        credentials.password = "123456";
                        return true;
                    }),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
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
                otherBus.getProxyBusObject(bus.uniqueName + "/test", callbacks.add(secureConnection));
            };
            var secureConnection = function(err, proxy) {
                assertFalsy(err);
                proxy.secureConnection(callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(createInterface));
        });
    },

    testSecureConnection2: function(queue) {
        queue.call(function(callbacks) {
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "test.SecureInterface",
                    secPolicy: org.alljoyn.bus.IfcSecurity.INHERIT,
                    method: [ { name: 'Ping', signature: 's', returnSignature: 's' } ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                // Note: leaving secure parameter unset to exercise default code path in binding.
                bus.registerBusObject("/test",
                                      {
                                          "test.SecureInterface": {
                                              Ping: function(context, inStr) { context.reply(inStr); }
                                          }
                                      },
                                      callbacks.add(enablePeerSecurity));
            };
            var enablePeerSecurity = function(err) {
                assertFalsy(err);
                bus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        credentials.password = "123456";
                        return true;
                    }),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
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
                bus.connect(callbacks.add(otherBusCreate));
            };
            var otherBusCreate = function(err) {
                assertFalsy(err);
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(otherBusEnablePeerSecurity));
            };
            var otherBusEnablePeerSecurity = function(err) {
                assertFalsy(err);
                otherBus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    onRequest: callbacks.add(function(authMechanism, peerName, authCount,
                                                      userName, credMask, credentials) {
                        credentials.password = "123456";
                        return true;
                    }),
                    onComplete: callbacks.add(function(authMechanism, peerName, success) {
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
                // Explicitly set secure parameter to true.
                otherBus.getProxyBusObject(bus.uniqueName + "/test", true, callbacks.add(secureConnection));
            };
            var secureConnection = function(err, proxy) {
                assertFalsy(err);
                proxy.secureConnection(callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(createInterface));
        });
    }

});

