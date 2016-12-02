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

import org.alljoyn.bus.common.KeyInfoNISTP256;

public class CertificateId {

    /**
     * serial Identity certificate's serial
     */
    String m_serial;

    /**
     * keyInfo Identity certificate's KeyInfoNISTP256 structure
     */
    KeyInfoNISTP256 m_issuerKeyInfo;

    private CertificateId(String serial, KeyInfoNISTP256 issuerKeyInfo){
        m_serial = serial;
        m_issuerKeyInfo = issuerKeyInfo;
    }

    public KeyInfoNISTP256 getIssuerKeyInfo() {
        return m_issuerKeyInfo;
    }

    public String getSerial() {
        return m_serial;
    }
}