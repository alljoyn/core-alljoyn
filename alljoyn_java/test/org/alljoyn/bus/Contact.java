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

import org.alljoyn.bus.annotation.Position;
import org.alljoyn.bus.annotation.Signature;

import java.util.TreeMap;

/**
 * Contact is a data container that describes an entry in the address book.
 */
public class Contact {

    /** First Name */
    @Position(0)
    public String firstName;

    /** Last Name */
    @Position(1)
    public String lastName;

    /**
     * Map of phone numbers.
     * Key is type of phone number such as "Home", "Main", etc.
     * Value is the actual phone number as a String.
     */
    @Position(2)
    @Signature("a{ss}")
    public TreeMap<String, String> phoneNumberMap;

    public Contact() {
        phoneNumberMap = new TreeMap<String, String>();
    }

}
