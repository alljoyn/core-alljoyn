/*
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
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

