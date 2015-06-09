/*
 * Copyright AllSeen Alliance. All rights reserved.
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
AsyncTestCase("AboutObjTest", {
    _setUp: ondeviceready(function(callback) {
        EXPECT_TYPE_ERROR = function(e) {
            if (e != 'TypeError') {
                fail("expect TypeError but was " + e);
            }
        };
        ABOUT_DATA = {
            'AppId' : [0x01, 0xB3, 0xBA, 0x14, 0x1E, 0x82, 0x11, 0xE4, 0x86, 0x51, 0xD1, 0x56, 0x1D, 0x5D, 0x46, 0xB0],
            'DefaultLanguage' : 'en',
            'DeviceName' : 'Desktop',
            'DeviceId' : '93c06771-c725-48c2-b1ff-6a2a59d445b8',
            'AppName' : 'AboutServicesample',
            'Manufacturer' : 'Manufacturer',
            'ModelNumber' : '12345',
            'Description' : 'This is a sample about service',
            'SoftwareVersion' : '1.0.1'
        };

        PORT = 25;
        var OBJECT_PATH = "/test/aboutObj";
        WILD_CARD_CHAR = "*";
        var serviceBusCreate = function(err) {
            assertFalsy(err);
            serviceBus = new org.alljoyn.bus.BusAttachment();
            serviceBus.create(false, createInterface);
        };
        var createInterface = function() {
            var interface_str =
                '<node>\n' +
                '  <interface name="test.about.aboutObj">\n' +
                '  </interface>\n' +
                '</node>\n';
            serviceBus.createInterfacesFromXML(interface_str, registerBusObject);
        };
        var testobject = {
            "test.about.aboutObj": { }
        };

        var registerBusObject = function(err) {
            serviceBus.registerBusObject(OBJECT_PATH, testobject, false, true, connect);
        };

        var connect = function(err) {
            serviceBus.connect(bindSessionPort);
        };

        var bindSessionPort = function(err) {
            assertFalsy(err);
            serviceBus.bindSessionPort({
                port: PORT,
                isMultipoint: true,
                onAccept: function(port, joiner, opts) {
                    return (port === PORT);
                },
                onJoined: function(port, id, joiner) {

                }
            }, callback);
        };

        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, serviceBusCreate);
    }),
    tearDown: function() {
        bus.destroy();
        serviceBus.destroy();
    },

    testAnnounceMissingRequiredFiled: function(queue) {
        queue.call(function(callbacks) {
            var aboutService = function(err) {
                var getAboutObject = function(err) {
                    serviceBus.getAboutObj(callbacks.add(announce));
                }

                var announce = function(err, aboutObj) {
                    BAD_ABOUT_DATA = {
                        'AppId' : [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
                    };
                    try {
                        aboutObj.announce(PORT, BAD_ABOUT_DATA, done);
                    } catch(e) {
                        EXPECT_TYPE_ERROR(e);
                    }

                    BAD_ABOUT_DATA['DeviceId'] = 'fakeID';
                    try {
                        aboutObj.announce(PORT, BAD_ABOUT_DATA, done);
                    } catch(e) {
                        EXPECT_TYPE_ERROR(e);
                    }

                    BAD_ABOUT_DATA['DefaultLanguage'] = 'en';
                    try {
                        aboutObj.announce(PORT, BAD_ABOUT_DATA, done);
                    } catch(e) {
                        EXPECT_TYPE_ERROR(e);
                    }

                    BAD_ABOUT_DATA['AppName'] = 'test';
                    try {
                        aboutObj.announce(PORT, BAD_ABOUT_DATA, done);
                    } catch(e) {
                        EXPECT_TYPE_ERROR(e);
                    }

                    BAD_ABOUT_DATA['Manufacturer'] = 'Manufacturer';
                    try {
                        aboutObj.announce(PORT, BAD_ABOUT_DATA, done);
                    } catch(e) {
                        EXPECT_TYPE_ERROR(e);
                    }
                    BAD_ABOUT_DATA['ModelNumber'] = '123456';
                    try {
                        aboutObj.announce(PORT, BAD_ABOUT_DATA, done);
                    } catch(e) {
                        EXPECT_TYPE_ERROR(e);
                    }
                    BAD_ABOUT_DATA['Description'] = 'description';
                    try {
                        aboutObj.announce(PORT, BAD_ABOUT_DATA, done);
                    } catch(e) {
                        EXPECT_TYPE_ERROR(e);
                    }
                    BAD_ABOUT_DATA['SoftwareVersion'] = '1.1.1';
                    // now all required fields are set.
                    aboutObj.announce(PORT, BAD_ABOUT_DATA, callbacks.add(done));
                }

                var done = function(err) {
                    assertFalsy(err);
                };
                getAboutObject();
            }

            this._setUp(callbacks.add(aboutService));
        });
    },

    testAnnounce: function(queue) {
        queue.call(function(callbacks) {

            var aboutService = function(err) {

                var getAboutObject = function(err) {
                    serviceBus.getAboutObj(callbacks.add(announce));
                }

                var announce = function(err, aboutObj) {
                    aboutObj.announce(PORT, ABOUT_DATA, callbacks.add(done));
                }

                getAboutObject();
            }

            var registerAboutListener = function(err) {
                var aboutListener = {
                    onAnnounced: callbacks.add(function(busName, version, port, objectDescriptionArg, aboutDataArg) {

                        assertEquals("AboutServicesample", aboutDataArg["AppName"]);
                        assertEquals("en", aboutDataArg["DefaultLanguage"]);
                        assertEquals("Desktop", aboutDataArg["DeviceName"]);
                        assertEquals("AboutServicesample", aboutDataArg["AppName"]);
                        assertEquals("Manufacturer", aboutDataArg["Manufacturer"]);
                        assertEquals("12345", aboutDataArg["ModelNumber"]);


                        var str = "";
                        for(var key in objectDescriptionArg) {
                            str += objectDescriptionArg[key];
                        }
                        assertNotEquals(-1, str.indexOf("/test/aboutObj"));
                        assertNotEquals(-1, str.indexOf("test.about.aboutObj"));

                        bus.cancelWhoImplements(WILD_CARD_CHAR, callbacks.add(function(err) {
                            assertFalsy(err);
                            bus.unregisterAboutListener(aboutListener, callbacks.add(function(err) { assertFalsy(err); }));
                        }));
                    }),
                };
                bus.registerAboutListener(aboutListener, callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(whoImplements));
            };

            var whoImplements = function(err) {
                assertFalsy(err);
                bus.whoImplements(WILD_CARD_CHAR, callbacks.add(aboutService));
            };

            var done = function(err) {
                assertFalsy(err);
            };
            this._setUp(callbacks.add(registerAboutListener));
        });

    }
});
