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

// Instance of the service application object
var service = null;

// Hook the application suspending/resuming events to the handlers so that exclusively-held resource (eg., network port) is release and other applications are allowed to acquire the resource
Windows.UI.WebUI.WebUIApplication.addEventListener("suspending", suspendingHandler);
function suspendingHandler(eventArgs) {
    try {
        if (service && service.busAtt && service.busAtt.onAppSuspend != undefined) {
            service.busAtt.onAppSuspend();
        }
    } catch (err) {
        console.log(err.stack);
    }
}

Windows.UI.WebUI.WebUIApplication.addEventListener("resuming", resumingHandler);
function resumingHandler() {
    try {
        if (service && service.busAtt && service.busAtt.onAppResume != undefined) {
            service.busAtt.onAppResume();
        }
    } catch (err) {
        console.log(err.stack);
    }
}

// Service object which establishes the service by advertising a service for clients to join. Once a
// join has occurred clients can call the 'Ping' method which intiates the authentication process. If 
// the device has already done authentication the KS may already contain authentication.
function Service() {
    try {
        OutputLine("Creating and registering the alljoyn bus, listeners and objects...");

        /* Create and connect all object and listeners of the service */
        this.busAtt = new AllJoyn.BusAttachment(APPLICATION_NAME, true, 4);

        this.busListener = new ServiceBusListener(this.busAtt);
        this.busAtt.registerBusListener(this.busListener.busListener);

        this.busObject = new ServiceBusObject(this.busAtt, OBJECT_PATH, false);
        this.busAtt.registerBusObject(this.busObject.busObject);

        this.busAtt.start();

        // Enable authorization security. Must be specified b/t busAtt.start and busAtt.connect.
        // Keystore is specified as shared so multiple applications on the same device can access
        // the key store.
        this.busAtt.enablePeerSecurity("ALLJOYN_SRP_KEYX", this.busListener.authListener, "/.alljoyn_keystore/s_central.ks", true);

        // Connect bus attachment to the Bus 
        this.busAtt.connectAsync(CONNECT_SPECS).then(advertiseName);
        OutputLine("Connecting to AllJoyn....");
    } catch (err) {
        OutputLine("Couldn't establish the service application.");
        service = null;
        $("startServer").innerHTML = "Start Server";
    }

    // Advertise the service for clients to join a session with by:
    // 1. Binding Session Port  2. Requesting well-known name  3. Advertising well-known name
    function advertiseName() {
        OutputLine("Bus Attachment connected to " + CONNECT_SPECS + " successfully.");
        try {
            // Make a SessionPort available for external BusAttachments to join. 
            var sessionOptIn = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES,
                false, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY)
            var sessionPortOut = new Array(1);

            service.busAtt.bindSessionPort(SESSION_PORT, sessionPortOut, sessionOptIn, service.busListener.sessionPortListener);
            OutputLine("Session Port was bound.");

            // Request a well-known name from the bus... 
            service.busAtt.requestName(WELL_KNOWN_NAME, AllJoyn.RequestNameType.dbus_NAME_DO_NOT_QUEUE);
            OutputLine("Obtained the well-known name: " + WELL_KNOWN_NAME + ".");

            // Advertise the well-known name of the service so client can discover 
            service.busAtt.advertiseName(WELL_KNOWN_NAME, AllJoyn.TransportMaskType.transport_ANY);
            OutputLine("Currently advertising the well-known name: " + WELL_KNOWN_NAME);
        } catch (err) {
            OutputLine("The was a problem advertising the well-known service name.");
            OutputLine(err.message);
            service.tearDown();
            service = null;
            $("startServer").innerHTML = "Start Server";
        }
    }

    // Break down the service by reversing order of initialization:
    // 1) Cancel Advertising WKN    2) Release WKN  3) Unbind Session Port  4) Diconnect and stop busAtt
    this.tearDown = function tearDown() {
        try {
            this.busAtt.disconnectAsync(CONNECT_SPECS).then(function () {
                this.busAtt.stopAsync();
            }).then(function () {
                OutputLine("The Server has been terminated.");
            });
        } catch (err) {
            OutputLine("Problem closing the application:");
            OutputLine(err.message);
        }
    }

}

//  Basic Service Object that will advertise the well-known interface and provide the 
//  interface to the client app
function ServiceBusObject(busAtt, busObjectPath) {
    this.busObject = new AllJoyn.BusObject(busAtt, busObjectPath, false);

    try {
        /* Implement the service's 'Ping' interface */
        var interfaceDescript = new Array(1);
        busAtt.createInterface(SECURE_INTERFACE_NAME, interfaceDescript, true);
        interfaceDescript[0].addMethod("Ping", "s", "s", "inStr,outStr", 0, "");
        interfaceDescript[0].activate();
        this.busObject.addInterface(interfaceDescript[0]);

        this.messageReceiver = new AllJoyn.MessageReceiver(busAtt);

        // Ping method handler called by client once authorization has completed with string
        // message which is sent directly back to user in reply
        this.Ping = function Ping(event) {
            var member = GetEventArg(event, 0);
            var message = GetEventArg(event, 1);
            var msgValue = message.getArg(0).value;

            OutputLine("");
            OutputLine("______-----------############Ping############-----------______");
            OutputLine(message.sender + " says:");
            OutputLine(msgValue);

            OutputLine("Replying to the sender with '" + msgValue + "'.");
            OutputLine("");

            var retMessageArg = new AllJoyn.MsgArg("s", new Array(msgValue));
            try {
                service.busObject.busObject.methodReply(message, new Array(retMessageArg));
            } catch (err) {
                OutputLine("Method Reply was unsuccessful.");
            }
        }

        // register event and method handles
        this.messageReceiver.addEventListener('methodhandler', this.Ping);
        this.busObject.addMethodHandler(interfaceDescript[0].getMember("Ping"), this.messageReceiver);

        OutputLine("Created Interface and registered 'Ping' method handler.");
    } catch (err) {
        OutputLine("There was a problem creating the interface and registering 'Ping' method handler.");
        OutputLine(err);
    }
}

//  This bus listener will handle bus and session events
function ServiceBusListener(busAtt) {
    this.sessionListener = new AllJoyn.SessionListener(busAtt);
    this.busListener = new AllJoyn.BusListener(busAtt);
    this.authListener = new AllJoyn.AuthListener(busAtt);
    try {
        this.sessionPortListener = new AllJoyn.SessionPortListener(busAtt);
    } catch (err) {
        var i = 0;
    }
    spl = this.sessionPortListener;

    // Authentication mechanism requests user credentials which is generated here by the service for
    // the client to input. If the user name is not an empty string the request is for credentials for 
    // that specific user. A count allows the listener to decide whether to allow or reject multiple 
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
        if ("ALLJOYN_SRP_KEYX" == authMechanism && AllJoyn.CredentialType.cred_PASSWORD & credMask) {
            if (authCount <= 3) {
                OutputLine("Requesting Credentials for authorizing " + peerName + " using mechanism " + authMechanism + "(Attempt#=" + authCount + ")");
                var pin = Math.floor(Math.random() * 999999);
                if (pin < 100000) {
                    pin += 100000;
                }
                OutputLine("One time pin : " + pin);
                var creds = new AllJoyn.Credentials();
                creds.expiration = 120;
                creds.password = pin;
                service.busListener.authListener.requestCredentialsResponse(context, true, creds);
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
            OutputLine("Authentication using " + authMechanism + " for " + peerName + " was a success.");
        } else {
            OutputLine("Authentication using " + authMechanism + " for " + peerName + " was a unsuccess.");
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
    }
    // Called by the bus when an advertisement previously reported through FoundName has become unavailable.
    this.BusListenerLostAdvertisedName = function (event) {
        var name = GetEventArg(event, 0);
        var previousOwner = GetEventArg(event, 1);
        var newOwner = GetEventArg(event, 2);
    }
    // Called by the bus when the ownership of any well-known name changes.
    this.BusListenerNameOwnerChanged = function (event) {
        var name = GetEventArg(event, 0);
        var prevOwner = GetEventArg(event, 1);
        var newOwner = GetEventArg(event, 2);
        OutputLine("Name Owner Changed (wkn=" + name + " prevOwner=" + prevOwner + " newOwner=" + newOwner + ")");
    }

    // Called by the bus when a session has been successfully joined. The session is now fully up.
    this.SessionPortListenerSessionJoined = function (event) {
        var sessionPort = GetEventArg(event, 0);
        var sessionId = GetEventArg(event, 1);
        var joiner = GetEventArg(event, 2);
        OutputLine("A session has been established with: " + joiner + " (sessionId=" + sessionId + ").");
    }
    // Accept or reject an incoming JoinSession request. The session does not exist until
    // after this function returns.
    this.SessionPortListenerAcceptSessionJoiner = function (event) {
        var sessionPort = GetEventArg(event, 0);
        var sessionId = GetEventArg(event, 1);
        var joiner = GetEventArg(event, 2);
        if (sessionPort == SESSION_PORT) {
            OutputLine("Accepting Join Session request from 'joiner' (sessionId=" + sessionId + ")");
            return true;
        } else {
            OutputLine("Rejecting Join Session request from 'joiner'.");
            return false;
        }
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

    this.sessionPortListener.addEventListener('acceptsessionjoiner', this.SessionPortListenerAcceptSessionJoiner);
    this.sessionPortListener.addEventListener("sessionjoined", this.SessionPortListenerSessionJoined);

    this.sessionListener.addEventListener('sessionlost', this.SessionListenerSessionLost);
    this.sessionListener.addEventListener('sessionmemberadded', this.SessionListenerSessionMemberAdded);
    this.sessionListener.addEventListener('sessionmemberremoved', this.SessionListenerSessionMemberRemoved);
}