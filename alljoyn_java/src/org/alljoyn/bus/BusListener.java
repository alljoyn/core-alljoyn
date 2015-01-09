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

package org.alljoyn.bus;

/**
 * Bus listeners are responsible for handling callbacks from the AllJoyn system.
 * It is expected that a user of the AllJoyn bus will specialize this class in
 * order to handle required events from the bus.
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
public class BusListener {

    /**
     * Destroy native resources held by objects of this class.
     */
    public BusListener() {
        create();
    }

    /**
     * Destroy native resources held by objects of this class.
     */
    protected void finalize() throws Throwable {
        try {
            destroy();
        } finally {
            super.finalize();
        }
    }

    /**
     * Create any native resources held by objects of this class.  Specifically,
     * we allocate a C++ counterpart of this listener object.
     */
    private native void create();

    /**
     * Release any native resources held by objects of this class.
     * Specifically, we may delete a C++ counterpart of this listener object.
     */
    private native void destroy();

    /**
     * Called by the bus when the listener is registered. This give the listener implementation the
     * opportunity to save a reference to the bus.
     *
     * @param bus  The bus the listener is registered with.
     */
    public void listenerRegistered(BusAttachment bus) {}

    /**
     * Called by the bus when the listener is unregistered.
     */
    public void listenerUnregistered() {}

    /**
     * Called by the bus when an external bus is discovered that is advertising
     * a well-known name that this attachment has registered interest in via a
     * call to findAdvertisedName
     *
     * Any implementation of this function must be multithread safe.  See the
     * class documentation for details.
     *
     * @param name         A well known name that the remote bus is advertising.
     * @param transport    Transport that received the advertisment.
     * @param namePrefix   The well-known name prefix used in call to
     *                     FindAdvertisedName that triggered this callback.
     */
    public void foundAdvertisedName(String name, short transport, String namePrefix) {}

    /**
     * Called by the bus when an advertisement previously reported through
     * foundAdvertisedName has become unavailable.
     *
     * Any implementation of this function must be multithread safe.  See the
     * class documentation for details.
     *
     * @param name         A well known name that the remote bus is advertising
     *                     that is of interest to this attachment.
     * @param transport    Transport that stopped receiving the given advertised name.
     * @param namePrefix   The well-known name prefix that was used in a call to
     *                     FindAdvertisedName that triggered this callback.
     */
    public void lostAdvertisedName(String name, short transport, String namePrefix) {}

    /**
     * Called by the bus when the ownership of any well-known name changes.
     *
     * Any implementation of this function must be multithread safe.  See the
     * class documentation for details.
     *
     * @param busName        The well-known name that has changed.
     * @param previousOwner  The unique name that previously owned the name or
     *                       NULL if there was no previous owner.
     * @param newOwner       The unique name that now owns the name or NULL if
     *                       there is no new owner.
     */
    public void nameOwnerChanged(String busName, String previousOwner, String newOwner) {}

    /**
     * @deprecated This is deprecated.  It is no longer called when properties change.  Use the property change notification mechanism for proxy bus objects instead.
     *
     * Called by the bus when the value of a property changes if that property has annotation
     * org.freedesktop.DBus.Properties.PropertiesChanged=true
     *
     * Any implementation of this function must be multithread safe.  See the
     * class documentation for details.
     *
     * @param propName       The well-known name that has changed.
     * @param propValue      The new value of the property; NULL if not present
     */
    @Deprecated
    public void propertyChanged(String propName, Variant propValue) {}

    /**
     * Called when a bus that this listener is registered with is stopping.
     *
     * Any implementation of this function must be multithread safe.  See the
     * class documentation for details.
     */
    public void busStopping() {}

    /**
     * Called when a BusAttachment this listener is registered with is has become disconnected from
     * the bus
     *
     * Any implementation of this function must be multithread safe.  See the
     * class documentation for details.
     */
    public void busDisconnected() {}


    /**
     * The opaque pointer to the underlying C++ object which is actually tied
     * to the AllJoyn code.
     */
    private long handle = 0;

}
