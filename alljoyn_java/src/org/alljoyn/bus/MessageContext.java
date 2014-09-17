/*
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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
 * Contains information about a specific method call or signal
 * message.
 */
public final class MessageContext {

    /**
     * {@code true} if the message is unreliable.  Unreliable messages have a non-zero
     * time-to-live and may be silently discarded.
     */
    public boolean isUnreliable;

    /**
     * The object path for this message.  An empty string if unable to find the
     * AllJoyn object path.
     */
    public String objectPath;

    /**
     * The interface for this message.  An empty string if unable to find the
     * AllJoyn interface.
     */
    public String interfaceName;

    /**
     * The member (method/signal) name for this message.  An empty string if
     * unable to find the member name.
     */
    public String memberName;

    /**
     * The destination for this message.  An empty string if unable to find the
     * message destination.
     */
    public String destination;

    /**
     * The sender for this message.  An empty string if the message did not
     * specify a sender.
     */
    public String sender;

    /**
     * The session ID that the sender used to send the message.
     */
    public int sessionId;

    /**
     * The serial number of the message.
     */
    public int serial;

    /**
     * The signature for this message.  An empty string if unable to find the
     * AllJoyn signature.
     */
    public String signature;

    /**
     * The authentication mechanism in use for this message.
     */
    public String authMechanism;

    @SuppressWarnings("unused")
    private MessageContext(boolean isUnreliable, String objectPath, String interfaceName,
                           String memberName, String destination, String sender, int sessionId,
                           String signature, String authMechanism, int serial) {
        this.isUnreliable = isUnreliable;
        this.objectPath = objectPath;
        this.interfaceName = interfaceName;
        this.memberName = memberName;
        this.destination = destination;
        this.sender = sender;
        this.sessionId = sessionId;
        this.signature = signature;
        this.authMechanism = authMechanism;
        this.serial = serial;
    }

    public MessageContext() {
    }
}
