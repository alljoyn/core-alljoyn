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

//  Connection specifications for the bus attachment
var CONNECT_SPECS = "null:";

var APPLICATION_NAME = "basicClient";

//  well-known service name prefix
var WELL_KNOWN_NAME = "org.alljoyn.Bus.sample";

//  Interface name for the 'cat' method
var BASIC_SERVICE_INTERFACE_NAME = "org.alljoyn.Bus.sample";

//  Relative path for the service 
var OBJECT_PATH = "/sample";

//  Session Port Num for client-service communication
var SESSION_PORT = 25;

//  Basic Service Object that will advertise the well-known interface and provide the 
//  interface to the client app, and provide functionality for the interface
function BasicClientBusObject(busAtt, busObjectPath) {

    this.proxyBusObject = new AllJoyn.ProxyBusObject(busAtt, WELL_KNOWN_NAME, OBJECT_PATH, sessionId);

    /* Calls the 'cat' method of the service with two string arguments */
    this.callCatMethod = function callCatMethod() {
        if (proxyBusObject.proxyBusObject.implementsInterface(BASIC_SERVICE_INTERFACE_NAME)) {
            var arg1 = new AllJoyn.MsgArg("s", new Array("Hello "));
            var arg2 = new AllJoyn.MsgArg("s", new Array("World!"));

            var serviceInterface = proxyBusObject.proxyBusObject.getInterface(BASIC_SERVICE_INTERFACE_NAME);
            var catMember = serviceInterface.getMember("cat");

            this.proxyBusObject.methodCallAsync(catMember, new Array(arg1, arg2), null, 1000, 0).then(function (callResults) {
                /* This is called when the 'cat' method call returns a value. Value is output to the 
                    user, then the client is terminated */
                if (callResults.message.type == AllJoyn.AllJoynMessageType.message_METHOD_RET) {
                    var message = callResults.message;
                    var returnMsg = message.getArg(0).value;
                    var sender = message.sender;

                    OutputLine("//////////////////////////////////////////////////////////////////");
                    OutputLine("The 'cat' method return the value: '" + returnMsg + "' from sender '" + sender + "'.");
                    OutputLine("");
                } else {
                    OutputLine("The 'cat' method call was unsuccessful, returned an error.\n");
                    var errType = callResults.message.type;
                }

                alljoyn.tearDown();
            });
            OutputLine("Called the 'cat' method with the arguments 'Hello ' and 'World!'.");
        }
    }
}

//  This bus listener will handle bus and session events
function MyBusListener(busAtt) {
    this.sessionListener = new AllJoyn.SessionListener(busAtt);
    this.busListener = new AllJoyn.BusListener(busAtt);

    // Called when a BusAttachment this listener is registered with is has become disconnected from the bus.
    this.BusListenerBusDisconnected = function () {
    }
    // Called when a BusAttachment this listener is registered with is stopping.
    this.BusListenerBusStopping = function () {
    }
    // Called by the bus when the listener is registered.
    this.BusListenerListenerRegistered = function (event) {
    }
    // Called by the bus when the listener is unregistered
    this.BusListenerListenerUnregistered = function () {
    }
    // Called by the bus when an external bus is discovered that is advertising a well-known name
    // that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
    this.BusListenerFoundAdvertisedName = function (event) {
        var name = GetEventArg(event, 0);
        var transport = GetEventArg(event, 1);
        var namePrefix = GetEventArg(event, 2);
        
        OutputLine("Found Advertised well-known name (name=" + name + " transport=" +
            transport + " prefix=" + namePrefix + ")");

        alljoyn.joinSession();
    }
    // Called by the bus when an advertisement previously reported through FoundName has become unavailable.
    this.BusListenerLostAdvertisedName = function (event) {
        var name = GetEventArg(event, 0);
        var transport = GetEventArg(event, 1);
        var namePrefix = GetEventArg(event, 2);
    }
    // Called by the bus when the ownership of any well-known name changes.
    this.BusListenerNameOwnerChanged = function (event) {
        var name = GetEventArg(event, 0);
        var previousOwner = GetEventArg(event, 1);
        var newOwner = GetEventArg(event, 2);
    };

    // Called by the bus when an existing session becomes disconnected.
    this.SessionListenerSessionLost = function (event) {
        var sessionId = GetEventArg(event, 0);
    };
    // Called by the bus when a member of a multipoint session is added.
    this.SessionListenerSessionMemberAdded = function (event) {
        var sessionId = GetEventArg(event, 0);
        var uniqueName = GetEventArg(event, 1);
    };
    // Called by the bus when a member of a multipoint session is removed.
    this.SessionListenerSessionMemberRemoved = function (event) {
        var sessionId = GetEventArg(event, 0);
        var uniqueName = GetEventArg(event, 1);
    };

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