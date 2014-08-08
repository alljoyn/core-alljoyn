/**
 * Copyright (c) 2009-2011,2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.IntrospectionListener;
import org.alljoyn.bus.IntrospectionWithDescriptionListener;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Introspectable;
import org.alljoyn.bus.ifaces.AllSeenIntrospectable;
import org.alljoyn.bus.Status;

import junit.framework.TestCase;

public class IntrospectionListenerTest extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    public IntrospectionListenerTest(String name) {
        super(name);
    }

    private String introspection =
        "  <interface name=\"\">\n" +
        "    <method name=\"Ping\">\n" +
        "      <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n" +
        "      <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n" +
        "    </method>\n" +
        "  </interface>\n";

    private String introspectionWithDescription = 
        "  <interface name=\"\">\n" +
        "    <description>This is a some interface</description>\n" +
        "    <method name=\"Ping\">\n" +
        "      <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n" +
        "      <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n" +
        "    </method>\n" +
        "  </interface>\n";

    public class Service implements BusObject,
                                    IntrospectionListener, IntrospectionWithDescriptionListener {

        public String generateIntrospection(boolean deep, int indent) {
            return introspection;
        }

        public String generateIntrospection(String language, boolean deep, int indent) {
            if(null != language && language.equals("en")){
                return introspectionWithDescription;
            }

            return introspection;
        }
    }

    public void testGenerate() throws Exception {

        BusAttachment bus = new BusAttachment(getClass().getName());
        assertEquals(Status.OK, bus.connect());

        Service service = new Service();
        assertEquals(Status.OK, bus.registerBusObject(service, "/introspectionlistener"));

        DBusProxyObj control = bus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                     control.RequestName("org.alljoyn.bus.IntrospectionListenerTest",
                                         DBusProxyObj.REQUEST_NAME_NO_FLAGS));

        ProxyBusObject remoteObj = bus.getProxyBusObject("org.alljoyn.bus.IntrospectionListenerTest",
                                                         "/introspectionlistener",
                                                         BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { Introspectable.class, AllSeenIntrospectable.class });
        Introspectable introspectable = remoteObj.getInterface(Introspectable.class);
        String actual = introspectable.Introspect();
        String expected =
            "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n" +
            "\"http://standards.freedesktop.org/dbus/introspect-1.0.dtd\">\n" +
            "<node>\n" +
            introspection +
            "</node>\n";
        assertEquals(expected, actual);

        AllSeenIntrospectable allseenIntrospectable = remoteObj.getInterface(AllSeenIntrospectable.class);
        actual = allseenIntrospectable.IntrospectWithDescription("en");
        expected = 
            "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n" +
            "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n" +
            "<node>\n" +
            introspectionWithDescription +
            "</node>\n";
        assertEquals(expected, actual);
    }
}
