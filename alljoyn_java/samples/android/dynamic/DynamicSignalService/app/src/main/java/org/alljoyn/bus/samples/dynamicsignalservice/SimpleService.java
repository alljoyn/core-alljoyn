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

package org.alljoyn.bus.samples.dynamicsignalservice;

import android.os.Handler;
import android.util.Log;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.AbstractDynamicBusObject;
import org.alljoyn.bus.MessageContext;
import org.alljoyn.bus.defs.BusObjectInfo;
import org.alljoyn.bus.defs.SignalDef;

import java.util.Arrays;

/**
 * The class that is our AllJoyn service. It implements the "simple interface" via
 * a DynamicBusObject.
 *
 * The AbstractDynamicBusObject is subclassed in order to simplify our dynamic bus
 * object implementation, needing only to override its default signalReceived method
 * in order to fulfill the "simple interface".
 */
public class SimpleService extends AbstractDynamicBusObject {

    private Handler mUiHandler;

    /**
     * Create a dynamic bus object.
     *
     * @param bus the bus attachment with which this dynamic object is registered.
     * @param objectDef the path and interface(s) definition that describes this service.
     * @param uiHandler used to send messages to the UI thread.
     */
    public SimpleService(BusAttachment bus, BusObjectInfo objectDef, Handler uiHandler) {
        super(bus, objectDef);
        mUiHandler = uiHandler;
    }

    /**
     * Overrides AbstractDynamicBusObject's default signalReceived method to provide a public
     * signal handler implementation for any signal defined by this service; i.e. Ping(string).
     *
     * It prints out the string argument it receives by sending it to the UI handler.
     */
    @Override
    public void signalReceived(Object... args) throws BusException {
        MessageContext ctx = getBus().getMessageContext();

        /*
         * You can retrieve this signal's full definition (if needed) via the BusObjectInfo interfaces
         * provided at construction time. However, there is no need to verify that the received signal
         * is valid, since ONLY signals defined by our interface can be received.
         */
        SignalDef signalDef = BusObjectInfo.getSignal(getInterfaces(), ctx.interfaceName, ctx.memberName, ctx.signature);

        if ((signalDef != null) && signalDef.getName().equals(SimpleInterface.SIGNAL_PING) && (args.length > 0)) {
            /* Send a message to the UI thread. */
            mUiHandler.sendMessage(mUiHandler.obtainMessage(Service.MESSAGE_PING, (String)args[0]));
        } else {
            /*
             * It should not be possible to reach this point. Only defined signals will ever be received,
             * and the AllJoyn framework should have already validated that the signal's arg signature
             * matches the definition published on the BusAttachment.
             */
            Log.w("SimpleService", String.format("Unsupported signal: %s:%s(%s) - %s",
                    ctx.interfaceName, ctx.memberName, ctx.signature, Arrays.toString(args)));
        }
    }
}