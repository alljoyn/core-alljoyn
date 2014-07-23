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

package org.alljoyn.bus.samples.props;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.PropertyChangedListener;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.ifaces.Properties;

import java.util.Map;

/**
 * Client accesses properties of the org.alljoyn.samples.props_interface implemented
 * by the /testProperties object of org.alljoyn.samples.props namespace.
 */
public class Client extends PropertyChangedListener {
    static {
        System.loadLibrary("alljoyn_java");
    }

    /** Main entry point */
    public static void main(String[] args) {

        try {
            Client client = new Client(args);
        } catch (BusException ex) {
            System.err.println("BusException: " + ex.getMessage());
            ex.printStackTrace();
        }
    }

    private Client(String[] args) throws BusException {

        /* Create a bus connection and connect to the bus */
        BusAttachment bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        if (Status.OK != status) {
            System.out.println("BusAttachment.connect() failed with " + status.toString());
            return;
        }

        /* Get a remote object */
        ProxyBusObject proxyObj =  bus.getProxyBusObject("org.alljoyn.bus.samples.props",
                                                          "/testProperties",
                                                          0, // zero = sessions are not used
                                                          new Class<?>[] { PropsInterface.class,
                                                                           Properties.class });
        PropsInterface proxy = proxyObj.getInterface(PropsInterface.class);

        proxyObj.registerPropertyChangedHandler("org.alljoyn.bus.samples.props.PropsInterface",
                                                "IntProp",
                                                this);

        /* Get a property */
        System.out.println("StringProp = " + proxy.getStringProp());

        /* Set a property */
        proxy.setStringProp("MyNewValue");

        /* Set the IntProp to elicit a call to the property change handler. */
        proxy.setIntProp(42);

        try {
            /* Pause abit for the property change handler to be called. */
            Thread.sleep(1000);
        } catch (Exception ex) {
            /* (We don't really care if Thread.sleep() failed in this sample.) */
        }

        /* Get all of the properties of the interface */
        System.out.println("StringProp = " + proxy.getStringProp());
        System.out.println("IntProp = " + proxy.getIntProp());

        /* Use the org.freedesktop.DBus.Properties interface to get all the properties */
        Properties properties = proxyObj.getInterface(Properties.class);
        Map<String, Variant> map = properties.GetAll("org.alljoyn.bus.samples.props.PropsInterface");
        System.out.println("StringProp = " + map.get("StringProp").getObject(String.class));
        System.out.println("IntProp = " + map.get("IntProp").getObject(Integer.class));

        proxyObj.unregisterPropertyChangedHandler("org.alljoyn.bus.samples.props.PropsInterface",
                                                  "IntProp",
                                                  this);
    }


    /**
     * Called by the bus when the value of a property changes if that property has annotation
     * org.freedesktop.DBus.Properties.EmitChangedSignal=true or
     * org.freedesktop.DBus.Properties.EmitChangedSignal=invalidated
     * 
     * Any implementation of this function must be multithread safe.  See the
     * class documentation for details.
     *
     * @param pObj           Bus Object that owns the property that changed.
     * @param ifaceName      The name of the interface the defines the property that changed.
     * @param propName       The name of the property that has changed.
     * @param propValue      The new value of the property; NULL to simply indicate that any local copy should now be considered invalidated.
     */
    public void propertyChanged(ProxyBusObject pObj, String ifaceName, String propName, Variant propValue) {
        if (propValue == null) {
            /* When propValue is null, the client needs to mark any local copies as invalid and get an updated value. */
            System.out.println(ifaceName + "." + propName + " from Bus Object " + pObj.getObjPath() + " at " + pObj.getBusName() + " was invalidated.");
        } else {
            System.out.println(ifaceName + "." + propName + " from Bus Object " + pObj.getObjPath() + " at " + pObj.getBusName() + " has a new value: " + propValue);
        }
    }
}
