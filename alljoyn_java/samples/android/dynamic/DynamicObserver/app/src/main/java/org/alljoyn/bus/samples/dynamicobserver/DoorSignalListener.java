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

package org.alljoyn.bus.samples.dynamicobserver;

import org.alljoyn.bus.AbstractDynamicBusObject;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.MessageContext;
import org.alljoyn.bus.defs.BusObjectInfo;

import java.util.Arrays;

/**
 * The DoorSignalListener (consumer-side DynamicBusObject) handles receipt of bus signals from
 * a remote Door interface provider.
 */
public class DoorSignalListener extends AbstractDynamicBusObject {

    private final BusHandler mBushandler;

    /**
     * Create door listener used to handle received bus signals.
     *
     * @param bus the bus attachment with which this listener is registered.
     * @param objectDef the bus object's interface(s) definition.
     * @param handler used to relay notifications regarding received signals.
     */
    public DoorSignalListener(BusAttachment bus, BusObjectInfo objectDef, BusHandler handler) {
        super(bus, objectDef);
        mBushandler = handler;
    }

    /**
     * Override AbstractDynamicBusObject.signalReceiver to provide a generic callback handler that
     * will receive incoming Door interface bus signals (namely the PersonPassedThrough signal).
     */
    @Override
    public void signalReceived(Object... args) throws BusException {
        MessageContext msgCtx = getBus().getMessageContext();
        String argString = Arrays.toString(args).replaceAll("^.|.$", "");

        String message = String.format("%s(%s) event received from %s", msgCtx.memberName, argString, msgCtx.sender);
        mBushandler.sendUIMessageOnSignal(message);
    }

}
