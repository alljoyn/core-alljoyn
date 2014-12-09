/*
 * Copyright (c) 2009-2011,2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.ifaces;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;

/**
 * The standard org.freedesktop.DBus.Peer interface.
 */
@BusInterface(name = "org.freedesktop.DBus.Peer")
public interface Peer {

    /**
     * Send a ping message to a remote connection and get a method reply in
     * response.
     *
     * @throws BusException indicating failure to make Ping method call
     */
    @BusMethod
    void Ping() throws BusException;

    /**
     * Get the DBus machine id.
     * The machine id corresponds to the AllJoyn router's guid in this
     * implementation.
     *
     * @return guid of local AllJoyn router
     * @throws BusException indicating failure to make GetMachineId method call
     */
    @BusMethod
    String GetMachineId() throws BusException;
}
