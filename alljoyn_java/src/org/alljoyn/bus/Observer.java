/*
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

import java.util.NavigableMap;
import java.util.TreeMap;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;

import org.alljoyn.bus.annotation.BusInterface;

/**
 * A convenient object discovery and access mechanism.
 * <p>
 * An Observer takes care of discovery, session management and ProxyBusObject
 * creation for bus objects that implement a specific set of interfaces.
 * <p>
 * The Observer monitors About announcements, and automatically sets up sessions
 * with all peers that offer objects of interest (i.e. objects that implement at
 * least the set of mandatory interfaces for this Observer). The Observer creates
 * a proxy bus object for each discovered object. The Observer::Listener class
 * is used to inform the application about the discovery of new objects, and about
 * the disappearance of objects from the bus.
 * <p>
 * Objects are considered lost in the following cases:
 * <ul>
 * <li> they are un-announced via About
 * <li> the hosting peer has closed the session
 * <li> the hosting peer stopped responding to Ping requests
 * </ul>
 */
public class Observer {
    /**
     * Listener for object lifecycle events
     */
    public interface Listener {
        /**
         * A new object has been discovered.
         *
         * @param object a proxy for the discovered object, supporting all
         *               mandatory interfaces, and those optional interfaces
         *               that the object implements.
         */
        void ObjectDiscovered(ProxyBusObject object);

        /**
         * A previously discovered object has been lost.
         *
         * The proxy bus object is from this point on invalid. Even if the
         * object reappears on the bus, you'll need a new proxy object (and
         * objectDiscovered will provide you with that new proxy) to interact
         * with the bus object.
         *
         * @param object the proxy for the object that has been lost.
         */
        void ObjectLost(ProxyBusObject object);
    };

    /**
     * Create an Observer that discovers all objects implementing a minimum set
     * of Interfaces.
     *
     * @param  bus  the bus attachment to be used by this Observer
     * @param  mandatoryInterfaces the minimal set of Interfaces that have to
     *                             be implemented by a bus object for the
     *                             Observer to discover that object.
     * @param optionalInterfaces additional Interfaces that will be added to the
     *                           {@link ProxyBusObject}s managed created by this
     *                           Observer, if they are supported by the
     *                           discovered bus objects.
     */
    public Observer(BusAttachment bus, Class<?>[] mandatoryInterfaces, Class<?>[] optionalInterfaces) {
        this.bus = bus;
        proxies = new TreeMap<ObjectId, ProxyBusObject>();
        listeners = new ArrayList<WrappedListener>();
        interfaceMap = new HashMap<String, Class<?>>();

        /* build the list of mandatory AllJoyn interface names */
        String[] mandatoryNames = new String[mandatoryInterfaces.length];
        for (int i = 0; i < mandatoryInterfaces.length; ++i) {
            String name = getBusInterfaceName(mandatoryInterfaces[i]);
            mandatoryNames[i] = name;
            interfaceMap.put(name, mandatoryInterfaces[i]);
        }
        if (optionalInterfaces != null) {
            for (Class<?> intf : optionalInterfaces) {
                interfaceMap.put(getBusInterfaceName(intf), intf);
            }
        }

        create(bus, mandatoryNames);
    }

    /**
     * Create an Observer that discovers all objects implementing a minimum set
     * of Interfaces.
     *
     * @param  bus  the bus attachment to be used by this Observer
     * @param  mandatoryInterfaces the minimal set of Interfaces that have to
     *                             be implemented by a bus object for the
     *                             Observer to discover that object.
     */
    public Observer(BusAttachment bus, Class<?>[] mandatoryInterfaces) {
        this(bus, mandatoryInterfaces, new Class<?>[] {});
    }

    /**
     * Register an {@link Observer.Listener}.
     *
     * @param listener the listener to register
     * @param triggerOnExisting indicates whether the listener's
     *                          objectDiscovered callback should be invoked for
     *                          all objects that have been discovered prior to
     *                          the registration of this listener.
     */
    public synchronized void registerListener(Listener listener, boolean triggerOnExisting) {
        listeners.add(new WrappedListener(listener, !triggerOnExisting));
        if (triggerOnExisting) {
            triggerEnablePendingListeners();
        }
    }

    /**
     * Register an {@link Observer.Listener}.
     *
     * The listener's objectDiscovered callback will be invoked for all objects
     * that have been discovered prior to the registration of the listener.
     *
     * @param listener the listener to register
     */
    public synchronized void registerListener(Listener listener) {
        registerListener(listener, true);
    }

    /**
     * Unregister a previously registered {@link Observer.Listener}.
     *
     * @param listener the listener to unregister
     */
    public synchronized void unregisterListener(Listener listener) {
        for (int i = 0; i < listeners.size(); ++i) {
            if (listeners.get(i).listener == listener) {
                listeners.remove(i);
                break;
            }
        }
    }

    /**
     * Unregister all listeners.
     */
    public synchronized void unregisterAllListeners() {
        listeners.clear();
    }

    /**
     * Retrieve a {@link ProxyBusObject}.
     *
     * If the supplied (busname, path) pair does not identify an object that
     * has been discovered by this Observer, or identifies an object that has
     * since disappeared from the bus, null will be returned.
     *
     * @param busname  unique bus name of the peer hosting the bus object
     * @param path the object's path
     * @return the ProxyBusObject or null if not found
     */
    public synchronized ProxyBusObject get(String busname, String path) {
        ObjectId oid = new ObjectId(busname, path);
        return proxies.get(oid);
    }

    /**
     * Retrieve the first {@link ProxyBusObject}.
     *
     * The getFirst/getNext pair of methods is used for iteration over all
     * discovered objects. null is returned to signal end of iteration.
     *
     * @return the first ProxyBusObject or null if no objects discovered
     */
    public synchronized ProxyBusObject getFirst() {
        if (proxies.size() == 0) {
            return null;
        }
        return proxies.firstEntry().getValue();
    }

    /**
     * Retrieve the next {@link ProxyBusObject}.
     *
     * The getFirst/getNext pair of methods is used for iteration over all
     * discovered objects. null is returned to signal end of iteration.
     *
     * @param object the previous ProxyBusObject
     * @return the next ProxyBusObject or null if iteration is finished
     */
    public synchronized ProxyBusObject getNext(ProxyBusObject object) {
        if (object == null) {
            return null;
        }

        Map.Entry<ObjectId, ProxyBusObject> entry = proxies.ceilingEntry(new ObjectId(object));
        if (entry == null) {
            return null;
        }

        return entry.getValue();
    }

    /**
     * Let the Java garbage collector release resources.
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
     * Points to the underlying native JObserver
     */
    private long handle;

    /**
     * Create native counterpart.
     */
    private native void create(BusAttachment bus, String[] interfaceNames);

    /**
     * Destroy native counterpart.
     */
    private synchronized native void destroy();

    /**
     * Make native call us back on the dispatcher thread.
     */
    private native void triggerEnablePendingListeners();

    /**
     * Encapsulates the unique id of a bus object (the busname/path pair).
     */
    private static class ObjectId implements Comparable<ObjectId> {
        /**
         * unique bus name
         */
        public String busname;

        /**
         * object path
         */
        public String path;

        ObjectId(String _busname, String _path) {
            busname = _busname;
            path = _path;
        }

        ObjectId(ProxyBusObject proxy) {
            this(proxy.getBusName(), proxy.getObjPath());
        }

        @Override
        public int hashCode() {
            return busname.hashCode() ^ path.hashCode();
        }

        @Override
        public boolean equals(Object other) {
            if (other == this) {
                return true;
            }
            if (other == null) {
                return false;
            }
            if (other.getClass() != getClass()) {
                return false;
            }

            ObjectId oid = (ObjectId) other;
            return busname.equals(oid.busname) && path.equals(oid.path);
        }

        @Override
        public int compareTo(ObjectId oid) {
            if (oid == null) {
                return 1;
            }

            int nameCmp = busname.compareTo(oid.busname);
            if (nameCmp == 0) {
                return path.compareTo(oid.path);
            }
            return nameCmp;
        }
    };

    /**
     * Wraps a listener to add some bookkeeping information.
     */
    private static class WrappedListener {
        /**
         * Is the listener enabled?
         *
         * triggerOnExisting listeners start out disabled, they get enabled
         * when native calls us back on the dispatcher thread via
         * enablePendingListeners.
         */
        public boolean enabled;

        /**
         * the listener
         */
        public Listener listener;

        WrappedListener(Listener l, boolean enable) {
            enabled = enable;
            listener = l;
        }
    };

    /**
     * Enable pending listeners.
     *
     * Listeners that are registered with triggerOnExisting == true, start off
     * in pending state. That means we don't dispatch callbacks to them until
     * we've been able to invoke objectDiscovered for all objects that are
     * already in our proxy list. Native calls us via this method to do those
     * initial callbacks.
     */
    private void enablePendingListeners() {
        ArrayList<Listener> pendingListeners = new ArrayList<Listener>();
        synchronized(this) {
            for (WrappedListener wl : listeners) {
                if (!wl.enabled) {
                    pendingListeners.add(wl.listener);
                    wl.enabled = true;
                }
            }
        }

        /* we have threading guarantees from the Core that the proxies set will not change
         * during this call. Therefore, we don't need to do anything special to safeguard
         * proxies during iteration. */
        for (Listener l : pendingListeners) {
            for (ProxyBusObject proxy : proxies.values()) {
                l.ObjectDiscovered(proxy);
            }
        }
    }

    /**
     * Native tells us a new object was discovered.
     *
     * @param busname unique bus name
     * @param path the object path
     * @param interfaces list of announced AllJoyn Interfaces for the discovered object
     * @param sessionId id of the session we can use to construct a ProxyBusObject
     */
    private void objectDiscovered(String busname, String path, String[] interfaces, int sessionId) {
        ArrayList<Class<?>> intfList = new ArrayList<Class<?>>();
        for (String intfName : interfaces) {
            Class<?> intf = interfaceMap.get(intfName);
            if (intf != null) {
                intfList.add(intf);
            }
        }
        //TODO figure out what to do with secure bus objects
        ProxyBusObject proxy = bus.getProxyBusObject(busname, path, sessionId, intfList.toArray(new Class<?>[]{}));

        WrappedListener[] copiedListeners;
        synchronized(this) {
            proxies.put(new ObjectId(busname, path), proxy);
            copiedListeners = listeners.toArray(new WrappedListener[]{});
        }

        /* we do the listener invocation outside of the critical region to avoid
         * lock ordering issues */
        for (WrappedListener wl : copiedListeners) {
            if (wl.enabled) {
                wl.listener.ObjectDiscovered(proxy);
            }
        }
    }

    /**
     * Native tells us an object has disappeared.
     *
     * @param busname unique bus name
     * @param path object path
     */
    private void objectLost(String busname, String path) {
        WrappedListener[] copiedListeners = null;
        ProxyBusObject obj = null;
        synchronized(this) {
            ObjectId oid = new ObjectId(busname, path);
            obj = proxies.remove(oid);
            if (obj != null) {
                copiedListeners = listeners.toArray(new WrappedListener[]{});
            }
        }

        if (obj == null) {
            return;
        }

        /* we do the listener invocation outside of the critical region to avoid
         * lock ordering issues */
        for (WrappedListener wl : copiedListeners) {
            if (wl.enabled) {
                wl.listener.ObjectLost(obj);
            }
        }
    }

    /**
     * Extract AllJoyn interface name from a Java interface.
     */
    private static String getBusInterfaceName(Class<?> intf) {
         BusInterface annotation = intf.getAnnotation(BusInterface.class);
         if (annotation != null && !annotation.name().equals("")) {
             return annotation.name();
         }
         return intf.getCanonicalName();
    }

    /** Bus attachment used by this Observer */
    private BusAttachment bus;
    /** Proxies for the discovered objects */
    private NavigableMap<ObjectId, ProxyBusObject> proxies;
    /** Registered listeners */
    private List<WrappedListener> listeners;
    /** Maps AllJoyn interface names to Java interfaces (needed for ProxyBusObject creation) */
    private HashMap<String, Class<?>> interfaceMap;
}
