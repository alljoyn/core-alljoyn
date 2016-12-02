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
function assertError(callback, error) {
    try {
        callback();
    } catch(e) {
        if (error && org.alljoyn.bus.BusError.name != error) {
            fail("expected to throw " + error + " but threw " + org.alljoyn.bus.BusError.name);
        }
        return true;
    }
    fail("expected to throw exception");
}

function assertNoError(callback) {
    try {
        callback();
    } catch(e) {
        fail("expected not to throw exception, but threw " + org.alljoyn.bus.BusError.name + " (" + org.alljoyn.bus.BusError.message + ")");
    }
}

function assertFalsy(actual) {
    if (actual) {
        fail("expected falsy but was " + actual);
    }
}

function ondeviceready(f) {
    return function(callback) {
        if (window.cordova) {
            document.addEventListener('deviceready', function() { f(callback); }, false);
        } else {
            f(callback);
        };
    };
};

