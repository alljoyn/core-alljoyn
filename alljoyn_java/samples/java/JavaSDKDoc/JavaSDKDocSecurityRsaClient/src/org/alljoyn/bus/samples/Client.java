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
    
    static class RsaKeyXListener implements AuthListener {
        public boolean requested(String mechanism, String peerName, int count, String userName,
                                 AuthRequest[] requests) {
            System.out.println(String.format("AuthListener.requested(%s, %s, %d, %s, %s);", 
                    mechanism ,
                    peerName,
                    count,
                    userName,
                    AuthRequestsToString(requests)));
            
             /* Collect the requests we're interested in to simplify processing below. */
            PasswordRequest passwordRequest = null;
            CertificateRequest certificateRequest = null;
            VerifyRequest verifyRequest = null;
            for (AuthRequest request : requests) {
                if (request instanceof PasswordRequest) {
                    passwordRequest = (PasswordRequest) request;
                } else if (request instanceof CertificateRequest) {
                    certificateRequest = (CertificateRequest) request;
                } else if (request instanceof VerifyRequest) {
                    verifyRequest = (VerifyRequest) request;
                }
            }

            if (verifyRequest != null) {
                /* Verify a certificate chain supplied by the peer. */
                return true;
            } else if (certificateRequest != null) {
                /* 
                 * The engine is asking us for our certificate chain.  
                 *
                 * If we return true and do not supply the certificate chain, then the engine will
                 * create a self-signed certificate for us.  It will ask for the passphrase to use
                 * for the private key via a PasswordRequest. 
                 */
                return true;
            } else if (passwordRequest != null) {
                /*
                 * A password request under the ALLJOYN_RSA_KEYX mechanism is for the passphrase of the
                 * private key.
                 *
                 * PasswordRequest.isNewPassword() indicates if the engine has created a private key
                 * for us (as part of creating a self-signed certificate).  Otherwise it is
                 * expecting the passphrase for the existing private key.
                 */
                if (count <= 3) {
                    /*
                     * Request the passphrase of our private key via the UI.  We need to wait
                     * here for the user to enter the passphrase before we can return.  The
                     * latch takes care of the synchronization for us.
                     */
                    if(passwordRequest.isNewPassword()){
                        System.out.print("Enter password to generate new " +
                                "private key and certificate : ");
                    } else {
                        System.out.print("Please enter private key password : ");
                    }
                    Scanner in = new Scanner(System.in);
                    String password = in.nextLine();
                    passwordRequest.setPassword(password.toCharArray());
                    return true;
                }
            }
            return false;
        }

        public void completed(String authMechanism, String authPeer, boolean authenticated) {
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
        mBus = new BusAttachment("RsaKeyXClient", BusAttachment.RemoteMessage.Receive);
        
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
