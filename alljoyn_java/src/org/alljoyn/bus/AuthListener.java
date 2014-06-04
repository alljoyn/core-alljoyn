/*
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

/**
 * Authentication listeners are responsible for handling AllJoyn authentication
 * requests.
 *
 * Listener objects are the Java objects that handle notification events and are
 * called from AllJoyn in the context of one of its threads.  All listener
 * objects are expected to be multithread safe (MT-Safe) between construction
 * and destruction.
 *
 * This rule extends to other objects accessed during processing of
 * notifications.  For example, it is a programming error to allow a notifiation
 * method to update a collection in another object without serializing access
 * to the collection.
 *
 * The important consideration in this case is that as soon as one sets up a
 * listener object to receive notifications from AllJoyn, one is implicitly
 * dealing with multithreaded code.
 *
 * Since listener objects generally run in the context of the AllJoyn thread
 * which manages reception of events, If a blocking AllJoyn call is made in
 * the context of a notification, the necessary and sufficient conditions for
 * deadlock are established.
 *
 * The important consideration in this case is that when one receives a
 * notification from AllJoyn, that notification is executing in the context of
 * an AllJoyn thread.  If one makes a blocking call back into AllJoyn on that
 * thread, a deadlock cycle is likely, and if this happens your bus attachment
 * receive thread will deadlock (with itself).  The deadlock is typically broken
 * after a bus timeout eventually happens.
 */
public interface AuthListener {

    /** Authentication credentials set via authentication requests. */
    class Credentials {
        byte[] password;
        String userName;
        String certificateChain;
        String privateKey;
        byte[] logonEntry;
        Integer expiration;

        Credentials() {}
    }

    /** Authentication request. */
    class AuthRequest {
        protected Credentials credentials;

        private AuthRequest() {}
    }

    /** Authentication request for a password, pincode, or passphrase. */
    class PasswordRequest extends AuthRequest {
        private boolean isNew;
        private boolean isOneTime;

        PasswordRequest(Credentials credentials, boolean isNew, boolean isOneTime) {
            this.credentials = credentials;
            this.isNew = isNew;
            this.isOneTime = isOneTime;
        }

        /**
         * Indicates request is for a newly created password.
         *
         * @return {@code true} if request is for a newly created password
         */
        public boolean isNewPassword() {
            return isNew;
        }

        /**
         * Indicates a request is for a one time use password.
         *
         * @return {@code true} if request is for a one time use password
         */
        public boolean isOneTimePassword() {
            return isOneTime;
        }

        /**
         * Sets a requested password, pincode, or passphrase.
         *
         * @param password the password to set
         */
        public void setPassword(char[] password) {
            credentials.password = BusAttachment.encode(password);
        }
    }

    /** Authentication request for a user name. */
    class UserNameRequest extends AuthRequest {

        UserNameRequest(Credentials credentials) {
            this.credentials = credentials;
        }

        /**
         * Sets a requested user name.
         *
         * @param userName the user name to set
         */
        public void setUserName(String userName) {
            credentials.userName = userName;
        }
    }

    /** Authentication request for a chain of PEM-encoded X509 certificates. */
    class CertificateRequest extends AuthRequest {

        CertificateRequest(Credentials credentials) {
            this.credentials = credentials;
        }

        /**
         * Sets a requested public key certificate chain. The certificate must
         * be PEM encoded.
         *
         * @param certificateChain the certificate to chain to set
         */
        public void setCertificateChain(String certificateChain) {
            credentials.certificateChain = certificateChain;
        }
    }

    /** Authentication request for a PEM encoded private key. */
    class PrivateKeyRequest extends AuthRequest {

        PrivateKeyRequest(Credentials credentials) {
            this.credentials = credentials;
        }

        /**
         * Sets a requested private key. The private key must be PEM encoded and
         * may be encrypted.
         *
         * @param privateKey the private key to set
         */
        public void setPrivateKey(String privateKey) {
            credentials.privateKey = privateKey;
        }
    }

    /** Authentication request for a logon entry. */
    class LogonEntryRequest extends AuthRequest {

        LogonEntryRequest(Credentials credentials) {
            this.credentials = credentials;
        }

        /**
         * Sets a logon entry. For example for the Secure Remote
         * Password protocol in RFC 5054, a logon entry encodes the
         * N,g, s and v parameters. An SRP logon entry has the form
         * N:g:s:v where N,g,s, and v are ASCII encoded hexadecimal
         * strings and are seperated by colons.
         *
         * @param logonEntry the logon entry to set
         */
        public void setLogonEntry(char[] logonEntry) {
            credentials.logonEntry = BusAttachment.encode(logonEntry);
        }
    }

    /** Authentication request to set an expiration time for the credentials. */
    class ExpirationRequest extends AuthRequest {

        ExpirationRequest(Credentials credentials) {
            this.credentials = credentials;
        }

        /**
         * Sets an expiration time in seconds relative to the current time for
         * the credentials. This value is optional and can be set on any
         * response to a credentials request. After the specified expiration
         * time has elapsed any secret keys based on the provided credentials
         * are invalidated and a new authentication exchange will be
         * required. If an expiration is not set the default expiration time for
         * the requested authentication mechanism is used.
         *
         * @param expiration the expiration time in seconds
         */
        public void setExpiration(int expiration) {
            credentials.expiration = expiration;
        }
    }

    /**
     * Authentication request for verification of a certificate chain from a
     * remote peer.
     */
    class VerifyRequest extends AuthRequest {
        private String certificateChain;

        VerifyRequest(String certificateChain) {
            this.certificateChain = certificateChain;
        }

        /**
         * Gets the PEM encoded X509 certificate chain to verify.
         *
         * @return an X509 certificate chain
         */
        public String getCertificateChain() {
            return certificateChain;
        }
    }

    /**
     * Called by an authentication mechanism making authentication requests.
     * A count allows the listener to decide whether to allow or reject mutiple
     * authentication attempts to the same peer.
     *
     * Any implementation of this function must be multithread safe.  See the
     * class documentation for details.
     *
     * @param mechanism the name of the authentication mechanism issuing the
     *                  request
     * @param peerName  the name of the remote peer being authenticated.  On the
     *                  initiating side this will be a well-known-name for the
     *                  remote peer. On the accepting side this will be the
     *                  unique bus name for the remote peer.
     * @param count the count (starting at 1) of the number of authentication
     *              request attempts made
     * @param userName the user name for the credentials being requested.  If
     *               this is not the empty string the request is specific to the
     *               named user.
     * @param requests the requests.  The application may handle none, some, or
     *                 all of the requests.
     *
     * @return {@code true} if the request is accepted or {@code false} if the request is
     *         rejected.  If the request is rejected the authentication is
     *         complete.
     */
    boolean requested(String mechanism, String peerName, int count, String userName,
                      AuthRequest[] requests);

    /**
     * Called by the authentication engine when all authentication attempts are
     * completed.
     *
     * Any implementation of this function must be multithread safe.  See the
     * class documentation for details.
     *
     * @param mechanism the name of the authentication mechanism that was used
     *                  or an empty string if the authentication failed
     * @param peerName  the name of the remote peer being authenticated.  On the
     *                  initiating side this will be a well-known-name for the
     *                  remote peer. On the accepting side this will be the
     *                  unique bus name for the remote peer.
     * @param authenticated {@code true} if the authentication succeeded
     */
    void completed(String mechanism, String peerName, boolean authenticated);
}
