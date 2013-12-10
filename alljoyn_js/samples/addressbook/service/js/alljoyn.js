/*
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
var alljoyn = (function() {
        var NAME = "org.alljoyn.bus.samples.addressbook",
            OBJECT_PATH = "/addressbook";

        var that,
            bus,
            properties;

        /*
         * AllJoyn STRUCTs are represented as JavaScript arrays.  toObject and toStruct are
         * convenience functions to convert between the array representation and a more convenient
         * object representation.
         */
        properties = ["firstName", "lastName", "phoneNumbers"];
        var toObject = function(contact) {
            var obj,
                i;

            obj = {};
            for (i = 0; i < properties.length; ++i) {
                obj[properties[i]] = contact[i];
            }
            return obj;
        }
        var toStruct = function(contact) {
            var struct,
                i;

            struct = [];
            for (i = 0; i < properties.length; ++i) {
                struct[i] = contact[properties[i]];
            }
            return struct;
        };

        var start = function(contacts) {
            bus = new org.alljoyn.bus.BusAttachment();
            /*
             * Enable peer security before calling connect() to ensure that everything is in place
             * before any remote peers access the service.
             *
             * The main differences between a secure application and a plain application, besides
             * the secure properties of the interfaces, are encapsulated in the AuthListener (the
             * second argument to this function).  The BusAttachment calls the listener with various
             * authentication requests in the process of authenticating a peer.  The requests made
             * are dependent on the specific authentication mechanism negotiated between the peers.
             */
            var enablePeerSecurity = function(err) {
                bus.enablePeerSecurity("ALLJOYN_SRP_KEYX", {
                    /*
                     * Authentication requests are being made.  Contained in this call are the
                     * mechanism in use, the number of attempts made so far, the desired user name
                     * for the requests, and the specific credentials being requested in the form of
                     * the credMask.
                     *
                     * A true return value tells the BusAttachment that the requests have been
                     * handled.
                     */
                    onRequest: function(authMechanism, peerName, authCount, userName, credMask, credentials) {
                        /*
                         * We only enabled the "ALLJOYN_SRP_KEYX" authentication mechanism, so we
                         * know that only the PASSWORD will be requested.  This listener sets the
                         * password to "123456".
                         */
                        if (credMask & credentials.PASSWORD) {
                            credentials.password = "123456";
                            return true;
                        }
                        return false;
                    },
                    onSecurityViolation: function(status, context) {
                        alert('Security violation [(' + status + ')]');
                    },
                    /*
                     * An authentication attempt has completed, either successfully or
                     * unsuccessfully.
                     */
                    onComplete: function(authMechanism, peerName, success) {
                        if (!success) {
                            alert('Authentication with "' + peerName + '" failed');
                        }
                    },
                }, connect);
            };
            var connect = function(err) {
                if (err) {
                    alert("Enable peer security failed failed [(" + err + ")]");
                    return;
                }
                bus.connect(createInterface);
            };
            var createInterface = function(err) {
                if (err) {
                    alert("Connect to AllJoyn failed [(" + err + ")]");
                    return;
                }
                bus.createInterface({
                    name: "org.alljoyn.bus.samples.addressbook.AddressBookInterface",
                    /*
                     * The true value of the secure property requests any method calls on this interface
                     * to first authenticate then encrypt the data between the AllJoyn peers.
                     */
                    secure: true,
                    /*
                     * The '(ssa{ss})' signature specifies the AllJoyn representation of a contact in
                     * the address book.
                     *
                     * Let's decode this signature into its JavaScript representation:
                     * - First, the '(' tells us this is an AllJoyn STRUCT.  The binding documentation
                     *   says that this maps to a JavaScript array.  That gives us '[...]'.
                     * - Next, the 's' tells us the first member of the struct is an AllJoyn STRING.
                     *   This maps to a JavaScript string.  The next 's' of the signature is also an
                     *   AllJoyn STRING, so we have '[<string>, <string> ...]'.
                     * - Next, the 'a' tells us this member of the struct is an AllJoyn array.  But the
                     *   following '{' tells us this is an array of AllJoyn DICT_ENTRYs.  This maps to
                     *   an object, where the key, value pair of each DICT_ENTRY is a JavaScript
                     *   property name, value pair.  We now have '[<string>, <string>, { <key0>:
                     *   <value0>, <key1>: <value1>...} ]'.
                     * - Finally, the signature of the key and values are both AllJoyn STRINGs, so
                     *   <keyN> and <valueN> above are both JavaScript strings.
                     *
                     * A specific example of a contact in JavaScript would be:
                     *     [
                     *         'Florence',
                     *         'Nightingale',
                     *         {
                     *             'home': '456-1234',
                     *             'work': '456-5678'
                     *         }
                     *     ]
                     */
                    method: [
                        { name: "setContact", signature: "(ssa{ss})", argNames: "contact" },
                        { name: "getContact", signature: "s", returnSignature: "(ssa{ss})", argNames: "lastName,contact" },
                        { name: "getContacts", signature: "as", returnSignature: "a(ssa{ss})", argNames: "lastNames,contacts" }
                    ]
                }, registerBusObject);
            };
            var registerBusObject = function(err) {
                bus.registerBusObject(OBJECT_PATH, {
                    "org.alljoyn.bus.samples.addressbook.AddressBookInterface": {
                        setContact: function(context, contact) {
                            contacts.set(toObject(contact));
                            /* A void reply to the setContact method call. */
                            context.reply();
                        },
                        getContact: function(context, lastName) {
                            var contact = contacts.get(lastName);
                            if (contact) {
                                context.reply(toStruct(contact));
                            } else {
                                /*
                                 * Reply with an AllJoyn BusError.  The name of the error must follow
                                 * the same naming conventions as AllJoyn interfaces, well-known names,
                                 * etc.  Here the error name is the well-known name concatenated with
                                 * 'ContactNotFound'.
                                 */
                                context.replyError(NAME + ".ContactNotFound", "No such contact");
                            }
                        },
                        getContacts: function(context, lastNames) {
                            var contact,
                                result,
                                i;

                            result = [];
                            for (i = 0; i < lastNames.length; ++i) {
                                contact = contacts.get(lastNames[i]);
                                if (contact) {
                                    result.push(toStruct(contact));
                                }
                            }
                            context.reply(result);
                        }
                    }
                }, requestName);
            };
            var requestName = function(err) {
                bus.requestName(NAME, done);
            };
            var done = function(err) {
                if (err) {
                    alert("Request name '" + NAME + "' failed [(" + err + ")]");
                }
            };
            bus.create(true, enablePeerSecurity);
        };

        that = {};

        that.start = function(contacts) {
            navigator.requestPermission("org.alljoyn.bus", function() { start(contacts); });
        }

        return that;
    }());
