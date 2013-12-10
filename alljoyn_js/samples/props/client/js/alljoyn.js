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
        var that,
            bus,
            proxyObj;

        var start = function() {
            bus = new org.alljoyn.bus.BusAttachment();
            var connect = function(err) {
                bus.connect(getProxyBusObject);
            };
            var getProxyBusObject = function(err) {
                if (err) {
                    alert("Connect to AllJoyn failed [(" + err + ")]");
                    return;
                }
                bus.getProxyBusObject("org.alljoyn.bus.samples.props/testProperties", getAll);
            };
            /*
             * Property Get, Set, and GetAll methods are part of the org.freedesktop.DBus.Properties
             * interface implemented by all objects containing properties.  The methods take the
             * property name and the interface name it is a part of as parameters.
             */
            var getAll = function(err, proxyBusObject) {
                proxyObj = proxyBusObject;
                that.getAll();
            };
            bus.create(true, connect);
        };

        that = {};

        that.start = function() {
            navigator.requestPermission("org.alljoyn.bus", function() { start(); });
        }

        that.getStringProp = function() {
            var onGet = function(err, context, value) {
                if (err) {
                    alert("Get 'StringProp' failed " + err);
                    return;
                }
                /*
                 * The method returnSignature of org.freedesktop.DBus.Properties.Get is "v", an
                 * AllJoyn VARIANT.  Incoming variants strip the outermost variant wrapper of their
                 * value.  So you see value used directly instead of value.s.
                 */
                that.onstringprop(value);
            };
            /*
             * Get the "StringProp" property of the "org.alljoyn.bus.samples.props.PropsInterface"
             * interface of the remote object.
             */
            proxyObj.methodCall("org.freedesktop.DBus.Properties", "Get", "org.alljoyn.bus.samples.props.PropsInterface", "StringProp", onGet);
        };

        that.setStringProp = function(value) {
            var onSet = function(err, context) {
                if (err) {
                    alert("Set 'StringProp' failed " + err);
                }
                that.onstringprop(value);
            };
            /*
             * Set the "StringProp" property of the "org.alljoyn.bus.samples.props.PropsInterface"
             * interface of the remote object.  The method signature of
             * org.freedesktop.DBus.Properties.Set is "v", an AllJoyn VARIANT.  Outgoing variants
             * must specify the signature of their value, so you see { "s": value } passed as the
             * argument to Set to indicate a STRING value.
             */
            proxyObj.methodCall("org.freedesktop.DBus.Properties", "Set", "org.alljoyn.bus.samples.props.PropsInterface", "StringProp", { "s": value }, onSet);
        };

        that.getIntProp = function() {
            var onGet = function(err, context, value) {
                if (err) {
                    alert("Get 'IntProp' failed " + err);
                }
                that.onintprop(value);
            };
            proxyObj.methodCall("org.freedesktop.DBus.Properties", "Get", "org.alljoyn.bus.samples.props.PropsInterface", "IntProp", onGet);
        };

        that.setIntProp = function(value) {
            var onSet = function(err, context) {
                if (err) {
                    alert("Get 'IntProp' failed " + err);
                }
                that.onintprop(value);
            };
            proxyObj.methodCall("org.freedesktop.DBus.Properties", "Set", "org.alljoyn.bus.samples.props.PropsInterface", "IntProp", { "i": value }, onSet);
        };

        that.getAll = function() {
            var onGet = function(err, context, values) {
                if (err) {
                    alert("Get all properties failed " + err);
                }
                /*
                 * The method returnSignature of org.freedesktop.DBus.Properties.GetAll is "a{sv}",
                 * an array of string to variant DICT_ENTRYs.  Incoming variants strip the outermost
                 * variant wrapper of their value, so you see values.StringProp used directly
                 * instead of values.StringProp.s (similarly values.IntProp instead of
                 * values.IntProp.i).
                 */
                that.onstringprop(values.StringProp);
                that.onintprop(values.IntProp);
            };
            proxyObj.methodCall("org.freedesktop.DBus.Properties", "GetAll", "org.alljoyn.bus.samples.props.PropsInterface", onGet);
        };

        return that;
    }());
