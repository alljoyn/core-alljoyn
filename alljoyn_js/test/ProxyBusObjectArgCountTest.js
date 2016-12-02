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
