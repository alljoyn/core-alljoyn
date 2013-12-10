/*
 * Copyright (c) 2011, 2013, AllSeen Alliance. All rights reserved.
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
AsyncTestCase("ProxyMethodArgCountTest", {
    _setUp: ondeviceready(function(callback) {
        var createInterface = function(err) {
            assertFalsy(err);
            bus.createInterface({
                name: "org.alljoyn.bus.NoReply",
                method: [
                    { name: 'Ping', signature: 's', argNames: 'inStr' }
                ]
            }, registerBusObject);
        };
        var registerBusObject = function(err) {
            assertFalsy(err);
            bus.registerBusObject("/testobject",
                                  {
                                      "org.alljoyn.bus.NoReply": {
                                          Ping: function(context, inStr) {}
                                      }
                                  },
                                  false,
                                  connect);
        };
        var connect = function(err) {
            assertFalsy(err);
            bus.connect(getProxyObj);
        };
        var getProxyObj = function(err) {
            assertFalsy(err);
            bus.getProxyBusObject(bus.uniqueName + "/testobject", callback);
        };
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, createInterface);
    }),
    _wrap: function(queue, f) {
        queue.call(function(callbacks) {
            this._setUp(callbacks.add(f));
        });
    },
    tearDown: function() {
        bus.destroy();
    },

    testMethodCall0: function(queue) {
        this._wrap(queue, function(err, testobject) {
            assertError(function () { testobject.methodCall("org.alljoyn.bus.NoReply", "Ping", "str"); }, "TypeError");
        });
    },
    testMethodCall1: function(queue) {
        this._wrap(queue, function(err, testobject) {
            assertError(function () { testobject.methodCall("org.alljoyn.bus.NoReply", "Ping", "str"); }, "TypeError");
        });
    },
    testMethodCall2a: function(queue) {
        this._wrap(queue, function(err, testobject) {
            assertError(function () { testobject.methodCall("org.alljoyn.bus.NoReply", "Ping", "str", null); }, "TypeError");
        });
    },
    testMethodCall2b: function(queue) {
        this._wrap(queue, function(err, testobject) {
            assertNoError(function () { testobject.methodCall("org.alljoyn.bus.NoReply", "Ping", "str", function() {}); });
        });
    },
    testMethodCall3: function(queue) {
        this._wrap(queue, function(err, testobject) {
            assertNoError(function () { testobject.methodCall("org.alljoyn.bus.NoReply", "Ping", "str", { timeout: 30 }, function() {}); });
        });
    },
});
