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

package org.alljoyn.bus.samples.securitymanager;

import java.nio.ByteBuffer;
import java.util.UUID;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.CryptoECC;
import org.alljoyn.bus.common.ECCPublicKey;

public class CertificateHelper {
    public static CertificateX509 generateSignedIdentityCertificate(BusAttachment bus, ECCPublicKey subjectPublicKey) throws BusException {
        CertificateX509 retCert = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        retCert.setSubjectAltName(("abcedf1234567").getBytes());
        retCert.setSubjectPublicKey(subjectPublicKey);
        retCert.setCA(false);
        retCert.setSubjectOU("MyTestIdentity".getBytes());
        retCert = setValidity(retCert);
        retCert.setSerial("0".getBytes());
        retCert.setIssuerCN("Issuer".getBytes());
        bus.getPermissionConfigurator().signCertificate(retCert);
        return retCert;
    }

    public static CertificateX509 generateSignedMembershipCertificate(BusAttachment bus, ECCPublicKey subjectPublicKey, UUID groupID) throws BusException{
        CertificateX509 retCert = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        ByteBuffer bb = ByteBuffer.wrap(new byte[16]);
        bb.putLong(groupID.getMostSignificantBits());
        bb.putLong(groupID.getLeastSignificantBits());
        retCert.setSubjectAltName(bb.array());
        retCert.setCA(false);

        byte[] arr = bb.array();
        retCert.setSubjectPublicKey(subjectPublicKey);
        retCert.setSubjectCN("MyTestIdentity".getBytes());

        retCert = setValidity(retCert);
        retCert.setSerial("10".getBytes());
        retCert.setIssuerCN("Issuer".getBytes());
        bus.getPermissionConfigurator().signCertificate(retCert);
        return retCert;
    }

    private static final int TEN_YEARS = 3600 * 24 * 10 * 365;

    private static CertificateX509 setValidity(CertificateX509 cert) throws BusException {
        long start = System.currentTimeMillis() / 1000;
        long end = start + TEN_YEARS;
        start -= 3600;

        cert.setValidity(start, end);
        return cert;
    }
}

