/******************************************************************************
 * Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/

package org.alljoyn.bus;

import org.alljoyn.bus.annotation.BusAnnotation;
import org.alljoyn.bus.annotation.BusAnnotations;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusSignal;
import org.alljoyn.bus.annotation.BusProperty;

@BusInterface(name="org.alljoyn.bus.InterfaceWithAnnotations")
@BusAnnotations({@BusAnnotation(name="org.freedesktop.DBus.Deprecated", value="true")})
public interface InterfaceWithAnnotations {

    @BusMethod(name="Ping", signature="s", replySignature="s")
    @BusAnnotations({@BusAnnotation(name="name", value="value"), @BusAnnotation(name="name2", value="value2")})
    public String ping(String inStr) throws BusException;

    @BusMethod(name="Pong", signature="s", replySignature="s")
    public String pong(String inStr) throws BusException;


    @BusMethod(name="Pong2", signature="s")
    @BusAnnotations({@BusAnnotation(name="org.freedesktop.DBus.Deprecated", value="true"),
        @BusAnnotation(name="org.freedesktop.DBus.Method.NoReply", value="true")})
    public void pong2(String inStr) throws BusException;


    @BusSignal(name="Signal")
    @BusAnnotations({@BusAnnotation(name="org.freedesktop.DBus.Deprecated", value="true")})
    public void signal() throws BusException;


    @BusProperty
    @BusAnnotations({@BusAnnotation(name="org.freedesktop.DBus.Property.EmitsChangedSignal", value="true")})
    public String getProp1() throws BusException;

    @BusProperty
    @BusAnnotations({@BusAnnotation(name="org.freedesktop.DBus.Property.EmitsChangedSignal", value="true")})
    public void setProp1(String s) throws BusException;


    @BusProperty
    @BusAnnotations({@BusAnnotation(name="org.freedesktop.DBus.Property.EmitsChangedSignal", value="invalidates")})
    public String getProp2() throws BusException;

    @BusProperty
    @BusAnnotations({@BusAnnotation(name="org.freedesktop.DBus.Property.EmitsChangedSignal", value="invalidates")})
    public void setProp2(String s) throws BusException;
}
