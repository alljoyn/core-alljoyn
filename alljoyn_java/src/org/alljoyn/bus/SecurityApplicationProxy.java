/*
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
 */

package org.alljoyn.bus;

import org.alljoyn.bus.ifaces.SecurityApplication;
import org.alljoyn.bus.common.ECCPublicKey;

public class SecurityApplicationProxy {

    /**
     * SecurityApplicationProxy constructor
     *
     * @param bus the BusAttachment that owns this ProxyBusObject
     * @param busName the unique or well-known name of the remote AllJoyn BusAttachment
     * @param sessionId the session ID this ProxyBusObject will use
     */
    public SecurityApplicationProxy(BusAttachment bus, String busName, int sessionId) {
        ProxyBusObject securityProxy = bus.getProxyBusObject(busName, SecurityApplication.OBJ_PATH, sessionId, new Class<?>[] {SecurityApplication.class});
        proxy = securityProxy.getInterface(SecurityApplication.class);
    }

    /**
     * Get the version of the remote SecurityApplication interface
     * @return the version of the remote SecurityApplication interface
     * @throws BusException
     *   throws an exception indicating failure to make the remote method call
     */
    public short getVersion() throws BusException {
        return proxy.getVersion();
    }

    /**
     * @return the Application State
     * @throws BusException indicating failure to read ApplicationState property
     */
    public SecurityApplication.ApplicationState getApplicationState() throws BusException {
        return proxy.getApplicationState();
    }

    /**
     * @return the ManifestTemplateDigest
     * @throws BusException indicating failure to read ManifestTemplateDigest property
     */
    public ManifestTemplateDigest getManifestTemplateDigest() throws BusException {
        return proxy.getManifestTemplateDigest();
    }

    /**
     * @return the EccPublicKey
     * @throws BusException indicating failure to read EccPublicKey property
     */
    public ECCPublicKey getEccPublicKey() throws BusException {
        return proxy.getEccPublicKey();
    }

    /**
     * @return the ManufacturerCertificate
     * @throws BusException indicating failure to read ManufacturerCertificate property
     */
    public ManufacturerCertificate[] getManufacturerCertificate() throws BusException {
        return proxy.getManufacturerCertificate();
    }

    /**
     * @return the ManifestTemplate
     * @throws BusException indicating failure to read ManifestTemplate property
     */
    public ManifestTemplateRule[] getManifestTemplate() throws BusException {
        return proxy.getManifestTemplate();
    }

    /**
     * @return the ClaimCapabilities
     * @throws BusException indicating failure to read ClaimCapabilities property
     */
    public short getClaimCapabilities() throws BusException {
        return proxy.getClaimCapabilities();
    }

    /**
     * @return the ClaimCapabilitiesAdditionalInfo
     * @throws BusException indicating failure to read ClaimCapabilitiesAdditionalInfo property
     */
    public short getClaimCapabilityAdditionalInfo() throws BusException {
        return proxy.getClaimCapabilityAdditionalInfo();
    }

    SecurityApplication proxy;
}
