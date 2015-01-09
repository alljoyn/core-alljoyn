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
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.PropertyChangedEmitter;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.ifaces.DBusProxyObj;

import java.lang.reflect.Method;
import java.util.Map;
import java.util.TreeMap;

/**
 * Service publishes a set of object properties using the standard DBus
 * properties interface (org.freedesktop.DBus.Properties)
 */
public class Service implements PropsInterface, BusObject {
    static {
        System.loadLibrary("alljoyn_java");
    }

    /* String property */
    private String stringProperty = "Hello";

    /* Integer property */
    private int intProperty = 6;

    /* Bus connection */
    BusAttachment bus = null;

    /**
     * Main entry point for org.alljoyn.bus.samples.props.Service
     */
    public static void main(String[] args) {

        try {
            /* Create a service and wait for Ctrl-C to exit */
            Service cc = new Service(args);
            while (true) {
                Thread.currentThread().sleep(10000);
            }
        } catch (BusException ex) {
            System.err.println("BusException: " + ex.getMessage());
            ex.printStackTrace();
        } catch (InterruptedException ex) {
            System.out.println("Interrupted");
        }
    }

    private Service(String[] args) throws BusException, InterruptedException {

        /* Create a bus connection and connect to the bus */
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        if (Status.OK != status) {
            throw new BusException("BusAttachment.connect() failed with " + status.toString());
        }

        /* Register the service */
        status = bus.registerBusObject(this, "/testProperties");
        if (Status.OK != status) {
            throw new BusException("BusAttachment.registerBusObject() failed: " + status.toString());
        }

        /* Request a well-known name */
        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.samples.props",
                                                                DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        if (res != DBusProxyObj.RequestNameResult.PrimaryOwner) {
            throw new BusException("Failed to obtain well-known name");
        }
    }

    /**
     * @inheritDoc
     */
    public String getStringProp() { return stringProperty; }

    /**
     * @inheritDoc
     */
    public void setStringProp(String stringProperty) { this.stringProperty = stringProperty; }

    /**
     * @inheritDoc
     */
    public int getIntProp() { return intProperty; }

    /**
     * @inheritDoc
     */
    public void setIntProp(int intProperty) throws BusException {
        this.intProperty = intProperty;
        PropertyChangedEmitter emitter = new PropertyChangedEmitter(this);
        try {
            Method m = getClass().getMethod("setIntProp", String.class, String.class, Variant.class);
            BusInterface ifc = m.getAnnotation(BusInterface.class);
            String name = ifc.name();
            emitter.PropertyChanged(name, "IntProp", new Variant(this.intProperty));
        } catch (BusException ex) {
            throw ex;
        } catch (Exception e) {
        }
    }
}
