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
AsyncTestCase("ProxyInterfaceTest", {
    _setUp: ondeviceready(function(callback) {
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        bus.destroy();
    },

    testEnumerate: function(queue) {
        queue.call(function(callbacks) {
            var getProxyBusObject = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject("org.alljoyn.Bus/org/alljoyn/Bus", callbacks.add(connect))
            };
            var proxy;
            var connect = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;
                bus.connect(callbacks.add(introspect));
            };
            var introspect = function(err) {
                assertFalsy(err);
                proxy.introspect(callbacks.add(onIntrospect));
            };
            var onIntrospect = function(err) {
                assertFalsy(err);
                proxy.getInterface("org.alljoyn.Bus", callbacks.add(done));
            };
            var done = function(err, intf) {
                assertFalsy(err);
                var actual = {};
                for (var i = 0; i < intf.method.length; ++i) {
                    actual[intf.method[i].name] = intf.method[i].name;
                }
                /*
                 * Chrome and Firefox both return an Object for the proxy
                 * interfaces.  Android returns a Function.
                 */
                assertEquals('AdvertiseName', actual['AdvertiseName']);
                assertEquals('AliasUnixUser', actual['AliasUnixUser']);
                assertEquals('BindSessionPort', actual['BindSessionPort']);
                assertEquals('BusHello', actual['BusHello']);
                assertEquals('CancelAdvertiseName', actual['CancelAdvertiseName']);
                assertEquals('CancelFindAdvertisedName', actual['CancelFindAdvertisedName']);
                assertEquals('FindAdvertisedName', actual['FindAdvertisedName']);
                assertEquals('GetSessionFd', actual['GetSessionFd']);
                assertEquals('JoinSession', actual['JoinSession']);
                assertEquals('LeaveSession', actual['LeaveSession']);
                assertEquals('SetLinkTimeout', actual['SetLinkTimeout']);
                assertEquals('UnbindSessionPort', actual['UnbindSessionPort']);
            };
            this._setUp(callbacks.add(getProxyBusObject));
        });
    },
});