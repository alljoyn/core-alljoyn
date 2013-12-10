/*
 * Copyright (c) 2011, 2013 AllSeen Alliance. All rights reserved.
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
AsyncTestCase("BusAttachmentHelpersTest", {
    _setUp: ondeviceready(function(callback) {
        bus = new org.alljoyn.bus.BusAttachment();
        var connect = function(err) {
            assertUndefined(err)
            bus.connect(callback);
        };
        bus.create(false, connect);
    }),
    tearDown: function() {
        bus.destroy();
    },

    testRequestReleaseName: function(queue) {
        queue.call(function(callbacks) {
            var requestName = function(err) {
                assertUndefined(err)
                bus.requestName("org.alljoyn.testName", 0, callbacks.add(releaseName));
            };
            var releaseName = function(err) {
                assertUndefined(err)
                bus.releaseName("org.alljoyn.testName", callbacks.add(done));
            };
            var done = function(err) {
                assertUndefined(err)
            };
            this._setUp(callbacks.add(requestName));
        });
    },

    testAddRemoveMatch: function(queue) {
        queue.call(function(callbacks) {
            var addMatch = function(err) {
                assertUndefined(err)
                bus.addMatch("type='signal'", callbacks.add(removeMatch));
            };
            var removeMatch = function(err) {
                assertUndefined(err)
                bus.removeMatch("type='signal'", callbacks.add(done));
            };
            var done = function(err) {
                assertUndefined(err)
            };
            this._setUp(callbacks.add(addMatch));
        });
    },

    testAdvertiseCancelAdvertiseName: function(queue) {
        queue.call(function(callbacks) {
            var advertiseName = function(err) {
                assertUndefined(err)
                bus.advertiseName("org.alljoyn.testName", 0xffff, callbacks.add(cancelAdvertiseName));
            };
            var cancelAdvertiseName = function(err) {
                assertUndefined(err)
                bus.cancelAdvertiseName("org.alljoyn.testName", 0xffff, callbacks.add(done));
            };
            var done = function(err) {
                assertUndefined(err)
            };
            this._setUp(callbacks.add(advertiseName));
        });
    },

    testFindCancelFindAdvertisedName: function(queue) {
        queue.call(function(callbacks) {
            var findAdvertisedName = function(err) {
                assertUndefined(err)
                bus.findAdvertisedName("org.alljoyn.testName", callbacks.add(cancelFindAdvertisedName));
            };
            var cancelFindAdvertisedName = function(err) {
                assertUndefined(err)
                bus.cancelFindAdvertisedName("org.alljoyn.testName", callbacks.add(done));
            };
            var done = function(err) {
                assertUndefined(err)
            };
            this._setUp(callbacks.add(findAdvertisedName));
        });
    },

    testFindCancelFindAdvertisedNameByTransport: function(queue) {
        queue.call(function(callbacks) {
            var findAdvertisedNameByTransport = function(err) {
                assertUndefined(err)
                bus.findAdvertisedNameByTransport("org.alljoyn.testName", 0xffff, callbacks.add(cancelFindAdvertisedNameByTransport));
            };
            var cancelFindAdvertisedNameByTransport = function(err) {
                assertUndefined(err)
                bus.cancelFindAdvertisedNameByTransport("org.alljoyn.testName", 0xffff, callbacks.add(done));
            };
            var done = function(err) {
                assertUndefined(err)
            };
            this._setUp(callbacks.add(findAdvertisedNameByTransport));
        });
    },

    testNameHasOwner: function(queue) {
        queue.call(function(callbacks) {
            var nameHasOwner = function(err) {
                assertUndefined(err)
                bus.nameHasOwner("org.freedesktop.DBus", callbacks.add(done));
            };
            var done = function(err, hasOwner) {
                assertUndefined(err)
                assertTrue(hasOwner);
            };
            this._setUp(callbacks.add(nameHasOwner));
        });
    },

    testSetDaemonDebug: function(queue) {
        queue.call(function(callbacks) {
            /* Will only succeed if the daemon was built in debug mode */
            var set = function(err) {
                bus.setDaemonDebug("ALL", 15, callbacks.add(clear));
            };
            var clear = function(err) {
                bus.setDaemonDebug("ALL", 0, callbacks.add(done));
            };
            var done = function(err) {
            };
            this._setUp(callbacks.add(set));
        });
    },
});
