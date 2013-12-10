/*
 * Copyright (c) 2011 - 2013, AllSeen Alliance. All rights reserved.
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

