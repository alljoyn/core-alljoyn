/*
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
package org.alljoyn.bus.common;

import org.alljoyn.bus.BusException;
import java.util.UUID;

public class CryptoECC {

    /**
     * The NIST recommended elliptic curve P-256
     */
    public static final byte ECC_NIST_P256 = 0;

    /**
     * Allocate native resources.
     */
    public native void create() throws BusException;

    /** Release native resources. */
    private synchronized native void destroy();

    /**
     * Default Constructor
     */
    public CryptoECC() throws BusException {
        create();
    }

    /**
     * Let the Java garbage collector release resources.
     */
    @Override
    protected void finalize() throws Throwable {
        destroy();
        super.finalize();
    }

    /**
     * Generates the Ephemeral Diffie-Hellman key pair.
     *
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    public native void generateDHKeyPair() throws BusException;

    /**
     * Generates the ephemeral key pair for EC-SPEKE. The key pair
     * can then be used with the other DH APIs to compute the shared secret.
     * @param pw          Password and additional data to use during key generation.
     * @param pwLen       The byte length of pw.
     * @param clientGUID  The client's GUID
     * @param serviceGUID The service's GUID
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    private native void generateSPEKEKeyPair(byte[] pw, long pwLen, UUID clientGUID, UUID serviceGUID) throws BusException;

    /**
     * Generates the ephemeral key pair for EC-SPEKE. The key pair
     * can then be used with the other DH APIs to compute the shared secret.
     * @param pw          Password and additional data to use during key generation.
     * @param clientGUID  The client's GUID
     * @param serviceGUID The service's GUID
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    public void generateSPEKEKeyPair(byte[] pw, UUID clientGUID, UUID serviceGUID) throws BusException {
        generateSPEKEKeyPair(pw, pw.length, clientGUID, serviceGUID);
    }

    /**
     * Generates the Diffie-Hellman shared secret.
     * @param   peerPublicKey the peer's public key
     * @param   secret the output shared secret
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    public native void generateSharedSecret(ECCPublicKey peerPublicKey, ECCSecret secret) throws BusException;

    /**
     * Retrieve the DH public key
     * @return  the DH public key.  It's a pointer to an internal buffer. Its lifetime is the same as the object's lifetime.
     * @throws BusException
     */
    public native ECCPublicKey getDHPublicKey() throws BusException;

    /**
     * Assign the DH public key
     * @param pubKey the public key to copy
     * @throws BusException
     */
    public native void setDHPublicKey(ECCPublicKey pubKey) throws BusException;

    /**
     * Retrieve the DH private key
     * @return  the DH private key.  Same lifetime as the object.
     * @throws BusException
     */
    public native ECCPrivateKey getDHPrivateKey() throws BusException;

    /**
     * Assign the DH private key
     * @param privateKey the private key to copy
     * @throws BusException
     */
    public native void setDHPrivateKey(ECCPrivateKey privateKey) throws BusException;

    /**
     * Retrieve the DSA public key
     * @return  the DSA public key.  Same lifetime as the object.
     * @throws BusException
     */
    public native ECCPublicKey getDSAPublicKey() throws BusException;

    /**
     * Assign the DSA public key
     * @param pubKey the public key to copy
     * @throws BusException
     */
    public native void setDSAPublicKey(ECCPublicKey pubKey) throws BusException;

    /**
     * Retrieve the DSA private key
     * @return  the DSA private key.  Same lifetime as the object.
     * @throws BusException
     */
    public native ECCPrivateKey getDSAPrivateKey() throws BusException;

    /**
     * Assign the DSA private key
     * @param privateKey the private key to copy
     * @throws BusException
     */
    public native void setDSAPrivateKey(ECCPrivateKey privateKey) throws BusException;

    /**
     * Generates the DSA key pair.
     *
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    public native void generateDSAKeyPair() throws BusException;

    /**
     * Sign a digest using the DSA key
     * @param digest The digest to sign
     * @param len The digest len
     * @return signature
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    private native ECCSignature DSASignDigest(byte[] digest, int len) throws BusException;

    /**
     * Sign a digest using the DSA key
     * @param digest The digest to sign
     * @return signature
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    public ECCSignature DSASignDigest(byte[] digest) throws BusException {
        return DSASignDigest(digest, digest.length);
    }

    /**
     * Sign a buffer using the DSA key
     * @param buf The buffer to sign
     * @param len The buffer len
     * @return signature
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    private native ECCSignature DSASign(byte[] buf, int len) throws BusException;

    /**
     * Sign a buffer using the DSA key
     * @param buf The buffer to sign
     * @return signature
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    public ECCSignature DSASign(byte[] buf) throws BusException {
        return DSASign(buf, buf.length);
    }

    /**
     * Verify DSA signature of a digest
     * @param digest The digest to sign
     * @param len The digest len
     * @param sig The signature
     * @throws BusException
     *          - ER_FAIL otherwise
     *          - Other error status.
     */
    private native void DSAVerifyDigest(byte[] digest, int len, ECCSignature sig) throws BusException;

    /**
     * Verify DSA signature of a digest
     * @param digest The digest to sign
     * @param sig The signature
     * @throws BusException
     *          - ER_FAIL otherwise
     *          - Other error status.
     */
    public void DSAVerifyDigest(byte[] digest, ECCSignature sig) throws BusException {
        DSAVerifyDigest(digest, digest.length, sig);
    }

    /**
     * Verify DSA signature of a buffer
     * @param buf The buffer to sign
     * @param len The buffer len
     * @param sig The signature
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    private native void DSAVerify(byte[] buf, int len, ECCSignature sig) throws BusException;

    /**
     * Verify DSA signature of a buffer
     * @param buf The buffer to sign
     * @param sig The signature
     * @throws BusException
     *      ER_FAIL otherwise
     *      Other error status.
     */
    public void DSAVerify(byte[] buf, ECCSignature sig) throws BusException {
        DSAVerify(buf, buf.length, sig);
    }

    /**
     * Retrieve the ECC curve type.
     * @return the ECC curve type
     */
    public byte getCurveType()
    {
        return ECC_NIST_P256;
    }

    /** The native connection handle. */
    private long handle;
}