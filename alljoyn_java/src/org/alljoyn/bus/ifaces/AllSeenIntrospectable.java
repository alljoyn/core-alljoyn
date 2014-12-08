/*
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
 * The org.allseen.Introspectable interface provides introspection xml
 * that contains description elements that document the object and it's various sub-elements.
 */
@BusInterface(name = "org.allseen.Introspectable")
public interface AllSeenIntrospectable {

   /**
     * Gets the description languages that are supported for the object.
     *
     * @return array of supported language names.
     * @throws BusException indicating failure when calling GetDescriptionLanguages method
     */
    @BusMethod
    String[] GetDescriptionLanguages() throws BusException;

    /**
     * Gets the introspect with description for the object.
     * @param language to return in the description.
     * @return introspect with description.
     * @throws BusException indicating failure when calling IntrospectWithDescription method
     */
    @BusMethod
    String IntrospectWithDescription(String language) throws BusException;
}
