/**
 * 
 */

var alljoyn = (function() {		

	var DEVICES = [];
	DEVICES.displayingIndex = -1;
	
	// About global variable
	var ABOUT = [];
	
	/*
	 * interface name for whoImplements
	 */ 
	ABOUT.interfaces = [];
	/*
	 * org.alljoyn.About interface definitions 
	 */
	ABOUT.INTERFACE_NAME = "org.alljoyn.About";
	/*
	 * org.alljoyn.About.Icon interface definitions
	 */ 
	ABOUT.ICON_OBJECT_PATH = "/About/DeviceIcon";
	ABOUT.ICON_INTERFACE_NAME = "org.alljoyn.Icon";	
	
	/*
	 * handler for the org.alljoyn.About.Anounce sessionless signal
	 * @param[in] busName              well know name of the remote BusAttachment
     * @param[in] version              version of the Announce signal from the remote About Object
     * @param[in] port                 SessionPort used by the announcer
     * @param[in] objectDescriptionArg  MsgArg the list of object paths and interfaces in the announcement
     * @param[in] aboutDataArg          MsgArg containing a dictionary of Key/Value pairs of the AboutData
	 */
	ABOUT.announceListener = {
		onAnnounced: function(busName, version, port, objectDescriptionArg, aboutDataArg) {
			console.log("==========================================");
			console.log("busName : " + busName + ", version : " + version + ", port : " + port);
			var objectDescStr = "";
			for(var path in objectDescriptionArg) {
				objectDescStr += objectDescriptionArg[path][0];
				objectDescStr += ', ';
				for(var intf in objectDescriptionArg[path][1]) {
					objectDescStr += objectDescriptionArg[path][1][intf];
				}
				objectDescStr += '\n';
			}
	        console.log(objectDescStr);
	        
	        var hasIcon = hasAboutIcon(objectDescriptionArg);
	        console.log("hasIcon : " + hasIcon);
	        if(hasIcon) {
	        	joinForGetIcon(busName, port);
	        }	        
	        
	        var aboutDataStr = "";
	        for(var key in aboutDataArg) {
	        	aboutDataStr += key + " = " + aboutDataArg[key] + '\n';
	        }
	        console.log(aboutDataStr);
	        console.log("==========================================");
	        DEVICES.push({
	        	busName : busName,
	            version : version,
	            port : port,	            
	            objectDescription : objectDescriptionArg,
	            aboutData : aboutDataArg,
	            imgIcon : "img/default_devices_icon.png",
	            sessionId : -1
	        });
	        ondraw();
		}
	}

	var that = [],
		bus;
	
	/*
     * Get the instance of our plugin.  All our scripting will begin with this.
     */
    aj = org.alljoyn.bus;
	
    var drawSelectedDevice = function(index) {
		var wrapper = document.getElementById('selected_device');
		var str = '<style type="text/css">';
		str += '.item:nth-child(';
		str += index;
		str += ') {box-shadow:0 0 8px rgba(255,0,0,1.0);}</style>';
		wrapper.innerHTML = str;
	}
    
	var ondraw = function() {
		var images = document.getElementById('main_section');
		
		if(DEVICES.displayingIndex > -1)			
			drawSelectedDevice(DEVICES.displayingIndex+1);
		var str = "";
		for(var i=0; i<DEVICES.length; ++i) {			
			str += '<div class="item" onclick="alljoyn.clickHandler(this.id)"';
			str += ' id=' + DEVICES[i].busName +'>';
			str += '<img src="';
			str += DEVICES[i].imgIcon + '"';
			str += ' />';			
			str += DEVICES[i].aboutData["DeviceName"];
		    str += '</div>';
		}
		images.innerHTML = str;
	}
	
	function decodeArrayBuffer(busName, buffer) {
	    var mime;
	    var a = new Uint8Array(buffer);
	    var nb = a.length;
	    if (nb < 4)
	        return null;
	    
	    if (a[0] == 0x89 && a[1] == 0x50 && a[2] == 0x4E && a[3] == 0x47)
	        mime = 'image/png';
	    else if (a[0] == 0xff && a[1] == 0xd8)
	        mime = 'image/jpeg';
	    else if (a[0] == 0x47 && a[1] == 0x49 && a[2] == 0x46)
	        mime = 'image/gif';
	    else
	        return null;
	    var binary = "";
	    for (var i = 0; i < nb; i++)
	        binary += String.fromCharCode(a[i]);
	    var base64 = window.btoa(binary);
	    var image = new Image();
	    image.onload = function() {
	    	for(var i=0; i<DEVICES.length; ++i) {
				if(DEVICES[i].busName == busName) {
					DEVICES[i].imgIcon = image.src;
					break;
				}
			}
	    	ondraw();
	    }
	    image.src = 'data:' + mime + ';base64,' + base64;
	}

	var showAnnouncedData = function(str) {
		var name = document.getElementById('main_aside');
		
		var aboutDataStr = "<H1>Device details</H1>";		
	    for(var key in str) {
	    	aboutDataStr += key + " = " + str[key] + '<p>';
	    }
		name.innerHTML = aboutDataStr;	
		drawSelectedDevice(DEVICES.displayingIndex+1);
	}
	
	var hasAboutIcon = function(objDescription) {
		for(var i=0; i<objDescription.length; ++i) {
			var path = objDescription[i];

			if(ABOUT.ICON_OBJECT_PATH == path[0]) {
				var intf = path[1];				
				for(var j=0; j<intf.length; ++j) {
					if(ABOUT.ICON_INTERFACE_NAME == intf[j])
						return true;
				}				
			}			
		}		
		return false;		
	}
	
	/*
	 * temporarily join for get AboutIcon data.
	 * after get AboutIcon data, leave the session.
	 */
	var joinForGetIcon = function(busName, port) {
    	var iconSessionId;
    	bus.joinSession({
    		host: busName,
    		port: port,
    		isMultipoint: true
    	}, function(err, id, opts) {
    		
    		if (err) {
    			alert("Join session failed [(" + err + ")]");
    			onerror();
    			return;
    		}
    		iconSessionId = id;

    		var iconJoined = function() {
    			console.log("joined for get icon");
    			var getIconData = function(err, proxyObj) {				

    				var icon = [];
    				proxyObj.methodCall(ABOUT.ICON_INTERFACE_NAME, "GetContent", icon, function(err, context, icon) {
    					if (err) {
    						console.log(err);
    						console.log("icon method call failed");
    						return;
    					}
    					
    					decodeArrayBuffer(busName, icon);
    					
    					var done = function(err) {
    						console.log("leaveSession done");
    						iconSessionId = -1;    		                
    		            }
    					bus.leaveSession(iconSessionId, done);
    				});
    			};
    			bus.getProxyBusObject(busName + "/About/DeviceIcon:sessionId=" + iconSessionId, getIconData);
    		}
    		iconJoined();
    	});    	
    }
	
	/*
	 * image click handler 
	 */
	that.clickHandler= function(busName) {
		var onJoined,
        onError,
        err;
		var port;
		
		onJoined = function(sessionId) {
			console.log("onJoined");
			var getAboutData = function(err, proxyObj) {				
				var str = [];
				proxyObj.methodCall(ABOUT.INTERFACE_NAME, "GetAboutData", str, function(err, context, str) {
					if (err) {
						console.log("method call failed");
						return;
					}
					showAnnouncedData(str);					
				});				
			};
			bus.getProxyBusObject(busName + "/About:sessionId=" + sessionId, getAboutData);
		}
		onError = function() {
			console.log("join error");
			/*
			 * TODO
			 */
		}
		
		var i;
		for(i=0; i<DEVICES.length; ++i) {
			if(DEVICES[i].busName == busName) {
				port = DEVICES[i].port;
				DEVICES.displayingIndex = i;
				break;
			}
		}
		if(DEVICES[i].sessionId != -1) {
			console.log("already joined");
			onJoined(DEVICES[i].sessionId);
		}
		else {
			that.joinForGetAbout(onJoined, onError, DEVICES[i]);
		}
	}
	
	var start = function() {		
        
		bus = new aj.BusAttachment();
        
        var registerAboutListener = function(err) {
        	console.log("register about listener");
        	if(err) {
        		alert("registerAboutListener failed [(" + err + ")]");
        	}        	
        	bus.registerAboutListener(ABOUT.announceListener, connect);
        }
        
        var connect = function(err) {
        	if(err) {
        		alert("connect failed [(" + err + ")]");
        	}
        	//bus.connect(addMatch);        	
        	bus.connect(whoImplements);
        };
        
        var whoImplements = function(err) {
        	if(err) {
        		alert("whoImplements failed [(" + err + ")]");
        	}
        	console.log("whoImplements");  
        	
        	bus.whoImplements(ABOUT.interfaces, lostAdvertisedName);        	
        }
        
        var addMatch = function(err) {
        	if(err) {
        		alert("addMatch failed [(" + err + ")]");
        	}
        	bus.addMatch("type='signal',sessionless='t'", lostAdvertisedName);
        }
        
        var lostAdvertisedName = function(err) {
        	if(err) {
        		alert("lostAdvertisedName failed [(" + err + ")]");
        	}
        	
        	bus.registerBusListener({
        		onLostAdvertisedName: function(name, transport, namePrefix) {
        			        			
        			for(var i=0; i<DEVICES.length; ++i) {
        				if(DEVICES[i].busName == name) {
        					DEVICES.splice(i, 1);
        					ondraw();
        					break;
        				}
        			}
        		}
        	}, findAdvertisedName);
        }
        
        var findAdvertisedName = function(err) {
        	if(err) {
        		alert("findAdvertisedName failed [(" + err + ")]");
        	}
        	bus.findAdvertisedName(":", function(err) {
				console.log("start done");						                
            });
        }
        
        console.log("create");
        bus.create(true, registerAboutListener);        
	};	
	
	
	that.start = function() {		
		navigator.requestPermission("org.alljoyn.bus", function() { start(); });
	}	
	
	that.cancelWhoImplements = function() {
		
    	bus.cancelWhoImplements(ABOUT.interfaces, function(err) {
    		if(err) {
    			console.log(err);
    			return;
    		}
    		console.log("cancelWhoImplements" + ABOUT.interfaces);
    	});
	}
	
	that.unregisterAboutListener = function() {
		bus.unregisterAboutListener(ABOUT.announceListener, function(err) {
			if(err) {
    			console.log(err);
    			return;
    		}
			console.log("unregisterAboutListener");
		});
	}
	
	that.unregisterAllAboutListeners = function() {
		bus.unregisterAllAboutListeners(function(err) {
			if(err) {
    			console.log(err);
    			return;
    		}
			console.log("unregisterAllAboutListeners");
		});
	}
	
	/* Joins an existing session. */
	that.joinForGetAbout = function(onjoined, onerror, device) {
        
        var onJoined = function(err, id, opts) {
            if (err) {
                alert("Join session failed [(" + err + ")]");
                onerror();
                return;
            }            
            //console.log(sessionId);
            device.sessionId = id;            
            
            onjoined(id);
        };

        bus.joinSession({        	
            host: device["busName"],
            port: device["port"],
            isMultipoint: true,
            /*
             * This method is called when the last remote participant session
             * leaves for some reason.
             */            
            onLost: function(id, reason) {
                console.log("session lost");
                var i=0;
                for(i=0; i<DEVICES.length; ++i) {
                	if(DEVICES[i].sessionId == id) {
                		DEVICES[i].sessionId = -1;
                		break;
                	}                	
                }
                if(DEVICES.displayingIndex == i)                	
                	showAnnouncedData("");
            }
        }, onJoined);
    };
    
	return that;
	
}());	