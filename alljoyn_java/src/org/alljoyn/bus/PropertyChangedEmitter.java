/*
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.alljoyn.bus.annotation.BusAnnotation;
import org.alljoyn.bus.annotation.BusAnnotations;
import org.alljoyn.bus.annotation.BusProperty;
import org.alljoyn.bus.ifaces.Properties;

/**
 * A helper proxy used by BusObjects to send property change notifications.  A PropertyChangedlEmitter
 * instance can be used to send any signal from a given AllJoyn interface.
 */
public class PropertyChangedEmitter extends SignalEmitter {

    private final Properties props;

    /**
     * Constructs a PropertyChangedEmitter.
     *
     * @param source the source object of any signals sent from this emitter
     * @param sessionId A unique SessionId for this AllJoyn session instance
     * @param globalBroadcast whether to forward broadcast signals across bus-to-bus connections
     */
    public PropertyChangedEmitter(BusObject source, int sessionId, GlobalBroadcast globalBroadcast) {
        super(source, null, sessionId, globalBroadcast);
        props = getInterface(Properties.class);
    }

    /**
     * Constructs a PropertyChangedEmitter.
     *
     * @param source the source object of any signals sent from this emitter
     * @param globalBroadcast whether to forward broadcast signals across bus-to-bus connections
     */
    public PropertyChangedEmitter(BusObject source, GlobalBroadcast globalBroadcast) {
        this(source, BusAttachment.SESSION_ID_ANY, globalBroadcast);
    }

    /**
     * Constructs a PropertyChangedEmitter.
     *
     * @param source the source object of any signals sent from this emitter
     * @param sessionId A unique SessionId for this AllJoyn session instance
     */
    public PropertyChangedEmitter(BusObject source, int sessionId) {
        this(source, sessionId, GlobalBroadcast.Off);
    }

    /**
     * Constructs a PropertyChangedEmitter used for broadcasting.
     *
     * @param source the source object of any signals sent from this emitter
     */
    public PropertyChangedEmitter(BusObject source) {
        this(source, BusAttachment.SESSION_ID_ANY);
    }

    /**
     * Sends the PropertyChanged signal.
     *
     * @param ifaceName name of the interface the property belongs to
     * @param propertyName name of the property that is changed
     * @param newValue Variant containing the new property value
     * @throws BusException indicating failure to send the PropertyChanged signal
     */
    public void PropertyChanged(String ifaceName, final String propertyName, final Variant newValue)
        throws BusException {
        Map<String, Variant> propsChanged = new HashMap<String, Variant>();
        String[] invalidatedProps;
        if (newValue == null) {
            invalidatedProps = new String[] {propertyName};
        } else {
            invalidatedProps = new String[] {};
            propsChanged.put(propertyName, newValue);
        }
        props.PropertiesChanged(ifaceName, propsChanged, invalidatedProps);
    }

    /**
     * Sends the PropertiesChanged signal
     *
     * @param iface the BusInterface the property belongs to
     * @param properties list of properties that were changed
     * @throws BusException indicating failure to send the PropertiesChanged signal
     */
    public void PropertiesChanged(Class<?> iface, Set<String> properties)
        throws BusException {
        String ifaceName = InterfaceDescription.getName(iface);
        Map<String, Variant> changedProps = new HashMap<String, Variant>();
        List<String> invalidatedProps = new ArrayList<String>();

        for (String propName : properties) {
            Method m = null;
            try {
                // try to find the get method
                m = iface.getMethod("get" + propName);
            } catch (NoSuchMethodException ex) {
                throw new IllegalArgumentException("Not property with name " + propName + " found");
            }
            BusProperty busPropertyAnn = m.getAnnotation(BusProperty.class);
            if (busPropertyAnn != null) {
                // need to emit
                BusAnnotations bas = m.getAnnotation(BusAnnotations.class);
                if (bas != null) {
                    for (BusAnnotation ba : bas.value()) {
                        if (ba.name().equals("org.freedesktop.DBus.Property.EmitsChangedSignal")) {
                            if (ba.value().equals("true")) {
                                Object o;
                                try {
                                    o = m.invoke(source);
                                } catch (Exception ex) {
                                    throw new BusException("can't get value of property " + propName, ex);
                                }
                                Variant v;
                                if (busPropertyAnn.signature() != null && !busPropertyAnn.signature().isEmpty()) {
                                    v = new Variant(o, busPropertyAnn.signature());
                                } else {
                                    v = new Variant(o);
                                }
                                changedProps.put(propName, v);
                            } else if (ba.value().equals("invalidates")) {
                                invalidatedProps.add(propName);
                            }
                        }
                    }
                }
            }
        }

        props.PropertiesChanged(ifaceName, changedProps, invalidatedProps.toArray(new String[invalidatedProps.size()]));
    }
}
