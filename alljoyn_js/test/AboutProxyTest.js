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
AsyncTestCase("AboutProxyTest", {
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
			'Description' : 'This is a test about service',
			'SoftwareVersion' : '1.0.1',
			'HardwareVersion' : '0.1.2',
			'SupportUrl' : 'http://allseenalliance.org'
		};
    	
    	PORT = 25;
    	OBJECT_PATH = "/test/aboutObj";    	
    	var serviceBusCreate = function(err) {
    		assertFalsy(err);
    		serviceBus = new org.alljoyn.bus.BusAttachment();
    		serviceBus.create(false, createInterface);
        };
        var createInterface = function() {
    		var interface_str = 
    			'<node>\n' +
                '  <interface name="test.about.aboutObj">\n' +
                '    <method name="Echo">\n' +
                '    </method>\n' +
                '  </interface>\n' +                    
                '  <interface name="test.about.test">\n' +
                '    <method name="Echo">\n' +
                '    </method>\n' +
                '  </interface>\n' +
                '</node>\n';
    		serviceBus.createInterfacesFromXML(interface_str, registerBusObject);
    	};
    	var testobject = {
            "test.about.aboutObj": {
                Echo: function(context, inStr) {  }
            },
            "test.about.test": {
                Echo: function(context, inStr) {  }
            }    	
        };
    	
    	var registerBusObject = function(err) {    		
			serviceBus.registerBusObject(OBJECT_PATH, testobject, connect);
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

    testGetAboutData: function(queue) {
    	queue.call(function(callbacks) {    		
    		// service
    		var aboutService = function(err) {
            	var getAboutObject = function(err) {        			
        			serviceBus.getAboutObj(callbacks.add(announce));
        		}
        		
        		var announce = function(err, aboutObj) {        			
        			aboutObj.announce(PORT, ABOUT_DATA, callbacks.add(done));
        		}		
        		
        		getAboutObject();
            }
    		
    		
    		// client
    		var onJoined = function(sessionId, busName) {    			
    			var getAboutData = function(err, proxyObj) {				
    				var str = [];
    				proxyObj.methodCall("org.alljoyn.About", "GetAboutData", str, callbacks.add(function(err, context, str) {
    					assertFalsy(err);	
    					assertEquals(ABOUT_DATA["AppName"], str["AppName"]);
                        assertEquals(ABOUT_DATA["DefaultLanguage"], str["DefaultLanguage"]);
                        assertEquals(ABOUT_DATA["DeviceName"], str["DeviceName"]);
                        assertEquals(ABOUT_DATA["AppName"], str["AppName"]);
                        assertEquals(ABOUT_DATA["Manufacturer"], str["Manufacturer"]);
                        assertEquals(ABOUT_DATA["ModelNumber"], str["ModelNumber"]);
                        assertEquals(ABOUT_DATA["Description"], str["Description"]);
                        assertEquals(ABOUT_DATA["SoftwareVersion"], str["SoftwareVersion"]);
                        assertEquals(ABOUT_DATA["HardwareVersion"], str["HardwareVersion"]);
                        assertEquals(ABOUT_DATA["SupportUrl"], str["SupportUrl"]);                        
                    }));				
    			};    			
    			bus.getProxyBusObject(busName + "/About:sessionId=" + sessionId, callbacks.add(getAboutData));
    		};
    		
    		var onError = function() {
    			assert("joinSession error");
    		}
            
            var joinSession = function(onjoined, onerror, busName, port) {
                
                var onJoined = function(err, id, opts) {
                	assertFalsy(err);                	
                    onjoined(id, busName);
                };
                
                bus.joinSession({        	
                    host: busName,
                    port: port,
                    isMultipoint: true,
                                
                    onLost: function(id, reason) {
                        
                    }
                }, callbacks.add(onJoined));
            };
            
        
        	var registerAboutListener = function(err) {        		
                var aboutListener = {
                    onAnnounced: callbacks.add(function(busName, version, port, objectDescriptionArg, aboutDataArg) {                    	
                    	joinSession(onJoined, onError, busName, port);
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
                bus.whoImplements(null, callbacks.add(aboutService));
            };
            
            var done = function(err) {
                assertFalsy(err);                
            };
            
            this._setUp(callbacks.add(registerAboutListener));
    	});
    },

    testGetObjectDescription: function(queue) {
    	queue.call(function(callbacks) {
    		
    		// service
    		var aboutService = function(err) {
            	var getAboutObject = function(err) {
        			serviceBus.getAboutObj(callbacks.add(announce));
        		}
        		
        		var announce = function(err, aboutObj) {						
        			aboutObj.announce(PORT, ABOUT_DATA, callbacks.add(done));
        		}		
        		
        		getAboutObject();
            }
    		
    		
    		// client
    		var onJoined = function(sessionId, busName) {    			
    			var getAboutData = function(err, proxyObj) {				
    				var str = [];
    				proxyObj.methodCall("org.alljoyn.About", "GetObjectDescription", str, callbacks.add(function(err, context, str) {
    					assertFalsy(err);
    					for(var path in str) {
    						assertEquals(OBJECT_PATH, str[path][0]);
    						assertEquals("test.about.aboutObj", str[path][1][0]);
    						assertEquals("test.about.test", str[path][1][1]);
    					}    					    					                        
                    }));				
    			};    			
    			bus.getProxyBusObject(busName + "/About:sessionId=" + sessionId, callbacks.add(getAboutData));
    		};
    		
    		var onError = function() {
    			assert("joinSession error");
    		}
            
            var joinSession = function(onjoined, onerror, busName, port) {
                
                var onJoined = function(err, id, opts) {
                	assertFalsy(err);                	
                    onjoined(id, busName);
                };
                
                bus.joinSession({        	
                    host: busName,
                    port: port,
                    isMultipoint: true,
                                
                    onLost: function(id, reason) {
                        
                    }
                }, callbacks.add(onJoined));
            };
        
        	var registerAboutListener = function(err) {        		
                var aboutListener = {
                    onAnnounced: callbacks.add(function(busName, version, port, objectDescriptionArg, aboutDataArg) {
                    	joinSession(onJoined, onError, busName, port);
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
                bus.whoImplements(null, callbacks.add(aboutService));
            };
            
            var done = function(err) {
                assertFalsy(err);                
            };
            this._setUp(callbacks.add(registerAboutListener));
    	});
    }    
});