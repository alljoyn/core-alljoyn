/**
 * This is an Android sample on how to use the AllJoyn properties.
 * This is the Bus Interface description.  It contains two set properties.
 * and two get properties.
 *
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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
package org.alljoyn.bus.samples.properties_client;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusProperty;

/*
 * BusInterface with the well-known name org.alljoyn.bus.samples.properties.
 */
@BusInterface (name = "org.alljoyn.bus.samples.properties")
public interface PropertiesInterface {
    /*
     * The BusProperty annotation signifies that this function should be used as part of the AllJoyn
     * interface. A BusProperty is always a method that starts with get or set.  The set method
     * always takes a single value.  while the get method always returns a single value. The single
     * value can be a complex data type such as an array or an Object.
     *
     * All methods that use the BusProperty annotation can throw a BusException and should indicate
     * this fact.
     */
    @BusProperty
    public String getBackGroundColor()throws BusException;

    @BusProperty
    public void setBackGroundColor(String color) throws BusException;

    @BusProperty
    public int getTextSize() throws BusException;

    @BusProperty
    public void setTextSize(int size) throws BusException;
}
