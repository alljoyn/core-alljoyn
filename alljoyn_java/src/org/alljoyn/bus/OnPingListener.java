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

/**
 * An OnPingListener is responsible for receiving completion indications
 * from asynchronous ping operations.  It is expected that an AllJoyn session
 * user will specialize this class in order to handle the callback.
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
 * receiver thread will deadlock (with itself).  The deadlock is typically broken
 * after a bus timeout eventually happens.
 */
public class OnPingListener {
    /**
     * Create native resources held by objects of this class.
     */
    public OnPingListener() {
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
     * Determine if you are able to find a remote connection based on its BusName.
     * The BusName can be the Unique or well-known name.
     *
     * This call executes asynchronously. When the ping response is received,
     * the callback will be called.
     *
     * @param status
     * <ul>
     * <li>OK the name is present and responding</li>
     * <li>ALLJOYN_PING_REPLY_UNREACHABLE the name is no longer present</li>
     * </ul>
     * The following return values indicate that the router cannot determine if the 
     * remote name is present and responding:
     * <ul>
     * <li>ALLJOYN_PING_REPLY_TIMEOUT Ping call timed out</li>
     * <li>ALLJOYN_PING_REPLY_UNKNOWN_NAME name not found currently or not part of any known session</li>
     * <li>ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE the remote routing node does not implement Ping</li>
     * </ul>
     * The following return values indicate an error with the ping call itself:
     * <ul>
     * <li>ALLJOYN_PING_FAILED Ping failed</li>
     * <li>BUS_UNEXPECTED_DISPOSITION An unexpected disposition was returned and has been treated as an error</li>
     * </ul>
     * @param context   The user-defined context object supplied in the call to {@link
     *                  BusAttachment#ping(String, int, OnPingListener, Object)}.
     */
    public void onPing(Status status, Object context) {
    }

    /*
     * The opaque pointer to the underlying C++ object which is actually tied
     * to the AllJoyn code.
     */
    private long handle = 0;
}
