/**
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

package org.alljoyn.bus.samples.dynamicobserver.provider;

import android.util.Log;

import org.alljoyn.bus.AbstractDynamicBusObject;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.PropertyChangedEmitter;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.SignalEmitter.GlobalBroadcast;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.defs.BusObjectInfo;
import org.alljoyn.bus.samples.dynamicobserver.BusHandler;
import org.alljoyn.bus.samples.dynamicobserver.Door;

import java.lang.reflect.Method;

/**
 * A Class implementing a basic door provider service via a DynamicBusObject.
 */
public class DoorService extends AbstractDynamicBusObject {
    private boolean mIsOpen;
    private final String mLocation;
    private int mKeyCode;
    private final PropertyChangedEmitter pce;
    private final BusHandler mBushandler;

    /**
     * Create a dynamic bus object.
     *
     * @param bus the bus attachment with which this dynamic object is registered.
     * @param objectDef the path and interface(s) definition that describes this service.
     * @param location
     * @param handler
     */
    public DoorService(BusAttachment bus, BusObjectInfo objectDef, String location, BusHandler handler) {
        super(bus, objectDef);
        mLocation = location;
        mBushandler = handler;
        pce = new PropertyChangedEmitter(DoorService.this, BusAttachment.SESSION_ID_ALL_HOSTED, GlobalBroadcast.Off);
    }

    /**
     * Override AbstractDynamicBusObject.getPropertyHandler in order to specify the java Method used
     * to handle each of the Door interface bus property definitions. This is called by the AllJoyn
     * framework, when the bus object is registered with the bus attachment, in order for the
     * AllJoyn JNI layer to map each Door interface bus property definition to your specific java
     * Method callback handler.
     *
     * NOTE: An alternative is to instead override AbstractDynamicBusObject.getPropertyReceived and
     * setPropertyReceived. The default getPropertyHandler implementation maps all bus property definitions
     * to the getPropertyReceived and setPropertyReceived Methods, so you could instead override
     * getPropertyReceived/setPropertyReceived in order to customize them to handle whichever get/set
     * Door interface bus property request is received at runtime. This may be a simpler solution when
     * there are no specific java Method callback handlers implemented for the defined Door interface
     * bus properties. See DoorSignalListener.signalReceived for an example.
     */
    @Override
    public Method[] getPropertyHandler(String interfaceName, String propertyName) {
        try {
            /* Return the get/set handler method corresponding to the given property name. */
            switch (propertyName) {
            case Door.PROPERTY_IS_OPEN:
                return new Method[]{DoorService.class.getMethod("getIsOpen"), null};
            case Door.PROPERTY_LOCATION:
                return new Method[]{DoorService.class.getMethod("getLocation"), null};
            case Door.PROPERTY_KEY_CODE:
                return new Method[]{DoorService.class.getMethod("getKeyCode"), null};
            default:
                Log.w("DoorService", String.format("Unsupported property: %s", propertyName));
            }
        } catch (java.lang.NoSuchMethodException ex) {
            ex.printStackTrace();
        }
        /* Returning null causes BusAttachment.registerBusObject() to fail with status BUS_CANNOT_ADD_HANDLER. */
        return null;
    }

    /**
     * Override AbstractDynamicBusObject.getMethodHandler in order to specify the java Method used to
     * handle each of the Door interface bus method definitions. This is called by the AllJoyn
     * framework, when the bus object is registered with the bus attachment, in order for the
     * AllJoyn JNI layer to map each Door interface bus method definition to your specific java
     * Method callback handler.
     *
     * NOTE: An alternative is to instead override AbstractDynamicBusObject.methodReceived.
     * The default getMethodHandler implementation maps all bus method definitions to the
     * methodReceived Method, so you could instead override methodReceived in order to customize
     * it to handle whichever Door interface bus method request is received at runtime. This may
     * be a simpler solution when there are no specific java Method callback handlers implemented
     * for the defined Door interface bus methods. See DoorSignalListener.signalReceived for an example.
     */
    @Override
    public Method getMethodHandler(String interfaceName, String methodName, String signature) {
        try {
            /* Return the handler method corresponding to the given method name/signature. */
            switch (methodName) {
            case Door.METHOD_OPEN:
                return DoorService.class.getMethod("open");
            case Door.METHOD_CLOSE:
                return DoorService.class.getMethod("close");
            case Door.METHOD_KNOCK_AND_RUN:
                return DoorService.class.getMethod("knockAndRun");
            default:
                Log.w("DoorService", String.format("Unsupported method: %s(%s)", methodName, signature));
            }
        } catch (java.lang.NoSuchMethodException ex) {
            ex.printStackTrace();
        }
        /* Returning null causes BusAttachment.registerBusObject() to fail with status BUS_CANNOT_ADD_HANDLER. */
        return null;
    }

    /**
     * Implements the read of the IsOpen property of the door interface.
     *
     * @return the current state of the door. True if the door is open.
     */
    public boolean getIsOpen() {
        sendUiMessage("Property IsOpen is queried");
        return mIsOpen;
    }

    /**
     * Implements the read of the Location property of the door interface.
     *
     * @return the current location of the door.
     */
    public String getLocation() {
        sendUiMessage("Property Location is queried");
        return mLocation;
    }

    /**
     * Implements the read of the KeyCode property of the door interface.
     *
     * @return the current key code for this door.
     */
    public int getKeyCode() {
        sendUiMessage("Property KeyCode is queried");
        return mKeyCode;
    }

    /**
     * Opens the door.
     */
    public void open() {
        sendUiMessage("Method Open is called");
        if (!mIsOpen) {
            mIsOpen = true;
            // Schedule a task to send out an event to all interested parties.
            sendWorkerMessage(new Runnable() {
                @Override
                public void run() {
                    sendDoorEvent(true);
                }
            });
        }
    }

    /**
     * Closes the Door.
     */
    public void close() {
        sendUiMessage("Method Close is called");
        if (mIsOpen) {
            mIsOpen = false;
            // Schedule a task to send out an event to all interested parties.
            sendWorkerMessage(new Runnable() {
                @Override
                public void run() {
                    sendDoorEvent(false);
                }
            });
        }
    }

    public void knockAndRun() {
        sendUiMessage("Method KnockAndRun is called");
        // Schedule a background task to to the knock and run behavior
        sendWorkerMessage(new Runnable() {
            @Override
            public void run() {
                if (!mIsOpen) {
                    mIsOpen = true;
                    sendDoorEvent(true);
                }
                try {
                    Thread.sleep(250);
                } catch (InterruptedException e) {
                }
                // Create a signal emitter and send out the personPassedThrough signal
                SignalEmitter se =
                     new SignalEmitter(DoorService.this, BusAttachment.SESSION_ID_ALL_HOSTED, GlobalBroadcast.Off);
                Door door = new Door(se); // helper Door proxy to the remote bus object
                try {
                    door.personPassedThrough("owner");
                } catch (BusException e) {
                    e.printStackTrace();
                }
                // Close the door and send an event
                mIsOpen = false;
                sendDoorEvent(false);

                // Invalidate the keyCode
                try {
                    pce.PropertyChanged(Door.INTERFACE, Door.PROPERTY_KEY_CODE, new Variant(0));
                } catch (BusException e) {
                    e.printStackTrace();
                }
            }
        });
    }

    private void sendUiMessage(String string) {
        mBushandler.sendUIMessageOnDoorEvent(string, this);
    }

    private void sendWorkerMessage(Runnable runnable) {
        mBushandler.sendMessage(mBushandler.obtainMessage(BusHandler.EXECUTE_TASK, runnable));
    }

    /**
     * Internal helper to send a property changed event to all connected
     * listeners.
     *
     * @param b
     */
    void sendDoorEvent(boolean b) {
        try {
            pce.PropertyChanged(Door.INTERFACE, Door.PROPERTY_IS_OPEN, new Variant(b));
        } catch (BusException e) {
            e.printStackTrace();
        }
    }

    /**
     * Internal function to get the location of this door. No event is sent to UI.
     *
     * @return Location of the door.
     */
    public String getInternalLocation() {
        return mLocation;
    }
}
