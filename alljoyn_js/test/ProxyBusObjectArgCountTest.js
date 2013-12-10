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
AsyncTestCase("ProxyBusObjectArgCountTest", {
    _setUp: ondeviceready(function(callback) {
        var getProxyObj = function(err) {
            assertFalsy(err);
            bus.getProxyBusObject("a.b/c", callback);
        };
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, getProxyObj);
    }),
    _wrap: function(queue, f) {
        queue.call(function(callbacks) {
            this._setUp(callbacks.add(f));
        });
    },
    tearDown: function() {
        bus.destroy();
    },

    testIntrospect0: function(queue) {
        this._wrap(queue, function(err, proxyObj) {
            assertError(function() { proxyObj.introspect(); }, "TypeError");
        });
    },
    testIntrospect1: function(queue) {
        this._wrap(queue, function(err, proxyObj) {
            assertNoError(function() { proxyObj.introspect(function() {}); }, "TypeError");
        });
    },

    testParseXML0: function(queue) {
        this._wrap(queue, function(err, proxyObj) {
            assertError(function() { proxyObj.parseXML(); }, "TypeError");
        });
    },
    testParseXML1a: function(queue) {
        this._wrap(queue, function(err, proxyObj) {
            assertError(function() { proxyObj.parseXML("xml"); });
        });
    },
    testParseXML1b: function(queue) {
        this._wrap(queue, function(err, proxyObj) {
            assertNoError(function() { proxyObj.parseXML("xml", function() {}); });
        });
    },

    testSecureConnection0a: function(queue) {
        this._wrap(queue, function(err, proxyObj) {
            assertError(function() { proxyObj.secureConnection(); });
        });
    },
    testSecureConnection0b: function(queue) {
        this._wrap(queue, function(err, proxyObj) {
            assertNoError(function() { proxyObj.secureConnection(function() {}); });
        });
    },
    testSecureConnection1a: function(queue) {
        this._wrap(queue, function(err, proxyObj) {
            assertError(function() { proxyObj.secureConnection(true); });
        });
    },
    testSecureConnection1b: function(queue) {
        this._wrap(queue, function(err, proxyObj) {
            assertNoError(function() { proxyObj.secureConnection(true, function() {}); });
        });
    },
});

