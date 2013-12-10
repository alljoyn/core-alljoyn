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
AsyncTestCase("InterfaceDescriptionTest", {
    _setUp: ondeviceready(function(callback) {
        otherBus = undefined;
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        otherBus && otherBus.destroy();
        bus.destroy();
    },

    testRegisterIntrospect: function(queue) {
        var expectedXML =
            '<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"\n' +
            '"http://standards.freedesktop.org/dbus/introspect-1.0.dtd">\n' +
            '<node>\n' +
            '  <interface name="test.Interface">\n' +
            '    <method name="method1">\n' +
            '      <arg name="in" type="s" direction="in"/>\n' +
            '      <arg name="out" type="s" direction="out"/>\n' +
            '    </method>\n' +
            '    <method name="method2">\n' +
            '      <arg type="s" direction="in"/>\n' +
            '      <arg type="s" direction="out"/>\n' +
            '    </method>\n' +
            '    <method name="method3">\n' +
            '      <arg type="s" direction="in"/>\n' +
            '    </method>\n' +
            '    <method name="method4">\n' +
            '    </method>\n' +
            '    <method name="method5">\n' +
            '      <annotation name="org.freedesktop.DBus.Deprecated" value="true"/>\n' +
            '    </method>\n' +
            '    <method name="method6">\n' +
            '      <annotation name="org.freedesktop.DBus.Method.NoReply" value="true"/>\n' +
            '    </method>\n' +
            '    <method name="method7">\n' +
            '      <annotation name="org.test" value="1"/>\n' +
            '    </method>\n' +
            '    <signal name="signal1">\n' +
            '      <arg name="in" type="s" direction="out"/>\n' +
            '    </signal>\n' +
            '    <signal name="signal2">\n' +
            '      <arg type="s" direction="out"/>\n' +
            '    </signal>\n' +
            '    <signal name="signal3">\n' +
            '    </signal>\n' +
            '    <signal name="signal4">\n' +
            '      <annotation name="org.freedesktop.DBus.Deprecated" value="true"/>\n' +
            '    </signal>\n' +
            '    <signal name="signal5">\n' +
            '      <annotation name="org.test" value="1.2"/>\n' +
            '    </signal>\n' +
            '    <property name="prop1" type="s" access="readwrite"/>\n' +
            '    <property name="prop2" type="s" access="read"/>\n' +
            '    <property name="prop3" type="s" access="write"/>\n' +
            '    <property name="prop4" type="s" access="readwrite">\n' +
            '      <annotation name="org.test" value="test"/>\n' +
            '    </property>\n' +
            '  </interface>\n' +
            '  <interface name="org.freedesktop.DBus.Introspectable">\n' +
            '    <method name="Introspect">\n' +
            '      <arg name="data" type="s" direction="out"/>\n' +
            '    </method>\n' +
            '  </interface>\n' +
            '  <interface name="org.freedesktop.DBus.Properties">\n' +
            '    <method name="Get">\n' +
            '      <arg name="interface" type="s" direction="in"/>\n' +
            '      <arg name="propname" type="s" direction="in"/>\n' +
            '      <arg name="value" type="v" direction="out"/>\n' +
            '    </method>\n' +
            '    <method name="GetAll">\n' +
            '      <arg name="interface" type="s" direction="in"/>\n' +
            '      <arg name="props" type="a{sv}" direction="out"/>\n' +
            '    </method>\n' +
            '    <method name="Set">\n' +
            '      <arg name="interface" type="s" direction="in"/>\n' +
            '      <arg name="propname" type="s" direction="in"/>\n' +
            '      <arg name="value" type="v" direction="in"/>\n' +
            '    </method>\n' +
            '  </interface>\n' +
            '</node>\n';
        queue.call(function(callbacks) {
            var createInterface = function(err) {
                assertFalsy(err);
                bus.createInterface({
                    name: "test.Interface",
                    method: [
                        { name: "method1", signature: "s", returnSignature: "s", argNames: "in,out" },
                        { name: "method2", signature: "s", returnSignature: "s" },
                        { name: "method3", signature: "s", },
                        { name: "method4" },
                        { name: "method5", "org.freedesktop.DBus.Deprecated": true },
                        { name: "method6", "org.freedesktop.DBus.Method.NoReply": true },
                        { name: "method7", "org.test": 1 }
                    ],
                    signal: [
                        { name: "signal1", signature: "s", argNames: "in" },
                        { name: "signal2", signature: "s" },
                        { name: "signal3" },
                        { name: "signal4", "org.freedesktop.DBus.Deprecated": true },
                        { name: "signal5", "org.test": 1.2 }
                    ],
                    property: [
                        { name: "prop1", signature: "s", access: "readwrite" },
                        { name: "prop2", signature: "s", access: "read" },
                        { name: "prop3", signature: "s", access: "write" },
                        { name: "prop4", signature: "s", access: "readwrite", "org.test": "test" }
                    ]
                }, callbacks.add(registerBusObject));
            };
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject("/busObject",
                                      { "test.Interface": {} },
                                      false,
                                      callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + "/busObject", callbacks.add(introspect));
            };
            var introspect = function(err, proxy) {
                assertFalsy(err);
                proxy.methodCall("org.freedesktop.DBus.Introspectable", "Introspect", callbacks.add(onIntrospect));
            };
            var onIntrospect = function(err, context, xml) {
                assertFalsy(err);
                assertEquals(expectedXML, xml);
            };
            this._setUp(callbacks.add(createInterface));
        });
    },

    testParseXML: function(queue) {
        queue.call(function(callbacks) {
            var xml =
                '<node>\n' +
                '  <interface name="test.Interface">\n' +
                '    <method name="method1">\n' +
                '      <arg name="in" type="s" direction="in"/>\n' +
                '      <arg name="out" type="s" direction="out"/>\n' +
                '      <annotation name="org.freedesktop.DBus.Deprecated" value="true"/>\n' +
                '      <annotation name="org.freedesktop.DBus.Method.NoReply" value="true"/>\n' +
                '      <annotation name="org.test" value="1"/>\n' +
                '    </method>\n' +
                '    <signal name="signal1">\n' +
                '      <arg name="in" type="s" direction="out"/>\n' +
                '      <annotation name="org.freedesktop.DBus.Deprecated" value="true"/>\n' +
                '      <annotation name="org.test" value="1.2"/>\n' +
                '    </signal>\n' +
                '    <property name="prop1" type="s" access="readwrite">\n' +
                '      <annotation name="org.test" value="test"/>\n' +
                '    </property>\n' +
                '  </interface>\n' +
                '  <interface name="org.freedesktop.DBus.Introspectable">\n' +
                '    <method name="Introspect">\n' +
                '      <arg name="data" type="s" direction="out"/>\n' +
                '    </method>\n' +
                '  </interface>\n' +
                '</node>\n';

            var createInterface = function(err) {
                bus.createInterfacesFromXML(xml, callbacks.add(getInterface));
            };
            var getInterface = function(err) {
                bus.getInterface("test.Interface", callbacks.add(done));
            };
            var done = function(err, intf) {
                assertEquals("method1", intf.method[0].name);
                assertEquals("s", intf.method[0].signature);
                assertEquals("s", intf.method[0].returnSignature);
                assertEquals("in,out", intf.method[0].argNames);
                assertEquals("true", intf.method[0]["org.freedesktop.DBus.Deprecated"]);
                assertEquals("true", intf.method[0]["org.freedesktop.DBus.Method.NoReply"]);
                assertEquals("1", intf.method[0]["org.test"]);

                assertEquals("signal1", intf.signal[0].name);
                assertEquals("s", intf.signal[0].signature);
                assertEquals("in", intf.signal[0].argNames);
                assertEquals("true", intf.signal[0]["org.freedesktop.DBus.Deprecated"]);
                assertEquals("1.2", intf.signal[0]["org.test"]);

                assertEquals("prop1", intf.property[0].name);
                assertEquals("s", intf.property[0].signature);
                assertEquals("readwrite", intf.property[0].access);
                assertEquals("test", intf.property[0]["org.test"]);
            };
            this._setUp(callbacks.add(createInterface));
        });
    },
});

