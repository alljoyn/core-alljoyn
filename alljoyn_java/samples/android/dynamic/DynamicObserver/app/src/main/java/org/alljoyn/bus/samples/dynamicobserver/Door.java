/*
 * Copyright AllSeen Alliance. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

package org.alljoyn.bus.samples.dynamicobserver;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.GenericInterface;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.defs.ArgDef;
import org.alljoyn.bus.defs.BusObjectInfo;
import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.MethodDef;
import org.alljoyn.bus.defs.PropertyDef;
import org.alljoyn.bus.defs.SignalDef;

import java.util.ArrayList;
import java.util.List;

/**
 * Helper class that facilitates proxy access to send Door method/signal/property calls to a
 * remote bus object. Also used to build dynamic definition of the Door interface (describing how
 * a door looks on the AllJoyn bus).
 *
 * The constructor taking a ProxyBusObject is used by Door interface consumers wishing to send bus
 * method/property calls to a Door interface provider. The constructor taking a SignalEmitter is
 * used by Door interface providers wishing to send bus signal calls to Door interface consumers.
 */
public class Door {

    public static final String INTERFACE = "com.example.Door";

    public static final String PROPERTY_IS_OPEN = "IsOpen";
    public static final String PROPERTY_LOCATION = "Location";
    public static final String PROPERTY_KEY_CODE = "KeyCode";

    public static final String METHOD_OPEN = "Open";
    public static final String METHOD_CLOSE = "Close";
    public static final String METHOD_KNOCK_AND_RUN = "KnockAndRun";

    public static final String SIGNAL_PERSON_PASSED_THROUGH = "PersonPassedThrough";

    private GenericInterface mInterface;

    /**
     * Build the Door interface definition for the given bus path.
     *
     * Note: Real-world applications would likely determine the interface(s) definition dynamically
     * (e.g. discovered via introspection or some other retrieval mechanism).
     *
     * @param path The bus object path used in the object definition.
     * @return BusObjectInfo An object definition containing the given path and a generated
     *         list of dynamic interface definitions.
     */
    public static BusObjectInfo getBusObjectInfo(String path) {
        InterfaceDef interfaceDef = new InterfaceDef(INTERFACE, true, null);  // isAnnounced true

        // Define property: IsOpen [boolean, read-only]
        PropertyDef propertyDef = new PropertyDef(PROPERTY_IS_OPEN, "b", PropertyDef.ACCESS_READ, INTERFACE);
        propertyDef.addAnnotation(PropertyDef.ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL, PropertyDef.ChangedSignalPolicy.TRUE.text);
        interfaceDef.addProperty(propertyDef);

        // Define property: Location [string, read-only]
        propertyDef = new PropertyDef(PROPERTY_LOCATION, "s", PropertyDef.ACCESS_READ, INTERFACE);
        propertyDef.addAnnotation(PropertyDef.ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL, PropertyDef.ChangedSignalPolicy.TRUE.text);
        interfaceDef.addProperty(propertyDef);

        // Define property: KeyCode [int, read-only]
        propertyDef = new PropertyDef(PROPERTY_KEY_CODE, "u", PropertyDef.ACCESS_READ, INTERFACE);
        propertyDef.addAnnotation(PropertyDef.ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL, PropertyDef.ChangedSignalPolicy.INVALIDATES.text);
        interfaceDef.addProperty(propertyDef);

        // Define method: void Open()
        MethodDef methodDef = new MethodDef(METHOD_OPEN, "", "", INTERFACE);
        methodDef.setDescription("Open the door", "en");
        interfaceDef.addMethod(methodDef);

        // Define method: void Close()
        methodDef = new MethodDef(METHOD_CLOSE, "", "", INTERFACE);
        methodDef.setDescription("Close the door", "en");
        interfaceDef.addMethod(methodDef);

        // Define method: void KnockAndRun()
        methodDef = new MethodDef(METHOD_KNOCK_AND_RUN, "", "", INTERFACE);
        methodDef.addAnnotation(MethodDef.ANNOTATION_METHOD_NOREPLY, "true");
        methodDef.setDescription("Knock on door and run", "en");
        interfaceDef.addMethod(methodDef);

        // Define signal: PersonPassedThrough(String person)
        SignalDef signalDef = new SignalDef(SIGNAL_PERSON_PASSED_THROUGH, "s", INTERFACE);
        signalDef.addArg(new ArgDef("person", "s"));
        signalDef.setDescription("Person passed through door", "en");
        interfaceDef.addSignal(signalDef);

        // Return the bus object info, which is the object path and interface definition.
        List<InterfaceDef> ifaces = new ArrayList<InterfaceDef>();
        ifaces.add(interfaceDef);
        return new BusObjectInfo(path, ifaces);
    }


    /**
     * Constructor. Allows a door interface consumer to send bus method/property calls to a provider.
     * The created Door wraps a generic interface (on the given proxy) to a remote bus object.
     *
     * @param proxy
     */
    public Door(ProxyBusObject proxy) {
        mInterface = proxy.getInterface(GenericInterface.class);
    }

    /**
     * Constructor. Allows a door interface provider to send bus signal calls to consumers.
     * The created Door wraps a generic interface (on the given emitter) to a remote bus object.
     *
     * @param emitter
     */
    public Door(SignalEmitter emitter) {
        mInterface = emitter.getInterface(GenericInterface.class);
    }


    /* Helper method to send getIsOpen property call (via generic interface proxy) to remote bus object. */
    public boolean getIsOpen() throws BusException {
        return (Boolean) mInterface.getProperty(INTERFACE, PROPERTY_IS_OPEN);
    }

    /* Helper method to send getLocation property call (via generic interface proxy) to remote bus object. */
    public String getLocation() throws BusException {
        return (String) mInterface.getProperty(INTERFACE, PROPERTY_LOCATION);
    }

    /* Helper method to send getKeyCode property call (via generic interface proxy) to remote bus object. */
    public int getKeyCode() throws BusException {
        return (Integer) mInterface.getProperty(INTERFACE, PROPERTY_KEY_CODE);
    }

    /* Helper method to send open() method call (via generic interface proxy) to remote bus object. */
    public void open() throws BusException {
        mInterface.methodCall(INTERFACE, METHOD_OPEN);
    }

    /* Helper method to send close() method call (via generic interface proxy) to remote bus object. */
    public void close() throws BusException {
        mInterface.methodCall(INTERFACE, METHOD_CLOSE);
    }

    /* Helper method to send knockAndRun() method call (via generic interface proxy) to remote bus object. */
    public void knockAndRun() throws BusException {
        mInterface.methodCall(INTERFACE, METHOD_KNOCK_AND_RUN);
    }

    /* Helper method to send personPassedThough() signal (via generic interface proxy) to remote bus object. */
    public void personPassedThrough(String person) throws BusException {
        mInterface.signal(INTERFACE, SIGNAL_PERSON_PASSED_THROUGH, person);
    }

}