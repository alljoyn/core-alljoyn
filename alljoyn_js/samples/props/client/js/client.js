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
var onDeviceReady = function() {
    var $ = function(id) {
        return document.getElementById(id);
    };

    var stringProp = $('stringProp'),
        intProp = $('intProp');

    stringProp.onkeypress = function(event) {
        var c = event.which ? event.which : event.keyCode;

        if (c === 13) {
            alljoyn.setStringProp(stringProp.value);
            return false;
        }
    };
    intProp.onkeypress = function(event) {
        var c = event.which ? event.which : event.keyCode;

        if (c === 13) {
            alljoyn.setIntProp(intProp.value);
            return false;
        }
    };

    alljoyn.onstringprop = function(value) {
        stringProp.placeholder = value;
    };
    alljoyn.onintprop = function(value) {
        intProp.placeholder = value;
    };
    alljoyn.start();
};

if (window.cordova) {
    document.addEventListener('deviceready', onDeviceReady, false);
} else {
    onDeviceReady();
}
