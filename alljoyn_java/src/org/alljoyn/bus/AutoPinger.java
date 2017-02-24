/*
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

public class AutoPinger {

    /**
     * Create native resources held by objects of this class.
     *
     * @param bus the BusAttachment this About BusObject will be registered with
     */
    public AutoPinger(BusAttachment bus) {
        create(bus);
    }

    /**
     * Destroy native resources held by objects of this class.
     */
    @Override
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
    private native void create(BusAttachment bus);

    /**
     * Release any native resources held by objects of this class.
     * Specifically, we may delete a C++ counterpart of this object.
     */
    private native void destroy();

    /**
     * Pause all ping actions
     */
    public native void pause();

    /**
     * Resume ping actions
     */
    public native void resume();

    /**
     * Define new ping group
     *
     * @param  group Ping group name
     * @param  listener Listener called when a change was detected in the reachability of a destination
     * @param  pingInterval Ping interval in seconds
     */
    public native void addPingGroup(String group, AutoPingListener listener, int pingInterval);

    /**
     * Remove complete ping group, including all destinations
     *
     * Do not invoke this method from within a PingListener callback. This will
     * cause a deadlock.
     *
     * @param  group Ping group name
     */
    public native void removePingGroup(String group);

    /**
     * Set ping interval of the specified group
     *
     * @param  group Ping group name
     * @param  pingInterval Ping interval in seconds
     * @return
     *  - #ER_OK: Interval updated
     *  - #ER_BUS_PING_GROUP_NOT_FOUND: group did not exist
     */
    public native Status setPingInterval(String group, int pingInterval);

    /**
     * Add a destination to the specified ping group
     * Destinations are refcounted and must be removed N times if they were added N times
     *
     * @param  group Ping group name
     * @param  destination Destination name to be pinged
     * @return
     *  - #ER_OK: destination added
     *  - #ER_BUS_PING_GROUP_NOT_FOUND: group did not exist
     */
    public native Status addDestination(String group, String destination);

    /**
     * Remove a destination from the specified ping group
     * This will lower the refcount by one and only remove the destination when the refcount reaches zero
     *
     * @param  group Ping group name
     * @param  destination Destination name to be removed
     * @param  removeAll Rather than decrementing the refcount by one, set refcount to zero and remove
     * @return
     *  - #ER_OK: destination removed or was not present
     *  - #ER_BUS_PING_GROUP_NOT_FOUND: group did not exist
     */
    public native Status removeDestination(String group, String destination, boolean removeAll);

    /**
     * Remove a destination from the specified ping group
     * This will lower the refcount by one and only remove the destination when the refcount reaches zero
     *
     * @param  group Ping group name
     * @param  destination Destination name to be removed
     * @return
     *  - #ER_OK: destination removed or was not present
     *  - #ER_BUS_PING_GROUP_NOT_FOUND: group did not exist
     */
    public Status removeDestination(String group, String destination) {
        return removeDestination(group, destination, false);
    }

    private long handle;
}