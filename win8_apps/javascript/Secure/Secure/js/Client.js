/*
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

// session Id of the currently joined session
var sessionId = 0;
// Instance of the client application object
var client = null;
// Context needed to response to a RequestCredentials query
var authContext = null;

function Client() {
    try {
        /* Create and connect all object and listeners of the service */
        this.busAtt = new AllJoyn.BusAttachment(APPLICATION_NAME, true, 4);
        OutputLine("BusAttachment was created.");

        this.busListener = new ClientBusListener(this.busAtt);
        this.busAtt.registerBusListener(this.busListener.busListener);
        OutputLine("Bus Listener was created and registered.");

        this.busAtt.start();

        // Enable authorization security. Must be specified b/t busAtt.start and busAtt.connect.
        // Keystore is specified as shared so multiple applications on the same device can access
        // the key store.
    
        this.busAtt.enablePeerSecurity("ALLJOYN_SRP_KEYX", this.busListener.authListener, "/.alljoyn_keystore/s_central.ks", true);
        if ($("KSCheckbox").checked) {
            this.busAtt.clearKeyStore();
        }

        /* Connect bus attachment to D-Bus and find well-known service name */
        this.busAtt.connectAsync(CONNECT_SPECS).then(function () {
            client.busAtt.findAdvertisedName(WELL_KNOWN_NAME);
        });
    } catch (err) {
        OutputLine("Couldn't successfully establish the client application.");
        client = null;
        $("startClient").innerHTML = "Start Client";
    }

    // Request to join a session with the discovered service
    this.joinSession = function joinSession() {
        var sessionOptIn = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES,
            false, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY)
        var sessionPortOut = new Array(1);
        this.busAtt.joinSessionAsync(WELL_KNOWN_NAME, SESSION_PORT, this.busListener.sessionListener, sessionOptIn, sessionPortOut, null).then(function (joinResult) {
            if (AllJoyn.QStatus.er_OK == joinResult.status) {
                OutputLine("Join Session was successful (SessionId=" + joinResult.sessionId + ")");
                sessionId = joinResult.sessionId;
                proxyRemoteConnect();
            } else {
                OutputLine("Join Session was unsuccessful.");
            }
        });
    }

    // Connect the proxy bus object to interface with the bus object of the service 
    function proxyRemoteConnect() {
        client.proxyBusObject = new ClientBusObject(client.busAtt, OBJECT_PATH);
        client.proxyBusObject.proxyBusObject.introspectRemoteObjectAsync(null).then(function (introResult) {
            if (AllJoyn.QStatus.er_OK == introResult.status) {
                OutputLine("Introspection of the service bus object was successful.");
                client.proxyBusObject.callPingMethod();
            } else {
                OutputLine("Introspection of the service bus object was unsuccessful.");
                client.busAtt.leaveSession(sessionId);
            }
        });
    }

    // Tear down the alljoyn bus Attachment 
    this.tearDown = function tearDown() {
        try {
            client.busAtt.disconnectAsync(CONNECT_SPECS).then(function () {
                client.busAtt.stopAsync();
            }).then(function () {
                OutputLine("The Client application has been closed.");
            });
        } catch (err) {
            OutputLine("Problems occurred while tearing down the client");
        }
    }
}

// During the authentication process the user should enter the pin and submit it 
// to the service by clicking the 'Send' button
$("sendBtn").onclick = function () {
    if (authContext != null) {
        var userInput = $("input").value;
        if (userInput != "") {
            var creds = new AllJoyn.Credentials();
            creds.expiration = 120;
            creds.password = userInput;
            client.busListener.authListener.requestCredentialsResponse(authContext, true, creds);
            $("sendBtn").disabled = "disabled";
            authContext = null;
        } else {
            OutputLine("You must enter a pin to submit authentication.");
        }
    }
}

// This is the object which acts as a proxy for the service's bus object and handles calls to 
// the 'Ping' method
function ClientBusObject(busAtt, busObjectPath) {

    this.proxyBusObject = new AllJoyn.ProxyBusObject(busAtt, WELL_KNOWN_NAME, OBJECT_PATH, sessionId);

    /* Calls the 'cat' method of the service with two string arguments */
    this.callPingMethod = function callPingMethod() {
        var callerMessage = "Hello from the Client!";
        var arg = new AllJoyn.MsgArg("s", new Array(callerMessage));

        var serviceInterface = this.proxyBusObject.getInterface(SECURE_INTERFACE_NAME);

        var pingMember = serviceInterface.getMember("Ping");
        this.proxyBusObject.methodCallAsync(pingMember, new Array(arg), null, 1000, 0).then(function (callResult) {
            // This executes when the service has received the method call and is replying with
            // its own message
            var message = callResult.message;
            if (message.type == AllJoyn.AllJoynMessageType.message_METHOD_RET) {
                var returnMsg = message.getArg(0).value;
                var sender = message.sender;

                OutputLine("//////////////////////////////////////////////////////////////////");
                OutputLine("'" + sender + "' return the value: '" + returnMsg + "'.");
                OutputLine("");
            } else {
                OutputLine("Authentication was unsuccessful or the 'Ping' method call returned errors.");
            }
            startClientClick(); //shut down the client
        });
        OutputLine("Called the 'Ping' method with value '" + callerMessage + "'.");
    }
}

//  This bus listener will handle bus and session events
function ClientBusListener(busAtt) {
    this.sessionListener = new AllJoyn.SessionListener(busAtt);
    this.busListener = new AllJoyn.BusListener(busAtt);
    this.authListener = new AllJoyn.AuthListener(busAtt);

    // Authentication mechanism requests user credentials which is a 6 digit pin generated by the 
    // service. If the user name is not an empty string the request is for credentials for that 
    // specific user. A count allows the listener to decide whether to allow or reject multiple 
    // authentication attempts to the same peer.
    this.AuthListenerRequestCredentials = function (event) {
        var authMechanism = GetEventArg(event, 0);
        var peerName = GetEventArg(event, 1);
        var authCount = GetEventArg(event, 2);
        var userName = GetEventArg(event, 3);
        var credMask = GetEventArg(event, 4);
        var context = GetEventArg(event, 5);

        // On first request of credentials a pin is generated, the user then has 3 attempts to
        // input the generated pin before they are blocked from authorization.
        if ("ALLJOYN_SRP_KEYX" == authMechanism && AllJoyn.CredentialType.cred_PASSWORD == credMask) {
            if (authCount <= 3) {
                OutputLine("Credentials for authorization have been requested using mechanism " + authMechanism + "(Attempt#=" + authCount + ")");
                OutputLine("Please enter the password below and click Send: ");
                authContext = context;
                $("sendBtn").disabled = "";
                return AllJoyn.QStatus.er_OK;
            }
        }
        return AllJoyn.QStatus.er_AUTH_FAIL;
    }
    // Reports successful or unsuccessful completion of authentication.
    this.AuthListenerAuthenticationComplete = function (event) {
        var authMechanism = GetEventArg(event, 0);
        var peerName = GetEventArg(event, 1);
        var success = GetEventArg(event, 2);

        if (success) {
            OutputLine("Authenification using " + authMechanism + " for " + peerName + " was a success.");
        } else {
            OutputLine("Authenification using " + authMechanism + " for " + peerName + " was unsuccessful.");
        }
    }

    // Called when a BusAttachment this listener is registered with is has become disconnected from the bus.
    this.BusListenerBusDisconnected = function () {
        var i = 0;
    }
    // Called when a BusAttachment this listener is registered with is stopping.
    this.BusListenerBusStopping = function () {
        var i = 0;
    }
    // Called by the bus when the listener is registered. This give the listener implementation the
    // opportunity to save a reference to the bus.
    this.BusListenerListenerRegistered = function (event) {
        var busAttachment = GetEventArg(event, 0);
    }
    //Called by the bus when the listener is unregistered.
    this.BusListenerListenerUnregistered = function () {
        var i = 0;
    }
    // Called by the bus when an external bus is discovered that is advertising a well-known name
    // that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
    this.BusListenerFoundAdvertisedName = function (event) {
        var name = GetEventArg(event, 0);
        var transport = GetEventArg(event, 1);
        var namePrefix = GetEventArg(event, 2);
        OutputLine("Found Advertised Name (wkn=" + name + " namePrefix=" + namePrefix + ")");
        client.joinSession();
    }
    // Called by the bus when an advertisement previously reported through FoundName has become unavailable.
    this.BusListenerLostAdvertisedName = function (event) {
        var name = GetEventArg(event, 0);
        var previousOwner = GetEventArg(event, 1);
        var newOwner = GetEventArg(event, 2);
        OutputLine("Lost Advertised Name (wkn=" + name + " prevOwner=" + prevOwner + " newOwner=" + newOwner + ")");
    }
    // Called by the bus when the ownership of any well-known name changes.
    this.BusListenerNameOwnerChanged = function (event) {
        var name = GetEventArg(event, 0);
        var prevOwner = GetEventArg(event, 1);
        var newOwner = GetEventArg(event, 2);
        OutputLine("Name Owner Changed (wkn=" + name + " prevOwner=" + prevOwner + " newOwner=" + newOwner + ")");
    }

    // Called by the bus when an existing session becomes disconnected.
    this.SessionListenerSessionLost = function (event) {
        var sessionId = GetEventArg(event, 0);
        OutputLine("Session Lost (sessionId=" + sessionId + ")");
    }
    // Called by the bus when a member of a multipoint session is added.
    this.SessionListenerSessionMemberAdded = function (event) {
        var sessionId = GetEventArg(event, 0);
        var uniqueName = GetEventArg(event, 1);
    }
    // Called by the bus when a member of a multipoint session is removed.
    this.SessionListenerSessionMemberRemoved = function (event) {
        var sessionId = GetEventArg(event, 0);
        var uniqueName = GetEventArg(event, 1);
    }

    // Add the event listeners and handlers to each of the listener objects
    this.authListener.addEventListener('requestcredentials', this.AuthListenerRequestCredentials);
    this.authListener.addEventListener('authenticationcomplete', this.AuthListenerAuthenticationComplete);

    this.busListener.addEventListener('busdisconnected', this.BusListenerBusDisconnected);
    this.busListener.addEventListener('busstopping', this.BusListenerBusStopping);
    this.busListener.addEventListener('listenerregistered', this.BusListenerListenerRegistered);
    this.busListener.addEventListener('listenerunregistered', this.BusListenerListenerUnregistered);
    this.busListener.addEventListener('foundadvertisedname', this.BusListenerFoundAdvertisedName);
    this.busListener.addEventListener('lostadvertisedname', this.BusListenerLostAdvertisedName);
    this.busListener.addEventListener('nameownerchanged', this.BusListenerNameOwnerChanged);

    this.sessionListener.addEventListener('sessionlost', this.SessionListenerSessionLost);
    this.sessionListener.addEventListener('sessionmemberadded', this.SessionListenerSessionMemberAdded);
    this.sessionListener.addEventListener('sessionmemberremoved', this.SessionListenerSessionMemberRemoved);
}