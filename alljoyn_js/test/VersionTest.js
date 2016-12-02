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
AsyncTestCase("VersionTest", {
    _setUp: ondeviceready(function(callback) {
    }),

    testVersion: function(queue) {
        var v = org.alljoyn.bus.Version;
        console.log("buildInfo: " + v.buildInfo);
        console.log("numericVersion: 0x" + v.numericVersion.toString(16));
        console.log("arch: " + v.arch);
        console.log("apiLevel: " + v.apiLevel);
        console.log("release: " + v.release);
        console.log("version: " + v.version);
    },
});
