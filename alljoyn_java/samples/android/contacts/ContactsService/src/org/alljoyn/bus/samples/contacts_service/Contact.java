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
package org.alljoyn.bus.samples.contacts_service;

import org.alljoyn.bus.annotation.Position;
import org.alljoyn.bus.annotation.Signature;

/*
 * Contact is a data container that describes an entry in the address book for a single contact.
 */
public class Contact {
    /*
     * Contact Name
     * The Position annotation indicates the numeric position of a field within an
     * AllJoyn struct (Java class). This value defines the order that fields are
     * marshalled and unmarshalled.
     *
     * Each element in the class must have a unique position index. The Position
     * count starts with a zero index and is incremented by one for each element.
     */
    @Position(0)
    @Signature("s")
    public String name;

    /*
     * Array of phone numbers
     */
    @Position(1)
    @Signature("ar")
    public Phone[] phone;

    @Position(2)
    @Signature("ar")
    public Email[] email;

    /*
     * buy default assume at least one phone number and one email.
     */
    public Contact() {
        phone = new Phone[1];
        phone[0] = new Phone();
        email = new Email[1];
        email[0] = new Email();
    }

    /*
     * If the contact has more than one phone number then this must be specified
     * before trying to put the phone numbers into the contact container.
     * @param size - number of phone numbers.
     */
    public void setPhoneCount(int size) {
        this.phone = new Phone[size];
        for (int i = 0; i < this.phone.length; i++) {
            this.phone[i] = new Phone();
        }
    }

    /*
     * If the contact has more than one e-mail then this must be specified
     * before trying to put the e-mail addresses into the contact container.
     * @param size - number of e-mail addresses
     */
    public void setEmailCount(int size) {
        this.email = new Email[size];
        for (int i = 0; i < this.email.length; i++) {
            this.email[i] = new Email();
        }
    }

    /*
     * Simple container to hold information about a single phone number.
     *
     * When specifying an AllJoyn struct (Java class) inside another class
     * it must be declared as a static class with out any methods.
     */
    public static class Phone {
        @Position(0)
        public String number;
        @Position(1)
        public int type;
        @Position(2)
        public String label;
    }

    /*
     * simple container to hold information about a single email
     */
    public static class Email {
        @Position(0)
        public String address;
        @Position(1)
        public int type;
        @Position(2)
        public String label;
    }
}