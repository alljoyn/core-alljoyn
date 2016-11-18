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

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusProperty;
import org.alljoyn.bus.annotation.Secure;

/**
 * The Door interface describing how a door looks on the AllJoyn bus.
 */
@BusInterface(name = Door.DOOR_INTF_NAME, announced = "true")
@Secure
public interface Door {

    String DOOR_INTF_NAME = "sample.securitymgr.door.Door";
    String PERSON_PASSED_THROUGH = "PersonPassedThrough";

    @BusProperty(annotation = BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL)
    String getLocation() throws BusException;

    @BusProperty(annotation = BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL)
    boolean getState() throws BusException;

    @BusProperty(annotation = BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL_INVALIDATES, signature = "u")
    int getKeyCode() throws BusException;

    @BusMethod(name = "Open")
    boolean open() throws BusException;

    @BusMethod(name = "Close")
    boolean close() throws BusException;

    @BusMethod(name = "GetState")
    boolean getStateM() throws BusException;

    @BusMethod(name = "KnockAndRun", annotation = BusMethod.ANNOTATE_NO_REPLY)
    void knockAndRun() throws BusException;
}
