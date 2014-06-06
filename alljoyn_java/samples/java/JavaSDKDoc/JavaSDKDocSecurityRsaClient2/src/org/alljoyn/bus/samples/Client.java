/*
 * Copyright (c) 2010-2011, 2013, AllSeen Alliance. All rights reserved.
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
package org.alljoyn.bus.samples;

import java.util.Scanner;

import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.AuthListener.AuthRequest;
import org.alljoyn.bus.AuthListener.CertificateRequest;
import org.alljoyn.bus.AuthListener.PasswordRequest;
import org.alljoyn.bus.AuthListener.PrivateKeyRequest;
import org.alljoyn.bus.AuthListener.VerifyRequest;

public class Client {
    static { 
        System.loadLibrary("alljoyn_java");
    }
    private static final short CONTACT_PORT=42;
    static BusAttachment mBus;
    
    private static ProxyBusObject mProxyObj;
    private static SecureInterface mSecureInterface;
    
    private static boolean isJoined = false;
    
    static class MyBusListener extends BusListener {
        public void foundAdvertisedName(String name, short transport, String namePrefix) {
            System.out.println(String.format("BusListener.foundAdvertisedName(%s, %d, %s)", name, transport, namePrefix));
            short contactPort = CONTACT_PORT;
            SessionOpts sessionOpts = new SessionOpts();
            sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
            sessionOpts.isMultipoint = false;
            sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
            sessionOpts.transports = SessionOpts.TRANSPORT_ANY;
            
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();
            
            mBus.enableConcurrentCallbacks();
            
            Status status = mBus.joinSession(name, contactPort, sessionId, sessionOpts,    new SessionListener(){
                            public void sessionLost(int sessionId, int reason) {
                                System.out.println("Session Lost : " + sessionId + " reason: " + reason);
                            }
            });
            if (status != Status.OK) {
                return;
            }
            System.out.println(String.format("BusAttachement.joinSession successful sessionId = %d", sessionId.value));
            
            mProxyObj =  mBus.getProxyBusObject("com.my.well.known.name",
                    "/testRsaSecurity",
                    sessionId.value,
                    new Class<?>[] { SecureInterface.class});

            mSecureInterface = mProxyObj.getInterface(SecureInterface.class);
            isJoined = true;
            
        }
        public void nameOwnerChanged(String busName, String previousOwner, String newOwner){
            if ("com.my.well.known.name".equals(busName)) {
                System.out.println("BusAttachement.nameOwnerChagned(" + busName + ", " + previousOwner + ", " + newOwner);
            }
        }
        
    }
    
    /* The RSA Key eXchange listener uses X.509 certificates to authenticate. */
    private static class RsaKeyXListener implements AuthListener { 
        /* password used when generating the private key */
        private static final String mPassword = "pass";
        /* 2048 bit RSA private key */
        private static final String mPrivateKey = 
            "-----BEGIN RSA PRIVATE KEY-----\n" +
            "Proc-Type: 4,ENCRYPTED\n" +
            "DEK-Info: DES-EDE3-CBC,61A8FD1CE34DE29B\n" +
            "\n" +
            "uIG/qraR9tvxhxyMPsLAP8ZimZBHMwJuMnRkAoU/fO/REnX7Zo+OLD+/ks4MnmBX\n" +
            "1ziBBP3MSRcvF47C8U5qmdHPg4ujzJgbX0F6tCMOmjdeAt+iFOjov43gfuGXY3sr\n" +
            "L/28OMUqrgyYVuaytVPBWOSFKy9BBSO/h/pCVHcQC+2C4vGXgz3aAoM4euqXgGXT\n" +
            "xVMh8wLAMfRFJHmTmb65n8LPb5j2UOLJ3MkFXXi753qwLfqMlEO7cE5UIh+C+LwA\n" +
            "KhFfehtAKMlaArnlfuJMnEuHj9vj7XfhFpdH23AXUt3CBngJvuF/xi9r0bfwEgzi\n" +
            "0Nc4wKCyj2VTr3qH22ZYat5gkSh8p8qs8FsQyZE/IE7BUp1han25fev1ycsvLaqZ\n" +
            "zxhoCNJMrX3o+B8OqKN4sJPMte6/5BcJkvap4Wih1iNv+H0Aflc0A9Wrfww16bgx\n" +
            "C3OtxfFWB8CC5d6DNxmc3Ef6M+wzbB4OW/umWThPLcqK7tjlq2hqeaOJnsW9CwXL\n" +
            "o/6LoAwrRss/Tp+1sw7FRKQ+cOdKXdjWwtpu8JV3EF9zibxMG00V2OLaHHU+JrOY\n" +
            "j9Mkru2ItcJEOtH1hv5iNpfTMnZbgfDF1iEhAZNKDdvoSVM8MrcrQONLZ8whiUlN\n" +
            "LQJU9I3oCLHJEHS23T2gMIJFyU7eb+8A+dbMEPqhvm6VK2PONEQZtQAEf+O+Hgq2\n" +
            "ccx94SayajiwQoPcu8uHPEeJym2rx3/IZhttymvAqxX9GzydvsPnQedRT31PjrMq\n" +
            "bZYdNMV2Y9+GLef/x6dwFMjzHZC2o8nglCJ/PYpebISPJLRsdYt/HuGr/XFAKPQv\n" +
            "jN1xbUWOYSiws/1xnidb6SYQEPHvsutaerplHeZ7XQkddKeXoz0BTF9XnY3magCd\n" +
            "tWnuZtznd56pggNY4skUMF8bh/sbHtF0/MfI5SOAPmrvGsC/6+lZG/HGM1LM9u5+\n" +
            "016U1T54DjV3yDgLen1rQ4R8wZ3DLc15VVX5L4ORKqFb3073oX+RGUeS0c5x+Pqd\n" +
            "/rp9CdWN3FcyC/NXsCo1GVR3qW/5/HwimQxghLqEgho1ZELIZskMP0zzBSejcUFJ\n" +
            "5vLiusCU8C0DdcV3sfye/8kNcIalIyBeMJviA9s4AOylZsXdJyADHpHdrMsIa0R0\n" +
            "FyqZBHilUMlHDbQgd0+Nkuh+zP4iXh89MJQl7Knb8Y/aqCO8LYGNLaXv8lOFnqYr\n" +
            "mYnR9HxLHfkKoe8qoKQqwDDrzIZGRhMYBAX2RgxSXSkfUSem9ccJ+t6vm7js5ORj\n" +
            "IT8JunaRFxBYkP9popYEjV0iEKWeshyulQHpScnncca0IqkHdyNfrK+zJhoi2KhU\n" +
            "eTlael/ACxP1GEZDd2pPTzwB/KdcoQQrjfi/zFovIOYmcIo1Q2eJZLgJyc3x1ne/\n" +
            "21/pYZ3FiOjr3tP9MHLg3tcdi7ZShzd81bvudFCuSI569MJpGGlLYuBn/+UvohBK\n" +
            "igOnHn/R60euBdsQdAxhDWJqzVDm1q5J0jmN5hk2RvQlfbX/L+cZvJsClhsO4ZX7\n" +
            "JoWdSxXn5gCsWAX8LZUa46tRac1QuCPSJw+q55dif6ze6/A7vnyjg3ZxDLKxcs/O\n" +
            "-----END RSA PRIVATE KEY-----\n";
        /* X.509 certificate good till Jul  9 15:11:06 2021 GMT */
        private static final String mCert = 
            "-----BEGIN CERTIFICATE-----\n" +
            "MIIEgzCCA2ugAwIBAgIJAPij5sS8BfieMA0GCSqGSIb3DQEBBQUAMIGHMQswCQYD\n" +
            "VQQGEwJOQTETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHU2VhdHRsZTEN\n" +
            "MAsGA1UEChMEUXVJQzEQMA4GA1UECxMHQWxsSm95bjENMAsGA1UEAxMEVXNlcjEh\n" +
            "MB8GCSqGSIb3DQEJARYSdXNlckBzb21lcGxhY2UuY29tMB4XDTExMDcxMjE1MTEw\n" +
            "NloXDTIxMDcwOTE1MTEwNlowgYcxCzAJBgNVBAYTAk5BMRMwEQYDVQQIEwpXYXNo\n" +
            "aW5ndG9uMRAwDgYDVQQHEwdTZWF0dGxlMQ0wCwYDVQQKEwRRdUlDMRAwDgYDVQQL\n" +
            "EwdBbGxKb3luMQ0wCwYDVQQDEwRVc2VyMSEwHwYJKoZIhvcNAQkBFhJ1c2VyQHNv\n" +
            "bWVwbGFjZS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDliF9P\n" +
            "2asoGLtCJ8UXKu7aGFLXM2YpO6T7e9k0YvPMaw5WFJG5KOu0Zy9BvQZ/AuaewSH7\n" +
            "HcLQfM++IG2VVsrWKuL/a+q3leIpLHtXLdL0d04Sg4ZFk5MhRosOQ0L2cv9U8XeE\n" +
            "286JAKO+uLYw0a5c67GZMwUUh/dmU2144rlVtGWhwNGlTK+GeMsYoeusAMeOxjTp\n" +
            "85UdMDcWsT3aOq4t8OAAGesiCaXxHrFf/gQSvR26wvyo85v/eL5PjNuDhOU+U7+Q\n" +
            "RGfkiWzM4iM4qyOKP47Y2qtNQ4ZU3eJsA/jOcDY0MhZRx6vIJSsvI/BDSzMZvVWp\n" +
            "BXp7o9Yyjy17w00ZAgMBAAGjge8wgewwHQYDVR0OBBYEFGa2ncrgF5LrW0kN7OMj\n" +
            "1PVoDvmwMIG8BgNVHSMEgbQwgbGAFGa2ncrgF5LrW0kN7OMj1PVoDvmwoYGNpIGK\n" +
            "MIGHMQswCQYDVQQGEwJOQTETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH\n" +
            "U2VhdHRsZTENMAsGA1UEChMEUXVJQzEQMA4GA1UECxMHQWxsSm95bjENMAsGA1UE\n" +
            "AxMEVXNlcjEhMB8GCSqGSIb3DQEJARYSdXNlckBzb21lcGxhY2UuY29tggkA+KPm\n" +
            "xLwF+J4wDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQUFAAOCAQEAyZk/35YsKQba\n" +
            "iBMkOBP4LeVutQIep9kOHVdIQZjOvsBgIDaRiNClJUd+MfsFHqLIfAC/OulUEz1D\n" +
            "mzaKcO15hcmTOar/nGglEZyIZtL93le3keQBeZHMPO0Dhrmhyhqj+W3lBmOQ4HPJ\n" +
            "CD9dTeY81suQimQwid/Tvl8cACxpQkr+GM3JiHknA7+pUpo+rMJRWn206cY00tzB\n" +
            "UIGMr0DQryQcHFmu2m3uxoS5B7MjlE9VwmOEKDjoxsmMQ9m/ajvj/7rT8g5/z1+m\n" +
            "N9Wk8yx0gDBJJqqmVCh+WYgniiMhfKMC+nCS9fjj9RJVumRndD/B38kpj0TyfyBt\n" +
            "e4BgMacL0w==\n" +
            "-----END CERTIFICATE-----";
        public boolean requested(String mechanism, String peerName, int count,
                String userName, AuthRequest[] requests) {
            System.out.println(String.format("AuthListener.requested(%s, %s, %d, %s, %s);", 
                    mechanism ,
                    peerName,
                    count,
                    userName,
                    AuthRequestsToString(requests)));
            
            /* Collect the requests we're interested in to simplify processing below. */
            PasswordRequest passwordRequest = null;
            PrivateKeyRequest privateKeyRequest = null;
            CertificateRequest certificateRequest = null;
            VerifyRequest verifyRequest = null;
            
            for (AuthRequest request : requests) {
                if (request instanceof PasswordRequest) {
                    passwordRequest = (PasswordRequest) request;
                } else if (request instanceof PrivateKeyRequest) {
                    privateKeyRequest = (PrivateKeyRequest) request;
                }else if (request instanceof CertificateRequest) {
                    certificateRequest = (CertificateRequest) request;
                } else if (request instanceof VerifyRequest) {
                    verifyRequest = (VerifyRequest) request;
                }
            }

            if (verifyRequest != null) {
                /* Verify a certificate chain supplied by the peer. */
                return true;
            } else if (certificateRequest != null && 
                       privateKeyRequest != null &&
                       certificateRequest != null ) {
                /* 
                 * The engine is asking us for our certificate chain.  
                 *
                 * We are providing the full certificate chain the password, 
                 * private key, and certificate.
                 */
                passwordRequest.setPassword(mPassword.toCharArray());
                privateKeyRequest.setPrivateKey(mPrivateKey);
                certificateRequest.setCertificateChain(mCert);
                
                return true;
            } 
            return false;
        }

        public void completed(String mechanism, String peerName, boolean authenticated) {
            if (!authenticated) {
                System.out.println("Authentication failed.");
            } else {
                System.out.println("Authentication successful.");
            }
        }
        
        private String AuthRequestsToString(AuthListener.AuthRequest[] requests) {
            String str;
            str = "[";
            for (AuthListener.AuthRequest request : requests) {
                if (request instanceof AuthListener.CertificateRequest) {
                    str += "CertificateRequest ";
                }
                if (request instanceof AuthListener.LogonEntryRequest) {
                    str += "LogonEntryRequest ";
                }
                if (request instanceof AuthListener.PrivateKeyRequest) {
                    str += "PrivateKeyRequest ";
                }
                if (request instanceof AuthListener.UserNameRequest) {
                    str += "UserNameRequest ";
                }
                if (request instanceof AuthListener.PasswordRequest) {
                    str += "PasswordRequest ";
                }
                if (request instanceof AuthListener.VerifyRequest) {
                    str += "VerifyRequest ";
                }
            }
            str += "]";
            return str;
        }
    }
    
    public static void main(String[] args) {
        mBus = new BusAttachment("RsaKeyXClient2", BusAttachment.RemoteMessage.Receive);
        
        RsaKeyXListener authListener = new RsaKeyXListener();
        Status status = mBus.registerAuthListener("ALLJOYN_RSA_KEYX", authListener);
        if (status != Status.OK) {
            return;
        }
        
        BusListener listener = new MyBusListener();
        mBus.registerBusListener(listener);

        status = mBus.connect();
        if (status != Status.OK) {
            return;
        }
        
        
        System.out.println("BusAttachment.connect successful");
        
        status = mBus.findAdvertisedName("com.my.well.known.name");
        if (status != Status.OK) {
            return;
        }
        System.out.println("BusAttachment.findAdvertisedName successful " + "com.my.well.known.name");
        
        while(!isJoined) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                System.out.println("Program interupted");
            }
        }
        
        try {
            System.out.println("Ping = " + mSecureInterface.Ping("Hello AllJoyn"));
        } catch (BusException e1) {
            System.out.println("-----BusException-----");
            e1.printStackTrace();
        }
        
    }
}
