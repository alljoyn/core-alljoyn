/**
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

package org.alljoyn.bus;

import java.lang.reflect.Method;
import java.lang.reflect.Type;
import java.util.Map;

/**
 * Implemented by a user-defined classes that is interested in handling
 * property change notification.
 *
 * Listener objects are the Java objects that handle notification events and are
 * called from AllJoyn in the context of one of its threads.  All listener
 * objects are expected to be multithread safe (MT-Safe) between construction
 * and destruction.  That is, every thread executing in a listener object's
 * methods 1) gets a unique copy of all temporary data (it is re-entrant); and
 * 2) all shared data -- the object instance's member variables or any globals
 * must contain no read-modify-write access patterns (okay to write or read,
 * just never to read-modify-write).  If such access patterns are required, it
 * is the responsibility of the client to, for example, add the synchronized
 * keyword when overriding one of the listener methods or provide some other
 * serialization mechanism in order to preserve MT-Safe operation.
 *
 * This rule extends to other objects accessed during processing of
 * notifications.  For example, it is a programming error to allow a notifiation
 * method to update a collection in another object without serializing access
 * to the collection.
 *
 * The important consideration in this case is that as soon as one sets up a
 * listener object to receive notifications from AllJoyn, one is implicitly
 * dealing with multithreaded code.
 *
 * Since listener objects generally run in the context of the AllJoyn thread
 * which manages reception of events, If a blocking AllJoyn call is made in
 * the context of a notification, the necessary and sufficient conditions for
 * deadlock are established.
 *
 * The important consideration in this case is that when one receives a
 * notification from AllJoyn, that notification is executing in the context of
 * an AllJoyn thread.  If one makes a blocking call back into AllJoyn on that
 * thread, a deadlock cycle is likely, and if this happens your bus attachment
 * receive thread will deadlock (with itself).  The deadlock is typically broken
 * after a bus timeout eventually happens.
 */
public abstract class PropertiesChangedListener {

    protected PropertiesChangedListener() {
        try {
            Method m = getClass().getMethod("propertiesChanged", new Class<?>[] {ProxyBusObject.class, String.class, Map.class, String[].class});
            Type p[] = m.getGenericParameterTypes();
            Type changedType = p[2];
            Type invalidatedType = p[3];
            create(changedType, invalidatedType);
        } catch (NoSuchMethodException ex) {
            System.err.println("failed to get propertiesChanged method");  // Should never happen.
        }
    }

    /** Allocate native resources. */
    private native void create(Type changed, Type invalidated);

    /** Release native resources. */
    private synchronized native void destroy();

    /**
     * Called by the bus when the value of a property changes if that property has annotation
     * org.freedesktop.DBus.Properties.EmitChangedSignal=true or
     * org.freedesktop.DBus.Properties.EmitChangedSignal=invalidated
     *
     * Any implementation of this function must be multithread safe.  See the
     * class documentation for details.
     *
     * @param pObj          Bus Object that owns the property that changed.
     * @param ifaceName     The name of the interface the defines the property that changed.
     * @param changed       Property values that changed as an array of dictionary entries, signature "a{sv}".
     * @param invalidated   Properties whose values have been invalidated, signature "as".
     */
    public abstract void propertiesChanged(ProxyBusObject pObj, String ifaceName, Map<String, Variant> changed, String[] invalidated);

    /**
     * The opaque pointer to the underlying C++ object which is actually tied
     * to the AllJoyn code.
     */
    private long handle = 0;
}
