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
