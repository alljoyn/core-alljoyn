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

package org.alljoyn.bus.samples.observer.provider;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.PropertyChangedEmitter;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.SignalEmitter.GlobalBroadcast;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.samples.observer.BusHandler;
import org.alljoyn.bus.samples.observer.Door;

/**
 * A Class implementing a basic door provider service.
 */
public class DoorService implements Door, BusObject {
    private boolean mIsOpen;
    private final String mLocation;
    private int mKeyCode;
    private final PropertyChangedEmitter pce;
    private final BusHandler mBushandler;

    public DoorService(String location, BusHandler handler) {
        mLocation = location;
        mBushandler = handler;
        pce = new PropertyChangedEmitter(DoorService.this,
                BusAttachment.SESSION_ID_ALL_HOSTED, GlobalBroadcast.Off);
    }

    /**
     * Implements the read of the IsOpen property of the door interface.
     *
     * @return the current state of the door. True if the door is open.
     */
    @Override
    public boolean getIsOpen() {
        sendUiMessage("Property IsOpen is queried");
        return mIsOpen;
    }


    /**
     * Implements the read of the Location property of the door interface.
     *
     * @return the current location of the door.
     */
    @Override
    public String getLocation() {
        sendUiMessage("Property Location is queried");
        return mLocation;
    }

    /**
     * Implements the read of the KeyCode property of the door interface.
     *
     * @return the current key code for this door.
     */
    @Override
    public int getKeyCode() {
        sendUiMessage("Property IsOpen is queried");
        return mKeyCode;
    }

    /**
     * Opens the door.
     */
    @Override
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
     * Internal helper to. Send a property changed event to all connected
     * listeners.
     *
     * @param b
     */
    void sendDoorEvent(boolean b) {
        try {
            pce.PropertyChanged(DOOR_INTF_NAME, "IsOpen", new Variant(b));
        } catch (BusException e) {
            e.printStackTrace();
        }
    }

    /**
     * Closes the Door.
     */
    @Override
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

    @Override
    public void knockAndRun() {
        sendUiMessage("Method KnockAndRun is called");
        // Schedule a background task to the knock and run behavior
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
                // Create a signal emitter and send out the personPassedThrough
                // signal
                SignalEmitter se = new SignalEmitter(DoorService.this,
                        BusAttachment.SESSION_ID_ALL_HOSTED,
                        GlobalBroadcast.Off);
                Door door = se.getInterface(Door.class);
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
                    pce.PropertyChanged(DOOR_INTF_NAME, "KeyCode", new Variant(
                            0));
                } catch (BusException e) {
                    e.printStackTrace();
                }
            }
        });
    }

    @Override
    public void personPassedThrough(String person) {
        // personPassedThrough is a signal. No implementation is required.
    }

    private void sendUiMessage(String string) {
        mBushandler.sendUIMessage(string, this);

    }

    private void sendWorkerMessage(Runnable runnable) {
        mBushandler.sendMessage(mBushandler.obtainMessage(
                BusHandler.EXECUTE_TASK,
                runnable));

    }

    /**
     * Internal function to get the location of this door. No event is sent to
     * UI
     *
     * @return Location of the door.
     */
    public String getInternalLocation() {
        return mLocation;
    }
}
