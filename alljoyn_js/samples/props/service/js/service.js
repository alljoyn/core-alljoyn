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

    alljoyn.onstringprop = function(value) {
        stringProp.innerHTML = value;
    };
    alljoyn.onintprop = function(value) {
        intProp.innerHTML = value;
    };
    alljoyn.start();
};

if (window.cordova) {
    document.addEventListener('deviceready', onDeviceReady, false);
} else {
    onDeviceReady();
}