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

import org.alljoyn.bus.defs.ArgDef;
import org.alljoyn.bus.defs.BusObjectInfo;
import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.SignalDef;

import java.util.ArrayList;
import java.util.List;

/**
 * Convenience class used to retrieve the dynamic definition of the Simple Interface,
 * describing how the interface looks on the AllJoyn bus.
 */
public class SimpleInterface {

    public static final String INTERFACE  = "org.alljoyn.bus.samples.simple.SimpleInterface";
    public static final String SIGNAL_PING = "Ping";

    /**
     * Build an interface definition named "...SimpleInterface" that contains the signal Ping(string).
     *
     * Note: Real-world applications would likely determine the interface(s) definition dynamically
     * (e.g. learned via introspection or some other discovery mechanism).
     *
     * @param path The bus object path used in the object definition.
     * @return BusObjectInfo An object definition containing the given path and its
     *         list of dynamic interface definitions.
     */
    public static BusObjectInfo getBusObjectInfo(String path) {
        InterfaceDef interfaceDef = new InterfaceDef(INTERFACE);

        /* Create the Ping signal definition. In the "real" world these details may be discovered. */
        SignalDef signalDef = new SignalDef(SIGNAL_PING, "s", INTERFACE);
        signalDef.addArg( new ArgDef("string", "s", ArgDef.DIRECTION_OUT) );
        signalDef.setDescription("Ping sent to remote object", "en");
        interfaceDef.addSignal(signalDef);

        /* Return the bus object info, which has an object path and interface definitions. */
        List<InterfaceDef> ifaces = new ArrayList<InterfaceDef>();
        ifaces.add(interfaceDef);
        return new BusObjectInfo(path, ifaces);
    }
}
