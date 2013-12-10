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
        var NAME = 'org.alljoyn.bus.samples.addressbook',
            OBJECT_PATH = '/addressbook';

        var that,
            bus,
            properties,
            proxyObj;

        /*
         * AllJoyn STRUCTs are represented as JavaScript arrays.  toObject and toStruct are
         * convenience functions to convert between the array representation and a more convenient
         * object representation.
         */
        properties = ['firstName', 'lastName', 'phoneNumbers'];
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

        var start = function() {
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
                bus.enablePeerSecurity('ALLJOYN_SRP_KEYX', {
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
                        var password;

                        /*
                         * We only enabled the "ALLJOYN_SRP_KEYX" authentication mechanism, so we
                         * know that only the PASSWORD will be requested.  This listener sets the
                         * password to a user-prompted value.
                         */
                        if (credMask & credentials.PASSWORD) {
                            password = prompt('Enter password');
                            if (password) {
                                credentials.password = password;
                                return true;
                            }
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
                    alert('Enable peer security failed failed [(' + err + ')]');
                    return;
                }
                bus.connect(getProxyBusObject);
            };
            var getProxyBusObject = function(err) {
                if (err) {
                    alert('Connect to AllJoyn failed [(' + err + ')]');
                    return;
                }
                bus.getProxyBusObject('org.alljoyn.bus.samples.addressbook/addressbook', done);
            };
            var done = function(err, proxyBusObject) {
                proxyObj = proxyBusObject;
            };
            bus.create(true, enablePeerSecurity);
        };

        that = {};

        that.start = function() {
            navigator.requestPermission("org.alljoyn.bus", function() { start(); });
        }

        that.get = function(onget, lastNames) {
            var onGet = function(err, context, contacts) {
                var result,
                    i;

                if (err) {
                    alert('Get failed ' + err);
                    return;
                }
                result = [];
                for (i = 0; i < contacts.length; ++i) {
                    result.push(toObject(contacts[i]));
                }
                onget(result);
            };
            proxyObj.methodCall('org.alljoyn.bus.samples.addressbook.AddressBookInterface', 'getContacts', lastNames, onGet);
        };

        that.add = function(contact) {
            var onSet = function(err, context) {
                if (err) {
                    alert('Set failed ' + err);
                }
            };
            proxyObj.methodCall('org.alljoyn.bus.samples.addressbook.AddressBookInterface', 'setContact', toStruct(contact), onSet);
        };

        return that;
    }());
