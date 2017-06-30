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
    private static SampleOnJoinSessionListener mOnJoined;

    static class MyBusListener extends BusListener {
        public void foundAdvertisedName(String name, short transport, String namePrefix) {
            System.out.println(String.format("BusListener.foundAdvertisedName(%s, %d, %s)", name, transport, namePrefix));

            mOnJoined.acquire();

            if (mOnJoined.isConnected()) {
                mOnJoined.release();
                return;
            }

            Status status = mBus.joinSession(name,
                    CONTACT_PORT,
                    new SessionOpts(),
                    new PrintSessionListener(),
                    mOnJoined,
                    null);

            if (status != Status.OK) {
                mOnJoined.release();
                System.out.println("BusAttachment.joinSession call failed " + status);
            }
        }

        public void nameOwnerChanged(String busName, String previousOwner, String newOwner){
            if ("com.my.well.known.name".equals(busName)) {
                System.out.println("BusAttachment.nameOwnerChagned(" + busName + ", " + previousOwner + ", " + newOwner);
            }
        }

    }

    static class SrpLogonListener implements AuthListener {
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
            UserNameRequest userNameRequest = null;
            for (AuthRequest request : requests) {
                if (request instanceof PasswordRequest) {
                    passwordRequest = (PasswordRequest) request;
                } else if (request instanceof UserNameRequest) {
                    userNameRequest = (UserNameRequest) request;
                }
            }

            if (count <= 3) {

                System.out.print("Please enter user name [user1]: ");
                Scanner in = new Scanner(System.in);
                String user = in.nextLine();
                //check if the string is empty if so use a default user name
                if(user !=null && user.length() == 0) {
                    user = "user1";
                }

                System.out.print("Please enter password [password1]: ");
                String password = in.nextLine();
                //check if the string is empty if so use a default password
                if(user != null && password.length() == 0) {
                    password = "password1";
                }

                if(userNameRequest != null) {
                    userNameRequest.setUserName(user);
                }
                if(passwordRequest != null) {
                    passwordRequest.setPassword(password.toCharArray());
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
        mBus = new BusAttachment("SRPLogonClient", BusAttachment.RemoteMessage.Receive);
        mOnJoined = new SampleOnJoinSessionListener();

        SrpLogonListener authListener = new SrpLogonListener();
        Status status = mBus.registerAuthListener("ALLJOYN_SRP_LOGON", authListener);
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

        while(!mOnJoined.isConnected()) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                System.out.println("Program interupted");
            }
        }

        mProxyObj =  mBus.getProxyBusObject("com.my.well.known.name",
                "/testLogonSecurity",
                mOnJoined.getSessionId(),
                new Class<?>[] { SecureInterface.class});

        mSecureInterface = mProxyObj.getInterface(SecureInterface.class);

        try {
            System.out.println("Ping = " + mSecureInterface.Ping("Hello AllJoyn"));
        } catch (BusException e1) {
            System.out.println("-----BusException-----");
            e1.printStackTrace();
        }

    }
}
