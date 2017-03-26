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

package org.alljoyn.bus.samples.dynamicsignalclient;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.AbstractDynamicBusObject;
import org.alljoyn.bus.defs.BusObjectInfo;

/**
 * Placeholder dynamic bus object used by this app when emitting signals to the remote object
 * (i.e. being used for sending signals, not for receiving signals). The AbstractDynamicBusObject
 * is subclassed in order to simplify our dynamic bus object implementation.
 * <p>
 * <b>NOTE:</b>
 * <p>
 * This bus object class is used to model the "simple interface", which has no bus methods nor
 * bus properties defined (and is not being used by this app to receive signals), therefore no
 * receive handlers need to be implemented here. Thus an empty class implementation.
 * <p>
 * However, this class is included for demonstrative purposes and to provide further explanation
 * to the reader. In the real world, it would have been more concise to instead instantiate an
 * anonymous AbstractDynamicBusObject instance as follows:
 * <pre><code>
 *     new AbstractDynamicBusObject(mBusAttachment, mBusObjectInfo) {}
 * </code></pre>
 */
public class SignalService extends AbstractDynamicBusObject {

    /**
     * Create a dynamic bus object.
     *
     * @param bus the bus attachment with which this dynamic object is registered.
     * @param objectDef the path and interface(s) definition that describes this service.
     */
    public SignalService(BusAttachment bus, BusObjectInfo objectDef) {
        super(bus, objectDef);
    }

    /**
     * Intentionally commented out (for illustrative purposes). Since this class is only being used
     * by this app for emitting signals, the signalReceived method handler is never called directly.
     *
     * Note that for static bus objects, we would have had to provide an intentionally empty signal
     * handler implementation; i.e. for ping(sting). However, with dynamic bus objects, this is not
     * necessary because AbstractDynamicBusObject's default implementation of signalReceived is
     * already a no-op.
     */
  // public void signalReceived(Object... args) throws BusException {
  // }

}