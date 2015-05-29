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
AsyncTestCase("AboutListenerTest", {
    _setUp: ondeviceready(function(callback) {
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
    	PORT = 910;
    	OBJECT_PATH = "/test/path";
    	INTERFACE_NAME = ["com.example.about.feature.interface.sample"];
    	var serviceBusCreate = function(err) {
    		assertFalsy(err);
    		serviceBus = new org.alljoyn.bus.BusAttachment();
    		serviceBus.create(false, createInterface);
        };
        var createInterface = function(err) {	
        	if(err)
        		alert(err);
			serviceBus.createInterface({
                name: "com.example.about.feature.interface.sample",
                method: [
                         { name: "Ping", signature: "s", returnSignature: "s", argNames: "inStr,outStr" }
                     ]
            }, registerBusObject);
        };
    	var registerBusObject = function(err) {
    		if(err)
        		alert(err);
    		busObject = {
    			"com.example.about.feature.interface.sample": {  
    				Ping: function(context, str) {
    				}	
				}
			};
			serviceBus.registerBusObject(OBJECT_PATH, busObject, aboutService);
		};
		var aboutService = function(err) {        	
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
    		        		
    		connect();
        }
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, serviceBusCreate);        
    }),
    tearDown: function() {
        bus.destroy();
        serviceBus.destroy();
    },
    
    testReceiverAnnouncement: function(queue) {
    	queue.call(function(callbacks) {
    		var aboutService = function(err) {

				var getAboutObject = function(err) {
					assertFalsy(err);
					serviceBus.getAboutObj(callbacks.add(announce));
				}

				var announce = function(err, aboutObj) {
					assertFalsy(err);
					aboutObj.announce(PORT, ABOUT_DATA, callbacks.add(done));
				}
				getAboutObject();
			}
        	
        	var registerAboutListener = function(err) {
        		assertFalsy(err);
                var aboutListener = {
                    onAnnounced: callbacks.add(function(busName, version, port, objectDescriptionArg, aboutDataArg) {
                    	bus.cancelWhoImplements(INTERFACE_NAME, callbacks.add(function(err) {
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
            	bus.whoImplements(INTERFACE_NAME, callbacks.add(aboutService));
            };
            
            var done = function(err) {
                assertFalsy(err);                
            };
            this._setUp(callbacks.add(registerAboutListener));
    	});
    },
    
    testNullWhoImplementsValue: function(queue) {
        queue.call(function(callbacks) {
			var aboutService = function(err) {

				var getAboutObject = function(err) {
					assertFalsy(err);
					serviceBus.getAboutObj(callbacks.add(announce));
				}

				var announce = function(err, aboutObj) {
					assertFalsy(err);
					aboutObj.announce(PORT, ABOUT_DATA, callbacks.add(done));
				}
				getAboutObject();
			}
        	
        	var registerAboutListener = function(err) {
        		assertFalsy(err);
        		var aboutListener = {
                        onAnnounced: callbacks.add(function(busName, version, port, objectDescriptionArg, aboutDataArg) {
                        	bus.cancelWhoImplements(null, callbacks.add(function(err) {
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
                bus.whoImplements(null, callbacks.add(aboutService));
            };
            
            var done = function(err) {
                assertFalsy(err);                
            };
            this._setUp(callbacks.add(registerAboutListener));
        });
    },    
    
    testUnregisterAllAboutListeners: function(queue) {
    	queue.call(function(callbacks) {
    		var registerAboutListener = function(err) {        		
                var aboutListener1 = {
                    onAnnounced: function(busName, version, port, objectDescriptionArg, aboutDataArg) {
                    },
                };
                var aboutListener2 = {
            		onAnnounced: function(busName, version, port, objectDescriptionArg, aboutDataArg) {
            		},
                };
                var aboutListener3 = {
            		onAnnounced: function(busName, version, port, objectDescriptionArg, aboutDataArg) {
            		},
                };
                bus.registerAboutListener(aboutListener1, function(err) { assertFalsy(err); });
                bus.registerAboutListener(aboutListener2, function(err) { assertFalsy(err); });
                bus.registerAboutListener(aboutListener3, callbacks.add(connect));
            };
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(whoImplements));
            };

            var whoImplements = function(err) {
                assertFalsy(err);
                bus.whoImplements(null, callbacks.add(unregisterAllAboutListeners));
            };
            
            var unregisterAllAboutListeners = function(err) {
            	assertFalsy(err);
            	bus.unregisterAllAboutListeners(callbacks.add(done));            	
            }
            
            var done = function(err) {
                assertFalsy(err);                
            };
            this._setUp(callbacks.add(registerAboutListener));
    		
    	});
    }
});
