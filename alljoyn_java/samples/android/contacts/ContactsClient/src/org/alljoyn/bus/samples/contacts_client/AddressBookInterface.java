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

package org.alljoyn.bus.samples.contacts_client;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.AccessPermission;

/*
 * AddressBookInterface is an example of an AllJoyn interface that uses complex
 * data types to publish simple contact information.
 */
@BusInterface (name = "org.alljoyn.bus.addressbook")
public interface AddressBookInterface {
    /*
     * The BusMethod annotation signifies that this function should be used as part of the AllJoyn
     * interface.
     * for this BusMethod we have manually specified the signature and replySignature.  In most
     * circumstances runtime can figure out what the signature should be.
     *
     * In this instance we inform the bus that the input is a String and integer. while the output
     * is a struct.  All AllJoyn structs must specify there marshaling order using the the @position
     * annotation see Contact.java
     *
     * replySignature could also be "(sa(sis)a(sis))" however it is best to let runtime figure out
     * as much as it can with out manually specifying the signature.
     *
     * All methods that use the BusMethod annotation can throw a BusException and should indicate
     * this fact.
     *
     * The AccessPermission annotation signifies that this function uses system resources that
     * require system permission "android.permission.READ_CONTACTS". Thus the application should
     * declare the required permissions in its manifest file. If there are more than one permission
     * item, they should be separated by ';'.
     */
    @BusMethod(signature = "si", replySignature = "r")
    @AccessPermission("android.permission.READ_CONTACTS")
    public Contact getContact(String name, int userId) throws BusException;

    /*
     * the replySignature indicates that this BusMethod will return an array of structs.
     */
    @BusMethod(replySignature = "ar")
    @AccessPermission("android.permission.READ_CONTACTS")
    public NameId[] getAllContactNames() throws BusException;
}

