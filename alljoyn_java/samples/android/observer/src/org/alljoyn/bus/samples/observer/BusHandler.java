/*
 * Copyright AllSeen Alliance. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

package org.alljoyn.bus.samples.observer;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;
import java.util.concurrent.ConcurrentHashMap;

import org.alljoyn.bus.AboutObj;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.MessageContext;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.Observer;
import org.alljoyn.bus.PropertiesChangedListener;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.annotation.BusSignalHandler;
import org.alljoyn.bus.samples.observer.provider.DoorService;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

/** This class will handle all AllJoyn calls. */
public class BusHandler extends Handler {
    BusAttachment mBus;
    private Observer mObserver;
    private final Vector<DoorService> doors = new Vector<DoorService>();

    private DoorEventListener dl;
    private final ConcurrentHashMap<DoorAdapterItem, ProxyBusObject> map = new ConcurrentHashMap<DoorAdapterItem, ProxyBusObject>();
    private final DoorAdapter mDoorAdapter;
    private AboutObj mAboutObj;
    private org.alljoyn.bus.samples.observer.MyAboutData mAboutData;

    /*
     * These are the messages sent to the BusHandler from the UI or event
     * callbacks.
     */
    public static final int CONNECT = 1;
    public static final int JOIN_SESSION = 2;
    public static final int DISCONNECT = 3;
    public static final int KNOCK_ON_DOOR = 4;
    public static final int TOGGLE_DOOR = 5;
    public static final int CREATE_DOOR = 6;
    public static final int EXECUTE_TASK = 7;
    public static final int DELETE_DOOR = 8;

    /** The port used to listen on for new incoming messages. */
    private static final short CONTACT_PORT = 2134;

    public BusHandler(DoorAdapter adapter, Looper looper) {
        super(looper);
        mDoorAdapter = adapter;
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
        /*
         * Connect to a remote instance of an object implementing the
         * BasicInterface.
         */
        case CONNECT: {
            Client client = (Client) msg.obj;
            org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(client.getApplicationContext());
            /*
             * All communication through AllJoyn begins with a
             * BusAttachment.
             *
             * A BusAttachment needs a name. The actual name is unimportant
             * except for internal security. As a default we use the class
             * name as the name.
             *
             * By default AllJoyn does not allow communication between
             * devices (i.e. bus to bus communication). The second argument
             * must be set to Receive to allow communication between
             * devices.
             */
            mBus = new BusAttachment(client.getPackageName(), BusAttachment.RemoteMessage.Receive);

            /*
             * To communicate with AllJoyn objects, we must connect the
             * BusAttachment to the bus.
             */
            Status status = mBus.connect();
            if (Status.OK != status) {
                client.finish();
                return;
            }

            /*
             * Create a SessionPortListener, so we provider door locally and
             * have peers set-up a session and used the doors.
             */
            Mutable.ShortValue contactPort = new Mutable.ShortValue(CONTACT_PORT);

            SessionOpts sessionOpts = new SessionOpts();
            sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
            sessionOpts.isMultipoint = false;
            sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
            sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

            mBus.bindSessionPort(contactPort, sessionOpts,
                    new SessionPortListener() {
                @Override
                public boolean acceptSessionJoiner(short sessionPort,
                        String joiner, SessionOpts sessionOpts) {
                    // Allow all connections on our contact port.
                    return sessionPort == CONTACT_PORT;
                }
            });

            /*
             * Create About data and ask the About service to publish this.
             */
            mAboutObj = new AboutObj(mBus);
            mAboutData = new MyAboutData();
            status = mAboutObj.announce(CONTACT_PORT, mAboutData);
            if (status != Status.OK) {
                Log.i(Client.TAG, "Problem while sending about info");
                return;
            }

            /*
             * Register a listener in order to receive signals emmited by doors.
             */
            dl = new DoorEventListener();
            status = mBus.registerSignalHandlers(dl);
            if (status != Status.OK) {
                Log.i(Client.TAG, "Problem while registering signal handler");
                return;
            }

            /*
             * Create an observer and add a listener to it.
             */
            mObserver = new Observer(mBus, new Class[] { Door.class });
            mObserver.registerListener(new Observer.Listener() {
                @Override
                public void objectLost(ProxyBusObject obj) {
                    // A door is no longer available. removed it from the local
                    // list.
                    for (Map.Entry<DoorAdapterItem, ProxyBusObject> entry : map.entrySet()) {
                        if (obj == entry.getValue()) {
                            // Notify the UI.
                            mDoorAdapter.remove(entry.getKey());
                            // Remove from internal map.
                            map.remove(entry.getKey());
                            return;
                        }
                    }
                }

                @Override
                public void objectDiscovered(ProxyBusObject obj) {
                    // A new door is found. Send a message to our worker thread.
                    Message msg = obtainMessage(JOIN_SESSION);
                    msg.obj = obj;
                    sendMessage(msg);
                }
            });
            break;
        }
        case (JOIN_SESSION): {
            /*
             * We have discovered a new door. Add it to our list and register
             * event listeners on it.
             */
            ProxyBusObject obj = (ProxyBusObject) msg.obj;
            obj.enablePropertyCaching();
            Door door = obj.getInterface(Door.class);
            try {
                String displayName = door.getLocation() + " (" + obj.getBusName() + obj.getObjPath() + ")";
                DoorAdapterItem item = new DoorAdapterItem(displayName, door.getIsOpen());
                map.put(item, obj);
                obj.registerPropertiesChangedListener(Door.DOOR_INTF_NAME, new String[] { "IsOpen" }, dl);
                mDoorAdapter.add(item);
            } catch (BusException e) {
                e.printStackTrace();
            }
            break;
        }

        /* Release all resources acquired in the connect. */
        case DISCONNECT: {
            mObserver.close();
            for (DoorService door : doors) {
                mBus.unregisterBusObject(door);
            }
            doors.clear();
            mBus.disconnect();
            getLooper().quit();
            break;
        }

        /*
         * Call the service's KnockOnDoor method through the ProxyBusObject.
         */
        case KNOCK_ON_DOOR: {
            try {
                ProxyBusObject obj = map.get(msg.obj);
                if (obj != null) {
                    Door door = obj.getInterface(Door.class);
                    door.knockAndRun();
                }
            } catch (BusException ex) {
                ex.printStackTrace();
            }
            break;
        }

        /*
         * Call Toggle a door.
         */
        case TOGGLE_DOOR: {
            try {
                ProxyBusObject obj = map.get(msg.obj);
                if (obj != null) {
                    Door door = obj.getInterface(Door.class);
                    if (door.getIsOpen()) {
                        door.close();
                    } else {
                        door.open();
                    }
                }
            } catch (BusException ex) {
                ex.printStackTrace();
            }
            break;
        }
        /*
         * Creates a local door
         */
        case CREATE_DOOR: {
            String location = (String) msg.obj;
            DoorService newDoor = new DoorService(location, this);
            if (!location.startsWith("/")) {
                location = "/" + location;
            }
            // Register the door on the local bus.
            if (Status.OK != mBus.registerBusObject(newDoor, location)) {
                mDoorAdapter.sendDoorEvent("Failed to create door '" + location + "'");
                break;
            }
            doors.add(newDoor);
            // Have About send a signal a new door is available.
            mAboutObj.announce(CONTACT_PORT, mAboutData);
            break;
        }
        case DELETE_DOOR: {
            @SuppressWarnings("unchecked")
            ArrayList<String> locations = (ArrayList<String>) msg.obj;

            Iterator<DoorService> it = doors.iterator();
            while (it.hasNext()) {
                DoorService door = it.next();
                if (locations.remove(door.getInternalLocation())) {
                    it.remove();
                    // unregister the door from the bus.
                    mBus.unregisterBusObject(door);
                    if (locations.size() == 0) {
                        // All doors are removed. Have About send a signal some
                        // doors no longer are available.
                        mAboutObj.announce(CONTACT_PORT, mAboutData);
                        break;
                    }
                }
            }
            break;
        }
        /*
         * Executes a local task.
         */
        case EXECUTE_TASK: {
            Runnable task = (Runnable) msg.obj;
            task.run();
        }
        default:
            break;
        }
    }

    public void sendUIMessage(String string, DoorService doorService) {
        mDoorAdapter.sendDoorEvent(doorService.getInternalLocation() + ": " + string);
    }

    /**
     * The DoorEventListener listens to both propertyChanged events and signals.
     */
    class DoorEventListener extends PropertiesChangedListener {
        /*
         * Called when a personPassedThrough signal is received.
         */
        @BusSignalHandler(iface = "org.alljoyn.bus.samples.observer.Door", signal = Door.PERSON_PASSED_THROUGH)
        public void personPassedThrough(String person) {
            MessageContext msgCtx = mBus.getMessageContext();
            mDoorAdapter.sendSignal("personPassedThrough(" + person + ") event received from " + msgCtx.sender);
        }

        /*
         * handle incoming property state changes.
         */
        @Override
        public void propertiesChanged(ProxyBusObject pObj, String ifaceName,
                                      Map<String, Variant> changed, String[] invalidated) {

            Variant v = changed.get("IsOpen");
            if (v != null) {
                try {
                    Boolean value = v.getObject(Boolean.class);
                    for (Map.Entry<DoorAdapterItem, ProxyBusObject> entry : map.entrySet()) {
                        if (pObj == entry.getValue()) {
                            DoorAdapterItem item = entry.getKey();
                            item.setIsOpen(value);
                            mDoorAdapter.propertyUpdate(item);
                        }
                    }
                } catch (BusException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    /**
     * Provides the list of local provided doornames
     *
     * @return a non-null possible empty list.
     */
    public String[] getDoorNames() {
        DoorService[] ds = doors.toArray(new DoorService[doors.size()]);
        String[] names = new String[ds.length];
        for (int i = 0; i < ds.length; i++) {
            names[i] = ds[i].getInternalLocation();
        }
        return names;
    }
}

