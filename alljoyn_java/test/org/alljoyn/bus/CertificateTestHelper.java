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

package org.alljoyn.bus;

import java.nio.ByteBuffer;
import java.util.UUID;

import org.alljoyn.bus.common.ECCPublicKey;
import org.alljoyn.bus.common.CryptoECC;
import org.alljoyn.bus.common.KeyInfoNISTP256;
import org.alljoyn.bus.common.CertificateX509;

public class CertificateTestHelper {
    public static CertificateX509[] createIdentityCert(
            String serial,
            UUID managerGuid,
            ECCPublicKey pubKey,
            String alias,
            long expiration,
            BusAttachment bus) throws BusException {

        PermissionConfigurator pc = bus.getPermissionConfigurator();
        CertificateX509 identityCertificate[] = new CertificateX509[1];
        identityCertificate[0] = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        identityCertificate[0].setSerial(serial.getBytes());
        ByteBuffer bbIssuer = ByteBuffer.wrap(new byte[16]);
        bbIssuer.putLong(managerGuid.getMostSignificantBits());
        bbIssuer.putLong(managerGuid.getLeastSignificantBits());
        identityCertificate[0].setIssuerCN(bbIssuer.array());
        identityCertificate[0].setSubjectCN(bus.getUniqueName().getBytes());
        identityCertificate[0].setSubjectPublicKey(pubKey);
        identityCertificate[0].setSubjectAltName(alias.getBytes());

        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + expiration;
        identityCertificate[0].setValidity(validFrom, validTo);

        pc.signCertificate(identityCertificate[0]);
        identityCertificate[0].verify(pc.getSigningPublicKey().getPublicKey());
        return identityCertificate;
    }

    public static CertificateX509[] createMembershipCert(
            String serial,
            UUID guild,
            boolean delegate,
            ECCPublicKey pubKey,
            long expiration,
            BusAttachment bus) throws BusException {

        PermissionConfigurator pc = bus.getPermissionConfigurator();
        CertificateX509 memberCert[] = new CertificateX509[1];
        memberCert[0] = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);
        memberCert[0].setSerial(serial.getBytes());
        ByteBuffer bbIssuer = ByteBuffer.wrap(new byte[16]);
        bbIssuer.putLong(guild.getMostSignificantBits());
        bbIssuer.putLong(guild.getLeastSignificantBits());
        memberCert[0].setIssuerCN(bbIssuer.array());
        memberCert[0].setSubjectCN(bus.getUniqueName().getBytes());
        memberCert[0].setSubjectPublicKey(pubKey);
        memberCert[0].setSubjectAltName(bbIssuer.array());
        memberCert[0].setCA(delegate);

        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + expiration;
        memberCert[0].setValidity(validFrom, validTo);

        pc.signCertificate(memberCert[0]);
        memberCert[0].verify(pc.getSigningPublicKey().getPublicKey());
        return memberCert;
    }
}