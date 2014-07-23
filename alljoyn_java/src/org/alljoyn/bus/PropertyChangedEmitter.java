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

package org.alljoyn.bus;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.ifaces.Properties;

import java.util.HashMap;


/**
 * A helper proxy used by BusObjects to send property change notifications.  A PropertyChangedlEmitter
 * instance can be used to send any signal from a given AllJoyn interface.
 */
public class PropertyChangedEmitter extends SignalEmitter {

    private Properties props;

    /**
     * Constructs a PropertyChangedEmitter.
     *
     * @param source the source object of any signals sent from this emitter
     * @param sessionId A unique SessionId for this AllJoyn session instance
     */
    public PropertyChangedEmitter(BusObject source, int sessionId) {
        super(source, null, sessionId, GlobalBroadcast.On);
        props = getInterface(Properties.class);
    }

    /**
     * Constructs a PropertyChangedEmitter used for broadcasting.
     *
     * @param source the source object of any signals sent from this emitter
     */
    public PropertyChangedEmitter(BusObject source) {
        this(source, BusAttachment.SESSION_ID_ANY);
    }

    /** Sends the signal. */
    public void PropertyChanged(String ifaceName, final String propertyName, final Variant newValue) throws BusException {
        if (newValue == null) {
            props.PropertiesChanged(ifaceName, null, new String [] { propertyName });
        } else {
            props.PropertiesChanged(ifaceName, new HashMap<String, Variant>() {{ put(propertyName, newValue); }}, null);
        }
    }
}
