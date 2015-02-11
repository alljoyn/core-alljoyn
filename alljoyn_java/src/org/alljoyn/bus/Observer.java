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
 *
 * An Observer takes care of discovery, session management and ProxyBusObject
 * creation for bus objects that implement a specific set of interfaces.
 *
 * The Observer monitors About announcements, and automatically sets up sessions
 * with all peers that offer objects of interest (i.e. objects that implement at
 * least the set of mandatory interfaces for this Observer). The Observer creates
 * a proxy bus object for each discovered object. The Observer::Listener class
 * is used to inform the application about the discovery of new objects, and about
 * the disappearance of objects from the bus.
 *
 * Objects are considered lost in the following cases:
 * - they are un-announced via About
 * - the hosting peer has closed the session
 * - the hosting peer stopped responding to Ping requests
 */
public class Observer {
    public interface Listener {
        void ObjectDiscovered(ProxyBusObject object);
        void ObjectLost(ProxyBusObject object);
    };

    public Observer(BusAttachment _bus, Class<?>[] mandatoryInterfaces, Class<?>[] optionalInterfaces) {
        bus = _bus;
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

    public Observer(BusAttachment bus, Class<?>[] mandatoryInterfaces) {
        this(bus, mandatoryInterfaces, new Class<?>[] {});
    }

    public synchronized void registerListener(Listener listener, boolean triggerOnExisting) {
        listeners.add(new WrappedListener(listener, !triggerOnExisting));
        if (triggerOnExisting) {
            triggerEnablePendingListeners();
        }
    }

    public synchronized void registerListener(Listener listener) {
        registerListener(listener, true);
    }

    public synchronized void unregisterListener(Listener listener) {
        for (int i = 0; i < listeners.size(); ++i) {
            if (listeners.get(i).listener == listener) {
                listeners.remove(i);
                break;
            }
        }
    }

    public synchronized void unregisterAllListeners() {
        listeners.clear();
    }

    public synchronized ProxyBusObject get(String busname, String path) {
        ObjectId oid = new ObjectId(busname, path);
        return proxies.get(oid);
    }

    public synchronized ProxyBusObject getFirst() {
        if (proxies.size() == 0) {
            return null;
        }
        return proxies.firstEntry().getValue();
    }

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

    private long handle;
    private native void create(BusAttachment bus, String[] interfaceNames);
    private synchronized native void destroy();
    private native void triggerEnablePendingListeners();

    private static class ObjectId implements Comparable<ObjectId> {
        public String busname;
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

    private static class WrappedListener {
        public boolean enabled;
        public Listener listener;
        WrappedListener(Listener l, boolean enable) {
            enabled = enable;
            listener = l;
        }
    };

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

    private static String getBusInterfaceName(Class<?> intf) {
         BusInterface annotation = intf.getAnnotation(BusInterface.class);
         if (annotation != null && !annotation.name().equals("")) {
             return annotation.name();
         }
         return intf.getCanonicalName();
    }

    private BusAttachment bus;
    private NavigableMap<ObjectId, ProxyBusObject> proxies;
    private List<WrappedListener> listeners;
    private HashMap<String, Class<?>> interfaceMap;
}
