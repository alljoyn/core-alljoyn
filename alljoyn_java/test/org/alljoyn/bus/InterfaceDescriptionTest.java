/**
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
*/

package org.alljoyn.bus;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.ifaces.DBusProxyObj;

import org.alljoyn.bus.DynamicBusObject;
import org.alljoyn.bus.InterfaceDescription;
import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.InterfaceDef.SecurityPolicy;
import org.alljoyn.bus.defs.MethodDef;
import org.alljoyn.bus.defs.SignalDef;
import org.alljoyn.bus.defs.PropertyDef;
import org.alljoyn.bus.defs.PropertyDef.ChangedSignalPolicy;
import org.alljoyn.bus.defs.ArgDef;

import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.ArrayList;
import java.lang.reflect.Method;

import junit.framework.TestCase;

public class InterfaceDescriptionTest extends TestCase {

    private static final String ifaceTextEN = "English interface description";
    private static final String ifaceTextFR = "French interface description";
    private static final String memberTextEN = "English member description";
    private static final String memberTextFR = "French member description";
    private static final String argTextEN = "English arg description";
    private static final String argTextFR = "French arg description";
    private static final String propertyTextEN = "English property description";
    private static final String propertyTextFR = "French property description";

    public InterfaceDescriptionTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class Service implements SimpleInterface,
                                    BusObject {
        public String ping(String inStr) throws BusException { return inStr; }
    }

    public class ServiceC implements SimpleInterfaceC,
                                     BusObject {
        public String ping(String inStr) throws BusException { return inStr; }
        public String pong(String inStr) throws BusException { return inStr; }
    }

    public class DynamicService implements DynamicBusObject {
        @Override
        public String getPath() {
            return "/org/alljoyn/bus/DBOTest";
        }

        @Override
        public List<InterfaceDef> getInterfaces() {
            List<InterfaceDef> ifaces = new ArrayList<InterfaceDef>();
            ifaces.add( buildDynamicInterfaceDef() );
            return ifaces;
        }

        @Override
        public Method getMethodHandler(String interfaceName, String methodName, String signature) {
            // implementation specific - determine and return the appropriate implementation Method
            if (signature.length() != 0) return null;
            try {
                return getClass().getDeclaredMethod( "myMethod", new Class[]{} );
            } catch (java.lang.NoSuchMethodException ex) {
                return null;
            }
        }

        @Override
        public Method getSignalHandler(String interfaceName, String signalName, String signature) {
            // implementation specific - determine and return the appropriate implementation Method
            if (signature.length() != 1) return null;
            try {
                return getClass().getDeclaredMethod( "mySignal", new Class[]{Object.class} );
            } catch (java.lang.NoSuchMethodException ex) {
                return null;
            }
        }

        @Override
        public Method[] getPropertyHandler(String interfaceName, String propertyName) {
            // implementation specific - determine and return the appropriate implementation Methods
            try {
                return new Method[] {
                    getClass().getDeclaredMethod( "getMyProperty", new Class[]{} ),
                    getClass().getDeclaredMethod( "setMyProperty", new Class[]{Object.class} )};
            } catch (java.lang.NoSuchMethodException ex) {
                return null;
            }
        }

        // My implementation handler for the MethodDef("DoSomething", "", "") method
        public void myMethod() {
        }

        // My implementation handler for the SignalDef("Ping", "s") signal
        public void mySignal(Object value) {
        }

        // My implementation handler for the PropertyDef("Color", "s") set property
        public void setMyProperty(Object value) {
        }

        // My implementation handler for the PropertyDef("Color", "s") get property
        public Object getMyProperty() {
            return "SomeValue";
        }

        private InterfaceDef buildDynamicInterfaceDef() {
            String ifaceName = "org.alljoyn.bus.DynamicInterfaceTest";
            boolean announced = true;

            MethodDef methodDef = new MethodDef("DoSomething", "", "", ifaceName);
            methodDef.addAnnotation(MethodDef.ANNOTATION_METHOD_NOREPLY, "true");

            SignalDef signalDef = new SignalDef("Ping", "s", ifaceName);
            signalDef.addArg( new ArgDef("value", "s", ArgDef.DIRECTION_OUT) );
            signalDef.addAnnotation(SignalDef.ANNOTATION_SIGNAL_SESSIONLESS, "true");

            PropertyDef propertyDef = new PropertyDef("Color", "s", PropertyDef.ACCESS_READWRITE, ifaceName);
            propertyDef.addAnnotation(PropertyDef.ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL, ChangedSignalPolicy.TRUE.text);

            InterfaceDef interfaceDef = new InterfaceDef(ifaceName, announced, null);
            interfaceDef.addAnnotation(InterfaceDef.ANNOTATION_SECURE, SecurityPolicy.OFF.text);
            interfaceDef.addMethod(methodDef);
            interfaceDef.addSignal(signalDef);
            interfaceDef.addProperty(propertyDef);

            return interfaceDef;
        }
    }


    private BusAttachment bus;

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());

        Status status = bus.connect();
        assertEquals(Status.OK, status);

        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.InterfaceDescriptionTest",
                                                                 DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res);
    }

    public void tearDown() throws Exception {
        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.ReleaseNameResult res = control.ReleaseName("org.alljoyn.bus.InterfaceDescriptionTest");
        assertEquals(DBusProxyObj.ReleaseNameResult.Released, res);

        bus.disconnect();
        bus = null;
    }

    public void testDifferentSignature() throws Exception {
        Service service = new Service();
        Status status = bus.registerBusObject(service, "/service");
        assertEquals(Status.OK, status);

        boolean thrown = false;
        try {
            SimpleInterfaceB proxy = bus.getProxyBusObject("org.alljoyn.bus.InterfaceDescriptionTest",
                "/service",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { SimpleInterfaceB.class }).getInterface(SimpleInterfaceB.class);
            proxy.ping(1);
        } catch (BusException ex) {
            thrown = true;
        }
        assertTrue(thrown);

        bus.unregisterBusObject(service);
    }

    public void testProxyInterfaceSubset() throws Exception {
        ServiceC service = new ServiceC();
        Status status = bus.registerBusObject(service, "/service");
        assertEquals(Status.OK, status);

        boolean thrown = false;
        try {
            SimpleInterface proxy = bus.getProxyBusObject("org.alljoyn.bus.InterfaceDescriptionTest", "/service",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { SimpleInterface.class }).getInterface(SimpleInterface.class);
            proxy.ping("str");
        } catch (BusException ex) {
            thrown = true;
        }
        assertTrue(thrown);

        bus.unregisterBusObject(service);
    }

    public void testProxyInterfaceSuperset() throws Exception {
        Service service = new Service();
        Status status = bus.registerBusObject(service, "/service");
        assertEquals(Status.OK, status);

        boolean thrown = false;
        try {
            SimpleInterfaceC proxy = bus.getProxyBusObject("org.alljoyn.bus.InterfaceDescriptionTest",
                "/service",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { SimpleInterfaceC.class }).getInterface(SimpleInterfaceC.class);
            proxy.ping("str");
        } catch (BusException ex) {
            thrown = true;
        }
        assertTrue(thrown);

        bus.unregisterBusObject(service);
    }

    public void testCreateDynamicInterfaceFromInterfaceDef() throws Exception {
        // Create dynamic bus object
        DynamicBusObject busObj = new DynamicService();
        assertNotNull(busObj.getInterfaces());
        assertEquals(1, busObj.getInterfaces().size());

        // Create dynamic interface description
        InterfaceDescription desc = new InterfaceDescription(busObj);
        Status status = desc.create(bus, busObj.getInterfaces().get(0));

        assertEquals(Status.OK, status);
        assertTrue(desc.isDynamicInterface());
        assertTrue(desc.isAnnounced());
    }

    public void testCreateDynamicInterfacesFromDynamicBusObject() throws Exception {
        // Create dynamic bus object
        DynamicBusObject busObj = new DynamicService();
        assertNotNull(busObj.getInterfaces());
        assertEquals(1, busObj.getInterfaces().size());

        // Create dynamic interface descriptions
        List<InterfaceDescription> descs = new ArrayList<InterfaceDescription>();
        Status status = InterfaceDescription.create(bus, busObj, descs);

        assertEquals(Status.OK, status);
        assertEquals(1, descs.size());
        assertTrue(descs.get(0).isDynamicInterface());
        assertTrue(descs.get(0).isAnnounced());
    }

    public void testGetDescriptionForLanguage_withAutoSet() throws Exception {
        String description;

        // Descriptions created automatically via static BusAnnotations specified in InterfaceWithAnnotationDescriptions
        Class<?>[] ifaces = { Class.forName("org.alljoyn.bus.InterfaceWithAnnotationDescriptions") };
        List<InterfaceDescription> descs = new ArrayList<InterfaceDescription>();
        Status status = InterfaceDescription.create(bus, ifaces, descs);
        assertEquals(Status.OK, status);
        InterfaceDescription ifaceDesc = descs.get(0);

        // Retrieve english description from interface
        description = ifaceDesc.getDescriptionForLanguage("en");
        assertEquals(ifaceTextEN, description);

        // Retrieve unknown description from interface
        description = ifaceDesc.getDescriptionForLanguage("");
        assertNull(description);

        // Retrieve unknown description from interface (invalid language tag)
        description = ifaceDesc.getDescriptionForLanguage(null);
        assertNull(description);

        // Retrieve french description from interface
        description = ifaceDesc.getDescriptionForLanguage("fr");
        assertEquals(ifaceTextFR, description);

        // Get description languages
        String[] langArray = ifaceDesc.getDescriptionLanguages();
        assertNotNull(langArray);
        assertEquals(2, langArray.length);
        assertTrue(Arrays.asList(langArray).contains("en"));
        assertTrue(Arrays.asList(langArray).contains("fr"));
    }

    // IGNORE: Currently InterfaceDescription.create auto-activates the interface, preventing later set descriptions.
    public void ignore_testGetDescriptionForLanguage_withManualSet() throws Exception {
        String description;
        Status status = Status.OK;

        // Descriptions created manually via InterfaceDescription.setDescriptionForLanguage()
        Class<?> iface = Class.forName("org.alljoyn.bus.InterfaceWithoutAnnotationDescriptions");
        InterfaceDescription ifaceDesc = new InterfaceDescription();
        //status = ifaceDesc.create(bus, iface, false); // unreleased HACK to prevent auto-activate
        assertEquals(Status.OK, status);

        // Get description languages (before any are set)
        String[] langArray = ifaceDesc.getDescriptionLanguages();
        assertNotNull(langArray);
        assertEquals(0, langArray.length);

        // Set english description on interface
        status = ifaceDesc.setDescriptionForLanguage(ifaceTextEN, "en");
        assertEquals(Status.OK, status);

        // Retrieve english description from interface
        description = ifaceDesc.getDescriptionForLanguage("en");
        assertEquals(ifaceTextEN, description);

        // Retrieve unknown description from interface
        description = ifaceDesc.getDescriptionForLanguage("");
        assertNull(description);

        // Retrieve unknown description from interface (invalid language tag)
        description = ifaceDesc.getDescriptionForLanguage(null);
        assertNull(description);

        // Set french description on interface (multiple descriptions on interface)
        status = ifaceDesc.setDescriptionForLanguage(ifaceTextFR, "fr");
        assertEquals(Status.OK, status);

        // Retrieve french description from interface
        description = ifaceDesc.getDescriptionForLanguage("fr");
        assertEquals(ifaceTextFR, description);

        // Re-retrieve english description from interface
        description = ifaceDesc.getDescriptionForLanguage("en");
        assertEquals(ifaceTextEN, description);

        // Get description languages
        langArray = ifaceDesc.getDescriptionLanguages();
        assertNotNull(langArray);
        assertEquals(2, langArray.length);
        assertTrue(Arrays.asList(langArray).contains("en"));
        assertTrue(Arrays.asList(langArray).contains("fr"));
    }

    // IGNORE: Currently InterfaceDescription.create auto-activates the interface, preventing later set descriptions.
    public void ignore_testInvalidSetDescriptionForLanguage() throws Exception {
        Status status = Status.OK;

        Class<?> iface = Class.forName("org.alljoyn.bus.InterfaceWithoutAnnotationDescriptions");
        InterfaceDescription ifaceDesc = new InterfaceDescription();
        //status = ifaceDesc.create(bus, iface, false); // unreleased HACK to prevent auto-activate
        assertEquals(Status.OK, status);

        // Set invalid description
        status = ifaceDesc.setDescriptionForLanguage(null, "en");
        assertEquals(Status.FAIL, status);

        // Set description using invalid language tag
        status = ifaceDesc.setDescriptionForLanguage("test", null);
        assertEquals(Status.FAIL, status);

        // Retrieve description using invalid language tag
        ifaceDesc.setDescriptionForLanguage("test", "en");
        String description = ifaceDesc.getDescriptionForLanguage(null);
        assertNull(description);
    }

    public void testGetMemberDescriptionForLanguage_withAutoSet() throws Exception {
        final String member = "Ping";
        String description;

        // Descriptions created automatically via static BusAnnotations specified in InterfaceWithAnnotationDescriptions
        Class<?>[] ifaces = { Class.forName("org.alljoyn.bus.InterfaceWithAnnotationDescriptions") };
        List<InterfaceDescription> descs = new ArrayList<InterfaceDescription>();
        Status status = InterfaceDescription.create(bus, ifaces, descs);
        assertEquals(Status.OK, status);
        InterfaceDescription ifaceDesc = descs.get(0);

        // Retrieve english description
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "en");
        assertEquals(memberTextEN, description);

        // Retrieve unknown description
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "");
        assertNull(description);

        // Retrieve unknown description (invalid language tag)
        description = ifaceDesc.getMemberDescriptionForLanguage(member, null);
        assertNull(description);

        // Retrieve french description
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "fr");
        assertEquals(memberTextFR, description);

        // Re-retrieve english description
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "en");
        assertEquals(memberTextEN, description);

        // Get description languages
        String[] langArray = ifaceDesc.getDescriptionLanguages();
        assertNotNull(langArray);
        assertEquals(2, langArray.length);
        assertTrue(Arrays.asList(langArray).contains("en"));
        assertTrue(Arrays.asList(langArray).contains("fr"));
    }

    // IGNORE: Currently InterfaceDescription.create auto-activates the interface, preventing later set descriptions.
    public void ignore_testGetMemberDescriptionForLanguage_withManualSet() throws Exception {
        final String member = "Ping";
        String description;
        Status status = Status.OK;

        // Descriptions created manually via InterfaceDescription.setMemberDescriptionForLanguage()
        Class<?> iface = Class.forName("org.alljoyn.bus.InterfaceWithoutAnnotationDescriptions");
        InterfaceDescription ifaceDesc = new InterfaceDescription();
        //status = ifaceDesc.create(bus, iface, false); // unreleased HACK to prevent auto-activate
        assertEquals(Status.OK, status);

        // Set english description
        status = ifaceDesc.setMemberDescriptionForLanguage(member, memberTextEN, "en");
        assertEquals(Status.OK, status);

        // Retrieve english description
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "en");
        assertEquals(memberTextEN, description);

        // Retrieve unknown description
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "");
        assertNull(description);

        // Retrieve unknown description (invalid language tag)
        description = ifaceDesc.getMemberDescriptionForLanguage(member, null);
        assertNull(description);

        // Set french description (multiple descriptions on interface)
        status = ifaceDesc.setMemberDescriptionForLanguage(member, memberTextFR, "fr");
        assertEquals(Status.OK, status);

        // Retrieve french description
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "fr");
        assertEquals(memberTextFR, description);

        // Re-retrieve english description
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "en");
        assertEquals(memberTextEN, description);

        // Get description languages
        String[] langArray = ifaceDesc.getDescriptionLanguages();
        assertNotNull(langArray);
        assertEquals(2, langArray.length);
        assertTrue(Arrays.asList(langArray).contains("en"));
        assertTrue(Arrays.asList(langArray).contains("fr"));
    }

    // IGNORE: Currently InterfaceDescription.create auto-activates the interface, preventing later set descriptions.
    public void ignore_testInvalidSetMemberDescriptionForLanguage() throws Exception {
        final String member = "Ping";
        Status status = Status.OK;

        Class<?> iface = Class.forName("org.alljoyn.bus.InterfaceWithoutAnnotationDescriptions");
        InterfaceDescription ifaceDesc = new InterfaceDescription();
        //status = ifaceDesc.create(bus, iface, false); // unreleased HACK to prevent auto-activate
        assertEquals(Status.OK, status);

        // Set description using invalid member name
        status = ifaceDesc.setMemberDescriptionForLanguage(null, "test", "en");
        assertEquals(Status.FAIL, status);

        // Set invalid description
        status = ifaceDesc.setMemberDescriptionForLanguage(member, null, "en");
        assertEquals(Status.FAIL, status);

        // Set description using invalid language tag
        status = ifaceDesc.setMemberDescriptionForLanguage(member, "test", null);
        assertEquals(Status.FAIL, status);

        // Retrieve description using invalid member name
        ifaceDesc.setMemberDescriptionForLanguage(member, "test", "en");
        String description = ifaceDesc.getMemberDescriptionForLanguage(null, "en");
        assertNull(description);

        // Retrieve description using invalid language tag
        ifaceDesc.setMemberDescriptionForLanguage(member, "test", "en");
        description = ifaceDesc.getMemberDescriptionForLanguage(member, null);
        assertNull(description);
    }

    public void testGetPropertyDescriptionForLanguage_withAutoSet() throws Exception {
        final String property = "StringProp";
        String description;

        // Descriptions created automatically via static BusAnnotations specified in InterfaceWithAnnotationDescriptions
        Class<?>[] ifaces = { Class.forName("org.alljoyn.bus.InterfaceWithAnnotationDescriptions") };
        List<InterfaceDescription> descs = new ArrayList<InterfaceDescription>();
        Status status = InterfaceDescription.create(bus, ifaces, descs);
        assertEquals(Status.OK, status);
        InterfaceDescription ifaceDesc = descs.get(0);

        // Retrieve english description
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "en");
        assertEquals(propertyTextEN, description);

        // Retrieve unknown description
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "");
        assertNull(description);

        // Retrieve unknown description (invalid language tag)
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, null);
        assertNull(description);

        // Retrieve french description
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "fr");
        assertEquals(propertyTextFR, description);

        // Re-retrieve english description
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "en");
        assertEquals(propertyTextEN, description);

        // Get description languages
        String[] langArray = ifaceDesc.getDescriptionLanguages();
        assertNotNull(langArray);
        assertEquals(2, langArray.length);
        assertTrue(Arrays.asList(langArray).contains("en"));
        assertTrue(Arrays.asList(langArray).contains("fr"));
    }

    // IGNORE: Currently InterfaceDescription.create auto-activates the interface, preventing later set descriptions.
    public void ignore_testGetPropertyDescriptionForLanguage_withManualSet() throws Exception {
        final String property = "StringProp";
        String description;
        Status status = Status.OK;

        // Descriptions created manually via InterfaceDescription.setPropertyDescriptionForLanguage()
        Class<?> iface = Class.forName("org.alljoyn.bus.InterfaceWithoutAnnotationDescriptions");
        InterfaceDescription ifaceDesc = new InterfaceDescription();
        //status = ifaceDesc.create(bus, iface, false); // unreleased HACK to prevent auto-activate
        assertEquals(Status.OK, status);

        // Set english description
        status = ifaceDesc.setPropertyDescriptionForLanguage(property, propertyTextEN, "en");
        assertEquals(Status.OK, status);

        // Retrieve english description
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "en");
        assertEquals(propertyTextEN, description);

        // Retrieve unknown description
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "");
        assertNull(description);

        // Retrieve unknown description (invalid language tag)
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, null);
        assertNull(description);

        // Set french description (multiple descriptions on interface)
        status = ifaceDesc.setPropertyDescriptionForLanguage(property, propertyTextFR, "fr");
        assertEquals(Status.OK, status);

        // Retrieve french description
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "fr");
        assertEquals(propertyTextFR, description);

        // Re-retrieve english description
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "en");
        assertEquals(propertyTextEN, description);

        // Get description languages
        String[] langArray = ifaceDesc.getDescriptionLanguages();
        assertNotNull(langArray);
        assertEquals(2, langArray.length);
        assertTrue(Arrays.asList(langArray).contains("en"));
        assertTrue(Arrays.asList(langArray).contains("fr"));
    }

    // IGNORE: Currently InterfaceDescription.create auto-activates the interface, preventing later set descriptions.
    public void ignore_testInvalidSetPropertyDescriptionForLanguage() throws Exception {
        final String property = "StringProp";
        Status status = Status.OK;

        Class<?> iface = Class.forName("org.alljoyn.bus.InterfaceWithoutAnnotationDescriptions");
        InterfaceDescription ifaceDesc = new InterfaceDescription();
        //status = ifaceDesc.create(bus, iface, false); // unreleased HACK to prevent auto-activate
        assertEquals(Status.OK, status);

        // Set description using invalid property name
        status = ifaceDesc.setPropertyDescriptionForLanguage(null, "test", "en");
        assertEquals(Status.FAIL, status);

        // Set invalid description
        status = ifaceDesc.setPropertyDescriptionForLanguage(property, null, "en");
        assertEquals(Status.FAIL, status);

        // Set description using invalid language tag
        status = ifaceDesc.setPropertyDescriptionForLanguage(property, "test", null);
        assertEquals(Status.FAIL, status);

        // Retrieve description using invalid member name
        ifaceDesc.setPropertyDescriptionForLanguage(property, "test", "en");
        String description = ifaceDesc.getPropertyDescriptionForLanguage(null, "en");
        assertNull(description);

        // Retrieve description using invalid language tag
        ifaceDesc.setPropertyDescriptionForLanguage(property, "test", "en");
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, null);
        assertNull(description);
    }

    /*
     * NOTE: Only testing arg descriptions using manual creation of descriptions,
     * as arg descriptions are not supported/created from the java-binding when
     * using its declarative BusAnnotations mechanism in the java interface definition.
     */
    // IGNORE: Currently InterfaceDescription.create auto-activates the interface, preventing later set descriptions.
    public void ignore_testGetArgDescriptionForLanguage_withManualSet() throws Exception {
        final String member = "Ping";
        final String arg = "str";
        String description;
        Status status = Status.OK;

        // Descriptions created manually via InterfaceDescription.setArgDescriptionForLanguage()
        Class<?> iface = Class.forName("org.alljoyn.bus.InterfaceWithoutAnnotationDescriptions");
        InterfaceDescription ifaceDesc = new InterfaceDescription();
        //status = ifaceDesc.create(bus, iface, false); // unreleased HACK to prevent auto-activate
        assertEquals(Status.OK, status);

        // Set english description
        status = ifaceDesc.setArgDescriptionForLanguage(member, arg, argTextEN, "en");
        assertEquals(Status.OK, status);

        // Retrieve english description
        description = ifaceDesc.getArgDescriptionForLanguage(member, arg, "en");
        assertEquals(argTextEN, description);

        // Retrieve unknown description
        description = ifaceDesc.getArgDescriptionForLanguage(member, arg, "");
        assertNull(description);

        // Retrieve unknown description (invalid language tag)
        description = ifaceDesc.getArgDescriptionForLanguage(member, arg, null);
        assertNull(description);

        // Set french description (multiple descriptions on interface)
        status = ifaceDesc.setArgDescriptionForLanguage(member, arg, argTextFR, "fr");
        assertEquals(Status.OK, status);

        // Retrieve french description
        description = ifaceDesc.getArgDescriptionForLanguage(member, arg, "fr");
        assertEquals(argTextFR, description);

        // Re-retrieve english description
        description = ifaceDesc.getArgDescriptionForLanguage(member, arg, "en");
        assertEquals(argTextEN, description);

        // Get description languages
        String[] langArray = ifaceDesc.getDescriptionLanguages();
        assertNotNull(langArray);
        assertEquals(2, langArray.length);
        assertTrue(Arrays.asList(langArray).contains("en"));
        assertTrue(Arrays.asList(langArray).contains("fr"));
    }

    // IGNORE: Currently InterfaceDescription.create auto-activates the interface, preventing later set descriptions.
    public void ignore_testInvalidSetArgDescriptionForLanguage() throws Exception {
        final String member = "Ping";
        final String arg = "str";
        Status status = Status.OK;

        Class<?> iface = Class.forName("org.alljoyn.bus.InterfaceWithoutAnnotationDescriptions");
        InterfaceDescription ifaceDesc = new InterfaceDescription();
        //status = ifaceDesc.create(bus, iface, false); // unreleased HACK to prevent auto-activate
        assertEquals(Status.OK, status);

        // Set description using invalid member name
        status = ifaceDesc.setArgDescriptionForLanguage(null, arg, "test", "en");
        assertEquals(Status.FAIL, status);

        // Set description using invalid arg name
        status = ifaceDesc.setArgDescriptionForLanguage(member, null, "test", "en");
        assertEquals(Status.FAIL, status);

        // Set invalid description
        status = ifaceDesc.setArgDescriptionForLanguage(member, arg, null, "en");
        assertEquals(Status.FAIL, status);

        // Set description using invalid language tag
        status = ifaceDesc.setArgDescriptionForLanguage(member, arg, "test", null);
        assertEquals(Status.FAIL, status);

        // Retrieve description using invalid member name
        ifaceDesc.setArgDescriptionForLanguage(member, arg, "test", "en");
        String description = ifaceDesc.getArgDescriptionForLanguage(null, arg, "en");
        assertNull(description);

        // Retrieve description using invalid arg name
        ifaceDesc.setArgDescriptionForLanguage(member, arg, "test", "en");
        description = ifaceDesc.getArgDescriptionForLanguage(member, null, "en");
        assertNull(description);

        // Retrieve description using invalid language tag
        ifaceDesc.setArgDescriptionForLanguage(member, arg, "test", "en");
        description = ifaceDesc.getArgDescriptionForLanguage(member, arg, null);
        assertNull(description);
    }

    public void testGetMultiDescriptionForLanguage_withAutoSet() throws Exception {
        final String member = "Ping";
        final String arg = "str";
        final String property = "StringProp";
        String description;

        // Descriptions created automatically via static BusAnnotations specified in InterfaceWithAnnotationDescriptions
        Class<?>[] ifaces = { Class.forName("org.alljoyn.bus.InterfaceWithAnnotationDescriptions") };
        List<InterfaceDescription> descs = new ArrayList<InterfaceDescription>();
        Status status = InterfaceDescription.create(bus, ifaces, descs);
        assertEquals(Status.OK, status);
        InterfaceDescription ifaceDesc = descs.get(0);

        // Retrieve interface descriptions
        description = ifaceDesc.getDescriptionForLanguage("en");
        assertEquals(ifaceTextEN, description);
        description = ifaceDesc.getDescriptionForLanguage("fr");
        assertEquals(ifaceTextFR, description);

        // Retrieve member descriptions
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "en");
        assertEquals(memberTextEN, description);
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "fr");
        assertEquals(memberTextFR, description);

/* NOTE: arg descriptions not supported/created via BusAnnotations
        // Retrieve arg descriptions
        description = ifaceDesc.getArgDescriptionForLanguage(member, arg, "en");
        assertEquals(argTextEN, description);
        description = ifaceDesc.getArgDescriptionForLanguage(member, arg, "fr");
        assertEquals(argTextFR, description);
*/
        // Retrieve property descriptions
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "en");
        assertEquals(propertyTextEN, description);
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "fr");
        assertEquals(propertyTextFR, description);

        // Get description languages
        String[] langArray = ifaceDesc.getDescriptionLanguages();
        assertNotNull(langArray);
        assertEquals(2, langArray.length);
        assertTrue(Arrays.asList(langArray).contains("en"));
        assertTrue(Arrays.asList(langArray).contains("fr"));
    }

    // IGNORE: Currently InterfaceDescription.create auto-activates the interface, preventing later set descriptions.
    public void ignore_testGetMultiDescriptionForLanguage_withManualSet() throws Exception {
        final String member = "Ping";
        final String arg = "str";
        final String property = "StringProp";
        String description;
        Status status = Status.OK;

        // Descriptions created manually via InterfaceDescription.setXXXDescriptionForLanguage()
        Class<?> iface = Class.forName("org.alljoyn.bus.InterfaceWithoutAnnotationDescriptions");
        InterfaceDescription ifaceDesc = new InterfaceDescription();
        //status = ifaceDesc.create(bus, iface, false); // unreleased HACK to prevent auto-activate
        assertEquals(Status.OK, status);

        // Set interface descriptions
        ifaceDesc.setDescriptionForLanguage(ifaceTextEN, "en");
        ifaceDesc.setDescriptionForLanguage(ifaceTextFR, "fr");

        // Set member descriptions
        ifaceDesc.setMemberDescriptionForLanguage(member, memberTextEN, "en");
        ifaceDesc.setMemberDescriptionForLanguage(member, memberTextFR, "fr");

        // Set arg descriptions
        ifaceDesc.setArgDescriptionForLanguage(member, arg, argTextEN, "en");
        ifaceDesc.setArgDescriptionForLanguage(member, arg, argTextFR, "fr");

        // Set property descriptions
        ifaceDesc.setPropertyDescriptionForLanguage(property, propertyTextEN, "en");
        ifaceDesc.setPropertyDescriptionForLanguage(property, propertyTextFR, "fr");

        // Retrieve interface descriptions
        description = ifaceDesc.getDescriptionForLanguage("en");
        assertEquals(ifaceTextEN, description);
        description = ifaceDesc.getDescriptionForLanguage("fr");
        assertEquals(ifaceTextFR, description);

        // Retrieve member descriptions
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "en");
        assertEquals(memberTextEN, description);
        description = ifaceDesc.getMemberDescriptionForLanguage(member, "fr");
        assertEquals(memberTextFR, description);

        // Retrieve arg descriptions
        description = ifaceDesc.getArgDescriptionForLanguage(member, arg, "en");
        assertEquals(argTextEN, description);
        description = ifaceDesc.getArgDescriptionForLanguage(member, arg, "fr");
        assertEquals(argTextFR, description);

        // Retrieve property descriptions
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "en");
        assertEquals(propertyTextEN, description);
        description = ifaceDesc.getPropertyDescriptionForLanguage(property, "fr");
        assertEquals(propertyTextFR, description);

        // Get description languages
        String[] langArray = ifaceDesc.getDescriptionLanguages();
        assertNotNull(langArray);
        assertEquals(2, langArray.length);
        assertTrue(Arrays.asList(langArray).contains("en"));
        assertTrue(Arrays.asList(langArray).contains("fr"));
    }

}
