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

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.Secure;

/**
 * AddressBookInterface is an example of an AllJoyn interface that uses complex
 * data types (Struct, Dictionary, etc.) to publish a simple addressbook.
 */
@BusInterface
@Secure
public interface AddressBookInterface {

    /**
     * Add or replace a contact in the address book.
     * Only one contact per last name is allowed.
     *
     * @param contact  The contact to add/replace.
     */
    @BusMethod(signature="r")
    public void setContact(Contact contact) throws BusException;

    /**
     * Find a contact in the address book based on last name.
     *
     * @param lastName   Last name of contact.
     * @return Matching contact or null if no such contact.
     */
    @BusMethod(signature="s", replySignature="r")
    public Contact getContact(String lastName) throws BusException;

    /**
     * Return all contacts whose last names match.
     *
     * @param lastNames   Array of last names to find in address book.
     * @return An array of contacts with last name as keys.
     */
    @BusMethod(signature="as", replySignature="ar")
    public Contact[] getContacts(String[] lastNames) throws BusException;
}
