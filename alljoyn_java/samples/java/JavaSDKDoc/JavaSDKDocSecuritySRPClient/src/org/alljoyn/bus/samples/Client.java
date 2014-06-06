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
import java.util.concurrent.CountDownLatch;

import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;

public class Client {
    static { 
        System.loadLibrary("alljoyn_java");
    }
    private static final short CONTACT_PORT=42;
    static BusAttachment mBus;
    
    private static ProxyBusObject mProxyObj;
    private static SecureInterface mSecureInterface;
    
    private static String mPassword;
    
    private static boolean isJoined = false;
    
    private static SrpKeyXListener mAuthListener;
    
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
                    "/testSrpSecurity",
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
    private static String AuthRequestToString(AuthListener.AuthRequest[] requests) {
        String str;
        str = "[";
        for (AuthListener.AuthRequest request : requests) {
            if (request instanceof AuthListener.AuthRequest) {
                str += "AuthRequest ";
            }
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
    
    /*
     * This Secure Remote Password Key eXchange listener will ask the user to 
     * enter the one time password.  This password must match the six-digit 
     * string randomly generated by the service's SrpKeyXListener class. 
     * 
     * The following code limits the user to three authentication attempts.
     */
    static class SrpKeyXListener implements AuthListener {
        public boolean requested(String authMechanism, String authPeer, int count, String userName,
                                 AuthRequest[] requests) {
            System.out.println(String.format("AuthListener.requested(%s, %s, %d, %s, %s);", 
                    authMechanism ,
                    authPeer,
                    count,
                    userName,
                    AuthRequestToString(requests)));
                    
            if (count <= 3) {
                System.out.println("Please enter one time password:");
                Scanner in = new Scanner(System.in);
                mPassword = in.nextLine();
                for (AuthRequest request : requests) {
                    if (request instanceof PasswordRequest) {
                        ((PasswordRequest) request).setPassword(mPassword.toCharArray());
                    }
                }
                return true;
            }
            return false;
        }

        public void completed(String authMechanism, String authPeer, boolean authenticated) {
            if (!authenticated) {
                System.out.println("Authentication failed.");
            }
        }
    }
    
    public static void main(String[] args) {
        mBus = new BusAttachment("SRPSecurityClient", BusAttachment.RemoteMessage.Receive);
        
        mAuthListener = new SrpKeyXListener();
        Status status = mBus.registerAuthListener("ALLJOYN_SRP_KEYX", mAuthListener);
        if (status != Status.OK) {
            return;
        }
        
        BusListener listener = new MyBusListener();
        mBus.registerBusListener(listener);

        status = mBus.connect();
        if (status != Status.OK) {
            return;
        }
        
        
        System.out.println("BusAttachment.connect successful on " + System.getProperty("org.alljoyn.bus.address"));
        
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
