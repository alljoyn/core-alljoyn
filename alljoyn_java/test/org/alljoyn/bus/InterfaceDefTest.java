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

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.defs.BusObjectInfo;
import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.InterfaceDef.SecurityPolicy;
import org.alljoyn.bus.defs.MethodDef;
import org.alljoyn.bus.defs.SignalDef;
import org.alljoyn.bus.defs.PropertyDef;
import org.alljoyn.bus.defs.PropertyDef.ChangedSignalPolicy;
import org.alljoyn.bus.defs.ArgDef;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;

import junit.framework.TestCase;

public class InterfaceDefTest extends TestCase {

    public InterfaceDefTest(String name) {
        super(name);
    }

    public void setUp() throws Exception {
    }

    public void tearDown() throws Exception {
    }

    public void testMethodDef() throws Exception {
        String ifaceName = "org.alljoyn.FooBar";
        MethodDef methodDef;
        ArgDef argDef;

        // Define method that has annotations but no args

        methodDef = new MethodDef("DoSomething", "", "", ifaceName);
        methodDef.addAnnotation("org.freedesktop.DBus.Method.NoReply", "true");
        methodDef.addAnnotation("org.freedesktop.DBus.Deprecated", "true");
        methodDef.setDescription("The method description", "en"); // an annotation

        assertEquals("DoSomething", methodDef.getName());
        assertEquals("", methodDef.getSignature());
        assertEquals("", methodDef.getReplySignature());
        assertEquals(ifaceName, methodDef.getInterfaceName());
        assertEquals(-1, methodDef.getTimeout());

        Map<String,String> annotations = methodDef.getAnnotationList();
        assertNotNull(annotations);
        assertEquals(3, annotations.size());

        String annotationValue = methodDef.getAnnotation(MethodDef.ANNOTATION_METHOD_NOREPLY);
        assertEquals("true", annotationValue);
        assertTrue(methodDef.isNoReply());

        annotationValue = methodDef.getAnnotation(MethodDef.ANNOTATION_DEPRECATED);
        assertEquals("true", annotationValue);
        assertTrue(methodDef.isDeprecated());

        methodDef.setDescription("The FRENCH method description", "fr"); // second language
        annotationValue = methodDef.getDescription("en");
        assertEquals("The method description", annotationValue);
        annotationValue = methodDef.getDescription("fr");
        assertEquals("The FRENCH method description", annotationValue);

        // Define method that has args

        methodDef = new MethodDef("GetSomething", "s", "a{sv}", ifaceName, 1000);
        methodDef.addArg( new ArgDef("item", "s", ArgDef.DIRECTION_IN) );
        methodDef.addArg( argDef = new ArgDef("data", "a{sv}", ArgDef.DIRECTION_OUT) );
        argDef.setDescription("Output arg is an array", "en");

        assertEquals("GetSomething", methodDef.getName());
        assertEquals("s", methodDef.getSignature());
        assertEquals("a{sv}", methodDef.getReplySignature());
        assertEquals(ifaceName, methodDef.getInterfaceName());
        assertEquals(1000, methodDef.getTimeout());
        assertFalse(methodDef.isNoReply());
        assertFalse(methodDef.isDeprecated());

        annotations = methodDef.getAnnotationList();
        assertNotNull(annotations);
        assertEquals(0, annotations.size());

        annotations = argDef.getAnnotationList();
        assertNotNull(annotations);
        assertEquals(1, annotations.size());
        annotationValue = argDef.getDescription("en");
        assertEquals("Output arg is an array", annotationValue);

        List<ArgDef> args = methodDef.getArgList();
        assertNotNull(args);
        assertEquals(2, args.size());

        ArgDef arg = methodDef.getArg("item");
        assertTrue(arg.isInArg());
        assertEquals(ArgDef.DIRECTION_IN, arg.getDirection());
        assertEquals("s", arg.getType());

        arg = methodDef.getArg("data");
        assertTrue(arg.isOutArg());
        assertEquals(ArgDef.DIRECTION_OUT, arg.getDirection());
        assertEquals("a{sv}", arg.getType());

        arg = methodDef.getArg("bogus");
        assertNull(arg);

        annotationValue = methodDef.getAnnotation("org.bogus.annotation.name");
        assertNull(annotationValue);
    }

    public void testSignalDef() throws Exception {
        String ifaceName = "org.alljoyn.FooBar";
        SignalDef signalDef;

        // Define signal that has annotations

        signalDef = new SignalDef("Ping", "s", ifaceName);
        signalDef.addArg( new ArgDef("message", "s") );
        signalDef.setDescription("The signal description", "en"); // an annotation
        signalDef.addAnnotation(SignalDef.ANNOTATION_DEPRECATED, "true");
        signalDef.addAnnotation(SignalDef.ANNOTATION_SIGNAL_SESSIONLESS, "true");
        signalDef.addAnnotation(SignalDef.ANNOTATION_SIGNAL_SESSIONCAST, "false");
        signalDef.addAnnotation(SignalDef.ANNOTATION_SIGNAL_UNICAST, "false");
        signalDef.addAnnotation(SignalDef.ANNOTATION_SIGNAL_GLOBAL_BROADCAST, "false");

        // Test signal definition

        assertEquals("Ping", signalDef.getName());
        assertEquals("s", signalDef.getSignature());
        assertEquals("", signalDef.getReplySignature());
        assertEquals(ifaceName, signalDef.getInterfaceName());

        List<ArgDef> args = signalDef.getArgList();
        assertNotNull(args);
        assertEquals(1, args.size());

        ArgDef arg = signalDef.getArg("message");
        assertTrue(arg.isOutArg());
        assertEquals(ArgDef.DIRECTION_OUT, arg.getDirection());
        assertEquals("s", arg.getType());

        Map<String,String> annotations = signalDef.getAnnotationList();
        assertNotNull(annotations);
        assertEquals(6, annotations.size());

        String annotationValue = signalDef.getAnnotation(SignalDef.ANNOTATION_DEPRECATED);
        assertEquals("true", annotationValue);
        assertTrue(signalDef.isDeprecated());

        assertTrue(signalDef.isSessionless());
        assertFalse(signalDef.isSessioncast());
        assertFalse(signalDef.isUnicast());
        assertFalse(signalDef.isGlobalBroadcast());

        // Test multiple language descriptions

        signalDef.setDescription("The FRENCH signal description", "fr");
        annotationValue = signalDef.getDescription("en");
        assertEquals("The signal description", annotationValue);
        annotationValue = signalDef.getDescription("fr");
        assertEquals("The FRENCH signal description", annotationValue);

        // Test retrieval of undefined annotations and args

        annotationValue = signalDef.getAnnotation("org.bogus.annotation.name");
        assertNull(annotationValue);

        arg = signalDef.getArg("bogus");
        assertNull(arg);
    }

    public void testPropertyDef() throws Exception {
        String ifaceName = "org.alljoyn.FooBar";
        PropertyDef propertyDef;

        // Define property

        propertyDef = new PropertyDef("Color", "s", PropertyDef.ACCESS_READWRITE, ifaceName);
        propertyDef.setDescription("The property description", "en"); // an annotation

        // Test property definition

        assertEquals("Color", propertyDef.getName());
        assertEquals("s", propertyDef.getType());
        assertEquals(PropertyDef.ACCESS_READWRITE, propertyDef.getAccess());
        assertEquals(ifaceName, propertyDef.getInterfaceName());
        assertEquals(-1, propertyDef.getTimeout());

        assertTrue(propertyDef.isReadWriteAccess());
        assertFalse(propertyDef.isReadAccess());
        assertFalse(propertyDef.isWriteAccess());
        assertFalse(propertyDef.isDeprecated());

        Map<String,String> annotations = propertyDef.getAnnotationList();
        assertNotNull(annotations);
        assertEquals(1, annotations.size()); // the description

        // Test multiple language descriptions

        propertyDef.setDescription("The FRENCH property description", "fr");
        String annotationValue = propertyDef.getDescription("en");
        assertEquals("The property description", annotationValue);
        annotationValue = propertyDef.getDescription("fr");
        assertEquals("The FRENCH property description", annotationValue);

        // Test retrieval of EmitsChangedSignal annotation when not set, then add it and retry

        annotationValue = propertyDef.getAnnotation(PropertyDef.ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL);
        assertNull(annotationValue);
        assertFalse(propertyDef.isEmitsChangedSignal());
        assertFalse(propertyDef.isEmitsChangedSignalInvalidates());

        propertyDef.addAnnotation(PropertyDef.ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL, ChangedSignalPolicy.TRUE.text);
        annotationValue = propertyDef.getAnnotation(PropertyDef.ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL);
        assertEquals(ChangedSignalPolicy.TRUE.text, annotationValue);
        assertEquals(3, propertyDef.getAnnotationList().size()); // en/fr descriptions and EmitsChangedSignal annotation
        assertTrue(propertyDef.isEmitsChangedSignal());
        assertFalse(propertyDef.isEmitsChangedSignalInvalidates());
    }

    public void testInterfaceDef() throws Exception {
        String ifaceName = "org.alljoyn.FooBar";
        InterfaceDef interfaceDef;

        // Define interface that has annotations

        interfaceDef = new InterfaceDef(ifaceName);
        interfaceDef.setDescription("The interface description", "en"); // an annotation

        // Test interface definition

        assertEquals(ifaceName, interfaceDef.getName());
        assertFalse(interfaceDef.isAnnounced());
        assertFalse(interfaceDef.isSecureRequired());
        assertTrue(interfaceDef.isSecureInherit()); // security defaults to inherit if not set

        interfaceDef.setAnnounced(true);
        assertTrue(interfaceDef.isAnnounced());

        interfaceDef.addAnnotation(InterfaceDef.ANNOTATION_SECURE, SecurityPolicy.TRUE.text);
        assertTrue(interfaceDef.isSecureRequired());
        assertFalse(interfaceDef.isSecureInherit());

        interfaceDef.addAnnotation(InterfaceDef.ANNOTATION_SECURE, SecurityPolicy.OFF.text); // replace value
        assertFalse(interfaceDef.isSecureRequired());
        assertFalse(interfaceDef.isSecureInherit());

        Map<String,String> annotations = interfaceDef.getAnnotationList();
        assertNotNull(annotations);
        assertEquals(2, annotations.size()); // the description and the Secure annotation

        // Test multiple language descriptions

        interfaceDef.setDescription("The FRENCH interface description", "fr");
        String annotationValue = interfaceDef.getDescription("en");
        assertEquals("The interface description", annotationValue);
        annotationValue = interfaceDef.getDescription("fr");
        assertEquals("The FRENCH interface description", annotationValue);

        // Test retrieval of undefined annotation

        annotationValue = interfaceDef.getAnnotation("org.bogus.annotation.name");
        assertNull(annotationValue);

        // Add methods, signals, properties

        MethodDef methodDef = new MethodDef("GetSomething", "s", "a{sv}", ifaceName);
        methodDef.addArg( new ArgDef("item", "s", ArgDef.DIRECTION_IN) );
        methodDef.addArg( new ArgDef("data", "a{sv}", ArgDef.DIRECTION_OUT) );

        SignalDef signalDef = new SignalDef("Ping", "s", ifaceName);
        signalDef.addArg( new ArgDef("message", "s") );

        PropertyDef propertyDef = new PropertyDef("Color", "s", PropertyDef.ACCESS_READWRITE, ifaceName);

        interfaceDef.addMethod(methodDef);
        interfaceDef.addSignal(signalDef);
        interfaceDef.addProperty(propertyDef);

        assertEquals(1, interfaceDef.getMethods().size());
        assertEquals(1, interfaceDef.getSignals().size());
        assertEquals(1, interfaceDef.getProperties().size());

        assertEquals(methodDef, interfaceDef.getMethods().get(0));
        assertEquals(signalDef, interfaceDef.getSignals().get(0));
        assertEquals(propertyDef, interfaceDef.getProperties().get(0));
    }

    public void testBusObjectInfo() throws Exception {
        String path = "/org/alljoyn/bus";
        List<InterfaceDef> interfaces = new ArrayList<InterfaceDef>();
        InterfaceDef interfaceDef;
        String ifaceName = "org.alljoyn.bus.iface1";

        // Define bus object info with interfaces

        interfaces.add( interfaceDef = new InterfaceDef(ifaceName) );
        interfaces.add( new InterfaceDef("org.alljoyn.bus.iface2") ); // presumably they'd have methods, signals, etc
        interfaces.add( new InterfaceDef("org.alljoyn.bus.iface3") );

        BusObjectInfo busObjInfo = new BusObjectInfo(path, interfaces);
        busObjInfo.addAnnotation(InterfaceDef.ANNOTATION_SECURE, SecurityPolicy.TRUE.text);
        busObjInfo.setDescription("The bus object description", "en"); // an annotation

        // Define methods, signals, properties for one of the interfaces

        MethodDef methodDef = new MethodDef("DoSomething", "", "b", ifaceName);
        methodDef.addArg( new ArgDef("result", "b", ArgDef.DIRECTION_OUT) );

        SignalDef signalDef = new SignalDef("EmitSomething", "s", ifaceName);
        signalDef.addArg( new ArgDef("something", "s") );

        PropertyDef propertyDef = new PropertyDef("SomeProperty", "s", PropertyDef.ACCESS_READWRITE, ifaceName);

        interfaceDef.addMethod(methodDef);
        interfaceDef.addSignal(signalDef);
        interfaceDef.addProperty(propertyDef);

        // Test bus object info definition

        assertEquals(path, busObjInfo.getPath());

        assertNotNull(busObjInfo.getInterfaces());
        assertEquals(3, busObjInfo.getInterfaces().size());
        assertEquals(interfaces, busObjInfo.getInterfaces());

        assertNotNull(busObjInfo.getAnnotationList());
        assertEquals(2, busObjInfo.getAnnotationList().size());
        assertEquals(SecurityPolicy.TRUE.text, busObjInfo.getAnnotation(InterfaceDef.ANNOTATION_SECURE));
        assertEquals("The bus object description", busObjInfo.getDescription("en"));

        // Test retrieving method/signal/property definitions

        assertEquals(interfaceDef, busObjInfo.getInterface(ifaceName));
        assertEquals(methodDef, busObjInfo.getMethod(ifaceName,"DoSomething",""));
        assertEquals(signalDef, busObjInfo.getSignal(ifaceName,"EmitSomething","s"));
        assertEquals(propertyDef, busObjInfo.getProperty(ifaceName,"SomeProperty"));

        assertNull(busObjInfo.getInterface("bogus"));
        assertNull(busObjInfo.getMethod(ifaceName,"bogus",null));
        assertNull(busObjInfo.getSignal(ifaceName,"bogus",null));
        assertNull(busObjInfo.getProperty(ifaceName,"bogus"));
        assertNull(busObjInfo.getSignal(ifaceName,"DoSomething","s"));
    }

}
