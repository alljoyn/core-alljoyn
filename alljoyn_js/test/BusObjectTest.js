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
AsyncTestCase("BusObjectTest", {
    _setUp: ondeviceready(function(callback) {
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        bus.destroy();
    },

    testMethodNotImplemented: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.samples.simple.SimpleInterface",
                    method: [
                        { name: 'Ping', signature: 's', returnSignature: 's', argNames: 'inStr,outStr' }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.samples.simple.SimpleInterface": { }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "Ping", "hello", callbacks.add(done));
            };
            var done = function(err) {
                assertEquals('org.alljoyn.Bus.ErStatus', err.name);
                assertEquals(err.BUS_OBJECT_NO_SUCH_MEMBER, err.code);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testRegisterUnregisterCallbacks: function(queue) {
        queue.call(function(callbacks) {
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/busObject",
                                      {
                                          onRegistered: callbacks.add(function() {
                                              bus.unregisterBusObject("/busObject", callbacks.add(function(err) {
                                                                                                      assertFalsy(err);
                                                                                                  }));
                                                                      }),
                                          onUnregistered: callbacks.add(function() {})
                                      },
                                      false,
                                      callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(done));
            };
            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(registerBusObject));
        });
    },

    testToXMLCallback: function(queue) {
        queue.call(function(callbacks) {
            var expectedCustomXML =
                '<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"\n' +
                '"http://standards.freedesktop.org/dbus/introspect-1.0.dtd">\n' +
                '<node>\n' +
                '  <interface name="org.alljoyn.bus.samples.simple.SimpleInterface">\n' +
                '    <method name="Ping">\n' +
                '      <arg name="inStr" type="s" direction="in"/>\n' +
                '      <arg name="outStr" type="s" direction="out"/>\n' +
                '      <annotation name="org.alljoyn.Foo" value="bar"/>\n' +
                '    </method>\n' +
                '  </interface>\n' +
                '  <interface name="org.freedesktop.DBus.Introspectable">\n' +
                '    <method name="Introspect">\n' +
                '      <arg name="data" type="s" direction="out"/>\n' +
                '    </method>\n' +
                '  </interface>\n' +
                '</node>\n';
            var expectedBuiltinXML =
                '<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"\n' +
                '"http://standards.freedesktop.org/dbus/introspect-1.0.dtd">\n' +
                '<node>\n' +
                '  <interface name="org.alljoyn.bus.samples.simple.SimpleInterface">\n' +
                '    <method name="Ping">\n' +
                '      <arg name="inStr" type="s" direction="in"/>\n' +
                '      <arg name="outStr" type="s" direction="out"/>\n' +
                '    </method>\n' +
                '  </interface>\n' +
                '  <interface name="org.freedesktop.DBus.Introspectable">\n' +
                '    <method name="Introspect">\n' +
                '      <arg name="data" type="s" direction="out"/>\n' +
                '    </method>\n' +
                '  </interface>\n' +
                '</node>\n';

            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.samples.simple.SimpleInterface",
                    method: [
                        { name: 'Ping', signature: 's', returnSignature: 's', argNames: 'inStr,outStr' }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var testobject = {
                "org.alljoyn.bus.samples.simple.SimpleInterface": {
                    Ping: function(context, inStr) { context.reply(inStr); }
                },
                toXML: function(deep, indent) {
                    var sp = '                '.slice(-indent);
                    return sp + '<interface name="org.alljoyn.bus.samples.simple.SimpleInterface">\n' +
                        sp + '  <method name="Ping">\n' +
                        sp + '    <arg name="inStr" type="s" direction="in"/>\n' +
                        sp + '    <arg name="outStr" type="s" direction="out"/>\n' +
                        sp + '    <annotation name="org.alljoyn.Foo" value="bar"/>\n' +
                        sp + '  </method>\n' +
                        sp + '</interface>\n' +
                        sp + '<interface name="org.freedesktop.DBus.Introspectable">\n' +
                        sp + '  <method name="Introspect">\n' +
                        sp + '    <arg name="data" type="s" direction="out"/>\n' +
                        sp + '  </method>\n' +
                        sp + '</interface>\n';
                }
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/testobject", testobject, false, callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(introspect));
            };
            var proxy;
            var introspect = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;
                proxy.methodCall("org.freedesktop.DBus.Introspectable", "Introspect", callbacks.add(onIntrospectCustom));
            };
            var onIntrospectCustom = function(err, context, xml) {
                assertFalsy(err);
                assertEquals(expectedCustomXML, xml);
                delete testobject.toXML;
                proxy.methodCall("org.freedesktop.DBus.Introspectable", "Introspect", callbacks.add(onIntrospectBuiltin));
            };
            var onIntrospectBuiltin = function(err, context, xml) {
                assertFalsy(err);
                assertEquals(expectedBuiltinXML, xml);
            };
            this._setUp(callbacks.add(createInterface));
        });
    },

    testDelayedReply: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "org.alljoyn.bus.samples.simple.SimpleInterface",
                    method: [
                        { name: 'Ping', signature: 's', returnSignature: 's', argNames: 'inStr,outStr' }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/testobject",
                                      {
                                          "org.alljoyn.bus.samples.simple.SimpleInterface": {
                                              Ping: function(context, inStr) {
                                                  window.setTimeout(callbacks.add(function() { context.reply(inStr); }), 250);
                                              }
                                          }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + "/testobject", callbacks.add(ping));
            };
            var ping = function(err, testobject) {
                assertFalsy(err);
                testobject.methodCall("org.alljoyn.bus.samples.simple.SimpleInterface", "Ping", "hello", callbacks.add(onPing));
            };
            var onPing = function(err, context, outStr) {
                assertFalsy(err);
                assertEquals("hello", outStr);
            };
            this._setUp(callbacks.add(connect));
        });
    },
});

