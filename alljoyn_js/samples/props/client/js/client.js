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