/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
package org.alljoyn.bus;

import junit.framework.TestCase;
import org.alljoyn.bus.common.CryptoECC;
import org.alljoyn.bus.common.ECCPublicKey;
import org.alljoyn.bus.common.ECCSecret;
import org.alljoyn.bus.common.ECCSignature;
import org.alljoyn.bus.common.ECCPrivateKey;
import java.util.UUID;

public class CryptoECCTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private CryptoECC cryptoECC;

    public void setUp() throws Exception {
        cryptoECC = new CryptoECC();
    }

    public void testBasic() throws Exception {
        cryptoECC.generateDSAKeyPair();
        ECCPublicKey eccPublicKey = cryptoECC.getDSAPublicKey();
        System.out.println(eccPublicKey.toString());

        ECCPrivateKey eccPrivateKey = cryptoECC.getDSAPrivateKey();
        System.out.println(eccPrivateKey.toString());

        cryptoECC.generateDHKeyPair();
        eccPublicKey = cryptoECC.getDHPublicKey();
        System.out.println(eccPublicKey.toString());

        eccPrivateKey = cryptoECC.getDHPrivateKey();
        System.out.println(eccPrivateKey.toString());

        byte[] password = {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};

        cryptoECC.generateSPEKEKeyPair(password, UUID.randomUUID(), UUID.randomUUID());

        cryptoECC.generateSharedSecret(eccPublicKey, new ECCSecret());

        cryptoECC.setDHPublicKey(eccPublicKey);
        cryptoECC.setDSAPublicKey(eccPublicKey);
        cryptoECC.setDSAPrivateKey(eccPrivateKey);
        cryptoECC.setDHPrivateKey(eccPrivateKey);

        ECCSignature eccsign = cryptoECC.DSASignDigest(eccPublicKey.getX());
        cryptoECC.DSAVerifyDigest(eccPublicKey.getX(), eccsign);

        eccsign = cryptoECC.DSASign(eccPublicKey.getX());
        cryptoECC.DSAVerify(eccPublicKey.getX(), eccsign);

        ECCSecret sec = new ECCSecret();
        cryptoECC.generateSharedSecret(eccPublicKey, sec);
        byte[] secretmaster = sec.derivePreMasterSecret();
    }
}
