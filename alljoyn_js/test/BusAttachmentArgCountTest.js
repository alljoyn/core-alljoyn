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

AsyncTestCase("BusAttachmentArgCountTest", {
    _wrap: function(queue, test) {
        queue.call(function(callbacks) {
            bus = new org.alljoyn.bus.BusAttachment();
            bus.create(false, callbacks.add(function(err) {
                test(callbacks.add(function() {}));
            }));
        });
    },
    tearDown: function() {
        bus.destroy();
        delete bus;
        bus = null;
    },

    testConstructor0: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() {
                var testbus = new org.alljoyn.bus.BusAttachment();
                testbus.destroy();
                delete testbus;
                testbus = null;
            });
            callback();
        });
    },

    testConnect0a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.connect(); });
            callback();
        });
    },
    testConnect0b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.connect(callback); });
        });
    },
    testConnect1: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.connect("connectSpec", callback); });
        });
    },

    testDisconnect0a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.disconnect(); });
            callback();
        });
    },
    testDisconnect0b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.disconnect(callback); });
        });
    },

    testRegisterSignalHandler0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.registerSignalHandler(); }, "TypeError");
            callback();
        });
    },
    testRegisterSignalHandler1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.registerSignalHandler(function() {}); }, "TypeError");
            callback();
        });
    },
    testRegisterSignalHandler2a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.registerSignalHandler(function() {}, "signalName"); });
            callback();
        });
    },
    testRegisterSignalHandler2b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.registerSignalHandler(function() {}, "signalName", callback); });
        });
    },
    testRegisterSignalHandler3a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.registerSignalHandler(function() {}, "signalName", "sourcePath"); });
            callback();
        });
    },
    testRegisterSignalHandler3b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.registerSignalHandler(function() {}, "signalName", "sourcePath", callback); });
        });
    },

    testUnregisterSignalHandler0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.unregisterSignalHandler(); }, "TypeError");
            callback();
        });
    },
    testUnregisterSignalHandler1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.unregisterSignalHandler(function() {}); }, "TypeError");
            callback();
        });
    },
    testUnregisterSignalHandler2a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.unregisterSignalHandler(function() {}, "signalName"); }, "TypeError");
            callback();
        });
    },
    testUnregisterSignalHandler2b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.unregisterSignalHandler(function() {}, "signalName", callback); });
        });
    },
    testUnregisterSignalHandler3a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.unregisterSignalHandler(function() {}, "signalName", "sourcePath"); }, "TypeError");
            callback();
        });
    },
    testUnregisterSignalHandler3b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.unregisterSignalHandler(function() {}, "signalName", "sourcePath", callback); });
        });
    },

    testRegisterBusListener0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.registerBusListener(); }, "TypeError");
            callback();
        });
    },
    testRegisterBusListener1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.registerBusListener(function() {}); });
            callback();
        });
    },
    testRegisterBusListener1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.registerBusListener(function() {}, callback); });
        });
    },
    testUnregisterBusListener0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.unregisterBusListener(); }, "TypeError");
            callback();
        });
    },
    testUnregisterBusListener1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.unregisterBusListener(function() {}); });
            callback();
        });
    },
    testUnregisterBusListener1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.unregisterBusListener(function() {}, callback); });
        });
    },

    testRequestName0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.requestName(); }, "TypeError");
            callback();
        });
    },
    testRequestName1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.requestName("requestedName"); });
            callback();
        });
    },
    testRequestName1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.requestName("requestedName", callback); });
        });
    },
    testRequestName2a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.requestName("requestedName", 0); });
            callback();
        });
    },
    testRequestName2b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.requestName("requestedName", 0, callback); });
        });
    },

    testReleaseName0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.releaseName(); }, "TypeError");
            callback();
        });
    },
    testReleaseName1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.releaseName("name"); });
            callback();
        });
    },
    testReleaseName1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.releaseName("name", callback); });
        });
    },

    testAddMatch0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.addMatch(); }, "TypeError");
            callback();
        });
    },
    testAddMatch1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.addMatch("rule"); });
            callback();
        });
    },
    testAddMatch1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.addMatch("rule", callback); });
        });
    },

    testRemoveMatch0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.removeMatch(); }, "TypeError");
            callback();
        });
    },
    testRemoveMatch1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.removeMatch("rule"); });
            callback();
        });
    },
    testRemoveMatch1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.removeMatch("rule", callback); });
        });
    },

    testAdvertiseName0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.advertiseName(); }, "TypeError");
            callback();
        });
    },
    testAdvertiseName1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.advertiseName("name"); }, "TypeError");
            callback();
        });
    },
    testAdvertiseName2a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.advertiseName("name", 0); });
            callback();
        });
    },
    testAdvertiseName2b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.advertiseName("name", 0, callback); });
        });
    },

    testCancelAdvertiseName0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.cancelAdvertiseName(); }, "TypeError");
            callback();
        });
    },
    testCancelAdvertiseName1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.cancelAdvertiseName("name"); }, "TypeError");
            callback();
        });
    },
    testCancelAdvertiseName2a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.cancelAdvertiseName("name", 0); });
            callback();
        });
    },
    testCancelAdvertiseName2b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.cancelAdvertiseName("name", 0, callback); });
        });
    },

    testFindAdvertisedName0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.findAdvertisedName(); }, "TypeError");
            callback();
        });
    },
    testFindAdvertisedName1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.findAdvertisedName("namePrefix"); });
            callback();
        });
    },
    testFindAdvertisedName1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.findAdvertisedName("namePrefix", callback); });
        });
    },

    testCancelFindAdvertisedName0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.cancelFindAdvertisedName(); }, "TypeError");
            callback();
        });
    },
    testCancelFindAdvertisedName1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.cancelFindAdvertisedName("namePrefix"); });
            callback();
        });
    },
    testCancelFindAdvertisedName1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.cancelFindAdvertisedName("namePrefix", callback); });
        });
    },

    testBindSessionPort0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.bindSessionPort(); }, "TypeError");
            callback();
        });
    },
    testBindSessionPort1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.bindSessionPort({}); });
            callback();
        });
    },
    testBindSessionPort1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.bindSessionPort({}, callback); });
        });
    },

    testUnbindSessionPort0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.unbindSessionPort(); }, "TypeError");
            callback();
        });
    },
    testUnbindSessionPort1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.unbindSessionPort(0); });
            callback();
        });
    },
    testUnbindSessionPort1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.unbindSessionPort(0, callback); });
        });
    },

    testSetSessionListener0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setSessionListener(); }, "TypeError");
            callback();
        });
    },
    testSetSessionListener1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setSessionListener(0); }, "TypeError");
            callback();
        });
    },
    testSetSessionListener2a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setSessionListener(0, function() {}); });
            callback();
        });
    },
    testSetSessionListener2b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.setSessionListener(0, function() {}, callback); });
        });
    },

    testJoinSession0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.joinSession(); }, "TypeError");
            callback();
        });
    },
    testJoinSession1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.joinSession({ host: 0, port: 1 }); });
            callback();
        });
    },
    testJoinSession1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.joinSession({ host: 0, port: 1 }, callback); });
        });
    },

    testLeaveSession0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.leaveSession(); }, "TypeError");
            callback();
        });
    },
    testLeaveSession1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.leaveSession(0); });
            callback();
        });
    },
    testLeaveSession1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.leaveSession(0, callback); });
        });
    },

    testSetLinkTimeout0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setLinkTimeout(); }, "TypeError");
            callback();
        });
    },
    testSetLinkTimeout1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setLinkTimeout(1); }, "TypeError");
            callback();
        });
    },
    //should assert a TypeError if value other than a number is passed as parameter
    testSetLinkTimeout1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setLinkTimeout(a); }, "TypeError");
            callback();
        });
    },
    testSetLinkTimeout2: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setLinkTimeout(1, 1); }, "TypeError");
            callback();
        });
    },
    //should assert a TypeError if value other than a number is passed as parameter
    testSetLinkTimeout2a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setLinkTimeout(a, 1); }, "TypeError");
            callback();
        });
    },
    //should assert a TypeError if value other than a number is passed as parameter
    testSetLinkTimeout2b: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setLinkTimeout(1, a); }, "TypeError");
            callback();
        });
    },
    testSetLinkTimeout3: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(
                function() { bus.setLinkTimeout(1, 1,
                    function(err, linktimeout) {
                        assertFalsy(err);
                        assertEquals(1, linktimeout);
                    });
            });
            callback();
        });
    },

    testNameHasOwner0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.nameHasOwner(); }, "TypeError");
            callback();
        });
    },
    testNameHasOwner1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.nameHasOwner("name"); }, "TypeError");
            callback();
        });
    },

    testSetDaemonDebug0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setDaemonDebug(); }, "TypeError");
            callback();
        });
    },
    testSetDaemonDebug1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setDaemonDebug("_module"); }, "TypeError");
            callback();
        });
    },
    testSetDaemonDebug2a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setDaemonDebug("_module", 0); });
            callback();
        });
    },
    testSetDaemonDebug2b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.setDaemonDebug("_module", 0, callback); });
        });
    },

    testEnablePeerSecurity0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.enablePeerSecurity(); }, "TypeError");
            callback();
        });
    },
    testEnablePeerSecurity1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.enablePeerSecurity("authMechanisms"); });
            callback();
        });
    },
    testEnablePeerSecurity1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.enablePeerSecurity("authMechanisms", callback); });
        });
    },
    testEnablePeerSecurity2: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.enablePeerSecurity("authMechanisms", function() {}, callback); });
        });
    },

    testReloadKeyStore0a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.reloadKeyStore(); });
            callback();
        });
    },
    testReloadKeyStore0b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.reloadKeyStore(callback); });
        });
    },

    testClearKeyStore0a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.clearKeyStore(); });
            callback();
        });
    },
    testClearKeyStore0b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.clearKeyStore(callback); });
        });
    },

    testClearKeys0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.clearKeys(); }, "TypeError");
            callback();
        });
    },
    testClearKeys1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.clearKeys("guid"); });
            callback();
        });
    },
    testClearKeys1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.clearKeys("guid", callback); });
        });
    },

    testSetKeyExpiration0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setKeyExpiration(); }, "TypeError");
            callback();
        });
    },
    testSetKeyExpiration1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setKeyExpiration("guid"); }, "TypeError");
            callback();
        });
    },
    testSetKeyExpiration2a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.setKeyExpiration("guid", 0); }, "TypeError");
            callback();
        });
    },
    testSetKeyExpiration2b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.setKeyExpiration("guid", 0, callback); }, "TypeError");
        });
    },

    testGetKeyExpiration0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.getKeyExpiration(); }, "TypeError");
            callback();
        });
    },
    testGetKeyExpiration1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.getKeyExpiration("guid"); }, "TypeError");
            callback();
        });
    },

    testAddLogonEntry0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.addLogonEntry(); }, "TypeError");
            callback();
        });
    },
    testAddLogonEntry1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.addLogonEntry("authMechanism"); }, "TypeError");
            callback();
        });
    },
    testAddLogonEntry2: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.addLogonEntry("authMechanism", "userName"); });
            callback();
        });
    },
    testAddLogonEntry3a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.addLogonEntry("authMechanism", "userName", "password"); });
            callback();
        });
    },
    testAddLogonEntry3b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.addLogonEntry("authMechanism", "userName", "password", callback); });
        });
    },

    testGetPeerGUID0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.getPeerGUID(); }, "TypeError");
            callback();
        });
    },
    testGetPeerGUID1: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.getPeerGUID("name"); }, "TypeError");
            callback();
        });
    },

    testCreateInterfacesFromXML0: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.createInterfacesFromXML(); }, "TypeError");
            callback();
        });
    },
    testCreateInterfacesFromXML1a: function(queue) {
        this._wrap(queue, function(callback) {
            assertError(function() { bus.createInterfacesFromXML("xml"); });
            callback();
        });
    },
    testCreateInterfacesFromXML1b: function(queue) {
        this._wrap(queue, function(callback) {
            assertNoError(function() { bus.createInterfacesFromXML("xml", callback); });
        });
    },
});
