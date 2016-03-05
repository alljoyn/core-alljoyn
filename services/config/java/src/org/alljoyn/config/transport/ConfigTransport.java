/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

/**
 * 
 */
package org.alljoyn.config.transport;

import java.util.Map;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.annotation.BusAnnotation;
import org.alljoyn.bus.annotation.BusAnnotations;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusProperty;
import org.alljoyn.bus.annotation.Secure;

/**
 * Definition of the Config BusInterface
 */
@BusInterface(name = ConfigTransport.INTERFACE_NAME, announced = "true")
@Secure
public interface ConfigTransport extends BusObject {
    public static final String INTERFACE_NAME = "org.alljoyn.Config";
    public final static String OBJ_PATH = "/Config";

    /**
     * Get the writable properties of the device.
     * 
     * @param languageTag
     *            IETF language tags specified by RFC 5646
     * @return Return all the updatable configuration fields based on the
     *         language tag. See The list of known configuration fields for more
     *         details. If language tag is not specified (i.e. ""),
     *         configuration fields based on device's language preference are
     *         returned.
     * @throws BusException
     */
    @BusMethod(signature = "s", replySignature = "a{sv}")
    public Map<String, Variant> GetConfigurations(String languageTag) throws BusException;

    /**
     * Update the configuration data with the given map. Only the fields listed
     * in the map will be updated.
     * 
     * @param configuration
     * @throws BusException
     */
    @BusMethod(signature = "sa{sv}")
    public void UpdateConfigurations(String languageTag, Map<String, Variant> configuration) throws BusException;

    @BusMethod(signature = "sas")
    public void ResetConfigurations(String language, String[] fieldsToRemove) throws BusException;

    /**
     * @return Interface version
     * @throws BusException
     */
    @BusProperty(signature = "q")
    public short getVersion() throws BusException;

    /**
     * Change the pass phrase. If this feature is not allowed by the OEM, the
     * AllJoyn error code org.alljoyn.Error.FeatureNotAvailable will be
     * returned.
     * 
     * @param daemonRealm
     * @param newPasscode
     * @throws BusException
     */
    @BusMethod(signature = "say")
    public void SetPasscode(String daemonRealm, byte[] newPasscode) throws BusException;

    /**
     * Direct the device to disconnect from the personal AP, clear all
     * previously configured data, and start the soft AP mode. Some device may
     * not support this feature. In such a case, the AllJoyn error code
     * org.alljoyn.Error.FeatureNotAvailable will be returned in the AllJoyn
     * response.
     * 
     * @throws BusException
     */
    @BusMethod
    @BusAnnotations({ @BusAnnotation(name = "org.freedesktop.DBus.Method.NoReply", value = "true") })
    public void FactoryReset() throws BusException;

    /**
     * This method restarts or power cycles the device.
     * 
     * @throws BusException
     */
    @BusMethod
    @BusAnnotations({ @BusAnnotation(name = "org.freedesktop.DBus.Method.NoReply", value = "true") })
    public void Restart() throws BusException;

}
