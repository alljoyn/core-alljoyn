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

package org.alljoyn.bus.samples.securedoorservice;

import android.os.Handler;
import android.os.Message;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;

public class DoorImpl implements Door, BusObject {
    private String location;
    private int keyCode = 0;
    boolean isOpen;

    private Handler msgHandler;

    public DoorImpl(String locate, Handler handler) {
        location = locate;
        isOpen = false;
        msgHandler = handler;
    }

    public boolean getState() throws BusException {
        return isOpen;
    }

    public boolean getStateM() throws BusException {
        return isOpen;
    }

    public String getLocation() throws BusException {
        return location;
    }

    public int getKeyCode() throws BusException {
        return keyCode;
    }

    public boolean open() throws BusException {
        displayMessage(location + " door opened");
        isOpen = true;
        return isOpen;
    }

    public boolean close() throws BusException {
        displayMessage(location + " door closed");
        isOpen = false;
        return isOpen;
    }

    public void knockAndRun() throws BusException {
        displayMessage("knocked " + location);
    }

    private void displayMessage(String msg) {
        Message entry = msgHandler.obtainMessage(Service.MESSAGE_ADD_LIST_ENTRY, msg);
        msgHandler.sendMessage(entry);
    }
}
