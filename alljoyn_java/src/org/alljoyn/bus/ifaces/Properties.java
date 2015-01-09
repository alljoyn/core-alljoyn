/*
 * Copyright (c) 2009-2011, 2014 AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.ifaces;

import java.util.Map;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusSignal;

/**
 * The standard org.freedesktop.DBus.Properties interface that can be
 * implemented by bus objects to expose a generic "setter/getter" inteface for
 * user-defined properties on DBus.
 */
@BusInterface(name = "org.freedesktop.DBus.Properties")
public interface Properties {

    /**
     * Gets a property that exists on a named interface of a bus object.
     *
     * @param iface the interface that the property exists on
     * @param propName the name of the property
     * @return the value of the property
     * @throws BusException if the named property doesn't exist
     */
    @BusMethod
    Variant Get(String iface, String propName) throws BusException;

    /**
     * Sets a property that exists on a named interface of a bus object.
     *
     * @param iface the interface that the property exists on
     * @param propName the name of the property
     * @param value the value for the property
     * @throws BusException if the named property doesn't exist or cannot be set
     */
    @BusMethod
    void Set(String iface, String propName, Variant value) throws BusException;

    /**
     * Gets all properties for a given interface.
     *
     * @param iface the interface
     * @return a Map of name/value associations
     * @throws BusException if request cannot be honored
     */
    @BusMethod(signature = "s", replySignature = "a{sv}")
    Map<String, Variant> GetAll(String iface) throws BusException;

    /**
     * Notifies others about changes to properties.
     *
     * @param iface the interface
     * @param changedProps a map of property names an their new values
     * @param invalidatedProps a list of property names whose values are invalidated
     * @throws BusException indicating failure sending PropertiesChanged signal
     */
    @BusSignal(signature = "sa{sv}as")
    void PropertiesChanged(String iface, Map<String, Variant> changedProps, String [] invalidatedProps) throws BusException;
}
